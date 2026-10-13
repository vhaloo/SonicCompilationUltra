#include "rom_code.h"
#include <array>
#include <set>
#include <algorithm>
extern "C" {
unsigned scu_disassemble_raw(char *, unsigned, const unsigned char *, const unsigned char *,
                             unsigned);
unsigned scu_decode_valid(unsigned, unsigned);
unsigned m68k_read_disassembler_16(unsigned) {
    return 0;
}
unsigned m68k_read_disassembler_32(unsigned) {
    return 0;
}
}
namespace scu {
std::vector<unsigned> codeInstructions(const RomView &r, unsigned start, unsigned end) {
    end = std::min(end, start + 65536);
    std::set<unsigned> visited;
    std::vector<unsigned> pending{start};
    auto queue = [&](unsigned pc) {
        if (pc >= start && pc < end && !(pc & 1) && r.valid(pc, 2) && !visited.count(pc))
            pending.push_back(pc);
    };
    while (!pending.empty() && visited.size() < 4096) {
        unsigned c = pending.back();
        pending.pop_back();
        if (c < start || c >= end || !r.valid(c, 2) || !visited.insert(c).second)
            continue;
        unsigned op = r.word(c);
        if (!scu_decode_valid(op, 1))
            continue;
        std::array<unsigned char, 32> bytes{};
        for (unsigned n = 0; n < bytes.size(); ++n)
            bytes[n] = r.byte(c + n);
        char text[256]{};
        unsigned length = scu_disassemble_raw(text, c, bytes.data(), bytes.data(), 1);
        if (!length || length > 16 || c + length > end || !r.valid(c, length))
            continue;
        if (op == 0x4E75 || op == 0x4E73 || op == 0x4E77 || op == 0x4AFC)
            continue;
        if ((op & 0xF000) == 0x6000) {
            unsigned to = c + 2 + (op & 255 ? int8_t(op) : int16_t(r.word(c + 2)));
            queue(to);
            if ((op & 0xFF00) != 0x6000)
                queue(c + length);
            continue;
        }
        if ((op & 0xF0F8) == 0x50C8)
            queue(r.relative(c + 2));
        if (op == 0x4EFB || op == 0x4EBB) {
            unsigned table = c + 2 + int8_t(r.word(c + 2));
            unsigned first = r.word(table);
            if (first == 0x6000)
                for (unsigned n = 0; n < 16 && r.word(table + n * 4) == 0x6000; ++n)
                    queue(table + n * 4);
            else if (first && !(first & 1) && first <= 512)
                for (unsigned n = 0; n < std::min(64u, first / 2) && r.valid(table + n * 2, 2);
                     ++n) {
                    int offset = int16_t(r.word(table + n * 2));
                    if (offset < int((n + 1) * 2) || (offset & 1))
                        break;
                    queue(table + offset);
                }
            if (op == 0x4EBB)
                queue(c + length);
            continue;
        }
        if (op == 0x4EF9 || op == 0x4EFA) {
            queue(op == 0x4EF9 ? r.pointer(c + 2) : r.relative(c + 2));
            continue;
        }
        if ((op & 0xFFC0) == 0x4EC0)
            continue;
        if (op == 0x4EB9 || op == 0x4EBA)
            queue(op == 0x4EB9 ? r.pointer(c + 2) : r.relative(c + 2));
        // S3 changes an object's own continuation pointer instead of using
        // a routine byte. Follow that local continuation, without executing it.
        if (op == 0x20BC || (op == 0x217C && r.word(c + 6) == 0))
            queue(r.pointer(c + 2));
        queue(c + length);
    }
    return {visited.begin(), visited.end()};
}
} // namespace scu
