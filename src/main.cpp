#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif
#include "ui.h"
#include "quest.h"
#include "mobile.h"
#include <SDL.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <map>
#include <chrono>
#include <cmath>
#include <thread>
using namespace scu;
struct Action {
    unsigned step;
    std::string verb, args;
};
static uint16_t buttonMask(std::string text) {
    for (auto &c : text)
        if (c == ',')
            c = ' ';
    std::istringstream in(text);
    std::string s;
    uint16_t result = 0;
    while (in >> s) {
        if (s == "UP")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_UP;
        else if (s == "DOWN")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN;
        else if (s == "LEFT")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT;
        else if (s == "RIGHT")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        else if (s == "A")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_Y;
        else if (s == "B")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_B;
        else if (s == "C")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_A;
        else if (s == "START")
            result |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
    }
    return result;
}
static void view(Settings &s, const std::string &v) {
    s.view = v == "pixel"      ? 0
             : v == "panorama" ? 1
             : v == "overview" ? 2
             : v == "classic"  ? 4
                               : std::stoi(v);
}
static void option(Settings &s, const std::string &key, const std::string &value) {
    if (key == "view") {
        view(s, value);
        return;
    }
    int n = std::stoi(value);
#define SET(k)                                                                                     \
    if (key == #k) {                                                                               \
        s.k = n;                                                                                   \
        return;                                                                                    \
    }
    SET(zoom)
    SET(zoomOut)
    SET(panoramaHeight)
    SET(mapExtension)
    SET(smoothMap)
    SET(camera)
    SET(voxelMode)
    SET(voxelDetail)
    SET(voxelFraming)
    SET(voxelYaw)
    SET(voxelPitch)
    SET(voxelThickness)
    SET(voxelSpacing)
    SET(voxelRelief)
    SET(voxelLight)
    SET(voxelBackground)
    SET(desktopPresentation)
    SET(hud)
    SET(hudScale)
    SET(background)
    SET(blur)
    SET(brightness)
    SET(parallax)
    SET(minimap)
    SET(locator)
    SET(spindash)
    SET(invincible)
    SET(infiniteAir)
    SET(rewind) SET(nativeScale) SET(ghost) SET(left) SET(top) SET(volume) SET(language) SET(help)
        SET(workers) SET(fullscreen) SET(span) SET(vsync) SET(uiScale) SET(scanlines) SET(lockon)
            SET(fastBoot) SET(menuLayout) SET(display) SET(padLabels) SET(jumpLayout)
                SET(quickControls) SET(confirmActions) SET(confirmQuit) SET(highContrast)
                    SET(readyOnly) SET(playerInset) SET(quality) SET(overviewStyle)
                        SET(enemyPreview) SET(enemyOpacity) SET(enemyStyle) SET(bossPreview)
                            SET(scriptPreview) SET(spawnTint) SET(spawnLabels) SET(fitSpawns)
                                SET(rewindAudio) SET(rewindButton) SET(rewindMemory) SET(hintCorner)
                                    SET(hintScale) SET(hintOpacity) SET(hintMargin) SET(hintDetail)
                                        SET(hintSeconds) SET(noticeSeconds) SET(rewindIndicator)
                                            SET(performance)
#undef SET
                                                throw std::runtime_error("Unknown option: " + key);
}
static void execute(App &a, const Action &cmd, uint16_t &mask, bool &rewindMask) {
    std::istringstream in(cmd.args);
    if (cmd.verb == "hold")
        mask = buttonMask(cmd.args);
    else if (cmd.verb == "menu")
        a.menu = cmd.args == "on";
    else if (cmd.verb == "tab")
        a.showSection(std::stoi(cmd.args));
    else if (cmd.verb == "guide")
        a.showGuide(cmd.args);
    else if (cmd.verb == "option") {
        std::string key, val;
        in >> key >> val;
        option(a.settings, key, val);
        a.language = detectLanguage(a.settings.language);
        a.core.updateOptions();
        a.dirty = true;
        if (key == "fullscreen" || key == "span")
            a.applyDisplay();
    } else if (cmd.verb == "save")
        a.core.save(std::stoi(cmd.args));
    else if (cmd.verb == "load")
        a.core.loadState(std::stoi(cmd.args));
    else if (cmd.verb == "rewindhold")
        rewindMask = cmd.args == "on";
    else if (cmd.verb == "rewind") {
        if (!a.core.rewind())
            throw std::runtime_error("No rewind checkpoint available");
    } else if (cmd.verb == "game") {
        if (!a.play(cmd.args))
            throw std::runtime_error(a.notice);
    } else if (cmd.verb == "reset")
        a.core.reset();
    else if (cmd.verb == "present") {
        const auto &frame = a.core.native;
        if (frame.empty() || std::all_of(frame.begin(), frame.end(),
                                         [&](auto pixel) { return pixel == frame.front(); }))
            throw std::runtime_error("Blank cartridge frame at timeline step " +
                                     std::to_string(cmd.step));
        a.present();
    } else if (cmd.verb == "assert_scene") {
        unsigned family = 0, stage = 0;
        auto scene = a.core.world.inspect(a.core.snapshot, a.core.game);
        if (!(in >> family >> std::hex >> stage) || !scene.ready || scene.family != int(family) ||
            scene.stage != int(stage))
            throw std::runtime_error("Unexpected cartridge scene at timeline step " +
                                     std::to_string(cmd.step));
    } else if (cmd.verb == "capture") {
        a.capturePath = cmd.args;
        a.present(true);
        a.capturePath.clear();
    } else if (cmd.verb == "assert") {
        std::string addr, value;
        in >> addr >> value;
        auto p = std::stoul(addr, nullptr, 16) & 65535, v = std::stoul(value, nullptr, 16);
        if (a.core.snapshot.ram[p] != v)
            throw std::runtime_error("RAM assertion failed at " + addr);
#ifdef SCU_PRIVATE_QA
    } else if (cmd.verb == "vpad") {
        if (!a.headless)
            throw std::runtime_error("Virtual controller probes require an isolated session");
        static std::array<SDL_JoystickID, 2> devices{{-1, -1}};
        int slot, control, value;
        std::string operation;
        if (!(in >> slot >> operation) || slot < 0 || slot >= int(devices.size()))
            throw std::runtime_error("Invalid virtual controller slot");
        if (operation == "attach") {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            SDL_VirtualJoystickDesc desc{};
            desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
            desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
            desc.naxes = SDL_CONTROLLER_AXIS_MAX;
            desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
            desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX) - 1;
            desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX) - 1;
            desc.name = slot ? "SCU QA Controller B" : "SCU QA Controller A";
            int index = SDL_JoystickAttachVirtualEx(&desc);
            if (index < 0)
                throw std::runtime_error(SDL_GetError());
            devices[slot] = SDL_JoystickGetDeviceInstanceID(index);
        } else {
            auto *pad = SDL_JoystickFromInstanceID(devices[slot]);
            if (!pad || !(in >> control >> value))
                throw std::runtime_error("Virtual controller unavailable");
            const int result = operation == "axis" ? SDL_JoystickSetVirtualAxis(pad, control, value)
                             : operation == "button" ? SDL_JoystickSetVirtualButton(pad, control, value)
                                                     : -1;
            if (result)
                throw std::runtime_error(SDL_GetError());
            SDL_JoystickUpdate();
        }
    } else if (cmd.verb == "ram8" || cmd.verb == "ram16") {
        if (!a.headless || !a.core.loaded)
            throw std::runtime_error("Cartridge-state probes require an isolated loaded session");
        unsigned address = 0, value = 0;
        bool word = cmd.verb == "ram16";
        if (!(in >> std::hex >> address >> value) || address > (word ? 65534u : 65535u) ||
            (word && (address & 1)) || value > (word ? 65535u : 255u))
            throw std::runtime_error("Invalid cartridge-state probe");
        if (word)
            scu_ram_write16(address, uint16_t(value));
        else
            scu_ram_write8(address, uint8_t(value));
#endif
    } else if (cmd.verb == "trace") {
        auto &r = a.core.snapshot.ram;
        fprintf(stderr,
                "TRACE step=%u frame=%llu mask=%04X mode=%02X pad=%02X/%02X s1routine=%d s2id=%d "
                "vs=%04X px=%u charge=%d menu=%d\n",
                cmd.step, (unsigned long long)a.core.frame, mask, r[0xF600], r[0xF604], r[0xF605],
                r[0xD024], r[0xB000], be16(r, 0xFFD8), be16(r, 0xD008), a.core.spinCharge,
                a.menu ? 1 : 0);
    } else if (cmd.verb == "event") {
        SDL_Event e{};
        if (cmd.args == "select") {
            e.type = SDL_CONTROLLERBUTTONDOWN;
            e.cbutton.button = SDL_CONTROLLER_BUTTON_BACK;
        } else {
            e.type = SDL_KEYDOWN;
            e.key.state = SDL_PRESSED;
            e.key.keysym.sym = cmd.args == "esc"     ? SDLK_ESCAPE
                               : cmd.args == "enter" ? SDLK_RETURN
                               : cmd.args == "down"  ? SDLK_DOWN
                               : cmd.args == "up"    ? SDLK_UP
                               : cmd.args == "right" ? SDLK_RIGHT
                                                     : SDLK_LEFT;
            e.key.keysym.scancode = SDL_GetScancodeFromKey(e.key.keysym.sym);
        }
        a.event(e);
        if (e.type == SDL_KEYDOWN) {
            e.type = SDL_KEYUP;
            e.key.state = SDL_RELEASED;
            a.event(e);
        }
    } else
        throw std::runtime_error("Unknown timeline command: " + cmd.verb);
}
int main(int argc, char **argv) {
#ifdef _WIN32
    int wideCount = 0;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &wideCount);
    std::vector<std::string> utf8;
    std::vector<char *> pointers;
    if (wide) {
        for (int i = 0; i < wideCount; i++) {
            int count = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
            std::string text(count, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, text.data(), count, nullptr, nullptr);
            text.pop_back();
            utf8.push_back(std::move(text));
        }
        LocalFree(wide);
        for (auto &s : utf8)
            pointers.push_back(s.data());
        argc = wideCount;
        argv = pointers.data();
    }
