#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
// An original 68000 test program: writes a RAM marker, sets a red VDP backdrop,
// then increments a counter. No commercial cartridge bytes or artwork are used.
static std::vector<uint8_t> testProgram(bool sound = false) {
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
    if (sound) {
        // An authored PSG tone for sound-history tests, without game audio.
        for (auto value : {0x84, 0x08, 0x9B}) {
            word(0x13FC);
            word(uint16_t(value));
            address(0xC00011);
        }
    }
    word(0x5239);
    address(0xFF1235); // addq.b #1, absolute-long RAM counter
    word(0x60F8);      // branch back to the counter
    return rom;
}
