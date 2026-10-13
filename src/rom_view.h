#pragma once
#include "rom.h"
namespace scu {
struct RomView {
    const std::vector<uint8_t> &rom;
    uint32_t crc;
    unsigned address(unsigned a) const {
        a &= 0xFFFFFF;
        if (crc == 0x2AC1E7C6 && a < 0x100000)
            a += 0x200000;
        if (crc == 0x0658F691)
            a &= 0x1FFFFF;
        return a;
    }
    bool valid(unsigned a, unsigned length = 1) const {
        auto p = address(a);
        return p < rom.size() && length <= rom.size() - p;
    }
    uint8_t byte(unsigned a) const {
        return valid(a) ? rom[address(a)] : 0;
    }
    uint16_t word(unsigned a) const {
        return uint16_t((byte(a) << 8) | byte(a + 1));
    }
    uint32_t pointer(unsigned a) const {
        return (uint32_t(word(a)) << 16) | word(a + 2);
    }
    unsigned relative(unsigned a) const {
        return a + int16_t(word(a));
    }
};
} // namespace scu
