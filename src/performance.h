#pragma once
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

namespace scu {
struct PerformancePolicy {
    bool enabled = true, thermalAware = true, effects = true, stereo = false;
    int minimum = 35, maximum = 100;
    int reservePercent = 25;
    double targetHz = 60;
};
struct PerformanceSample {
    double simulationMs = 0, renderMs = 0, gpuMs = -1;
    bool active = true;
    int thermal = 0;
    double presentationMs = 0;
};
class PerformanceGovernor {
    PerformancePolicy policy;
    double cpu = 0, peak = 0, gpu = -1, gpuPeak = 0, simulation = 0, clock = 0, changedAt = -100,
           eyeChangedAt = -100, effectChangedAt = -100, pressure = 0;
    double scenePeak = 0, presentationPeak = 0, percentile = 0;
    std::array<double, 90> history{};
    unsigned historyAt = 0, historyCount = 0;
    unsigned samples = 0, overloaded = 0, spare = 0, eyeOverloaded = 0, eyeSpare = 0,
             effectOverloaded = 0, effectSpare = 0;
    int percentage = 100, eyePercentage = 100, heatLimit = 100, effectsLevel = 3;
    bool simulationBound = false;

  public:
    void configure(PerformancePolicy value);
    void observe(const PerformanceSample &sample, double elapsedSeconds);
    void reset();
    int scale() const;
    int stereoScale() const;
    int effectLevel() const;
    double availableHeadroom() const;
    double usableBudgetMs() const {
        return 1000. / policy.targetHz * (1. - policy.reservePercent / 100.);
    }
    double cpuMs() const {
        return cpu;
    }
    double gpuMs() const {
        return gpu;
    }
    double simulationMs() const {
        return simulation;
    }
    bool coreLimited() const {
        return simulationBound;
    }
    int thermalLimit() const {
        return heatLimit;
    }
};
// No sleeping or clock API here: platform code supplies monotonic seconds and
// sleeps the returned delay. Absolute deadlines avoid rounding 16.67 ms to 17 ms.
class FrameDeadline {
    double next = 0, hz = 0;

  public:
    double delay(double now, double targetHz);
    void reset() {
        next = hz = 0;
    }
};
double pacedRefresh(double desired, double display, bool vsync);
uint64_t terrainGeometryRevision(const uint8_t *ram, int family);
struct DamageRect {
    int x = 0, y = 0, w = 0, h = 0;
};
// Coarse tiles merge overlapping sprites into a few upload/restore rectangles.
// Marks are conservative; original pixel colors and draw order never change.
class DamageMap {
    int width = 0, height = 0, columns = 0, rows = 0;
    std::vector<uint8_t> tiles;
    bool whole = false;

  public:
    void resize(int w, int h);
    void clear();
    void full();
    void mark(int x, int y, int w, int h);
    void merge(const DamageMap &other);
    std::vector<DamageRect> rectangles(size_t maximum = 32) const;
    size_t restore(std::vector<uint32_t> &destination, const std::vector<uint32_t> &source) const;
};
// The cartridges place the same 128- or 256-pixel chunk many times. Decode its
// tile attributes once, keeping a small offset per placement for raster workers.
class TileAttributeCache {
    uint64_t key = 0;
    int width = 0, height = 0, shift = 0, columns = 0;
    std::vector<uint32_t> offsets;
    std::vector<uint16_t> values;

  public:
    template <class Locate, class Decode>
    bool prepare(uint64_t revision, int w, int h, int chunkShift, Locate locate, Decode decode) {
        if (w <= 0 || h <= 0 || w > 32768 || h > 4096 || (chunkShift != 7 && chunkShift != 8)) {
            clear();
            return false;
        }
        if (key == revision && width == w && height == h && shift == chunkShift && !values.empty())
            return true;
        width = w;
        height = h;
        shift = chunkShift;
        key = revision;
        const int side = 1 << shift, tiles = side / 8;
        columns = (w + side - 1) / side;
        offsets.resize(size_t(columns) * ((h + side - 1) / side));
        std::array<uint32_t, 256> decoded{};
        values.assign(tiles * tiles, 0); // Offset zero is an absent layout entry.
        size_t placement = 0;
        for (int y = 0; y < h; y += side) {
            for (int x = 0; x < w; x += side) {
                int chunk = locate(x, y);
                uint32_t offset = 0;
                if (unsigned(chunk) < decoded.size()) {
                    if (!decoded[chunk]) {
                        decoded[chunk] = uint32_t(values.size());
                        values.resize(values.size() + tiles * tiles);
                        auto *dest = values.data() + decoded[chunk];
                        for (int ty = 0; ty < side; ty += 8)
                            for (int tx = 0; tx < side; tx += 8)
                                *dest++ = decode(chunk, tx, ty);
                    }
                    offset = decoded[chunk];
                }
                offsets[placement++] = offset;
            }
        }
        return true;
    }
    uint16_t sample(int x, int y) const {
        if (unsigned(x) >= unsigned(width) || unsigned(y) >= unsigned(height))
            return 0;
        const int mask = (1 << shift) - 1;
        const auto offset = offsets[(y >> shift) * columns + (x >> shift)];
        return values[offset + (((y & mask) >> 3) << (shift - 3)) + ((x & mask) >> 3)];
    }
    void clear() {
        std::vector<uint32_t>().swap(offsets);
        std::vector<uint16_t>().swap(values);
        key = 0;
        width = height = columns = 0;
    }
    size_t bytes() const {
        return values.capacity() * sizeof(uint16_t) + offsets.capacity() * sizeof(uint32_t);
    }
};
} // namespace scu
