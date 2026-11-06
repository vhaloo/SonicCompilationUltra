#include "performance.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
using namespace scu;
static void require(bool value, const char *why) {
    if (!value) {
        std::fprintf(stderr, "FAIL %s\n", why);
        std::exit(1);
    }
}
static uint64_t hash(const std::vector<uint32_t> &v) {
    uint64_t h = 0;
    for (auto p : v)
        h = (h * 131) ^ p;
    return h;
}
int main() {
    {
        PerformanceGovernor governor;
        PerformancePolicy vr;
        vr.stereo = true;
        vr.reservePercent = 35;
        vr.targetHz = 90;
        governor.configure(vr);
        for (int i = 0; i < 360; ++i)
            governor.observe({1, 1, 10, true, 0, 1}, 1. / 90);
        require(governor.stereoScale() < 100 && governor.scale() == 100,
                "GPU pressure blurred CPU terrain or failed to reduce stereo work");
        require(governor.effectLevel() < 3, "effects did not adapt before resolution");
        governor.reset();
        for (int i = 0; i < 360; ++i)
            governor.observe({1, 10, 1, true, 0, 1}, 1. / 90);
        require(governor.scale() < 100 && governor.stereoScale() == 100,
                "CPU terrain pressure unnecessarily blurred stereo eyes");
        require(std::abs(governor.usableBudgetMs() - 1000. / 90 * .65) < .001,
                "explicit reserve did not reduce usable budget");
        vr.enabled = false;
        governor.configure(vr);
        require(governor.effectLevel() == 3 && governor.stereoScale() == 100,
                "manual quality mode retained automatic reductions");
    }
    PerformanceGovernor g;
    PerformancePolicy p;
    g.configure(p);
    for (int i = 0; i < 180; ++i)
        g.observe({1, 20, -1, true, 0}, 1. / 60);
    require(g.scale() <= 60 && g.scale() >= 35, "CPU pressure did not lower sampling");
    int low = g.scale();
    for (int i = 0; i < 100; ++i)
        g.observe({1, 1, -1, true, 0}, 1. / 60);
    require(g.scale() == low, "quality recovery oscillates too quickly");
    for (int i = 0; i < 900; ++i)
        g.observe({1, 1, -1, true, 0}, 1. / 60);
    require(g.scale() > low, "quality did not recover after sustained headroom");
    g.reset();
    g.configure(p);
    for (int i = 0; i < 180; ++i)
        g.observe({1, 1, 24, true, 0}, 1. / 60);
    require(g.scale() < 100, "GPU pressure ignored");
    g.reset();
    g.configure(p);
    for (int i = 0; i < 180; ++i)
        g.observe({1, i % 3 == 0 ? 22. : 0., -1, true, 0}, 1. / 90);
    require(g.scale() < 100, "intermittent XR composition overruns masked by empty frames");
    g.reset();
    g.configure(p);
    for (int i = 0; i < 180; ++i)
        g.observe({28, 1, -1, true, 0}, 1. / 60);
    require(g.coreLimited() && g.scale() == 100,
            "simulation bottleneck unnecessarily blurred the scene");
    g.reset();
    g.configure(p);
    g.observe({1, 1, -1, true, 3}, 1. / 60);
    require(g.scale() == 65, "severe thermal cap");
    p.thermalAware = false;
    g.configure(p);
    g.observe({1, 1, -1, true, 4}, 1. / 60);
    require(g.scale() == 100, "thermal opt-out ignored");
    p.enabled = false;
    g.configure(p);
    for (int i = 0; i < 180; ++i)
        g.observe({1, 40, 40, true, 6}, 1. / 60);
    require(g.scale() == 100, "manual full-resolution mode changed");
    p = {};
    g.configure(p);
    g.reset();
    for (int i = 0; i < 30; ++i)
        g.observe({1, 100, 100, false, 0}, 1. / 60);
    g.observe({1, 100, 100, true, 0}, 3.);
    g.observe({1, 1, std::numeric_limits<double>::quiet_NaN(), true, 0}, 1. / 60);
    require(g.scale() == 100, "pause or invalid GPU timings polluted adaptation");
    p.minimum = 80;
    p.maximum = 40;
    p.targetHz = std::numeric_limits<double>::quiet_NaN();
    g.configure(p);
    for (int i = 0; i < 120; ++i)
        g.observe({1, 30, 40, true, 0}, 1. / 60);
    require(g.scale() == 80, "invalid min/max bounds escaped normalization");
    FrameDeadline deadline;
    double now = 1., first = now;
    for (int i = 0; i < 600; ++i) {
        now += .002;
        double wait = deadline.delay(now, 60);
        now += std::ceil(wait * 1000) / 1000;
    }
    require(std::abs(now - first - 10) < .025, "fractional deadline pacing drifted at 60 Hz");
    require(deadline.delay(now + 3, 60) <= 1. / 60 + .00001,
            "pause generated a pacing catch-up burst");
    require(pacedRefresh(60, 144, true) == 72 && pacedRefresh(60, 120, true) == 60 &&
                pacedRefresh(60, 90, true) == 90 && pacedRefresh(60, 50, true) == 50,
            "vsync divisor compatibility");
    require(pacedRefresh(60, 144, false) == 60, "non-vsync cap changed target");
    std::mt19937 random(90261);
    uint64_t restored = 0, fullBytes = 0;
    for (auto shape : {std::pair<int, int>{1, 1},
                       {31, 33},
                       {320, 224},
                       {1280, 720},
                       {2560, 1440},
                       {7680, 1440}}) {
        int w = shape.first, h = shape.second;
        std::vector<uint32_t> base(size_t(w) * h);
        for (auto &v : base)
            v = random();
        auto optimized = base, reference = base;
        DamageMap old, changed;
        old.resize(w, h);
        changed.resize(w, h);
        for (int frame = 0; frame < 20; ++frame) {
            restored += old.restore(optimized, base);
            reference = base;
            changed.clear();
            changed.merge(old);
            old.clear();
            for (int actor = 0; actor < 12; ++actor) {
                int x = int(random() % unsigned(w + 100)) - 50,
                    y = int(random() % unsigned(h + 100)) - 50;
                int aw = 1 + random() % 80, ah = 1 + random() % 50;
                old.mark(x, y, aw, ah);
                for (int yy = std::max(0, y); yy < std::min(h, y + ah); ++yy)
                    for (int xx = std::max(0, x); xx < std::min(w, x + aw); ++xx) {
                        auto at = size_t(yy) * w + xx;
                        reference[at] = optimized[at] = random();
                    }
            }
            require(optimized == reference, "sparse restore differs from complete redraw");
            changed.merge(old);
            auto rects = changed.rectangles();
            auto uploaded = base;
            // Emulate a texture retaining the previous complete frame, then apply
            // only declared uploads. This catches trails and missed damage marks.
            static std::vector<uint32_t> previous;
            if (frame == 0)
                previous = base;
            for (auto r : rects)
                for (int yy = r.y; yy < r.y + r.h; ++yy)
                    std::copy_n(optimized.begin() + size_t(yy) * w + r.x, r.w,
                                previous.begin() + size_t(yy) * w + r.x);
            require(previous == optimized, "partial texture uploads omitted changed pixels");
            fullBytes += size_t(w) * h * 4;
        }
        old.full();
        require(old.restore(optimized, base) == size_t(w) * h * 4, "full invalidation missing");
        require(optimized == base, "terrain invalidation did not restore every pixel");
    }
    TileAttributeCache cache;
    int loads = 0;
    auto loader = [&](int x, int y) {
        ++loads;
        return uint16_t(((x / 8) * 31 + (y / 8) * 73) & 65535);
    };
    require(cache.prepare(1, 16384, 2048, loader), "tile cache capacity");
    int initial = loads;
    require(cache.prepare(1, 16384, 2048, loader) && loads == initial,
            "unchanged geometry rebuilt its lookup cache");
    for (int i = 0; i < 100000; ++i) {
        int x = random() % 16384, y = random() % 2048;
        require(cache.sample(x, y) == loader(x & ~7, y & ~7), "cached terrain attribute mismatch");
    }
    require(cache.sample(-1, 0) == 0 && cache.sample(16384, 0) == 0, "tile domain bounds");
    cache.prepare(2, 16384, 2048, loader);
    require(loads > initial * 2, "layout revision did not invalidate");
    require(!cache.prepare(3, 100000, 100000, loader), "unbounded cache allocation");
    std::array<uint8_t, 65536> geometry{};
    geometry[0xA400] = 1; // foreground uses chunk 0, which uses high block 1023.
    geometry[0] = 3;
    geometry[1] = 255;
    uint64_t revision = terrainGeometryRevision(geometry.data(), 1);
    geometry[0xCFFE] = 42;
    require(terrainGeometryRevision(geometry.data(), 1) != revision,
            "referenced high Sonic 1 block omitted from cache key");
    revision = terrainGeometryRevision(geometry.data(), 1);
    geometry[0xE123] = 77;
    require(terrainGeometryRevision(geometry.data(), 1) == revision,
            "unreferenced actor state invalidated Sonic 1 geometry");
    geometry[0xA400] = 128;
    revision = terrainGeometryRevision(geometry.data(), 1);
    geometry[0xFE00] = 91;
    require(terrainGeometryRevision(geometry.data(), 1) != revision,
            "high referenced chunk change missed");
    revision = terrainGeometryRevision(geometry.data(), 2);
    geometry[0xA7FF] ^= 1;
    require(terrainGeometryRevision(geometry.data(), 2) != revision,
            "Sonic 2/3 tile-bank boundary missed");
    // Pure generated-data microbenchmark. These timings never run an emulator or
    // application and cannot certify a hardware/game frame rate.
    constexpr int w = 3840, h = 2160;
    std::vector<uint32_t> base(size_t(w) * h, 0xff123456), buffer = base;
    DamageMap damage;
    damage.resize(w, h);
    damage.mark(100, 200, 96, 96);
    damage.mark(1500, 800, 320, 224);
    auto begin = std::chrono::steady_clock::now();
    uint64_t sparse = 0;
    for (int i = 0; i < 200; ++i)
        sparse += damage.restore(buffer, base);
    double sparseMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 200; ++i)
        buffer = base;
    double fullMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count();
    std::printf(
        "PASS generated checks: adaptive CPU/GPU/thermal policy, bounded fractional deadlines, "
        "120 damage frames, texture equivalence and terrain revision caching.\n");
    std::printf("BENCH 4K generated restore: full_bytes=%llu sparse_bytes=%llu full_ms=%.3f "
                "sparse_ms=%.3f checksum=%llu\n",
                (unsigned long long)(size_t(w) * h * 4 * 200), (unsigned long long)sparse, fullMs,
                sparseMs, (unsigned long long)hash(buffer));
    std::printf("Synthetic scenarios restored %llu / %llu full-copy bytes. No application, game, "
                "SDL or headset linked or executed.\n",
                (unsigned long long)restored, (unsigned long long)fullBytes);
}
