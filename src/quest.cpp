#include "quest.h"
#ifdef SCU_QUEST
#include "ui.h"
#include <SDL_system.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cfloat>
#include <stdexcept>
#include <string>
#include <vector>

namespace scu {
namespace {
constexpr float pi = 3.14159265358979323846f;
XrVector3f add(XrVector3f a, XrVector3f b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
XrVector3f sub(XrVector3f a, XrVector3f b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
XrVector3f mul(XrVector3f a, float n) {
    return {a.x * n, a.y * n, a.z * n};
}
float dot(XrVector3f a, XrVector3f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float length(XrVector3f a) {
    return std::sqrt(dot(a, a));
}
XrQuaternionf inverse(XrQuaternionf q) {
    return {-q.x, -q.y, -q.z, q.w};
}
XrQuaternionf product(XrQuaternionf a, XrQuaternionf b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
XrVector3f rotate(XrQuaternionf q, XrVector3f v) {
    auto r = product(product(q, {v.x, v.y, v.z, 0}), inverse(q));
    return {r.x, r.y, r.z};
}
XrPosef identity() {
    return {{0, 0, 0, 1}, {0, 0, 0}};
}
XrPosef transform(XrPosef a, XrPosef b) {
    return {product(a.orientation, b.orientation),
            add(a.position, rotate(a.orientation, b.position))};
}
void check(XrResult result, const char *operation) {
    if (XR_FAILED(result))
        throw std::runtime_error(std::string(operation) + " (OpenXR " + std::to_string(result) +
                                 ")");
}
bool valid(XrSpaceLocation location) {
    constexpr XrSpaceLocationFlags mask =
        XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    return (location.locationFlags & mask) == mask;
}
} // namespace

struct QuestPresentation::Impl {
    App &app;
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId system = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace local = XR_NULL_HANDLE, view = XR_NULL_HANDLE;
    XrSwapchain swapchain = XR_NULL_HANDLE;
    XrActionSet actions = XR_NULL_HANDLE;
    XrAction stick = XR_NULL_HANDLE, trigger = XR_NULL_HANDLE, squeeze = XR_NULL_HANDLE;
    XrAction buttonA = XR_NULL_HANDLE, buttonB = XR_NULL_HANDLE, buttonX = XR_NULL_HANDLE,
             buttonY = XR_NULL_HANDLE;
    XrAction thumb = XR_NULL_HANDLE, aim = XR_NULL_HANDLE, grip = XR_NULL_HANDLE,
             haptic = XR_NULL_HANDLE;
    std::array<XrPath, 2> hands{};
    std::array<XrSpace, 2> aimSpaces{}, gripSpaces{};
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    XrFrameState frame{XR_TYPE_FRAME_STATE};
    XrSessionState state = XR_SESSION_STATE_UNKNOWN;
    bool running = false, frameOpen = false, needRecenter = true, haveHead = false;
    bool passthroughSupported = false, passthroughActive = false, previousX = false,
         previousThumb = false, previousClick = false;
    bool grabbing = false, pointerHit = false, pointerClick = false, previousB = false,
         bForMenu = false;
    bool lastMenu = true, blockPointerTrigger = false, previousY = false;
    float pointerX = 0, pointerY = 0, wheel = 0;
    XrPosef head = identity(), panel = identity(), grabPanel = identity();
    XrVector3f grabMid{};
    float grabSeparation = 1, grabWidth = 3.2f;
    int width = 0, height = 0, lastResolution = -1;
    std::array<int, 4> lastPlacement{{0, 0, 0, 0}};
    SDL_Texture *canvas = nullptr;
    GLuint framebuffer = 0;
    SDL_Joystick *joystick = nullptr;
    jobject activity = nullptr;
    JavaVM *vm = nullptr;
    XrPassthroughFB passthrough = XR_NULL_HANDLE;
    XrPassthroughLayerFB passthroughLayer = XR_NULL_HANDLE;
    PFN_xrCreatePassthroughFB createPassthrough = nullptr;
    PFN_xrDestroyPassthroughFB destroyPassthrough = nullptr;
    PFN_xrPassthroughStartFB startPassthrough = nullptr;
    PFN_xrPassthroughPauseFB pausePassthrough = nullptr;
    PFN_xrCreatePassthroughLayerFB createPassthroughLayer = nullptr;
    PFN_xrDestroyPassthroughLayerFB destroyPassthroughLayer = nullptr;
    PFN_xrPassthroughLayerResumeFB resumePassthroughLayer = nullptr;
    PFN_xrPassthroughLayerPauseFB pausePassthroughLayer = nullptr;

    explicit Impl(App &a) : app(a) {}
    XrPath path(const char *name) {
        XrPath out = XR_NULL_PATH;
        check(xrStringToPath(instance, name, &out), "Name input path");
        return out;
    }
    template <class T> void function(const char *name, T &out) {
        check(xrGetInstanceProcAddr(instance, name, reinterpret_cast<PFN_xrVoidFunction *>(&out)),
              name);
    }
    void initialise() {
        auto env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
        jobject object = static_cast<jobject>(SDL_AndroidGetActivity());
        if (!env || !object)
            throw std::runtime_error("Android activity is unavailable");
        env->GetJavaVM(&vm);
        activity = env->NewGlobalRef(object);
        env->DeleteLocalRef(object);
        PFN_xrInitializeLoaderKHR initialiseLoader = nullptr;
        check(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                    reinterpret_cast<PFN_xrVoidFunction *>(&initialiseLoader)),
              "Find Android OpenXR loader");
        XrLoaderInitInfoAndroidKHR loader{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loader.applicationVM = vm;
        loader.applicationContext = activity;
        check(initialiseLoader(reinterpret_cast<XrLoaderInitInfoBaseHeaderKHR *>(&loader)),
              "Initialize Android OpenXR loader");
        uint32_t count = 0;
        check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr),
              "Read OpenXR extensions");
        std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
        check(xrEnumerateInstanceExtensionProperties(nullptr, count, &count, extensions.data()),
              "Read OpenXR extensions");
        auto supports = [&](const char *name) {
            return std::any_of(extensions.begin(), extensions.end(),
                               [&](const auto &e) { return !std::strcmp(e.extensionName, name); });
        };
        if (!supports(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME) ||
            !supports(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME))
            throw std::runtime_error(
                "This headset does not expose the Android OpenXR graphics interface");
        std::vector<const char *> enabled = {XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
                                             XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
        passthroughSupported = supports(XR_FB_PASSTHROUGH_EXTENSION_NAME);
        if (passthroughSupported)
            enabled.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
        XrInstanceCreateInfoAndroidKHR android{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
        android.applicationVM = vm;
        android.applicationActivity = activity;
        XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
        info.next = &android;
        std::strcpy(info.applicationInfo.applicationName, "Sonic Compilation Ultra XR");
        info.applicationInfo.applicationVersion = 400;
        std::strcpy(info.applicationInfo.engineName, "SCU / Genesis Plus GX");
        info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
        info.enabledExtensionCount = uint32_t(enabled.size());
        info.enabledExtensionNames = enabled.data();
        check(xrCreateInstance(&info, &instance), "Create OpenXR instance");
        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        check(xrGetSystem(instance, &systemInfo, &system), "Find Quest headset");
        PFN_xrGetOpenGLESGraphicsRequirementsKHR requirementsFunction = nullptr;
        function("xrGetOpenGLESGraphicsRequirementsKHR", requirementsFunction);
        XrGraphicsRequirementsOpenGLESKHR requirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        check(requirementsFunction(instance, system, &requirements),
              "Read headset graphics requirements");
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        if (XR_MAKE_VERSION(major, minor, 0) < requirements.minApiVersionSupported)
            throw std::runtime_error(
                "The active OpenGL ES context is below the headset requirement");
        XrGraphicsBindingOpenGLESAndroidKHR graphics{
            XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR};
        graphics.display = eglGetCurrentDisplay();
        graphics.context = eglGetCurrentContext();
        EGLint configId = 0, found = 0;
        eglQueryContext(graphics.display, graphics.context, EGL_CONFIG_ID, &configId);
        EGLint attributes[] = {EGL_CONFIG_ID, configId, EGL_NONE};
        if (!eglChooseConfig(graphics.display, attributes, &graphics.config, 1, &found) || !found)
            throw std::runtime_error("Cannot identify the SDL OpenGL ES configuration");
        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.next = &graphics;
        sessionInfo.systemId = system;
        check(xrCreateSession(instance, &sessionInfo, &session), "Create Quest session");
        XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceInfo.poseInReferenceSpace = identity();
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        check(xrCreateReferenceSpace(session, &spaceInfo, &local), "Create room reference");
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        check(xrCreateReferenceSpace(session, &spaceInfo, &view), "Create head reference");
        createActions();
        if (passthroughSupported) {
            function("xrCreatePassthroughFB", createPassthrough);
            function("xrDestroyPassthroughFB", destroyPassthrough);
            function("xrPassthroughStartFB", startPassthrough);
            function("xrPassthroughPauseFB", pausePassthrough);
            function("xrCreatePassthroughLayerFB", createPassthroughLayer);
            function("xrDestroyPassthroughLayerFB", destroyPassthroughLayer);
            function("xrPassthroughLayerResumeFB", resumePassthroughLayer);
            function("xrPassthroughLayerPauseFB", pausePassthroughLayer);
            XrPassthroughCreateInfoFB create{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
            if (XR_SUCCEEDED(createPassthrough(session, &create, &passthrough))) {
                XrPassthroughLayerCreateInfoFB layer{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
                layer.passthrough = passthrough;
                layer.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
                if (XR_FAILED(createPassthroughLayer(session, &layer, &passthroughLayer)))
                    passthroughSupported = false;
            } else
                passthroughSupported = false;
        }
        glGenFramebuffers(1, &framebuffer);
        resize();
    }
    void createActions() {
        hands = {path("/user/hand/left"), path("/user/hand/right")};
        XrActionSetCreateInfo set{XR_TYPE_ACTION_SET_CREATE_INFO};
        std::strcpy(set.actionSetName, "collection");
        std::strcpy(set.localizedActionSetName, "Sonic Collection");
        check(xrCreateActionSet(instance, &set, &actions), "Create Quest actions");
        auto action = [&](const char *name, XrActionType type, bool both) {
            XrActionCreateInfo create{XR_TYPE_ACTION_CREATE_INFO};
            std::strcpy(create.actionName, name);
            std::strcpy(create.localizedActionName, name);
            create.actionType = type;
            create.countSubactionPaths = both ? 2 : 0;
            create.subactionPaths = both ? hands.data() : nullptr;
            XrAction value = XR_NULL_HANDLE;
            check(xrCreateAction(actions, &create, &value), name);
            return value;
        };
        stick = action("stick", XR_ACTION_TYPE_VECTOR2F_INPUT, true);
        trigger = action("trigger", XR_ACTION_TYPE_FLOAT_INPUT, true);
        squeeze = action("grip", XR_ACTION_TYPE_FLOAT_INPUT, true);
        buttonA = action("confirm_jump", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
        buttonB = action("back_start", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
        buttonX = action("guide_overview", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
        buttonY = action("options", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
        thumb = action("recenter", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
        aim = action("pointer", XR_ACTION_TYPE_POSE_INPUT, true);
        grip = action("hand_pose", XR_ACTION_TYPE_POSE_INPUT, true);
        haptic = action("feedback", XR_ACTION_TYPE_VIBRATION_OUTPUT, true);
        std::vector<XrActionSuggestedBinding> bindings;
        for (int i = 0; i < 2; ++i) {
            std::string hand = i ? "/user/hand/right" : "/user/hand/left";
            for (auto pair : {std::pair<XrAction, const char *>{stick, "/input/thumbstick"},
                              {trigger, "/input/trigger/value"},
                              {squeeze, "/input/squeeze/value"},
                              {aim, "/input/aim/pose"},
                              {grip, "/input/grip/pose"},
                              {haptic, "/output/haptic"}})
                bindings.push_back({pair.first, path((hand + pair.second).c_str())});
        }
        for (auto pair :
             {std::pair<XrAction, const char *>{buttonA, "/user/hand/right/input/a/click"},
              {buttonB, "/user/hand/right/input/b/click"},
              {buttonX, "/user/hand/left/input/x/click"},
              {buttonY, "/user/hand/left/input/y/click"},
              {thumb, "/user/hand/right/input/thumbstick/click"}})
            bindings.push_back({pair.first, path(pair.second)});
        XrInteractionProfileSuggestedBinding profile{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        profile.interactionProfile = path("/interaction_profiles/oculus/touch_controller");
        profile.countSuggestedBindings = uint32_t(bindings.size());
        profile.suggestedBindings = bindings.data();
        check(xrSuggestInteractionProfileBindings(instance, &profile), "Bind Quest Touch controls");
        XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attach.countActionSets = 1;
        attach.actionSets = &actions;
        check(xrAttachSessionActionSets(session, &attach), "Attach Quest controls");
        for (int i = 0; i < 2; ++i) {
            XrActionSpaceCreateInfo space{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            space.poseInActionSpace = identity();
            space.subactionPath = hands[i];
            space.action = aim;
            check(xrCreateActionSpace(session, &space, &aimSpaces[i]), "Create controller pointer");
            space.action = grip;
            check(xrCreateActionSpace(session, &space, &gripSpaces[i]), "Create controller grip");
        }
    }
    void detachController() {
        if (!joystick)
            return;
        const auto instance = SDL_JoystickInstanceID(joystick);
        SDL_JoystickClose(joystick);
        joystick = nullptr;
        // SDL device indices move when another controller is unplugged.
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
            if (SDL_JoystickGetDeviceInstanceID(i) == instance) {
                SDL_JoystickDetachVirtual(i);
                break;
            }
    }
    void controller(bool enabled) {
        if (enabled && !joystick) {
            SDL_VirtualJoystickDesc desc{};
            desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
            desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
            desc.naxes = SDL_CONTROLLER_AXIS_MAX;
            desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
            desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX) - 1;
            desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX) - 1;
            desc.name = "SCU Quest Touch";
            const int virtualIndex = SDL_JoystickAttachVirtualEx(&desc);
            if (virtualIndex < 0)
                throw std::runtime_error(SDL_GetError());
            joystick = SDL_JoystickOpen(virtualIndex);
            if (!joystick)
                throw std::runtime_error(SDL_GetError());
            SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
            SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
            app.refreshControllers();
        } else if (!enabled && joystick) {
            detachController();
            app.refreshControllers();
        }
    }
    void resize() {
        int resolution = std::clamp(app.settings.questResolution, 0, 3);
        if (resolution == lastResolution)
            return;
        if (canvas) {
            SDL_SetRenderTarget(app.renderer, nullptr);
            SDL_DestroyTexture(canvas);
            canvas = nullptr;
        }
        if (swapchain) {
            xrDestroySwapchain(swapchain);
            swapchain = XR_NULL_HANDLE;
        }
        const std::array<std::pair<int, int>, 4> sizes{
            {{1280, 720}, {1920, 1080}, {2560, 1440}, {3072, 576}}};
        width = sizes[resolution].first;
        height = sizes[resolution].second;
        XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
        check(xrGetSystemProperties(instance, system, &properties), "Read headset limits");
        float scale =
            std::min({1.f, float(properties.graphicsProperties.maxSwapchainImageWidth) / width,
                      float(properties.graphicsProperties.maxSwapchainImageHeight) / height});
        width = std::max(320, int(width * scale));
        height = std::max(180, int(height * scale));
        uint32_t count = 0;
        check(xrEnumerateSwapchainFormats(session, 0, &count, nullptr),
              "Read screen texture formats");
        std::vector<int64_t> formats(count);
        check(xrEnumerateSwapchainFormats(session, count, &count, formats.data()),
              "Read screen texture formats");
        int64_t format = GL_RGBA8;
        if (std::find(formats.begin(), formats.end(), format) == formats.end()) {
            format = GL_SRGB8_ALPHA8;
            if (std::find(formats.begin(), formats.end(), format) == formats.end())
                throw std::runtime_error("The headset has no supported RGBA screen format");
        }
        XrSwapchainCreateInfo create{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        create.usageFlags =
            XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        create.format = format;
        create.sampleCount = 1;
        create.width = width;
        create.height = height;
        create.faceCount = 1;
        create.arraySize = 1;
        create.mipCount = 1;
        check(xrCreateSwapchain(session, &create, &swapchain), "Create spatial screen");
        check(xrEnumerateSwapchainImages(swapchain, 0, &count, nullptr), "Read screen images");
        images.assign(count, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        check(xrEnumerateSwapchainImages(
                  swapchain, count, &count,
                  reinterpret_cast<XrSwapchainImageBaseHeader *>(images.data())),
              "Read screen images");
        canvas = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                   width, height);
        if (!canvas)
            throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_NONE);
        lastResolution = resolution;
    }
    XrSpaceLocation locate(XrSpace space) {
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        if (XR_FAILED(xrLocateSpace(space, local, frame.predictedDisplayTime, &location)))
            location.locationFlags = 0;
        return location;
    }
    XrPosef panelPose() const {
        if (!app.settings.questAnchor)
            return panel;
        float angle = -std::clamp(app.settings.questTilt, -60, 80) * pi / 180;
        XrPosef offset{{std::sin(angle / 2), 0, 0, std::cos(angle / 2)},
                       {0, app.settings.questHeight * .01f, -app.settings.questDistance * .01f}};
        return transform(head, offset);
    }
    void place() {
        if (!haveHead)
            return;
        auto forward = rotate(head.orientation, {0, 0, -1});
        float yaw = std::atan2(-forward.x, -forward.z);
        XrQuaternionf orientation{0, std::sin(yaw / 2), 0, std::cos(yaw / 2)};
        float tilt = -std::clamp(app.settings.questTilt, -60, 80) * pi / 180;
        panel.orientation = product(orientation, {std::sin(tilt / 2), 0, 0, std::cos(tilt / 2)});
        panel.position =
            add(head.position, rotate(orientation, {0, app.settings.questHeight * .01f,
                                                    -app.settings.questDistance * .01f}));
        needRecenter = false;
    }
    void events() {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        while (xrPollEvent(instance, &event) == XR_SUCCESS) {
            if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                state = reinterpret_cast<XrEventDataSessionStateChanged *>(&event)->state;
                if (state == XR_SESSION_STATE_READY && !running) {
                    XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};
                    begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    check(xrBeginSession(session, &begin), "Begin headset session");
                    running = true;
                    needRecenter = true;
                } else if (state == XR_SESSION_STATE_STOPPING && running) {
                    check(xrEndSession(session), "Pause headset session");
                    running = false;
                } else if (state == XR_SESSION_STATE_EXITING ||
                           state == XR_SESSION_STATE_LOSS_PENDING) {
                    app.running = false;
                    running = false;
                }
            } else if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
                app.running = false;
            else if (event.type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING)
                needRecenter = true;
            event = {XR_TYPE_EVENT_DATA_BUFFER};
        }
    }
    float scalar(XrAction action, int hand) {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = action;
        get.subactionPath = hands[hand];
        XrActionStateFloat value{XR_TYPE_ACTION_STATE_FLOAT};
        return XR_SUCCEEDED(xrGetActionStateFloat(session, &get, &value)) && value.isActive
                   ? value.currentState
                   : 0;
    }
    bool boolean(XrAction action) {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = action;
        XrActionStateBoolean value{XR_TYPE_ACTION_STATE_BOOLEAN};
        return XR_SUCCEEDED(xrGetActionStateBoolean(session, &get, &value)) && value.isActive &&
               value.currentState;
    }
    XrVector2f axes(int hand) {
        XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};
        get.action = stick;
        get.subactionPath = hands[hand];
        XrActionStateVector2f value{XR_TYPE_ACTION_STATE_VECTOR2F};
        return XR_SUCCEEDED(xrGetActionStateVector2f(session, &get, &value)) && value.isActive
                   ? value.currentState
                   : XrVector2f{};
    }
    void feedback() {
        if (!app.settings.questHaptics)
            return;
        XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};
        info.action = haptic;
        info.subactionPath = hands[1];
        XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
        vibration.duration = 20000000;
        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
        vibration.amplitude = std::clamp(app.settings.questHaptics, 0, 100) / 100.f;
        xrApplyHapticFeedback(session, &info, reinterpret_cast<XrHapticBaseHeader *>(&vibration));
    }
    void input() {
        controller(app.settings.questTouch);
        pointerHit = pointerClick = false;
        wheel = 0;
        XrActiveActionSet active{actions, XR_NULL_PATH};
        XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};
        sync.countActiveActionSets = 1;
        sync.activeActionSets = &active;
        auto result = xrSyncActions(session, &sync);
        if (result == XR_SESSION_NOT_FOCUSED || state != XR_SESSION_STATE_FOCUSED) {
            if (joystick) {
                for (int n = 0; n < SDL_CONTROLLER_BUTTON_MAX; ++n)
                    SDL_JoystickSetVirtualButton(joystick, n, 0);
                for (int n = 0; n < SDL_CONTROLLER_AXIS_MAX; ++n)
                    SDL_JoystickSetVirtualAxis(joystick, n,
                                               n >= SDL_CONTROLLER_AXIS_TRIGGERLEFT ? -32768 : 0);
                SDL_JoystickUpdate();
            }
            grabbing = false;
            previousClick = false;
            return;
        }
        check(result, "Read Quest controls");
        bool x = boolean(buttonX), a = boolean(buttonA), b = boolean(buttonB), y = boolean(buttonY),
             clicked = boolean(thumb);
        // Disabling Touch gameplay must leave a way back into its settings.
        if (!app.settings.questTouch) {
            if (y && !previousY) {
                SDL_Event event{};
                event.type = SDL_CONTROLLERBUTTONDOWN;
                event.cbutton.button = SDL_CONTROLLER_BUTTON_BACK;
                event.cbutton.state = SDL_PRESSED;
                app.event(event);
            }
            if (x && !previousX && app.menu)
                app.showGuide(app.guideTopic());
        }
        previousY = y;
        auto left = axes(0), right = axes(1);
        float lt = scalar(trigger, 0), rt = scalar(trigger, 1), lg = scalar(squeeze, 0),
              rg = scalar(squeeze, 1);
        if (b && !previousB)
            bForMenu = app.menu;
        previousB = b;
        if (app.menu && !lastMenu)
            blockPointerTrigger = rt > .2f;
        if (rt < .2f)
            blockPointerTrigger = false;
        lastMenu = app.menu;
        if (clicked && !previousThumb)
            needRecenter = true;
        previousThumb = clicked;
        if (app.settings.questTouch && x && !previousX && !app.menu) {
            app.settings.view = app.settings.view == 2 ? 0 : 2;
            app.dirty = true;
        }
        previousX = x;
        auto l = locate(gripSpaces[0]), r = locate(gripSpaces[1]);
        if (app.settings.questTouch && !app.settings.questAnchor && lg > .75f && rg > .75f &&
            valid(l) && valid(r)) {
            auto midpoint = mul(add(l.pose.position, r.pose.position), .5f);
            float separation = length(sub(l.pose.position, r.pose.position));
            if (!grabbing) {
                grabbing = true;
                grabMid = midpoint;
                grabPanel = panel;
                grabSeparation = std::max(.15f, separation);
                grabWidth = app.settings.questWidth * .01f;
            }
            panel.position = add(grabPanel.position, sub(midpoint, grabMid));
            app.settings.questWidth =
                int(std::clamp(grabWidth * separation / grabSeparation, .8f, 8.f) * 100);
            app.settings.questDistance =
                int(std::clamp(length(sub(panel.position, head.position)), .8f, 6.f) * 100);
            app.dirty = true;
        } else
            grabbing = false;
        if (app.settings.questTouch || app.menu) {
            auto pointer = locate(aimSpaces[1]);
            if (valid(pointer)) {
                auto pose = panelPose();
                auto q = inverse(pose.orientation);
                auto origin = rotate(q, sub(pointer.pose.position, pose.position));
                auto direction = rotate(q, rotate(pointer.pose.orientation, {0, 0, -1}));
                if (direction.z < -.0001f) {
                    float distance = -origin.z / direction.z;
                    auto hit = add(origin, mul(direction, distance));
                    float w = app.settings.questWidth * .01f, h = w * height / width;
                    pointerHit =
                        distance > 0 && std::abs(hit.x) <= w * .5f && std::abs(hit.y) <= h * .5f;
                    if (pointerHit) {
                        pointerX = (hit.x / w + .5f) * width;
                        pointerY = (.5f - hit.y / h) * height;
                    }
                }
            }
            pointerClick = app.menu && pointerHit && rt > .55f && !grabbing && !blockPointerTrigger;
            if (pointerClick && !previousClick)
                feedback();
            previousClick = pointerClick;
            wheel = app.menu && std::abs(right.y) > .2f
                        ? right.y * float(frame.predictedDisplayPeriod) * 1e-9f * 12.f
                        : 0;
        }
        if (joystick) {
            auto axis = [&](int axis, float value) {
                SDL_JoystickSetVirtualAxis(joystick, axis,
                                           Sint16(std::clamp(value, -1.f, 1.f) * 32767));
            };
            auto button = [&](int button, bool down) {
                SDL_JoystickSetVirtualButton(joystick, button, down ? 1 : 0);
            };
            axis(SDL_CONTROLLER_AXIS_LEFTX, left.x);
            axis(SDL_CONTROLLER_AXIS_LEFTY, -left.y);
            axis(SDL_CONTROLLER_AXIS_RIGHTX, app.menu ? 0 : right.x);
            axis(SDL_CONTROLLER_AXIS_RIGHTY, app.menu ? 0 : -right.y);
            axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT, app.menu ? -1 : lt * 2 - 1);
            axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -1);
            button(SDL_CONTROLLER_BUTTON_A, a || (!app.menu && rt > .55f));
            button(SDL_CONTROLLER_BUTTON_B, bForMenu && b);
            button(SDL_CONTROLLER_BUTTON_START, !bForMenu && b);
            button(SDL_CONTROLLER_BUTTON_BACK, y);
            button(SDL_CONTROLLER_BUTTON_Y, app.menu && x);
            button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, app.menu && lg > .7f && rg < .2f);
            button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, app.menu && rg > .7f && lg < .2f);
            SDL_JoystickUpdate();
        }
    }
    bool begin() {
        events();
        app.focused = state == XR_SESSION_STATE_FOCUSED;
        if (!running || !app.running)
            return false;
        resize();
        XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
        check(xrWaitFrame(session, &wait, &frame), "Wait for headset frame");
        XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};
        check(xrBeginFrame(session, &begin), "Begin headset frame");
        frameOpen = true;
        auto located = locate(view);
        haveHead = valid(located);
        if (haveHead)
            head = located.pose;
        auto placement = std::array<int, 4>{{app.settings.questDistance, app.settings.questHeight,
                                             app.settings.questTilt, app.settings.questAnchor}};
        if (placement != lastPlacement && !grabbing)
            needRecenter = true;
        lastPlacement = placement;
        if (needRecenter)
            place();
        input();
        if (needRecenter)
            place();
        bool want = app.settings.questPassthrough && passthroughSupported;
        if (want != passthroughActive) {
            if (want) {
                if (XR_SUCCEEDED(startPassthrough(passthrough)) &&
                    XR_SUCCEEDED(resumePassthroughLayer(passthroughLayer)))
                    passthroughActive = true;
                else {
                    app.settings.questPassthrough = false;
                    app.notify(tr("questarunavailable", app.language));
                }
            } else {
                pausePassthroughLayer(passthroughLayer);
                pausePassthrough(passthrough);
                passthroughActive = false;
            }
        }
        if (SDL_SetRenderTarget(app.renderer, canvas))
            throw std::runtime_error(SDL_GetError());
        return true;
    }
    void end() {
        if (!frameOpen)
            return;
        std::array<const XrCompositionLayerBaseHeader *, 2> layers{};
        uint32_t layerCount = 0;
        XrCompositionLayerPassthroughFB room{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
        XrCompositionLayerQuad screen{XR_TYPE_COMPOSITION_LAYER_QUAD};
        if (frame.shouldRender && passthroughActive) {
            room.layerHandle = passthroughLayer;
            layers[layerCount++] = reinterpret_cast<XrCompositionLayerBaseHeader *>(&room);
        }
        if (frame.shouldRender && haveHead) {
            SDL_RenderFlush(app.renderer);
            XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            uint32_t index = 0;
            check(xrAcquireSwapchainImage(swapchain, &acquire, &index), "Acquire screen image");
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wait.timeout = XR_INFINITE_DURATION;
            check(xrWaitSwapchainImage(swapchain, &wait), "Wait for screen image");
            GLint read = 0, draw = 0;
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   images[index].image, 0);
            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("The headset screen framebuffer is incomplete");
            // SDL target textures use a top-at-zero convention. OpenXR GL images use bottom-left.
            glBlitFramebuffer(0, 0, width, height, 0, app.settings.questFlip ? 0 : height, width,
                              app.settings.questFlip ? height : 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read));
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(draw));
            if (scissor)
                glEnable(GL_SCISSOR_TEST);
            glFlush();
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            check(xrReleaseSwapchainImage(swapchain, &release), "Release screen image");
            screen.space = local;
            screen.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
            screen.pose = panelPose();
            screen.subImage.swapchain = swapchain;
            screen.subImage.imageRect = {{0, 0}, {width, height}};
            screen.size.width = app.settings.questWidth * .01f;
            screen.size.height = screen.size.width * height / width;
            layers[layerCount++] = reinterpret_cast<XrCompositionLayerBaseHeader *>(&screen);
        }
        XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};
        end.displayTime = frame.predictedDisplayTime;
        // Quest's reconstruction passthrough extension still uses the OPAQUE environment blend
        // mode.
        end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        end.layerCount = layerCount;
        end.layers = layers.data();
        check(xrEndFrame(session, &end), "Submit headset frame");
        frameOpen = false;
        SDL_SetRenderTarget(app.renderer, nullptr);
    }
    ~Impl() {
        if (frameOpen && session) {
            XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};
            end.displayTime = frame.predictedDisplayTime;
            end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            xrEndFrame(session, &end);
        }
        detachController();
        SDL_SetRenderTarget(app.renderer, nullptr);
        if (canvas)
            SDL_DestroyTexture(canvas);
        if (framebuffer)
            glDeleteFramebuffers(1, &framebuffer);
        if (passthroughLayer && destroyPassthroughLayer)
            destroyPassthroughLayer(passthroughLayer);
        if (passthrough && destroyPassthrough)
            destroyPassthrough(passthrough);
        for (auto s : aimSpaces)
            if (s)
                xrDestroySpace(s);
        for (auto s : gripSpaces)
            if (s)
                xrDestroySpace(s);
        if (swapchain)
            xrDestroySwapchain(swapchain);
        if (view)
            xrDestroySpace(view);
        if (local)
            xrDestroySpace(local);
        if (session) {
            if (running)
                xrEndSession(session);
            xrDestroySession(session);
        }
        if (actions)
            xrDestroyActionSet(actions);
        if (instance)
            xrDestroyInstance(instance);
        if (activity)
            static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv())->DeleteGlobalRef(activity);
        app.quest = nullptr;
    }
};