#endif
    SDL_SetMainReady();
    std::map<std::string, std::string> args;
    std::vector<std::string> imports;
    try {
        for (int i = 1; i < argc; i++) {
            std::string k = argv[i];
            if (k == "--help") {
                printf(
                    "Sonic Compilation Ultra\nUsage: SonicCompilationUltra [--game ID] [--import "
                    "ROM] [--data-dir DIR]\n  --fullscreen --mute --software --language "
                    "en|fr|es|ja|auto\n  --view pixel|panorama|overview|classic --zoom 1..8 "
                    "--preset light|balanced|ultra\n  --size WIDTHxHEIGHT --headless --frames N "
                    "--script TIMELINE\n  --capture FILE.bmp --report FILE.json --render-every N "
                    "--realtime --resume-last\nROMs and copyrighted game assets are not "
                    "included.\n");
                return 0;
            }
            if (k == "--fullscreen" || k == "--mute" || k == "--software" || k == "--headless" ||
                k == "--realtime" || k == "--menu" || k == "--no-autoresume" ||
                k == "--no-fast-boot" || k == "--resume-last" || k == "--menu-only" ||
                k == "--xr-options") {
                args[k] = "1";
                continue;
            }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing argument: " + k);
            std::string val = argv[++i];
            if (k == "--import")
                imports.push_back(val);
            else
                args[k] = val;
        }
        if (args.count("--spawn-audit")) {
            auto rom = readBytes(fs::u8path(args["--spawn-audit"]));
            const Game *game = identify(rom);
            if (!game || !game->family)
                throw std::runtime_error("Spawn audit requires a supported platform cartridge");
            int stage =
                args.count("--audit-stage") ? std::stoi(args["--audit-stage"], nullptr, 16) : 0;
            auto snapshot = std::make_unique<SCUSnapshot>();
            *snapshot = {};
            EnemyPreviews previews;
            previews.update(*snapshot, game, rom, stage, 0, 0);
            std::ostringstream out;
            out << "{\"version\":\"" << SCU_VERSION << "\",\"game\":\"" << game->id
                << "\",\"stage\":" << stage << ",\"placements\":" << previews.placementCount
                << ",\"bosses\":" << previews.bossCount << ",\"scripts\":" << previews.scriptCount
                << ",\"points\":[";
            bool first = true;
            for (const auto &p : previews.visible) {
                if (!first)
                    out << ',';
                first = false;
                out << "{\"x\":" << p.x << ",\"y\":" << p.y << ",\"kind\":" << int(p.kind)
                    << ",\"relative\":" << (p.relative ? "true" : "false")
                    << ",\"variable\":" << (p.variable ? "true" : "false")
                    << ",\"sprite\":" << (p.image ? "true" : "false") << '}';
            }
            out << "]}\n";
            if (args.count("--report")) {
                std::ofstream file(fs::u8path(args["--report"]));
                file << out.str();
                if (!file)
                    throw std::runtime_error("Cannot write spawn audit report");
            } else
                printf("%s", out.str().c_str());
            return 0;
        }
#ifdef SCU_QUEST
        SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
#endif
        if (args.count("--startup-error"))
            throw std::runtime_error(args["--startup-error"]);
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER))
            throw std::runtime_error(SDL_GetError());
        fs::path dir;
        if (args.count("--data-dir"))
            dir = fs::u8path(args["--data-dir"]);
        else {
            char *p = SDL_GetPrefPath("vhaloo", "SonicCompilationUltra");
            if (!p)
                throw std::runtime_error(SDL_GetError());
            dir = fs::u8path(p);
            SDL_free(p);
        }
