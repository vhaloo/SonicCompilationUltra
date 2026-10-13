#include "ui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <chrono>
#include <ctime>
namespace scu {
void App::control(const char *key) {
#ifdef SCU_UI_TESTS
    controls.emplace(key, ImGui::GetItemID());
    auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
    controlRects.emplace(key, ImVec4(min.x, min.y, max.x, max.y));
#endif
}
bool App::button(const char *key, const ImVec2 &size) {
    std::string label = std::string(tr(key, language)) + "###" + key;
    bool result = ImGui::Button(label.c_str(), size);
    control(key);
    return result;
}
void App::nextButton(const char *key) {
    float width = ImGui::CalcTextSize(tr(key, language)).x + ImGui::GetStyle().FramePadding.x * 2;
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= right)
        ImGui::SameLine();
}
bool App::tick(const char *key, bool &value) {
    ImGui::BeginGroup();
    bool changed = ImGui::Checkbox((std::string("##") + key).c_str(), &value);
    control(key);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::EndGroup();
    return changed;
}
bool App::choice(const char *key, int &value, std::initializer_list<const char *> keys) {
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::SetNextItemWidth(-1);
    std::vector<const char *> labels;
    for (auto k : keys)
        labels.push_back(tr(k, language));
    bool changed =
        ImGui::Combo((std::string("##") + key).c_str(), &value, labels.data(), int(labels.size()));
    control(key);
    return changed;
}
bool App::slider(const char *key, int &value, int lo, int hi, const char *format) {
    ImGui::TextWrapped("%s", tr(key, language));
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::SliderInt((std::string("##") + key).c_str(), &value, lo, hi, format,
                                    ImGuiSliderFlags_NoInput);
    control(key);
    return changed;
}
void App::initialise(const fs::path &base, bool hidden) {
    basePath = base;
    fs::create_directories(library.dataDir);
    settings.load(library.dataDir);
    library.load();
    language = detectLanguage(settings.language);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    window = SDL_CreateWindow("Sonic Compilation Ultra", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1280, 720,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI |
                                  (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    if (!window)
        throw std::runtime_error(SDL_GetError());
    SDL_SetWindowMinimumSize(window, 320, 240);
    renderer = SDL_CreateRenderer(window, -1,
                                  settings.renderer
                                      ? SDL_RENDERER_SOFTWARE
                                      : SDL_RENDERER_ACCELERATED |
                                            (settings.vsync ? SDL_RENDERER_PRESENTVSYNC : 0));
    if (!renderer)
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
        throw std::runtime_error(SDL_GetError());
    SDL_RendererInfo rendererInfo{};
    SDL_GetRendererInfo(renderer, &rendererInfo);
    fprintf(stderr, "SCU renderer: %s; max texture %d x %d\n", rendererInfo.name,
            rendererInfo.max_texture_width, rendererInfo.max_texture_height);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavEscapeClearFocusItem = false;
    io.ConfigNavEscapeClearFocusWindow = false;
    io.IniFilename = nullptr;
    static ImVector<ImWchar> ranges;
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    static const ImWchar punctuation[] = {0x2000, 0x206F, 0};
    builder.AddRanges(punctuation);
    builder.AddChar(0x2212);
    builder.AddChar(0x00D7);
    builder.BuildRanges(&ranges);
    ImFontConfig fontCfg;
    auto font = (basePath / "assets/SCUInterface.ttf").u8string();
    if (fs::exists(fs::u8path(font)))
        io.Fonts->AddFontFromFileTTF(font.c_str(), 24, &fontCfg, ranges.Data);
    else
        io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 16;
    style.FrameRounding = 8;
    style.ChildRounding = 10;
    style.PopupRounding = 8;
    style.WindowPadding = {22, 18};
    style.FramePadding = {10, 8};
    style.ItemSpacing = {12, 10};
    style.ScrollbarSize = 13;
    style.Colors[ImGuiCol_WindowBg] = {0.025f, 0.047f, 0.08f, 0.98f};
    style.Colors[ImGuiCol_ChildBg] = {0.04f, 0.075f, 0.12f, 0.96f};
    style.Colors[ImGuiCol_FrameBg] = {0.075f, 0.12f, 0.18f, 1};
    style.Colors[ImGuiCol_Button] = {0.04f, 0.29f, 0.43f, 1};
    style.Colors[ImGuiCol_ButtonHovered] = {0.07f, 0.49f, 0.65f, 1};
    style.Colors[ImGuiCol_ButtonActive] = {0.05f, 0.56f, 0.65f, 1};
    style.Colors[ImGuiCol_CheckMark] = {0.28f, 0.89f, 0.85f, 1};
    style.Colors[ImGuiCol_SliderGrab] = {0.2f, 0.78f, 0.87f, 1};
    style.Colors[ImGuiCol_Header] = {0.05f, 0.28f, 0.4f, 1};
    style.Colors[ImGuiCol_TabSelected] = {0.06f, 0.36f, 0.49f, 1};
    style.Colors[ImGuiCol_Text] = {0.87f, 0.94f, 0.98f, 1};
    style.Colors[ImGuiCol_NavCursor] = {1, 0.85f, 0.25f, 1};
    baseStyle = style;
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    refreshControllers();
    if (!hidden)
        applyDisplay();
}
App::~App() {
    try {
        flushSettings();
    } catch (...) {
    }
    core.unload();
    for (auto p : pads)
        if (p)
            SDL_GameControllerClose(p);
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
    if (worldTexture)
        SDL_DestroyTexture(worldTexture);
    if (nativeTexture)
        SDL_DestroyTexture(nativeTexture);
    if (bgTexture)
        SDL_DestroyTexture(bgTexture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
}
void App::refreshControllers() {
    // The UI and cartridge input share the same owned controller handles.
    ImGui_ImplSDL2_SetGamepadMode(ImGui_ImplSDL2_GamepadMode_Manual, nullptr, 0);
    for (auto &p : pads)
        if (p && !SDL_GameControllerGetAttached(p)) {
            SDL_GameControllerClose(p);
            p = nullptr;
        }
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i))
            continue;
        auto instance = SDL_JoystickGetDeviceInstanceID(i);
        bool already = false;
        for (auto p : pads)
            if (p && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p)) == instance)
                already = true;
        if (already)
            continue;
        for (auto &p : pads)
            if (!p) {
                p = SDL_GameControllerOpen(i);
                break;
            }
    }
    syncNavigationController();
}
void App::syncNavigationController() {
    if (!pads[activePad])
        activePad = pads[0] ? 0 : 1;
}
void App::updateNavigationController() {
    auto &io = ImGui::GetIO();
    auto pad = pads[activePad];
    static const ImGuiKey keys[] = {ImGuiKey_GamepadFaceDown,
                                    ImGuiKey_GamepadFaceRight,
                                    ImGuiKey_GamepadFaceLeft,
                                    ImGuiKey_GamepadFaceUp,
                                    ImGuiKey_None,
                                    ImGuiKey_None,
                                    ImGuiKey_GamepadStart,
                                    ImGuiKey_GamepadL3,
                                    ImGuiKey_GamepadR3,
                                    ImGuiKey_GamepadL1,
                                    ImGuiKey_GamepadR1,
                                    ImGuiKey_GamepadDpadUp,
                                    ImGuiKey_GamepadDpadDown,
                                    ImGuiKey_GamepadDpadLeft,
                                    ImGuiKey_GamepadDpadRight};
    bool held = false;
    for (int n = 0; n < int(sizeof keys / sizeof keys[0]); ++n)
        if (keys[n] != ImGuiKey_None && pad &&
            SDL_GameControllerGetButton(pad, SDL_GameControllerButton(n)))
            held = true;
    if (!held)
        releaseNavigation = false;
    bool enabled = menu && pad && !releaseNavigation;
    if (pad)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    for (int n = 0; n < int(sizeof keys / sizeof keys[0]); ++n)
        if (keys[n] != ImGuiKey_None)
            io.AddKeyEvent(
                keys[n], enabled && SDL_GameControllerGetButton(pad, SDL_GameControllerButton(n)));
    auto analog = [&](ImGuiKey key, int value, int dead) {
        float amount = enabled ? std::clamp(float(value - dead) / (32767 - dead), 0.0f, 1.0f) : 0;
        io.AddKeyAnalogEvent(key, amount > 0, amount);
    };
    int x = pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) : 0,
        y = pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) : 0;
    analog(ImGuiKey_GamepadLStickLeft, -x, settings.deadzone);
    analog(ImGuiKey_GamepadLStickRight, x, settings.deadzone);
    analog(ImGuiKey_GamepadLStickUp, -y, settings.deadzone);
    analog(ImGuiKey_GamepadLStickDown, y, settings.deadzone);
    analog(ImGuiKey_GamepadL2,
           pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT) : 0, 3000);
    analog(ImGuiKey_GamepadR2,
           pad ? SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) : 0, 3000);
}
void App::toggleMenu() {
    if (!core.loaded) {
        menu = true;
        focusMenu = true;
        return;
    }
    menu = !menu;
    focusMenu = menu;
    releaseNavigation = menu;
    releaseInput = !menu;
    pendingButtons[0] = pendingButtons[1] = 0;
    core.clearAudio();
    dirty = true;
}
void App::applyDisplay() {
    if (!previousFullscreen && settings.fullscreen) {
        SDL_GetWindowPosition(window, &windowedBounds.x, &windowedBounds.y);
        SDL_GetWindowSize(window, &windowedBounds.w, &windowedBounds.h);
    }
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowBordered(window, settings.fullscreen ? SDL_FALSE : SDL_TRUE);
    if (settings.fullscreen) {
        SDL_Rect r{};
        int display = settings.display < 0 ? SDL_GetWindowDisplayIndex(window) : settings.display;
        display = std::clamp(display, 0, std::max(0, SDL_GetNumVideoDisplays() - 1));
        SDL_GetDisplayBounds(std::max(0, display), &r);
        if (settings.span) {
            for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
                SDL_Rect b{}, u;
                SDL_GetDisplayBounds(i, &b);
                SDL_UnionRect(&r, &b, &u);
                r = u;
            }
        }
        SDL_SetWindowPosition(window, r.x, r.y);
        SDL_SetWindowSize(window, r.w, r.h);
    } else if (previousFullscreen) {
        SDL_Rect bounds{};
        SDL_GetDisplayUsableBounds(std::max(0, SDL_GetWindowDisplayIndex(window)), &bounds);
        SDL_SetWindowSize(window, std::max(320, std::min(windowedBounds.w, bounds.w)),
                          std::max(240, std::min(windowedBounds.h, bounds.h)));
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    previousFullscreen = settings.fullscreen;
    SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
    dirty = true;
}
void App::error(const std::exception &e) {
    notice = std::string(tr("error", language)) + ": " + e.what();
}
void App::import(const fs::path &path, bool folder) {
    try {
        int n = folder ? library.importFolder(path) : library.importFile(path);
        notice = tr(n ? "imported" : "noimport", language);
        if (n)
            notice += " (" + std::to_string(n) + ")";
        settings.lastDirectory = (folder ? path : path.parent_path()).u8string();
        dirty = true;
    } catch (const std::exception &e) {
        error(e);
    }
}
bool App::play(const std::string &id) {
    if (!core.load(id)) {
        notice = std::string(tr("error", language)) + ": " + core.status;
        return false;
    }
    menu = false;
    releaseInput = true;
    dirty = true;
    notice.clear();
    backgroundStage = -1;
    backgroundFrozen = false;
    return true;
}
void App::beginBrowser(bool folder) {
    folderMode = folder;
    browserOpen = true;
    focusBrowser = true;
    browsePath =
        settings.lastDirectory.empty() ? fs::current_path().u8string() : settings.lastDirectory;
    std::error_code ec;
    if (!fs::is_directory(fs::u8path(browsePath), ec))
        browsePath = fs::current_path().u8string();
    snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
}
void App::flushSettings() {
    if (dirty) {
        settings.save(library.dataDir);
        dirty = false;
    }
}
void App::event(const SDL_Event &e) {
    ImGui_ImplSDL2_ProcessEvent(&e);
    if (e.type == SDL_QUIT) {
        if (settings.confirmQuit && core.loaded) {
            requestAction(1);
        } else
            running = false;
    }
    if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_CONTROLLERDEVICEREMOVED)
        refreshControllers();
    if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
            focused = true;
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            focused = false;
            pendingButtons[0] = pendingButtons[1] = 0;
            core.clearAudio();
        }
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            backgroundStage = -1;
        }
    }
    if (e.type == SDL_DROPFILE) {
        if (e.drop.file) {
            import(fs::u8path(e.drop.file));
            SDL_free(e.drop.file);
        }
        menu = true;
        tab = 0;
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN ||
        (e.type == SDL_CONTROLLERAXISMOTION && std::abs(int(e.caxis.value)) > settings.deadzone)) {
        padUsed = true;
        for (int p = 0; p < 2; ++p)
            if (pads[p] &&
                SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[p])) == e.cdevice.which)
                activePad = p;
        syncNavigationController();
    }
    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_KEYDOWN)
        padUsed = false;
    if (menu && !releaseNavigation &&
        (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP)) {
        // Preserve taps that begin and end between display frames. The platform
        // backend also polls held buttons; ImGui deduplicates those state events.
        static const ImGuiKey keys[] = {ImGuiKey_GamepadFaceDown,
                                        ImGuiKey_GamepadFaceRight,
                                        ImGuiKey_GamepadFaceLeft,
                                        ImGuiKey_GamepadFaceUp,
                                        ImGuiKey_None,
                                        ImGuiKey_None,
                                        ImGuiKey_GamepadStart,
                                        ImGuiKey_GamepadL3,
                                        ImGuiKey_GamepadR3,
                                        ImGuiKey_GamepadL1,
                                        ImGuiKey_GamepadR1,
                                        ImGuiKey_GamepadDpadUp,
                                        ImGuiKey_GamepadDpadDown,
                                        ImGuiKey_GamepadDpadLeft,
                                        ImGuiKey_GamepadDpadRight};
        auto button = e.cbutton.button;
        if (button < sizeof keys / sizeof keys[0] && keys[button] != ImGuiKey_None)
            ImGui::GetIO().AddKeyEvent(keys[button], e.type == SDL_CONTROLLERBUTTONDOWN);
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        auto b = e.cbutton.button;
        if (b == SDL_CONTROLLER_BUTTON_BACK) {
            if (menu)
                selectRequested = true;
            else
                toggleMenu();
            return;
        }
        if (menu && b == SDL_CONTROLLER_BUTTON_B)
            backRequested = true;
        if (!menu && core.loaded && settings.quickControls && pads[activePad] &&
            SDL_GameControllerGetButton(pads[activePad], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            auto scene = core.world.inspect(core.snapshot, core.game);
            if (b == SDL_CONTROLLER_BUTTON_Y)
                settings.view = settings.view == 2 ? 0 : 2;
            else if (b == SDL_CONTROLLER_BUTTON_DPAD_UP || b == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                settings.view = 0;
                settings.zoom =
                    std::clamp(settings.zoom + (b == SDL_CONTROLLER_BUTTON_DPAD_UP ? 1 : -1), 1, 8);
            } else if (b == SDL_CONTROLLER_BUTTON_X) {
                if (settings.camera == 2)
                    settings.camera = 0;
                else {
                    if (settings.view == 2) {
                        settings.view = 0;
                        settings.left = scene.playerX - drawableW / settings.zoom / 2;
                        settings.top = scene.playerY - drawableH / settings.zoom / 2;
                        settings.camera = 2;
                    } else
                        core.world.freeze(scene, settings);
                }
            } else if (b == SDL_CONTROLLER_BUTTON_B) {
                settings.view = 0;
                settings.camera = 1;
            } else if (b == SDL_CONTROLLER_BUTTON_START)
                screenshotRequested = true;
            dirty = true;
            pendingButtons[0] = pendingButtons[1] = 0;
            return;
        }
        if (!menu && core.loaded && !releaseInput) {
            int bits[] = {RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_B,
                          RETRO_DEVICE_ID_JOYPAD_A};
            if (settings.jumpLayout == 1) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_Y;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_B;
            } else if (settings.jumpLayout == 2) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_B;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_Y;
            }
            int bit = b == SDL_CONTROLLER_BUTTON_X       ? bits[0]
                      : b == SDL_CONTROLLER_BUTTON_A     ? bits[1]
                      : b == SDL_CONTROLLER_BUTTON_B     ? bits[2]
                      : b == SDL_CONTROLLER_BUTTON_START ? RETRO_DEVICE_ID_JOYPAD_START
                                                         : -1;
            if (bit >= 0)
                pendingButtons[activePad] |= uint16_t(1u << bit);
        }
    }
    if (e.type == SDL_KEYDOWN && !e.key.repeat) {
        auto k = e.key.keysym.sym;
        auto mod = e.key.keysym.mod;
        if (k == SDLK_ESCAPE || k == SDLK_F10) {
            if (menu) {
                if (k == SDLK_F10)
                    selectRequested = true;
                else
                    backRequested = true;
            } else if (core.loaded)
                toggleMenu();
            else
                menu = true;
            return;
        }
        if (k == SDLK_RETURN && (mod & KMOD_ALT)) {
            settings.fullscreen = !settings.fullscreen;
            applyDisplay();
            return;
        }
        if (!menu && core.loaded) {
            // Keep a short press until the next native tick, even if the display
            // loop observes its down and up events within one refresh.
            auto scan = e.key.keysym.scancode;
            auto tap = [&](int player, SDL_Scancode expected, unsigned bit) {
                if (scan == expected)
                    pendingButtons[player] |= uint16_t(1u << bit);
            };
            if (!(mod & (KMOD_CTRL | KMOD_ALT))) {
                tap(0, SDL_SCANCODE_UP, RETRO_DEVICE_ID_JOYPAD_UP);
                tap(0, SDL_SCANCODE_DOWN, RETRO_DEVICE_ID_JOYPAD_DOWN);
                tap(0, SDL_SCANCODE_LEFT, RETRO_DEVICE_ID_JOYPAD_LEFT);
                tap(0, SDL_SCANCODE_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT);
                tap(0, SDL_SCANCODE_Z, RETRO_DEVICE_ID_JOYPAD_Y);
                tap(0, SDL_SCANCODE_X, RETRO_DEVICE_ID_JOYPAD_B);
                tap(0, SDL_SCANCODE_SPACE, RETRO_DEVICE_ID_JOYPAD_B);
                tap(0, SDL_SCANCODE_C, RETRO_DEVICE_ID_JOYPAD_A);
                tap(0, SDL_SCANCODE_RETURN, RETRO_DEVICE_ID_JOYPAD_START);
                tap(1, SDL_SCANCODE_I, RETRO_DEVICE_ID_JOYPAD_UP);
                tap(1, SDL_SCANCODE_K, RETRO_DEVICE_ID_JOYPAD_DOWN);
                tap(1, SDL_SCANCODE_J, RETRO_DEVICE_ID_JOYPAD_LEFT);
                tap(1, SDL_SCANCODE_L, RETRO_DEVICE_ID_JOYPAD_RIGHT);
                tap(1, SDL_SCANCODE_U, RETRO_DEVICE_ID_JOYPAD_Y);
                tap(1, SDL_SCANCODE_O, RETRO_DEVICE_ID_JOYPAD_B);
                tap(1, SDL_SCANCODE_P, RETRO_DEVICE_ID_JOYPAD_A);
                tap(1, SDL_SCANCODE_RSHIFT, RETRO_DEVICE_ID_JOYPAD_START);
            }
            if (k >= SDLK_F1 && k <= SDLK_F9) {
                try {
                    if (mod & KMOD_SHIFT)
                        core.save((int)(k - SDLK_F1 + 1));
                    else
                        core.loadState((int)(k - SDLK_F1 + 1));
                } catch (const std::exception &e) {
                    error(e);
                }
                return;
            }
            if (k == SDLK_F12) {
                screenshotRequested = true;
                return;
            }
            if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) {
                settings.view = 0;
                settings.zoom = std::min(8, settings.zoom + 1);
                dirty = true;
            }
            if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                settings.view = 0;
                settings.zoom = std::max(1, settings.zoom - 1);
                dirty = true;
            }
            if (k == SDLK_f) {
                core.world.freeze(core.world.inspect(core.snapshot, core.game), settings);
                dirty = true;
            }
            if (k == SDLK_r) {
                settings.camera = 0;
                dirty = true;
            }
            if (k == SDLK_m) {
                settings.minimap = !settings.minimap;
                dirty = true;
            }
            if (k == SDLK_h) {
                settings.hud = (settings.hud + 1) % 4;
                dirty = true;
            }
            if ((mod & KMOD_CTRL) && k >= SDLK_1 && k <= SDLK_5) {
                int n = k - SDLK_1;
                settings.view = n == 3 ? 1 : n == 4 ? 2 : 0;
                settings.zoom = n < 3 ? n + 1 : settings.zoom;
                dirty = true;
            }
        }
    }
    if (e.type == SDL_MOUSEWHEEL && !menu && (SDL_GetModState() & KMOD_CTRL)) {
        settings.view = 0;
        settings.zoom = std::clamp(settings.zoom + (e.wheel.y > 0 ? 1 : -1), 1, 8);
        dirty = true;
    }
}
void App::pollInput() {
    const Uint8 *k = SDL_GetKeyboardState(nullptr);
    core.buttons[0] = core.buttons[1] = 0;
    if (menu || (!focused && !headless))
        return;
    core.buttons[0] = pendingButtons[0];
    core.buttons[1] = pendingButtons[1];
    auto set = [&](int p, bool on, int bit) {
        if (on)
            core.buttons[p] |= 1 << bit;
    };
    set(0, k[SDL_SCANCODE_UP], RETRO_DEVICE_ID_JOYPAD_UP);
    set(0, k[SDL_SCANCODE_DOWN], RETRO_DEVICE_ID_JOYPAD_DOWN);
    set(0, k[SDL_SCANCODE_LEFT], RETRO_DEVICE_ID_JOYPAD_LEFT);
    set(0, k[SDL_SCANCODE_RIGHT], RETRO_DEVICE_ID_JOYPAD_RIGHT);
    set(0, k[SDL_SCANCODE_Z], RETRO_DEVICE_ID_JOYPAD_Y);
    set(0, k[SDL_SCANCODE_X] || k[SDL_SCANCODE_SPACE], RETRO_DEVICE_ID_JOYPAD_B);
    set(0, k[SDL_SCANCODE_C], RETRO_DEVICE_ID_JOYPAD_A);
    set(0, k[SDL_SCANCODE_RETURN], RETRO_DEVICE_ID_JOYPAD_START);
    set(1, k[SDL_SCANCODE_I], RETRO_DEVICE_ID_JOYPAD_UP);
    set(1, k[SDL_SCANCODE_K], RETRO_DEVICE_ID_JOYPAD_DOWN);
    set(1, k[SDL_SCANCODE_J], RETRO_DEVICE_ID_JOYPAD_LEFT);
    set(1, k[SDL_SCANCODE_L], RETRO_DEVICE_ID_JOYPAD_RIGHT);
    set(1, k[SDL_SCANCODE_U], RETRO_DEVICE_ID_JOYPAD_Y);
    set(1, k[SDL_SCANCODE_O], RETRO_DEVICE_ID_JOYPAD_B);
    set(1, k[SDL_SCANCODE_P], RETRO_DEVICE_ID_JOYPAD_A);
    set(1, k[SDL_SCANCODE_RSHIFT], RETRO_DEVICE_ID_JOYPAD_START);
    for (int p = 0; p < 2; p++)
        if (pads[p]) {
            auto pad = pads[p];
            int dx = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX),
                dy = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY),
                dead = settings.deadzone;
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) || dy < -dead,
                RETRO_DEVICE_ID_JOYPAD_UP);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || dy > dead,
                RETRO_DEVICE_ID_JOYPAD_DOWN);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || dx < -dead,
                RETRO_DEVICE_ID_JOYPAD_LEFT);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || dx > dead,
                RETRO_DEVICE_ID_JOYPAD_RIGHT);
            int bits[] = {RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_B,
                          RETRO_DEVICE_ID_JOYPAD_A};
            if (settings.jumpLayout == 1) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_Y;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_B;
            } else if (settings.jumpLayout == 2) {
                bits[0] = RETRO_DEVICE_ID_JOYPAD_A;
                bits[1] = RETRO_DEVICE_ID_JOYPAD_B;
                bits[2] = RETRO_DEVICE_ID_JOYPAD_Y;
            }
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X), bits[0]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A), bits[1]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B), bits[2]);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START),
                RETRO_DEVICE_ID_JOYPAD_START);
            if (settings.quickControls &&
                SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
                core.buttons[p] = 0;
        }
    if (releaseInput) {
        if (!(core.buttons[0] | core.buttons[1]))
            releaseInput = false;
        core.buttons[0] = core.buttons[1] = 0;
        pendingButtons[0] = pendingButtons[1] = 0;
    }
}
void App::pan() {
    if (menu || settings.camera != 3)
        return;
    auto k = SDL_GetKeyboardState(nullptr);
    int speed = settings.panSpeed * ((k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 3 : 1);
    settings.left += (k[SDL_SCANCODE_D] - k[SDL_SCANCODE_A]) * speed;
    settings.top += (k[SDL_SCANCODE_S] - k[SDL_SCANCODE_W]) * speed;
    if (pads[0]) {
        int dx = SDL_GameControllerGetAxis(pads[0], SDL_CONTROLLER_AXIS_RIGHTX),
            dy = SDL_GameControllerGetAxis(pads[0], SDL_CONTROLLER_AXIS_RIGHTY);
        if (std::abs(dx) > settings.deadzone)
            settings.left += dx * speed / 32767;
        if (std::abs(dy) > settings.deadzone)
            settings.top += dy * speed / 32767;
    }
    settings.left = std::clamp(settings.left, -32768, 32768);
    settings.top = std::clamp(settings.top, -16384, 16384);
}
void App::present(bool capture) {
    SDL_GetRendererOutputSize(renderer, &drawableW, &drawableH);
    SDL_SetRenderDrawColor(renderer, 6, 12, 24, 255);
    SDL_RenderClear(renderer);
    Scene scene = core.world.inspect(core.snapshot, core.game);
    uint64_t hash = 1469598103934665603ull;
    for (int v :
         {drawableW, drawableH, settings.view, settings.zoom, settings.camera, settings.left,
          settings.top, settings.background, settings.blur, settings.brightness,
          settings.overviewStyle, (int)settings.parallax, (int)settings.reduceMotion}) {
        hash ^= (uint32_t)v;
        hash *= 1099511628211ull;
    }
    bool refresh = core.imageVersion != lastImageVersion || hash != lastPresentationHash;
    if (refresh) {
        cachedExpanded =
            core.loaded && core.world.compose(core.snapshot, core.game, settings, drawableW,
                                              drawableH, &core.native, core.nativeW, core.nativeH);
        lastImageVersion = core.imageVersion;
        lastPresentationHash = hash;
    }
    bool expanded = core.loaded && cachedExpanded;
    if (core.loaded) {
        bool fixed =
            !settings.parallax || settings.reduceMotion ||
            (scene.ready && (settings.camera == 2 || settings.camera == 4 || settings.view == 2));
        if (refresh)
            core.world.makeBackdrop(core.snapshot, scene, settings, core.native, core.nativeW,
                                    core.nativeH);
        if (fixed && backgroundFrozen && backgroundStage == scene.stage)
            core.world.backdrop = frozenBackground;
        else {
            frozenBackground = core.world.backdrop;
            backgroundFrozen = fixed;
            backgroundStage = scene.stage;
        }
        if (!bgTexture) {
            bgTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, 320, 224);
            SDL_SetTextureScaleMode(bgTexture, SDL_ScaleModeLinear);
        }
        SDL_UpdateTexture(bgTexture, nullptr, core.world.backdrop.data(), 320 * 4);
        if (settings.background == 2 && expanded) {
            int scale = std::max(1, (int)core.world.canvas.scale), tw = 320 * scale,
                th = 224 * scale;
            for (int y = 0; y < drawableH; y += th)
                for (int x = 0; x < drawableW; x += tw) {
                    SDL_Rect d{x, y, tw, th};
                    SDL_RenderCopy(renderer, bgTexture, nullptr, &d);
                }
        } else
            SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);
        if (expanded) {
            auto &c = core.world.canvas;
            if (!worldTexture || textureW != c.width || textureH != c.height) {
                if (worldTexture)
                    SDL_DestroyTexture(worldTexture);
                worldTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING, c.width, c.height);
                textureW = c.width;
                textureH = c.height;
                SDL_SetTextureBlendMode(worldTexture, SDL_BLENDMODE_BLEND);
                SDL_SetTextureScaleMode(worldTexture, SDL_ScaleModeNearest);
            }
            if (!worldTexture)
                throw std::runtime_error(SDL_GetError());
            if (refresh)
                SDL_UpdateTexture(worldTexture, nullptr, c.pixels.data(), c.width * 4);
            SDL_RenderCopy(renderer, worldTexture, nullptr, nullptr);
            if (settings.view == 2 && settings.playerInset && !core.native.empty()) {
                if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
                    if (nativeTexture)
                        SDL_DestroyTexture(nativeTexture);
                    nativeTexture =
                        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, core.nativeW, core.nativeH);
                    nativeTW = core.nativeW;
                    nativeTH = core.nativeH;
                }
                SDL_SetTextureScaleMode(nativeTexture, SDL_ScaleModeNearest);
                SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4);
                int safeW = std::min(drawableW, int(drawableH * 16.0 / 9)),
                    w = int(std::min(safeW * 0.28, drawableH * 0.26 * core.nativeW / core.nativeH)),
                    h = w * core.nativeH / core.nativeW, x = (drawableW + safeW) / 2 - w - 16,
                    y = drawableH - h - (settings.help ? std::max(48, int(drawableH * 0.065)) : 0) -
                        16;
                SDL_Rect border{x - 3, y - 3, w + 6, h + 6}, d{x, y, w, h};
                SDL_SetRenderDrawColor(renderer, 85, 206, 217, 255);
                SDL_RenderFillRect(renderer, &border);
                SDL_RenderCopy(renderer, nativeTexture, nullptr, &d);
            }
        } else if (!core.native.empty()) {
            if (!nativeTexture || nativeTW != core.nativeW || nativeTH != core.nativeH) {
                if (nativeTexture)
                    SDL_DestroyTexture(nativeTexture);
                nativeTexture =
                    SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, core.nativeW, core.nativeH);
                nativeTW = core.nativeW;
                nativeTH = core.nativeH;
            }
            SDL_SetTextureScaleMode(nativeTexture, settings.integerScale ? SDL_ScaleModeNearest
                                                                         : SDL_ScaleModeLinear);
            SDL_UpdateTexture(nativeTexture, nullptr, core.native.data(), core.nativeW * 4);
            int availableH = drawableH - (settings.help ? std::max(48, int(drawableH * 0.065)) : 0),
                safeW = std::min(drawableW, (int)(drawableH * 16.0 / 9));
            float scale = settings.nativeScale ? settings.nativeScale
                                               : std::min(float(safeW) / core.nativeW,
                                                          float(availableH) / core.nativeH);
            if (settings.integerScale && scale >= 1)
                scale = std::floor(scale);
            scale = std::min(
                scale, std::min(float(drawableW) / core.nativeW, float(availableH) / core.nativeH));
            SDL_Rect d{(drawableW - (int)(core.nativeW * scale)) / 2,
                       (availableH - (int)(core.nativeH * scale)) / 2, (int)(core.nativeW * scale),
                       (int)(core.nativeH * scale)};
            SDL_RenderCopy(renderer, nativeTexture, nullptr, &d);
        }
        if (settings.scanlines) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, settings.scanlines * 255 / 100);
            for (int y = 1; y < drawableH; y += 3)
                SDL_RenderDrawLine(renderer, 0, y, drawableW, y);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    updateNavigationController();
    bool editing = GImGui->ActiveId != 0 ||
                   (!GImGui->OpenPopupStack.empty() && GImGui->OpenPopupStack.back().Window &&
                    !(GImGui->OpenPopupStack.back().Window->Flags & ImGuiWindowFlags_Modal));
    auto &io = ImGui::GetIO();
    uiFactor = settings.uiScale == 1   ? 0.85f
               : settings.uiScale == 2 ? 1.3f
               : settings.uiScale == 3 ? 1.65f
                                       : std::clamp(io.DisplaySize.y / 720.0f, 0.85f, 2.0f);
    ImGui::GetStyle() = baseStyle;
    ImGui::GetStyle().ScaleAllSizes(uiFactor);
    if (settings.highContrast) {
        ImGui::GetStyle().Colors[ImGuiCol_Text] = {1, 1, 1, 1};
        ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = {0.02f, 0.04f, 0.08f, 1};
        ImGui::GetStyle().Colors[ImGuiCol_NavCursor] = {1, 1, 0, 1};
    }
    io.FontGlobalScale = 0.75f * uiFactor;
    ImGui::NewFrame();
    menuInput(editing);
