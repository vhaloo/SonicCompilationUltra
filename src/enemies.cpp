#include "enemies.h"
#include "rom_graphics.h"
#include "rom_code.h"
#include <set>
#include <tuple>
#include <algorithm>
#include <array>
#include <cmath>

namespace scu {
namespace {
using Reader = RomView;
uint16_t ramWord(const SCUSnapshot &s, unsigned p) {
    return uint16_t((s.ram[p & 65535] << 8) | s.ram[(p + 1) & 65535]);
}
uint32_t ramPointer(const SCUSnapshot &s, unsigned p) {
    return (uint32_t(ramWord(s, p)) << 16) | ramWord(s, p + 2);
}
bool contains(std::initializer_list<unsigned> ids, unsigned id) {
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}
bool enemy(int family, bool originalS3, bool skSet, unsigned id) {
    if (family == 1)
        return contains(
            {0x1E, 0x1F, 0x22, 0x2B, 0x2C, 0x2D, 0x40, 0x42, 0x43, 0x50, 0x55, 0x5F, 0x60, 0x78},
            id);
    if (family == 2)
        return contains({0x4A, 0x4B, 0x50, 0x5C, 0x8C, 0x8D, 0x8E, 0x91, 0x92,
                         0x94, 0x95, 0x96, 0x99, 0x9A, 0x9D, 0x9E, 0x9F, 0xA1,
                         0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xAC, 0xAD, 0xC8},
                        id);
    if (originalS3)
        return contains({129, 130, 131, 135, 136, 137, 138, 139, 153, 154, 155, 156, 157,
                         158, 159, 160, 161, 162, 163, 165, 166, 167, 168, 170, 171},
                        id);
    if (skSet)
        return contains({140, 141, 142, 143, 144, 148, 149, 150, 153, 154, 155, 160, 164, 165}, id);
    return contains({140, 141, 142, 143, 147, 148, 149, 150, 151, 152, 155, 156, 157,
                     163, 164, 165, 168, 169, 173, 174, 190, 191, 192, 193, 194},
                    id);
}
unsigned callTarget(const Reader &r, unsigned p) {
    if (r.word(p) == 0x4EBA || r.word(p) == 0x6100)
        return r.relative(p + 2);
    if (r.word(p) == 0x4EB9)
        return r.pointer(p + 2);
    return 0;
}
unsigned lea(const Reader &r, unsigned p, unsigned reg, unsigned &next) {
    unsigned op = 0x41FA + reg * 0x200;
    if (r.word(p) == op) {
        next = p + 4;
        return r.relative(p + 2);
    }
    if (r.word(p) == op - 1) {
        next = p + 6;
        return r.pointer(p + 2);
    }
    return 0;
}
struct Piece {
    int x, y, width, height;
    unsigned attr;
};
std::vector<Piece> pieces(const Reader &r, int family, unsigned map, unsigned frame) {
    std::vector<Piece> result;
    if (!map || !r.valid(map + frame * 2, 2))
        return result;
    unsigned p = map + int16_t(r.word(map + frame * 2));
    unsigned count = family == 1 ? r.byte(p++) : r.word(p);
    if (family != 1)
        p += 2;
    unsigned length = family == 1 ? 5 : family == 2 ? 8 : 6;
    if (!count || count > 64 || !r.valid(p, count * length))
        return result;
    for (unsigned n = 0; n < count; ++n, p += length) {
        int x = family == 1 ? int8_t(r.byte(p + 4)) : int16_t(r.word(p + (family == 2 ? 6 : 4))),
            y = int8_t(r.byte(p)), size = r.byte(p + 1) & 15;
        if (std::abs(x) > 128 || std::abs(y) > 128)
            return {};
        result.push_back({x, y, (((size >> 2) & 3) + 1) * 8, ((size & 3) + 1) * 8, r.word(p + 2)});
    }
    return result;
}
} // namespace

void EnemyPreviews::reset() {
    placements.clear();
    appearances.clear();
    decodedArt.clear();
    functionBounds.clear();
    visible.clear();
    cachedStage = -1;
    cachedCRC = 0;
    placementCount = 0;
    bossCount = scriptCount = 0;
    supported = false;
}
void EnemyPreviews::update(const SCUSnapshot &s, const Game *g, const std::vector<uint8_t> &rom,
                           int stage, int cameraX, int cameraY, bool bosses, bool scripts) {
    visible.clear();
    supported =
        g &&
        contains({0xF9394E97, 0x7B905383, 0x2AC1E7C6, 0x9BC192CE, 0x0658F691, 0x63522553}, g->crc);
    if (!supported)
        return;
    Reader r{rom, g->crc};
    int family = g->family;
    bool originalS3 = g->crc == 0x9BC192CE;
    unsigned zone = (stage >> 8) & 255;
    bool skSet = family == 3 && !originalS3 && zone >= 7 && zone < 14;
    unsigned index = family == 1   ? 0xD382
                     : family == 2 ? 0x1600C
                     : originalS3  ? 0x5CC96
                     : skSet       ? 0x952A2
                                   : 0x94EA2;
    if (cachedCRC != g->crc || cachedStage != stage) {
        reset();
        supported = true;
        cachedCRC = g->crc;
        cachedStage = stage;
        unsigned base;
        if (family == 1) {
            unsigned off = ((stage & 0xFF00) >> 4) | ((stage & 3) << 2);
            base = 0x6B000 + int16_t(r.word(0x6B000 + off));
        } else if (family == 2) {
            unsigned off = zone * 4 + (stage & 1) * 2;
            base = 0xE6800 + int16_t(r.word(0xE6800 + off));
        } else {
            unsigned table = originalS3 ? 0x5E0D8 : 0x1E3D98;
            base = r.pointer(table + zone * 8 + (stage & 1) * 4);
        }
        unsigned state = family == 3 ? 0 : 1;
        std::vector<SpawnPlacement> roots;
        for (unsigned n = 0, p = base; n < 768 && r.valid(p, 6); ++n, p += 6) {
            unsigned x = r.word(p), y = r.word(p + 2), id = r.byte(p + 4);
            if (x == 0xFFFF)
                break;
            if (x >= 32768)
                break; // Reject malformed/foreign lists rather than inventing spawn positions.
            unsigned st =
                family == 3 ? state++ : ((family == 1 ? id & 128 : y & 0x8000) ? state++ : 0);
            if (family == 1)
                id &= 127;
            SpawnPlacement point;
            point.x = int(x);
            point.y = int(y & 0xFFF);
            point.id = id;
            point.subtype = r.byte(p + 5);
            point.flags = (y >> (family == 3 ? 13 : 14)) & 3;
            point.state = st;
            point.function = objectFunction(r, index, family, id);
            point.sourceId = id;
            point.sourceFunction = point.function;
            point.sourceState = family == 3 ? 0xEB00 + st : st ? 0xFC02 + st : 0;
            bool boss = isBoss(family, originalS3, skSet, id);
            point.kind = boss ? SpawnKind::Boss : SpawnKind::Enemy;
            if (boss) {
                point.bossOrigin = id;
                point.bossOriginFunction = point.function;
            }
            roots.push_back(point);
            if (boss || enemy(family, originalS3, skSet, id))
                placements.push_back(point);
        }
        size_t old = placements.size();
        addEventBosses(r, family, stage, index, placements);
        if (family == 3)
            addScreenEventSpawns(r, originalS3, stage, index, placements);
        roots.insert(roots.end(), placements.begin() + old, placements.end());
        std::vector<SpawnPlacement> derived;
        addScriptSpawns(r, family, originalS3, index, roots, derived);
        std::set<std::tuple<int, int, unsigned, unsigned>> seen;
        for (auto &p : placements)
            seen.emplace(p.x, p.y, p.function, p.subtype);
        for (unsigned depth = 0; depth < 4 && !derived.empty(); ++depth) {
            std::vector<SpawnPlacement> fresh;
            for (auto &p : derived)
                if (p.x >= 0 && p.x < 32768 && p.y >= 0 && p.y < 4096 &&
                    seen.emplace(p.x, p.y, p.function, p.subtype).second) {
                    placements.push_back(p);
                    fresh.push_back(p);
                }
            derived.clear();
            if (depth < 3)
                addScriptSpawns(r, family, originalS3, index, fresh, derived);
        }
        placementCount = placements.size();
        for (auto &p : placements) {
            bossCount += p.kind == SpawnKind::Boss;
            scriptCount += p.kind == SpawnKind::Scripted;
        }
    }
    auto endOf = [&](unsigned fn) {
        auto at = functionBounds.find(fn);
        if (at == functionBounds.end())
            at = functionBounds.emplace(fn, objectEnd(r, index, family, fn)).first;
        return at->second;
    };
    for (auto p : placements) {
        if ((p.kind == SpawnKind::Boss && !bosses) || (p.kind == SpawnKind::Scripted && !scripts))
            continue;
        bool child = p.parentFunction != 0;
        unsigned stateAddr = p.event || child ? 0
                             : family == 3    ? 0xEB00 + p.state
                             : p.state        ? 0xFC02 + p.state
                                              : 0;
        if ((family < 3 && p.state > 255) || stateAddr >= 65536)
            continue;
        bool active = false, drawn = false;
        unsigned start = family == 1   ? 0xD040
                         : family == 2 ? 0xB080
                                       : 0xB094,
                 end = family == 1   ? 0xF000
                       : family == 2 ? 0xD000
                                     : 0xCFE0,
                 step = family == 3 ? 0x4A : 0x40;
        if (child && p.relative) {
            for (unsigned o = start; o + step <= end; o += step) {
                bool parent = family < 3 ? s.ram[o] == p.parentId
                                         : (ramPointer(s, o) >= p.parentFunction &&
                                            ramPointer(s, o) < endOf(p.parentFunction)) ||
                                               (ramPointer(s, o + 0x34) >= p.parentFunction &&
                                                ramPointer(s, o + 0x34) < endOf(p.parentFunction));
                if (!parent)
                    continue;
                bool same =
                    family == 3     ? (p.event || ramWord(s, o + 0x48) == 0xEB00 + p.parentState)
                    : p.parentState ? s.ram[o + 0x23] == p.parentState
                                    : (p.bossOrigin && p.parentId == p.bossOrigin) ||
                                          std::abs(int(ramWord(s, o + 8)) - p.x + p.dx) < 192;
                if (!same)
                    continue;
                if (p.relativeX)
                    p.x = int(ramWord(s, o + (family == 3 ? 0x10 : 8))) + p.dx;
                if (p.relativeY)
                    p.y = int(ramWord(s, o + (family == 3 ? 0x14 : 12))) + p.dy;
                break;
            }
        }
        bool sourceActive = false;
        if (child && p.sourceState && (s.ram[p.sourceState] & 128))
            for (unsigned o = start; o + step <= end; o += step) {
                if (family < 3)
                    sourceActive |=
                        s.ram[o] == p.sourceId && s.ram[o + 0x23] == p.sourceState - 0xFC02;
                else
                    sourceActive |= ramPointer(s, o) != 0 && ramWord(s, o + 0x48) == p.sourceState;
            }
        unsigned fnEnd = endOf(p.function);
        for (unsigned o = start; o + step <= end; o += step) {
            unsigned liveFunction = family == 3 ? ramPointer(s, o) : 0;
            if (family == 3 ? (!liveFunction ||
                               (stateAddr ? ramWord(s, o + 0x48) != stateAddr
                                          : liveFunction < p.function || liveFunction >= fnEnd))
                            : s.ram[o] != p.id)
                continue;
            bool match =
                p.event || child
                    ? std::abs(int(ramWord(s, o + (family == 3 ? 0x10 : 8))) - p.x) < 192 &&
                          std::abs(int(ramWord(s, o + (family == 3 ? 0x14 : 12))) - p.y) < 256
                : family == 3 ? ramWord(s, o + 0x48) == stateAddr
                : p.state     ? s.ram[o + 0x23] == p.state
                              : std::abs(int(ramWord(s, o + 8)) - p.x) < 96 &&
                                std::abs(int(ramWord(s, o + 12)) - p.y) < 256;
            if (!match)
                continue;
            active = true;
            unsigned flags = s.ram[o + (family == 3 ? 4 : 1)],
                     map = ramPointer(s, o + (family == 3 ? 0xC : 4)),
                     frame = s.ram[o + (family == 3 ? 0x22 : 0x1A)];
            int x = ramWord(s, o + (family == 3 ? 0x10 : 8)),
                y = ramWord(s, o + (family == 3 ? 0x14 : 12));
            if ((flags & 128) && x >= cameraX - 32 && x < cameraX + 352 && y >= cameraY - 32 &&
                y < cameraY + 256 && !pieces(r, family, map, frame).empty())
                drawn = true;
        }
        // Bit 7 remembers loaded or defeated objects. A waiting, empty sprite
        // can still be previewed; a defeated object with no live record cannot.
        if (drawn || (stateAddr && (s.ram[stateAddr] & 128) && !active))
            continue;
        if (child && p.sourceState && (s.ram[p.sourceState] & 128) && !sourceActive && !active)
            continue;
        if (family == 2 && stage == 0x0E00 && p.bossOrigin == 0xAF && s.ram[0xF7AA] >= 10)
            continue;
        if (p.bossOrigin && family < 3 && s.ram[0xF7A7]) {
            if (family == 1 || stage != 0x0E00 || (p.id == 0xAF && s.ram[0xF7AA] >= 9) ||
                (p.bossOrigin == 0xAF && s.ram[0xF7AA] >= 9) ||
                (p.bossOrigin == 0xC7 && s.ram[0xF7AA] >= 10))
                continue;
        }
        if (p.variable) {
            visible.push_back({p.x, p.y, p.flags, nullptr, p.kind, p.relative, true});
            continue;
        }
        uint64_t key = 1469598103934665603ull;
        for (unsigned part :
             {p.function, p.attributes, p.subtype, p.frame, unsigned(p.kind), p.bossOrigin}) {
            key ^= part;
            key *= 1099511628211ull;
        }
        auto found = appearances.find(key);
        if (found == appearances.end()) {
            Appearance a;
            unsigned fn = p.function, dplc = 0;
            unsigned end = std::min(objectEnd(r, index, family, fn),
                                    fn + (p.kind == SpawnKind::Enemy ? 768 : 8192));
            bool dynamic = false;
            // An enemy's setup can branch within its routine; never borrow a
            // following object's mapping, attributes or dynamic-art script.
            for (unsigned i = 0; i < (family == 1 ? 140u : 256u); ++i) {
                unsigned nextObject = r.pointer(index + i * 4);
                if (nextObject > fn && nextObject < end)
                    end = nextObject;
            }
            unsigned attributes = originalS3 ? 0x529A8 : 0x84038,
                     slotted = originalS3 ? 0x53638 : 0x84FA4,
                     perform = originalS3 ? 0x536B6 : 0x85022;
            for (unsigned c : codeInstructions(r, fn, end)) {
                if (!r.valid(c, 12))
                    continue;
                if (family < 3) {
                    if (!a.map && (r.word(c) == 0x217C || r.word(c) == 0x237C) &&
                        r.word(c + 6) == 4)
                        a.map = r.pointer(c + 2);
                    if (!a.gfx && (r.word(c) == 0x317C || r.word(c) == 0x337C) &&
                        r.word(c + 4) == 2)
                        a.gfx = r.word(c + 2);
                } else {
                    unsigned next = 0, data = lea(r, c, 1, next), target = callTarget(r, next);
                    if (!a.map && data && (target == attributes || target == slotted)) {
                        dynamic = target == slotted;
                        a.map = r.pointer(data + (target == slotted ? 8 : 0));
                        a.gfx = r.word(data + (target == slotted ? 2 : 4));
                        a.frame = r.byte(data + (target == slotted ? 16 : 10));
                    }
                    data = lea(r, c, 2, next);
                    if (!dplc && data && callTarget(r, next) == perform)
                        dplc = data;
                }
            }
            if (family == 2 && p.id >= 0x8C && p.id != 0xC8 && p.id != 0xC7) {
                unsigned data = 0x36628 + int16_t(r.word(0x36628 + (p.subtype & 254)));
                a.map = r.pointer(data);
                a.gfx = r.word(data + 4);
            }
            if (p.attributes && r.valid(p.attributes, 12)) {
                if (family == 1) {
                    a.map = r.pointer(p.attributes + 6);
                    a.gfx = r.word(p.attributes + 4);
                } else {
                    a.map = r.pointer(p.attributes);
                    a.gfx = r.word(p.attributes + 4);
                    a.frame = r.byte(p.attributes + 10);
                }
            }
            if (p.frame)
                a.frame = p.frame;
            if (family == 2 && p.id == 0xC7) {
                for (unsigned c : codeInstructions(r, fn, fn + 128)) {
                    if (!r.valid(c, 12))
                        continue;
                    unsigned after = 0, data = lea(r, c, 1, after);
                    if (data && callTarget(r, after) == 0x36602) {
                        a.map = r.pointer(data);
                        a.gfx = r.word(data + 4);
                        a.frame = 3;
                        break;
                    }
                }
            }
            // Newtron's first native frame is deliberately invisible.
            if (family == 1 && p.id == 0x42)
                a.frame = 3;
            if (dynamic && !dplc)
                a.map = 0;
            if (dynamic && dplc) {
                unsigned art = r.pointer(dplc), table = r.pointer(dplc + 4),
                         data = table + int16_t(r.word(table + a.frame * 2));
                unsigned count = r.word(data) + 1;
                if (count <= 32)
                    for (unsigned n = 0; n < count; ++n) {
                        unsigned entry = r.word(data + 2 + n * 2), source = art + (entry >> 4) * 32,
                                 bytes = ((entry & 15) + 1) * 32;
                        if (!r.valid(source, bytes) || a.tiles.size() + bytes > 8192) {
                            a.tiles.clear();
                            break;
                        }
                        for (unsigned b = 0; b < bytes; ++b)
                            a.tiles.push_back(r.byte(source + b));
                    }
                // Never substitute another object's current dynamic VRAM slot.
                if (a.tiles.empty())
                    a.map = 0;
            }
            if (!dynamic && p.bossOrigin) {
                a.tiles.assign(65536, 0);
                a.knownTiles.assign(2048, 0);
                a.sheet = true;
                auto install = [&](unsigned art, unsigned destination, bool kos) {
                    unsigned key = (r.address(art) << 1) | unsigned(kos);
                    auto found = decodedArt.find(key);
                    if (found == decodedArt.end())
                        found = decodedArt
                                    .emplace(key, kos ? decodeKosinskiModules(rom, r.address(art))
                                                      : decodeNemesis(rom, r.address(art)))
                                    .first;
                    auto &bytes = found->second;
                    if (bytes.empty() || destination >= 65536 || bytes.size() > 65536 - destination)
                        return;
                    std::copy(bytes.begin(), bytes.end(), a.tiles.begin() + destination);
                    for (unsigned t = destination / 32; t < (destination + bytes.size()) / 32; ++t)
                        a.knownTiles[t] = 1;
                };
                auto cue = [&](unsigned table, unsigned number) {
                    if (!r.valid(table + number * 2, 2))
                        return;
                    unsigned data = table + int16_t(r.word(table + number * 2)),
                             count = r.word(data) + 1;
                    if (!count || count > 32 || !r.valid(data + 2, count * 6))
                        return;
                    for (unsigned n = 0; n < count; ++n)
                        install(r.pointer(data + 2 + n * 6), r.word(data + 6 + n * 6), false);
                };
                if (family == 1)
                    cue(0x1D612, p.bossOrigin == 0x85 ? 31 : 17);
                if (family == 2) {
                    unsigned number = 0;
                    switch (p.bossOrigin) {
                    case 0x5D:
                        number = 40;
                        break;
                    case 0x56:
                        number = 41;
                        break;
                    case 0x52:
                        number = 42;
                        break;
                    case 0x89:
                        number = 43;
                        break;
                    case 0x57:
                        number = 44;
                        break;
                    case 0x51:
                        number = 45;
                        break;
                    case 0x54:
                        number = 46;
                        break;
                    case 0x55:
                        number = 47;
                        break;
                    case 0xC7:
                        number = 49;
                        break;
                    case 0xC5:
                        number = 62;
                        break;
                    case 0xAF:
                        number = 33;
                        break;
                    }
                    if (number)
                        cue(0x42660, number);
                }
                if (family == 3) {
                    unsigned loader = originalS3 ? 0x1984 : 0x16FA, table = 0;
                    for (unsigned c = loader; c < loader + 32; c += 2)
                        if (r.word(c) == 0x43F9) {
                            table = r.pointer(c + 2);
                            break;
                        }
                    for (unsigned origin : {fn, p.bossOriginFunction}) {
                        unsigned limit = std::min(endOf(origin), origin + 65536);
                        for (unsigned c : codeInstructions(r, origin, limit)) {
                            if (!r.valid(c, 8))
                                continue;
                            if (callTarget(r, c) == loader) {
                                unsigned number = 256;
                                for (unsigned pc = c >= 16 ? c - 16 : 0; pc < c; pc += 2) {
                                    if ((r.word(pc) & 0xFF00) == 0x7000)
                                        number = r.word(pc) & 255;
                                    if (r.word(pc) == 0x303C)
                                        number = r.word(pc + 2);
                                }
                                if (table && number < 256)
                                    cue(table, number);
                            }
                            unsigned after = 0, art = lea(r, c, 1, after);
                            if (!art || !r.valid(art, 2))
                                continue;
                            unsigned destination = 65536;
                            for (unsigned pc = after; pc < after + 24 && pc + 6 < limit; pc += 2) {
                                if (r.word(pc) == 0x343C)
                                    destination = r.word(pc + 2);
                                unsigned callee = callTarget(r, pc);
                                if (callee == (originalS3 ? 0x1D5C : 0x1AD2) &&
                                    destination < 65536) {
                                    install(art, destination, true);
                                    break;
                                }
                            }
                            // Queue_Kos_Module has a stable routine signature;
                            // only an actual call to it authorizes this source.
                        }
                    }
                }
                if (std::none_of(a.knownTiles.begin(), a.knownTiles.end(),
                                 [](uint8_t b) { return b != 0; }))
                    a.map = 0;
            }
            found = appearances.emplace(key, std::move(a)).first;
        }
        auto &a = found->second;
        auto shape = pieces(r, family, a.map, a.frame);
        auto &image = a.image;
        image = {};
        if (!shape.empty()) {
            int left = 128, top = 128, right = -128, bottom = -128;
            for (auto &q : shape) {
                left = std::min(left, q.x);
                top = std::min(top, q.y);
                right = std::max(right, q.x + q.width);
                bottom = std::max(bottom, q.y + q.height);
            }
            if (right - left <= 192 && bottom - top <= 192) {
                image.left = left;
                image.top = top;
                image.width = right - left;
                image.height = bottom - top;
                image.pixels.assign(size_t(image.width) * image.height, 0);
                for (auto &q : shape)
                    for (int y = 0; y < q.height; ++y)
                        for (int x = 0; x < q.width; ++x) {
                            unsigned attr = q.attr + a.gfx;
                            int ix = (attr & 0x800) ? q.width - 1 - x : x,
                                iy = (attr & 0x1000) ? q.height - 1 - y : y;
                            unsigned tile = (a.tiles.empty() || a.sheet ? attr : q.attr) & 2047,
                                     offset = (tile + (ix / 8) * (q.height / 8) + iy / 8) * 32 +
                                              (iy & 7) * 4 + (ix & 7) / 2;
                            unsigned b = a.sheet && !a.knownTiles[(offset / 32) & 2047] ? 0
                                         : a.tiles.empty()         ? s.vram[offset & 65535]
                                         : offset < a.tiles.size() ? a.tiles[offset]
                                                                   : 0;
                            unsigned color = (ix & 1) ? b & 15 : b >> 4;
                            if (color)
                                image.pixels[size_t(q.y - top + y) * image.width + q.x - left + x] =
                                    uint8_t(((attr >> 9) & 48) | color);
                        }
            }
        }
        if (std::none_of(image.pixels.begin(), image.pixels.end(),
                         [](uint8_t pixel) { return pixel != 0; }))
            image.pixels.clear();
        visible.push_back(
            {p.x, p.y, p.flags,
             image.pixels.empty() || p.variable || (family == 2 && child && p.id == 0xC7) ? nullptr
                                                                                          : &image,
             p.kind, p.relative, p.variable});
    }
}
} // namespace scu
