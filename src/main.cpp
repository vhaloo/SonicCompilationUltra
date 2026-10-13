#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#endif
#include "ui.h"
#include <SDL.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <map>
#include <chrono>
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
    SET(camera) SET(hud) SET(hudScale) SET(background) SET(blur) SET(brightness) SET(parallax)
        SET(minimap) SET(locator) SET(spindash) SET(invincible) SET(infiniteAir) SET(rewind)
            SET(nativeScale) SET(ghost) SET(left) SET(top) SET(volume) SET(language) SET(help)
                SET(workers) SET(fullscreen) SET(span) SET(vsync) SET(uiScale) SET(scanlines)
                    SET(lockon) SET(fastBoot)
#undef SET
                        throw std::runtime_error("Unknown option: " + key);
}
static void execute(App &a, const Action &cmd, uint16_t &mask) {
    std::istringstream in(cmd.args);
    if (cmd.verb == "hold")
        mask = buttonMask(cmd.args);
    else if (cmd.verb == "menu")
        a.menu = cmd.args == "on";
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
    else if (cmd.verb == "game")
        a.play(cmd.args);
    else if (cmd.verb == "reset")
        a.core.reset();
    else if (cmd.verb == "capture") {
        a.capturePath = cmd.args;
        a.present(true);
        a.capturePath.clear();
    } else if (cmd.verb == "assert") {
        std::string addr, value;
        in >> addr >> value;
        auto p = std::stoul(addr, nullptr, 16) & 65535, v = std::stoul(value, nullptr, 16);
        if (a.core.snapshot.ram[p] != v)
            throw std::runtime_error("RAM assertion failed at " + addr);
    } else if (cmd.verb == "trace") {
        auto &r = a.core.snapshot.ram;
        fprintf(stderr,
                "TRACE step=%u frame=%llu mask=%04X mode=%02X pad=%02X/%02X s1routine=%d s2id=%d "
                "vs=%04X px=%u\n",
                cmd.step, (unsigned long long)a.core.frame, mask, r[0xF600], r[0xF604], r[0xF605],
                r[0xD024], r[0xB000], be16(r, 0xFFD8), be16(r, 0xD008));
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
                    "--realtime\nROMs and copyrighted game assets are not included.\n");
                return 0;
            }
            if (k == "--fullscreen" || k == "--mute" || k == "--software" || k == "--headless" ||
                k == "--realtime" || k == "--menu" || k == "--no-autoresume" ||
                k == "--no-fast-boot") {
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
        auto holder = std::make_unique<App>(dir);
        auto &a = *holder;
        a.headless = args.count("--headless");
        a.core.headless = a.headless;
        char *base = SDL_GetBasePath();
        fs::path baseDir = base ? fs::u8path(base) : fs::current_path();
        SDL_free(base);
        if (args.count("--software"))
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
#ifdef _WIN32
        else
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#endif
        a.initialise(baseDir, a.headless);
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
            SDL_SetWindowSize(a.window, std::stoi(v.substr(0, x)), std::stoi(v.substr(x + 1)));
        }
        for (auto &p : imports)
            a.import(fs::u8path(p));
        if (args.count("--scan"))
            a.import(fs::u8path(args["--scan"]), true);
        if (args.count("--game")) {
            if (!a.play(args["--game"]))
                throw std::runtime_error(a.notice);
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
        bool scripted = args.count("--script");
        double freq = (double)SDL_GetPerformanceFrequency(),
               previous = SDL_GetPerformanceCounter() / freq, start = previous, accumulator = 0,
               measureStart = previous;
        uint64_t measureFrames = 0, executedFrames = 0;
        while (a.running) {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                a.event(e);
            double now = SDL_GetPerformanceCounter() / freq,
                   elapsed = std::min(0.25, now - previous);
            previous = now;
            while (next < actions.size() && actions[next].step == step) {
                execute(a, actions[next], mask);
                ++next;
            }
            a.pollInput();
            if (scripted)
                a.core.buttons[0] = mask;
            bool paused = a.menu || (!a.focused && a.settings.pauseUnfocused && !a.headless);
            if (!paused && a.core.loaded) {
                if (a.settings.rewind &&
                    (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_BACKSPACE] ||
                     (a.pads[0] &&
                      SDL_GameControllerGetButton(a.pads[0], SDL_CONTROLLER_BUTTON_LEFTSHOULDER))))
                    a.core.rewind();
                else {
                    if (limit && !args.count("--realtime")) {
                        a.core.tick();
                        ++measureFrames;
                        ++executedFrames;
                    } else {
                        accumulator += elapsed;
                        int ticks = 0;
                        while (accumulator >= 1 / a.core.fps && ticks++ < 4) {
                            a.core.tick();
                            ++measureFrames;
                            ++executedFrames;
                            accumulator -= 1 / a.core.fps;
                        }
                        if (ticks >= 4)
                            accumulator = 0;
                    }
                    a.pan();
                }
            } else {
                accumulator = 0;
                a.core.clearAudio();
            }
            if (now - measureStart >= 1) {
                a.measuredFps = measureFrames / (now - measureStart);
                measureFrames = 0;
                measureStart = now;
            }
            if (step % renderEvery == 0 || a.menu)
                a.present();
            if (a.dirty && step % 60 == 0)
                a.flushSettings();
            ++step;
            if (limit && step >= limit)
                break;
            if (args.count("--seconds")&&now-start>=std::stod(args["--seconds"]))
                break;
            if (!limit || args.count("--realtime"))
                SDL_Delay(a.settings.vsync ? 1 : 2);
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
            double duration = SDL_GetPerformanceCounter() / freq - start;
            std::ofstream out(fs::u8path(args["--report"]));
            out << "{\n  \"version\":\"" << SCU_VERSION << "\",\n  \"game\":\"" << a.core.currentId
                << "\",\n  \"frames\":" << a.core.frame << ",\n  \"elapsed_seconds\":" << duration
                << ",\n  \"average_frames_per_second\":" << executedFrames / duration
                << ",\n  \"renderer\":\"" << ri.name << "\",\n  \"drawable\":[" << a.drawableW
                << "," << a.drawableH << "],\n  \"canvas\":[" << a.core.world.canvas.width << ","
                << a.core.world.canvas.height << "],\n  \"camera\":[" << a.core.world.canvas.left
                << "," << a.core.world.canvas.top << "],\n  \"stage_size\":[" << v.width << ","
                << v.height << "],\n  \"player\":[" << v.playerX << "," << v.playerY
                << "],\n  \"world_ready\":" << (v.ready ? "true" : "false")
                << ",\n  \"game_mode\":" << (int)a.core.snapshot.ram[0xF600]
                << ",\n  \"language\":" << a.language
                << ",\n  \"spindash_charge\":" << a.core.spinCharge << "\n}\n";
        }
        a.dirty = true;
        a.flushSettings();
        holder.reset();
        SDL_Quit();
        return 0;
    } catch (const std::exception &e) {
        fprintf(stderr, "Sonic Compilation Ultra: %s\n", e.what());
        SDL_Quit();
        return 1;
    }
}
