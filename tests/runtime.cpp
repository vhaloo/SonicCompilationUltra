#include "core.h"
#include <SDL.h>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cstring>

using namespace scu;
static void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

#include "fixture.h"

int main() {
    try {
        SDL_SetMainReady();
        require(SDL_Init(SDL_INIT_TIMER) == 0, "SDL timer initialization");
        const auto tempRoot = fs::canonical(fs::temp_directory_path());
        const auto dir =
            tempRoot / ("SCU-runtime-test-" + std::to_string(SDL_GetPerformanceCounter()));
        require(fs::create_directory(dir), "Unique private test directory");
        const auto path = dir / "original-test-program.bin";
        writeBytesAtomic(path, testProgram());
        Library library(dir);
        require(library.importFile(path) == 1, "Synthetic cartridge import");
        Settings settings;
        settings.autosave = false;
        settings.fastBoot = false;
        settings.rewind = true;
        auto core = std::make_unique<Core>(settings, library);
        core->headless = true;
        require(core->load(library.entries.front().id), "Native core load");
        for (int n = 0; n < 60; ++n)
            core->tick();
        require(core->snapshot.ram[0x1234] == 0x42, "68000 program did not execute");
        require(!core->native.empty(), "No native video frame");
        bool red = false;
        for (auto pixel : core->native)
            red |= (pixel & 0xFF0000) >= 0xB00000 && (pixel & 0xFFFF) == 0;
        require(red, "VDP backdrop did not reach host framebuffer");
        auto state = core->state();
        const auto savedVideo = core->native;
        const auto savedBackdrop = core->snapshot.background[0][0];
        const auto counter = core->snapshot.ram[0x1235];
        const auto savedFrame = core->frame;
        for (int n = 0; n < 15; ++n)
            core->tick();
        core->native.assign(core->native.size(), 0);
        core->snapshot.background[0][0] ^= 0xFFFFFF;
        require(core->restore(state), "State restore");
        require(core->frame == savedFrame && core->snapshot.ram[0x1235] == counter,
                "State failed to restore native RAM and host time");
        require(core->native == savedVideo && core->snapshot.background[0][0] == savedBackdrop,
                "State failed to restore the paused presentation raster");
        auto damaged = state;
        damaged.back() ^= 1;
        require(!core->restore(damaged), "Corrupt state accepted");
        auto other = state;
        other[4] ^= 1;
        require(!core->restore(other), "Wrong cartridge state accepted");
        core->save(1);
        core->tick();
        core->loadState(1);
        require(core->frame == savedFrame, "File-backed state round trip");
        for (int n = 0; n < 16; ++n)
            core->tick();
        const auto later = core->frame;
        require(core->rewind() && core->frame < later, "Rewind did not restore an earlier frame");
        core.reset();
        // This path was created exclusively above; it is never supplied by a user.
        require(dir.parent_path() == tempRoot &&
                    dir.filename().string().rfind("SCU-runtime-test-", 0) == 0,
                "Test cleanup boundary");
        fs::remove_all(dir);
        SDL_Quit();
        std::cout << "PASS: original synthetic 68000 program, native VDP, state integrity, file "
                     "saves and rewind\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Runtime test: " << e.what() << '\n';
        SDL_Quit();
        return 1;
    }
}