#ifdef SCU_UI_TESTS
    controls.clear();
    controlRects.clear();
#endif
    if (expanded)
        hud();
    if (!menu && (settings.help || !notice.empty())) {
        auto &io = ImGui::GetIO();
        ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y - 6}, {}, {0.5f, 1});
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("##hint", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);
        if (io.DisplaySize.x < 600)
            ImGui::SetWindowFontScale(0.78f);
        if (button(io.DisplaySize.x < 600 ? "menushort" : "menuhint"))
            toggleMenu();
        if (settings.performance)
            ImGui::Text("%.1f fps · %d × %d", measuredFps, drawableW, drawableH);
        if (!notice.empty())
            ImGui::TextWrapped("%s", notice.c_str());
        ImGui::End();
    }
    if (menu)
        ui();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    if (capture || screenshotRequested) {
        fs::path p = capturePath.empty()
                         ? library.dataDir / "screenshots" /
                               (core.currentId + "-" + std::to_string(SDL_GetTicks64()) + ".bmp")
                         : fs::u8path(capturePath);
        screenshot(p);
        screenshotRequested = false;
    }
    SDL_RenderPresent(renderer);
}
void App::screenshot(const fs::path &path) {
    fs::create_directories(path.parent_path());
    auto surf =
        SDL_CreateRGBSurfaceWithFormat(0, drawableW, drawableH, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surf)
        throw std::runtime_error(SDL_GetError());
    if (SDL_RenderReadPixels(renderer, nullptr, surf->format->format, surf->pixels, surf->pitch) !=
        0) {
        SDL_FreeSurface(surf);
        throw std::runtime_error(SDL_GetError());
    }
    std::string p = path.u8string();
    auto rw = SDL_RWFromFile(p.c_str(), "wb");
    int result = rw ? SDL_SaveBMP_RW(surf, rw, 1) : -1;
    SDL_FreeSurface(surf);
    if (result)
        throw std::runtime_error(SDL_GetError());
    fprintf(stderr, "SCU capture: %d x %d\n", drawableW, drawableH);
}
void App::hud() {
    auto &io = ImGui::GetIO();
    auto v = core.world.inspect(core.snapshot, core.game);
    auto &c = core.world.canvas;
    float sx = (float)(io.DisplaySize.x / c.coverageW),
          sy = (float)(io.DisplaySize.y / c.coverageH), px = (v.playerX - c.left) * sx,
          py = (v.playerY - c.top) * sy;
    auto draw = ImGui::GetBackgroundDrawList();
    if (settings.locator) {
        float r = 18, mx = std::clamp(px, 24.0f, io.DisplaySize.x - 24),
              my = std::clamp(py, 32.0f, io.DisplaySize.y - 60);
        draw->AddCircle({mx, my}, r, IM_COL32(110, 242, 245, 210), 32, 2);
        draw->AddLine({mx, my - r - 5}, {mx, my - r - 16}, IM_COL32(240, 240, 240, 220), 2);
        if (mx != px || my != py) {
            float dx = px - mx, dy = py - my, len = std::sqrt(dx * dx + dy * dy);
            dx /= len;
            dy /= len;
            draw->AddTriangleFilled(
                {mx + dx * 24, my + dy * 24}, {mx + dx * 12 - dy * 6, my + dy * 12 + dx * 6},
                {mx + dx * 12 + dy * 6, my + dy * 12 - dx * 6}, IM_COL32(255, 230, 103, 255));
        }
        if (core.spinCharge) {
            draw->AddText({mx + 22, my - 10}, IM_COL32(255, 226, 103, 255), tr("charge", language));
            draw->AddRectFilled({mx + 22, my + 16}, {mx + 22 + core.spinCharge * 12.0f, my + 21},
                                IM_COL32(255, 222, 76, 255));
        }
    }
    if (settings.view == 2 &&
        (settings.overviewStyle == 2 || (settings.overviewStyle == 0 && v.family == 3)))
        draw->AddText(
            {io.DisplaySize.x / 2 - ImGui::CalcTextSize(tr("terrainmap", language)).x / 2, 12},
            IM_COL32(220, 248, 250, 255), tr("terrainmap", language));
    if (settings.ghost) {
        float x = (v.cameraX - c.left) * sx, y = (v.cameraY - c.top) * sy;
        draw->AddRect({x, y}, {x + 320 * sx, y + 224 * sy}, IM_COL32(255, 255, 255, 110), 0, 0, 1);
    }
    if (settings.hud != 3) {
        float safe = std::min(io.DisplaySize.x, io.DisplaySize.y * 16 / 9),
              x = settings.hud == 2 ? 18 : (io.DisplaySize.x - safe) / 2 + 18, y = 18;
        if (settings.hud == 1) {
            x = std::clamp(px + 30.0f, 0.0f, std::max(0.0f, io.DisplaySize.x - 320));
            y = std::clamp(py - 85.0f, 0.0f, std::max(0.0f, io.DisplaySize.y - 150));
        }
        ImGui::SetNextWindowPos({x, y});
        ImGui::SetNextWindowBgAlpha(0.76f);
        ImGui::Begin("##score", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings);
        float size = settings.hudScale ? float(settings.hudScale)
                                       : std::clamp(io.DisplaySize.y / 720.0f, 0.85f, 3.0f);
        ImGui::SetWindowFontScale(std::min(3.0f, size) / uiFactor);
        auto &r = core.snapshot.ram;
        unsigned score = (uint32_t(be16(r, 0xFE26)) << 16) | be16(r, 0xFE28);
        ImGui::TextColored({0.95f, 0.85f, 0.35f, 1}, "%s  %06u", tr("score", language), score * 10);
        ImGui::Text("%s  %02u:%02u", tr("time", language), r[0xFE23], r[0xFE24]);
        ImGui::Text("%s  %u", tr("rings", language), be16(r, 0xFE20));
        ImGui::Text("%s  %u", tr("lives", language), r[0xFE12]);
        ImGui::End();
    }
    if (settings.minimap) {
        float mapW = std::min(420.0f, io.DisplaySize.x * 0.34f),
              mapH = std::clamp(mapW * v.height / v.width, 35.0f, 100.0f),
              left = io.DisplaySize.x / 2 - mapW / 2, top = io.DisplaySize.y - mapH - 65;
        draw->AddRectFilled({left - 8, top - 8}, {left + mapW + 8, top + mapH + 8},
                            IM_COL32(6, 18, 32, 200), 8);
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 128; x++) {
                int wx = x * v.width / 128, wy = y * v.height / 16;
                if (core.world.attribute(core.snapshot, v.family, wx, wy))
                    draw->AddRectFilled({left + x * mapW / 128, top + y * mapH / 16},
                                        {left + (x + 1) * mapW / 128, top + (y + 1) * mapH / 16},
                                        IM_COL32(48, 134, 159, 190));
            }
        float mx = mapW / v.width, my = mapH / v.height;
        draw->AddRect(
            {left + c.left * mx, top + c.top * my},
            {left + float(c.left + c.coverageW) * mx, top + float(c.top + c.coverageH) * my},
            IM_COL32(230, 244, 255, 180));
        draw->AddCircleFilled({left + v.playerX * mx, top + v.playerY * my}, 4,
                              IM_COL32(255, 224, 88, 255));
    }
}
std::string App::navigationHint() const {
    int labels = settings.padLabels;
    if (!labels && pads[activePad]) {
        auto type = SDL_GameControllerGetType(pads[activePad]);
        labels = type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 ||
                         type == SDL_CONTROLLER_TYPE_PS5
                     ? 2
                 : type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO ||
                         type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR
                     ? 3
                     : 1;
    }
    return tr(labels == 2 ? "navps" : labels == 3 ? "navnintendo" : "navxbox", language);
}
void App::menuInput(bool editing) {
    if (!menu) {
        backRequested = selectRequested = false;
        return;
    }
    if (backRequested || selectRequested) {
        if (!editing) {
            if (pathEditor)
                pathEditor = false;
            else if (actionPopup)
                actionPopup = 0;
            else if (browserOpen) {
                browserOpen = false;
                focusMenu = true;
            } else if (core.loaded)
                toggleMenu();
            else if (tab != 0) {
                tab = 0;
                focusPage = true;
            }
        }
        backRequested = selectRequested = false;
    }
    if (!menu || editing || browserOpen || pathEditor || actionPopup)
        return;
    int direction = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false))
        direction = -1;
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false))
        direction = 1;
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
        direction = ImGui::GetIO().KeyShift ? -1 : 1;
    if (direction) {
        tab = (tab + direction + 9) % 9;
        focusPage = true;
        ImGui::SetNavCursorVisible(true);
    }
    if (core.loaded && ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false))
        toggleMenu();
}
void App::scrollPage() {
    float direction = ImGui::GetKeyData(ImGuiKey_GamepadR2)->AnalogValue -
                      ImGui::GetKeyData(ImGuiKey_GamepadL2)->AnalogValue;
    if (std::abs(direction) > 0.1f)
        ImGui::SetScrollY(ImGui::GetScrollY() +
                          direction * ImGui::GetIO().DeltaTime * 750 * uiFactor);
}
void App::requestAction(int action, int slot, const std::string &game) {
    actionSlot = slot;
    actionGame = game;
    actionPopup = action;
    menu = true;
    core.clearAudio();
    pendingButtons[0] = pendingButtons[1] = 0;
    if ((action == 1 && (!settings.confirmQuit || !core.loaded)) ||
        (action != 1 && !settings.confirmActions))
        commitAction();
}
void App::commitAction() {
    int action = actionPopup;
    actionPopup = 0;
    try {
        if (action == 1)
            running = false;
        else if (action == 2) {
            if (settings.lockon != core.locked && core.currentId != "sonic2k" &&
                core.currentId != "sonic3k" && core.currentId != "sphere") {
                bool automatic = settings.autosave;
                settings.autosave = false;
                std::string id = core.currentId;
                play(id);
                settings.autosave = automatic;
                menu = true;
            } else
                core.reset();
            notice = tr("resetdone", language);
        } else if (action == 3) {
            auto previous = settings;
            settings = Settings{};
            settings.language = previous.language;
            settings.lastDirectory = previous.lastDirectory;
            settings.lastGame = previous.lastGame;
            settings.mute = previous.mute;
            language = detectLanguage(settings.language);
            core.updateOptions();
            backgroundStage = -1;
            applyDisplay();
            dirty = true;
            notice = tr("defaultsdone", language);
        } else if (action == 4) {
            core.loadState(actionSlot);
            notice = tr("loaded", language);
        } else if (action == 5) {
            core.save(actionSlot);
            notice = tr("saved", language);
        } else if (action == 6)
            play(actionGame);
    } catch (const std::exception &e) {
        error(e);
    }
    focusPage = true;
}
void App::ui() {
    auto &io = ImGui::GetIO();
    bool compact = settings.menuLayout == 1;
    float width = compact ? std::min(1120 * uiFactor, io.DisplaySize.x - 12) : io.DisplaySize.x;
    float height = compact ? std::min(820 * uiFactor, io.DisplaySize.y - 12) : io.DisplaySize.y;
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({width, height});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                        compact ? baseStyle.WindowRounding * uiFactor : 0.0f);
    if (focusMenu)
        ImGui::SetNextWindowFocus();
    ImGui::Begin("###scu-menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
    float safe = std::min(ImGui::GetContentRegionAvail().x, 1220 * uiFactor);
    ImGui::SetCursorPosX((width - safe) / 2);
    ImGui::BeginChild("##safe", {safe, 0}, ImGuiChildFlags_NavFlattened);
    bool small = safe < 650 * uiFactor, tiny = height < 450 * uiFactor;
    if (!tiny) {
        ImGui::TextColored({0.25f, 0.84f, 0.91f, 1}, "SONIC COMPILATION ULTRA");
        if (!small)
            ImGui::TextWrapped("%s", tr("tagline", language));
    }
    if (focusMenu) {
        ImGui::SetKeyboardFocusHere();
        ImGui::SetNavCursorVisible(true);
    }
    const char *importKey = safe < 400 * uiFactor ? "importshort" : "import";
    if (core.loaded) {
        if (button("resume"))
            toggleMenu();
        nextButton(importKey);
    }
    if (button(importKey))
        beginBrowser(false);
    control("import");
    nextButton("quit");
    if (button("quit"))
        requestAction(1);
    focusMenu = false;
    ImGui::Separator();
    const char *tabs[] = {"library",  "video",    "camera", "atmosphere", "sound",
                          "controls", "gameplay", "saves",  "about"};
    if (small) {
        if (button("prevsection")) {
            tab = (tab + 8) % 9;
            focusPage = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(std::max(80.0f, safe - 100 * uiFactor));
        std::vector<const char *> labels;
        for (auto key : tabs)
            labels.push_back(tr(key, language));
        if (ImGui::Combo("##section", &tab, labels.data(), 9))
            focusPage = true;
        control("section");
        ImGui::SameLine();
        if (button("nextsection")) {
            tab = (tab + 1) % 9;
            focusPage = true;
        }
    }
    float footer =
        ImGui::CalcTextSize(navigationHint().c_str(), nullptr, false, safe).y +
        ImGui::GetStyle().ItemSpacing.y * 3 +
        (!tiny ? ImGui::CalcTextSize(tr("autosettings", language), nullptr, false, safe).y : 0);
    float contentH = std::max(50.0f, ImGui::GetContentRegionAvail().y - footer);
    if (!small) {
        ImGui::BeginChild("##sections", {230 * uiFactor, contentH}, ImGuiChildFlags_NavFlattened);
        for (int n = 0; n < 9; ++n) {
            if (ImGui::Selectable((std::string(tr(tabs[n], language)) + "###" + tabs[n]).c_str(),
                                  tab == n)) {
                tab = n;
                focusPage = true;
            }
            control(tabs[n]);
        }
        ImGui::EndChild();
        ImGui::SameLine();
    }
    ImGui::BeginChild("##page", {0, contentH}, ImGuiChildFlags_NavFlattened);
    scrollPage();
    if (focusPage) {
        ImGui::SetScrollY(0);
        ImGui::SetKeyboardFocusHere();
    }
    if (!notice.empty()) {
        ImGui::TextWrapped("%s", notice.c_str());
        if (button("dismiss"))
            notice.clear();
        ImGui::Separator();
    }
    if (tab == 0)
        libraryUi();
    else
        optionsUi(tab);
    focusPage = false;
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::TextWrapped("%s", navigationHint().c_str());
    if (!tiny)
        ImGui::TextDisabled("%s", tr("autosettings", language));
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
    if (browserOpen || ImGui::IsPopupOpen("##browse"))
        browserUi();
    if (actionPopup)
        ImGui::OpenPopup("##confirm");
    ImGui::SetNextWindowSize({std::min(650 * uiFactor, io.DisplaySize.x - 16), 0});
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("##confirm", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        if (!actionPopup)
            ImGui::CloseCurrentPopup();
        const char *messages[] = {"",
                                  "quitconfirm",
                                  "resetconfirm",
                                  "defaultsconfirm",
                                  "loadconfirm",
                                  "overwriteconfirm",
                                  "switchconfirm"};
        ImGui::TextWrapped("%s", tr(messages[std::clamp(actionPopup, 0, 6)], language));
        if (button("cancel")) {
            actionPopup = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (button("yes")) {
            commitAction();
            ImGui::CloseCurrentPopup();
        }
        ImGui::TextWrapped("%s", navigationHint().c_str());
        ImGui::EndPopup();
    }
}
void App::libraryUi() {
    if (button("folder"))
        beginBrowser(true);
    if (ImGui::GetContentRegionAvail().x > 580 * uiFactor)
        ImGui::SameLine();
    if (button("sourcehelp")) {
        tab = 8;
        focusPage = true;
    }
    if (tick("readyonly", settings.readyOnly))
        dirty = true;
    if (!core.loaded && library.find(settings.lastGame)) {
        if (button("continue"))
            play(settings.lastGame);
    }
    if (library.entries.empty())
        ImGui::TextWrapped("%s", tr("empty", language));
    ImGui::TextWrapped("%s", tr("legalshort", language));
    float available = ImGui::GetContentRegionAvail().x;
    int cols = available > 750 * uiFactor ? 3 : available > 470 * uiFactor ? 2 : 1;
    if (ImGui::BeginTable("##games", cols, ImGuiTableFlags_SizingStretchSame)) {
        int index = 0;
        for (auto &g : games()) {
            if (std::string(g.id) == "skchip")
                continue;
            auto r = library.find(g.id);
            if (settings.readyOnly && !r)
                continue;
            ImGui::TableNextColumn();
            ImGui::PushID(g.id);
            ImGui::BeginChild("##card", {0, 186 * uiFactor},
                              ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
            ImGui::TextColored(r ? ImVec4{0.35f, 0.89f, 0.91f, 1} : ImVec4{0.55f, 0.62f, 0.69f, 1},
                               "%02d", ++index);
            ImGui::TextWrapped("%s", g.name);
            ImGui::TextWrapped("%s",
                               r ? (r->derived ? tr("derived", language) : tr("ready", language))
                                 : tr("missing", language));
            ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(), 134 * uiFactor));
            if (r) {
                if (button("play", {-1, 0})) {
                    if (core.loaded && core.currentId != g.id)
                        requestAction(6, 1, g.id);
                    else
                        play(g.id);
                }
                control((std::string("play-") + g.id).c_str());
            } else if (button("import", {-1, 0}))
                beginBrowser(false);
            ImGui::EndChild();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    for (auto &r : library.entries)
        if (r.id.rfind("custom-", 0) == 0) {
            ImGui::PushID(r.id.c_str());
            ImGui::TextWrapped("%s", fs::u8path(r.path).stem().u8string().c_str());
            if (button("play")) {
                if (core.loaded && core.currentId != r.id)
                    requestAction(6, 1, r.id);
                else
                    play(r.id);
            }
            control(("play-" + r.id).c_str());
            ImGui::PopID();
        }
}
void App::optionsUi(int section) {
    bool changed = false;
    auto check = [&](const char *key, bool &value) { changed |= tick(key, value); };
    auto slide = [&](const char *key, int &v, int a, int b, const char *fmt = "%d") {
        changed |= slider(key, v, a, b, fmt);
    };
    switch (section) {
    case 1: {
        int lang = settings.language + 1;
        if (choice("language", lang, {"auto", "English", "Français", "Español", "日本語"})) {
            settings.language = lang - 1;
            language = detectLanguage(settings.language);
            changed = true;
        }
        int preset = settings.quality;
        if (choice("preset", preset, {"low", "balanced", "ultra"})) {
            settings.preset(preset);
            backgroundStage = -1;
            changed = true;
        }
        int view = settings.view == 4 ? 3 : settings.view;
        if (choice("view", view, {"pixel", "panorama", "overview", "classic"})) {
            settings.view = view == 3 ? 4 : view;
            changed = true;
        }
        changed |=
            choice("overviewstyle", settings.overviewStyle, {"auto", "nativeart", "terrainmap"});
        ImGui::TextWrapped("%s", tr("maphelp", language));
        ImGui::TextWrapped("%s", tr("overviewhelp", language));
        if (slider("zoom", settings.zoom, 1, 8, "%d×")) {
            settings.view = 0;
            changed = true;
        }
        ImGui::TextWrapped("%s", tr("pixelhelp", language));
        bool full = settings.fullscreen, span = settings.span;
        int display = settings.display;
        check("fullscreen", settings.fullscreen);
        check("span", settings.span);
        ImGui::TextWrapped("%s", tr("display", language));
        std::vector<std::string> names{tr("currentdisplay", language)};
        for (int n = 0; n < SDL_GetNumVideoDisplays(); ++n) {
            SDL_Rect bounds{};
            SDL_GetDisplayBounds(n, &bounds);
            names.push_back(std::to_string(n + 1) + " · " + SDL_GetDisplayName(n) + " · " +
                            std::to_string(bounds.w) + " × " + std::to_string(bounds.h));
        }
        int chosen = std::clamp(settings.display + 1, 0, int(names.size()) - 1);
        std::vector<const char *> list;
        for (auto &name : names)
            list.push_back(name.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##display", &chosen, list.data(), int(list.size()))) {
            settings.display = chosen - 1;
            changed = true;
        }
        control("display");
        if (full != settings.fullscreen || span != settings.span || display != settings.display)
            applyDisplay();
        changed |= choice("menulayout", settings.menuLayout, {"fullmenu", "compactmenu"});
        changed |=
            choice("uiscale", settings.uiScale, {"auto", "smalltext", "largetext", "hugetext"});
        check("contrast", settings.highContrast);
        check("vsync", settings.vsync);
        check("nearest", settings.integerScale);
        slide("nativescale", settings.nativeScale, 0, 8);
        slide("workers", settings.workers, 0, 16);
        bool soft = settings.renderer != 0;
        if (tick("soft", soft)) {
            settings.renderer = soft ? 1 : 0;
            changed = true;
        }
        slide("scanlines", settings.scanlines, 0, 60);
        check("performance", settings.performance);
        if (button("defaults"))
            requestAction(3);
        break;
    }
    case 2:
        if (button("showstage")) {
            settings.view = 2;
            settings.locator = true;
            changed = true;
        }
        if (button("focusplayer")) {
            settings.view = 0;
            settings.camera = 1;
            changed = true;
        }
        check("playerinset", settings.playerInset);
        if (choice("cameramode", settings.camera,
                   {"bounded", "centred", "fixed", "free", "level"})) {
            settings.view = settings.camera == 4 ? 2 : 0;
            changed = true;
        }
        ImGui::TextWrapped("%s", tr("overviewcamera", language));
        ImGui::BeginDisabled(settings.view == 2);
        if (button("freeze")) {
            core.world.freeze(core.world.inspect(core.snapshot, core.game), settings);
            changed = true;
        }
        ImGui::EndDisabled();
        if (button("follow")) {
            settings.camera = 0;
            settings.view = 0;
            changed = true;
        }
        slide("panspeed", settings.panSpeed, 1, 32);
        changed |= choice("hud", settings.hud, {"hudcentre", "hudplayer", "hudedge", "hidden"});
        slide("hudsize", settings.hudScale, 0, 6);
        check("locator", settings.locator);
        check("minimap", settings.minimap);
        check("ghost", settings.ghost);
        check("help", settings.help);
        break;
    case 3:
        changed |= choice("backgroundstyle", settings.background,
                          {"blurred", "sharpbg", "parallaxbg", "gradient", "dark", "black"});
        slide("blur", settings.blur, 0, 16);
        slide("brightness", settings.brightness, 10, 100, "%d%%");
        check("parallax", settings.parallax);
        check("motion", settings.reduceMotion);
        ImGui::TextWrapped("%s", tr("bghelp", language));
        if (changed)
            backgroundStage = -1;
        break;
    case 4:
        slide("volume", settings.volume, 0, 100, "%d%%");
        check("mute", settings.mute);
        check("pausefocus", settings.pauseUnfocused);
        ImGui::TextWrapped("%s", tr("audiohelp", language));
        break;
    case 5: {
        changed |=
            choice("padlabels", settings.padLabels, {"auto", "Xbox", "PlayStation", "Nintendo"});
        changed |=
            choice("jumpbuttons", settings.jumpLayout, {"jumpclassic", "jumpabc", "jumpbac"});
        check("quickcontrols", settings.quickControls);
        slide("deadzone", settings.deadzone, 3000, 28000);
        ImGui::TextWrapped("%s", tr("controllers", language));
        bool any = false;
        for (int p = 0; p < 2; ++p)
            if (pads[p]) {
                any = true;
                ImGui::TextWrapped("%d · %s", p + 1, SDL_GameControllerName(pads[p]));
                auto dx = SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_LEFTX);
                auto dy = SDL_GameControllerGetAxis(pads[p], SDL_CONTROLLER_AXIS_LEFTY);
                ImGui::Text("X: %d  Y: %d", dx, dy);
            }
        if (!any)
            ImGui::TextWrapped("%s", tr("nopad", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", navigationHint().c_str());
        ImGui::TextWrapped("%s", tr("quickhelp", language));
        ImGui::TextWrapped("%s", tr("keyhelp", language));
        ImGui::TextWrapped("%s", tr("inputhelp", language));
        break;
    }
    case 6:
        check("lockon", settings.lockon);
        ImGui::TextWrapped("%s", tr("lockonhelp", language));
        check("spindash", settings.spindash);
        ImGui::TextWrapped("%s", tr("dashhelp", language));
        check("fastboot", settings.fastBoot);
        check("spritelimit", settings.noSpriteLimit);
        check("rewind", settings.rewind);
        check("assist", settings.invincible);
        check("air", settings.infiniteAir);
        changed |= choice("region", settings.region,
                          {"auto", "USA / NTSC", "Europe / PAL", "Japan / NTSC"});
        check("confirmactions", settings.confirmActions);
        check("confirmquit", settings.confirmQuit);
        ImGui::TextWrapped("%s", tr("accuracy", language));
        break;
    case 7: {
        check("autosave", settings.autosave);
        ImGui::TextWrapped("%s", tr("slothelp", language));
        for (int n = 1; n <= 9; ++n) {
            auto path =
                library.dataDir / "saves" / (core.saveKey + "-" + std::to_string(n) + ".state");
            std::error_code ec;
            bool exists = core.loaded && fs::exists(path, ec);
            std::string status = tr(exists ? "occupied" : "emptyslot", language);
            if (exists) {
                auto stamp = fs::last_write_time(path, ec);
                if (!ec) {
                    auto clock = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        stamp - fs::file_time_type::clock::now() +
                        std::chrono::system_clock::now());
                    std::time_t time = std::chrono::system_clock::to_time_t(clock);
                    char buf[32]{};
                    if (auto tm = std::localtime(&time))
                        std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tm);
                    status += " · ";
                    status += buf;
                }
            }
            std::string label = std::string(tr("slotnumber", language)) + " " + std::to_string(n) +
                                " · " + status + "###slot" + std::to_string(n);
            if (ImGui::Selectable(label.c_str(), settings.slot == n)) {
                settings.slot = n;
                changed = true;
            }
            control(("slot" + std::to_string(n)).c_str());
        }
        ImGui::BeginDisabled(!core.loaded);
        if (button("save")) {
            auto path = library.dataDir / "saves" /
                        (core.saveKey + "-" + std::to_string(settings.slot) + ".state");
            if (fs::exists(path))
                requestAction(5, settings.slot);
            else {
                try {
                    core.save(settings.slot);
                    notice = tr("saved", language);
                } catch (const std::exception &e) {
                    error(e);
                }
            }
        }
        std::error_code ec;
        auto path = library.dataDir / "saves" /
                    (core.saveKey + "-" + std::to_string(settings.slot) + ".state");
        ImGui::BeginDisabled(!fs::exists(path, ec));
        if (button("load"))
            requestAction(4, settings.slot);
        ImGui::EndDisabled();
        if (button("reset"))
            requestAction(2);
        if (button("capture"))
            screenshotRequested = true;
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("savehelp", language));
        break;
    }
    case 8:
        ImGui::Text("Sonic Compilation Ultra %s", SCU_VERSION);
        if (button("sourcehelp"))
            notice = tr("romguide", language);
        ImGui::TextWrapped("%s", tr("romguide", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("romformats", language));
        ImGui::TextWrapped("%s", tr("romrevision", language));
        if (ImGui::BeginTable("##identity", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
            ImGui::TableSetupColumn(tr("gamename", language));
            ImGui::TableSetupColumn(tr("bytes", language));
            ImGui::TableSetupColumn("CRC32");
            ImGui::TableHeadersRow();
            for (auto &g : games()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", g.name);
                ImGui::TableNextColumn();
                ImGui::Text("%zu", g.size);
                ImGui::TableNextColumn();
                ImGui::Text("%08X", g.crc);
            }
            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("credits", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("limitations", language));
        break;
    }
    if (changed) {
        dirty = true;
        core.updateOptions();
        SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
        core.clearAudio();
    }
}
void App::changeDirectory(const fs::path &path) {
    std::error_code ec;
    if (!fs::is_directory(path, ec)) {
        notice = tr("pathmissing", language);
        return;
    }
    browsePath = path.u8string();
    snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
    focusBrowser = true;
}
void App::browserUi() {
    auto &io = ImGui::GetIO();
    if (browserOpen)
        ImGui::OpenPopup("##browse");
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({std::min(1000 * uiFactor, io.DisplaySize.x - 12),
                              std::min(750 * uiFactor, io.DisplaySize.y - 12)});
    if (ImGui::BeginPopupModal("##browse", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        if (!browserOpen)
            ImGui::CloseCurrentPopup();
        ImGui::TextWrapped("%s", tr(folderMode ? "folder" : "import", language));
        ImGui::TextWrapped("%s", browsePath.c_str());
        if (focusBrowser) {
            ImGui::SetKeyboardFocusHere();
            ImGui::SetNavCursorVisible(true);
        }
        if (button(io.DisplaySize.x < 600 ? "upshort" : "up"))
            changeDirectory(fs::u8path(browsePath).parent_path());
        control("up");
        nextButton(io.DisplaySize.x < 600 ? "pathshort" : "editpath");
        if (button(io.DisplaySize.x < 600 ? "pathshort" : "editpath")) {
            pathEditor = true;
            snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
        }
        control("editpath");
        nextButton("cancel");
        if (button("cancel")) {
            browserOpen = false;
            ImGui::CloseCurrentPopup();
            focusMenu = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && !pathEditor)
            changeDirectory(fs::u8path(browsePath).parent_path());
        ImGui::SetNextItemWidth(-1);
        std::vector<std::string> roots;
#ifdef _WIN32
        for (char d = 'A'; d <= 'Z'; ++d) {
            std::string root = std::string(1, d) + ":/";
            std::error_code ec;
            if (fs::is_directory(fs::u8path(root), ec))
                roots.push_back(root);
        }
#else
        roots = {"/", fs::current_path().u8string(), "/Volumes", "/media", "/mnt"};
        if (const char *home = SDL_getenv("HOME"))
            roots.insert(roots.begin(), home);
#endif
        if (ImGui::BeginCombo("##locations", tr("locations", language))) {
            for (auto &root : roots)
                if (ImGui::Selectable(root.c_str()))
                    changeDirectory(fs::u8path(root));
            ImGui::EndCombo();
        }
        control("locations");
        if (folderMode && button("thisfolder")) {
            import(fs::u8path(browsePath), true);
            browserOpen = false;
            ImGui::CloseCurrentPopup();
        }
        if (!notice.empty())
            ImGui::TextWrapped("%s", notice.c_str());
        ImGui::BeginChild("##files", {0, 0},
                          ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened);
        scrollPage();
        if (focusBrowser)
            ImGui::SetScrollY(0);
        std::error_code ec;
        std::vector<fs::directory_entry> files;
        fs::directory_iterator it(fs::u8path(browsePath),
                                  fs::directory_options::skip_permission_denied, ec);
        for (fs::directory_iterator end; !ec && it != end; it.increment(ec)) {
            std::error_code fileError;
            bool dir = it->is_directory(fileError);
            auto ext = it->path().extension().u8string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return char(std::tolower(c)); });
            if (dir || (!folderMode && (ext == ".zip" || ext == ".bin" || ext == ".md" ||
                                        ext == ".gen" || ext == ".smd")))
                files.push_back(*it);
        }
        std::sort(files.begin(), files.end(), [](auto &a, auto &b) {
            std::error_code ea, eb;
            bool da = a.is_directory(ea), db = b.is_directory(eb);
            return da != db ? da > db : a.path().filename() < b.path().filename();
        });
        if (files.empty())
            ImGui::TextWrapped("%s", tr(ec ? "pathmissing" : "nofiles", language));
        std::string next;
        for (auto &entry : files) {
            std::error_code error;
            bool dir = entry.is_directory(error);
            auto name = entry.path().filename().u8string();
            std::string label = (dir ? "[+] " : "    ") + name;
            if (ImGui::Selectable(label.c_str())) {
                if (dir)
                    next = entry.path().u8string();
                else {
                    import(entry.path());
                    browserOpen = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            control(("file:" + name).c_str());
        }
        ImGui::EndChild();
        focusBrowser = false;
        if (!next.empty())
            changeDirectory(fs::u8path(next));
        if (pathEditor || ImGui::IsPopupOpen("##path"))
            pathEditorUi();
        ImGui::EndPopup();
    }
}
void App::pathEditorUi() {
    auto &io = ImGui::GetIO();
    if (pathEditor)
        ImGui::OpenPopup("##path");
    ImGui::SetNextWindowSize({std::min(850 * uiFactor, io.DisplaySize.x - 12),
                              std::min(660 * uiFactor, io.DisplaySize.y - 12)});
    if (ImGui::BeginPopupModal("##path", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        if (!pathEditor)
            ImGui::CloseCurrentPopup();
        ImGui::TextWrapped("%s", tr("editpath", language));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##pathtext", pathBuffer, sizeof pathBuffer);
        control("pathtext");
        if (button("applypath") || ImGui::IsKeyPressed(ImGuiKey_GamepadStart, false)) {
            std::error_code ec;
            auto path = fs::u8path(pathBuffer);
            if (fs::is_directory(path, ec)) {
                changeDirectory(path);
                pathEditor = false;
                ImGui::CloseCurrentPopup();
            } else
                notice = tr("pathmissing", language);
        }
        nextButton("backspace");
        if (button("backspace")) {
            size_t n = std::strlen(pathBuffer);
            if (n) {
                --n;
                while (n && (uint8_t(pathBuffer[n]) & 0xC0) == 0x80)
                    --n;
                pathBuffer[n] = 0;
            }
        }
        nextButton("cancel");
        if (button("cancel")) {
            pathEditor = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::BeginChild("##keys", {0, 0}, ImGuiChildFlags_NavFlattened);
        scrollPage();
        static const char keys[] =
            "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:/\\._- ()[]";
        if (ImGui::BeginTable("##keyboard", 10, ImGuiTableFlags_SizingStretchSame)) {
            for (int n = 0; keys[n]; ++n) {
                ImGui::TableNextColumn();
                ImGui::PushID(n);
                char label[2] = {keys[n], 0};
                if (ImGui::Button(keys[n] == ' ' ? "_" : label, {-1, 0})) {
                    size_t len = std::strlen(pathBuffer);
                    if (len + 1 < sizeof pathBuffer) {
                        pathBuffer[len] = keys[n];
                        pathBuffer[len + 1] = 0;
                    }
                }
                control((std::string("key:") + keys[n]).c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}
} // namespace scu
