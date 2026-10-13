#include "world.h"
#include "rom_graphics.h"
#include "rom_code.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <algorithm>

using namespace scu;
namespace {
void check(bool result, const char *message) {
    if (!result) {
        std::cerr << "Enemy previews: " << message << '\n';
        std::exit(1);
    }
}
struct Fixture {
    const Game *game;
    std::vector<uint8_t> rom;
    std::unique_ptr<SCUSnapshot> s = std::make_unique<SCUSnapshot>();
    unsigned id, map = 0x3000, stage = 0, base, state;
    Fixture(const char *name, bool slotted = false) : game(gameById(name)), rom(game->size) {
        std::memset(s.get(), 0, sizeof *s);
        int family = game->family;
        bool originalS3 = game->crc == 0x9BC192CE;
        if (game->crc == 0x0658F691)
            stage = 0x700;
        id = family == 1 ? 0x40 : family == 2 ? 0x4B : originalS3 ? 129 : 140;
        unsigned index = family == 1   ? 0xD382
                         : family == 2 ? 0x1600C
                         : originalS3  ? 0x5CC96
                         : stage       ? 0x952A2
                                       : 0x94EA2;
        put32(index + (family == 3 ? id : id - 1) * 4, 0x1000);
        if (family < 3) {
            put16(0x1000, 0x217C);
            put32(0x1002, map);
            put16(0x1006, 4);
            put16(0x1008, 0x317C);
            put16(0x100A, 0x100);
            put16(0x100C, 2);
            unsigned table = family == 1 ? 0x6B000 : 0xE6800;
            base = table + 0x100;
            put16(table, 0x100);
        } else {
            put16(0x1000, 0x43FA);
            put16(0x1002, 0x2300 - 0x1002);
            put16(0x1004, 0x4EB9);
            put32(0x1006, slotted      ? (originalS3 ? 0x53638 : 0x84FA4)
                          : originalS3 ? 0x529A8
                                       : 0x84038);
            if (slotted) {
                put16(0x2300, 1);
                put16(0x2302, 0x2100);
                put32(0x2308, map);
                put16(0x100A, 0x45FA);
                put16(0x100C, 0x2400 - 0x100C);
                put16(0x100E, 0x4EB9);
                put32(0x1010, originalS3 ? 0x536B6 : 0x85022);
                put32(0x2400, 0x110000);
                put32(0x2404, 0x2500);
                put16(0x2500, 2);
                put16(0x2502, 0); // One transfer of four original, synthetic tiles.
                put16(0x2504, 3);
                for (unsigned i = 0; i < 128; ++i)
                    put8(0x110000 + i, 0x11);
            } else {
                // A high CPU address is mirrored by standalone S&K.
                put32(0x2300, game->crc == 0x0658F691 ? map + 0x200000 : map);
                put16(0x2304, 0x100);
            }
            unsigned table = originalS3 ? 0x5E0D8 : 0x1E3D98;
            base = table + 0x100;
            put32(table + (stage >> 8) * 8, base);
        }
        put16(map, 4);
        unsigned piece = map + 4;
        if (family == 1)
            put8(piece++, 1);
        else {
            put16(piece, 1);
            piece += 2;
        }
        put8(piece, uint8_t(-8));
        put8(piece + 1, 5);
        put16(piece + 2, 0);
        if (family == 1)
            put8(piece + 4, uint8_t(-8));
        else
            put16(piece + (family == 2 ? 6 : 4), uint16_t(-8));
        for (unsigned n = 0; n < 3; ++n) {
            put16(base + n * 6, 1000 + n * 500);
            put16(base + n * 6 + 2, 300 | (family == 2 ? 0x8000 : 0));
            put8(base + n * 6 + 4, id | (family == 1 ? 128 : 0));
        }
        put16(base + 18, 0xFFFF);
        state = family == 3 ? 0xEB00 : 0xFC03;
        if (!slotted)
            for (unsigned i = 0; i < 128; ++i)
                s->vram[0x100 * 32 + i] = 0x11;
        for (auto &row : s->palettes)
            for (auto &color : row)
                color = 0xFF55AA88;
    }
    unsigned address(unsigned a) {
        if (game->crc == 0x2AC1E7C6 && a < 0x100000)
            return a + 0x200000;
        if (game->crc == 0x0658F691)
            return a & 0x1FFFFF;
        return a;
    }
    void put8(unsigned a, unsigned v) {
        rom.at(address(a)) = uint8_t(v);
    }
    void put16(unsigned a, unsigned v) {
        put8(a, v >> 8);
        put8(a + 1, v);
    }
    void put32(unsigned a, unsigned v) {
        put16(a, v >> 16);
        put16(a + 2, v);
    }
    void ram16(unsigned a, unsigned v) {
        s->ram[a] = uint8_t(v >> 8);
        s->ram[a + 1] = uint8_t(v);
    }
    void ram32(unsigned a, unsigned v) {
        ram16(a, v >> 16);
        ram16(a + 2, v);
    }
};
} // namespace

