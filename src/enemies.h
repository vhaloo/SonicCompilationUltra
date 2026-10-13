#pragma once
#include "observer.h"
#include "rom.h"
#include <unordered_map>

namespace scu {
// Presentation-only data: never allocates or writes a cartridge object.
struct EnemyImage {
    int left = 0, top = 0, width = 0, height = 0;
    std::vector<uint8_t> pixels; // Native palette indices; zero is transparent.
};
struct EnemyPreview {
    int x = 0, y = 0;
    unsigned flags = 0;
    const EnemyImage *image = nullptr;
};
class EnemyPreviews {
    struct Placement {
        int x, y;
        unsigned id, subtype, flags, state;
    };
    struct Appearance {
        unsigned map = 0, gfx = 0, frame = 0;
        std::vector<uint8_t> tiles;
        EnemyImage image;
    };
    std::vector<Placement> placements;
    std::unordered_map<unsigned, Appearance> appearances;
    int cachedStage = -1;
    uint32_t cachedCRC = 0;

  public:
    bool supported = false;
    size_t placementCount = 0;
    std::vector<EnemyPreview> visible;
    void reset();
    void update(const SCUSnapshot &s, const Game *game, const std::vector<uint8_t> &rom, int stage,
                int cameraX, int cameraY);
};
} // namespace scu
