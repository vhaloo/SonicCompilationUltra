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
const char *label(touch::Action a, bool pinball, int language) {
    using A = touch::Action;
    switch (a) {
    case A::A:
        return pinball ? "L" : "A";
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
    app.mobileRewind = false;
    scroll = 0;
    if (pointerDown)
        ImGui::GetIO().AddMouseButtonEvent(0, false);
    pointerDown = false;
}
void MobilePresentation::beginFrame() {
    frameStart = SDL_GetTicks64();
    SDL_SetRenderTarget(app.renderer, nullptr);
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(app.renderer, &w, &h);
    w = std::max(1, w);
    h = std::max(1, h);
    float left = 0, top = 0, right = 0, bottom = 0, newDensity = density;
    {
        Activity a;
        if (a.type) {
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
            if (result)
                a.env->DeleteLocalRef(result);
        }
    }
    bool controls = app.settings.mobileTouch == 1 ||
                    (app.settings.mobileTouch == 0 &&
                     !(app.pads[0] && SDL_GameControllerGetAttached(app.pads[0])));
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
    options.pinball = app.core.currentId == "spinball";
    touch::Rect safe{left / density, top / density, std::max(1.f, (w - left - right) / density),
                     std::max(1.f, (h - top - bottom) / density)};
    auto next = touch::arrange(safe, options);
    // Moving system bars, safe areas or a changed setting must release old captures.
    if (next.canvas.x != layout.canvas.x || next.canvas.y != layout.canvas.y ||
        next.canvas.w != layout.canvas.w || next.canvas.h != layout.canvas.h ||
        next.stick.x != layout.stick.x || next.stick.y != layout.stick.y ||
        next.stick.w != layout.stick.w)
        clearInput();
    layout = std::move(next);
    float renderScale = 1;
    // Pixel mode is literal 1:1. Overview/panorama may trade resolution for battery life.
    if (!app.menu && s.view != 0) {
        float budget = s.mobileRenderQuality == 0   ? 1000000.f
                       : s.mobileRenderQuality == 2 ? 8000000.f
                                                    : 2500000.f;
        renderScale =
            std::min(1.f, std::sqrt(budget / std::max(1.f, layout.canvas.w * layout.canvas.h *
                                                               density * density)));
    }
    int tw = std::max(1, int(layout.canvas.w * density * renderScale));
    int th = std::max(1, int(layout.canvas.h * density * renderScale));
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
    int config = s.mobileOrientation + 4 * s.mobileRefresh + 32 * int(s.mobileImmersive);
    if (config != displayConfig) {
        Activity a;
        if (a.type)
            a.env->CallVoidMethod(
                a.object, a.env->GetMethodID(a.type, "configureDisplay", "(IIZ)V"),
                s.mobileOrientation, s.mobileRefresh, s.mobileImmersive ? JNI_TRUE : JNI_FALSE);
        displayConfig = config;
    }
    SDL_SetRenderTarget(app.renderer, canvas);
}
void MobilePresentation::logicalSize(int &w, int &h) const {
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
void MobilePresentation::action(touch::Action a) {
    using A = touch::Action;
    if (a == A::Menu)
        app.toggleMenu();
    else if (a == A::Back) {
        app.backRequested = true;
    } else if (a == A::Previous || a == A::Next) {
        if (!app.guideOpen && !app.actionPopup && !app.browsing())
            app.showSection((app.menuTab() + App::sectionCount() + (a == A::Next ? 1 : -1)) %
                            App::sectionCount());
    } else if (a == A::Overview) {
        app.settings.view = app.settings.view == 2 ? 0 : 2;
        app.dirty = true;
    } else if (a == A::ZoomIn || a == A::ZoomOut) {
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
        // App-owned streaming textures are recreated on the next upload.
        if (canvas)
            SDL_DestroyTexture(canvas);
        canvas = nullptr;
        if (app.worldTexture)
            SDL_DestroyTexture(app.worldTexture);
        app.worldTexture = nullptr;
        if (app.nativeTexture)
            SDL_DestroyTexture(app.nativeTexture);
        app.nativeTexture = nullptr;
        if (app.bgTexture)
            SDL_DestroyTexture(app.bgTexture);
        app.bgTexture = nullptr;
        app.backgroundStage = -1;
        app.lastImageVersion = ~uint64_t(0);
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
        auto a = input.down(id, p, layout);
        if (a != touch::Action::None) {
            action(a);
            return true;
        }
        if (app.menu && !pointerDown && layout.canvas.contains(p)) {
            pointer = id;
            pointerDown = true;
            io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
            io.AddMousePosEvent(p.x - layout.canvas.x, p.y - layout.canvas.y);
            io.AddMouseButtonEvent(0, true);
        }
    } else {
        input.move(id, p);
        if (pointerDown && id == pointer) {
            io.AddMousePosEvent(p.x - layout.canvas.x, p.y - layout.canvas.y);
            if (e.type == SDL_FINGERUP) {
                io.AddMouseButtonEvent(0, false);
                pointerDown = false;
            }
        }
        if (e.type == SDL_FINGERUP)
            input.up(id);
    }
    return true;
}
void MobilePresentation::poll() {
    auto s = input.state(layout, app.settings.mobileDeadzone / 100.f);
    app.mobileButtons = s.buttons;
    app.mobileRewind = s.rewind;
    scroll = s.scroll;
}
void MobilePresentation::drawControls() {
    ImDrawList draw(ImGui::GetDrawListSharedData());
    draw._ResetForNewFrame();
    draw.PushTextureID(ImGui::GetIO().Fonts->TexID);
    draw.PushClipRect({0, 0}, {width / density, height / density});
    auto state = input.state(layout, app.settings.mobileDeadzone / 100.f);
    int alpha = app.menu ? 245 : app.settings.mobileOpacity * 255 / 100;
    auto panel = IM_COL32(12, 27, 46, alpha),
         edge = IM_COL32(84, 204, 221, std::min(255, alpha + 50));
    auto white = IM_COL32(235, 248, 255, std::min(255, alpha + 65));
    auto font = ImGui::GetFont();
    auto text = [&](const touch::Rect &r, const char *s, float size = 15.f) {
        auto extent = font->CalcTextSizeA(size, 10000, 0, s);
        draw.AddText(font, size, {r.x + (r.w - extent.x) / 2, r.y + (r.h - extent.y) / 2}, white,
                     s);
    };
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
        draw.AddCircleFilled(center, radius, panel, 48);
        draw.AddCircle(center, radius, edge, 48, 2);
        draw.AddLine({center.x - radius * .6f, center.y}, {center.x + radius * .6f, center.y}, edge,
                     2);
        draw.AddLine({center.x, center.y - radius * .6f}, {center.x, center.y + radius * .6f}, edge,
                     2);
        draw.AddCircleFilled(
            {center.x + state.stick.x * radius * .55f, center.y + state.stick.y * radius * .55f},
            radius * .3f, edge, 32);
    }
    for (const auto &b : layout.buttons) {
        auto r = b.rect;
        draw.AddRectFilled({r.x, r.y}, {r.x + r.w, r.y + r.h}, panel, 10);
        draw.AddRect({r.x, r.y}, {r.x + r.w, r.y + r.h}, edge, 10, 0, 1.5f);
        text(r, label(b.action, app.core.currentId == "spinball", app.language),
             r.w < 44 ? 14.f : 15.f);
    }
    if (!app.menu && layout.deck && app.settings.mobileHints) {
        auto r = layout.canvas;
        draw.AddText(font, 12, {r.x + 6, r.y + r.h + 6}, white,
                     tr(app.core.currentId == "spinball" ? "mobilepinballhint" : "mobiletouchhint",
                        app.language));
    }
    draw.PopClipRect();
    draw.PopTextureID();
    ImDrawData data;
    data.Valid = true;
    data.DisplayPos = {0, 0};
    data.DisplaySize = {width / density, height / density};
    data.FramebufferScale = {density, density};
    data.AddDrawList(&draw);
    ImGui_ImplSDLRenderer2_RenderDrawData(&data, app.renderer);
}
void MobilePresentation::endFrame() {
    SDL_SetRenderTarget(app.renderer, nullptr);
    SDL_SetRenderDrawColor(app.renderer, 4, 10, 20, 255);
    SDL_RenderClear(app.renderer);
    SDL_Rect dest{int(layout.canvas.x * density), int(layout.canvas.y * density),
                  int(layout.canvas.w * density), int(layout.canvas.h * density)};
    SDL_RenderCopy(app.renderer, canvas, nullptr, &dest);
    drawControls();
    SDL_RenderPresent(app.renderer);
    const int rates[] = {60, 30, 90, 120, 0};
    double rate = rates[std::clamp(app.settings.mobileRefresh, 0, 4)];
    if (rate == 0)
        rate = displayHz;
    if (!app.focused)
        rate = 15;
    auto elapsed = SDL_GetTicks64() - frameStart;
    if (rate > 0 && elapsed < 1000.0 / rate)
        SDL_Delay(Uint32(std::ceil(1000.0 / rate - elapsed)));
}
void MobilePresentation::chooseRom() {
    Activity a;
    if (a.type)
        a.env->CallVoidMethod(a.object, a.env->GetMethodID(a.type, "chooseRom", "()V"));
}
void MobilePresentation::options() {
    auto &s = app.settings;
    bool changed = false;
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