#if defined(SCU_QUEST) || defined(SCU_MOBILE)
        fs::current_path(dir.parent_path());
#endif
        auto holder = std::make_unique<App>(dir);
        auto &a = *holder;
        a.menuVerificationOnly = args.count("--menu-only");
        a.core.executionDisabled = a.menuVerificationOnly;
        if (a.menuVerificationOnly)
            SDL_Log("SCU menu verification: cartridge execution locked at core boundary");
        a.headless = args.count("--headless");
        a.core.headless = a.headless;
        char *base = SDL_GetBasePath();
        fs::path baseDir = base ? fs::u8path(base) : fs::current_path();
        SDL_free(base);
#if defined(SCU_QUEST) || defined(SCU_MOBILE)
        baseDir = dir.parent_path();
#endif
#ifdef __APPLE__
        if (fs::exists(baseDir / "../Resources/assets/SCUInterface.ttf"))
            baseDir = (baseDir / "../Resources").lexically_normal();
#endif
        if (args.count("--software"))
            SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE);
        try {
            a.initialise(baseDir, a.headless);
        } catch (const std::exception &error) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SCU startup failed: %s", error.what());
            throw;
        }
#ifdef SCU_QUEST
        // Only personal APKs opt into this one-time profile, including on upgrade.
        const auto personalFlag = baseDir / "personal-wall-preset-0500.txt";
        const auto personalMarker = dir / "personal-wall-preset-0500.applied";
        if (!a.menuVerificationOnly && fs::is_regular_file(personalFlag) &&
            !fs::exists(personalMarker)) {
            const auto backup = dir / "settings-before-personal-wall-0500.ini";
            if (fs::is_regular_file(dir / "settings.ini") && !fs::exists(backup))
                writeBytesAtomic(backup, readBytes(dir / "settings.ini"));
            a.settings.questMode = 2;
            a.settings.questPassthrough = true;
            a.settings.questWallHeight = 200;
            a.settings.questWallGap = 0;
            a.settings.questRefresh = 90;
            a.settings.questAutoWall = true;
            a.settings.questWallLocked = true;
            a.settings.questCompanion = true;
            a.settings.questMenuDistance = 100;
            a.settings.questWallDetail = 1;
            a.settings.questAdaptive = true;
            a.settings.questPauseLost = true;
            a.settings.view = 2;
            a.settings.camera = 4;
            a.settings.fitSpawns = false;
            a.settings.mapExtension = true;
            a.settings.menuLayout = 0;
            a.settings.save(dir);
            const std::string note =
                "Personal wall profile 0500 applied; later choices are kept.\n";
            writeBytesAtomic(personalMarker, std::vector<uint8_t>(note.begin(), note.end()));
        }
        const auto xrProfile = baseDir / "personal-xr-preset-0600.txt";
        const auto xrMarker = dir / "personal-xr-preset-0600.applied";
        if (!a.menuVerificationOnly && fs::is_regular_file(xrProfile) && !fs::exists(xrMarker)) {
            const auto backup = dir / "settings-before-personal-xr-0600.ini";
            if (fs::is_regular_file(dir / "settings.ini") && !fs::exists(backup))
                writeBytesAtomic(backup, readBytes(dir / "settings.ini"));
            a.settings.questMode = 2;
            a.settings.questPassthrough = true;
            a.settings.questSpinballAuto = true;
            a.settings.questScale = 100;
            a.settings.questWallHeight = 200;
            a.settings.questWallGap = 0;
            a.settings.questRefresh = 90;
            a.settings.questWallLocked = true;
            a.settings.questCompanion = true;
            a.settings.questCabinetWidth = 140;
            a.settings.questCabinetHeight = 85;
            a.settings.questCabinetTilt = 12;
            a.settings.questSurround = true;
            a.settings.questCrt = 2;
            a.settings.questAutoWall = true;
            a.settings.questAdaptive = true;
            a.settings.view = 2;
            a.settings.mapExtension = true;
            a.settings.menuLayout = 0;
            a.settings.save(dir);
            const std::string note =
                "Personal XR 0600 defaults applied; later choices are retained.\n";
            writeBytesAtomic(xrMarker, std::vector<uint8_t>(note.begin(), note.end()));
        }
        if (a.menuVerificationOnly && args.count("--xr-preview")) {
            a.settings.questMode = std::clamp(std::stoi(args["--xr-preview"]), 0, 6);
            a.settings.questAutoWall = false;
            a.settings.questPassthrough = false;
            a.settings.questCompanion = true;
        }
        if (a.menuVerificationOnly && args.count("--xr-capture"))
            a.xrMenuCapture = args["--xr-capture"];
        if (a.menuVerificationOnly && args.count("--xr-options"))
            a.showSection(9);
        auto headset = std::make_unique<QuestPresentation>(a);
        a.quest = headset.get();
        headset->initialise();
