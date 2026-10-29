#include "world.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#ifdef _OPENMP
#include <omp.h>
#endif
namespace scu {
static int clampi(int v, int a, int b) {
    return std::max(a, std::min(b, v));
}
static uint64_t hashTerrain(uint64_t hash, const void *data, size_t bytes) {
    auto p = static_cast<const uint8_t *>(data);
    for (; bytes >= 8; bytes -= 8, p += 8) {
        uint64_t word;
        std::memcpy(&word, p, 8);
        hash = (hash ^ word) * 1099511628211ull;
    }
    while (bytes--)
        hash = (hash ^ *p++) * 1099511628211ull;
    return hash;
}
template <typename Sample>
static uint32_t filteredPixel(double wx, double wy, double dx, double dy, bool smooth,
                              const Sample &sample) {
    if (!smooth)
        return sample(int(std::floor(wx)), int(std::floor(wy)));
    unsigned r = 0, g = 0, b = 0, alpha = 0, count = 0;
    for (double oy : {0.25, 0.75})
        for (double ox : {0.25, 0.75}) {
            uint32_t color = sample(int(std::floor(wx + ox * dx)), int(std::floor(wy + oy * dy)));
            if (color >> 24) {
                r += (color >> 16) & 255;
                g += (color >> 8) & 255;
                b += color & 255;
                alpha += color >> 24;
                ++count;
            }
        }
    return count ? ((alpha / 4) << 24) | ((r / count) << 16) | ((g / count) << 8) | (b / count) : 0;
}
Scene World::inspect(const SCUSnapshot &s, const Game *g) const {
    Scene v;
    if (!g)
        return v;
    auto rw = [&](size_t a) -> unsigned {
        return a + 1 < rom.size() ? (unsigned(rom[a]) << 8) | rom[a + 1] : 0;
    };
    auto rl = [&](size_t a) { return (rw(a) << 16) | rw(a + 2); };
    auto ramLong = [&](unsigned a) {
        return (unsigned(be16(s.ram, a)) << 16) | be16(s.ram, a + 2);
    };
    if (g->crc == 0x677206CB) {
        v.family = 4;
        unsigned player = be16(s.ram, 0x5736);
        if (player >= 4)
            return v;
        v.stage = be16(s.ram, 0x5788 + player * 76);
        if (v.stage > 3 || rom.size() != 1048576)
            return v;
        v.width = rw(0xBFC70 + v.stage * 4);
        v.height = rw(0xBFC72 + v.stage * 4);
        v.cameraX = be16(s.ram, 8);
        v.cameraY = be16(s.ram, 10);
        v.playerX = be16(s.ram, 0x5766);
        v.playerY = be16(s.ram, 0x576A);
        unsigned layout = ramLong(0x3CAC);
        v.ready = (s.regs[1] & 64) && v.width >= 320 && v.width <= 2048 && v.height == 2048 &&
                  be16(s.ram, 0x5748) == v.width - 320 && be16(s.ram, 0x55A2) == v.height - 224 &&
                  (layout == rl(0xBFC00 + v.stage * 4) || layout == rl(0xBFC10 + v.stage * 4));
        return v;
    }
    if (g->crc == 0x44A2CA44) {
        v.family = 5;
        v.stage = be16(s.ram, 0x0678);
        if (v.stage < 1 || v.stage > 22 || rom.size() != 4194304)
            return v;
        unsigned h = 0x1AB62 + (v.stage - 1) * 42;
        v.width = 8192;
        v.height = 4096;
        v.cameraX = be16(s.ram, 0x084E);
        v.cameraY = be16(s.ram, 0x0850);
        int x = (int16_t)be16(s.ram, 0xC1EC), z = (int16_t)be16(s.ram, 0xC1F4);
        v.playerX = x - z + 4096;
        v.playerY = (x + z) / 2 - (int16_t)be16(s.ram, 0xC1F0);
        v.ready = (s.regs[1] & 64) && ramLong(0x09EC) == rl(h + 12) &&
                  ramLong(0x09F4) == rl(h + 20) && ramLong(0x09F8) == rl(h + 24);
        return v;
    }
    v.family = g->family;
    auto &r = s.ram;
    int mode = r[0xF600] & 127;
    // Sonic Classics banks the original S1/S2 programs into the cartridge
    // window. Their installed RAM layouts can be observed independently of
    // the menu and Mean Bean engine. Sprite art comes from the hardware SAT.
    if (g->crc == 0x95B5E8D7 && (mode == 8 || mode == 12))
        v.family = r[0xD000] == 1 ? 1 : (r[0xB000] == 1 || r[0xB000] == 2) ? 2 : 0;
    if (!v.family)
        return v;
    v.stage = be16(r, 0xFE10);
    if (mode != 8 && mode != 12)
        return v;
    if (v.family == 1) {
        if (r[0xD000] != 1 || r[0xD024] >= 6)
            return v;
        v.cameraX = be16(r, 0xF700);
        v.cameraY = (int16_t)be16(r, 0xF704);
        v.playerX = be16(r, 0xD008);
        v.playerY = be16(r, 0xD00C);
        int cols = 0, rows = 0;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 64; x++)
                if (r[0xA400 + y * 128 + x]) {
                    cols = std::max(cols, x + 1);
                    rows = std::max(rows, y + 1);
                }
        v.width = cols * 256;
        v.height = rows * 256;
    } else if (v.family == 2) {
        // The Knuckles patch uses bit 15 here as its lock-on flag.
        unsigned competition = be16(r, 0xFFD8);
        if (g->crc == 0x2AC1E7C6)
            competition &= 0x7FFF;
        if ((r[0xB000] != 1 && r[0xB000] != 2) || competition)
            return v;
        v.cameraX = be16(r, 0xEEF0);
        v.cameraY = (int16_t)be16(r, 0xEEF4);
        v.playerX = be16(r, 0xB008);
        v.playerY = be16(r, 0xB00C);
        int cols = 0, rows = 0;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 128; x++)
                if (r[0x8000 + y * 256 + x]) {
                    cols = std::max(cols, x + 1);
                    rows = std::max(rows, y + 1);
                }
        v.width = cols * 128;
        v.height = rows * 128;
    } else {
        if (!be16(r, 0xB000) && !be16(r, 0xB002))
            return v;
        if (be16(r, g->crc == 0x9BC192CE ? 0xFFD8 : 0xFFE8))
            return v;
        v.cameraX = be16(r, 0xEE80);
        v.cameraY = (int16_t)be16(r, 0xEE84);
        v.playerX = be16(r, 0xB010);
        v.playerY = be16(r, 0xB014);
        v.width = be16(r, 0x8000) * 128;
        v.height = be16(r, 0x8004) * 128;
        auto row = be16(r, 0x8008);
        if (row < 0x8088 || row >= 0x9000)
            return v;
        if (v.height < 128 || v.height > 4096)
            v.height = 2048;
    }
    v.ready = v.width >= 320 && v.width <= 32768 && v.height >= 128 && v.height <= 4096;
    // Cinematic transitions retain the native framebuffer until the stage is installed.
    if (r[0xF600] & 128)
        v.ready = false;
    return v;
}
uint16_t World::attribute(const SCUSnapshot &s, int family, int x, int y, bool bg) const {
    if (family >= 4)
        return atlas.sample(x, y) & 63;
    auto r = s.ram;
    if (x < 0 || x >= 32768 || y < 0 || y >= (family == 3 ? 4096 : 2048))
        return 0;
    uint16_t block = 0;
    unsigned bank;
    if (family == 1) {
        if (x >= 16384)
            return 0;
        unsigned l = (bg ? 0xA440 : 0xA400) + (y >> 8) * 128 + (x >> 8), chunk = r[l];
        if (!chunk)
            return 0;
        unsigned a = ((chunk - 1) & 127) * 512 + (y & 240) * 2 + (x & 240) / 8;
        block = be16(r, a);
        bank = 0xB000;
    } else {
        unsigned cell;
        if (family == 2) {
            if (x >= 16384)
                return 0;
            cell = (bg ? 0x8080 : 0x8000) + (y >> 7) * 256 + (x >> 7);
        } else {
            unsigned row = 0x8008 + (y >> 7) * 4 + (bg ? 2 : 0);
            cell = be16(r, row) + (x >> 7);
            if (cell < 0x8088 || cell >= 0x9000)
                return 0;
        }
        unsigned chunk = r[cell];
        block = be16(r, chunk * 128 + (y & 112) + (x & 112) / 8);
        bank = 0x9000;
    }
    int tx = (x >> 3) & 1, ty = (y >> 3) & 1;
    unsigned flip = family == 1 ? (block & 0x1800) : ((block & 0xC00) << 1);
    if (flip & 0x800)
        tx ^= 1;
    if (flip & 0x1000)
        ty ^= 1;
    unsigned a = bank + (block & 1023) * 8 + (ty * 2 + tx) * 2;
    if (family != 1 && a >= 0xA800)
        return 0;
    return be16(r, a) ^ flip;
}
uint8_t World::read(unsigned a) const {
    a &= 0xFFFFFF;
    if (a < rom.size())
        return rom[a];
    if (a >= 0xFF0000 && snap)
        return snap->ram[a & 65535];
    return 0;
}
uint16_t World::word(unsigned a) const {
    return (uint16_t)((read(a) << 8) | read(a + 1));
}
uint32_t World::longword(unsigned a) const {
    return (uint32_t(word(a)) << 16) | word(a + 2);
}
void World::mapping(unsigned addr, unsigned frame, unsigned gfx, unsigned flags, int x, int y,
                    bool direct) {
    unsigned p = addr;
    int count = 1, family = lastFamily;
    if (!direct) {
        p += (int16_t)word(p + (frame & 127) * 2);
        count = family == 1 ? read(p++) : word(p);
        if (family != 1)
            p += 2;
    }
    if (count < 0 || count > 128 || sprites.size() > 16000)
        return;
    int length = family == 1 ? 5 : family == 2 ? 8 : 6;
    for (int n = 0; n < count; n++, p += length) {
        int dy = (int8_t)read(p), sz = read(p + 1) & 15,
            dx = family == 1 ? (int8_t)read(p + 4) : (int16_t)word(p + (family == 2 ? 6 : 4));
        uint16_t attr = word(p + 2) + (uint16_t)gfx;
        if (flags & 1) {
            dx = -dx - (((sz >> 2) & 3) + 1) * 8;
            attr ^= 0x800;
        }
        if (flags & 2) {
            dy = -dy - ((sz & 3) + 1) * 8;
            attr ^= 0x1000;
        }
        sprites.push_back({x + dx, y + dy, attr, (uint8_t)sz});
    }
}
void World::captureActors(const Scene &v, const Game *g) {
    sprites.clear();
    auto r = snap->ram;
    // BuildSprites empties the game queues before the completed-frame snapshot.
    // The published hardware SAT is the authoritative visible actor list.
    unsigned satBase = (snap->regs[5] & ((snap->regs[12] & 1) ? 0x7E : 0x7F)) * 512u, index = 0;
    std::vector<Sprite> satSprites;
    for (unsigned n = 0; n < 80; n++) {
        unsigned p = (satBase + index * 8) & 65535;
        int y = (be16(snap->vram, p) & 511) - 128;
        uint16_t shape = be16(snap->vram, p + 2), a = be16(snap->vram, p + 4);
        int x = (be16(snap->vram, p + 6) & 511) - 128;
        unsigned tile = a & 2047;
        bool hud = v.family == 4   ? y < 16
                   : v.family == 5 ? y < 32 || y >= 196
                                   : (tile >= 0x6CA && tile < 0x704) ||
                                         (x < 100 && y >= 192 && tile >= 0x7D4 && tile < 0x7E8);
        if (!hud)
            satSprites.push_back({x + v.cameraX, y + v.cameraY, a, (uint8_t)((shape >> 8) & 15)});
        index = shape & 127;
        if (!index)
            break;
    }
    for (auto it = satSprites.rbegin(); it != satSprites.rend(); ++it)
        sprites.push_back(*it);
    if (v.family >= 4 || !g->family)
        return;
    // Read the game's priority queues. The hardware/CPU keeps native spawning and collision.
    for (int pri = 7; pri >= 0 && satSprites.empty(); pri--) {
        unsigned q = 0xAC00 + pri * 128, n = be16(r, q);
        if (n > 126)
            continue;
        for (int k = (int)n; k >= 2; k -= 2) {
            unsigned o = be16(r, q + k);
            if (o < (v.family == 1 ? 0xD000 : 0xB000) || o >= 0xF000)
                continue;
            if (v.family < 3) {
                if (!r[o] || r[o] == 0x21)
                    continue;
                unsigned flags = r[o + 1];
                if (!(flags & 12) && !(flags & 64))
                    continue;
                int x = be16(r, o + 8), y = be16(r, o + 12);
                unsigned map = longword(0xFF0000 + o + 4), gfx = be16(r, o + 2);
                if (v.family == 2 && (flags & 64)) {
                    if (r[o + 0xB])
                        mapping(map, r[o + 0xB], gfx, flags, x, y);
                    int count = std::min(8, (int)r[o + 0xF]);
                    for (int n = 0; n < count; n++) {
                        unsigned child = o + 0x10 + n * 6;
                        mapping(map, r[child + 5], gfx, flags, be16(r, child), be16(r, child + 2));
                    }
                } else
                    mapping(map, r[o + 0x1A], gfx, flags, x, y, (flags & 32) != 0);
            } else {
                if (!longword(0xFF0000 + o))
                    continue;
                unsigned flags = r[o + 4];
                if (!(flags & 4))
                    continue;
                unsigned map = longword(0xFF0000 + o + 0xC), gfx = be16(r, o + 0xA);
                int x = be16(r, o + 0x10), y = be16(r, o + 0x14);
                if (flags & 64) {
                    if (r[o + 0x22])
                        mapping(map, r[o + 0x22], gfx, flags, x, y);
                    unsigned count = std::min(8, (int)be16(r, o + 0x16));
                    for (unsigned n = 0; n < count; n++) {
                        unsigned c = o + 0x18 + n * 6;
                        mapping(map, r[c + 5], gfx, flags, be16(r, c), be16(r, c + 2));
                    }
                } else
                    mapping(map, r[o + 0x22], gfx, flags, x, y, (flags & 32) != 0);
            }
        }
    }
    // Rings are static placements/status tables, so the overview can show them
    // without spawning distant enemies or changing the cartridge's object pool.
    if (v.family == 1) {
        unsigned index = ((v.stage & 0xFF00) >> 4) | ((v.stage & 3) << 2),
                 base = 0x6B000 + (int16_t)word(0x6B000 + index), state = 1;
        static const int distances[16][2] = {
            {16, 0},  {24, 0},   {32, 0},   {0, 16},   {0, 24}, {0, 32},  {16, 16}, {24, 24},
            {32, 32}, {-16, 16}, {-24, 24}, {-32, 32}, {16, 8}, {24, 16}, {-16, 8}, {-24, 16}};
        for (unsigned p = base, n = 0; n < 1024 && p + 6 < rom.size(); n++, p += 6) {
            unsigned x = word(p);
            if (x == 65535)
                break;
            unsigned id = read(p + 4), st = (id & 128) ? state++ : 0;
            if ((id & 127) != 0x25)
                continue;
            int y = word(p + 2) & 0xFFF, sub = read(p + 5), count = std::min(7, (sub & 7) + 1),
                dir = (sub >> 4) & 15;
            for (int k = 0; k < count; k++)
                if (!(r[0xFC02 + (st & 255)] & (1 << k)))
                    mapping(0x9F92, r[0xFEC3] & 3, 0x27B2, 0, x + distances[dir][0] * k,
                            y + distances[dir][1] * k);
        }
    } else if (v.family == 2) {
        unsigned maps = g && g->crc == 0x2AC1E7C6 ? 0x21736A : 0x1736A;
        for (unsigned p = 0xE806; p + 6 < 0xF600; p += 6) {
            int x = be16(r, p + 2);
            if (x == 65535)
                break;
            if (!x)
                continue;
            if (be16(r, p) & 0x8000)
                continue;
            unsigned frame = r[p + 1] ? r[p + 1] : r[0xFEA3],
                     map = maps + (int16_t)word(maps + frame * 2);
            mapping(map, 0, 0x26BC, 0, x, be16(r, p + 4), true);
        }
    } else {
        unsigned table = g && g->crc == 0x9BC192CE ? 0x5E198 : 0x1E3E58,
                 map = g && g->crc == 0x9BC192CE ? 0xF87E : 0xEBEE;
        unsigned base = longword(table + (r[0xFE10] * 8) + (r[0xFE11] & 1) * 4);
        if (base < rom.size())
            for (unsigned p = base + 4, n = 1; n < 512 && p + 4 < rom.size(); n++, p += 4) {
                unsigned x = word(p);
                if (x == 65535)
                    break;
                if (be16(r, 0xE700 + n * 2) & 0x8000)
                    continue;
                unsigned frame = r[0xE701 + n * 2] ? r[0xE701 + n * 2] : r[0xFEB3],
                         m = map + (frame & 7) * 8;
                uint16_t attr = word(m + 4);
                sprites.push_back({(int)x + (int16_t)word(m + 6),
                                   (int)word(p + 2) + (int16_t)word(m), attr,
                                   (uint8_t)word(m + 2)});
            }
    }
}
void World::freeze(const Scene &, Settings &s) {
    s.left = canvas.left;
    s.top = canvas.top;
    s.camera = 2;
    cameraValid = true;
}
bool World::compose(const SCUSnapshot &s, const Game *g, Settings &cfg, int dw, int dh,
                    const std::vector<uint32_t> *native, int nw, int nh) {
    enemySpriteCount = enemyMarkerCount = 0;
    Scene v = inspect(s, g);
    if (!v.ready || cfg.view == 4 || (v.family >= 4 && !cfg.mapExtension)) {
        if (v.family >= 4 && !cfg.mapExtension)
            atlas.clear();
        enemies.visible.clear();
        return false;
    }
    snap = &s;
    if (v.family >= 4 && !atlas.prepare(rom, s, v.family, v.stage))
        return false;
    if (lastStage != v.stage || lastFamily != v.family) {
        lastStage = v.stage;
        lastFamily = v.family;
        cameraValid = false;
    }
    int w, h;
    double coverW = 0, coverH = 0;
    bool overview = cfg.view == 2;
    if (overview && cfg.fixedMapWidth > 0 && cfg.fixedMapHeight > 0) {
        v.width = cfg.fixedMapWidth;
        v.height = cfg.fixedMapHeight;
    }
    if (cfg.enemyPreview && g->family && v.family <= 3) {
        enemies.update(s, g, rom, v.stage, v.cameraX, v.cameraY, cfg.bossPreview,
                       cfg.scriptPreview);
        if (overview && cfg.fitSpawns)
            for (const auto &p : enemies.visible) {
                v.width = std::min(32768, std::max(v.width, p.x + 96));
                v.height = std::min(4096, std::max(v.height, p.y + 96));
            }
    } else
        enemies.visible.clear();
    if (cfg.view == 1) {
        coverH = clampi(cfg.panoramaHeight, 224, 4096);
        coverW = double(dw) * coverH / std::max(1, dh);
        w = std::max(1, std::min(dw, int(std::ceil(coverW))));
        h = std::max(1, std::min(dh, int(coverH)));
    } else if (overview) {
        double scale = std::min(double(dw) / v.width, double(dh) / v.height);
        coverW = dw / scale;
        coverH = dh / scale;
        int sample = cfg.quality == 0 ? 3 : cfg.quality == 1 ? 2 : 1;
        w = std::max(1, dw / sample);
        h = std::max(1, dh / sample);
    } else {
        double scale = cfg.pixelScale();
        coverW = dw / scale;
        coverH = dh / scale;
        w = std::max(1, std::min(dw, int(std::ceil(coverW))));
        h = std::max(1, std::min(dh, int(std::ceil(coverH))));
    }
    // Memory/texture bounds are limits on output dimensions, not aspect ratio.
    double cap = std::min({1.0, 16384.0 / w, 8192.0 / h, std::sqrt(33554432.0 / (double(w) * h))});
    w = std::max(1, int(w * cap));
    h = std::max(1, int(h * cap));
    canvas.coverageW = coverW;
    canvas.coverageH = coverH;
    canvas.width = w;
    canvas.height = h;
    canvas.scale = std::min(float(dw) / w, float(dh) / h);
    if (!cameraValid) {
        cfg.left = int(v.playerX - coverW / 2);
        cfg.top = int(v.playerY - coverH / 2);
        cameraValid = true;
    }
    if (overview || cfg.camera == 4) {
        canvas.left = (int)((v.width - coverW) / 2);
        canvas.top = (int)((v.height - coverH) / 2);
    } else if (cfg.camera == 2 || cfg.camera == 3) {
        canvas.left = cfg.left;
        canvas.top = cfg.top;
    } else {
        canvas.left = int(v.playerX - coverW / 2);
        canvas.top = int(v.playerY - coverH / 2);
        if (cfg.camera == 0) {
            canvas.left = v.width < coverW ? int((v.width - coverW) / 2)
                                           : clampi(canvas.left, 0, int(v.width - coverW));
            canvas.top = v.height < coverH ? int((v.height - coverH) / 2)
                                           : clampi(canvas.top, 0, int(v.height - coverH));
        }
    }
    for (unsigned t = 0; t < 2048; t++) {
        if (patternsValid && !std::memcmp(patternSource.data() + t * 32, s.vram + t * 32, 32))
            continue;
        std::memcpy(patternSource.data() + t * 32, s.vram + t * 32, 32);
        visibleTile[t] = false;
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                int b = s.vram[t * 32 + y * 4 + x / 2];
                patterns[t][y * 8 + x] = (uint8_t)((x & 1) ? b & 15 : b >> 4);
                visibleTile[t] = visibleTile[t] || patterns[t][y * 8 + x] != 0;
            }
    }
    patternsValid = true;
    uint64_t shape = 1469598103934665603ull;
    for (int value : {v.family, v.stage, v.width, v.height, w, h, canvas.left, canvas.top,
                      v.cameraY, cfg.view, cfg.overviewStyle, int(cfg.smoothMap)})
        shape = hashTerrain(shape, &value, sizeof value);
    shape = hashTerrain(shape, &coverW, sizeof coverW);
    shape = hashTerrain(shape, &coverH, sizeof coverH);
    if (v.family <= 3) {
        shape = hashTerrain(shape, s.ram, 0xA800);
        if (v.family == 1)
            shape = hashTerrain(shape, s.ram + 0xB000, 0x1800);
        if (shape != shapeSignature) {
            terrainUsed.fill(false);
            // Enumerate tile references once per geometry/camera change, rather
            // than hashing animated character graphics or SAT positions.
            int x0 = std::max(0, canvas.left & ~7), y0 = std::max(0, canvas.top & ~7);
            int x1 = std::min(v.width, int(std::ceil(canvas.left + coverW))),
                y1 = std::min(v.height, int(std::ceil(canvas.top + coverH)));
            for (int y = y0; y < y1; y += 8)
                for (int x = x0; x < x1; x += 8)
                    terrainUsed[attribute(s, v.family, x, y) & 2047] = true;
        }
    } else {
        auto version = atlas.version();
        shape = hashTerrain(shape, &version, sizeof version);
    }
    shapeSignature = shape;
    bool schematic =
        overview && (cfg.overviewStyle == 2 || (cfg.overviewStyle == 0 && v.family == 3));
    uint64_t signature = schematic ? shape : hashTerrain(shape, s.palettes, sizeof s.palettes);
    if (v.family <= 3)
        for (unsigned tile = 0; tile < 2048; ++tile)
            if (terrainUsed[tile])
                signature = schematic ? hashTerrain(signature, &visibleTile[tile], sizeof(bool))
                                      : hashTerrain(signature, s.vram + tile * 32, 32);
    bool terrainCached = signature == terrainSignature && terrainPixels.size() == size_t(w) * h;
    if (terrainCached) {
        canvas.pixels = terrainPixels;
        priority = terrainPriority;
        ++terrainCacheHits;
    } else {
        canvas.pixels.assign(size_t(w) * h, 0);
        priority.assign(size_t(w) * h, 0);
        ++terrainCacheMisses;
    }
    int workers = cfg.workers;