QuestPresentation::QuestPresentation(App &app) : impl(std::make_unique<Impl>(app)) {}
QuestPresentation::~QuestPresentation() = default;
void QuestPresentation::initialise() {
    impl->initialise();
}
bool QuestPresentation::beginFrame() {
    return impl->begin();
}
void QuestPresentation::endFrame() {
    impl->end();
}
void QuestPresentation::recenter() {
    impl->needRecenter = true;
}
void QuestPresentation::logicalSize(int &w, int &h) const {
    w = impl->width;
    h = impl->height;
}
void QuestPresentation::chooseRom() {
    auto env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    auto type = env->GetObjectClass(impl->activity);
    auto method = env->GetMethodID(type, "chooseRom", "()V");
    if (method)
        env->CallVoidMethod(impl->activity, method);
    env->DeleteLocalRef(type);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        impl->app.notify("Android could not open the document picker.");
    }
}
void QuestPresentation::prepareUi() {
    auto &io = ImGui::GetIO();
    io.DisplaySize = {float(impl->width), float(impl->height)};
    io.DisplayFramebufferScale = {1, 1};
    io.AddMousePosEvent(impl->pointerHit ? impl->pointerX : -FLT_MAX,
                        impl->pointerHit ? impl->pointerY : -FLT_MAX);
    io.AddMouseButtonEvent(0, impl->pointerClick);
    if (impl->wheel)
        io.AddMouseWheelEvent(0, impl->wheel);
}
void QuestPresentation::drawReticle() {
    if (impl->app.menu && impl->pointerHit) {
        auto draw = ImGui::GetForegroundDrawList();
        ImVec2 p{impl->pointerX, impl->pointerY};
        draw->AddCircleFilled(p, impl->pointerClick ? 7 : 4, IM_COL32(255, 213, 104, 255));
        draw->AddCircle(p, 10, IM_COL32(20, 35, 50, 230), 24, 2);
    }
}
void QuestPresentation::options() {
    auto &a = impl->app;
    auto &s = a.settings;
    bool changed = false;
    ImGui::TextWrapped("%s", tr("questintro", a.language));
    ImGui::BeginDisabled(!impl->passthroughSupported);
    changed |= a.tick("questar", s.questPassthrough);
    ImGui::EndDisabled();
    if (!impl->passthroughSupported)
        ImGui::TextWrapped("%s", tr("questarunavailable", a.language));
    changed |= a.choice("questanchor", s.questAnchor, {"questroom", "questhead"});
    changed |= a.slider("questwidth", s.questWidth, 80, 800, "%d cm");
    changed |= a.slider("questdistance", s.questDistance, 80, 600, "%d cm");
    changed |= a.slider("questheight", s.questHeight, -150, 150, "%d cm");
    changed |= a.slider("questtilt", s.questTilt, -60, 80, "%d°");
    changed |= a.choice("questresolution", s.questResolution,
                        {"questeco", "questbalanced", "questsharp", "questwide"});
    changed |= a.tick("questtouch", s.questTouch);
    changed |= a.slider("questhaptics", s.questHaptics, 0, 100, "%d%%");
    changed |= a.tick("questflip", s.questFlip);
    if (a.button("questrecenter"))
        recenter();
    a.nextButton("questcinema");
    if (a.button("questcinema")) {
        s.questWidth = 320;
        s.questDistance = 240;
        s.questHeight = 0;
        s.questTilt = 0;
        s.questAnchor = 0;
        changed = true;
    }
    a.nextButton("questtabletop");
    if (a.button("questtabletop")) {
        s.questWidth = 220;
        s.questDistance = 140;
        s.questHeight = -60;
        s.questTilt = 65;
        s.questAnchor = 0;
        s.view = 2;
        changed = true;
    }
    ImGui::TextWrapped("%s", tr("questgrab", a.language));
    ImGui::TextWrapped("%s", tr("questqualitynote", a.language));
    if (changed)
        a.dirty = true;
}
} // namespace scu
#endif
