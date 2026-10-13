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

// An original 68000 test program: writes a RAM marker, sets a red VDP backdrop,
// then increments a counter. No commercial cartridge bytes or artwork are used.
static std::vector<uint8_t> testProgram() {
    std::vector<uint8_t> rom(256 * 1024);
    auto longword = [&](size_t offset, uint32_t n) {
        for (int i = 0; i < 4; ++i)
            rom[offset + i] = uint8_t(n >> (24 - i * 8));
    };
    longword(0, 0x00FF8000);
    longword(4, 0x200);
    std::memcpy(rom.data() + 0x100, "SEGA MEGA DRIVE ", 16);
    std::memcpy(rom.data() + 0x120, "SCU ORIGINAL RUNTIME TEST", 25);
    rom[0x1F0] = 'U';
    longword(0x1A4, uint32_t(rom.size() - 1));
    longword(0x1A8, 0xFF0000);
    longword(0x1AC, 0xFFFFFF);
    size_t pc = 0x200;
    auto word = [&](uint16_t n) {
        rom[pc++] = uint8_t(n >> 8);
        rom[pc++] = uint8_t(n);
    };
    auto address = [&](uint32_t n) {
        word(uint16_t(n >> 16));
        word(uint16_t(n));
    };
    auto moveWord = [&](uint16_t n, uint32_t destination) {
        word(0x33FC);
        word(n);
        address(destination);
    };
    word(0x46FC);
    word(0x2700); // Disable interrupts in this minimal program.
    word(0x13FC);
    word(0x0042);
    address(0xFF1234);
    moveWord(0x8004, 0xC00004);
    moveWord(0x8144, 0xC00004);
    moveWord(0x8705, 0xC00004);
    word(0x23FC);
    address(0xC00A0000);
    address(0xC00004);
    moveWord(0x000E, 0xC00000);
    word(0x5239);
    address(0xFF1235); // addq.b #1, absolute-long RAM counter
    word(0x60F8);      // branch back to the counter
    return rom;
}

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
