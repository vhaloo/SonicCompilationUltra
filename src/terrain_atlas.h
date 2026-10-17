#pragma once
#include "observer.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <array>
namespace scu {
// Bounded cartridge codecs. Output is cleared on failure; no game data is shipped.
bool decodeSSC(const std::vector<uint8_t> &input, size_t offset, size_t size,
               std::vector<uint8_t> &output);
bool decodeRNC2Row(const std::vector<uint8_t> &input, size_t offset, std::vector<uint8_t> &output);
class TerrainAtlas {
    int family = -1, stage = -1;
    std::vector<uint8_t> pixels;
    std::vector<std::array<uint16_t, 2>> cells;
    std::array<std::vector<uint32_t>, 2048> tileUses;
    std::array<uint8_t, 65536> previousVram{};
    unsigned terrainArt = 0;
    std::vector<uint8_t> spinArt;
    uint64_t revision = 0;
    int backgroundY(int y) const;
    void drawCell(uint32_t cell, const std::vector<uint8_t> &rom, const SCUSnapshot &snapshot,
                  bool live);
    void animate(const std::vector<uint8_t> &rom, const SCUSnapshot &snapshot);

  public:
    int width = 0, height = 0;
    bool prepare(const std::vector<uint8_t> &rom, const SCUSnapshot &snapshot, int kind, int act);
    void clear();
    uint8_t sample(int x, int y) const {
        return x >= 0 && y >= 0 && x < width && y < height ? pixels[size_t(y) * width + x] : 0;
    }
    size_t bytes() const {
        size_t total = pixels.capacity() + cells.capacity() * sizeof(cells[0]) +
                       spinArt.capacity() + previousVram.size();
        for (const auto &uses : tileUses)
            total += uses.capacity() * sizeof(uint32_t);
        return pixels.empty() ? 0 : total;
    }
    uint64_t version() const {
        return revision;
    }
};
} // namespace scu
