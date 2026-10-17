#pragma once
#include "observer.h"
#include "settings.h"
#include "enemies.h"
#include "terrain_atlas.h"
#include <array>
namespace scu {
inline uint16_t be16(const uint8_t *p, unsigned a) {
    a &= 65535;
    return (uint16_t)((p[a] << 8) | p[(a + 1) & 65535]);
}
struct Scene {
    bool ready = false;
    int family = 0, stage = 0, width = 320, height = 224;
    int cameraX = 0, cameraY = 0, playerX = 0, playerY = 0;
};
struct Canvas {
    int width = 320, height = 224, left = 0, top = 0;
    double coverageW = 320, coverageH = 224;
    float scale = 1;
    std::vector<uint32_t> pixels;
};
struct Sprite {
    int x, y;
    uint16_t attr;
    uint8_t size;
};
class World {
    std::array<std::array<uint8_t, 64>, 2048> patterns{};
    std::array<uint8_t, 65536> patternSource{};
    std::array<bool, 2048> visibleTile{};
    bool patternsValid = false;
    std::array<bool, 2048> terrainUsed{};
    std::vector<uint32_t> terrainPixels;
    std::vector<uint8_t> terrainPriority;
    uint64_t shapeSignature = 0, terrainSignature = 0;
    std::vector<uint8_t> priority;
    std::vector<Sprite> sprites;
    std::vector<uint8_t> rom;
    int lastStage = -1, lastFamily = -1;
    bool cameraValid = false;
    uint8_t read(unsigned address) const;
    uint16_t word(unsigned address) const;
    uint32_t longword(unsigned address) const;
    const SCUSnapshot *snap = nullptr;
    void mapping(unsigned addr, unsigned frame, unsigned gfx, unsigned flags, int x, int y,
                 bool direct = false);
    void captureActors(const Scene &scene, const Game *game);

  public:
    TerrainAtlas atlas;
    EnemyPreviews enemies;
    int enemySpriteCount = 0, enemyMarkerCount = 0;
    Canvas canvas;
    uint64_t terrainCacheHits = 0, terrainCacheMisses = 0;
    std::array<uint32_t, 320 * 224> backdrop{};
    Scene inspect(const SCUSnapshot &s, const Game *game) const;
    void setRom(const std::vector<uint8_t> &r) {
        rom = r;
        atlas.clear();
        patternsValid = false;
        shapeSignature = terrainSignature = 0;
        terrainPixels.clear();
        terrainPriority.clear();
        reset();
    }
    void reset() {
        lastStage = -1;
        cameraValid = false;
        enemies.reset();
    }
    void freeze(const Scene &scene, Settings &settings);
    uint16_t attribute(const SCUSnapshot &s, int family, int x, int y, bool bg = false) const;
    bool compose(const SCUSnapshot &s, const Game *game, Settings &settings, int drawableW,
                 int drawableH, const std::vector<uint32_t> *native = nullptr, int nativeW = 320,
                 int nativeH = 224);
    void makeBackdrop(const SCUSnapshot &s, const Scene &scene, const Settings &settings,
                      const std::vector<uint32_t> &native, int nw, int nh);
};
} // namespace scu
