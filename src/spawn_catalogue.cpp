#include "spawn_catalogue.h"
#include "rom_code.h"
#include <algorithm>
#include <array>
#include <set>
#include <tuple>
#include <map>
namespace scu {
namespace {
bool has(std::initializer_list<unsigned> list, unsigned id) {
    return std::find(list.begin(), list.end(), id) != list.end();
}
unsigned target(const RomView &r, unsigned c) {
    if (r.word(c) == 0x4EB9 || r.word(c) == 0x4EF9)
        return r.pointer(c + 2);
    if (r.word(c) == 0x4EBA || r.word(c) == 0x6100 || r.word(c) == 0x4EFA || r.word(c) == 0x6000)
        return r.relative(c + 2);
    return 0;
}
unsigned addressLoad(const RomView &r, unsigned c, unsigned reg, unsigned &next) {
    if (r.word(c) == 0x41FA + reg * 0x200) {
        next = c + 4;
        return r.relative(c + 2);
    }
    if (r.word(c) == 0x41F9 + reg * 0x200) {
        next = c + 6;
        return r.pointer(c + 2);
    }
    return 0;
}
struct Position {
    bool known = false, relative = false;
    int value = 0;
};
void coordinates(const RomView &r, unsigned start, unsigned end, int family, Position &x,
                 Position &y, unsigned destinationReg = 0) {
    unsigned xo = family == 3 ? 0x10 : 8, yo = family == 3 ? 0x14 : 12;
    std::array<Position, 8> registers{};
    bool writtenX = false, writtenY = false;
    for (unsigned c : codeInstructions(r, start, end)) {
        unsigned op = r.word(c), arg = r.word(c + 2);
        unsigned dest = (op >> 9) & 7, mode = (op >> 6) & 7, src = (op >> 3) & 7, reg = op & 7;
        if ((op & 0xF000) == 0x3000 && mode == 0) {
            Position value;
            if (src == 0)
                value = registers[reg];
            if (src == 7 && reg == 4)
                value = {true, false, int16_t(arg)};
            if (src == 5 && reg == 0 && (arg == xo || arg == yo))
                value = {true, true, 0};
            registers[dest] = value;
        }
        if ((op & 0xF100) == 0x7000)
            registers[dest] = {true, false, int8_t(op)};
        for (unsigned n = 0; n < 8; ++n) {
            if (op == 0x0640 + n && registers[n].known)
                registers[n].value += int16_t(arg);
            if (op == 0x0440 + n && registers[n].known)
                registers[n].value -= int16_t(arg);
        }
        if (((op & 0xF000) == 0x3000 || (op & 0xF000) == 0x2000) && mode == 5 &&
            dest == destinationReg) {
            unsigned extra = src <= 4 ? 0 : src == 7 && reg == 1 ? 4 : 2;
            unsigned field = r.word(c + 2 + extra);
            Position *out = field == xo ? &x : field == yo ? &y : nullptr;
            bool *written = field == xo ? &writtenX : field == yo ? &writtenY : nullptr;
            if (out && !*written) {
                Position value;
                if (src == 0)
                    value = registers[reg];
                if (src == 7 && reg == 4 && (op & 0xF000) == 0x3000)
                    value = {true, false, int16_t(arg)};
                if (src == 5 && reg == 0 && arg == field)
                    value = {true, true, 0};
                *out = value;
                *written = true;
            }
        }
        unsigned field = r.word(c + 4);
        Position *out = field == xo ? &x : field == yo ? &y : nullptr;
        if (out && out->known && (op == 0x0668 + destinationReg || op == 0x0468 + destinationReg))
            out->value += (op == 0x0668 + destinationReg ? 1 : -1) * int16_t(arg);
    }
}
bool generatedThreat(int family, unsigned id) {
    if (family == 1)
        return has({0x14, 0x1E, 0x1F, 0x20, 0x22, 0x23, 0x2B, 0x2C, 0x2D, 0x3D, 0x40,
                    0x42, 0x43, 0x48, 0x4D, 0x50, 0x55, 0x5F, 0x60, 0x73, 0x74, 0x75,
                    0x76, 0x77, 0x78, 0x7A, 0x7B, 0x82, 0x84, 0x85, 0x86},
                   id);
    return has({0x20, 0x4A, 0x4B, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x5C, 0x5D, 0x89,
                0x8C, 0x8D, 0x8E, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
                0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xAC,
                0xAD, 0xAF, 0xB7, 0xB8, 0xB9, 0xBC, 0xBE, 0xC5, 0xC6, 0xC7, 0xC8},
               id);
}
void followAncestor(const SpawnPlacement &root, SpawnPlacement &p) {
    p.sourceId = root.sourceId;
    p.sourceFunction = root.sourceFunction;
    p.sourceState = root.sourceState;
    p.variable |= root.variable;
    if (!root.parentFunction || !p.relative)
        return;
    // Forecast later generations relative to the placed source, even before
    // the intermediate script objects exist in the cartridge's object pool.
    if (p.relativeX) {
        p.dx += root.dx;
        p.relativeX = root.relativeX;
    }
    if (p.relativeY) {
        p.dy += root.dy;
        p.relativeY = root.relativeY;
    }
    p.relative = p.relativeX || p.relativeY;
    p.parentId = root.parentId;
    p.parentFunction = root.parentFunction;
    p.parentState = root.parentState;
}
} // namespace
bool isBoss(int family, bool originalS3, bool skSet, unsigned id) {
    if (family == 1)
        return has({0x3D, 0x73, 0x75, 0x77, 0x7A, 0x82, 0x85}, id);
    if (family == 2)
        return has({0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 0x5D, 0x89, 0xAF, 0xC5, 0xC7}, id);
    if (originalS3)
        return has({128, 133, 134, 140, 141, 142, 143, 169, 172, 173, 174, 175,
                    176, 177, 178, 179, 180, 181, 182, 183, 196, 204, 205, 208},
                   id);
    if (skSet)
        return has({145, 146, 147, 151, 152, 156, 157, 161, 162, 163, 166, 167, 178, 182}, id);
    return has({144, 145, 146, 153, 154, 158, 159, 160, 161, 162, 166, 167, 170,
                171, 172, 188, 189, 195, 196, 197, 198, 201, 202, 203, 204, 205},
               id);
}
unsigned objectFunction(const RomView &r, unsigned index, int family, unsigned id) {
    return r.pointer(index + (family == 3 ? id : id ? id - 1 : 0) * 4);
}
unsigned objectEnd(const RomView &r, unsigned index, int family, unsigned fn) {
    unsigned end = fn + 65536;
    for (unsigned i = 0; i < (family == 1 ? 140u : 256u); ++i) {
        unsigned next = r.pointer(index + i * 4);
        if (next > fn && next < end)
            end = next;
    }
    return end;
}
void addEventBosses(const RomView &r, int family, int stage, unsigned index,
                    std::vector<SpawnPlacement> &points) {
    struct Event {
        int stage;
        unsigned id, start, end;
    };
    static const Event s1[] = {{0x0002, 0x3D, 0x6EA4, 0x6EF2}, {0x0102, 0x77, 0x6F0C, 0x6F66},
                               {0x0202, 0x73, 0x709C, 0x70F2}, {0x0302, 0x7A, 0x7132, 0x7168},
                               {0x0402, 0x75, 0x71D0, 0x720A}, {0x0501, 0x82, 0x729A, 0x72B8},
                               {0x0502, 0x85, 0x72F6, 0x7318}};
    static const Event s2[] = {{0x0001, 0x56, 0xE6EE, 0xE738}, {0x0500, 0x54, 0xE7F6, 0xE82E},
                               {0x0701, 0x52, 0xEFE8, 0xF022}, {0x0A01, 0x55, 0xF0EC, 0xF124},
                               {0x0B01, 0x57, 0xF206, 0xF23E}, {0x0C01, 0x51, 0xF318, 0xF350},
                               {0x0D01, 0x5D, 0xF3FA, 0xF432}, {0x0F01, 0x89, 0xF520, 0xF55C},
                               {0x0E00, 0xAF, 0xF45E, 0xF490}};
    if (family > 2) {
        // SOZ1 creates its miniboss through a delayed control object. Its
        // position varies with the camera: preview the authored trigger.
        if (r.crc != 0x9BC192CE && stage == 0x0800 && r.word(0x55B78) == 0x303C &&
            r.word(0x55B82) == 0x0C78 && r.word(0x55B86) == 0xB014) {
            unsigned fn = objectFunction(r, index, family, 151);
            bool creates = false;
            for (unsigned c = 0x55F7C; c < 0x55F90; c += 2)
                creates |= r.word(c) == 0x22BC && r.pointer(c + 2) == fn;
            if (creates) {
                SpawnPlacement p;
                p.x = r.word(0x55B7A);
                p.y = r.word(0x55B84);
                p.id = 151;
                p.function = fn;
                p.kind = SpawnKind::Boss;
                p.event = p.variable = true;
                p.bossOrigin = p.id;
                p.bossOriginFunction = fn;
                points.push_back(p);
            }
        }
        return;
    }
    auto list = family == 1 ? s1 : s2;
    size_t count = family == 1 ? std::size(s1) : std::size(s2);
    for (size_t i = 0; i < count; ++i) {
        auto event = list[i];
        if (event.stage != stage || !r.valid(event.start, event.end - event.start))
            continue;
        bool creates = false;
        for (unsigned c = event.start; c + 4 <= event.end; c += 2)
            if ((r.word(c) == 0x12BC || (r.word(c) == 0x137C && r.word(c + 4) == 0)) &&
                r.word(c + 2) == event.id)
                creates = true;
        if (!creates)
            continue;
        SpawnPlacement p;
        p.id = event.id;
        p.function = objectFunction(r, index, family, p.id);
        p.kind = SpawnKind::Boss;
        p.event = true;
        p.bossOrigin = p.id;
        p.bossOriginFunction = p.function;
        for (unsigned c = event.start; c + 6 <= event.end; c += 2)
            if (r.word(c) == 0x137C && r.word(c + 4) == 0x28)
                p.subtype = r.byte(c + 3);
        Position x, y;
        coordinates(r, event.start, event.end, family, x, y, 1);
        Position initX, initY;
        coordinates(r, p.function,
                    std::min(objectEnd(r, index, family, p.function), p.function + 2048), family,
                    initX, initY);
        if (!x.known || x.relative)
            x = initX;
        if (!y.known || y.relative)
            y = initY;
        // Oil Ocean writes its initial horizontal position into boss RAM.
        if (family == 2 && p.id == 0x55)
            for (unsigned c = p.function; c < p.function + 512 && r.valid(c, 6); c += 2)
                if (r.word(c) == 0x31FC && r.word(c + 4) == 0xF750)
                    x = {true, false, int(r.word(c + 2))};
        // Final Zone initializes its arena machinery from a ten-byte descriptor
        // table. The second record is the fixed cylinder panel, not the dummy
        // off-stage Eggman record at the beginning of that table.
        if (family == 1 && p.id == 0x85)
            for (unsigned c : codeInstructions(r, p.function, p.function + 512)) {
                unsigned after = 0, table = addressLoad(r, c, 2, after);
                if (table && r.valid(table, 60) && r.word(table) == 0x100 &&
                    r.word(table + 2) == 0x100) {
                    p.attributes = table + 10;
                    x = {true, false, int(r.word(table + 10))};
                    y = {true, false, int(r.word(table + 12))};
                    break;
                }
            }
        if (!x.known || !y.known || x.relative || y.relative)
            continue;
        p.x = x.value;
        p.y = y.value;
        if (p.x >= 0 && p.x < 32768 && p.y >= 0 && p.y < 4096)
            points.push_back(p);
    }
}
void addScriptSpawns(const RomView &r, int family, bool originalS3, unsigned index,
                     const std::vector<SpawnPlacement> &roots,
                     std::vector<SpawnPlacement> &points) {
    struct Factory {
        unsigned function;
        int format;
    };
    std::array<Factory, 10> factories{};
    if (family == 3) {
        unsigned base = originalS3 ? 0x529D0 : 0x84060;
        unsigned offsets[] = {0, 0x50, 0xB4, 0x106, 0x146, 0x1C0, 0x1F8, 0x248, 0x288, 0x2C8};
        for (unsigned n = 0; n < 10; ++n)
            factories[n] = {base + offsets[n], int(n + 1)};
        if (originalS3) {
            factories[6].function = 0x52BBC;
            factories[7].function = 0x52C0C;
            factories[8].function = factories[9].function = 0;
        }
    }
    std::set<std::tuple<int, int, unsigned, unsigned, unsigned>> seen;
    std::map<std::pair<unsigned, unsigned>, std::vector<unsigned>> codeCache;
    for (auto &root : roots) {
        unsigned fn = root.function ? root.function : objectFunction(r, index, family, root.id),
                 end = root.codeEnd ? root.codeEnd : objectEnd(r, index, family, fn);
        if (!fn || !r.valid(fn, 2))
            continue;
        auto key = std::make_pair(fn, end);
        if (!codeCache.count(key))
            codeCache.emplace(key, codeInstructions(r, fn, end));
        for (unsigned c : codeCache.at(key)) {
            if (!r.valid(c, 12))
                continue;
            if (family == 2) {
                unsigned after = 0, data = addressLoad(r, c, 2, after);
                unsigned callee = target(r, after);
                if (data && r.valid(data, 4) && callee == 0x367D0) {
                    unsigned id = r.byte(data + 2);
                    if (!generatedThreat(family, id))
                        continue;
                    SpawnPlacement p;
                    p.x = root.x;
                    p.y = root.y;
                    p.relative = p.relativeX = p.relativeY = true;
                    p.id = id;
                    p.subtype = r.byte(data + 3);
                    p.function = objectFunction(r, index, family, id);
                    p.kind = SpawnKind::Scripted;
                    p.parentId = root.id;
                    p.parentFunction = fn;
                    p.parentState = root.state;
                    p.event = root.event;
                    p.bossOrigin = root.bossOrigin;
                    p.bossOriginFunction = root.bossOriginFunction;
                    Position x{true, true, 0}, y{true, true, 0};
                    coordinates(r, after + (r.word(after) == 0x4EB9 ? 6 : 4),
                                std::min(end, after + 48), family, x, y, 1);
                    p.variable = !x.known || !y.known;
                    if (x.known) {
                        p.x = x.relative ? root.x + x.value : x.value;
                        p.dx = x.relative ? x.value : 0;
                        p.relativeX = x.relative;
                    }
                    if (y.known) {
                        p.y = y.relative ? root.y + y.value : y.value;
                        p.dy = y.relative ? y.value : 0;
                        p.relativeY = y.relative;
                    }
                    followAncestor(root, p);
                    if (seen.emplace(p.x, p.y, p.function, p.subtype, fn).second)
                        points.push_back(p);
                    continue;
                }
                if (data && callee == 0x3681A) {
                    unsigned count = 0, sub = 0;
                    for (unsigned pc = c >= 32 ? c - 32 : 0; pc < c; pc += 2) {
                        if ((r.word(pc) & 0xFF00) == 0x7C00)
                            count = (r.word(pc) & 255) + 1;
                        if ((r.word(pc) & 0xFF00) == 0x7400)
                            sub = r.word(pc) & 255;
                    }
                    if (!count || count > 64 || !r.valid(data, count * 6))
                        continue;
                    for (unsigned n = 0; n < count; ++n) {
                        SpawnPlacement p;
                        p.dx = int8_t(r.byte(data + n * 6));
                        p.dy = int8_t(r.byte(data + n * 6 + 1));
                        p.x = root.x + p.dx;
                        p.y = root.y + p.dy;
                        p.relative = p.relativeX = p.relativeY = true;
                        p.id = 0x98;
                        p.subtype = sub;
                        p.frame = r.byte(data + n * 6 + 4);
                        p.function = objectFunction(r, index, family, p.id);
                        p.kind = SpawnKind::Scripted;
                        p.parentId = root.id;
                        p.parentFunction = fn;
                        p.parentState = root.state;
                        p.event = root.event;
                        p.bossOrigin = root.bossOrigin;
                        p.bossOriginFunction = root.bossOriginFunction;
                        followAncestor(root, p);
                        if (seen.emplace(p.x, p.y, p.function, p.frame, fn).second)
                            points.push_back(p);
                    }
                    continue;
                }
            }
            if (family < 3 &&
                (r.word(c) == 0x12BC || (r.word(c) == 0x137C && r.word(c + 4) == 0))) {
                unsigned id = r.word(c + 2);
                if (!id || id > 255 || id == root.id || !generatedThreat(family, id))
                    continue;
                Position x, y;
                unsigned limit = std::min(end, c + 160);
                for (unsigned n = c + 4; n < limit; n += 2)
                    if (r.word(n) == 0x12BC || r.word(n) == 0x4E75) {
                        limit = n;
                        break;
                    }
                coordinates(r, c + (r.word(c) == 0x137C ? 6 : 4), limit, family, x, y, 1);
                SpawnPlacement p;
                p.variable = !x.known || !y.known;
                if (!x.known)
                    x = {true, true, 0};
                if (!y.known)
                    y = {true, true, 0};
                p.id = id;
                p.function = objectFunction(r, index, family, id);
                p.kind = isBoss(family, false, false, id) ? SpawnKind::Boss : SpawnKind::Scripted;
                p.relative = x.relative || y.relative;
                p.relativeX = x.relative;
                p.relativeY = y.relative;
                p.dx = x.relative ? x.value : 0;
                p.dy = y.relative ? y.value : 0;
                p.x = x.relative ? root.x + x.value : x.value;
                p.y = y.relative ? root.y + y.value : y.value;
                p.parentId = root.id;
                p.parentFunction = fn;
                p.parentState = root.state;
                p.event = root.event;
                p.bossOrigin = root.bossOrigin;
                p.bossOriginFunction = root.bossOriginFunction;
                followAncestor(root, p);
                if (seen.emplace(p.x, p.y, p.function, 0, fn).second)
                    points.push_back(p);
                continue;
            }
            if (family != 3)
                continue;
            if (r.word(c) == 0x22BC && r.valid(r.pointer(c + 2), 2)) {
                unsigned child = r.pointer(c + 2);
                if (child == fn)
                    continue;
                unsigned limit = std::min(end, c + 192);
                for (unsigned pc = c + 6; pc < limit; pc += 2)
                    if (r.word(pc) == 0x22BC || r.word(pc) == 0x4E75) {
                        limit = pc;
                        break;
                    }
                Position x, y;
                coordinates(r, c + 6, limit, family, x, y, 1);
                SpawnPlacement p;
                p.variable = !x.known || !y.known;
                if (!x.known)
                    x = {true, true, 0};
                if (!y.known)
                    y = {true, true, 0};
                p.dx = x.relative ? x.value : 0;
                p.dy = y.relative ? y.value : 0;
                p.x = x.relative ? root.x + x.value : x.value;
                p.y = y.relative ? root.y + y.value : y.value;
                p.relativeX = x.relative;
                p.relativeY = y.relative;
                p.relative = x.relative || y.relative;
                p.function = child;
                p.kind = SpawnKind::Scripted;
                for (unsigned id = 0; id < 256; ++id)
                    if (objectFunction(r, index, family, id) == child &&
                        isBoss(family, originalS3, !originalS3 && index == 0x952A2, id)) {
                        p.id = id;
                        p.kind = SpawnKind::Boss;
                        break;
                    }
                p.parentId = root.id;
                p.parentFunction = fn;
                p.parentState = root.state;
                p.event = root.event;
                p.bossOrigin = root.bossOrigin             ? root.bossOrigin
                               : p.kind == SpawnKind::Boss ? p.id
                                                           : 0;
                p.bossOriginFunction = root.bossOriginFunction     ? root.bossOriginFunction
                                       : p.kind == SpawnKind::Boss ? child
                                                                   : 0;
                followAncestor(root, p);
                if (seen.emplace(p.x, p.y, p.function, 0, fn).second)
                    points.push_back(p);
                continue;
            }
            unsigned next = 0, data = addressLoad(r, c, 2, next);
            if (!data || !r.valid(data, 6))
                continue;
            unsigned callee = 0;
            for (unsigned pc = next; pc < next + 16 && pc + 6 <= end; pc += 2) {
                callee = target(r, pc);
                if (callee)
                    break;
            }
            int format = 0;
            for (auto f : factories)
                if (f.function && callee == f.function)
                    format = f.format;
            if (!format)
                continue;
            unsigned count = r.word(data) + 1;
            if (!count || count > 64)
                continue;
            bool repeated = has({3, 4, 6, 8}, unsigned(format)),
                 complex = format == 2 || format == 5;
            unsigned length = complex ? 22 : has({4, 6, 8, 9}, unsigned(format)) ? 4 : 6;
            if (!r.valid(data + 2, repeated ? length : count * length))
                continue;
            for (unsigned n = 0; n < (repeated ? 1u : count); ++n) {
                unsigned entry = data + 2 + n * length, child = r.pointer(entry);
                if (!r.valid(child, 2) || child == fn)
                    continue;
                int dx = 0, dy = 0;
                if (complex) {
                    dx = int8_t(r.byte(entry + 16));
                    dy = int8_t(r.byte(entry + 17));
                } else if (length == 6) {
                    dx = int8_t(r.byte(entry + 4));
                    dy = int8_t(r.byte(entry + 5));
                }
                if ((format == 5 || format == 10) && (root.flags & 1))
                    dx = -dx;
                SpawnPlacement p;
                p.x = root.x + dx;
                p.y = root.y + dy;
                p.dx = dx;
                p.dy = dy;
                p.relative = p.relativeX = p.relativeY = true;
                p.function = child;
                p.attributes = complex ? r.pointer(entry + 4) : 0;
                p.kind = SpawnKind::Scripted;
                for (unsigned id = 0; id < 256; ++id)
                    if (objectFunction(r, index, family, id) == child &&
                        isBoss(family, originalS3, !originalS3 && index == 0x952A2, id)) {
                        p.id = id;
                        p.kind = SpawnKind::Boss;
                        break;
                    }
                p.parentId = root.id;
                p.parentFunction = fn;
                p.parentState = root.state;
                p.event = root.event;
                p.bossOrigin = root.bossOrigin             ? root.bossOrigin
                               : p.kind == SpawnKind::Boss ? p.id
                                                           : 0;
                p.bossOriginFunction = root.bossOriginFunction     ? root.bossOriginFunction
                                       : p.kind == SpawnKind::Boss ? child
                                                                   : 0;
                followAncestor(root, p);
                if (seen.emplace(p.x, p.y, p.function, p.attributes, fn).second)
                    points.push_back(p);
            }
        }
    }
}
void addScreenEventSpawns(const RomView &r, bool originalS3, int stage, unsigned index,
                          std::vector<SpawnPlacement> &points) {
    unsigned table = originalS3 ? 0x39328 : 0x4E468;
    unsigned zones = originalS3 ? 7 : 13, zone = unsigned(stage) >> 8;
    if (zone >= zones || !r.valid(table - 16, zones * 32 + 16))
        return;
    std::vector<SpawnPlacement> events;
    for (unsigned layer = 0; layer < 3; ++layer) {
        unsigned resizeTable = originalS3 ? 0x1A8A8 : 0x1C3F6;
        unsigned fn = layer < 2
                          ? r.pointer(table + zone * 32 + (stage & 1) * 8 + layer * 4)
                          : resizeTable + int16_t(r.word(resizeTable + zone * 4 + (stage & 1) * 2));
        if (!fn || !r.valid(fn, 2))
            continue;
        unsigned end = fn + 65536;
        unsigned bounds = layer < 2 ? table - 16 : resizeTable;
        for (unsigned p = bounds; p < bounds + (layer < 2 ? zones * 32 : 64);
             p += layer < 2 ? 4 : 2) {
            unsigned next = layer < 2 ? r.pointer(p) : resizeTable + int16_t(r.word(p));
            if (next > fn && next < end)
                end = next;
        }
        SpawnPlacement root;
        root.function = fn;
        root.codeEnd = end;
        root.event = true;
        addScriptSpawns(r, 3, originalS3, index, {root}, events);
    }
    for (auto p : events) {
        // A global event has no placed parent. Its program or initialization
        // must establish both axes before we can display a fixed spawn point.
        Position x, y;
        coordinates(r, p.function, std::min(objectEnd(r, index, 3, p.function), p.function + 512),
                    3, x, y);
        bool knownX = !p.relativeX && !p.variable;
        bool knownY = !p.relativeY && !p.variable;
        if (!knownX && x.known && !x.relative) {
            p.x = x.value;
            knownX = true;
        }
        if (!knownY && y.known && !y.relative) {
            p.y = y.value;
            knownY = true;
        }
        if (!knownX || !knownY || p.x < 0 || p.x >= 32768 || p.y < 0 || p.y >= 4096)
            continue;
        p.parentFunction = p.parentId = p.parentState = 0;
        p.relative = p.relativeX = p.relativeY = p.variable = false;
        points.push_back(p);
    }
}
} // namespace scu
