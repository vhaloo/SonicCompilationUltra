#include "world.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>
#include <cstring>
using namespace scu;
static void require(bool v, const char *s) {
    if (!v) {
        std::fprintf(stderr, "FAIL %s\n", s);
        std::exit(1);
    }
}
static void word(uint8_t *bytes, unsigned at, unsigned value) {
    bytes[at] = uint8_t(value >> 8);
    bytes[at + 1] = uint8_t(value);
}
static std::unique_ptr<SCUSnapshot> generated() {
    auto s = std::make_unique<SCUSnapshot>();
    s->ram[0xF600] = 12;
    s->ram[0xD000] = 1;
    s->regs[1] = 64;
    s->regs[5] = 120;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 8; ++x)
            s->ram[0xA400 + y * 128 + x] = 1;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 8; ++x)
            s->ram[0xA440 + y * 128 + x] = 1;
    for (int n = 0; n < 256; ++n)
        word(s->ram, n * 2, 1);
    for (int n = 0; n < 4; ++n)
        word(s->ram, 0xB008 + n * 2, n + 1);
    for (int tile = 1; tile < 8; ++tile)
        for (int n = 0; n < 32; ++n)
            s->vram[tile * 32 + n] = uint8_t((tile << 4) | ((tile + n) & 15));
    for (int y = 0; y < 224; ++y) {
        for (int p = 0; p < 64; ++p)
            s->palettes[y][p] = 0xff000000 | uint32_t(p * 37 % 256) << 16 |
                                uint32_t(p * 67 % 256) << 8 | uint32_t(p * 97 % 256);
        for (int x = 0; x < 320; ++x)
            s->background[y][x] = 0xff102030;
    }
    return s;
}
int main() {
    Game fixture{"generated", "Generated compositor data", 0, 0, 1, "no cartridge"};
    int comparisons = 0;
    int volumeComparisons = 0;
    uint64_t uploaded = 0, full = 0;
    for (int view : {0, 1, 2})
        for (auto size : {std::pair<int, int>{320, 224}, {1280, 720}, {7680, 1440}}) {
            auto snapshot = generated();
            auto retained = std::make_unique<World>();
            retained->setRom({});
            std::vector<uint32_t> native(320 * 224), texture;
            int textureWidth = 0, textureHeight = 0;
            for (int frame = 0; frame < 24; ++frame) {
                int cameraX = frame < 3 ? 0 : std::min(frame, 16) * 11,
                    cameraY = frame < 8 ? 0 : std::min(frame, 16);
                word(snapshot->ram, 0xF700, cameraX);
                word(snapshot->ram, 0xF704, cameraY);
                word(snapshot->ram, 0xD008, cameraX + 160);
                word(snapshot->ram, 0xD00C, cameraY + 112);
                word(snapshot->vram, 0xF000, 128 + 80 + frame % 4);
                word(snapshot->vram, 0xF002, 0);
                word(snapshot->vram, 0xF004, 5);
                word(snapshot->vram, 0xF006, 128 + 80 + frame * 3);
                if (frame == 4)
                    word(snapshot->ram, 0xB00A, 7 | 0x8000);
                if (frame == 7)
                    snapshot->vram[4 * 32 + 1] ^= 0x33;
                if (frame == 10)
                    for (int y = 0; y < 224; ++y)
                        snapshot->palettes[y][3] ^= 0x00335577;
                if (frame >= 16)
                    for (int y = 0; y < 224; ++y) {
                        snapshot->palettes[y][3] ^= 0x00335577;
                        snapshot->palettes[y][2] = frame % 2 ? 0 : 0xaaff0044;
                    }
                for (size_t i = 0; i < native.size(); ++i)
                    native[i] = (i % 29 == unsigned(frame))
                                    ? 0xffff8800
                                    : snapshot->background[i / 320][i % 320];
                Settings settings;
                settings.view = view;
                settings.camera = view == 2 ? 4 : 0;
                settings.zoom = 1;
                settings.quality = 1;
                settings.enemyPreview = false;
                settings.compositionPixels = 400000;
                settings.compositionScale = frame < 5    ? 100
                                            : frame < 9  ? 75
                                            : frame < 12 ? 35
                                                         : 100;
                auto fresh = std::make_unique<World>();
                fresh->setRom({});
                Settings ref = settings;
                require(retained->compose(*snapshot, &fixture, settings, size.first, size.second,
                                          &native),
                        "retained synthetic scene not ready");
                require(fresh->compose(*snapshot, &fixture, ref, size.first, size.second, &native),
                        "fresh synthetic scene not ready");
                require(retained->canvas.width == fresh->canvas.width &&
                            retained->canvas.height == fresh->canvas.height,
                        "adaptive dimensions changed between paths");
                require(retained->canvas.pixels == fresh->canvas.pixels,
                        "retained compositor differs from fresh full composition");
                if (frame % 8 == 0)
                    for (int mode : {1, 2, 3}) {
                        Settings volumeSettings = settings;
                        volumeSettings.voxelMode = mode;
                        volumeSettings.voxelDetail = 0;
                        volumeSettings.voxelBackground = true;
                        require(retained->prepareVolume(*snapshot, &fixture, volumeSettings, native,
                                                        320, 224, true),
                                "retained volume ready");
                        require(fresh->prepareVolume(*snapshot, &fixture, volumeSettings, native,
                                                     320, 224, true),
                                "fresh volume ready");
                        require(retained->volume.pixels == fresh->volume.pixels,
                                "volume colours differ between cached and fresh terrain");
                        require(retained->volume.hardwareLayers && !retained->volume.nativeOnly,
                                "supported terrain retains priority layers");
                        for (size_t layer = 0; layer < voxel::planeCount; ++layer) {
                            const auto &a = retained->volumeMesh.layers()[layer],
                                       &b = fresh->volumeMesh.layers()[layer];
                            require(a.opaque.size() == b.opaque.size() &&
                                        a.translucent.size() == b.translucent.size(),
                                    "cached volume topology differs");
                            if (!a.opaque.empty())
                                require(std::memcmp(a.opaque.data(), b.opaque.data(),
                                                    a.opaque.size() * sizeof(voxel::Vertex)) == 0,
                                        "cached opaque volume positions or UVs differ");
                            if (!a.translucent.empty())
                                require(std::memcmp(a.translucent.data(), b.translucent.data(),
                                                    a.translucent.size() * sizeof(voxel::Vertex)) ==
                                            0,
                                        "cached transparent volume positions or UVs differ");
                        }
                        ++volumeComparisons;
                    }
                auto &canvas = retained->canvas;
                if (textureWidth != canvas.width || textureHeight != canvas.height) {
                    texture = canvas.pixels;
                    textureWidth = canvas.width;
                    textureHeight = canvas.height;
                } else {
                    for (auto rect : retained->uploadRegions)
                        for (int y = rect.y; y < rect.y + rect.h; ++y)
                            std::copy_n(canvas.pixels.begin() + size_t(y) * canvas.width + rect.x,
                                        rect.w,
                                        texture.begin() + size_t(y) * canvas.width + rect.x);
                }
                require(texture == canvas.pixels,
                        "actual compositor damage omitted a texture update");
                for (auto r : retained->uploadRegions)
                    uploaded += size_t(r.w) * r.h * 4;
                full += retained->canvas.pixels.size() * 4;
                ++comparisons;
            }
            require(retained->terrainCacheHits > 0,
                    "synthetic scenario never exercised cache reuse");
            require(retained->paletteCacheHits > 0,
                    "palette cycle did not exercise indexed sample cache");
        }
    std::printf("PASS %d retained/full compositor comparisons: generated terrain, SAT actors, "
                "native foreground, palette/VRAM/layout changes, three views and "
                "tiny/normal/ultrawide sizes. Upload bytes %llu / full %llu.\n",
                comparisons, (unsigned long long)uploaded, (unsigned long long)full);
    {
        auto snapshot = generated();
        auto world = std::make_unique<World>();
        word(snapshot->ram, 0xB00A, 0x8007);
        // Earlier low-priority SAT sprite overlaps a later high one. Tile 5
        // has an opaque border, so one extra output column is observable.
        word(snapshot->vram, 0xF000, 128 + 80);
        word(snapshot->vram, 0xF002, 1);
        word(snapshot->vram, 0xF004, 5);
        word(snapshot->vram, 0xF006, 128 + 80);
        word(snapshot->vram, 0xF008, 128 + 80);
        word(snapshot->vram, 0xF00A, 0);
        word(snapshot->vram, 0xF00C, 0x8006);
        word(snapshot->vram, 0xF00E, 128 + 84);
        std::fill_n(snapshot->vram + 5 * 32, 32, uint8_t(0x55));
        std::fill_n(snapshot->vram + 6 * 32, 32, uint8_t(0x66));
        Settings cfg;
        cfg.view = 0;
        cfg.zoom = 1;
        cfg.voxelMode = 2;
        cfg.voxelDetail = 3;
        cfg.voxelBackground = true;
        cfg.autoPerformance = false;
        cfg.smoothMap = false;
        cfg.enemyPreview = false;
        std::vector<uint32_t> native(320 * 224, 0xFF102030);
        require(world->compose(*snapshot, &fixture, cfg, 320, 224, &native),
                "fine priority fixture ready");
        require(world->prepareVolume(*snapshot, &fixture, cfg, native, 320, 224, true),
                "fine priority volume ready");
        require(world->volume.width == 320 && world->volume.height == 224,
                "priority check uses original-pixel sampling");
        require((world->volume.at(voxel::Plane::BackgroundHigh, 10, 4) >> 24) != 0,
                "high-priority Plane B is kept ahead of low actors");
        require(world->volume.at(voxel::Plane::ActorsLow, 87, 87) == snapshot->palettes[87][5] &&
                    world->volume.at(voxel::Plane::ActorsHigh, 87, 87) == 0,
                "SAT order wins an overlapping low/high sprite texel");
        require(world->volume.at(voxel::Plane::ActorsLow, 88, 87) == 0 &&
                    world->volume.at(voxel::Plane::ActorsLow, 87, 88) == 0 &&
                    world->volume.at(voxel::Plane::ActorsHigh, 88, 87) == snapshot->palettes[87][6],
                "solid actor borders end at their exact right and lower world coordinates");
        EnemyImage image;
        image.width = 16;
        image.height = 16;
        image.pixels.assign(256, 0);
        image.pixels[8 * 16 + 8] = 7;
        world->enemies.visible.push_back({95, 88, 0, &image, SpawnKind::Boss, false, false});
        cfg.enemyPreview = true;
        cfg.enemyStyle = 0;
        cfg.voxelDetail = 0;
        require(world->prepareVolume(*snapshot, &fixture, cfg, native, 320, 224, true),
                "minified spawn preview volume ready");
        bool marker = false;
        for (int y = 0; y < world->volume.height; ++y)
            for (int x = 0; x < world->volume.width; ++x)
                marker |= (world->volume.at(voxel::Plane::Preview, x, y) >> 24) != 0;
        require(marker, "a tiny transparent-edged enemy still has a visible spawn marker");
    }
    {
        auto snapshot = generated();
        auto world = std::make_unique<World>();
        Settings cfg;
        cfg.voxelMode = 2;
        cfg.voxelDetail = 3;
        cfg.autoPerformance = false;
        cfg.projectionTransparent = true;
        std::vector<uint32_t> pixels(320 * 224);
        for (size_t i = 0; i < pixels.size(); ++i)
            pixels[i] = 0xFF000000 | uint32_t(i * 0x12345) & 0xFFFFFF;
        auto original = pixels;
        const auto before = std::make_unique<SCUSnapshot>(*snapshot);
        require(world->prepareVolume(*snapshot, nullptr, cfg, pixels, 320, 224, false),
                "unprofiled native screen still supports pixel relief");
        require(world->volume.nativeOnly && !world->volume.hardwareLayers,
                "native fallback never invents actor planes");
        require(world->volume.width == 320 && world->volume.height == 224,
                "fine native detail preserves every source pixel");
        for (int y = 0; y < 224; ++y)
            for (int x = 0; x < 320; ++x)
                require(world->volume.at(voxel::Plane::TerrainLow, x, y) == pixels[y * 320 + x],
                        "native voxel texture changed original pixels");
        require(pixels == original &&
                    std::memcmp(before.get(), snapshot.get(), sizeof *before) == 0,
                "volume generation changed snapshot or native source");
        const auto epoch = world->volume.revision;
        cfg.voxelMode = 0;
        require(!world->prepareVolume(*snapshot, nullptr, cfg, pixels, 320, 224, false) &&
                    world->volume.pixels.capacity() == 0,
                "flat mode releases the retained voxel atlas");
        cfg.voxelMode = 2;
        pixels[0] ^= 0x00FFFFFF;
        require(world->prepareVolume(*snapshot, nullptr, cfg, pixels, 320, 224, false) &&
                    world->volume.revision > epoch,
                "new volume cannot reuse an old texture cache key");
    }
    std::puts("No cartridge, emulator, SDL, window, app or device execution; presentation-only "
              "code with generated RAM/VRAM/palette data.");
    std::printf("PASS %d additional cached/fresh priority-aware procedural volume comparisons.\n",
                volumeComparisons);
}
