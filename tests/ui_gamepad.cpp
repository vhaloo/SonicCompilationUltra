#include "ui.h"
#include "fixture.h"
#include <imgui_internal.h>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
using namespace scu;
static void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Harness {
    std::unique_ptr<App> app;
    SDL_Joystick *joystick = nullptr;
    int index = -1;
    fs::path dir;
    std::set<std::string> visited;
    Harness() {
        SDL_VirtualJoystickDesc desc{};
        desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
        desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
        desc.naxes = SDL_CONTROLLER_AXIS_MAX;
        desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
        desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX) - 1;
        desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX) - 1;
        desc.name = "SCU virtual controller";
        index = SDL_JoystickAttachVirtualEx(&desc);
        require(index >= 0, SDL_GetError());
        joystick = SDL_JoystickOpen(index);
        require(joystick != nullptr, "virtual joystick open");
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
        SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
        dir = fs::temp_directory_path() /
              ("SCU-gamepad-" + std::to_string(SDL_GetPerformanceCounter()));
        fs::create_directories(dir / "cartridges" / "nested");
        writeBytesAtomic(dir / "cartridges" / "nested" / "test.bin", testProgram());
        Settings settings;
        settings.fullscreen = false;
        settings.vsync = false;
        settings.mute = true;
        settings.fastBoot = false;
        settings.autosave = false;
        settings.renderer = 1;
        settings.lastDirectory = (dir / "cartridges").u8string();
        settings.save(dir);
        app = std::make_unique<App>(dir);
        app->headless = true;
        app->core.headless = true;
        auto base = SDL_GetBasePath();
        app->initialise(base ? fs::u8path(base) : fs::current_path(), true);
        SDL_free(base);
        require(app->pads[0] != nullptr, "SDL virtual controller is not recognised");
        frame(6);
    }
    ~Harness() {
        app.reset();
        if (joystick)
            SDL_JoystickClose(joystick);
        if (index >= 0)
            SDL_JoystickDetachVirtual(index);
        std::error_code ec;
        auto resolved = fs::weakly_canonical(dir, ec);
        if (!ec && resolved.parent_path() == fs::weakly_canonical(fs::temp_directory_path()) &&
            resolved.filename().u8string().rfind("SCU-gamepad-", 0) == 0)
            fs::remove_all(resolved, ec);
    }
    void frame(int count = 2) {
        for (int n = 0; n < count; ++n) {
            SDL_JoystickUpdate();
            SDL_Event e;
            while (SDL_PollEvent(&e))
                app->event(e);
            app->pollInput();
            app->present();
            for (auto &item : app->controls)
                if (GImGui->NavId == item.second)
                    visited.insert(item.first);
            SDL_Delay(1);
        }
    }
    void press(SDL_GameControllerButton button) {
        SDL_JoystickSetVirtualButton(joystick, button, SDL_PRESSED);
        frame(1);
        SDL_JoystickSetVirtualButton(joystick, button, SDL_RELEASED);
        frame(2);
    }
    void tab(int target) {
        if (!app->menu)
            press(SDL_CONTROLLER_BUTTON_BACK);
        int attempts = 0;
        bool alreadyHere = app->menuTab() == target;
        while (app->menuTab() != target && attempts++ < 12)
            press(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        require(app->menuTab() == target, "shoulder cannot reach tab " + std::to_string(target) +
                                              "; current=" + std::to_string(app->menuTab()) +
                                              "; modal=" + std::to_string(app->confirmation()) +
                                              "; active=" + std::to_string(GImGui->ActiveId));
        // A section change already focuses its first control. Only re-enter
        // when a caller asks to reset focus on the section it is already in.
        if (alreadyHere) {
            press(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            press(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        }
        frame();
    }
    void find(const std::string &key) {
        require(app->controls.count(key) && app->controls.at(key), "missing widget: " + key);
        auto wanted = app->controls.at(key);
        if (GImGui->NavId == wanted)
            return;
        std::set<std::pair<ImGuiID, int>> tried;
        for (int n = 0; n < 100; ++n) {
            auto nav = GImGui->NavWindow;
            require(nav != nullptr, "controller focus window missing");
            auto rect = ImGui::WindowRectRelToAbs(nav, nav->NavRectRel[0]);
            auto target = app->controlRects.at(key);
            float dx = (target.x + target.z) / 2 - rect.GetCenter().x;
            float dy = (target.y + target.w) / 2 - rect.GetCenter().y;
            bool horizontal = std::abs(dy) < std::max(10.0f, (target.w - target.y) * 0.6f);
            auto preferred =
                horizontal
                    ? (dx < 0 ? SDL_CONTROLLER_BUTTON_DPAD_LEFT : SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                    : (dy < 0 ? SDL_CONTROLLER_BUTTON_DPAD_UP : SDL_CONTROLLER_BUTTON_DPAD_DOWN);
            bool moved = false;
            for (auto direction :
                 {preferred, SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
                  SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT}) {
                if (!tried.insert({GImGui->NavId, int(direction)}).second)
                    continue;
                auto before = GImGui->NavId;
                press(direction);
                if (GImGui->NavId == wanted)
                    return;
                if (GImGui->NavId != before) {
                    moved = true;
                    break;
                }
            }
            if (!moved)
                break;
        }
        for (auto &item : app->controls)
            std::cerr << item.first << " = " << item.second << "\n";
        app->screenshot(fs::current_path() / "ui-gamepad-failure.bmp");
        throw std::runtime_error("unreachable with controller: " + key +
                                 " (nav=" + std::to_string(GImGui->NavId) +
                                 ", wanted=" + std::to_string(wanted) + ")");
    }
    void activate(const std::string &key) {
        find(key);
        press(SDL_CONTROLLER_BUTTON_A);
    }
    void adjust(const std::string &key, int &value) {
        find(key);
        int before = value;
        press(SDL_CONTROLLER_BUTTON_A);
        press(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        require(value > before, "slider did not respond to gamepad: " + key);
        press(SDL_CONTROLLER_BUTTON_B);
        require(app->menu, "back from slider closed menu: " + key);
    }
    void choose(const std::string &key) {
        activate(key);
        require(!GImGui->OpenPopupStack.empty(), "choice popup not opened: " + key);
        press(SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        press(SDL_CONTROLLER_BUTTON_A);
        require(GImGui->OpenPopupStack.empty(), "choice popup not confirmed: " + key);
    }
};
static void testCompactOverlays(Harness &h) {
    auto &a = *h.app;
    const auto previous = a.settings;
    const auto previousNotice = a.notice;
    a.menu = false;
    a.notice.clear();
    a.settings.view = 4;
    a.settings.help = true;
    a.settings.hintDetail = 0;
    a.settings.hintScale = 100;
    a.settings.hintMargin = 8;
    a.settings.hintSeconds = 0;
    a.settings.performance = false;
    auto visible = [&](const char *id) { return a.overlayBounds(id).z > 0; };
    auto inside = [&](const char *id, int w, int height) {
        auto r = a.overlayBounds(id);
        require(r.z > r.x && r.w > r.y && r.x >= 0 && r.y >= 0 && r.z <= w && r.w <= height,
                std::string("overlay outside drawable window: ") + id);
        auto window = ImGui::FindWindowByName((std::string("##overlay-") + id).c_str());
        require(window && std::abs(window->Pos.x - r.x) < 1 && std::abs(window->Pos.y - r.y) < 1 &&
                    std::abs(window->Size.x - (r.z - r.x)) < 1 &&
                    std::abs(window->Size.y - (r.w - r.y)) < 1,
                "actual overlay window disagrees with bounded layout");
    };
    for (auto size :
         {std::pair<int, int>{320, 240}, {640, 360}, {1080, 1920}, {1920, 1080}, {7680, 1440}}) {
        SDL_SetWindowSize(a.window, size.first, size.second);
        for (int language = 0; language < 4; ++language) {
            a.language = language;
            for (int corner = 0; corner < 4; ++corner) {
                a.settings.hintCorner = corner;
                a.settings.uiScale = corner; // Hint size must not follow menu scale.
                h.frame();
                inside("hint", size.first, size.second);
                auto r = a.overlayBounds("hint");
                require(r.z - r.x < 240 && r.w - r.y <= 28,
                        "default reminder is no longer a small single-line badge");
                require(std::abs((corner % 2 ? r.x : size.first - r.z) - 8) < 1 &&
                            std::abs((corner < 2 ? size.second - r.w : r.y) - 8) < 1,
                        "reminder is not anchored to its chosen screen corner");
            }
        }
        const auto native = a.nativeFrameBounds();
        a.settings.help = false;
        h.frame();
        const auto without = a.nativeFrameBounds();
        require(native.x == without.x && native.y == without.y && native.z == without.z &&
                    native.w == without.w,
                "reminder visibility still reserves a gameplay footer");
        a.settings.help = true;
    }
    SDL_SetWindowSize(a.window, 320, 240);
    a.settings.hintScale = 180;
    a.settings.hintMargin = 64;
    a.settings.hintDetail = 2;
    a.settings.performance = true;
    a.settings.noticeSeconds = 0;
    a.notice = std::string(600, 'W');
    for (int language = 0; language < 4; ++language) {
        a.language = language;
        for (int corner = 0; corner < 4; ++corner) {
            a.settings.hintCorner = corner;
            h.frame();
            for (auto id : {"hint", "notice", "performance"})
                inside(id, 320, 240);
        }
    }
    a.settings = Settings{};
    a.settings.fullscreen = false;
    a.settings.view = 4;
    a.settings.mute = true;
    a.settings.performance = true;
    a.settings.hintSeconds = a.settings.noticeSeconds = 1;
    a.overlayTestClock = 1000;
    a.menu = true;
    a.notice = "Timed notification";
    h.frame();
    a.toggleMenu();
    h.frame();
    require(visible("hint") && visible("notice"), "corner timers did not start on resume");
    a.overlayTestClock = 2001;
    h.frame();
    require(!visible("hint") && !visible("notice") && visible("performance") &&
                a.notice == "Timed notification",
            "overlay timeout hid independent performance or lost the menu notification");
    a.notify("Timed notification");
    h.frame();
    require(visible("notice"), "repeating the same notification did not restart its timer");
    h.press(SDL_CONTROLLER_BUTTON_BACK);
    require(a.menu, "Select cannot open options after reminder timeout");
    h.press(SDL_CONTROLLER_BUTTON_START);
    h.frame();
    require(!a.menu && visible("hint"), "resuming did not rearm the reminder");
    a.settings.help = false;
    a.core.setRewinding(true);
    h.frame();
    require(visible("rewind") && visible("performance"),
            "rewind or performance status depends on the menu reminder");
    a.settings.rewindIndicator = false;
    h.frame();
    require(!visible("rewind"), "rewind indicator toggle ignored");
    a.core.setRewinding(false);
    SDL_Event escape{};
    escape.type = SDL_KEYDOWN;
    escape.key.keysym.sym = SDLK_ESCAPE;
    escape.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
    a.event(escape);
    h.frame();
    require(a.menu, "Esc cannot open options while the reminder is disabled");
    escape.type = SDL_KEYUP;
    a.event(escape);
    a.settings.help = true;
    a.settings.hintSeconds = 0;
    a.settings.hintDetail = 1;
    a.toggleMenu();
    SDL_Event key{};
    key.type = SDL_KEYDOWN;
    key.key.keysym.sym = SDLK_x;
    key.key.keysym.scancode = SDL_SCANCODE_X;
    a.event(key);
    h.frame();
    require(a.overlayText("hint") == tr("hintkeyboard", a.language),
            "active-input reminder did not follow keyboard use");
    key.type = SDL_KEYUP;
    a.event(key);
    for (int labels = 1; labels <= 3; ++labels) {
        a.settings.padLabels = labels;
        h.press(SDL_CONTROLLER_BUTTON_A);
        require(a.overlayText("hint") == tr(labels == 2   ? "hintps"
                                            : labels == 3 ? "hintnintendo"
                                                          : "hintxbox",
                                            a.language),
                "active-input reminder did not follow controller labels");
    }
    const auto badge = a.overlayBounds("hint");
    SDL_Event mouse{};
    mouse.type = SDL_MOUSEMOTION;
    mouse.motion.windowID = SDL_GetWindowID(a.window);
    mouse.motion.x = int((badge.x + badge.z) / 2);
    mouse.motion.y = int((badge.y + badge.w) / 2);
    SDL_PushEvent(&mouse);
    h.frame(); // Active-device text changes, so use the resulting badge centre.
    const auto keyboardBadge = a.overlayBounds("hint");
    mouse.type = SDL_MOUSEBUTTONDOWN;
    mouse.button.windowID = SDL_GetWindowID(a.window);
    mouse.button.button = SDL_BUTTON_LEFT;
    mouse.button.x = int((keyboardBadge.x + keyboardBadge.z) / 2);
    mouse.button.y = int((keyboardBadge.y + keyboardBadge.w) / 2);
    SDL_PushEvent(&mouse);
    h.frame();
    mouse.type = SDL_MOUSEBUTTONUP;
    SDL_PushEvent(&mouse);
    h.frame();
    require(a.menu, "clicking the corner badge did not open options");
    a.settings = previous;
    a.language = detectLanguage(previous.language);
    a.notice = previousNotice;
    a.overlayTestClock = 0;
    SDL_SetWindowSize(a.window, 1280, 720);
    h.frame();
    std::cerr << "Compact overlays: 80 corner/language/size layouts, 16 crowded tiny layouts, "
                 "independent status, timers, shortcuts and native framing passed\n";
}
int main() {
    try {
        SDL_SetMainReady();
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        require(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER |
                         SDL_INIT_TIMER) == 0,
                SDL_GetError());
        {
            Harness h;
            auto &a = *h.app;
            h.activate("import");
            require(a.browsing(), "A must open ROM browser");
            h.activate("file:nested");
            h.frame();
            h.activate("editpath");
            require(a.editingPath(), "gamepad path editor");
            h.activate("key:A");
            h.activate("backspace");
            h.press(SDL_CONTROLLER_BUTTON_B);
            require(!a.editingPath() && a.browsing(), "B closes only path editor");
            h.activate("file:test.bin");
            require(!a.browsing() && a.library.entries.size() == 1, "gamepad ROM import");
            h.tab(0);
            h.activate("play-" + a.library.entries.front().id);
            require(a.core.loaded && !a.menu, "gamepad game launch");
            a.core.tick();
            h.press(SDL_CONTROLLER_BUTTON_BACK);
            require(a.menu, "Select opens options");
            std::cerr << "Controller import and launch passed\n";
            h.tab(1);
            h.choose("language");
            require(a.settings.language == 0, "language choice");
            h.choose("preset");
            require(a.settings.view == 2, "quality preset replaced stage view");
            h.choose("view");
            require(a.settings.view == 4, "view choice");
            h.tab(1);
            h.adjust("zoom", a.settings.zoom);
            h.activate("contrast");
            require(a.settings.highContrast, "checkbox via gamepad");
            h.activate("defaults");
            require(a.confirmation() == 3, "defaults needs confirmation");
            h.press(SDL_CONTROLLER_BUTTON_B);
            require(a.confirmation() == 0 && a.menu, "cancel defaults safely");
            h.tab(2);
            h.activate("showstage");
            require(a.settings.view == 2, "stage shortcut");
            h.activate("focusplayer");
            require(a.settings.view == 0 && a.settings.camera == 1, "focus shortcut");
            h.activate("enemypreview");
            require(!a.settings.enemyPreview, "enemy preview toggle via gamepad");
            h.activate("enemypreview");
            h.choose("enemystyle");
            require(a.settings.enemyStyle == 1, "enemy style choice via gamepad");
            h.adjust("enemyopacity", a.settings.enemyOpacity);
            for (auto item : {std::make_pair("bosspreview", &a.settings.bossPreview),
                              std::make_pair("scriptpreview", &a.settings.scriptPreview),
                              std::make_pair("spawntint", &a.settings.spawnTint),
                              std::make_pair("spawnlabels", &a.settings.spawnLabels),
                              std::make_pair("fitspawns", &a.settings.fitSpawns)}) {
                h.activate(item.first);
                require(!*item.second, std::string("spawn toggle via gamepad: ") + item.first);
                h.activate(item.first);
                require(*item.second,
                        std::string("restore spawn toggle via gamepad: ") + item.first);
            }
            h.tab(3);
            h.choose("backgroundstyle");
            h.adjust("blur", a.settings.blur);
            h.tab(4);
            h.adjust("volume", a.settings.volume);
            h.tab(5);
            h.choose("padlabels");
            h.choose("jumpbuttons");
            h.choose("rewindbutton");
            require(a.settings.rewindButton == 1, "rewind binding choice via gamepad");
            a.settings.rewindButton = 0;
            h.tab(6);
            h.activate("spindash");
            require(a.settings.spindash, "gameplay toggle");
            h.activate("rewindaudio");
            require(!a.settings.rewindAudio, "reverse sound toggle via gamepad");
            h.activate("rewindaudio");
            h.choose("rewindmemory");
            require(a.settings.rewindMemory == 2, "rewind memory choice via gamepad");
            h.tab(7);
            h.activate("slot2");
            require(a.settings.slot == 2, "select save slot");
            std::cerr << "Controller settings passed\n";
            h.activate("save");
            require(fs::exists(a.library.dataDir / "saves" / (a.core.saveKey + "-2.state")),
                    "save slot via gamepad");
            h.activate("save");
            require(a.confirmation() == 5, "overwrite confirmation");
            h.press(SDL_CONTROLLER_BUTTON_B);
            require(a.confirmation() == 0, "overwrite cancelled");
            h.activate("load");
            require(a.confirmation() == 4, "load confirmation");
            h.activate("yes");
            require(a.confirmation() == 0, "load confirmed by gamepad");
            std::cerr << "Controller saves and dialogs passed\n";
            h.tab(8);
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);
            h.frame(80);
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
            h.frame();
            bool scrolled = false;
            for (auto w : GImGui->Windows)
                if (std::string(w->Name).find("##page") != std::string::npos && w->Scroll.y > 10)
                    scrolled = true;
            require(scrolled, "right trigger must scroll help");
            h.press(SDL_CONTROLLER_BUTTON_START);
            require(!a.menu, "Start resumes");
            SDL_JoystickSetVirtualButton(h.joystick, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 1);
            h.frame();
            h.press(SDL_CONTROLLER_BUTTON_Y);
            require(a.settings.view == 2 && a.core.buttons[0] == 0,
                    "overview chord must not inject gameplay buttons");
            SDL_JoystickSetVirtualButton(h.joystick, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0);
            h.frame();
            a.settings.rewind = true;
            a.settings.rewindButton = 0;
            for (int displayRate : {10, 30, 60, 144}) {
                for (int n = 0; n < 90; ++n)
                    a.advanceGame(0, true);
                const auto before = a.core.frame;
                SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 32767);
                h.frame();
                require(a.rewindHeld, "L2/LT did not activate rewind");
                unsigned ticks = 0;
                for (int n = 0; n < displayRate; ++n)
                    ticks += a.advanceGame(1.0 / displayRate);
                require(ticks == unsigned(a.core.fps) && a.core.frame == before - ticks,
                        "Rewind speed depends on the display refresh rate");
                SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
                h.frame();
                require(!a.rewindHeld, "L2/LT release did not stop rewind");
                a.advanceGame(1 / a.core.fps);
                require(a.core.frame == before - ticks + 1 && !a.core.isRewinding(),
                        "Trigger release did not resume the restored timeline");
            }
            // Hysteresis prevents a lightly jittering analogue trigger from
            // rapidly switching sound direction. Raw joystick axes map to 0..32767.
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -22768);
            h.frame();
            require(!a.rewindHeld, "Trigger resting below activation threshold rewound");
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -12768);
            h.frame();
            require(a.rewindHeld, "Partially squeezed trigger did not activate");
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -22768);
            h.frame();
            require(a.rewindHeld, "Trigger hysteresis did not retain the hold");
            h.press(SDL_CONTROLLER_BUTTON_BACK);
            const auto pausedFrame = a.core.frame;
            require(a.menu && !a.rewindHeld && a.advanceGame(0.1) == 0 &&
                        a.core.frame == pausedFrame,
                    "Menu trigger scrolling rewound gameplay");
            h.press(SDL_CONTROLLER_BUTTON_START);
            require(!a.menu && !a.rewindHeld, "Held menu trigger leaked into resumed gameplay");
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
            h.frame();
            h.press(SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
            require(!a.rewindHeld, "LB is still the default rewind binding");
            a.settings.rewindButton = 1;
            SDL_JoystickSetVirtualButton(h.joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 1);
            h.frame();
            require(a.rewindHeld, "Optional LB binding did not activate");
            SDL_JoystickSetVirtualButton(h.joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0);
            a.settings.rewindButton = 2;
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);
            h.frame();
            require(a.rewindHeld, "Optional RT binding did not activate");
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
            a.settings.rewindButton = 0;
            h.frame();
            std::cerr << "L2/LT rewind, 30/60/144 Hz pacing, bindings and menu isolation passed\n";
            h.press(SDL_CONTROLLER_BUTTON_BACK);
            h.tab(1);
            // Visit every enabled option on every page, using directional input.
            std::vector<std::vector<std::string>> expected = {
                {"folder", "readyonly", "continue"},
                {"language",   "preset",     "view",           "overviewstyle", "zoom",
                 "zoomout",    "pixelreset", "panoramaheight", "mapextension",  "smoothmap",
                 "fullscreen", "span",       "display",        "menulayout",    "uiscale",
                 "contrast",   "vsync",      "nearest",        "nativescale",   "workers",
                 "soft",       "scanlines",  "performance",    "defaults"},
                {"showstage",     "focusplayer",   "playerinset",    "enemypreview", "bosspreview",
                 "scriptpreview", "enemystyle",    "enemyopacity",   "spawntint",    "spawnlabels",
                 "fitspawns",     "cameramode",    "follow",         "panspeed",     "hud",
                 "hudsize",       "locator",       "minimap",        "ghost",        "help",
                 "hintcorner",    "hintdetail",    "hintscale",      "hintopacity",  "hintmargin",
                 "hintseconds",   "noticeseconds", "rewindindicator"},
                {"backgroundstyle", "blur", "brightness", "parallax", "motion"},
                {"volume", "mute", "pausefocus"},
                {"padlabels", "jumpbuttons", "rewindbutton", "quickcontrols", "deadzone"},
                {"lockon", "spindash", "fastboot", "spritelimit", "rewind", "rewindaudio",
                 "rewindmemory", "assist", "air", "region", "confirmactions", "confirmquit"},
                {"autosave", "slot1", "slot2", "slot3", "slot4", "slot5", "slot6", "slot7", "slot8",
                 "slot9", "save", "load", "reset", "capture"},
                {"sourcehelp"}};
            // A loaded game uses Resume, rather than Continue, in the header.
            expected[0].pop_back();
            for (int n = 0; n < 9; ++n) {
                h.tab(n);
                for (auto &key : expected[n]) {
                    h.find(key);
                    if (n >= 1 && n <= 7) {
                        h.frame(1);
                        require(settingHelp(key.c_str()),
                                "setting lacks illustrated guide: " + key);
                        require(a.guideTopic() == key,
                                "guide did not follow controller focus: " + key);
                    }
                }
                if (n >= 1 && n <= 7) {
                    h.press(SDL_CONTROLLER_BUTTON_Y);
                    require(a.guideVisible(), "top face button must open setting explanation");
                    h.press(SDL_CONTROLLER_BUTTON_B);
                    require(!a.guideVisible() && a.menu, "B must close guide without leaving menu");
                }
            }
            std::cerr << "All settings reached\n";
            h.tab(2);
            h.choose("hintcorner");
            h.choose("hintdetail");
            h.adjust("hintscale", a.settings.hintScale);
            h.adjust("hintopacity", a.settings.hintOpacity);
            h.adjust("hintmargin", a.settings.hintMargin);
            h.adjust("hintseconds", a.settings.hintSeconds);
            h.adjust("noticeseconds", a.settings.noticeSeconds);
            const bool oldIndicator = a.settings.rewindIndicator;
            h.activate("rewindindicator");
            require(a.settings.rewindIndicator != oldIndicator, "rewind indicator via gamepad");
            testCompactOverlays(h);
            h.tab(1);
            h.find("zoomout");
            SDL_Event helpKey{};
            helpKey.type = SDL_KEYDOWN;
            helpKey.key.keysym.sym = SDLK_F1;
            helpKey.key.keysym.scancode = SDL_SCANCODE_F1;
            a.event(helpKey);
            h.frame();
            require(a.guideVisible() && a.guideTopic() == "zoomout",
                    "F1 must explain the focused setting");
            helpKey.type = SDL_KEYUP;
            a.event(helpKey);
            h.press(SDL_CONTROLLER_BUTTON_B);
            require(!a.guideVisible() && a.menu, "guide keyboard/controller close");
            a.core
                .unload(); // Resize checks exercise the menu without expensive backdrop upscaling.
            for (auto size : {std::pair<int, int>{320, 240},
                              {640, 360},
                              {1080, 1920},
                              {1920, 1080},
                              {7680, 1440}}) {
                SDL_SetWindowSize(a.window, size.first, size.second);
                h.frame();
                for (int n = 0; n < 9; ++n) {
                    h.tab(n);
                    require(GImGui->NavId != 0, "no gamepad focus at resized menu");
                }
                h.tab(1);
                h.find("zoomout");
                h.press(SDL_CONTROLLER_BUTTON_Y);
                require(a.guideVisible(), "illustrated detail unavailable at resized menu");
                h.press(SDL_CONTROLLER_BUTTON_BACK);
                require(!a.guideVisible() && a.menu, "Select must close guide at resized menu");
            }
            require(a.play(a.library.entries.front().id), "reload for quit test");
            h.press(SDL_CONTROLLER_BUTTON_BACK);
            h.tab(0);
            h.activate("quit");
            require(a.confirmation() == 1, "quit confirmation");
            h.press(SDL_CONTROLLER_BUTTON_B);
            require(a.running && a.confirmation() == 0, "B cancels quit");
            // Reconnection uses a fresh SDL handle and restores navigation.
            SDL_JoystickClose(h.joystick);
            h.joystick = nullptr;
            SDL_JoystickDetachVirtual(h.index);
            h.index = -1;
            h.frame();
            for (auto pad : a.pads)
                require(!pad ||
                            std::string(SDL_GameControllerName(pad)) != "SCU virtual controller",
                        "removed gamepad remained connected");
            h.index =
                SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                          SDL_CONTROLLER_BUTTON_MAX, 0);
            require(h.index >= 0, "virtual gamepad reconnect");
            h.joystick = SDL_JoystickOpen(h.index);
            require(h.joystick != nullptr, "open reconnected gamepad");
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
            SDL_JoystickSetVirtualAxis(h.joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
            h.frame();
            int before = a.menuTab();
            h.press(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            require(a.menuTab() == (before + 1) % 9, "reconnected controller cannot navigate");
            std::cout << "PASS: SDL virtual gamepad import, path keyboard, all settings/pages, "
                         "sliders/choices, saves, confirmations, shortcuts, 5 sizes and reconnect; "
                      << h.visited.size() << " focused controls\n";
        }
        SDL_Quit();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        SDL_Quit();
        return 1;
    }
}
