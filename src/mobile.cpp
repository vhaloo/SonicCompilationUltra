#include "mobile.h"
#ifdef SCU_MOBILE
#include "ui.h"
#include <SDL_system.h>
#include <jni.h>
#include <imgui_internal.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_impl_sdl2.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <atomic>
#include <cstdio>
#include <mutex>

namespace {
std::atomic<float> crownDelta{0};
std::mutex motionMutex;
scu::motion::Sample motionSample;
}
extern "C" JNIEXPORT void JNICALL
Java_com_vhaloo_soniccompilationultra_mobile_MobileActivity_nativeMotion(
    JNIEnv *, jclass, jfloat x,jfloat y,jfloat z,jfloat w,jfloat gx,jfloat gy,jfloat gz,
    jfloat ax,jfloat ay,jfloat az,jint source,jint rotation) {
    std::lock_guard<std::mutex> lock(motionMutex);
    motionSample={{x,y,z,w},{gx,gy,gz},{ax,ay,az},SDL_GetTicks64(),source,rotation};
}
extern "C" JNIEXPORT void JNICALL
Java_com_vhaloo_soniccompilationultra_mobile_MobileActivity_nativeRotary(JNIEnv *, jclass,
                                                                         jfloat value) {
    if (!std::isfinite(value))
        return;
    float old = crownDelta.load(std::memory_order_relaxed);
    while (!crownDelta.compare_exchange_weak(old, std::clamp(old + value, -60.f, 60.f),
                                             std::memory_order_relaxed)) {
    }
}

