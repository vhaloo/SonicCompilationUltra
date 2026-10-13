#include "world.h"
#include "localization.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <cassert>
using namespace scu;
void testEnemyPreviews();
void testBossScriptPreviews();
static void require(bool c, const char *msg) {
    if (!c) {
        std::cerr << msg << "\n";
        std::exit(1);
    }
}
int main() {
    testEnemyPreviews();
    testBossScriptPreviews();
    require(games().size() >= 12, "collection identities missing");
    require(gameById("sonic3k")->size == 4194304, "lock-on size");
    require(detectLanguage(3) == 3, "explicit Japanese language");
    require(std::strcmp(tr("menuhint", 0), tr("menuhint", 3)) != 0, "Japanese menu hint");
    auto world = std::make_unique<World>();
    auto s = std::make_unique<SCUSnapshot>();
    std::memset(s.get(), 0, sizeof *s);
    auto put = [&](int a, int v) {
        s->ram[a] = v >> 8;
        s->ram[a + 1] = v & 255;
    };
    // Synthetic S1 stage with a real chunk/block/pattern topology and a
    // vertically distant tile. This catches old fixed-224-height assumptions.
    s->ram[0xF600] = 12;
    s->ram[0xD000] = 1;
    s->ram[0xD024] = 2;
    s->ram[0xA400] = 1;
    s->ram[0xA400 + 128 * 3 + 31] = 1;
    put(0xD008, 100);
    put(0xD00C, 100);
    put(0xB000, 1);
    for (int i = 0; i < 32; i++)
        s->vram[32 + i] = 0x11;
    for (auto &row : s->palettes)
        for (auto &c : row)
            c = 0xFF336699;
    auto scene = world->inspect(*s, gameById("sonic1"));
    require(scene.ready && scene.width == 8192 && scene.height == 1024, "stage bounds");
    require(world->attribute(*s, 1, 0, 0) == 1, "S1 block decode");
    require(world->attribute(*s, 1, -8, 0) == 0, "outside stage guard");
    auto before = s->ram[0xD008];
    Settings settings;
    require(settings.fullscreen && !settings.span && settings.view == 2 &&
                settings.menuLayout == 0 && settings.enemyPreview && settings.enemyOpacity == 35 &&
                settings.enemyStyle == 0,
            "new installations must use fullscreen, full menu and stage overview");
    settings.view = 0;
    settings.zoom = 1;
    settings.camera = 2;
    settings.left = 0;
    settings.top = 0;
    world->compose(*s, gameById("sonic1"), settings, 7680, 1440);
    require(world->canvas.width == 7680 && world->canvas.height == 1440, "true physical 1:1");
    require(world->canvas.pixels.size() == 7680 * 1440, "expanded storage");
    require(s->ram[0xD008] == before, "compositor changed guest RAM");
    settings.zoom = 2;
    world->compose(*s, gameById("sonic1"), settings, 7680, 1440);
    require(world->canvas.width == 3840 && world->canvas.height == 720, "integer zoom 2");
    settings.zoom = 1;
    world->compose(*s, gameById("sonic1"), settings, 320, 240);
    require(world->canvas.width == 320 && world->canvas.height == 240, "tiny window");
    settings.view = 2;
    settings.quality = 2;
    world->compose(*s, gameById("sonic1"), settings, 1920, 1080);
    require(world->canvas.coverageW >= scene.width && world->canvas.coverageH >= scene.height &&
                world->canvas.width == 1920,
            "overview omits terrain or overallocates");
    settings.view = 0;
    settings.zoom = 1;
    world->compose(*s, gameById("sonic1"), settings, 1080, 1920);
    require(world->canvas.height == 1920, "portrait canvas");
    settings.view = 4;
    require(!world->compose(*s, gameById("sonic1"), settings, 1920, 1080), "classic fallback");
    require(!world->inspect(*s, gameById("meanbean")).ready, "puzzle must keep native framing");
    // S2 flip bits differ from S1. Test a flipped block without confusing them.
    s->ram[0x8000] = 2;
    s->ram[0xB000] = 1;
    s->ram[0x8002] = 2;
    put(0xFFD8, 0x8000);
    require(world->inspect(*s, gameById("sonic2k")).ready,
            "Knuckles lock-on flag mistaken for competition mode");
    require(!world->inspect(*s, gameById("sonic2")).ready,
            "Sonic 2 competition must keep native framing");
    put(0xFFD8, 1);
    require(!world->inspect(*s, gameById("sonic2k")).ready,
            "Knuckles competition must keep native framing");
    put(0xFFD8, 0);
    put(2 * 128, 0x400 | 3);
    put(0x9000 + 3 * 8 + 2, 0x55);
    require(world->attribute(*s, 2, 0, 0) == (0x55 ^ 0x800), "S2 block flip decode");
    // S3 uses row pointers and independently stored plane dimensions.
    put(0x8000, 40);
    put(0x8004, 8);
    put(0x8008, 0x8088);
    s->ram[0x8088] = 2;
    require(world->attribute(*s, 3, 0, 0) == (0x55 ^ 0x800), "S3 row-pointer decode");
    Settings mapSettings;
    mapSettings.quality = 2;
    world->compose(*s, gameById("sonic3"), mapSettings, 640, 360);
    require(world->canvas.coverageW >= 5120 && world->canvas.coverageH >= 1024,
            "schematic overview must retain stage coverage");
    for (int n = 0; n < 3; ++n) {
        settings.view = 2;
        settings.preset(n);
        world->compose(*s, gameById("sonic1"), settings, 1920, 1080);
        require(settings.view == 2 && world->canvas.coverageW >= scene.width &&
                    world->canvas.coverageH >= scene.height,
                "quality preset must not crop the overview");
    }
    std::cout << "PASS: cartridge identities, localisation, world addressing, 1:1 / zoom / "
                 "overview / portrait / tiny / classic and read-only guest memory\n";
}
