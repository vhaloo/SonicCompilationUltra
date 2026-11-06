#include "performance.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace scu {
void PerformanceGovernor::configure(PerformancePolicy value) {
    value.minimum = std::clamp(value.minimum, 25, 100);
    value.maximum = std::clamp(value.maximum, value.minimum, 100);
    value.reservePercent = std::clamp(value.reservePercent, 10, 45);
    value.targetHz = std::isfinite(value.targetHz) ? std::clamp(value.targetHz, 30., 240.) : 60.;
    value.targetHz = std::round(value.targetHz * 10.) / 10.;
    if (policy.targetHz != value.targetHz || policy.enabled != value.enabled ||
        policy.stereo != value.stereo)
        reset();
    policy = value;
    percentage = std::clamp(percentage, value.minimum, value.maximum);
    eyePercentage = std::clamp(eyePercentage, value.minimum, value.maximum);
}
void PerformanceGovernor::reset() {
    cpu = peak = simulation = clock = gpuPeak = scenePeak = presentationPeak = percentile =
        pressure = 0;
    gpu = -1;
    changedAt = eyeChangedAt = effectChangedAt = -100;
    samples = overloaded = spare = eyeOverloaded = eyeSpare = effectOverloaded = effectSpare = 0;
    historyAt = historyCount = 0;
    simulationBound = false;
    heatLimit = 100;
    percentage = policy.maximum;
    eyePercentage = policy.maximum;
    effectsLevel = 3;
}
int PerformanceGovernor::scale() const {
    return policy.enabled
               ? std::clamp(std::min(percentage, heatLimit), policy.minimum, policy.maximum)
               : 100;
}
int PerformanceGovernor::stereoScale() const {
    return policy.enabled && policy.stereo
               ? std::clamp(std::min(eyePercentage, heatLimit), policy.minimum, policy.maximum)
               : 100;
}
int PerformanceGovernor::effectLevel() const {
    return policy.enabled && policy.effects ? std::min(effectsLevel, heatLimit <= 50   ? 0
                                                                     : heatLimit <= 65 ? 1
                                                                     : heatLimit <= 85 ? 2
                                                                                       : 3)
                                            : 3;
}
double PerformanceGovernor::availableHeadroom() const {
    return std::clamp(100. * (1. - pressure * policy.targetHz / 1000.), -100., 100.);
}
void PerformanceGovernor::observe(const PerformanceSample &s, double elapsed) {
    if (!s.active || !std::isfinite(elapsed) || elapsed <= 0 || elapsed > .25) {
        samples = overloaded = spare = 0;
        peak = 0; // Preserve the last measured readout while the menu pauses play.
        scenePeak = gpuPeak = presentationPeak = 0;
        historyAt = historyCount = 0;
        eyeOverloaded = eyeSpare = effectOverloaded = effectSpare = 0;
        return;
    }
    clock += elapsed;
    if (!std::isfinite(s.simulationMs) || !std::isfinite(s.renderMs) ||
        !std::isfinite(s.presentationMs) || s.renderMs < 0 || s.presentationMs < 0 ||
        s.simulationMs < 0 || s.simulationMs + s.renderMs + s.presentationMs > 250)
        return;
    const double a = samples ? .12 : 1.;
    const double work = s.simulationMs + s.renderMs + s.presentationMs;
    const double decay = std::exp(-elapsed / .22);
    cpu += a * (work - cpu);
    peak = std::max(peak * decay, work);
    scenePeak = std::max(scenePeak * decay, s.simulationMs + s.renderMs);
    presentationPeak = std::max(presentationPeak * decay, s.presentationMs);
    history[historyAt] = work;
    historyAt = (historyAt + 1) % history.size();
    historyCount = std::min<unsigned>(historyCount + 1, unsigned(history.size()));
    if (samples % 8 == 0) {
        auto sorted = history;
        auto nth = sorted.begin() + (historyCount - 1) * 95 / 100;
        std::nth_element(sorted.begin(), nth, sorted.begin() + historyCount);
        percentile = *nth;
    }
    simulation += a * (s.simulationMs - simulation);
    const bool validGpu = std::isfinite(s.gpuMs) && s.gpuMs >= 0 && s.gpuMs <= 250;
    if (validGpu)
        gpu = gpu < 0 ? s.gpuMs : gpu + a * (s.gpuMs - gpu);
    gpuPeak = validGpu ? std::max(gpuPeak * decay, s.gpuMs) : gpuPeak * decay;
    const double budget = 1000. / policy.targetHz;
    heatLimit = !policy.thermalAware ? 100
                : s.thermal >= 4     ? 50
                : s.thermal >= 3     ? 65
                : s.thermal >= 2     ? 85
                                     : 100;
    simulationBound = simulation > budget * .9 && cpu - simulation < budget * .12 &&
                      (!validGpu || gpu < budget * .8);
    pressure =
        std::max({cpu, peak * .94, percentile, validGpu ? std::max(gpu, gpuPeak * .94) : 0.});
    if (!policy.enabled || ++samples < 8)
        return;
    const double usable = usableBudgetMs();
    const double sceneLoad = policy.stereo ? scenePeak : pressure;
    const double eyeLoad = std::max(presentationPeak, validGpu ? std::max(gpu, gpuPeak * .94) : 0.);
    overloaded = sceneLoad > usable ? overloaded + 1 : 0;
    eyeOverloaded = eyeLoad > usable ? eyeOverloaded + 1 : 0;
    effectOverloaded = pressure > usable ? effectOverloaded + 1 : 0;
    // Recover only after a complete, quiet history window. CPU and stereo
    // sampling are independent: expensive CPU terrain must not blur both eyes.
    const bool timely = elapsed <= 1.20 / policy.targetHz;
    const bool comfortable = pressure < usable * .70 && heatLimit == 100 && timely;
    spare = comfortable ? spare + 1 : 0;
    eyeSpare = comfortable ? eyeSpare + 1 : 0;
    effectSpare = comfortable ? effectSpare + 1 : 0;
    if (policy.effects && effectOverloaded >= 3 && clock - effectChangedAt >= .20 &&
        effectsLevel > 0) {
        --effectsLevel;
        effectChangedAt = clock;
        effectOverloaded = effectSpare = 0;
    }
    if (!simulationBound && overloaded >= 5 && clock - changedAt >= .35 &&
        (!policy.effects || effectsLevel <= 1 || sceneLoad > budget)) {
        const int reduction = sceneLoad > budget * 1.15 ? 10 : 5;
        percentage = std::max(policy.minimum, percentage - reduction);
        changedAt = clock;
        overloaded = spare = 0;
    } else if (spare >= 180 && clock - changedAt >= 4. &&
               pressure * std::pow(double(percentage + 5) / percentage, 2) < usable * .92) {
        percentage = std::min(policy.maximum, percentage + 5);
        changedAt = clock;
        overloaded = spare = 0;
    }
    if (policy.stereo && eyeOverloaded >= 4 && clock - eyeChangedAt >= .25 &&
        (!policy.effects || effectsLevel <= 1 || eyeLoad > budget)) {
        eyePercentage = std::max(policy.minimum, eyePercentage - (eyeLoad > budget ? 10 : 5));
        eyeChangedAt = clock;
        eyeOverloaded = eyeSpare = 0;
    } else if (policy.stereo && eyeSpare >= 180 && clock - eyeChangedAt >= 4. &&
               pressure * std::pow(double(eyePercentage + 5) / eyePercentage, 2) < usable * .92) {
        eyePercentage = std::min(policy.maximum, eyePercentage + 5);
        eyeChangedAt = clock;
        eyeOverloaded = eyeSpare = 0;
    }
    if (policy.effects && effectSpare >= 240 && clock - effectChangedAt >= 6. &&
        percentage >= policy.maximum && (!policy.stereo || eyePercentage >= policy.maximum)) {
        effectsLevel = std::min(3, effectsLevel + 1);
        effectChangedAt = clock;
        effectSpare = 0;
    }
}
double FrameDeadline::delay(double now, double targetHz) {
    if (!std::isfinite(now) || !std::isfinite(targetHz) || targetHz <= 0) {
        reset();
        return 0;
    }
    targetHz = std::clamp(targetHz, 1., 240.);
    double period = 1. / targetHz;
    if (hz != targetHz || next == 0 || now < next - period * 2 || now > next + period * 4)
        next = now;
    hz = targetHz;
    next += period;
    // Do not accumulate a catch-up burst after a missed deadline.
    if (next < now)
        next = now;
    return std::max(0., next - now);
}
double pacedRefresh(double desired, double display, bool vsync) {
    desired = std::isfinite(desired) ? std::clamp(desired, 30., 240.) : 60.;
    if (!std::isfinite(display) || display < 20 || display > 1000)
        return desired;
    if (!vsync)
        return std::min(desired, display);
    // Integer display divisors avoid a 60 Hz software cap fighting a 144 Hz
    // blocking swap. Console simulation timing remains independently paced.
    return display / std::max(1., std::floor(display / desired + .001));
}
uint64_t terrainGeometryRevision(const uint8_t *ram, int family) {
    uint64_t hash = 1469598103934665603ull;
    auto append = [&](const uint8_t *data, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            hash ^= data[i];
            hash *= 1099511628211ull;
        }
    };
    uint8_t kind = uint8_t(family);
    append(&kind, 1);
    if (family != 1) {
        append(ram, 0xA800);
        return hash;
    }
    // Sonic 1's legal decoder range can reference high chunk/block banks.
    // Hash exactly the foreground references, including C800-CFFF if used,
    // instead of hashing actor RAM or overlooking a referenced high block.
    std::array<bool, 128> chunks{};
    std::array<bool, 1024> blocks{};
    for (unsigned row = 0; row < 8; ++row) {
        auto line = ram + 0xA400 + row * 128;
        append(line, 64);
        for (unsigned x = 0; x < 64; ++x)
            if (line[x])
                chunks[(line[x] - 1) & 127] = true;
    }
    for (unsigned i = 0; i < chunks.size(); ++i)
        if (chunks[i]) {
            auto chunk = ram + i * 512;
            append(chunk, 512);
            for (unsigned word = 0; word < 512; word += 2)
                blocks[((unsigned(chunk[word]) << 8) | chunk[word + 1]) & 1023] = true;
        }
    for (unsigned i = 0; i < blocks.size(); ++i)
        if (blocks[i])
            append(ram + 0xB000 + i * 8, 8);
    return hash;
}
void DamageMap::resize(int w, int h) {
    w = std::max(0, w);
    h = std::max(0, h);
    if (width == w && height == h)
        return;
    width = w;
    height = h;
    columns = (w + 31) / 32;
    rows = (h + 31) / 32;
    tiles.assign(size_t(columns) * rows, 0);
    whole = false;
}
void DamageMap::clear() {
    std::fill(tiles.begin(), tiles.end(), 0);
    whole = false;
}
void DamageMap::full() {
    whole = true;
}
void DamageMap::mark(int x, int y, int w, int h) {
    if (whole || w <= 0 || h <= 0 || !width || !height)
        return;
    int left = std::clamp(x, 0, width), top = std::clamp(y, 0, height);
    int right = int(std::clamp(int64_t(x) + w, int64_t(0), int64_t(width)));
    int bottom = int(std::clamp(int64_t(y) + h, int64_t(0), int64_t(height)));
    if (left >= right || top >= bottom)
        return;
    for (int row = top / 32; row <= (bottom - 1) / 32; ++row)
        std::fill(tiles.begin() + size_t(row) * columns + left / 32,
                  tiles.begin() + size_t(row) * columns + (right - 1) / 32 + 1, 1);
}
void DamageMap::merge(const DamageMap &other) {
    if (width != other.width || height != other.height) {
        full();
        return;
    }
    whole |= other.whole;
    if (!whole)
        for (size_t i = 0; i < tiles.size(); ++i)
            tiles[i] |= other.tiles[i];
}
std::vector<DamageRect> DamageMap::rectangles(size_t maximum) const {
    if (!width || !height)
        return {};
    if (whole)
        return {{0, 0, width, height}};
    std::vector<DamageRect> out;
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns;) {
            if (!tiles[size_t(y) * columns + x]) {
                ++x;
                continue;
            }
            int begin = x;
            while (x < columns && tiles[size_t(y) * columns + x])
                ++x;
            DamageRect r{begin * 32, y * 32, std::min(width, x * 32) - begin * 32,
                         std::min(32, height - y * 32)};
            bool joined = false;
            for (auto &previous : out)
                if (previous.x == r.x && previous.w == r.w && previous.y + previous.h == r.y) {
                    previous.h += r.h;
                    joined = true;
                    break;
                }
            if (!joined)
                out.push_back(r);
            if (out.size() > maximum)
                return {{0, 0, width, height}};
        }
    size_t area = 0;
    for (auto r : out)
        area += size_t(r.w) * r.h;
    if (area > size_t(width) * height / 2)
        return {{0, 0, width, height}};
    return out;
}
size_t DamageMap::restore(std::vector<uint32_t> &destination,
                          const std::vector<uint32_t> &source) const {
    if (destination.size() != size_t(width) * height || source.size() != destination.size())
        return 0;
    size_t bytes = 0;
    for (auto r : rectangles())
        for (int y = r.y; y < r.y + r.h; ++y) {
            std::memcpy(destination.data() + size_t(y) * width + r.x,
                        source.data() + size_t(y) * width + r.x, size_t(r.w) * 4);
            bytes += size_t(r.w) * 4;
        }
    return bytes;
}
} // namespace scu
