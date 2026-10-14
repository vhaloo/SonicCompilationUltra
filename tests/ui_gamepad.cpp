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
            for (int displayRate : {30, 60, 144}) {
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
                {"language", "preset", "view", "overviewstyle", "zoom", "fullscreen", "span",
                 "display", "menulayout", "uiscale", "contrast", "vsync", "nearest", "nativescale",
                 "workers", "soft", "scanlines", "performance", "defaults"},
                {"showstage",     "focusplayer", "playerinset",  "enemypreview", "bosspreview",
                 "scriptpreview", "enemystyle",  "enemyopacity", "spawntint",    "spawnlabels",
                 "fitspawns",     "cameramode",  "follow",       "panspeed",     "hud",
                 "hudsize",       "locator",     "minimap",      "ghost",        "help"},
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
                for (auto &key : expected[n])
                    h.find(key);
            }
            std::cerr << "All settings reached\n";
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
