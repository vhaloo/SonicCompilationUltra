#include "world.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <memory>

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