#endif
#ifdef SCU_MOBILE
        auto mobile = std::make_unique<MobilePresentation>(a);
        mobile->beginFrame();
#endif
        if (args.count("--preset"))
            a.settings.preset(args["--preset"] == "light"   ? 0
                              : args["--preset"] == "ultra" ? 2
                                                            : 1);
        if (args.count("--no-autoresume"))
            a.settings.autosave = false;
        if (args.count("--no-fast-boot"))
            a.settings.fastBoot = false;
        if (args.count("--mute"))
            a.settings.mute = true;
        if (args.count("--software"))
            a.settings.renderer = 1;
        if (args.count("--language")) {
            auto l = args["--language"];
            a.settings.language = l == "fr"   ? 1
                                  : l == "es" ? 2
                                  : l == "ja" ? 3
                                  : l == "en" ? 0
                                              : -1;
            a.language = detectLanguage(a.settings.language);
        }
        if (args.count("--view"))
            view(a.settings, args["--view"]);
        if (args.count("--zoom"))
            a.settings.zoom = std::clamp(std::stoi(args["--zoom"]), 1, 8);
        if (args.count("--fullscreen")) {
            a.settings.fullscreen = true;
            a.applyDisplay();
        }
        if (args.count("--size")) {
            std::string v = args["--size"];
            auto x = v.find('x');
            if (x == std::string::npos)
                throw std::runtime_error("Size format must be WIDTHxHEIGHT");
            a.settings.fullscreen = false;
            a.applyDisplay();
            SDL_SetWindowSize(a.window, std::stoi(v.substr(0, x)), std::stoi(v.substr(x + 1)));
        }
        for (auto &p : imports)
            a.import(fs::u8path(p));
        if (args.count("--scan"))
            a.import(fs::u8path(args["--scan"]), true);
        if (args.count("--game")) {
            if (!a.play(args["--game"]))
                throw std::runtime_error(a.notice);
        } else if (args.count("--resume-last") && a.library.find(a.settings.lastGame)) {
            // If a cartridge has moved, keep the GUI and importer available.
            a.play(a.settings.lastGame);
        }
        if (args.count("--menu"))
            a.menu = true;
        std::vector<Action> actions;
        if (args.count("--script")) {
            std::ifstream f(fs::u8path(args["--script"]));
            if (!f)
                throw std::runtime_error("Cannot read timeline");
            std::string line;
            while (std::getline(f, line)) {
                if (line.empty() || line[0] == '#')
                    continue;
                std::istringstream in(line);
                Action v{};
                if (!(in >> v.step >> v.verb))
                    throw std::runtime_error("Invalid timeline row");
                std::getline(in, v.args);
                if (!v.args.empty() && v.args[0] == ' ')
                    v.args.erase(0, 1);
                actions.push_back(std::move(v));
            }
            std::stable_sort(actions.begin(), actions.end(),
                             [](auto &x, auto &y) { return x.step < y.step; });
        }
        unsigned limit = args.count("--frames") ? std::stoul(args["--frames"]) : 0,
                 renderEvery = args.count("--render-every")
                                   ? std::max(1, std::stoi(args["--render-every"]))
                                   : 1,
                 step = 0;
        size_t next = 0;
        uint16_t mask = 0;
        bool rewindMask = false;
        bool scripted = args.count("--script");
        double freq = (double)SDL_GetPerformanceFrequency(),
               previous = SDL_GetPerformanceCounter() / freq, start = previous,
               measureStart = previous;
        uint64_t measureFrames = 0, executedFrames = 0, presentedFrames = 0;
        const bool measurePresentation = args.count("--report") && renderEvery == 1 &&
                                         args.count("--realtime");
        std::array<uint64_t, 251> presentationIntervals{};
        double lastFrameTime = 0;
        FrameDeadline pacing;
        double panelHz = 60;
        uint64_t displayPoll = 0;
        while (a.running) {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                a.event(e);
#ifdef SCU_QUEST
            if (!headset->beginFrame()) {
                // Do not turn a suspended headset session into simulation catch-up.
                previous = SDL_GetPerformanceCounter() / freq;
                a.core.clearAudio();
                SDL_Delay(10);
                continue;
            }
            // Deliver freshly sampled Touch input before the simulation and menu.
            while (SDL_PollEvent(&e))
                a.event(e);
#endif
#ifdef SCU_MOBILE
            mobile->beginFrame();
            mobile->poll();
#endif
            double now = SDL_GetPerformanceCounter() / freq, rawElapsed = now - previous,
                   elapsed = std::min(0.25, rawElapsed);
            previous = now;
            while (next < actions.size() && actions[next].step == step) {
                execute(a, actions[next], mask, rewindMask);
                ++next;
            }
            a.pollInput();
            if (scripted) {
                a.core.buttons[0] = mask;
                a.rewindHeld = rewindMask;
            }
            double targetHz = a.settings.performanceTarget;
#ifdef SCU_QUEST
            targetHz = headset->refreshLimit();
#elif defined(SCU_MOBILE)
            targetHz = mobile->refreshLimit();
#else
            if (SDL_GetTicks64() - displayPoll > 1000 || !displayPoll) {
                SDL_DisplayMode mode{};
                if (SDL_GetCurrentDisplayMode(std::max(0, SDL_GetWindowDisplayIndex(a.window)),
                                              &mode) == 0 &&
                    mode.refresh_rate > 0)
                    panelHz = mode.refresh_rate;
                displayPoll = SDL_GetTicks64();
            }
            targetHz = pacedRefresh(targetHz, panelHz, a.settings.vsync);
#endif
            PerformancePolicy policy;
            policy.enabled = a.settings.autoPerformance;
#ifdef SCU_QUEST
            policy.enabled &= a.settings.questAdaptive;
#endif
            policy.thermalAware = a.settings.thermalAware;
            policy.reservePercent = a.settings.performanceHeadroom;
            policy.effects = a.settings.adaptiveEffects;
#ifdef SCU_QUEST
            policy.stereo = true;
#endif
            policy.targetHz = targetHz;
            policy.minimum = a.settings.minimumRenderScale;
            policy.maximum = std::max(a.settings.minimumRenderScale, a.settings.maximumRenderScale);
            a.performanceGovernor.configure(policy);
            a.settings.stereoScale = a.performanceGovernor.stereoScale();
            a.settings.effectsLevel = a.performanceGovernor.effectLevel();
            const auto simulationStarted = SDL_GetPerformanceCounter();
            const unsigned ticks = a.advanceGame(elapsed, limit && !args.count("--realtime"));
            const double simulationMs =
                double(SDL_GetPerformanceCounter() - simulationStarted) * 1000 / freq;
            bool drewFrame = false;
            measureFrames += ticks;
            executedFrames += ticks;
            if (now - measureStart >= 1) {
                a.measuredFps = measureFrames / (now - measureStart);
                measureFrames = 0;
                measureStart = now;
            }
            if ((step % renderEvery == 0 || a.menu)
#ifdef SCU_QUEST
                && headset->needsRedraw()
#endif
            ) {
#ifdef SCU_QUEST
                headset->prepareRender();
#endif
                a.present();
                drewFrame = true;
#ifdef SCU_QUEST
                headset->rendered();
#endif
                ++presentedFrames;
                if (measurePresentation) {
                    const double frameTime = SDL_GetPerformanceCounter() / freq;
                    if (lastFrameTime)
                        ++presentationIntervals[std::clamp(
                            int(std::round((frameTime - lastFrameTime) * 1000)), 0, 250)];
                    lastFrameTime = frameTime;
                }
            }
#ifdef SCU_QUEST
            headset->endFrame();
#endif
#ifdef SCU_MOBILE
            mobile->endFrame();
#endif
            PerformanceSample sample;
            sample.simulationMs = simulationMs;
            sample.renderMs = drewFrame ? a.lastRenderMs : 0;
            // Options are also an active XR scene. Measure their actual work;
            // the frame/swap waits remain outside the measured intervals.
            sample.active = a.focused;
            sample.thermal = a.thermalStatus;
#ifdef SCU_QUEST
            sample.presentationMs = headset->submissionMilliseconds();
            sample.gpuMs = headset->gpuMilliseconds();
#elif defined(SCU_MOBILE)
            sample.presentationMs = mobile->lastSubmissionMs;
            if (a.lastPresentMs > 1000. / targetHz * 1.2)
                sample.gpuMs = a.lastPresentMs;
#else
            // A sustained blocking swap longer than a whole refresh period can
            // expose GPU pressure even with SDL renderers lacking timer queries.
            if (a.lastPresentMs > 1000. / targetHz * 1.2)
                sample.gpuMs = a.lastPresentMs;
#endif
            a.performanceGovernor.observe(sample, rawElapsed);
            if (a.dirty && step % 60 == 0)
                a.flushSettings();
            ++step;
            if (limit && step >= limit)
                break;
            if (args.count("--seconds") && now - start >= std::stod(args["--seconds"]))
                break;
#if !defined(SCU_QUEST) && !defined(SCU_MOBILE)
            if (!limit || args.count("--realtime"))
                if (double delay = pacing.delay(SDL_GetPerformanceCounter() / freq,
                                                a.focused || a.headless ? targetHz : 15.);
                    delay > 0)
                    SDL_Delay(Uint32(std::ceil(delay * 1000)));
#endif
        }
        if (args.count("--capture")) {
            a.capturePath = args["--capture"];
            a.present(true);
        }
        if (args.count("--dump-snapshot")) {
            auto path = fs::u8path(args["--dump-snapshot"]);
            std::vector<uint8_t> bytes((const uint8_t *)&a.core.snapshot,
                                       (const uint8_t *)&a.core.snapshot + sizeof a.core.snapshot);
            writeBytesAtomic(path, bytes);
        }
        if (args.count("--report")) {
            auto v = a.core.world.inspect(a.core.snapshot, a.core.game);
            SDL_RendererInfo ri{};
            SDL_GetRendererInfo(a.renderer, &ri);
            auto hintBounds = a.overlayBounds("hint");
            auto nativeBounds = a.nativeFrameBounds();
            auto insetBounds = a.playerInsetBounds();
            const auto voxelPicture = a.voxelPictureSize();
            double duration = SDL_GetPerformanceCounter() / freq - start;
            uint64_t measuredIntervals = 0;
            for (auto count : presentationIntervals)
                measuredIntervals += count;
            auto percentile = [&](double fraction) {
                uint64_t count = 0;
                for (size_t i = 0; i < presentationIntervals.size(); ++i) {
                    count += presentationIntervals[i];
                    if (count >= measuredIntervals * fraction)
                        return i;
                }
                return size_t(0);
            };
            std::ofstream out(fs::u8path(args["--report"]));
            out << "{\n  \"version\":\"" << SCU_VERSION << "\",\n  \"game\":\"" << a.core.currentId
                << "\",\n  \"frames\":" << a.core.frame << ",\n  \"elapsed_seconds\":" << duration
                << ",\n  \"average_frames_per_second\":" << executedFrames / duration
                << ",\n  \"presented_frames\":" << presentedFrames
                << ",\n  \"average_presented_frames_per_second\":" << presentedFrames / duration
                << ",\n  \"presentation_intervals\":" << measuredIntervals
                << ",\n  \"presentation_interval_p95_ms\":" << percentile(.95)
                << ",\n  \"terrain_cache_bytes\":" << a.core.world.atlas.bytes()
                << ",\n  \"terrain_cache_hits\":" << a.core.world.terrainCacheHits
                << ",\n  \"terrain_cache_misses\":" << a.core.world.terrainCacheMisses
                << ",\n  \"renderer\":\"" << ri.name << "\",\n  \"drawable\":[" << a.drawableW
                << "," << a.drawableH << "],\n  \"canvas\":[" << a.core.world.canvas.width << ","
                << a.core.world.canvas.height << "],\n  \"camera\":[" << a.core.world.canvas.left
                << "," << a.core.world.canvas.top << "],\n  \"stage_size\":[" << v.width << ","
                << v.height << "],\n  \"player\":[" << v.playerX << "," << v.playerY
                << "],\n  \"world_ready\":" << (v.ready ? "true" : "false")
                << ",\n  \"game_mode\":" << (int)a.core.snapshot.ram[0xF600]
                << ",\n  \"stage\":" << v.stage
                << ",\n  \"voxel_mode\":" << a.settings.voxelMode
                << ",\n  \"voxel_rendered\":" << (a.voxelPresented() ? "true" : "false")
                << ",\n  \"voxel_faces\":" << a.core.world.volumeMesh.faces()
                << ",\n  \"voxel_grid\":[" << a.core.world.volume.width << ","
                << a.core.world.volume.height << "]"
                << ",\n  \"voxel_picture\":[" << voxelPicture[0] << "," << voxelPicture[1] << "]"
                << ",\n  \"voxel_angle\":[" << a.settings.voxelYaw << "," << a.settings.voxelPitch << "]"
                << ",\n  \"voxel_framing\":" << a.settings.voxelFraming
                << ",\n  \"language\":" << a.language << ",\n  \"view\":" << a.settings.view
                << ",\n  \"stage_coverage\":[" << a.core.world.canvas.coverageW << ","
                << a.core.world.canvas.coverageH << "]"
                << ",\n  \"menu_layout\":" << a.settings.menuLayout << ",\n  \"hint_bounds\":["
                << hintBounds.x << "," << hintBounds.y << "," << hintBounds.z << "," << hintBounds.w
                << "]"
                << ",\n  \"native_bounds\":[" << nativeBounds.x << "," << nativeBounds.y << ","
                << nativeBounds.z << "," << nativeBounds.w << "]"
                << ",\n  \"inset_bounds\":[" << insetBounds.x << "," << insetBounds.y << ","
                << insetBounds.z << "," << insetBounds.w << "]"
                << ",\n  \"player_inset\":" << (a.settings.playerInset ? "true" : "false")
                << ",\n  \"enemy_preview\":" << (a.settings.enemyPreview ? "true" : "false")
                << ",\n  \"enemy_placements\":" << a.core.world.enemies.placementCount
                << ",\n  \"boss_placements\":" << a.core.world.enemies.bossCount
                << ",\n  \"scripted_placements\":" << a.core.world.enemies.scriptCount
                << ",\n  \"enemy_previews\":" << a.core.world.enemies.visible.size()
                << ",\n  \"enemy_sprites_drawn\":" << a.core.world.enemySpriteCount
                << ",\n  \"enemy_markers_drawn\":" << a.core.world.enemyMarkerCount
                << ",\n  \"native_ram_crc32\":"
                << checksum(std::vector<uint8_t>(a.core.snapshot.ram,
                                                 a.core.snapshot.ram + sizeof a.core.snapshot.ram))
                << ",\n  \"native_vram_crc32\":"
                << checksum(std::vector<uint8_t>(
                       a.core.snapshot.vram, a.core.snapshot.vram + sizeof a.core.snapshot.vram))
                << ",\n  \"spindash_charge\":" << a.core.spinCharge
                << ",\n  \"rewind_enabled\":" << (a.settings.rewind ? "true" : "false")
                << ",\n  \"rewind_audio\":" << (a.settings.rewindAudio ? "true" : "false")
                << ",\n  \"rewind_button\":" << a.settings.rewindButton
                << ",\n  \"rewind_history_seconds\":" << a.core.rewindSeconds()
                << ",\n  \"adaptive_render_percent\":" << a.performanceGovernor.scale()
                << ",\n  \"adaptive_stereo_percent\":" << a.performanceGovernor.stereoScale()
                << ",\n  \"adaptive_effect_level\":" << a.performanceGovernor.effectLevel()
                << ",\n  \"reserved_frame_percent\":" << a.settings.performanceHeadroom
                << ",\n  \"measured_headroom_percent\":"
                << a.performanceGovernor.availableHeadroom()
                << ",\n  \"composition_cpu_ms\":" << a.performanceGovernor.cpuMs()
                << ",\n  \"stereo_gpu_ms\":" << a.performanceGovernor.gpuMs()
                << ",\n  \"restored_terrain_bytes\":" << a.core.world.restoredBytes
                << ",\n  \"palette_sample_cache_hits\":" << a.core.world.paletteCacheHits
                << ",\n  \"uploaded_world_bytes\":" << a.uploadedBytes
                << ",\n  \"rewind_history_bytes\":" << a.core.rewindBytes() << "\n}\n";
        }
        a.dirty = true;
        a.flushSettings();
#ifdef SCU_QUEST
        // Release XR images and GL objects while the owning SDL context is alive.
        headset.reset();
#endif
#ifdef SCU_MOBILE
        mobile.reset();
#endif
        holder.reset();
        SDL_Quit();
        return 0;
    } catch (const std::exception &e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SCU fatal: %s", e.what());
        fprintf(stderr, "Sonic Compilation Ultra: %s\n", e.what());
        SDL_Quit();
        return 1;
    }
}