namespace scu {
namespace {
struct Activity {
    JNIEnv *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
    jobject object = static_cast<jobject>(SDL_AndroidGetActivity());
    jclass type = object && env ? env->GetObjectClass(object) : nullptr;
    ~Activity() {
        if (env) {
            if (type)
                env->DeleteLocalRef(type);
            if (object)
                env->DeleteLocalRef(object);
        }
    }
};
const char *label(touch::Action a, touch::Profile profile, int language) {
    using A = touch::Action;
    const bool pinball = profile == touch::Profile::Pinball;
    switch (a) {
    case A::A:
        return pinball ? "L" : profile == touch::Profile::Jump ? tr("mobilejump", language) : "A";
    case A::B:
        return pinball ? "R" : "B";
    case A::C:
        return pinball ? "L+R" : "C";
    case A::Start:
        return "Start";
    case A::Rewind:
        return "<<";
    case A::Menu:
        return tr("mobilemenu", language);
    case A::Overview:
        return tr("mobilemap", language);
    case A::ZoomIn:
        return "+";
    case A::ZoomOut:
        return "-";
    case A::Back:
        return tr("mobileback", language);
    case A::Previous:
        return "<";
    case A::Next:
        return ">";
    case A::ScrollUp:
        return tr("mobileup", language);
    case A::ScrollDown:
        return tr("mobiledown", language);
    case A::Utility:
        return "...";
    default:
        return "";
    }
}
} // namespace
MobilePresentation::MobilePresentation(App &a) : app(a) {
    app.mobile = this;
}
MobilePresentation::~MobilePresentation() {
    clearInput();
    if (canvas)
        SDL_DestroyTexture(canvas);
    app.mobile = nullptr;
}
void MobilePresentation::clearInput() {
    input.clear();
    app.mobileButtons = 0;
    app.motionButtons = 0;
    motionController.recenter();
    motionState = {};
    app.mobileRewind = false;
    scroll = 0;
    if (pointerDown)
        ImGui::GetIO().AddMouseButtonEvent(0, false);
    pointerDown = false;
    pointerScrolling = false;
}
void MobilePresentation::beginFrame() {
    frameStart = SDL_GetTicks64();
    SDL_SetRenderTarget(app.renderer, nullptr);
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(app.renderer, &w, &h);
    w = std::max(1, w);
    h = std::max(1, h);
    float left = safeLeft, top = safeTop, right = safeRight, bottom = safeBottom,
          newDensity = density;
    if (w != width || h != height || !metricsAt || SDL_GetTicks64() - metricsAt >= 250) {
        metricsAt = SDL_GetTicks64();
        Activity a;
        if (a.type) {
            availableMotion = a.env->CallIntMethod(a.object,
                a.env->GetMethodID(a.type, "motionSource", "()I"));
            auto method = a.env->GetMethodID(a.type, "displayInfo", "()[I");
            auto result = static_cast<jintArray>(a.env->CallObjectMethod(a.object, method));
            if (result && a.env->GetArrayLength(result) >= 7) {
                jint v[7];
                a.env->GetIntArrayRegion(result, 0, 7, v);
                newDensity = std::clamp(v[0] / 1000.f, .75f, 6.f);
                float fx = v[5] > 0 ? float(w) / v[5] : 1, fy = v[6] > 0 ? float(h) / v[6] : 1;
                left = v[1] * fx;
                top = v[2] * fy;
                right = v[3] * fx;
                bottom = v[4] * fy;
                if (a.env->GetArrayLength(result) >= 8) {
                    jint hz = 60000;
                    a.env->GetIntArrayRegion(result, 7, 1, &hz);
                    displayHz = std::clamp(hz / 1000.f, 30.f, 240.f);
                }
            }
            if (result && a.env->GetArrayLength(result) >= 9) {
                jint thermal = 0;
                a.env->GetIntArrayRegion(result, 8, 1, &thermal);
                app.thermalStatus = std::clamp(int(thermal), 0, 6);
            }
            if (result && a.env->GetArrayLength(result) >= 10) {
                jint flags = 0;
                a.env->GetIntArrayRegion(result, 9, 1, &flags);
                app.settings.wearActive = (flags & 1) != 0;
                app.settings.wearRound = (flags & 2) != 0;
                appIsWatch = app.settings.wearActive;
                crownDetected = (flags & 4) != 0;
            }
            if (result)
                a.env->DeleteLocalRef(result);
        }
    }
    safeLeft = left;
    safeTop = top;
    safeRight = right;
    safeBottom = bottom;
    // An attached controller can be an idle or virtual Android input device.
    // Auto follows the input the player actually uses.
    const bool usingPad =
        app.padUsed && app.pads[0] && SDL_GameControllerGetAttached(app.pads[0]);
    bool controls = app.settings.mobileTouch == 1 ||
                    (app.settings.mobileTouch == 0 && !usingPad);
    if (w != width || h != height || density != newDensity || wasMenu != app.menu ||
        wasControls != controls) {
        clearInput();
        width = w;
        height = h;
        density = newDensity;
        wasMenu = app.menu;
        wasControls = controls;
    }
    auto &s = app.settings;
    if (s.wearActive && !s.wearFillMigrated) {
        s.wearFisheye = true;
        s.wearFillMigrated = true;
        app.dirty = true;
    }
    const bool sensorActive = s.motionMode && app.focused &&
        ((app.core.loaded && !app.menu) || app.controlsMenu());
    const int sensorConfig = int(sensorActive) + 2 * int(s.motionFlick);
    if (motionConfig != sensorConfig) {
        Activity a;
        if (a.type)
            a.env->CallVoidMethod(a.object,
                a.env->GetMethodID(a.type, "configureMotion", "(ZZ)V"),
                sensorActive ? JNI_TRUE : JNI_FALSE, s.motionFlick ? JNI_TRUE : JNI_FALSE);
        motionConfig = sensorConfig;
        motionController.recenter();
    }
    touch::Options options;
    options.menu = app.menu;
    options.controls = controls;
    options.layout = s.mobileLayout;
    options.leftHanded = s.mobileLeftHanded;
    options.scale = s.mobileTouchScale / 100.f;
    options.margin = float(s.mobileSafeMargin);
    options.stickX = float(s.mobileStickX);
    options.stickY = float(s.mobileStickY);
    options.buttonsX = float(s.mobileButtonsX);
    options.buttonsY = float(s.mobileButtonsY);
    options.profile = s.mobileActions ? touch::Profile::ABC :
        touch::profileForGame(app.core.currentId,
            app.core.currentId == "compilation" ?
                app.core.world.inspect(app.core.snapshot, app.core.game).family : 0);
    options.watch = s.wearActive;
    options.circular = s.wearRound;
    options.utilityBank = utilityBank;
    touch::Rect safe{left / density, top / density, std::max(1.f, (w - left - right) / density),
                     std::max(1.f, (h - top - bottom) / density)};
    uint64_t nextKey = 1469598103934665603ull;
    for (int value : {w,
                      h,
                      int(density * 1000),
                      int(left * 1000),
                      int(top * 1000),
                      int(right * 1000),
                      int(bottom * 1000),
                      int(app.menu),
                      int(controls),
                      s.mobileLayout,
                      int(s.mobileLeftHanded),
                      s.mobileTouchScale,
                      s.mobileSafeMargin,
                      s.mobileStickX,
                      s.mobileStickY,
                      s.mobileButtonsX,
                      s.mobileButtonsY,
                      int(options.profile),
                      int(options.watch),
                      int(options.circular),
                      int(utilityBank)}) {
        nextKey = (nextKey ^ uint32_t(value)) * 1099511628211ull;
    }
    if (nextKey != layoutKey) {
        auto next = touch::arrange(safe, options);
        // Moving system bars, safe areas or a changed setting must release old captures.
        if (next.canvas.x != layout.canvas.x || next.canvas.y != layout.canvas.y ||
            next.canvas.w != layout.canvas.w || next.canvas.h != layout.canvas.h ||
            next.stick.x != layout.stick.x || next.stick.y != layout.stick.y ||
            next.stick.w != layout.stick.w)
            clearInput();
        layout = std::move(next);
        layoutKey = nextKey;
    }
    // Only the world sampler is adaptive. UI and native overlays render at the
    // drawable resolution and are not blurred by a low-resolution scene target.
    float renderScale = 1;
    int tw = std::max(1, int(layout.canvas.w * density * renderScale));
    int th = std::max(1, int(layout.canvas.h * density * renderScale));
    if (s.wearActive && !app.menu)
        th = std::max(1, int(double(tw) * app.core.nativeH / std::max(1, app.core.nativeW)));
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(app.renderer, &info);
    float maxScale =
        std::min({1.f, info.max_texture_width > 0 ? float(info.max_texture_width) / tw : 1.f,
                  info.max_texture_height > 0 ? float(info.max_texture_height) / th : 1.f});
    tw = std::max(1, int(tw * maxScale));
    th = std::max(1, int(th * maxScale));
    if (!canvas || textureW != tw || textureH != th) {
        if (canvas)
            SDL_DestroyTexture(canvas);
        canvas = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                   tw, th);
        if (!canvas)
            throw std::runtime_error(SDL_GetError());
        textureW = tw;
        textureH = th;
        SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(canvas, SDL_ScaleModeNearest);
    }
    int config = s.mobileOrientation + 4 * s.mobileRefresh + 32 * int(s.mobileImmersive) +
                 s.performanceTarget * 64;
    if (config != displayConfig) {
        Activity a;
        if (a.type)
            a.env->CallVoidMethod(a.object,
                                  a.env->GetMethodID(a.type, "configureDisplay", "(IIIZ)V"),
                                  s.mobileOrientation, s.mobileRefresh, s.performanceTarget,
                                  s.mobileImmersive ? JNI_TRUE : JNI_FALSE);
        displayConfig = config;
    }
    SDL_SetRenderTarget(app.renderer, canvas);
}
void MobilePresentation::logicalSize(int &w, int &h) const {
    if (app.settings.wearActive && !app.menu) {
        w = std::max(1, int(textureW / density));
        h = std::max(1, int(textureH / density));
        return;
    }
    w = std::max(1, int(layout.canvas.w));
    h = std::max(1, int(layout.canvas.h));
}
void MobilePresentation::prepareUi() {
    auto &io = ImGui::GetIO();
    int w, h;
    logicalSize(w, h);
    io.DisplaySize = {float(w), float(h)};
    io.DisplayFramebufferScale = {float(textureW) / w, float(textureH) / h};
}
float MobilePresentation::topControlsBottom() const {
    if (app.menu || app.settings.wearActive)
        return 0.f;
    for (const auto &button : layout.buttons)
        if (button.action == touch::Action::Menu)
            return std::max(0.f, button.rect.y + button.rect.h - layout.canvas.y + 6.f);
    return 0.f;
}
void MobilePresentation::action(touch::Action a) {
    using A = touch::Action;
    if (a == A::Utility) {
        utilityBank = !utilityBank;
        clearInput();
    } else if (a == A::Menu)
        app.toggleMenu();
    else if (a == A::Back) {
        app.backRequested = true;
    } else if (a == A::Previous || a == A::Next) {
        if (!app.guideOpen && !app.actionPopup && !app.browsing())
            app.changeSection(a == A::Next ? 1 : -1);
    } else if (a == A::Overview) {
        if (app.settings.wearActive)
            app.settings.wearZoom = 0;
        else
            app.settings.view = app.settings.view == 2 ? 0 : 2;
        app.dirty = true;
    } else if (a == A::ZoomIn || a == A::ZoomOut) {
        if (app.settings.wearActive) {
            app.settings.wearZoom =
                int(wear::crownZoom(app.settings.wearZoom, a == A::ZoomIn ? 2 : -2,
                                    app.settings.wearCrownSpeed, false));
            app.dirty = true;
            zoomHintUntil = SDL_GetTicks64() + 1400;
            return;
        }
        app.settings.view = 0;
        app.settings.stepZoom(a == A::ZoomIn ? 1 : -1);
        app.dirty = true;
    }
}
bool MobilePresentation::event(const SDL_Event &e) {
    if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                                      e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        clearInput();
    if (e.type == SDL_APP_WILLENTERBACKGROUND || e.type == SDL_APP_DIDENTERBACKGROUND) {
        clearInput();
        app.focused = false;
        app.core.clearAudio();
        app.dirty = true;
        app.flushSettings();
    }
    if (e.type == SDL_APP_DIDENTERFOREGROUND) {
        app.focused = true;
        displayConfig = -1;
    }
    if (e.type == SDL_RENDER_TARGETS_RESET || e.type == SDL_RENDER_DEVICE_RESET) {
        if (canvas)
            SDL_DestroyTexture(canvas);
        canvas = nullptr;
        clearInput();
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_AC_BACK) {
        if (app.menu)
            app.backRequested = true;
        else
            app.toggleMenu();
        return true;
    }
    if ((e.type == SDL_MOUSEMOTION && e.motion.which == SDL_TOUCH_MOUSEID) ||
        ((e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) &&
         e.button.which == SDL_TOUCH_MOUSEID))
        return true;
    if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
        SDL_Event mouse = e;
        int ww = 1, wh = 1;
        SDL_GetWindowSize(app.window, &ww, &wh);
        if (e.type == SDL_MOUSEMOTION) {
            mouse.motion.x =
                int(e.motion.x * float(width) / std::max(1, ww) / density - layout.canvas.x);
            mouse.motion.y =
                int(e.motion.y * float(height) / std::max(1, wh) / density - layout.canvas.y);
        } else {
            mouse.button.x =
                int(e.button.x * float(width) / std::max(1, ww) / density - layout.canvas.x);
            mouse.button.y =
                int(e.button.y * float(height) / std::max(1, wh) / density - layout.canvas.y);
        }
        ImGui_ImplSDL2_ProcessEvent(&mouse);
        app.padUsed = false;
        return true;
    }
    if (e.type != SDL_FINGERDOWN && e.type != SDL_FINGERMOTION && e.type != SDL_FINGERUP)
        return false;
    auto id = e.tfinger.fingerId;
    touch::Point p{e.tfinger.x * width / density, e.tfinger.y * height / density};
    auto &io = ImGui::GetIO();
    if (e.type == SDL_FINGERDOWN) {
        app.padUsed = false;
        auto a = input.down(id, p, layout);
        if (a != touch::Action::None) {
            action(a);
            return true;
        }
        if (app.menu && !pointerDown && layout.canvas.contains(p)) {
            pointer = id;
            pointerDown = true;
            pointerScrolling = false;
            pointerStart = pointerLast = p;
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(p.x - layout.canvas.x, p.y - layout.canvas.y);
            io.AddMouseButtonEvent(0, true);
        }
    } else {
        input.move(id, p);
        if (pointerDown && id == pointer) {
            const float dx = p.x - pointerStart.x, dy = p.y - pointerStart.y;
            if (app.menu && app.settings.wearActive && !pointerScrolling &&
                std::abs(dy) > 6.f && std::abs(dy) > std::abs(dx)) {
                pointerScrolling = true;
                ImGui::ClearActiveID();
                io.AddMouseButtonEvent(0, false);
            }
            if (pointerScrolling) {
                io.AddMousePosEvent(pointerStart.x - layout.canvas.x,
                                   pointerStart.y - layout.canvas.y);
                io.AddMouseWheelEvent(0, (p.y - pointerLast.y) /
                                            std::max(1.f, ImGui::GetFontSize() * 5.f));
            } else
                io.AddMousePosEvent(p.x - layout.canvas.x, p.y - layout.canvas.y);
            pointerLast = p;
            if (e.type == SDL_FINGERUP) {
                io.AddMouseButtonEvent(0, false);
                pointerDown = false;
                pointerScrolling = false;
            }
        }
        if (e.type == SDL_FINGERUP)
            input.up(id);
    }
    return true;
}
void MobilePresentation::poll() {
    float rotary = crownDelta.exchange(0, std::memory_order_relaxed);
    if (rotary && app.settings.wearActive) {
        crownDetected = true;
        if (app.menu) {
            int w, h;
            logicalSize(w, h);
            if (!pointerDown)
                ImGui::GetIO().AddMousePosEvent(w * .5f, h * .5f);
            ImGui::GetIO().AddMouseWheelEvent(0, rotary);
        } else {
            app.settings.wearZoom =
                int(wear::crownZoom(app.settings.wearZoom, rotary, app.settings.wearCrownSpeed,
                                    app.settings.wearCrownReverse));
            app.dirty = true;
            zoomHintUntil = SDL_GetTicks64() + 1400;
        }
    }
    uint64_t now = SDL_GetTicks64();
    if (!zoomInitialised || app.settings.reduceMotion)
        smoothedZoom = app.settings.wearZoom;
    else {
        double dt = std::clamp(double(now - zoomAt) / 1000., 0., .1);
        smoothedZoom += (app.settings.wearZoom - smoothedZoom) * (1. - std::exp(-dt / .065));
        if (std::abs(smoothedZoom - app.settings.wearZoom) < .4)
            smoothedZoom = app.settings.wearZoom;
    }
    zoomInitialised = true;
    zoomAt = now;
    app.settings.wearZoomValue = smoothedZoom;
    auto s = input.state(layout, app.settings.mobileDeadzone / 100.f);
    app.mobileButtons = s.buttons;
    app.mobileRewind = s.rewind;
    scroll = s.scroll;
    const auto &settings = app.settings;
    motionController.configure({settings.motionMode,settings.motionSensitivity,settings.motionDeadzone,
        settings.motionFlickThreshold,settings.motionFlick,settings.motionInvertX,settings.motionInvertY});
    motion::Sample sample;
    { std::lock_guard<std::mutex> lock(motionMutex); sample=motionSample; }
    if (app.focused && ((!app.menu && app.core.loaded) || app.controlsMenu())) {
        motionController.update(sample);
        motionState=motionController.state(SDL_GetTicks64());
        app.motionButtons=app.menu?0:motionState.directions;
        if (!app.menu && motionState.flick)
            app.motionButtons|=uint16_t(1u << (app.core.currentId=="spinball"?8:9));
    } else {
        motionController.recenter();motionState={};app.motionButtons=0;
    }
}
void MobilePresentation::drawControls() {
    if (!controlsDraw)
        controlsDraw = std::make_unique<ImDrawList>(ImGui::GetDrawListSharedData());
    auto &draw = *controlsDraw;
    draw._ResetForNewFrame();
    draw.PushTextureID(ImGui::GetIO().Fonts->TexID);
    draw.PushClipRect({0, 0}, {width / density, height / density});
    auto state = input.state(layout, app.settings.mobileDeadzone / 100.f);
    const float opacity = app.menu ? 1.f : app.settings.mobileOpacity * .01f;
    const auto white = IM_COL32(248, 248, 248, int(216 * opacity));
    const auto shadow = IM_COL32(0, 0, 0, int(190 * opacity));
    auto font = ImGui::GetFont();
    auto text = [&](const touch::Rect &r, const char *s, float size = 12.f) {
        auto extent = font->CalcTextSizeA(size, 10000, 0, s);
        if (extent.x > r.w * .8f && extent.x > 0) {
            size *= std::max(.5f, r.w * .8f / extent.x);
            extent = font->CalcTextSizeA(size, 10000, 0, s);
        }
        ImVec2 p{r.x + (r.w - extent.x) / 2, r.y + (r.h - extent.y) / 2};
        draw.AddText(font, size, {p.x + .5f, p.y + .5f}, shadow, s);
        draw.AddText(font, size, p, white, s);
    };
    auto ring = [&](ImVec2 center, float radius, bool pressed) {
        const float a = pressed ? std::min(1.f, opacity + .25f) : opacity;
        draw.AddCircleFilled(center, radius, IM_COL32(0, 0, 0, int((pressed ? 22 : 5) * a)), 40);
        draw.AddCircle(center, radius, IM_COL32(0, 0, 0, int(156 * a)), 40, 1.8f);
        draw.AddCircle(center, radius, IM_COL32(248, 248, 248, int(216 * a)), 40, .65f);
    };
    // Keep overview statistics flat and compact, after the lens. Native zoom
    // already contains the cartridge HUD; drawing a second copy would clutter it.
    if (app.settings.wearActive && !app.menu && app.core.loaded &&
        app.settings.hud != 3 && app.settings.wearZoomValue < 999.5) {
        auto scene=app.core.world.inspect(app.core.snapshot,app.core.game);
        const float screenW=width/density,screenH=height/density;
        const float size=app.settings.hudScale?std::clamp(app.settings.hudScale*4.f,7.f,18.f):8.5f;
        float cx=screenW*.5f,cy=screenH*.17f;
        if(app.settings.hud==2)cx=screenW*.39f;
        if(app.settings.hud==1 && scene.ready && app.core.world.canvas.coverageW>0 &&
           app.core.world.canvas.coverageH>0) {
            const auto &c=app.core.world.canvas;
            const float u=std::clamp(float((scene.playerX-c.left)/c.coverageW),0.f,1.f);
            const float v=std::clamp(float((scene.playerY-c.top)/c.coverageH),0.f,1.f);
            const auto point=lens.project(u,v);
            cx=std::clamp(point.x/density,screenW*.3f,screenW*.7f);
            cy=std::clamp(point.y/density-30.f,screenH*.17f,screenH*.72f);
        }
        if(scene.ready && scene.family==4 && app.nativeFrameTexture()) {
            const float w=screenW*.66f,h=w*16/app.core.nativeW;
            draw.AddImage(reinterpret_cast<ImTextureID>(app.nativeFrameTexture()),
                {cx-w*.5f,cy},{cx+w*.5f,cy+h},{0,0},{1,16.f/app.core.nativeH});
        } else if(scene.ready) {
            const auto &r=app.core.snapshot.ram;
            char first[96]{},second[96]{};
            if(scene.family==5)
                std::snprintf(first,sizeof first,"%s %u   %s %u",tr("rings",app.language),
                    unsigned(be16(r,0x0A5A)),tr("lives",app.language),unsigned(be16(r,0x0680)));
            else {
                const unsigned score=((uint32_t(be16(r,0xFE26))<<16)|be16(r,0xFE28))*10;
                std::snprintf(first,sizeof first,"%06u   %02u:%02u",score,unsigned(r[0xFE23]),unsigned(r[0xFE24]));
                std::snprintf(second,sizeof second,"%s %u   %s %u",tr("rings",app.language),
                    unsigned(be16(r,0xFE20)),tr("lives",app.language),unsigned(r[0xFE12]));
            }
            for(int line=0;line<2;++line) {
                const char *caption=line?second:first;if(!*caption)continue;
                auto extent=font->CalcTextSizeA(size,10000,0,caption);
                const ImVec2 p{cx-extent.x*.5f,cy+line*(size+2)};
                draw.AddText(font,size,{p.x+.6f,p.y+.6f},IM_COL32(0,0,0,235),caption);
                draw.AddText(font,size,p,IM_COL32(248,248,248,235),caption);
            }
        }
    }
    if (layout.deck) {
        auto c = layout.canvas;
        auto safe = layout.safe;
        draw.AddRectFilled({safe.x, c.y + c.h}, {safe.x + safe.w, safe.y + safe.h},
                           IM_COL32(5, 13, 25, 255));
        draw.AddLine({safe.x, c.y + c.h}, {safe.x + safe.w, c.y + c.h}, IM_COL32(40, 84, 107, 255));
    }
    if (layout.stick.w > 0) {
        auto r = layout.stick;
        ImVec2 center{r.x + r.w / 2, r.y + r.h / 2};
        float radius = r.w / 2;
        const bool pressed = std::hypot(state.stick.x, state.stick.y) > .05f;
        ring(center, radius - 1, pressed);
        for (int direction = 0; direction < 4; ++direction) {
            const float angle = direction * 1.570796327f;
            auto point = [&](float x, float y) {
                return ImVec2{center.x + x * std::cos(angle) - y * std::sin(angle),
                              center.y + x * std::sin(angle) + y * std::cos(angle)};
            };
            const ImVec2 arrow[] = {point(-radius * .14f, -radius * .40f),
                                    point(0, -radius * .64f),
                                    point(radius * .14f, -radius * .40f)};
            draw.AddPolyline(arrow, 3, shadow, 0, 1.8f);
            draw.AddPolyline(arrow, 3, white, 0, .7f);
        }
        ImVec2 thumb{center.x + state.stick.x * radius * .55f,
                     center.y + state.stick.y * radius * .55f};
        if (pressed)
            ring(thumb, radius * .23f, true);
        draw.AddCircleFilled(thumb, .8f, white, 12);
    }
    for (const auto &b : layout.buttons) {
        auto r = b.rect;
        const int bit = b.action == touch::Action::A ? 9 : b.action == touch::Action::B ? 0 :
                        b.action == touch::Action::C ? 8 : b.action == touch::Action::Start ? 3 : -1;
        const bool pressed = bit >= 0 ? (state.buttons & (1u << bit)) != 0 :
                             b.action == touch::Action::Rewind && state.rewind;
        if (b.circular)
            ring({r.x + r.w * .5f, r.y + r.h * .5f}, std::min(r.w, r.h) * .5f - 1, pressed);
        else {
            draw.AddRect({r.x, r.y}, {r.x + r.w, r.y + r.h}, shadow, 9, 0, 1.8f);
            draw.AddRect({r.x, r.y}, {r.x + r.w, r.y + r.h}, white, 9, 0, .65f);
        }
        const char *caption = app.settings.wearActive && b.action == touch::Action::Menu ? "..." :
                              app.settings.wearActive && b.action == touch::Action::Utility ? "+" :
                              label(b.action, layout.profile, app.language);
        text(r, caption, r.w < 35 ? 10.f : 12.f);
    }
    if (!app.menu && layout.deck && app.settings.mobileHints) {
        auto r = layout.canvas;
        draw.AddText(font, 12, {r.x + 6, r.y + r.h + 6}, white,
                     tr(app.core.currentId == "spinball" ? "mobilepinballhint" : "mobiletouchhint",
                        app.language));
    }
    if (!app.menu && app.settings.wearActive && zoomHintUntil > SDL_GetTicks64()) {
        char hint[80];
        std::snprintf(hint, sizeof hint, "%s %d%%", tr("zoom", app.language),
                      app.settings.wearZoom / 10);
        float size = 12.f;
        auto ext = font->CalcTextSizeA(size, 10000, 0, hint);
        auto c = layout.canvas;
        draw.AddText(font, size, {c.x + (c.w - ext.x) / 2, c.y + c.h * .21f}, white, hint);
    }
    draw.PopClipRect();
    draw.PopTextureID();
    ImDrawData data;
    data.Valid = true;
    data.DisplayPos = {0, 0};
    data.DisplaySize = {width / density, height / density};
    data.FramebufferScale = {density, density};
    data.AddDrawList(&draw);
    app.renderImGui(&data);
}
void MobilePresentation::endFrame() {
    const auto submitStarted = SDL_GetPerformanceCounter();
    SDL_SetRenderTarget(app.renderer, nullptr);
    SDL_SetRenderDrawColor(app.renderer, 4, 10, 20, 255);
    SDL_RenderClear(app.renderer);
    SDL_Rect dest{int(layout.canvas.x * density), int(layout.canvas.y * density),
                  int(layout.canvas.w * density), int(layout.canvas.h * density)};
    if (app.settings.wearActive && !app.menu) {
        float blend = app.settings.wearFisheye ? 1.f : 0.f;
        lens.update(dest.w, dest.h, float(textureW) / textureH, app.settings.wearRound, blend,
                    app.settings.wearLensStrength * .0125f);
        SDL_Rect oldViewport;
        SDL_RenderGetViewport(app.renderer, &oldViewport);
        SDL_RenderSetViewport(app.renderer, &dest);
        SDL_Color white{255, 255, 255, 255};
        auto &v = lens.vertices;
        SDL_SetTextureScaleMode(canvas, blend > .01f ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
        SDL_RenderGeometryRaw(app.renderer, canvas, &v[0].x, sizeof(wear::Vertex), &white, 0,
                              &v[0].u, sizeof(wear::Vertex), int(v.size()), lens.indices.data(),
                              int(lens.indices.size()), sizeof(int));
        SDL_RenderSetViewport(app.renderer, &oldViewport);
    } else
        SDL_RenderCopy(app.renderer, canvas, nullptr, &dest);
    drawControls();
    SDL_RenderFlush(app.renderer);
    lastSubmissionMs =
        double(SDL_GetPerformanceCounter() - submitStarted) * 1000 / SDL_GetPerformanceFrequency();
    const auto swapStarted = SDL_GetPerformanceCounter();
    SDL_RenderPresent(app.renderer);
    app.lastPresentMs =
        double(SDL_GetPerformanceCounter() - swapStarted) * 1000 / SDL_GetPerformanceFrequency();
    double rate = refreshLimit();
    if (!app.focused)
        rate = 15;
    if (double delay =
            pacing.delay(double(SDL_GetPerformanceCounter()) / SDL_GetPerformanceFrequency(), rate);
        delay > 0)
        SDL_Delay(Uint32(std::ceil(delay * 1000)));
}
double MobilePresentation::refreshLimit() const {
    const double rates[] = {double(app.settings.performanceTarget), 30, 90, 120, 0};
    double rate = rates[std::clamp(app.settings.mobileRefresh, 0, 4)];
    return std::min(rate > 0 ? rate : double(displayHz), double(displayHz));
}
bool MobilePresentation::chooseRom() {
    Activity a;
    if (!a.type)
        return false;
    if (a.env->CallBooleanMethod(a.object, a.env->GetMethodID(a.type, "chooseRom", "()Z")))
        return true;
    auto path = static_cast<jstring>(a.env->CallObjectMethod(
        a.object, a.env->GetMethodID(a.type, "romInboxPath", "()Ljava/lang/String;")));
    if (path) {
        const char *text = a.env->GetStringUTFChars(path, nullptr);
        if (text) {
            app.settings.lastDirectory = text;
            a.env->ReleaseStringUTFChars(path, text);
        }
        a.env->DeleteLocalRef(path);
    }
    return false;
}
void MobilePresentation::inputOptions() {
    auto &s=app.settings;
    bool changed=false;
    if(s.advancedOptions)
        changed|=app.choice("mobileactions",s.mobileActions,{"mobileactionauto","mobileactionabc"});
    changed|=app.choice("motioncontrol",s.motionMode,{"off","motionhorizontal","motiontwod"});
    if(s.motionMode) {
        ImGui::TextWrapped("%s",tr(availableMotion==1?"motionfused":
            availableMotion==2?"motiongravity":availableMotion==3?"motionaccelerometer":"motionunavailable",app.language));
        changed|=app.tick("motionflick",s.motionFlick);
        if(app.button("motioncenter"))motionController.recenter();
        ImGui::TextWrapped("%s",tr("motioncenterhelp",app.language));
        const float side=std::min(94.f,ImGui::GetContentRegionAvail().x);
        auto p=ImGui::GetCursorScreenPos();
        ImVec2 center{p.x+side*.5f,p.y+side*.5f};
        auto *draw=ImGui::GetWindowDrawList();
        draw->AddCircle(center,side*.4f,IM_COL32(140,157,181,150),40,1);
        const float neutral=s.motionDeadzone*.01f*side*.4f;
        draw->AddCircle(center,neutral,IM_COL32(140,157,181,100),24,1);
        const ImVec2 indicator{center.x+motionState.x*side*.38f,center.y+motionState.y*side*.38f};
        draw->AddLine(center,indicator,IM_COL32(123,218,213,220),2);
        draw->AddCircleFilled(indicator,4,IM_COL32(123,218,213,230),16);
        ImGui::Dummy({side,side});
        if(s.advancedOptions) {
            changed|=app.slider("motionsensitivity",s.motionSensitivity,40,240,"%d%%");
            changed|=app.slider("motiondeadzone",s.motionDeadzone,5,50,"%d%%");
            if(s.motionFlick)
                changed|=app.slider("motionflickthreshold",s.motionFlickThreshold,3,20,"%d m/s2");
            changed|=app.tick("motioninvertx",s.motionInvertX);
            changed|=app.tick("motioninverty",s.motionInvertY);
        }
    }
    if(!s.advancedOptions)
        changed|=app.slider("mobileopacity",s.mobileOpacity,20,100,"%d%%");
    if(changed)app.dirty=true;
}
void MobilePresentation::options() {
    auto &s = app.settings;
    bool changed = false;
    if (s.wearActive) {
        ImGui::TextWrapped("%s",
                           tr(crownDetected ? "wearcrownready" : "wearcrownauto", app.language));
        changed |= app.slider("wearzoom", s.wearZoom, 0, 1000, "%d / 1000");
        if (app.button("wearwholelevel")) {
            s.wearZoom = 0;
            changed = true;
        }
        app.nextButton("wearnative");
        if (app.button("wearnative")) {
            s.wearZoom = 1000;
            changed = true;
        }
        changed |= app.tick("wearfisheye", s.wearFisheye);
        changed |= app.slider("wearlensstrength", s.wearLensStrength, 0, 100, "%d%%");
        changed |= app.choice("wearcrownspeed", s.wearCrownSpeed, {"slow", "balanced", "fast"});
        changed |= app.tick("wearcrownreverse", s.wearCrownReverse);
        ImGui::TextWrapped("%s", tr("wearcontrolhelp", app.language));
    }
    ImGui::TextWrapped("%s", tr("mobileintro", app.language));
    changed |= app.choice("mobilelayout", s.mobileLayout, {"auto", "mobileoverlay", "mobiledeck"});
    changed |= app.choice("mobileorientation", s.mobileOrientation,
                          {"auto", "mobilelandscape", "mobileportrait"});
    changed |=
        app.choice("mobiletouch", s.mobileTouch, {"mobileautopad", "mobilealways", "mobilehidden"});
    changed |= app.slider("mobiletouchscale", s.mobileTouchScale, 65, 160, "%d%%");
    changed |= app.slider("mobileopacity", s.mobileOpacity, 20, 100, "%d%%");
    changed |= app.slider("mobiledeadzone", s.mobileDeadzone, 10, 65, "%d%%");
    changed |= app.slider("mobilesafemargin", s.mobileSafeMargin, 4, 40, "%d dp");
    changed |= app.tick("mobilelefthanded", s.mobileLeftHanded);
    changed |= app.tick("mobilehints", s.mobileHints);
    changed |= app.tick("mobileimmersive", s.mobileImmersive);
    changed |= app.choice("mobilerefresh", s.mobileRefresh,
                          {"mobile60", "mobile30", "mobile90", "mobile120", "mobilemax"});
    changed |= app.choice("mobilerenderquality", s.mobileRenderQuality,
                          {"mobilebattery", "mobilebalanced", "mobilesharp"});
    changed |= app.slider("mobilestickx", s.mobileStickX, -160, 160, "%d dp");
    changed |= app.slider("mobilesticky", s.mobileStickY, -160, 160, "%d dp");
    changed |= app.slider("mobilebuttonsx", s.mobileButtonsX, -160, 160, "%d dp");
    changed |= app.slider("mobilebuttonsy", s.mobileButtonsY, -160, 160, "%d dp");
    if (app.button("mobilereset")) {
        s.mobileLayout = 0;
        s.mobileTouch = 0;
        s.mobileTouchScale = 100;
        s.mobileOpacity = 70;
        s.mobileSafeMargin = 8;
        s.mobileLeftHanded = false;
        s.mobileDeadzone = 25;
        s.mobileStickX = s.mobileStickY = s.mobileButtonsX = s.mobileButtonsY = 0;
        changed = true;
    }
    ImGui::TextWrapped("%s", tr("mobilehelp", app.language));
    app.dirty |= changed;
}
} // namespace scu
#endif
