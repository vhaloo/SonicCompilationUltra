#include "quest.h"
#ifdef SCU_QUEST
#include "ui.h"
#include "quest_scene.h"
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
XrPosef untransform(XrPosef a, XrPosef b) {
    auto q = inverse(a.orientation);
    return {product(q, b.orientation), rotate(q, sub(b.position, a.position))};
}
bool sameUuid(XrUuidEXT a, XrUuidEXT b) {
    return !std::memcmp(a.data, b.data, sizeof a.data);
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
    XrSpace local = XR_NULL_HANDLE, view = XR_NULL_HANDLE, floorSpace = XR_NULL_HANDLE;
    struct Screen {
        XrSwapchain swapchain = XR_NULL_HANDLE;
        std::vector<XrSwapchainImageOpenGLESKHR> images;
        SDL_Texture *canvas = nullptr;
        int width = 0, height = 0, requestedWidth = 0, requestedHeight = 0, mode = -1;
        bool valid = false;
        uint64_t imageVersion = ~uint64_t(0);
    } gameScreen, menuScreen;
    Screen *activeScreen = nullptr;
    XrActionSet actions = XR_NULL_HANDLE;
    XrAction stick = XR_NULL_HANDLE, trigger = XR_NULL_HANDLE, squeeze = XR_NULL_HANDLE;
    XrAction buttonA = XR_NULL_HANDLE, buttonB = XR_NULL_HANDLE, buttonX = XR_NULL_HANDLE,
             buttonY = XR_NULL_HANDLE;
    XrAction thumb = XR_NULL_HANDLE, aim = XR_NULL_HANDLE, grip = XR_NULL_HANDLE,
             haptic = XR_NULL_HANDLE;
    std::array<XrPath, 2> hands{};
    std::array<XrSpace, 2> aimSpaces{}, gripSpaces{};
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
    int width = 1920, height = 1080;
    std::array<int, 4> lastPlacement{{0, 0, 0, 0}};
    GLuint framebuffer = 0;
    SDL_Joystick *joystick = nullptr;
    jobject activity = nullptr;
    JavaVM *vm = nullptr;
    jmethodID permissionMethod = nullptr, requestPermissionMethod = nullptr;
    std::unique_ptr<QuestScene> scene;
    bool sceneSupported = false, storageSupported = false, captureSupported = false;
    bool refreshSupported = false, localFloorSupported = false;
    bool permissionAsked = false, permissionGranted = false, queriedScene = false, restored = false,
         captureAfterPermission = false;
    bool wallPlaced = false, wallTracked = true, selectedWall = false, useSavedAnchor = true,
         menuRecovery = false;
    bool automaticPlaced = false, menuPlaced = false, companionLast = false, wasDrawn = false;
    bool floorKnown = false, manualFloor = false, referenceLost = false, pendingPin = false,
         previousWallTracked = true;
    float floorY = 0, wallPhysicalWidth = 3.2f, wallPhysicalHeight = 2.f;
    float displayHz = 0, requestedHz = 0;
    int lastRefresh = -999, lastMode = -1, mapWidth = 0, mapHeight = 0, mapStage = -1,
        mapFamily = -1;
    int adaptiveStep = 0, overBudget = 0, underBudget = 0;
    uint64_t permissionPoll = 0, ratePoll = 0, adaptiveAt = 0, renderStarted = 0;
    std::string mapGame;
    XrUuidEXT selectedUuid{};
    XrPosef wallBase = identity(), wallOffset = identity(), menuPanel = identity(),
            grabBase = identity();
    std::vector<XrEventDataReferenceSpaceChangePending> referenceChanges;
    XrPosef leftGrip = identity();
    bool leftGripValid = false;
    PFN_xrEnumerateDisplayRefreshRatesFB enumerateRates = nullptr;
    PFN_xrRequestDisplayRefreshRateFB requestRate = nullptr;
    PFN_xrGetDisplayRefreshRateFB getRate = nullptr;
    std::vector<float> rates;
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
        auto activityType = env->GetObjectClass(activity);
        permissionMethod = env->GetMethodID(activityType, "scenePermissionGranted", "()Z");
        requestPermissionMethod = env->GetMethodID(activityType, "requestScenePermission", "()V");
        env->DeleteLocalRef(activityType);
        if (env->ExceptionCheck())
            env->ExceptionClear();
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
        bool spatial = supports(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME) &&
                       supports(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
        sceneSupported = spatial && supports(XR_FB_SCENE_EXTENSION_NAME);
        storageSupported = spatial && supports(XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME);
        captureSupported = sceneSupported && supports(XR_FB_SCENE_CAPTURE_EXTENSION_NAME);
        if (sceneSupported || storageSupported) {
            enabled.push_back(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
            enabled.push_back(XR_FB_SPATIAL_ENTITY_QUERY_EXTENSION_NAME);
        }
        if (sceneSupported)
            enabled.push_back(XR_FB_SCENE_EXTENSION_NAME);
        if (storageSupported)
            enabled.push_back(XR_FB_SPATIAL_ENTITY_STORAGE_EXTENSION_NAME);
        if (captureSupported)
            enabled.push_back(XR_FB_SCENE_CAPTURE_EXTENSION_NAME);
        refreshSupported = supports(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
        if (refreshSupported)
            enabled.push_back(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
        localFloorSupported = supports(XR_EXT_LOCAL_FLOOR_EXTENSION_NAME);
        if (localFloorSupported)
            enabled.push_back(XR_EXT_LOCAL_FLOOR_EXTENSION_NAME);
        XrInstanceCreateInfoAndroidKHR android{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
        android.applicationVM = vm;
        android.applicationActivity = activity;
        XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
        info.next = &android;
        std::strcpy(info.applicationInfo.applicationName, "Sonic Compilation Ultra XR");
        info.applicationInfo.applicationVersion = 500;
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
        uint32_t spaceCount = 0;
        check(xrEnumerateReferenceSpaces(session, 0, &spaceCount, nullptr),
              "Read tracking origins");
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        check(xrEnumerateReferenceSpaces(session, spaceCount, &spaceCount, spaces.data()),
              "Read tracking origins");
        for (auto type : {XR_REFERENCE_SPACE_TYPE_STAGE, XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT}) {
            if (type == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT && !localFloorSupported)
                continue;
            if (std::find(spaces.begin(), spaces.end(), type) != spaces.end()) {
                spaceInfo.referenceSpaceType = type;
                if (XR_SUCCEEDED(xrCreateReferenceSpace(session, &spaceInfo, &floorSpace)))
                    break;
            }
        }
        scene = std::make_unique<QuestScene>(app.library.dataDir);
        scene->initialise(instance, session, sceneSupported, storageSupported, captureSupported);
        if (refreshSupported) {
            function("xrEnumerateDisplayRefreshRatesFB", enumerateRates);
            function("xrRequestDisplayRefreshRateFB", requestRate);
            function("xrGetDisplayRefreshRateFB", getRate);
            uint32_t rateCount = 0;
            if (XR_SUCCEEDED(enumerateRates(session, 0, &rateCount, nullptr)) && rateCount <= 128) {
                rates.resize(rateCount);
                if (XR_FAILED(enumerateRates(session, rateCount, &rateCount, rates.data())))
                    rates.clear();
                rates.erase(
                    std::remove_if(rates.begin(), rates.end(),
                                   [](float n) { return !std::isfinite(n) || n < 30 || n > 240; }),
                    rates.end());
                std::sort(rates.begin(), rates.end());
                rates.erase(std::unique(rates.begin(), rates.end()), rates.end());
            }
        }
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
    void destroy(Screen &screen) {
        SDL_SetRenderTarget(app.renderer, nullptr);
        if (screen.canvas)
            SDL_DestroyTexture(screen.canvas);
        if (screen.swapchain)
            xrDestroySwapchain(screen.swapchain);
        screen = {};
    }
    void resize(Screen &screen, int desiredWidth, int desiredHeight) {
        if (screen.requestedWidth == desiredWidth && screen.requestedHeight == desiredHeight)
            return;
        destroy(screen);
        XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
        check(xrGetSystemProperties(instance, system, &properties), "Read headset limits");
        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
        float scale = std::min(
            {1.f, float(properties.graphicsProperties.maxSwapchainImageWidth) / desiredWidth,
             float(properties.graphicsProperties.maxSwapchainImageHeight) / desiredHeight,
             float(limit) / std::max(desiredWidth, desiredHeight)});
        int w = std::max(1, int(desiredWidth * scale)), h = std::max(1, int(desiredHeight * scale));
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
        create.width = w;
        create.height = h;
        create.faceCount = 1;
        create.arraySize = 1;
        create.mipCount = 1;
        check(xrCreateSwapchain(session, &create, &screen.swapchain), "Create spatial screen");
        check(xrEnumerateSwapchainImages(screen.swapchain, 0, &count, nullptr),
              "Read screen images");
        screen.images.assign(count, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        check(xrEnumerateSwapchainImages(
                  screen.swapchain, count, &count,
                  reinterpret_cast<XrSwapchainImageBaseHeader *>(screen.images.data())),
              "Read screen images");
        screen.canvas = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET, w, h);
        if (!screen.canvas)
            throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(screen.canvas, SDL_BLENDMODE_NONE);
        // Remember requested size as well as the runtime-clamped image dimensions.
        screen.width = w;
        screen.height = h;
        screen.requestedWidth = desiredWidth;
        screen.requestedHeight = desiredHeight;
    }
    bool fixed() const {
        return app.settings.questMode != 0;
    }
    bool companion() const {
        return fixed() && (app.settings.questCompanion || menuRecovery) && app.menu;
    }
    XrQuaternionf yawOrientation() const {
        auto forward = rotate(head.orientation, {0, 0, -1});
        float yaw = std::atan2(-forward.x, -forward.z);
        return {0, std::sin(yaw / 2), 0, std::cos(yaw / 2)};
    }
    void placeMenu() {
        if (!haveHead)
            return;
        menuPanel.orientation = yawOrientation();
        menuPanel.position =
            add(head.position,
                rotate(menuPanel.orientation, {0, -.06f, -app.settings.questMenuDistance * .01f}));
        menuPlaced = true;
    }
    float floorGap() const {
        float gap = app.settings.questWallGap * .01f;
        return app.settings.questMode == 1 ? std::min(gap, std::max(0.f, wallPhysicalHeight - .5f))
                                           : gap;
    }
    float muralHeight() const {
        float h = app.settings.questWallHeight * .01f;
        if (app.settings.questMode == 1)
            h = std::min(h, std::max(.5f, wallPhysicalHeight - floorGap()));
        return h;
    }
    float aspect() const {
        if (app.settings.questMode == 1)
            return std::clamp(wallPhysicalWidth / muralHeight(), .25f, 128.f);
        return mapWidth && mapHeight ? float(mapWidth) / mapHeight : 16.f / 9;
    }
    XrExtent2Df physicalSize() const {
        if (fixed()) {
            float h = muralHeight();
            return {h * aspect(), h};
        }
        float w = app.settings.questWidth * .01f;
        return {w, w * gameScreen.height / std::max(1, gameScreen.width)};
    }
    XrPosef worldPose() const {
        if (!fixed()) {
            if (!app.settings.questAnchor)
                return panel;
            float angle = -std::clamp(app.settings.questTilt, -60, 80) * pi / 180;
            XrPosef offset{
                {std::sin(angle / 2), 0, 0, std::cos(angle / 2)},
                {0, app.settings.questHeight * .01f, -app.settings.questDistance * .01f}};
            return transform(head, offset);
        }
        auto base = wallBase;
        const float h = muralHeight(), gap = floorGap();
        if (app.settings.questMode == 3) {
            XrPosef offset{{-std::sqrt(.5f), 0, 0, std::sqrt(.5f)}, {0, .015f + gap, -h / 2}};
            return transform(base, offset);
        }
        return transform(base, {{0, 0, 0, 1}, {0, h / 2 + gap, .025f}});
    }
    void manualWall(bool detach = true) {
        if (!haveHead)
            return;
        wallBase.orientation = yawOrientation();
        wallBase.position = add(head.position, rotate(wallBase.orientation,
                                                      {0, 0, -app.settings.questDistance * .01f}));
        wallBase.position.y = floorKnown ? floorY : head.position.y - 1.65f;
        wallPhysicalWidth = app.settings.questWidth * .01f;
        wallPhysicalHeight = 2.5f;
        if (detach) {
            selectedWall = false;
            useSavedAnchor = false;
        }
        referenceLost = false;
        wallPlaced = wallTracked = true;
    }
    void layout() {
        auto map = app.core.world.inspect(app.core.snapshot, app.core.game);
        if (app.core.loaded && map.ready &&
            (mapGame != app.core.currentId || mapStage != map.stage || mapFamily != map.family ||
             !mapWidth)) {
            mapGame = app.core.currentId;
            mapStage = map.stage;
            mapFamily = map.family;
            mapWidth = map.width;
            mapHeight = map.height;
            gameScreen.valid = false;
        }
        if (mapGame != app.core.currentId && !app.core.loaded) {
            mapGame.clear();
            mapWidth = mapHeight = 0;
            mapStage = -1;
            mapFamily = -1;
        }
        if (app.settings.questMode != lastMode) {
            if (fixed() && !wallPlaced)
                manualWall(false);
            automaticPlaced = false;
            gameScreen.valid = false;
            lastMode = app.settings.questMode;
            menuPlaced = false;
        }
    }
    std::pair<int, int> canvasSize(bool menu) const {
        const std::array<std::pair<int, int>, 4> sizes{
            {{1280, 720}, {1920, 1080}, {2560, 1440}, {3072, 576}}};
        int resolution = std::clamp(app.settings.questResolution, 0, 3);
        if (menu)
            return resolution == 0 ? sizes[0] : sizes[1];
        if (!fixed())
            return sizes[resolution];
        int detail = std::clamp(app.settings.questWallDetail, 0, 2);
        int capWidth = detail == 0 ? 4096 : detail == 1 ? 8192 : 12288;
        double pixels = detail == 0 ? 2097152. : detail == 1 ? 4194304. : 8388608.;
        int step = app.settings.questAdaptive ? adaptiveStep : 0;
        pixels /= double(1 << step);
        float ratio = aspect();
        int w = std::min(capWidth, int(std::sqrt(pixels * ratio)));
        int h = std::max(1, int(std::lround(w / ratio)));
        if (h > 2048) {
            h = 2048;
            w = std::max(1, int(std::lround(h * ratio)));
        }
        return {std::max(64, w), h};
    }
    void prepareRender() {
        bool menu = companion();
        if (menu && (!companionLast || !menuPlaced))
            placeMenu();
        companionLast = menu;
        activeScreen = menu ? &menuScreen : &gameScreen;
        auto size = canvasSize(menu);
        resize(*activeScreen, size.first, size.second);
        width = activeScreen->width;
        height = activeScreen->height;
        if (SDL_SetRenderTarget(app.renderer, activeScreen->canvas))
            throw std::runtime_error(SDL_GetError());
    }
    void refreshRate() {
        if (!refreshSupported || (rates.empty() && lastRefresh != -999))
            return;
        if (lastRefresh != app.settings.questRefresh) {
            if (lastRefresh == -999) {
                uint32_t count = 0;
                if (XR_SUCCEEDED(enumerateRates(session, 0, &count, nullptr)) && count &&
                    count <= 128) {
                    std::vector<float> available(count);
                    if (XR_SUCCEEDED(enumerateRates(session, count, &count, available.data()))) {
                        available.erase(std::remove_if(available.begin(), available.end(),
                                                       [](float rate) {
                                                           return !std::isfinite(rate) ||
                                                                  rate < 30 || rate > 240;
                                                       }),
                                        available.end());
                        std::sort(available.begin(), available.end());
                        available.erase(std::unique(available.begin(), available.end()),
                                        available.end());
                        if (!available.empty())
                            rates = std::move(available);
                    }
                }
            }
            lastRefresh = app.settings.questRefresh;
            if (rates.empty())
                return;
            float wanted = lastRefresh == 0  ? rates.back()
                           : lastRefresh < 0 ? 0.f
                                             : float(lastRefresh);
            if (wanted && std::none_of(rates.begin(), rates.end(),
                                       [&](float n) { return std::abs(n - wanted) < .1f; }))
                wanted = rates.back();
            if (XR_SUCCEEDED(requestRate(session, wanted)))
                requestedHz = wanted;
            else {
                requestedHz = 0;
                requestRate(session, 0);
                app.notify(tr("questrefreshdenied", app.language));
            }
        }
        if (SDL_GetTicks64() - ratePoll >= 1000) {
            ratePoll = SDL_GetTicks64();
            getRate(session, &displayHz);
        }
    }
    XrSpaceLocation locate(XrSpace space) {
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        if (XR_FAILED(xrLocateSpace(space, local, frame.predictedDisplayTime, &location)))
            location.locationFlags = 0;
        return location;
    }
    void requestScenePermission() {
        if (!requestPermissionMethod)
            return;
        auto env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
        env->CallVoidMethod(activity, requestPermissionMethod);
        if (env->ExceptionCheck())
            env->ExceptionClear();
        permissionAsked = true;
    }
    bool selectWall(bool next = false) {
        struct Choice {
            float score, width, height;
            XrUuidEXT uuid;
            XrPosef base, offset;
        };
        std::vector<Choice> choices;
        for (auto &surface : scene->surfaces()) {
            if (!surface.wall || !surface.metadata)
                continue;
            auto located = locate(surface.space);
            if (!valid(located))
                continue;
            auto up = rotate(located.pose.orientation, {0, 1, 0});
            auto normal = rotate(located.pose.orientation, {0, 0, 1});
            if (std::abs(up.y) < .8f || std::abs(normal.y) > .25f)
                continue;
            auto &b = surface.bounds;
            auto center =
                add(located.pose.position,
                    rotate(located.pose.orientation,
                           {b.offset.x + b.extent.width / 2, b.offset.y + b.extent.height / 2, 0}));
            auto delta = sub(center, head.position);
            const float distance = length(delta);
            if (distance < .25f || distance > 10.f)
                continue;
            if (dot(normal, sub(head.position, center)) < 0)
                normal = mul(normal, -1);
            normal.y = 0;
            normal = mul(normal, 1.f / std::max(.01f, length(normal)));
            float yaw = std::atan2(normal.x, normal.z);
            XrQuaternionf q{0, std::sin(yaw / 2), 0, std::cos(yaw / 2)};
            float low = 1000, high = -1000;
            for (float x : {b.offset.x, b.offset.x + b.extent.width})
                for (float y : {b.offset.y, b.offset.y + b.extent.height}) {
                    auto point =
                        add(located.pose.position, rotate(located.pose.orientation, {x, y, 0}));
                    low = std::min(low, point.y);
                    high = std::max(high, point.y);
                }
            if (floorKnown && std::abs(floorY - low) < .5f)
                low = floorY;
            if (high - low < .75f || head.position.y < low + .2f || head.position.y > high + .3f)
                continue;
            XrPosef base{q, {center.x, low, center.z}};
            auto direction = rotate(head.orientation, {0, 0, -1});
            float denominator = dot(direction, normal);
            float rayDistance =
                std::abs(denominator) > .001f ? dot(delta, normal) / denominator : -1;
            bool hit = false;
            if (rayDistance > 0) {
                auto point = add(head.position, mul(direction, rayDistance));
                auto p = rotate(inverse(q), sub(point, base.position));
                hit = std::abs(p.x) <= b.extent.width / 2 && p.y >= 0 && p.y <= high - low;
            }
            float score =
                hit ? rayDistance
                    : 100.f + distance + 10.f * std::max(0.f, -dot(direction, delta) / distance);
            choices.push_back({score, b.extent.width, high - low, surface.uuid, base,
                               untransform(located.pose, base)});
        }
        if (choices.empty())
            return false;
        std::sort(choices.begin(), choices.end(),
                  [](const Choice &a, const Choice &b) { return a.score < b.score; });
        size_t index = 0;
        if (next && selectedWall)
            for (size_t i = 0; i < choices.size(); ++i)
                if (sameUuid(choices[i].uuid, selectedUuid)) {
                    index = (i + 1) % choices.size();
                    break;
                }
        auto &chosen = choices[index];
        selectedUuid = chosen.uuid;
        wallOffset = chosen.offset;
        wallBase = chosen.base;
        wallPhysicalWidth = chosen.width;
        wallPhysicalHeight = chosen.height;
        wallPlaced = wallTracked = selectedWall = automaticPlaced = true;
        useSavedAnchor = false;
        referenceLost = false;
        return true;
    }
    void updateWall() {
        if (!fixed() || !haveHead)
            return;
        scene->update();
        if (!manualFloor && floorSpace) {
            auto floor = locate(floorSpace);
            if (valid(floor)) {
                floorY = floor.pose.position.y;
                floorKnown = true;
            }
        }
        if (!manualFloor)
            for (auto &surface : scene->surfaces())
                if (surface.floor) {
                    auto floor = locate(surface.space);
                    if (valid(floor) && length(sub(floor.pose.position, head.position)) < 10 &&
                        head.position.y - floor.pose.position.y > .2f &&
                        head.position.y - floor.pose.position.y < 3.f) {
                        floorY = floor.pose.position.y;
                        floorKnown = true;
                        break;
                    }
                }
        if (!wallPlaced)
            manualWall(false);
        if (pendingPin && !std::strcmp(scene->status(), "questwallsaved")) {
            useSavedAnchor = true;
            selectedWall = false;
            pendingPin = false;
        } else if (pendingPin && !std::strcmp(scene->status(), "questsavefailed"))
            pendingPin = false;
        if (useSavedAnchor && !restored && !scene->busy() && scene->storageSupported()) {
            scene->restoreAnchor();
            restored = true;
        }
        if (app.settings.questAutoWall && app.settings.questMode != 3 && scene->supported()) {
            const auto now = SDL_GetTicks64();
            if (!permissionPoll || now - permissionPoll >= 1000) {
                permissionPoll = now;
                auto env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
                permissionGranted =
                    permissionMethod && env->CallBooleanMethod(activity, permissionMethod);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    permissionGranted = false;
                }
            }
            if (!permissionGranted && !permissionAsked)
                requestScenePermission();
            if (permissionGranted && captureAfterPermission && !scene->busy()) {
                scene->captureScene();
                captureAfterPermission = false;
                automaticPlaced = false;
                queriedScene = true;
            }
            if (permissionGranted && !queriedScene && !scene->busy()) {
                scene->queryScene();
                queriedScene = true;
            }
            if (!automaticPlaced && !(useSavedAnchor && scene->anchor()) && !scene->busy())
                selectWall();
        }
        wallTracked = !referenceLost;
        if (useSavedAnchor && scene->anchor()) {
            auto anchor = locate(scene->anchor());
            wallTracked = valid(anchor);
            if (wallTracked) {
                wallBase = anchor.pose;
                wallPhysicalWidth = scene->anchorBounds().width;
                wallPhysicalHeight = scene->anchorBounds().height;
                selectedWall = false;
                referenceLost = false;
            }
        } else if (selectedWall) {
            wallTracked = false;
            for (auto &surface : scene->surfaces())
                if (sameUuid(surface.uuid, selectedUuid)) {
                    auto located = locate(surface.space);
                    wallTracked = valid(located);
                    if (wallTracked) {
                        wallBase = transform(located.pose, wallOffset);
                        referenceLost = false;
                    }
                    break;
                }
        } else if (floorKnown && !referenceLost)
            wallBase.position.y = floorY;
    }
    XrPosef panelPose() const {
        return companion() && menuPlaced ? menuPanel : worldPose();
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
            if (scene)
                scene->event(event);
            if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                state = reinterpret_cast<XrEventDataSessionStateChanged *>(&event)->state;
                if (state == XR_SESSION_STATE_FOCUSED)
                    lastRefresh = -999;
                if (state == XR_SESSION_STATE_READY && !running) {
                    XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};
                    begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    check(xrBeginSession(session, &begin), "Begin headset session");
                    running = true;
                    needRecenter = true;
                    lastRefresh = -999;
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
            else if (event.type == XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING) {
                auto value = reinterpret_cast<XrEventDataReferenceSpaceChangePending &>(event);
                if (value.referenceSpaceType == XR_REFERENCE_SPACE_TYPE_LOCAL) {
                    referenceChanges.push_back(value);
                    menuPlaced = false;
                    if (!fixed())
                        needRecenter = true;
                }
            } else if (event.type == XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB)
                displayHz = reinterpret_cast<XrEventDataDisplayRefreshRateChangedFB &>(event)
                                .toDisplayRefreshRate;
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
            leftGripValid = false;
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
        if (clicked && !previousThumb) {
            if (fixed() && (app.settings.questWallLocked || app.menu)) {
                if (app.menu)
                    menuRecovery = true;
                placeMenu();
                if (!app.menu)
                    app.notify(tr("questwalllockednotice", app.language));
            } else
                needRecenter = true;
        }
        previousThumb = clicked;
        if (app.settings.questTouch && x && !previousX && !app.menu && !fixed()) {
            app.settings.view = app.settings.view == 2 ? 0 : 2;
            app.dirty = true;
        } else if (app.settings.questTouch && x && !previousX && !app.menu && fixed()) {
            app.settings.playerInset = !app.settings.playerInset;
            app.dirty = true;
        }
        previousX = x;
        auto l = locate(gripSpaces[0]), r = locate(gripSpaces[1]);
        leftGripValid = valid(l);
        if (leftGripValid)
            leftGrip = l.pose;
        if (app.settings.questTouch && (fixed() || !app.settings.questAnchor) &&
            (!fixed() || !app.settings.questWallLocked) && lg > .75f && rg > .75f && valid(l) &&
            valid(r)) {
            auto midpoint = mul(add(l.pose.position, r.pose.position), .5f);
            float separation = length(sub(l.pose.position, r.pose.position));
            if (!grabbing) {
                grabbing = true;
                grabMid = midpoint;
                grabPanel = panel;
                grabBase = wallBase;
                grabSeparation = std::max(.15f, separation);
                grabWidth =
                    (fixed() ? app.settings.questWallHeight : app.settings.questWidth) * .01f;
                if (fixed()) {
                    selectedWall = useSavedAnchor = false;
                    automaticPlaced = true;
                }
            }
            if (fixed()) {
                wallBase.position = add(grabBase.position, sub(midpoint, grabMid));
                wallBase.position.y = grabBase.position.y;
                app.settings.questWallHeight =
                    int(std::clamp(grabWidth * separation / grabSeparation, .5f, 5.f) * 100);
                wallTracked = !referenceLost;
            } else {
                panel.position = add(grabPanel.position, sub(midpoint, grabMid));
                app.settings.questWidth =
                    int(std::clamp(grabWidth * separation / grabSeparation, .8f, 8.f) * 100);
                app.settings.questDistance =
                    int(std::clamp(length(sub(panel.position, head.position)), .8f, 6.f) * 100);
            }
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
                    auto size = physicalSize();
                    float w = companion() ? app.settings.questMenuDistance * .014f : size.width;
                    float h = companion() ? w * 9 / 16 : size.height;
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
        XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
        check(xrWaitFrame(session, &wait, &frame), "Wait for headset frame");
        XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};
        check(xrBeginFrame(session, &begin), "Begin headset frame");
        frameOpen = true;
        renderStarted = SDL_GetPerformanceCounter();
        if (!app.menu)
            menuRecovery = false;
        wasDrawn = false;
        activeScreen = nullptr;
        auto located = locate(view);
        haveHead = valid(located);
        if (haveHead)
            head = located.pose;
        while (!referenceChanges.empty() &&
               referenceChanges.front().changeTime <= frame.predictedDisplayTime) {
            auto change = referenceChanges.front();
            referenceChanges.erase(referenceChanges.begin());
            if (change.poseValid) {
                wallBase = untransform(change.poseInPreviousSpace, wallBase);
                panel = untransform(change.poseInPreviousSpace, panel);
                if (manualFloor)
                    floorY = untransform(change.poseInPreviousSpace, {{0, 0, 0, 1}, {0, floorY, 0}})
                                 .position.y;
            } else if (fixed() && !selectedWall && !(useSavedAnchor && scene->anchor()))
                referenceLost = true;
            menuPlaced = false;
        }
        layout();
        updateWall();
        if (fixed() && !wallTracked && haveHead && app.settings.questPauseLost) {
            menuRecovery = true;
            if (!app.menu) {
                app.toggleMenu();
                placeMenu();
            }
            if (previousWallTracked)
                app.notify(tr("questwalltrackinglost", app.language));
        }
        previousWallTracked = wallTracked && haveHead;
        auto placement = std::array<int, 4>{{app.settings.questDistance, app.settings.questHeight,
                                             app.settings.questTilt, app.settings.questAnchor}};
        if (placement != lastPlacement && !grabbing && !fixed())
            needRecenter = true;
        lastPlacement = placement;
        if (needRecenter && !fixed())
            place();
        input();
        if (needRecenter) {
            if (fixed()) {
                if (!app.settings.questWallLocked)
                    manualWall();
                placeMenu();
                needRecenter = false;
            } else
                place();
        }
        bool want = app.settings.questPassthrough && passthroughSupported;
        if (want != passthroughActive) {
            lastRefresh = -999;
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
        refreshRate();
        return true;
    }
    void end() {
        if (!frameOpen)
            return;
        std::array<const XrCompositionLayerBaseHeader *, 3> layers{};
        uint32_t layerCount = 0;
        XrCompositionLayerPassthroughFB room{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
        XrCompositionLayerQuad screen{XR_TYPE_COMPOSITION_LAYER_QUAD};
        XrCompositionLayerQuad controls{XR_TYPE_COMPOSITION_LAYER_QUAD};
        if (frame.shouldRender && passthroughActive) {
            room.layerHandle = passthroughLayer;
            layers[layerCount++] = reinterpret_cast<XrCompositionLayerBaseHeader *>(&room);
        }
        if (frame.shouldRender && wasDrawn && activeScreen) {
            auto &target = *activeScreen;
            SDL_RenderFlush(app.renderer);
            XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            uint32_t index = 0;
            check(xrAcquireSwapchainImage(target.swapchain, &acquire, &index),
                  "Acquire screen image");
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            wait.timeout = XR_INFINITE_DURATION;
            check(xrWaitSwapchainImage(target.swapchain, &wait), "Wait for screen image");
            GLint read = 0, draw = 0;
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   target.images[index].image, 0);
            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("The headset screen framebuffer is incomplete");
            // SDL target textures use a top-at-zero convention. OpenXR GL images use bottom-left.
            glBlitFramebuffer(0, 0, target.width, target.height, 0,
                              app.settings.questFlip ? 0 : target.height, target.width,
                              app.settings.questFlip ? target.height : 0, GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read));
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(draw));
            if (scissor)
                glEnable(GL_SCISSOR_TEST);
            glFlush();
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            check(xrReleaseSwapchainImage(target.swapchain, &release), "Release screen image");
            target.valid = true;
            target.imageVersion = app.core.imageVersion;
            target.mode = app.settings.questMode;
        }
        if (frame.shouldRender && haveHead && gameScreen.valid &&
            gameScreen.mode == app.settings.questMode && (!fixed() || wallTracked)) {
            screen.space = local;
            screen.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
            screen.pose = worldPose();
            screen.subImage.swapchain = gameScreen.swapchain;
            screen.subImage.imageRect = {{0, 0}, {gameScreen.width, gameScreen.height}};
            screen.size = physicalSize();
            layers[layerCount++] = reinterpret_cast<XrCompositionLayerBaseHeader *>(&screen);
        }
        if (frame.shouldRender && haveHead && companion() && menuScreen.valid && menuPlaced) {
            controls.space = local;
            controls.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
            controls.pose = menuPanel;
            controls.subImage.swapchain = menuScreen.swapchain;
            controls.subImage.imageRect = {{0, 0}, {menuScreen.width, menuScreen.height}};
            controls.size.width = app.settings.questMenuDistance * .014f;
            controls.size.height = controls.size.width * 9 / 16;
            layers[layerCount++] = reinterpret_cast<XrCompositionLayerBaseHeader *>(&controls);
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
        if (wasDrawn && activeScreen == &gameScreen && fixed() && !app.menu &&
            app.settings.questAdaptive && frame.predictedDisplayPeriod > 0) {
            double seconds =
                double(SDL_GetPerformanceCounter() - renderStarted) / SDL_GetPerformanceFrequency();
            double period = frame.predictedDisplayPeriod * 1e-9;
            overBudget = seconds > period * .85 ? overBudget + 1 : std::max(0, overBudget - 1);
            underBudget = seconds < period * .45 ? underBudget + 1 : 0;
            auto now = SDL_GetTicks64();
            if (now - adaptiveAt > 3000 && overBudget > 45 && adaptiveStep < 2) {
                ++adaptiveStep;
                adaptiveAt = now;
                overBudget = underBudget = 0;
            } else if (now - adaptiveAt > 10000 && underBudget > 600 && adaptiveStep > 0) {
                --adaptiveStep;
                adaptiveAt = now;
                overBudget = underBudget = 0;
            }
        }
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
        destroy(menuScreen);
        destroy(gameScreen);
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
        scene.reset();
        if (floorSpace)
            xrDestroySpace(floorSpace);
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
bool QuestPresentation::needsRedraw() const {
    auto &p = *impl;
    if (!p.frameOpen || !p.frame.shouldRender || !p.haveHead)
        return false;
    auto &target = p.companion() ? p.menuScreen : p.gameScreen;
    auto size = p.canvasSize(p.companion());
    return p.app.menu || !target.valid || target.mode != p.app.settings.questMode ||
           target.imageVersion != p.app.core.imageVersion || p.app.dirty ||
           p.app.screenshotRequested || target.requestedWidth != size.first ||
           target.requestedHeight != size.second;
}
void QuestPresentation::prepareRender() {
    impl->prepareRender();
}
void QuestPresentation::rendered() {
    impl->wasDrawn = true;
}
void QuestPresentation::configureRender(Settings &settings) const {
    auto &p = *impl;
    if (p.companion()) {
        settings.view = 4;
        settings.projectionUiOnly = true;
        return;
    }
    if (p.fixed()) {
        settings.view = 2;
        settings.camera = 4;
        settings.mapExtension = true;
        settings.fitSpawns = false;
        settings.parallax = false;
        settings.reduceMotion = true;
        settings.fixedMapWidth = p.mapWidth;
        settings.fixedMapHeight = p.mapHeight;
        settings.quality = std::clamp(settings.questWallDetail, 0, 2);
        if (p.height < 240) {
            settings.playerInset = false;
            settings.hud = 3;
        }
    }
}
bool QuestPresentation::simulationPaused() const {
    return impl->fixed() && impl->app.settings.questPauseLost &&
           (!impl->wallTracked || !impl->haveHead);
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
    if (a.choice("questmode", s.questMode,
                 {"questscreenmode", "questwallfit", "questmural", "questflooratlas"})) {
        if (s.questMode) {
            s.questPassthrough = true;
            s.questWallLocked = true;
        }
        changed = true;
    }
    if (impl->fixed()) {
        changed |= a.slider("questwallheight", s.questWallHeight, 50, 500, "%d cm");
        changed |= a.slider("questwallgap", s.questWallGap, 0, 100, "%d cm");
        changed |= a.slider("questwalldistance", s.questDistance, 80, 600, "%d cm");
        changed |= a.tick("questautowall", s.questAutoWall);
        changed |= a.tick("questwalllock", s.questWallLocked);
        changed |= a.tick("questpauselost", s.questPauseLost);
        changed |= a.tick("questcompanion", s.questCompanion);
        changed |= a.slider("questmenudistance", s.questMenuDistance, 60, 250, "%d cm");
        changed |= a.choice("questwalldetail", s.questWallDetail,
                            {"questdetailfast", "questdetailbalanced", "questdetailsharp"});
        changed |= a.tick("questadaptive", s.questAdaptive);
        const char *state = !impl->wallTracked   ? "questwalltrackinglost"
                            : impl->selectedWall ? "questwallscanned"
                            : impl->useSavedAnchor && impl->scene->anchor() ? "questwallanchored"
                                                                            : "questwallmanual";
        ImGui::TextWrapped("%s", tr(state, a.language));
        ImGui::TextWrapped(
            "%s", tr(impl->floorKnown ? "questfloorknown" : "questfloorestimated", a.language));
        ImGui::TextWrapped("%s", tr(impl->scene->status(), a.language));
        if (a.button("questplacewall")) {
            impl->manualWall();
            impl->automaticPlaced = true;
            s.questWallLocked = true;
            changed = true;
        }
        ImGui::BeginDisabled(!impl->scene->supported() || impl->scene->busy());
        if (a.button("questfindwall")) {
            s.questAutoWall = true;
            impl->automaticPlaced = false;
            impl->useSavedAnchor = false;
            impl->queriedScene = false;
            if (!impl->permissionGranted)
                impl->requestScenePermission();
            changed = true;
        }
        a.nextButton("questnextwall");
        if (a.button("questnextwall"))
            impl->selectWall(true);
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!impl->scene->captureSupported() || impl->scene->busy());
        if (a.button("questscanroom")) {
            s.questAutoWall = true;
            impl->captureAfterPermission = true;
            if (!impl->permissionGranted)
                impl->requestScenePermission();
            changed = true;
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!impl->leftGripValid);
        if (a.button("questcalibratefloor")) {
            impl->floorY = impl->leftGrip.position.y;
            impl->floorKnown = impl->manualFloor = true;
            impl->manualWall();
            impl->automaticPlaced = true;
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!impl->scene->storageSupported() || impl->scene->busy() ||
                             !impl->wallTracked);
        if (a.button("questsavewall")) {
            impl->scene->saveAnchor(impl->local, impl->frame.predictedDisplayTime, impl->wallBase,
                                    {impl->wallPhysicalWidth, impl->wallPhysicalHeight});
            impl->pendingPin = true;
        }
        a.nextButton("questrestorewall");
        if (a.button("questrestorewall")) {
            impl->useSavedAnchor = true;
            impl->restored = false;
        }
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("questwallnote", a.language));
    }
    std::vector<std::string> refreshLabels{"questrefreshmax", "questrefreshsystem"};
    int refreshChoice = s.questRefresh < 0 ? 1 : 0;
    for (float rate : impl->rates) {
        refreshLabels.push_back(std::to_string(int(std::lround(rate))) + " Hz");
        if (std::abs(rate - s.questRefresh) < .1f)
            refreshChoice = int(refreshLabels.size()) - 1;
    }
    ImGui::BeginDisabled(impl->rates.empty());
    ImGui::TextWrapped("%s", tr("questrefresh", a.language));
    ImGui::SetNextItemWidth(-1);
    std::vector<const char *> rateText;
    for (const auto &text : refreshLabels)
        rateText.push_back(tr(text.c_str(), a.language));
    if (ImGui::Combo("##questrefresh", &refreshChoice, rateText.data(), int(rateText.size()))) {
        s.questRefresh = refreshChoice == 0   ? 0
                         : refreshChoice == 1 ? -1
                                              : int(std::lround(impl->rates[refreshChoice - 2]));
        changed = true;
    }
    a.control("questrefresh");
    ImGui::EndDisabled();
    if (impl->displayHz <= 0)
        ImGui::TextWrapped("%s", tr("questrefreshunknown", a.language));
    else if (impl->requestedHz <= 0)
        ImGui::Text(tr("questrefreshsystemactive", a.language), impl->displayHz);
    else
        ImGui::Text(tr("questrefreshruntime", a.language), impl->displayHz, impl->requestedHz);
    ImGui::BeginDisabled(!impl->passthroughSupported);
    changed |= a.tick("questar", s.questPassthrough);
    ImGui::EndDisabled();
    if (!impl->passthroughSupported)
        ImGui::TextWrapped("%s", tr("questarunavailable", a.language));
    ImGui::BeginDisabled(impl->fixed());
    changed |= a.choice("questanchor", s.questAnchor, {"questroom", "questhead"});
    changed |= a.slider("questwidth", s.questWidth, 80, 800, "%d cm");
    changed |= a.slider("questdistance", s.questDistance, 80, 600, "%d cm");
    changed |= a.slider("questheight", s.questHeight, -150, 150, "%d cm");
    changed |= a.slider("questtilt", s.questTilt, -60, 80, "%d°");
    ImGui::EndDisabled();
    changed |= a.choice("questresolution", s.questResolution,
                        {"questeco", "questbalanced", "questsharp", "questwide"});
    changed |= a.tick("questtouch", s.questTouch);
    changed |= a.slider("questhaptics", s.questHaptics, 0, 100, "%d%%");
    changed |= a.tick("questflip", s.questFlip);
    if (a.button("questrecenter"))
        recenter();
    a.nextButton("questcinema");
    if (a.button("questcinema")) {
        s.questMode = 0;
        s.questPassthrough = false;
        s.questWidth = 320;
        s.questDistance = 240;
        s.questHeight = 0;
        s.questTilt = 0;
        s.questAnchor = 0;
        changed = true;
    }
    a.nextButton("questtabletop");
    if (a.button("questtabletop")) {
        s.questMode = 0;
        s.questWidth = 220;
        s.questDistance = 140;
        s.questHeight = -60;
        s.questTilt = 65;
        s.questAnchor = 0;
        s.view = 2;
        changed = true;
    }
    a.nextButton("questpersonal");
    if (a.button("questpersonal")) {
        s.questMode = 0;
        s.questAnchor = 1;
        s.questWidth = 140;
        s.questDistance = 100;
        s.questHeight = s.questTilt = 0;
        changed = true;
    }
    ImGui::TextWrapped("%s", tr("questgrab", a.language));
    ImGui::TextWrapped("%s", tr("questqualitynote", a.language));
    if (changed)
        a.dirty = true;
}
} // namespace scu
#endif