void testEnemyPreviews() {
    for (const char *name : {"sonic1", "sonic2", "sonic2k", "sonic3", "sandk", "sonic3k"}) {
        Fixture f(name);
        EnemyPreviews e;
        auto before = std::make_unique<SCUSnapshot>(*f.s);
        e.update(*f.s, f.game, f.rom, f.stage, 0, 0);
        check(e.supported && e.placementCount == 3 && e.visible.size() == 3,
              "validated family must show all authored distant enemies");
        check(e.visible[0].x == 1000 && e.visible[0].y == 300, "spawn coordinates");
        check(e.visible[0].image && e.visible[0].image->width == 16 &&
                  e.visible[0].image->pixels[0] == 1,
              "native cartridge sprite decoding");
        check(std::memcmp(before.get(), f.s.get(), sizeof *f.s) == 0, "guest snapshot was mutated");
        f.s->ram[f.state] = 128;
        e.update(*f.s, f.game, f.rom, f.stage, 0, 0);
        check(e.visible.size() == 2 && e.visible[0].x == 1500,
              "defeated remembered enemy must disappear");
        unsigned o = f.game->family == 1 ? 0xD040 : f.game->family == 2 ? 0xB080 : 0xB094;
        if (f.game->family == 3) {
            f.ram32(o, 0x1000);
            f.ram16(o + 0x48, f.state);
            f.ram32(o + 0xC, f.map);
            f.ram16(o + 0x10, 1000);
            f.ram16(o + 0x14, 300);
            f.s->ram[o + 4] = 0x84;
        } else {
            f.s->ram[o] = uint8_t(f.id);
            f.s->ram[o + 0x23] = 1;
            f.ram32(o + 4, f.map);
            f.ram16(o + 8, 1000);
            f.ram16(o + 12, 300);
            f.s->ram[o + 1] = 0x84;
        }
        e.update(*f.s, f.game, f.rom, f.stage, 950, 250);
        check(e.visible.size() == 2, "native visible enemy must not have a duplicate ghost");
        f.ram32(o + (f.game->family == 3 ? 0xC : 4), 0);
        e.update(*f.s, f.game, f.rom, f.stage, 950, 250);
        check(e.visible.size() == 3, "waiting empty native sprite must retain its warning");
    }
    for (const char *name : {"sonic3", "sandk", "sonic3k"}) {
        Fixture f(name, true);
        EnemyPreviews e;
        e.update(*f.s, f.game, f.rom, f.stage, 0, 0);
        check(e.visible.size() == 3 && e.visible[0].image && e.visible[0].image->pixels[0] == 17,
              "dynamic DPLC preview must use ROM art even with empty VRAM");
    }
    Fixture f("sonic1");
    auto world = std::make_unique<World>();
    world->setRom(f.rom);
    f.s->ram[0xF600] = 12;
    f.s->ram[0xD000] = 1;
    f.s->ram[0xD024] = 2;
    f.s->ram[0xA400 + 128 * 3 + 8] = 1;
    f.ram16(0xD008, 1000);
    f.ram16(0xD00C, 300);
    Settings cfg;
    cfg.quality = 2;
    cfg.enemyStyle = 1;
    auto before = std::make_unique<SCUSnapshot>(*f.s);
    check(world->compose(*f.s, f.game, cfg, 1920, 1080), "expanded compositor fixture");
    check(world->enemySpriteCount == 3, "transparent sprites must reach the actual compositor");
    bool translucent = false;
    for (auto color : world->canvas.pixels)
        if ((color >> 24) == 35 * 255 / 100)
            translucent = true;
    check(translucent, "configured opacity was not applied");
    cfg.enemyStyle = 2;
    world->compose(*f.s, f.game, cfg, 320, 240);
    check(world->enemyMarkerCount == 3 && world->enemySpriteCount == 0,
          "tiny overview must retain readable spawn markers");
    cfg.enemyPreview = false;
    world->compose(*f.s, f.game, cfg, 640, 360);
    check(world->enemies.visible.empty() && !world->enemyMarkerCount && !world->enemySpriteCount,
          "disabled preview must not draw");
    check(std::memcmp(before.get(), f.s.get(), sizeof *f.s) == 0, "rendering altered guest state");
    EnemyPreviews malformed;
    malformed.update(*f.s, f.game, {0, 0}, 0, 0, 0);
    check(malformed.visible.empty(), "truncated ROM must not invent enemies");
    malformed.update(*f.s, gameById("meanbean"), f.rom, 0, 0, 0);
    check(!malformed.supported && malformed.visible.empty(), "unsupported projection preview");
}

