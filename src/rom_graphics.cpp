#include "rom_graphics.h"
#include <array>
#include <algorithm>
namespace scu {
std::vector<uint8_t> decodeNemesis(const std::vector<uint8_t> &rom, unsigned address) {
    if (address > rom.size() || rom.size() - address < 2)
        return {};
    unsigned header = (rom[address] << 8) | rom[address + 1], size = (header & 0x7FFF) * 32;
    if (!size || size > 65536)
        return {};
    struct Code {
        unsigned length = 0, count = 0, color = 0;
    };
    std::array<Code, 512> codes{};
    size_t p = address + 2;
    unsigned color = 0, entries = 0;
    while (p < rom.size() && entries++ < 512) {
        unsigned descriptor = rom[p++];
        if (descriptor == 255)
            break;
        if (descriptor & 128) {
            color = descriptor & 15;
            continue;
        }
        unsigned length = descriptor & 15;
        if (!length || length > 8 || p == rom.size())
            return {};
        unsigned code = rom[p++];
        if (code >= (1u << length))
            return {};
        codes[(1u << length) + code] = {length, ((descriptor >> 4) & 7) + 1, color};
    }
    if (entries >= 512 || p == rom.size())
        return {};
    uint64_t bits = 0;
    unsigned available = 0;
    auto take = [&](unsigned n, unsigned &value) {
        while (available < n) {
            if (p == rom.size())
                return false;
            bits = (bits << 8) | rom[p++];
            available += 8;
        }
        available -= n;
        value = unsigned((bits >> available) & ((1u << n) - 1));
        return true;
    };
    std::vector<uint8_t> out(size, 0);
    unsigned nibble = 0;
    while (nibble < size * 2) {
        unsigned code = 0, count = 0, pixel = 0;
        bool found = false;
        for (unsigned length = 1; length <= 8; ++length) {
            unsigned bit;
            if (!take(1, bit))
                return {};
            code = (code << 1) | bit;
            if (length == 6 && code == 63) {
                if (!take(3, count) || !take(4, pixel))
                    return {};
                ++count;
                found = true;
                break;
            }
            auto entry = codes[(1u << length) + code];
            if (entry.length) {
                count = entry.count;
                pixel = entry.color;
                found = true;
                break;
            }
        }
        if (!found || count > size * 2 - nibble)
            return {};
        while (count--) {
            out[nibble / 2] |= uint8_t(pixel << ((nibble & 1) ? 0 : 4));
            ++nibble;
        }
    }
    if (header & 0x8000) {
        std::array<uint8_t, 4> previous{};
        for (size_t i = 0; i < out.size(); ++i) {
            out[i] ^= previous[i & 3];
            previous[i & 3] = out[i];
        }
    }
    return out;
}
std::vector<uint8_t> decodeKosinskiModules(const std::vector<uint8_t> &rom, unsigned address) {
    if (address > rom.size() || rom.size() - address < 2)
        return {};
    unsigned total = (rom[address] << 8) | rom[address + 1];
    if (!total || total > 65536)
        return {};
    size_t p = address + 2;
    std::vector<uint8_t> out;
    out.reserve(total);
    while (out.size() < total) {
        size_t module = p, base = out.size();
        unsigned descriptor = 0, left = 0;
        bool valid = true;
        auto refill = [&]() {
            if (p + 2 > rom.size()) {
                valid = false;
                return;
            }
            descriptor = rom[p] | (rom[p + 1] << 8);
            p += 2;
            left = 16;
        };
        auto bit = [&]() {
            if (!left)
                refill();
            unsigned value = descriptor & 1;
            descriptor >>= 1;
            // The native reader fetches the next descriptor before the token
            // data of the sixteenth bit, rather than before the next token.
            if (left && !--left)
                refill();
            return value;
        };
        auto byte = [&]() {
            if (p >= rom.size()) {
                valid = false;
                return 0u;
            }
            return unsigned(rom[p++]);
        };
        bool ended = false;
        for (unsigned tokens = 0; valid && tokens < 65536; ++tokens) {
            if (bit()) {
                unsigned value = byte();
                if (!valid || out.size() >= total || out.size() - base >= 4096)
                    return {};
                out.push_back(uint8_t(value));
                continue;
            }
            int offset;
            unsigned count;
            if (bit()) {
                unsigned lo = byte(), hi = byte();
                offset = int16_t(0xE000 | ((hi & 0xF8) << 5) | lo);
                count = hi & 7;
                if (!count) {
                    count = byte();
                    if (!count) {
                        ended = true;
                        break;
                    }
                    if (count == 1)
                        continue;
                    ++count;
                } else
                    count += 2;
            } else {
                unsigned high = bit(), low = bit();
                count = (high * 2 + low) + 2;
                offset = int8_t(byte());
            }
            if (!valid || offset >= 0 || size_t(-offset) > out.size() - base ||
                count > total - out.size() || count > 4096 - (out.size() - base))
                return {};
            while (count--)
                out.push_back(out[out.size() + offset]);
        }
        if (!valid || !ended || out.size() == base)
            return {};
        if (out.size() < total) {
            if (out.size() - base != 4096)
                return {};
            p = module + ((p - module + 15) & ~size_t(15));
        }
    }
    return out;
}
} // namespace scu