#ifdef _OPENMP
    if (!workers)
        workers = std::max(1, std::min(8, omp_get_num_procs() - 1));
    omp_set_num_threads(workers);
#endif
    if (!terrainCached) {
        int xBegin = clampi(int(std::floor(-canvas.left * w / coverW)) - 1, 0, w),
            xEnd = clampi(int(std::ceil((v.width - canvas.left) * w / coverW)) + 1, 0, w),
            yBegin = clampi(int(std::floor(-canvas.top * h / coverH)) - 1, 0, h),
            yEnd = clampi(int(std::ceil((v.height - canvas.top) * h / coverH)) + 1, 0, h);
        if (v.family >= 4) {
            bool smooth = !schematic && cfg.smoothMap && (coverW > w * 1.01 || coverH > h * 1.01);
            int row;
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (w * h > 500000)
#endif
            for (row = yBegin; row < yEnd; ++row) {
                int wy = canvas.top + int(row * coverH / h);
                auto pal = s.palettes[clampi(wy - v.cameraY, 0, 223)];
                for (int x = xBegin; x < xEnd; ++x) {
                    int wx = canvas.left + int(x * coverW / w);
                    uint8_t p = atlas.sample(wx, wy);
                    size_t at = size_t(row) * w + x;
                    auto sample = [&](int xx, int yy) -> uint32_t {
                        unsigned index = atlas.sample(xx, yy) & 63;
                        return index ? (schematic ? 0xD03F8C9D : pal[index]) : 0;
                    };
                    canvas.pixels[at] =
                        filteredPixel(canvas.left + x * coverW / w, canvas.top + row * coverH / h,
                                      coverW / w, coverH / h, smooth, sample);
                    priority[at] = p >> 7;
                }
            }
        } else if (overview || std::abs(coverW - w) > 0.01 || std::abs(coverH - h) > 0.01) {
            bool map = schematic;
            bool smooth = !map && cfg.smoothMap && (coverW > w * 1.01 || coverH > h * 1.01);
            int row;
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (w * h > 500000)
#endif
            for (row = yBegin; row < yEnd; row++) {
                int wy = canvas.top + (int)(row * coverH / h),
                    nativeLine = clampi(wy - v.cameraY, 0, 223);
                auto pal = s.palettes[nativeLine];
                for (int x = xBegin; x < xEnd; x++) {
                    int wx = canvas.left + (int)(x * coverW / w);
                    uint16_t a = attribute(s, v.family, wx, wy);
                    auto sample = [&](int xx, int yy) -> uint32_t {
                        unsigned attr = attribute(s, v.family, xx, yy);
                        if (!attr)
                            return 0;
                        if (map)
                            return (attr & 2047) && visibleTile[attr & 2047] ? 0xD03F8C9D : 0;
                        int ix = (xx & 7) ^ ((attr & 0x800) ? 7 : 0),
                            iy = (yy & 7) ^ ((attr & 0x1000) ? 7 : 0);
                        unsigned p = patterns[attr & 2047][iy * 8 + ix];
                        return p ? pal[((attr >> 9) & 48) | p] : 0;
                    };
                    size_t at = (size_t)row * w + x;
                    canvas.pixels[at] =
                        filteredPixel(canvas.left + x * coverW / w, canvas.top + row * coverH / h,
                                      coverW / w, coverH / h, smooth, sample);
                    priority[at] = (a & 0x8000) ? 1 : 0;
                }
            }
        } else {
            int firstY = std::max(0, canvas.top & ~7) - canvas.top,
                firstX = std::max(0, canvas.left & ~7) - canvas.left;
            int lastY = std::min(h, v.height - canvas.top),
                lastX = std::min(w, v.width - canvas.left);
            int rows = std::max(0, (lastY - firstY + 7) / 8), row;
#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (w * h > 1000000)
#endif
            for (row = 0; row < rows; row++) {
                int oy = firstY + row * 8, wy = canvas.top + oy;
                for (int ox = firstX; ox < lastX; ox += 8) {
                    int wx = canvas.left + ox;
                    uint16_t a = attribute(s, v.family, wx, wy);
                    if (!a)
                        continue;
                    auto &tile = patterns[a & 2047];
                    int fx = (a & 0x800) ? 7 : 0, fy = (a & 0x1000) ? 7 : 0, pi = (a >> 9) & 48;
                    for (int py = std::max(0, -oy); py < 8 && oy + py < h; py++) {
                        int ny = clampi(wy + py - v.cameraY, 0, 223);
                        auto pal = s.palettes[ny];
                        size_t offset = (size_t)(oy + py) * w;
                        for (int px = std::max(0, -ox); px < 8 && ox + px < w; px++) {
                            int p = tile[(py ^ fy) * 8 + (px ^ fx)];
                            if (p) {
                                auto i = offset + ox + px;
                                canvas.pixels[i] = pal[pi | p];
                                priority[i] = (a & 0x8000) ? 1 : 0;
                            }
                        }
                    }
                }
            }
        }
        terrainPixels = canvas.pixels;
        terrainPriority = priority;
        terrainSignature = signature;
    }
    captureActors(v, g);
    for (auto &q : sprites) {
        int sw = (((q.size >> 2) & 3) + 1) * 8, sh = ((q.size & 3) + 1) * 8;
        double screenX = (q.x - canvas.left) * w / coverW,
               screenY = (q.y - canvas.top) * h / coverH;
        int x = (int)std::floor(screenX), y = (int)std::floor(screenY),
            outW = (int)std::ceil(sw * w / coverW), outH = (int)std::ceil(sh * h / coverH);
        if (x >= w || y >= h || x + outW <= 0 || y + outH <= 0)
            continue;
        for (int py = std::max(0, -y); py < outH && y + py < h; py++) {
            int rawY = clampi((int)((y + py - screenY) * coverH / h), 0, sh - 1),
                iy = (q.attr & 0x1000) ? sh - 1 - rawY : rawY;
            int native = clampi(q.y + rawY - v.cameraY, 0, 223);
            auto pal = s.palettes[native];
            for (int px = std::max(0, -x); px < outW && x + px < w; px++) {
                int rawX = clampi((int)((x + px - screenX) * coverW / w), 0, sw - 1),
                    ix = (q.attr & 0x800) ? sw - 1 - rawX : rawX;
                unsigned t = ((q.attr & 2047) + (ix / 8) * (sh / 8) + iy / 8) & 2047;
                int color = patterns[t][(iy & 7) * 8 + (ix & 7)];
                auto at = (size_t)(y + py) * w + x + px;
                if (color && ((q.attr & 0x8000) || !priority[at]))
                    canvas.pixels[at] = pal[((q.attr >> 9) & 48) | color];
            }
        }
    }
    // Keep the cartridge's exact visible foreground and actors, including
    // raster effects, fine streamed edits and stage-transition compositions.
    // Plane B stays separately styled. Distant terrain uses the stage decoder.
    if (native && nw == 320 && nh == 224 && native->size() == 320 * 224) {
        std::array<uint8_t, 320 * 224> hudMask{};
        if (v.family >= 4) {
            int top = v.family == 4 ? 16 : 32, bottom = v.family == 5 ? 196 : 224;
            for (int y = 0; y < 224; ++y)
                if (y < top || y >= bottom)
                    std::fill(hudMask.begin() + y * 320, hudMask.begin() + (y + 1) * 320, 1);
        }
        unsigned satBase = (s.regs[5] & ((s.regs[12] & 1) ? 0x7E : 0x7F)) * 512, index = 0;
        for (unsigned n = 0; n < 80; n++) {
            unsigned a = (satBase + index * 8) & 65535;
            int y = (be16(s.vram, a) & 511) - 128, x = (be16(s.vram, a + 6) & 511) - 128;
            uint16_t shape = be16(s.vram, a + 2), attr = be16(s.vram, a + 4);
            int tile = attr & 2047, sw = (((shape >> 10) & 3) + 1) * 8,
                sh = (((shape >> 8) & 3) + 1) * 8;
            if (v.family <= 3 && ((tile >= 0x6C0 && tile < 0x704) ||
                                  (x < 100 && y >= 192 && tile >= 0x7D4 && tile < 0x7E8))) {
                for (int yy = std::max(0, -y); yy < sh && y + yy < 224; yy++)
                    for (int xx = std::max(0, -x); xx < sw && x + xx < 320; xx++) {
                        int ix = (attr & 0x800) ? sw - 1 - xx : xx,
                            iy = (attr & 0x1000) ? sh - 1 - yy : yy,
                            t = (tile + (ix / 8) * (sh / 8) + iy / 8) & 2047;
                        if (patterns[t][(iy & 7) * 8 + (ix & 7)])
                            hudMask[(y + yy) * 320 + x + xx] = 1;
                    }
            }
            index = shape & 127;
            if (!index)
                break;
        }
        int x0 = (int)std::floor((v.cameraX - canvas.left) * w / coverW),
            x1 = (int)std::ceil((v.cameraX + 320 - canvas.left) * w / coverW),
            y0 = (int)std::floor((v.cameraY - canvas.top) * h / coverH),
            y1 = (int)std::ceil((v.cameraY + 224 - canvas.top) * h / coverH);
        for (int y = std::max(0, y0); y < std::min(h, y1); y++) {
            int ny = clampi((int)(canvas.top + y * coverH / h - v.cameraY), 0, 223);
            for (int x = std::max(0, x0); x < std::min(w, x1); x++) {
                int nx = clampi((int)(canvas.left + x * coverW / w - v.cameraX), 0, 319);
                auto original = (*native)[ny * 320 + nx];
                if (!hudMask[ny * 320 + nx] && (v.family >= 4 || original != s.background[ny][nx]))
                    canvas.pixels[(size_t)y * w + x] = original;
            }
        }
    }
    if (cfg.enemyPreview) {
        unsigned alpha = std::clamp(cfg.enemyOpacity, 10, 80) * 255 / 100;
        auto blend = [&](int x, int y, uint32_t color) {
            if (x < 0 || x >= w || y < 0 || y >= h)
                return;
            auto &destination = canvas.pixels[size_t(y) * w + x];
            unsigned retained = (destination >> 24) * (255 - alpha) / 255,
                     combined = alpha + retained;
            uint32_t out = combined << 24;
            for (unsigned shift : {0u, 8u, 16u}) {
                unsigned value =
                    (((color >> shift) & 255) * alpha + ((destination >> shift) & 255) * retained) /
                    combined;
                out |= value << shift;
            }
            destination = out;
        };
        for (auto &p : enemies.visible) {
            uint32_t tint = p.kind == SpawnKind::Boss       ? 0xFFFF8194
                            : p.kind == SpawnKind::Scripted ? 0xFFC18CFF
                                                            : 0xFFFFCA75;
            bool drawn = false;
            if (p.image && cfg.enemyStyle != 2) {
                auto &image = *p.image;
                int left = (p.flags & 1) ? -image.left - image.width : image.left,
                    top = (p.flags & 2) ? -image.top - image.height : image.top;
                double screenX = (p.x + left - canvas.left) * w / coverW,
                       screenY = (p.y + top - canvas.top) * h / coverH;
                int x0 = int(std::floor(screenX)), y0 = int(std::floor(screenY)),
                    x1 = int(std::ceil(screenX + image.width * w / coverW)),
                    y1 = int(std::ceil(screenY + image.height * h / coverH));
                for (int y = std::max(0, y0); y < std::min(h, y1); ++y)
                    for (int x = std::max(0, x0); x < std::min(w, x1); ++x) {
                        int ix = clampi(int((x - screenX) * coverW / w), 0, image.width - 1),
                            iy = clampi(int((y - screenY) * coverH / h), 0, image.height - 1);
                        if (p.flags & 1)
                            ix = image.width - 1 - ix;
                        if (p.flags & 2)
                            iy = image.height - 1 - iy;
                        unsigned color = image.pixels[size_t(iy) * image.width + ix];
                        if (color) {
                            uint32_t pixel = s.palettes[clampi(p.y - v.cameraY, 0, 223)][color];
                            if (cfg.spawnTint && p.kind != SpawnKind::Enemy) {
                                uint32_t tinted = 0xFF000000;
                                for (unsigned shift : {0u, 8u, 16u})
                                    tinted |=
                                        ((((pixel >> shift) & 255) * 2 + ((tint >> shift) & 255)) /
                                         3)
                                        << shift;
                                pixel = tinted;
                            }
                            blend(x, y, pixel);
                            drawn = true;
                        }
                    }
                if (drawn)
                    ++enemySpriteCount;
            }
            // A small, original marker makes a spawn visible even when a full
            // stage reduces its sprite below a few pixels or art is unavailable.
            if (cfg.enemyStyle != 1 || !p.image || !drawn) {
                int x = int(std::lround((p.x - canvas.left) * w / coverW)),
                    y = int(std::lround((p.y - canvas.top) * h / coverH)),
                    radius = std::max(
                        2, int(std::lround((p.kind == SpawnKind::Boss ? 9 : 5) / canvas.scale)));
                if (x + radius < 0 || x - radius >= w || y + radius < 0 || y - radius >= h)
                    continue;
                for (int k = -radius; k <= radius; ++k) {
                    int dy = radius - std::abs(k);
                    blend(x + k, y + dy, tint);
                    if (dy)
                        blend(x + k, y - dy, tint);
                }
                blend(x, y, tint);
                if (p.kind != SpawnKind::Enemy)
                    for (int k = -radius / 2; k <= radius / 2; ++k) {
                        blend(x + k, y, tint);
                        if (p.kind == SpawnKind::Scripted)
                            blend(x, y + k, tint);
                    }
                ++enemyMarkerCount;
            }
        }
    } else
        enemies.visible.clear();
    return true;
}
static void blurImage(std::array<uint32_t, 320 * 224> &img, int r) {
    std::array<uint32_t, 320 * 224> tmp;
    for (int pass = 0; pass < 2; pass++)
        for (int vertical = 0; vertical < 2; vertical++) {
            int lines = vertical ? 320 : 224, len = vertical ? 224 : 320, step = vertical ? 320 : 1;
            for (int l = 0; l < lines; l++) {
                int base = vertical ? l : l * 320, rs = 0, gs = 0, bs = 0, n = 2 * r + 1;
                for (int k = -r; k <= r; k++) {
                    auto c = img[base + clampi(k, 0, len - 1) * step];
                    rs += (c >> 16) & 255;
                    gs += (c >> 8) & 255;
                    bs += c & 255;
                }
                for (int i = 0; i < len; i++) {
                    tmp[base + i * step] =
                        0xFF000000u | ((rs / n) << 16) | ((gs / n) << 8) | (bs / n);
                    auto a = img[base + clampi(i - r, 0, len - 1) * step],
                         b = img[base + clampi(i + r + 1, 0, len - 1) * step];
                    rs += ((b >> 16) & 255) - ((a >> 16) & 255);
                    gs += ((b >> 8) & 255) - ((a >> 8) & 255);
                    bs += (b & 255) - (a & 255);
                }
            }
            img.swap(tmp);
        }
}
void World::makeBackdrop(const SCUSnapshot &s, const Scene &v, const Settings &cfg,
                         const std::vector<uint32_t> &native, int nw, int nh) {
    if (v.ready) {
        std::memcpy(backdrop.data(), s.background, 320 * 224 * sizeof(uint32_t));
    } else if (!native.empty()) {
        for (int y = 0; y < 224; y++)
            for (int x = 0; x < 320; x++)
                backdrop[y * 320 + x] = native[(size_t)y * nh / 224 * nw + x * nw / 320];
    } else {
        backdrop.fill(0xFF122039);
    }
    if (cfg.background == 0 && cfg.blur > 0)
        blurImage(backdrop, cfg.blur);
    uint32_t top = backdrop[160], bottom = backdrop[223 * 320 + 160];
    for (int y = 0; y < 224; y++)
        for (int x = 0; x < 320; x++) {
            auto &c = backdrop[y * 320 + x];
            if (cfg.background == 3) {
                int rr = (((top >> 16) & 255) * (223 - y) + ((bottom >> 16) & 255) * y) / 223,
                    gg = (((top >> 8) & 255) * (223 - y) + ((bottom >> 8) & 255) * y) / 223,
                    bb = ((top & 255) * (223 - y) + (bottom & 255) * y) / 223;
                c = 0xFF000000u | (rr << 16) | (gg << 8) | bb;
            } else if (cfg.background == 4)
                c = 0xFF12233C;
            else if (cfg.background == 5)
                c = 0xFF000000;
            c = 0xFF000000u | ((((c >> 16) & 255) * cfg.brightness / 100) << 16) |
                ((((c >> 8) & 255) * cfg.brightness / 100) << 8) |
                ((c & 255) * cfg.brightness / 100);
        }
}
} // namespace scu