void testBossScriptPreviews() {
    Fixture codeFixture("sonic2");
    codeFixture.put16(0x1000, 0x203C);
    codeFixture.put32(0x1002, 0x12BC0098);
    codeFixture.put16(0x1006, 0x4E75);
    codeFixture.put16(0x1008, 0x12BC);
    codeFixture.put16(0x100A, 0x98);
    auto decoded =
        codeInstructions(RomView{codeFixture.rom, codeFixture.game->crc}, 0x1000, 0x1020);
    check(decoded == std::vector<unsigned>({0x1000, 0x1006}),
          "creation-looking operands and unreachable data must not become script spawns");
    check(isBoss(1, false, false, 0x85) && isBoss(2, false, false, 0xC7), "final boss catalogue");
    check(isBoss(3, true, false, 208) && isBoss(3, false, true, 182), "S3 and DDZ boss catalogue");
    check(!isBoss(3, false, false, 175) && !isBoss(3, false, false, 199) &&
              !isBoss(3, false, false, 200) && !isBoss(3, false, true, 160),
          "rings, pillars and EggRobos are not bosses");
    Fixture f("sonic1");
    f.put16(0x6B008, 0x100);
    f.put32(0xD382 + (0x3D - 1) * 4, 0x5000);
    f.put32(0xD382 + (0x78 - 1) * 4, 0x10000);
    f.put16(0x5000, 0x217C);
    f.put32(0x5002, f.map);
    f.put16(0x5006, 4);
    f.put16(0x5008, 0x317C);
    f.put16(0x500A, 0x100);
    f.put16(0x500C, 2);
    f.put16(0x500E, 0x4E71);
    f.put16(0x5010, 0x12BC);
    f.put16(0x5012, 0x78);
    f.put16(0x5014, 0x3368);
    f.put16(0x5016, 8);
    f.put16(0x5018, 8);
    f.put16(0x501A, 0x0669);
    f.put16(0x501C, 32);
    f.put16(0x501E, 8);
    f.put16(0x5020, 0x3368);
    f.put16(0x5022, 12);
    f.put16(0x5024, 12);
    f.put16(0x5026, 0x4E75);
    f.put16(0x6EA4, 0x12BC);
    f.put16(0x6EA6, 0x3D);
    f.put16(0x6EA8, 0x337C);
    f.put16(0x6EAA, 2400);
    f.put16(0x6EAC, 8);
    f.put16(0x6EAE, 0x337C);
    f.put16(0x6EB0, 600);
    f.put16(0x6EB2, 12);
    // One synthetic Nemesis tile, dictionary code 0 = eight nibbles of color 1.
    // A PLC relative offset must fit a signed word. Place this cue nearby.
    unsigned cue = 0x1D712;
    f.put16(0x1D612 + 17 * 2, 0x100);
    f.put16(cue, 0);
    f.put32(cue + 2, 0x7000);
    f.put16(cue + 6, 0x2000);
    unsigned n = 0x7000;
    for (auto b : {0, 1, 0x81, 0x71, 0, 0xFF, 0})
        f.put8(n++, b);
    EnemyPreviews e;
    auto before = std::make_unique<SCUSnapshot>(*f.s);
    e.update(*f.s, f.game, f.rom, 2, 0, 0);
    check(e.bossCount == 1 && e.scriptCount == 1 && e.visible.size() == 5,
          "event boss and parent-relative scripted threat must be forecast before activation");
    check(std::memcmp(before.get(), f.s.get(), sizeof *f.s) == 0,
          "spawn reader changed native state");
    bool bossArt = false, child = false;
    for (auto p : e.visible) {
        if (p.kind == SpawnKind::Boss)
            bossArt = p.x == 2400 && p.y == 600 && p.image && p.image->pixels[0] == 1;
        if (p.kind == SpawnKind::Scripted)
            child = p.x == 2432 && p.y == 600 && p.relative;
    }
    check(bossArt && child, "boss art must come from the cartridge PLC before VRAM loading");
    e.update(*f.s, f.game, f.rom, 2, 0, 0, false, false);
    check(e.visible.size() == 3, "boss and script filters must work independently of enemies");
    f.s->ram[0xD040] = 0x3D;
    f.ram16(0xD048, 2600);
    f.ram16(0xD04C, 640);
    e.update(*f.s, f.game, f.rom, 2, 0, 0);
    child = false;
    for (auto p : e.visible)
        if (p.kind == SpawnKind::Scripted)
            child = p.x == 2632 && p.y == 640;
    check(child, "a relative source must follow its live parent");
    *f.s = *before;
    f.s->ram[0xF7A7] = 1;
    e.update(*f.s, f.game, f.rom, 2, 0, 0);
    check(e.visible.size() == 3, "defeated boss and its scripted children must disappear");
    *f.s = *before;
    check(std::memcmp(before.get(), f.s.get(), sizeof *f.s) == 0,
          "spawn reader changed native state");
    auto world = std::make_unique<World>();
    world->setRom(f.rom);
    f.s->ram[0xF600] = 12;
    f.s->ram[0xD000] = 1;
    f.s->ram[0xD024] = 2;
    f.s->ram[0xA400] = f.s->ram[0xA401] = 1;
    f.ram16(0xFE10, 2);
    Settings cfg;
    cfg.enemyStyle = 2;
    auto guest = std::make_unique<SCUSnapshot>(*f.s);
    check(world->compose(*f.s, f.game, cfg, 640, 360) && world->enemyMarkerCount == 5 &&
              world->canvas.coverageW >= 2528,
          "overview includes an arena beyond the loaded terrain");
    cfg.bossPreview = cfg.scriptPreview = false;
    world->compose(*f.s, f.game, cfg, 320, 240);
    check(world->enemyMarkerCount == 3, "actual compositor respects boss/script controls");
    check(std::memcmp(guest.get(), f.s.get(), sizeof *f.s) == 0,
          "boss compositor changed guest memory");

    for (const char *name : {"sonic3", "sandk", "sonic3k"}) {
        Fixture g(name);
        for (unsigned c = 0x100A; c < 0x1020; c += 2)
            g.put16(c, 0x4E71);
        bool s3 = g.game->crc == 0x9BC192CE;
        unsigned index = s3 ? 0x5CC96 : g.stage ? 0x952A2 : 0x94EA2;
        g.put16(0x1020, 0x45FA);
        g.put16(0x1022, 0x2600 - 0x1022);
        g.put16(0x1024, 0x4EB9);
        g.put32(0x1026, s3 ? 0x529D0 : 0x84060);
        g.put16(0x2600, 0);
        g.put32(0x2602, 0x2000);
        g.put8(0x2606, 24);
        g.put8(0x2607, uint8_t(-16));
        g.put32(index + 220 * 4, 0x2100); // Bound the child program.
        EnemyPreviews forecasts;
        forecasts.update(*g.s, g.game, g.rom, g.stage, 0, 0);
        check(forecasts.scriptCount == 3,
              "S3 normal child factory table must be decoded in each bank");
        unsigned matched = 0;
        for (auto p : forecasts.visible)
            matched += p.kind == SpawnKind::Scripted && p.x % 500 == 24 && p.y == 284;
        check(matched == 3, "signed child offsets are relative to each authored parent");
    }
    Fixture unknown("sonic2");
    Fixture typed("sonic2");
    for (unsigned c = 0x100E; c < 0x1020; c += 2)
        typed.put16(c, 0x4E71);
    typed.put16(0x1020, 0x45FA);
    typed.put16(0x1022, 0x2600 - 0x1022);
    typed.put16(0x1024, 0x4EB9);
    typed.put32(0x1026, 0x367D0);
    typed.put16(0x102A, 0x0669);
    typed.put16(0x102C, 32);
    typed.put16(0x102E, 8);
    typed.put16(0x1030, 0x4E75);
    typed.put16(0x2600, 0x34);
    typed.put8(0x2602, 0x98);
    typed.put8(0x2603, 0x84);
    SpawnPlacement typedRoot;
    typedRoot.function = 0x1000;
    typedRoot.x = 1000;
    typedRoot.y = 300;
    std::vector<SpawnPlacement> typedPoints;
    addScriptSpawns(RomView{typed.rom, typed.game->crc}, 2, false, 0x1600C, {typedRoot},
                    typedPoints);
    check(typedPoints.size() == 1 && typedPoints.front().subtype == 0x84 &&
              typedPoints.front().x == 1032,
          "S2 typed child table and absolute-long factory call");
    for (int format = 1; format <= 10; ++format) {
        Fixture g("sandk");
        for (unsigned c = 0x100A; c < 0x1020; c += 2)
            g.put16(c, 0x4E71);
        unsigned offsets[] = {0, 0x50, 0xB4, 0x106, 0x146, 0x1C0, 0x1F8, 0x248, 0x288, 0x2C8};
        g.put16(0x1020, 0x45FA);
        g.put16(0x1022, 0x2600 - 0x1022);
        g.put16(0x1024, 0x4EB9);
        g.put32(0x1026, 0x84060 + offsets[format - 1]);
        g.put16(0x102A, 0x4E75);
        g.put32(0x952A2 + 220 * 4, 0x2100);
        bool repeated = format == 3 || format == 4 || format == 6 || format == 8;
        bool complex = format == 2 || format == 5;
        unsigned length = complex                                                    ? 22
                          : format == 4 || format == 6 || format == 8 || format == 9 ? 4
                                                                                     : 6;
        g.put16(0x2600, 1);
        for (unsigned n = 0; n < (repeated ? 1u : 2u); ++n) {
            unsigned entry = 0x2602 + n * length;
            g.put32(entry, 0x2000);
            if (complex) {
                g.put32(entry + 4, 0x2300);
                g.put32(entry + 12, 0x2000);
                g.put8(entry + 16, 24 + n * 16);
                g.put8(entry + 17, uint8_t(-16));
            } else if (length == 6) {
                g.put8(entry + 4, 24 + n * 16);
                g.put8(entry + 5, uint8_t(-16));
            }
        }
        std::vector<SpawnPlacement> points;
        SpawnPlacement parent;
        parent.x = 1000;
        parent.y = 300;
        parent.function = 0x1000;
        addScriptSpawns(RomView{g.rom, g.game->crc}, 3, false, 0x952A2, {parent}, points);
        unsigned expected = repeated || format == 9 ? 1 : 2;
        check(points.size() == expected,
              "all S3 child factory formats must decode and deduplicate positions");
        check(points.front().x == 1000 + (length == 4 ? 0 : 24) &&
                  points.front().y == 300 - (length == 4 ? 0 : 16),
              "factory offsets and descriptor lengths");
    }
    for (unsigned c = 0x100E; c < 0x1020; c += 2)
        unknown.put16(c, 0x4E71);
    unknown.put16(0x1020, 0x12BC);
    unknown.put16(0x1022, 0x98);
    unknown.put16(0x1024, 0x4E75);
    EnemyPreviews origins;
    origins.update(*unknown.s, unknown.game, unknown.rom, 0, 0, 0);
    unsigned variables = 0;
    for (auto p : origins.visible)
        variables += p.variable && p.relative && !p.image;
    check(variables == 3,
          "unresolved attacks must show marked origins, without invented ghost art");
    unknown.s->ram[unknown.state] = 128;
    origins.update(*unknown.s, unknown.game, unknown.rom, 0, 0, 0);
    check(origins.visible.size() == 4, "defeated remembered source must hide its future attacks");

    auto nem = decodeNemesis({0, 1, 0x81, 0x71, 0, 255, 0}, 0);
    check(nem.size() == 32 && std::all_of(nem.begin(), nem.end(), [](auto b) { return b == 0x11; }),
          "Nemesis dictionary decoding");
    auto xorArt = decodeNemesis({0x80, 1, 0x81, 0x71, 0, 255, 0}, 0);
    check(xorArt.size() == 32 && xorArt[0] == 0x11 && xorArt[4] == 0 && xorArt[8] == 0x11,
          "Nemesis XOR scanlines");
    check(decodeNemesis({0, 1, 255}, 0).empty(), "truncated Nemesis must fail closed");
    std::vector<uint8_t> inlineArt{0, 1, 255};
    unsigned accumulator = 0, bits = 0;
    for (unsigned run = 0; run < 8; ++run)
        for (int bit = 12; bit >= 0; --bit) {
            accumulator = (accumulator << 1) | ((0x1FF2 >> bit) & 1);
            if (++bits == 8) {
                inlineArt.push_back(uint8_t(accumulator));
                bits = 0;
                accumulator = 0;
            }
        }
    auto inlinePixels = decodeNemesis(inlineArt, 0);
    check(inlinePixels.size() == 32 && inlinePixels.front() == 0x22 && inlinePixels.back() == 0x22,
          "Nemesis inline runs");
    auto kos = decodeKosinskiModules({0, 3, 0x41, 0, 0xAB, 0xFF, 0, 0, 0}, 0);
    check(kos == std::vector<uint8_t>({0xAB, 0xAB, 0xAB}), "Kosinski literal and short copy");
    check(decodeKosinskiModules({0, 3, 0x41, 0, 0xAB}, 0).empty(),
          "truncated Kosinski must fail closed");
    auto longCopy = decodeKosinskiModules({0, 10, 0x15, 0, 0xAB, 0xFF, 0xF8, 8, 0, 0, 0}, 0);
    check(longCopy.size() == 10 && longCopy.back() == 0xAB, "Kosinski extended long copy");
    std::vector<uint8_t> boundary{0, 16, 255, 255};
    for (unsigned i = 0; i < 15; ++i)
        boundary.push_back(uint8_t(i));
    for (auto b : {2, 0, 15, 0, 0, 0})
        boundary.push_back(uint8_t(b));
    auto boundaryPixels = decodeKosinskiModules(boundary, 0);
    check(boundaryPixels.size() == 16 && boundaryPixels.front() == 0 && boundaryPixels.back() == 15,
          "Kosinski prefetch before the sixteenth token's data");
}
