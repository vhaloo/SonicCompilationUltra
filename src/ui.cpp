#include "ui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <chrono>
namespace scu {
static bool combo(const char *label, int *value, std::initializer_list<const char *> keys,
                  int language) {
    std::vector<const char *> v;
    for (auto k : keys)
        v.push_back(tr(k, language));
    return ImGui::Combo(tr(label, language), value, v.data(), (int)v.size());
}
void App::initialise(const fs::path &base, bool hidden) {
    basePath = base;
    fs::create_directories(library.dataDir);
    settings.load(library.dataDir);
    library.load();
    language = detectLanguage(settings.language);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
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
        io.Fonts->AddFontFromFileTTF(font.c_str(), 18, &fontCfg, ranges.Data);
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
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    refreshControllers();
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
}
void App::toggleMenu() {
    menu = !menu;
    pendingButtons[0] = pendingButtons[1] = 0;
    core.clearAudio();
    dirty = true;
}
void App::applyDisplay() {
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowBordered(window, settings.fullscreen ? SDL_FALSE : SDL_TRUE);
    if (settings.fullscreen) {
        SDL_Rect r{};
        int display = SDL_GetWindowDisplayIndex(window);
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
    } else {
        SDL_Rect bounds{};
        SDL_GetDisplayUsableBounds(std::max(0, SDL_GetWindowDisplayIndex(window)), &bounds);
        SDL_SetWindowSize(window, std::min(1280, bounds.w), std::min(720, bounds.h));
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
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
    dirty = true;
    notice.clear();
    backgroundStage = -1;
    backgroundFrozen = false;
    return true;
}
void App::beginBrowser(bool folder) {
    folderMode = folder;
    browserOpen = true;
    browsePath =
        settings.lastDirectory.empty() ? fs::current_path().u8string() : settings.lastDirectory;
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
            quitPopup = true;
            menu = true;
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
    if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
        toggleMenu();
        return;
    }
    if (e.type == SDL_KEYDOWN && !e.key.repeat) {
        auto k = e.key.keysym.sym;
        auto mod = e.key.keysym.mod;
        if (k == SDLK_ESCAPE || k == SDLK_F10) {
            if (browserOpen)
                browserOpen = false;
            else if (core.loaded)
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
    if (menu)
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
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X),
                RETRO_DEVICE_ID_JOYPAD_Y);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A),
                RETRO_DEVICE_ID_JOYPAD_B);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B),
                RETRO_DEVICE_ID_JOYPAD_A);
            set(p, SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START),
                RETRO_DEVICE_ID_JOYPAD_START);
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
    for (int v : {drawableW, drawableH, settings.view, settings.zoom, settings.camera,
                  settings.left, settings.top, settings.background, settings.blur,
                  settings.brightness, (int)settings.parallax, (int)settings.reduceMotion}) {
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
            int availableH = drawableH - (settings.help ? 48 : 0),
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
    ImGui::NewFrame();
    ImGui::GetIO().FontGlobalScale = settings.uiScale == 1   ? 0.85f
                                     : settings.uiScale == 2 ? 1.25f
                                     : settings.uiScale == 3 ? 1.5f
                                                             : 1;
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
        if (ImGui::Button(tr("menuhint", language)))
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
        float size =
            settings.hudScale ? settings.hudScale : std::max(1, int(io.DisplaySize.y / 720));
        ImGui::SetWindowFontScale(std::min(3.0f, size));
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
        draw->AddRect({left + c.left * mx, top + c.top * my},
                      {left + (c.left + c.width) * mx, top + (c.top + c.height) * my},
                      IM_COL32(230, 244, 255, 180));
        draw->AddCircleFilled({left + v.playerX * mx, top + v.playerY * my}, 4,
                              IM_COL32(255, 224, 88, 255));
    }
}
void App::ui() {
    auto &io = ImGui::GetIO();
    float width = std::min(1100.0f, io.DisplaySize.x - 12),
          height = std::min(820.0f, io.DisplaySize.y - 60);
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({width, std::max(160.0f, height)});
    ImGui::Begin("Sonic Compilation Ultra", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    if (width < 600)
        ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored({0.25f, 0.84f, 0.91f, 1}, "SONIC COMPILATION ULTRA");
    if (io.DisplaySize.y >= 500)
        ImGui::TextWrapped("%s", tr("tagline", language));
    if (core.loaded) {
        if (ImGui::Button(tr("resume", language)))
            toggleMenu();
        ImGui::SameLine();
    }
    if (ImGui::Button(tr("import", language)))
        beginBrowser(false);
    ImGui::SameLine();
    if (ImGui::Button(tr("quit", language))) {
        if (settings.confirmQuit && core.loaded)
            quitPopup = true;
        else
            running = false;
    }
    if (!notice.empty())
        ImGui::TextWrapped("%s", notice.c_str());
    ImGui::Separator();
    const char *tabs[] = {"library",  "video",    "camera", "atmosphere", "sound",
                          "controls", "gameplay", "saves",  "about"};
    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (int n = 0; n < 9; n++) {
            if (ImGui::BeginTabItem(tr(tabs[n], language), nullptr,
                                    tab == n ? ImGuiTabItemFlags_SetSelected : 0)) {
                tab = -1;
                ImGui::BeginChild("##page", {0, -30}, false);
                if (n == 0)
                    libraryUi();
                else
                    optionsUi(n);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::TextDisabled("%s", tr("menuhint", language));
    ImGui::End();
    if (browserOpen)
        browserUi();
    if (quitPopup)
        ImGui::OpenPopup("##quit");
    if (ImGui::BeginPopupModal("##quit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(tr("quitconfirm", language));
        if (ImGui::Button(tr("yes", language))) {
            running = false;
            quitPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("cancel", language))) {
            quitPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
void App::libraryUi() {
    ImGui::TextWrapped("%s", tr("legalshort", language));
    if (ImGui::GetIO().DisplaySize.y >= 500) {
        if (ImGui::Button(tr("folder", language)))
            beginBrowser(true);
        ImGui::SameLine();
        if (ImGui::Button(tr("sourcehelp", language)))
            tab = 8;
    }
    if (library.entries.empty())
        ImGui::TextWrapped("%s", tr("empty", language));
    float available = ImGui::GetContentRegionAvail().x;
    int cols = available > 750 ? 3 : available > 470 ? 2 : 1;
    if (ImGui::BeginTable("##games", cols, ImGuiTableFlags_SizingStretchSame)) {
        int index = 0;
        for (auto &g : games()) {
            if (std::string(g.id) == "skchip")
                continue;
            ImGui::TableNextColumn();
            ImGui::PushID(g.id);
            auto r = library.find(g.id);
            ImGui::BeginChild("##card", {0, ImGui::GetIO().DisplaySize.y < 500 ? 146.0f : 170.0f},
                              true);
            ImGui::TextColored(r ? ImVec4{0.35f, 0.89f, 0.91f, 1} : ImVec4{0.55f, 0.62f, 0.69f, 1},
                               "%02d", ++index);
            ImGui::TextWrapped("%s", g.name);
            ImGui::TextWrapped("%s",
                               r ? (r->derived ? tr("derived", language) : tr("ready", language))
                                 : tr("missing", language));
            if (r) {
                if (ImGui::Button(tr("play", language), {-1, 0}))
                    play(g.id);
            } else {
                if (ImGui::Button(tr("import", language), {-1, 0}))
                    beginBrowser(false);
            }
            ImGui::EndChild();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    for (auto &r : library.entries)
        if (r.id.rfind("custom-", 0) == 0) {
            ImGui::PushID(r.id.c_str());
            auto title = fs::u8path(r.path).stem().u8string();
            ImGui::TextWrapped("%s", title.c_str());
            ImGui::SameLine();
            if (ImGui::Button(tr("play", language)))
                play(r.id);
            ImGui::PopID();
        }
}
void App::optionsUi(int section) {
    bool changed = false;
    auto check = [&](const char *key, bool &v) {
        changed |= ImGui::Checkbox(tr(key, language), &v);
    };
    auto slide = [&](const char *key, int &v, int a, int b, const char *fmt = "%d") {
        changed |= ImGui::SliderInt(tr(key, language), &v, a, b, fmt);
    };
    ImGui::PushItemWidth(std::min(370.0f, ImGui::GetContentRegionAvail().x * 0.56f));
    switch (section) {
    case 1: {
        int lang = settings.language + 1;
        const char *langs[] = {tr("auto", language), "English", "Français", "Español", "日本語"};
        if (ImGui::Combo(tr("language", language), &lang, langs, 5)) {
            settings.language = lang - 1;
            language = detectLanguage(settings.language);
            changed = true;
        }
        int preset = settings.quality;
        if (combo("preset", &preset, {"low", "balanced", "ultra"}, language)) {
            settings.preset(preset);
            backgroundStage = -1;
            changed = true;
        }
        int view = settings.view == 4 ? 3 : settings.view;
        if (combo("view", &view, {"pixel", "panorama", "overview", "classic"}, language)) {
            settings.view = view == 3 ? 4 : view;
            changed = true;
        }
        slide("zoom", settings.zoom, 1, 8, "%d×");
        ImGui::TextWrapped("%s", tr("pixelhelp", language));
        bool before = settings.fullscreen;
        check("fullscreen", settings.fullscreen);
        check("span", settings.span);
        if (before != settings.fullscreen)
            applyDisplay();
        check("vsync", settings.vsync);
        check("nearest", settings.integerScale);
        slide("nativescale", settings.nativeScale, 0, 8);
        slide("uiscale", settings.uiScale, 0, 3);
        slide("workers", settings.workers, 0, 16);
        bool soft = settings.renderer != 0;
        if (ImGui::Checkbox(tr("soft", language), &soft)) {
            settings.renderer = soft ? 1 : 0;
            changed = true;
        }
        slide("scanlines", settings.scanlines, 0, 60);
        check("performance", settings.performance);
        if (ImGui::Button(tr("defaults", language))) {
            int lang = settings.language;
            settings = Settings{};
            settings.language = lang;
            changed = true;
            applyDisplay();
        }
        break;
    }
    case 2:
        changed |= combo("cameramode", &settings.camera,
                         {"bounded", "centred", "fixed", "free", "level"}, language);
        if (ImGui::Button(tr("freeze", language))) {
            core.world.freeze(core.world.inspect(core.snapshot, core.game), settings);
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("follow", language))) {
            settings.camera = 0;
            changed = true;
        }
        slide("panspeed", settings.panSpeed, 1, 32);
        changed |=
            combo("hud", &settings.hud, {"hudcentre", "hudplayer", "hudedge", "hidden"}, language);
        slide("hudsize", settings.hudScale, 0, 6);
        check("locator", settings.locator);
        check("minimap", settings.minimap);
        check("ghost", settings.ghost);
        check("help", settings.help);
        break;
    case 3:
        changed |=
            combo("backgroundstyle", &settings.background,
                  {"blurred", "sharpbg", "parallaxbg", "gradient", "dark", "black"}, language);
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
    case 5:
        ImGui::TextWrapped("%s", tr("keyhelp", language));
        ImGui::Separator();
        slide("deadzone", settings.deadzone, 3000, 28000);
        ImGui::Text("%s:", tr("controllers", language));
        for (int p = 0; p < 2; p++)
            if (pads[p])
                ImGui::Text("%d · %s", p + 1, SDL_GameControllerName(pads[p]));
        ImGui::TextWrapped("%s", tr("inputhelp", language));
        break;
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
        changed |= combo("region", &settings.region,
                         {"auto", "USA / NTSC", "Europe / PAL", "Japan / NTSC"}, language);
        ImGui::TextWrapped("%s", tr("accuracy", language));
        break;
    case 7:
        check("autosave", settings.autosave);
        slide("slot", settings.slot, 1, 9);
        ImGui::BeginDisabled(!core.loaded);
        if (ImGui::Button(tr("save", language))) {
            try {
                core.save(settings.slot);
                notice = tr("save", language);
            } catch (const std::exception &e) {
                error(e);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("load", language))) {
            try {
                core.loadState(settings.slot);
                notice = tr("load", language);
            } catch (const std::exception &e) {
                error(e);
            }
        }
        if (ImGui::Button(tr("reset", language))) {
            if (settings.lockon != core.locked && core.currentId != "sonic2k" &&
                core.currentId != "sonic3k" && core.currentId != "sphere") {
                std::string id = core.currentId;
                bool automatic = settings.autosave;
                settings.autosave = false;
                play(id);
                settings.autosave = automatic;
                menu = true;
            } else
                core.reset();
        }
        ImGui::SameLine();
        if (ImGui::Button(tr("capture", language)))
            screenshotRequested = true;
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", tr("savehelp", language));
        break;
    case 8:
        ImGui::Text("Sonic Compilation Ultra %s", SCU_VERSION);
        ImGui::TextWrapped("%s", tr("romguide", language));
        ImGui::Separator();
        ImGui::TextWrapped("%s", tr("romformats", language));
        ImGui::TextWrapped("%s", tr("romrevision", language));
        if (ImGui::BeginTable("##identity", 3,
                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                                  ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Game");
            ImGui::TableSetupColumn("Bytes");
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
    ImGui::PopItemWidth();
    if (changed) {
        dirty = true;
        core.updateOptions();
        SDL_RenderSetVSync(renderer, settings.vsync ? 1 : 0);
        core.clearAudio();
    }
}
void App::browserUi() {
    auto &io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x / 2, io.DisplaySize.y / 2}, {}, {0.5f, 0.5f});
    ImGui::SetNextWindowSize(
        {std::min(850.0f, io.DisplaySize.x - 16), std::min(600.0f, io.DisplaySize.y - 26)});
    ImGui::Begin(tr("browse", language), &browserOpen, ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetNextItemWidth(-1);
    bool enter = ImGui::InputText(tr("path", language), pathBuffer, sizeof pathBuffer,
                                  ImGuiInputTextFlags_EnterReturnsTrue);
    if (enter) {
        fs::path p = fs::u8path(pathBuffer);
        if (fs::is_directory(p))
            browsePath = p.u8string();
        else {
            import(p);
            browserOpen = false;
        }
    }
    if (ImGui::Button(tr("up", language))) {
        auto parent = fs::u8path(browsePath).parent_path();
        if (!parent.empty()) {
            browsePath = parent.u8string();
            snprintf(pathBuffer, sizeof pathBuffer, "%s", browsePath.c_str());
        }
    }
#ifdef _WIN32
    ImGui::SameLine();
    for (char drive = 'C'; drive <= 'Z'; drive++) {
        std::string root = std::string(1, drive) + ":/";
        std::error_code ec;
        if (fs::exists(fs::u8path(root), ec)) {
            if (ImGui::SmallButton(root.c_str())) {
                browsePath = root;
                snprintf(pathBuffer, sizeof pathBuffer, "%s", root.c_str());
            }
            ImGui::SameLine();
        }
    }
    ImGui::NewLine();
#endif
    if (folderMode && ImGui::Button(tr("thisfolder", language))) {
        import(fs::u8path(browsePath), true);
        browserOpen = false;
    }
    ImGui::BeginChild("##files", {0, -35}, true);
    std::error_code ec;
    std::vector<fs::directory_entry> files;
    fs::directory_iterator it(fs::u8path(browsePath), ec);
    if (!ec) {
        for (auto &p : it)
            files.push_back(p);
        std::sort(files.begin(), files.end(), [](auto &a, auto &b) {
            std::error_code e1, e2;
            bool da = a.is_directory(e1), db = b.is_directory(e2);
            return da != db ? da > db : a.path().filename() < b.path().filename();
        });
    }
    std::string next;
    for (auto &p : files) {
        std::error_code e;
        bool dir = p.is_directory(e);
        auto name = p.path().filename().u8string();
        auto ext = p.path().extension().u8string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (!dir && (folderMode || (ext != ".zip" && ext != ".bin" && ext != ".md" &&
                                    ext != ".gen" && ext != ".smd")))
            continue;
        std::string label = (dir ? "[+] " : "    ") + name;
        if (ImGui::Selectable(label.c_str())) {
            if (dir)
                next = p.path().u8string();
            else {
                import(p.path());
                browserOpen = false;
            }
        }
    }
    if (!next.empty()) {
        browsePath = next;
        snprintf(pathBuffer, sizeof pathBuffer, "%s", next.c_str());
    }
    ImGui::EndChild();
    if (ImGui::Button(tr("cancel", language)))
        browserOpen = false;
    ImGui::End();
}
} // namespace scu
