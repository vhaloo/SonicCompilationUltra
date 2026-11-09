#include "world.h"
#include <algorithm>
#include <cmath>

namespace scu {
namespace {
int bound(int value, int limit) {
    return std::clamp(value, 0, std::max(0, limit - 1));
}
uint32_t over(uint32_t front, uint32_t back) {
    unsigned a = front >> 24, b = (back >> 24) * (255 - a) / 255, total = a + b;
    if (!total)
        return 0;
    uint32_t out = total << 24;
    for (unsigned shift : {0u, 8u, 16u})
        out |= ((((front >> shift) & 255) * a + ((back >> shift) & 255) * b) / total) << shift;
    return out;
}
uint32_t tint(uint32_t source, uint32_t target) {
    uint32_t out = 0xFF000000;
    for (unsigned shift : {0u, 8u, 16u})
        out |= (((((source >> shift) & 255) * 2 + ((target >> shift) & 255)) / 3)) << shift;
    return out;
}
} // namespace
bool World::prepareVolume(const SCUSnapshot &s, const Game *game, const Settings &cfg,
                          const std::vector<uint32_t> &native, int nw, int nh, bool expanded) {
    if (!cfg.voxelMode) {
        if (volume.pixels.capacity()) {
            volume.discard();
            volumeMesh.discard();
        }
        return false;
    }
    Scene scene = inspect(s, game);
    const auto mode = s.ram[0xF600];
    if (scene.family >= 1 && scene.family <= 3 && !scene.ready &&
        (mode == 0 || mode == 4 || (mode & 0x80))) {
        if (volume.valid()) {
            volume.clear();
            volumeMesh.clear();
        }
        return false;
    }
    expanded = expanded && scene.ready && canvas.width > 0 && canvas.height > 0 &&
               terrainPixels.size() == size_t(canvas.width) * canvas.height &&
               priority.size() == terrainPixels.size();
    if (!expanded && (nw <= 0 || nh <= 0 || native.size() != size_t(nw) * nh)) {
        volume.clear();
        volumeMesh.clear();
        return false;
    }
    int sourceW = expanded ? canvas.width : nw, sourceH = expanded ? canvas.height : nh;
    double left = expanded ? canvas.left : 0, top = expanded ? canvas.top : 0,
           cw = expanded ? canvas.coverageW : nw, ch = expanded ? canvas.coverageH : nh;
    bool mobile = false;
#ifdef __ANDROID__
    mobile = true;
#endif
    size_t budget = voxel::cellBudget(cfg.voxelDetail, cfg.wearActive, mobile, cfg.effectsLevel,
                                      cfg.autoPerformance);
    voxel::Options options;
    options.mode = cfg.voxelMode;
    options.thickness = float(cfg.voxelThickness);
    options.spacing = float(cfg.voxelSpacing);
    options.relief = float(cfg.voxelRelief);
    options.faceLimit = std::min<size_t>(100000, std::max<size_t>(2048, budget * 2));
    const bool backdrop = cfg.voxelBackground && cfg.background != 6 && !cfg.projectionTransparent;
    // Reduce the complete grid for a complex silhouette. Never drop trailing
    // objects or publish a partially generated scene when the budget is reached.
    for (int attempt = 0; attempt < 7; ++attempt, budget = std::max<size_t>(16, budget / 4)) {
        auto dims = voxel::grid(cw, ch, sourceW, sourceH, budget);
        if (!dims[0] || !dims[1])
            break;
        volume.configure(dims[0], dims[1], left, top, cw, ch);
        const int w = volume.width, h = volume.height;
        volume.hardwareLayers = expanded;
        volume.nativeOnly = !expanded;
        const double dx = cw / w, dy = ch / h;
        auto toPixel = [&](double x, double y, bool end = false) {
            const double px = (x - left) * w / cw, py = (y - top) * h / ch;
            return end ? std::array<int, 2>{int(std::ceil(px)), int(std::ceil(py))}
                       : std::array<int, 2>{int(std::floor(px)), int(std::floor(py))};
        };
        if (expanded) {
            // Retain terrain underneath actors. Hardware priority selects the
            // depth plane; black or dark art never guesses a hole or distance.
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    int sx = bound(int((x + .5) * sourceW / w), sourceW),
                        sy = bound(int((y + .5) * sourceH / h), sourceH);
                    size_t at = size_t(sy) * sourceW + sx;
                    uint32_t color = terrainPixels[at];
                    bool high = priority[at] != 0;
                    if (!(color >> 24)) {
                        for (auto offset : {std::array<double, 2>{.12, .12},
                                            {.88, .12},
                                            {.12, .88},
                                            {.88, .88}}) {
                            int xx = bound(int((x + offset[0]) * sourceW / w), sourceW),
                                yy = bound(int((y + offset[1]) * sourceH / h), sourceH);
                            size_t sample = size_t(yy) * sourceW + xx;
                            if (terrainPixels[sample] >> 24) {
                                color = terrainPixels[sample];
                                high = priority[sample] != 0;
                                break;
                            }
                        }
                    }
                    volume.at(high ? voxel::Plane::TerrainHigh : voxel::Plane::TerrainLow, x, y) =
                        color;
                    if (backdrop) {
                        int wx = int(left + (x + .5) * dx), wy = int(top + (y + .5) * dy);
                        uint32_t bg = 0;
                        bool highBackground = false;
                        if (scene.family > 0 && scene.family <= 3) {
                            unsigned attr = attribute(s, scene.family, wx, wy, true);
                            highBackground = (attr & 0x8000) != 0;
                            unsigned ix = (wx & 7) ^ ((attr & 0x800) ? 7 : 0),
                                     iy = (wy & 7) ^ ((attr & 0x1000) ? 7 : 0);
                            unsigned index = patterns[attr & 2047][iy * 8 + ix];
                            if (index)
                                bg = s.palettes[bound(wy - scene.cameraY, 224)]
                                               [((attr >> 9) & 48) | index];
                        } else {
                            int bx = ((wx - scene.cameraX) % 320 + 320) % 320,
                                by = ((wy - scene.cameraY) % 224 + 224) % 224;
                            bg = s.background[by][bx];
                        }
                        volume.at(highBackground ? voxel::Plane::BackgroundHigh
                                                 : voxel::Plane::Background,
                                  x, y) = bg;
                    }
                }
            // Reuse the panorama's read-only SAT/mapping pieces. Low-priority
            // actors stay behind high terrain even when head movement reveals
            // their formerly occluded pixels. World positions are unchanged.
            for (const auto &sprite : sprites) {
                int sw = (((sprite.size >> 2) & 3) + 1) * 8, sh = ((sprite.size & 3) + 1) * 8;
                auto a = toPixel(sprite.x, sprite.y),
                     b = toPixel(sprite.x + sw, sprite.y + sh, true);
                int x1 = std::min(w, b[0]), y1 = std::min(h, b[1]);
                const bool high = (sprite.attr & 0x8000) != 0;
                auto plane = high ? voxel::Plane::ActorsHigh : voxel::Plane::ActorsLow;
                auto behind = high ? voxel::Plane::ActorsLow : voxel::Plane::ActorsHigh;
                for (int y = std::max(0, a[1]); y < y1; ++y)
                    for (int x = std::max(0, a[0]); x < x1; ++x) {
                        int ix = bound(int(left + (x + .5) * dx - sprite.x), sw),
                            iy = bound(int(top + (y + .5) * dy - sprite.y), sh);
                        const int scanline = bound(sprite.y + iy - scene.cameraY, 224);
                        if (sprite.attr & 0x800)
                            ix = sw - 1 - ix;
                        if (sprite.attr & 0x1000)
                            iy = sh - 1 - iy;
                        unsigned tile =
                                     ((sprite.attr & 2047) + (ix / 8) * (sh / 8) + iy / 8) & 2047,
                                 index = patterns[tile][(iy & 7) * 8 + (ix & 7)];
                        if (index) {
                            const uint32_t color =
                                s.palettes[scanline][((sprite.attr >> 9) & 48) | index];
                            if (!(color >> 24))
                                continue;
                            // SAT list is back to front. Its first hardware sprite
                            // wins overlaps regardless of the background priority bit.
                            volume.at(behind, x, y) = 0;
                            volume.at(plane, x, y) = color;
                        }
                    }
            }
            if (cfg.enemyPreview)
                for (const auto &preview : enemies.visible) {
                    const auto *image = preview.image;
                    const bool useImage =
                        image && cfg.enemyStyle != 2 && image->width > 0 && image->height > 0 &&
                        image->width <= 2048 && image->height <= 2048 &&
                        image->pixels.size() == size_t(image->width) * image->height;
                    int px = preview.x + (useImage
                                              ? ((preview.flags & 1) ? -image->left - image->width
                                                                     : image->left)
                                              : -5),
                        py = preview.y +
                             (useImage
                                  ? ((preview.flags & 2) ? -image->top - image->height : image->top)
                                  : -5),
                        sw = useImage ? image->width : 11, sh = useImage ? image->height : 11;
                    auto a = toPixel(px, py), b = toPixel(px + sw, py + sh, true);
                    int x1 = std::min(w, b[0]), y1 = std::min(h, b[1]);
                    unsigned alpha = unsigned(std::clamp(cfg.enemyOpacity, 10, 80)) * 255 / 100;
                    bool drawn = false;
                    for (int y = std::max(0, a[1]); y < y1; ++y)
                        for (int x = std::max(0, a[0]); x < x1; ++x) {
                            uint32_t color = 0xFFFFCA75;
                            int ix = bound(int(left + (x + .5) * dx - px), sw),
                                iy = bound(int(top + (y + .5) * dy - py), sh);
                            if (useImage) {
                                if (preview.flags & 1)
                                    ix = sw - 1 - ix;
                                if (preview.flags & 2)
                                    iy = sh - 1 - iy;
                                unsigned index = image->pixels[size_t(iy) * sw + ix];
                                if (!index || index >= 64)
                                    continue;
                                color = s.palettes[bound(preview.y - scene.cameraY, 224)][index];
                            } else if (std::abs(ix - 5) + std::abs(iy - 5) > 5)
                                continue;
                            if (cfg.spawnTint && preview.kind != SpawnKind::Enemy) {
                                uint32_t target = preview.kind == SpawnKind::Boss ? 0xFFFF8194
                                                  : preview.kind == SpawnKind::MovingObject
                                                      ? 0xFF84E1CB
                                                      : 0xFFC18CFF;
                                color = tint(color, target);
                            }
                            auto &destination = volume.at(voxel::Plane::Preview, x, y);
                            destination = over((color & 0xFFFFFF) | (alpha << 24), destination);
                            drawn = true;
                        }
                    // Match the 2D preview's small-marker fallback. A large map
                    // must not lose a spawn just because its sprite samples a hole.
                    if (cfg.enemyStyle != 1 || !drawn) {
                        auto center = toPixel(preview.x, preview.y);
                        uint32_t color = preview.kind == SpawnKind::Boss           ? 0xFFFF8194
                                         : preview.kind == SpawnKind::MovingObject ? 0xFF84E1CB
                                         : preview.kind == SpawnKind::Scripted     ? 0xFFC18CFF
                                                                                   : 0xFFFFCA75;
                        for (int yy = -2; yy <= 2; ++yy)
                            for (int xx = -2; xx <= 2; ++xx) {
                                if (std::abs(xx) + std::abs(yy) > 2)
                                    continue;
                                int x = center[0] + xx, y = center[1] + yy;
                                if (x < 0 || x >= w || y < 0 || y >= h)
                                    continue;
                                auto &destination = volume.at(voxel::Plane::Preview, x, y);
                                destination = over((color & 0xFFFFFF) | (alpha << 24), destination);
                            }
                    }
                }
        } else {
            // Unprofiled scenes still support exact screen-pixel relief. Do not
            // invent missing object coordinates or advertise artist-made models.
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    int sx = bound(int((x + .5) * nw / w), nw),
                        sy = bound(int((y + .5) * nh / h), nh);
                    uint32_t color = native[size_t(sy) * nw + sx];
                    volume.at(voxel::Plane::TerrainLow, x, y) = color;
                }
        }
        if (cfg.voxelMode == 1) {
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x) {
                    uint32_t color = 0;
                    for (size_t p = 0; p < size_t(voxel::Plane::Preview); ++p) {
                        color = over(volume.at(voxel::Plane(p), x, y), color);
                        volume.at(voxel::Plane(p), x, y) = 0;
                    }
                    volume.at(voxel::Plane::TerrainLow, x, y) = color;
                }
        }
        if (volumeMesh.update(volume, options))
            return true;
    }
    volume.clear();
    volumeMesh.clear();
    return false;
}
} // namespace scu
