#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace scu {
struct PerformancePolicy {
    bool enabled = true, thermalAware = true;
    int minimum = 35, maximum = 100;
    double targetHz = 60;
};
struct PerformanceSample {
    double simulationMs = 0, renderMs = 0, gpuMs = -1;
    bool active = true;
    int thermal = 0;
};
class PerformanceGovernor {
    PerformancePolicy policy;
    double cpu = 0, peak = 0, gpu = -1, simulation = 0, clock = 0, changedAt = -100;
    unsigned samples = 0, overloaded = 0, spare = 0;
    int percentage = 100, heatLimit = 100;
    bool simulationBound = false;

  public:
    void configure(PerformancePolicy value);
    void observe(const PerformanceSample &sample, double elapsedSeconds);
    void reset();
    int scale() const;
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
// Whole-stage tile attributes are geometry, independent of palette animation,
// render sampling and camera position. The loader is called only on a revision.
class TileAttributeCache {
    uint64_t key = 0;
    int columns = 0, rows = 0;
    std::vector<uint16_t> values;

  public:
    template <class Loader> bool prepare(uint64_t revision, int w, int h, Loader load) {
        const int cols = (w + 7) / 8, lines = (h + 7) / 8;
        if (cols <= 0 || lines <= 0 || size_t(cols) * lines > 2097152) {
            clear();
            return false;
        }
        if (key == revision && columns == cols && rows == lines && !values.empty())
            return true;
        columns = cols;
        rows = lines;
        key = revision;
        values.resize(size_t(cols) * lines);
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x)
                values[size_t(y) * columns + x] = load(x * 8, y * 8);
        return true;
    }
    uint16_t sample(int x, int y) const {
        return x < 0 || y < 0 || (x >> 3) >= columns || (y >> 3) >= rows
                   ? 0
                   : values[size_t(y >> 3) * columns + (x >> 3)];
    }
    void clear() {
        std::vector<uint16_t>().swap(values);
        key = 0;
        columns = rows = 0;
    }
    size_t bytes() const {
        return values.capacity() * sizeof(uint16_t);
    }
};
} // namespace scu
