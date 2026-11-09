#include "voxel.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace scu::voxel {
namespace {
constexpr uint64_t basis = 1469598103934665603ull;
uint64_t mix(uint64_t h, uint64_t v) {
    return (h ^ v) * 1099511628211ull;
}
uint64_t floatBits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}
bool occupied(uint32_t c) {
    return c >> 24;
}
unsigned alphaClass(uint32_t c) {
    return !occupied(c) ? 0 : c >> 24 == 255 ? 2 : 1;
}
unsigned reliefStep(uint32_t c, const Options &o) {
    if (o.mode != 3 || !occupied(c))
        return 0;
    unsigned luma = (((c >> 16) & 255) * 54 + ((c >> 8) & 255) * 183 + (c & 255) * 19) >> 8;
    return std::min(3u, luma >> 6);
}
} // namespace
void Frame::configure(int w, int h, double x, double y, double cw, double ch) {
    if (w <= 0 || h <= 0 || w > maxWidth || h > maxHeight ||
        !std::isfinite(cw) || !std::isfinite(ch) ||
        !std::isfinite(x) || !std::isfinite(y) || cw <= 0 || ch <= 0) {
        clear();
        return;
    }
    width = w;
    height = h;
    left = x;
    top = y;
    coverageWidth = cw;
    coverageHeight = ch;
    pixels.resize(size_t(textureWidth()) * textureHeight());
    std::fill(pixels.begin(), pixels.end(), 0);
    hardwareLayers = false;
    nativeOnly = false;
    planes = allPlanes;
    planeRevisions.fill(0);
    ++revision;
}
uint32_t &Frame::at(Plane p, int x, int y) {
    return row(p, y)[x];
}
uint32_t Frame::at(Plane p, int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height
               ? row(p, y)[x]
               : 0;
}
const uint32_t *Frame::row(Plane p, int y) const {
    return pixels.data() + (size_t(p) / 2 * height + y) * textureWidth() + size_t(p) % 2 * width;
}
uint32_t *Frame::row(Plane p, int y) {
    return pixels.data() + (size_t(p) / 2 * height + y) * textureWidth() + size_t(p) % 2 * width;
}
bool Frame::valid() const {
    return width > 0 && height > 0 && width <= maxWidth && height <= maxHeight && coverageWidth > 0 &&
           coverageHeight > 0 && std::isfinite(coverageWidth) && std::isfinite(coverageHeight) &&
           std::isfinite(left) && std::isfinite(top) &&
           pixels.size() == size_t(textureWidth()) * textureHeight();
}
void Frame::clear() {
    width = height = 0;
    pixels.clear();
    ++revision;
}
void Frame::discard() {
    const auto next = revision + 1;
    *this = {};
    revision = next;
}
std::array<float, 2> column(Plane p, uint32_t c, const Options &o) {
    const float thickness = std::clamp(o.thickness, .25f, 32.f);
    const float relief = o.mode == 3 ? std::clamp(o.relief, 0.f, 12.f) * reliefStep(c, o) / 3.f : 0;
    const int rank = o.mode == 1 ? (p == Plane::Preview ? 1 : 0) : int(p);
    // Reserving the complete thickness + maximum relief keeps every plane's
    // front and back disjoint, even when the user chooses zero air spacing.
    const float band = thickness + (o.mode == 3 ? std::clamp(o.relief, 0.f, 12.f) : 0) +
                       std::clamp(o.spacing, 0.f, 64.f) + .05f;
    const float back = rank * band;
    return {back, back + thickness + relief};
}
static uint64_t geometryConfiguration(const Frame &f, const Options &o) {
    uint64_t h = basis;
    for (int value : {f.width, f.height, o.mode, std::clamp(o.geometryStep, 1, 64)})
        h = mix(h, uint32_t(value));
    for (float value : {float(f.left), float(f.top), float(f.coverageWidth),
                        float(f.coverageHeight), o.thickness, o.spacing, o.relief})
        h = mix(h, floatBits(value));
    return h;
}
static uint64_t prepareLayer(const Frame &f, Plane p, const Options &o, uint64_t configuration,
                             uint64_t *pixelSignature, std::vector<uint32_t> &shape,
                             std::vector<uint32_t> &samples) {
    const int step = std::clamp(o.geometryStep, 1, 64);
    uint64_t h = mix(configuration, unsigned(p));
    uint64_t pixels = mix(mix(basis, f.width), f.height);
    if (!f.valid() || !(f.planes & (1u << unsigned(p)))) {
        if (pixelSignature)
            *pixelSignature = pixels;
        return h;
    }
    if (step == 1) {
        for (int y = 0; y < f.height; ++y) {
            const auto *row = f.row(p, y);
            for (int x = 0; x < f.width; ++x) {
                const auto color = row[x];
                h = mix(h, alphaClass(color) | (reliefStep(color, o) << 2));
                pixels = mix(pixels, color);
            }
        }
    } else {
        const int columns = (f.width + step - 1) / step, rows = (f.height + step - 1) / step;
        shape.resize(size_t(columns) * rows);
        samples.resize(shape.size());
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x) {
                const int left = x * step, top = y * step;
                const int right = std::min(left + step, f.width);
                const int bottom = std::min(top + step, f.height);
                uint32_t representative = 0, sample = 0;
                bool translucent = false;
                for (int yy = top; yy < bottom; ++yy) {
                    const auto *row = f.row(p, yy);
                    for (int xx = left; xx < right; ++xx) {
                        const auto color = row[xx];
                        pixels = mix(pixels, color);
                        if (occupied(color)) {
                            if (!representative) {
                                representative = color;
                                sample = yy * f.width + xx;
                            }
                            translucent |= color >> 24 != 255;
                        }
                    }
                }
                if (translucent)
                    representative = (representative & 0x00FFFFFFu) | 0x80000000u;
                const size_t index = size_t(y) * columns + x;
                shape[index] = representative;
                samples[index] = sample;
                h = mix(h, alphaClass(representative) | (reliefStep(representative, o) << 2));
                h = mix(h, sample);
            }
    }
    if (pixelSignature)
        *pixelSignature = pixels;
    return h;
}
uint64_t shapeSignature(const Frame &f, Plane p, const Options &o, uint64_t *pixelSignature) {
    std::vector<uint32_t> shape, samples;
    return prepareLayer(f, p, o, geometryConfiguration(f, o), pixelSignature, shape, samples);
}
bool Mesher::buildLayer(const Frame &f, size_t plane, const Options &o, LayerMesh &out,
                        size_t limit) {
    out.opaque.clear();
    out.translucent.clear();
    if (!(f.planes & (1u << plane)))
        return true;
    const int step = std::clamp(o.geometryStep, 1, 64);
    const int w = (f.width + step - 1) / step, h = (f.height + step - 1) / step;
    const auto p = Plane(plane);
    const float dx = float(f.coverageWidth / f.width), dy = float(f.coverageHeight / f.height),
                left = float(f.left), top = float(f.top), tw = float(f.textureWidth()),
                th = float(f.textureHeight());
    size_t faces = 0;
    auto emit = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal, std::array<float, 2> ua,
                    std::array<float, 2> ub, std::array<float, 2> uc, std::array<float, 2> ud,
                    bool alpha) {
        if (++faces > limit)
            return false;
        auto &target = alpha ? out.translucent : out.opaque;
        const std::array<Vertex, 4> v{{{a, normal, ua[0], ua[1]},
                                       {b, normal, ub[0], ub[1]},
                                       {c, normal, uc[0], uc[1]},
                                       {d, normal, ud[0], ud[1]}}};
        for (int n : {0, 1, 2, 0, 2, 3})
            target.push_back(v[n]);
        return true;
    };
    auto colorAt = [&](int x, int y) -> uint32_t {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return 0;
        return step == 1 ? f.row(p, y)[x] : shapePixels[size_t(y) * w + x];
    };
    auto texel = [&](float x, float y) {
        return std::array<float, 2>{((plane % 2) * f.width + x) / tw,
                                    ((plane / 2) * f.height + y) / th};
    };
    auto sourceX = [&](float x) { return std::min(x * step, float(f.width)); };
    auto sourceY = [&](float y) { return std::min(y * step, float(f.height)); };
    auto tex = [&](float x, float y) {
        return texel(sourceX(x), sourceY(y));
    };
    auto sideTexel = [&](int x, int y) {
        const auto sample = shapeTexels[size_t(y) * w + x];
        return texel(sample % f.width + .5f, sample / f.width + .5f);
    };
    // Rectangles merge only identical height/alpha classes. The texture retains
    // every colour and texel boundary, so palette cycling never rebuilds faces.
    visited.assign(size_t(w) * h, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            size_t at = size_t(y) * w + x;
            uint32_t color = colorAt(x, y);
            if (visited[at] || !occupied(color))
                continue;
            auto extent = column(p, color, o);
            unsigned key = alphaClass(color) | (reliefStep(color, o) << 2);
            auto same = [&](int xx, int yy) {
                auto c = colorAt(xx, yy);
                return !visited[size_t(yy) * w + xx] && occupied(c) &&
                       (alphaClass(c) | (reliefStep(c, o) << 2)) == key;
            };
            int rw = 1, rh = 1;
            while (x + rw < w && same(x + rw, y))
                ++rw;
            while (y + rh < h) {
                bool full = true;
                for (int xx = 0; xx < rw; ++xx)
                    if (!same(x + xx, y + rh)) {
                        full = false;
                        break;
                    }
                if (!full)
                    break;
                ++rh;
            }
            for (int yy = 0; yy < rh; ++yy)
                std::fill_n(visited.data() + size_t(y + yy) * w + x, rw, uint8_t(1));
            float x0 = left + sourceX(float(x)) * dx, x1 = left + sourceX(float(x + rw)) * dx,
                  y0 = top + sourceY(float(y)) * dy, y1 = top + sourceY(float(y + rh)) * dy;
            auto a = tex(float(x), float(y)), b = tex(float(x + rw), float(y)),
                 c = tex(float(x + rw), float(y + rh)), d = tex(float(x), float(y + rh));
            bool alpha = key % 4 == 1;
            if (!emit({x0, y0, extent[1]}, {x1, y0, extent[1]}, {x1, y1, extent[1]},
                      {x0, y1, extent[1]}, {0, 0, 1}, a, b, c, d, alpha) ||
                !emit({x0, y1, extent[0]}, {x1, y1, extent[0]}, {x1, y0, extent[0]},
                      {x0, y0, extent[0]}, {0, 0, -1}, d, c, b, a, alpha))
                return false;
        }
    // Only exposed boundary intervals are emitted. Equal neighbours have no
    // interior face; stepped neighbours expose just the taller interval.
    for (int direction = 0; direction < 4; ++direction) {
        const bool vertical = direction < 2;
        const int outer = vertical ? w : h, inner = vertical ? h : w;
        for (int i = 0; i < outer; ++i)
            for (int j = 0; j < inner;) {
                int x = vertical ? i : j, y = vertical ? j : i;
                uint32_t color = colorAt(x, y);
                if (!occupied(color)) {
                    ++j;
                    continue;
                }
                auto extent = column(p, color, o);
                const int nx = x + (direction == 0   ? -1
                                    : direction == 1 ? 1
                                                     : 0),
                          ny = y + (direction == 2   ? -1
                                    : direction == 3 ? 1
                                                     : 0);
                auto neighbor = colorAt(nx, ny);
                float lower = occupied(neighbor) ? column(p, neighbor, o)[1] : extent[0];
                lower = std::max(lower, extent[0]);
                if (lower >= extent[1] - .0001f) {
                    ++j;
                    continue;
                }
                const unsigned key = alphaClass(color) | (reliefStep(color, o) << 2);
                int run = 1;
                while (step == 1 && j + run < inner) {
                    int xx = vertical ? i : j + run, yy = vertical ? j + run : i;
                    uint32_t c = colorAt(xx, yy);
                    auto n = colorAt(
                                  xx + (direction == 0   ? -1
                                        : direction == 1 ? 1
                                                         : 0),
                                  yy + (direction == 2   ? -1
                                        : direction == 3 ? 1
                                                         : 0));
                    if (!occupied(c) || (alphaClass(c) | (reliefStep(c, o) << 2)) != key)
                        break;
                    float low = occupied(n) ? std::max(extent[0], column(p, n, o)[1]) : extent[0];
                    if (std::abs(low - lower) > .0001f)
                        break;
                    ++run;
                }
                float x0 = left + sourceX(float(x)) * dx,
                      x1 = left + sourceX(float(x + (vertical ? 1 : run))) * dx,
                      y0 = top + sourceY(float(y)) * dy,
                      y1 = top + sourceY(float(y + (vertical ? run : 1))) * dy;
                const float z0 = lower, z1 = extent[1];
                bool alpha = alphaClass(color) == 1;
                if (vertical) {
                    float u = x + .5f, xx = direction == 0 ? x0 : x1;
                    auto a = tex(u, float(y)), b = tex(u, float(y + run));
                    if (step > 1)
                        a = b = sideTexel(x, y);
                    if (direction == 0) {
                        if (!emit({xx, y0, z0}, {xx, y0, z1}, {xx, y1, z1}, {xx, y1, z0},
                                  {-1, 0, 0}, a, a, b, b, alpha))
                            return false;
                    } else if (!emit({xx, y1, z0}, {xx, y1, z1}, {xx, y0, z1}, {xx, y0, z0},
                                     {1, 0, 0}, b, b, a, a, alpha))
                        return false;
                } else {
                    float v = y + .5f, yy = direction == 2 ? y0 : y1;
                    auto a = tex(float(x), v), b = tex(float(x + run), v);
                    if (step > 1)
                        a = b = sideTexel(x, y);
                    if (direction == 2) {
                        if (!emit({x1, yy, z0}, {x1, yy, z1}, {x0, yy, z1}, {x0, yy, z0},
                                  {0, -1, 0}, b, b, a, a, alpha))
                            return false;
                    } else if (!emit({x0, yy, z0}, {x0, yy, z1}, {x1, yy, z1}, {x1, yy, z0},
                                     {0, 1, 0}, a, a, b, b, alpha))
                        return false;
                }
                j += run;
            }
    }
    return true;
}
bool Mesher::update(const Frame &f, const Options &o) {
    if (!f.valid() || o.faceLimit < 6) {
        clear();
        return false;
    }
    const size_t limit = std::min<size_t>(o.faceLimit, 200000);
    bool changed = !valid_;
    size_t faces = 0;
    const uint64_t configuration = geometryConfiguration(f, o);
    for (size_t p = 0; p < planeCount; ++p) {
        auto &layer = layers_[p];
        if (valid_ && f.planeRevisions[p] && layer.sourceRevision == f.planeRevisions[p] &&
            layer.configuration == configuration) {
            ++cacheHits;
        } else {
            const uint64_t signature = prepareLayer(f, Plane(p), o, configuration,
                                                    &layer.pixelSignature, shapePixels, shapeTexels);
            if (!valid_ || signature != layer.signature) {
                if (!buildLayer(f, p, o, layer, limit)) {
                    valid_ = false;
                    return false;
                }
                layer.signature = signature;
                ++rebuiltLayers;
                changed = true;
            } else {
                ++cacheHits;
            }
            layer.sourceRevision = f.planeRevisions[p];
            layer.configuration = configuration;
        }
        faces += (layer.opaque.size() + layer.translucent.size()) / 6;
        if (faces > limit) {
            valid_ = false;
            return false;
        }
    }
    faces_ = faces;
    // Reserve the same depth as sprites and previews enter or leave the frame.
    const auto firstPlane = !f.nativeOnly && o.mode > 1 &&
                                    (f.planes & (1u << unsigned(Plane::Background)))
                                ? Plane::Background : Plane::TerrainLow;
    const auto lastPlane = f.nativeOnly ? Plane::TerrainLow : Plane::Preview;
    const float z0 = column(firstPlane, 0xFFFFFFFF, o)[0];
    const float z1 = column(lastPlane, 0xFFFFFFFF, o)[1];
    bounds_ = {{float(f.left), float(f.top), z0},
               {float(f.left + f.coverageWidth), float(f.top + f.coverageHeight), z1}};
    valid_ = true;
    geometryStep = std::clamp(o.geometryStep, 1, 64);
    if (changed)
        ++revision;
    return true;
}
void Mesher::clear() {
    for (auto &l : layers_) {
        l.opaque.clear();
        l.translucent.clear();
        l.signature = l.pixelSignature = 0;
        l.sourceRevision = l.configuration = 0;
    }
    valid_ = false;
    faces_ = 0;
    ++revision;
}
void Mesher::discard() {
    const auto next = revision + 1;
    *this = {};
    revision = next;
}
size_t Mesher::bytes() const {
    size_t bytes = visited.capacity() +
                   (shapePixels.capacity() + shapeTexels.capacity()) * sizeof(uint32_t);
    for (auto &l : layers_)
        bytes += (l.opaque.capacity() + l.translucent.capacity()) * sizeof(Vertex);
    return bytes;
}
size_t cellBudget(int detail, bool watch, bool mobile, int effects, bool adaptive) {
    size_t value = std::array<size_t, 3>{16384, 65536, 1048576}[std::clamp(detail, 0, 2)];
    if (watch)
        value = std::min<size_t>(value, 8192);
    else if (mobile)
        value = std::min<size_t>(value, 163840);
    if (adaptive && detail < 2)
        value >>= unsigned((3 - std::clamp(effects, 0, 3)) / 2);
    return std::max<size_t>(1024, value);
}
std::array<int, 2> grid(double cw, double ch, int sw, int sh, size_t budget, int textureLimit) {
    if (!std::isfinite(cw) || !std::isfinite(ch) || cw <= 0 || ch <= 0 || sw <= 0 || sh <= 0)
        return {0, 0};
    const int capW = std::clamp(textureLimit / 2, 1, Frame::maxWidth);
    const int capH = std::clamp(textureLimit / 4, 1, Frame::maxHeight);
    const double scale = std::min({1., double(capW) / sw, double(capH) / sh,
                                  std::sqrt(double(std::max<size_t>(1, budget)) / sw / sh)});
    if (scale == 1.)
        return {sw, sh};
    auto bucket = [](double dimension) {
        const int size = std::max(1, int(dimension));
        return size >= 32 ? size / 8 * 8 : size;
    };
    int width = bucket(sw * scale), height = bucket(sh * scale);
    budget = std::max<size_t>(1, budget);
    if (size_t(width) * height > budget) {
        if (width > height)
            width = int(budget / height);
        else
            height = int(budget / width);
    }
    return {width, height};
}
std::array<int, 2> pictureSize(int width, int height, int quality, int detail, int scale,
                               int maximum, bool mobile, bool watch, int sourceWidth,
                               int sourceHeight) {
    const bool precise = detail >= 2 && !watch;
    const auto budgets = precise ? std::array<double, 3>{900000, 4000000, 12000000}
                                 : std::array<double, 3>{360000, 960000, 2400000};
    const double pixels = budgets[std::clamp(quality, 0, 2)] * (detail == 0 ? .60 : 1.) *
                          (watch ? .09 : mobile ? .50 : 1.);
    const int cap = std::clamp(maximum, 8, watch ? 512 : mobile ? 2560 : precise ? 8192 : 4096);
    const double headroom = scale > 85 ? 1. : scale > 65 ? .75 : scale > 42 ? .5 : .35;
    const double w = std::max(1, width), h = std::max(1, height);
    const double limit = std::min({1., double(cap) / w, double(cap) / h});
    const double floor = precise ? (mobile ? 1. : 2.) *
                                      std::max(sourceWidth / w, sourceHeight / h)
                                 : 0.;
    const double fit = std::min(limit, std::max(floor, std::min(limit, std::sqrt(pixels / (w * h))) *
                                                          headroom));
    if (fit >= 1.)
        return {width, height};
    return {std::min(width, std::max(8, int(w * fit) / 8 * 8)),
            std::min(height, std::max(8, int(h * fit) / 8 * 8))};
}
Matrix Matrix::identity() {
    Matrix r;
    r.v[0] = r.v[5] = r.v[10] = r.v[15] = 1;
    return r;
}
Matrix multiply(const Matrix &a, const Matrix &b) {
    Matrix r;
    for (int c = 0; c < 4; ++c)
        for (int y = 0; y < 4; ++y)
            for (int k = 0; k < 4; ++k)
                r.v[c * 4 + y] += a.v[k * 4 + y] * b.v[c * 4 + k];
    return r;
}
Vec3 transform(const Matrix &m, Vec3 p) {
    return {m.v[0] * p.x + m.v[4] * p.y + m.v[8] * p.z + m.v[12],
            m.v[1] * p.x + m.v[5] * p.y + m.v[9] * p.z + m.v[13],
            m.v[2] * p.x + m.v[6] * p.y + m.v[10] * p.z + m.v[14]};
}
Matrix pictureCamera(const Bounds &b, int width, int height, float yaw, float pitch) {
    const float rad = 3.14159265358979323846f / 180;
    yaw = std::clamp(yaw, -65.f, 65.f) * rad;
    pitch = std::clamp(pitch, -65.f, 65.f) * rad;
    Matrix y = Matrix::identity(), x = Matrix::identity(), t = Matrix::identity();
    y.v[0] = y.v[10] = std::cos(yaw);
    y.v[8] = std::sin(yaw);
    y.v[2] = -std::sin(yaw);
    x.v[5] = x.v[10] = std::cos(pitch);
    x.v[9] = -std::sin(pitch);
    x.v[6] = std::sin(pitch);
    t.v[5] = -1;
    t.v[12] = -(b.min.x + b.max.x) * .5f;
    t.v[13] = (b.min.y + b.max.y) * .5f;
    t.v[14] = -(b.min.z + b.max.z) * .5f;
    Matrix model = multiply(multiply(y, x), t);
    float ex = .5f, ey = .5f, ez = .5f;
    for (int i = 0; i < 8; ++i) {
        auto p = transform(model, {i & 1 ? b.max.x : b.min.x, i & 2 ? b.max.y : b.min.y,
                                   i & 4 ? b.max.z : b.min.z});
        ex = std::max(ex, std::abs(p.x));
        ey = std::max(ey, std::abs(p.y));
        ez = std::max(ez, std::abs(p.z));
    }
    float aspect = float(std::max(1, width)) / std::max(1, height);
    ex = std::max(ex, ey * aspect);
    ey = ex / aspect;
    Matrix projection = Matrix::identity();
    projection.v[0] = 1 / ex;
    projection.v[5] = 1 / ey;
    projection.v[10] = -1 / (ez + 1);
    return multiply(projection, model);
}
void panelTriangles(const std::vector<Vertex> &source, const Frame &f, const Options &o,
                    bool curved, float arcDegrees, std::vector<Vertex> &out) {
    out.clear();
    if (!f.valid() || source.size() % 6)
        return;
    float angle = std::clamp(arcDegrees, 10.f, 220.f) * 3.14159265358979323846f / 180,
          radius = 1 / angle, anchor = column(Plane::TerrainLow, 0xFF000000, o)[1];
    float maximum = std::max(std::abs(column(Plane::Background, 0xFF000000, o)[0] - anchor),
                             std::abs(column(Plane::Preview, 0xFFFFFFFF, o)[1] - anchor)) /
                    float(f.coverageWidth);
    float depthScale = curved ? std::min(1.f, radius * .35f / std::max(.0001f, maximum)) : 1.f;
    auto project = [&](Vertex v) {
        float x = (v.position.x - float(f.left)) / float(f.coverageWidth) - .5f,
              y = .5f - (v.position.y - float(f.top)) / float(f.coverageHeight),
              z = (v.position.z - anchor) / float(f.coverageWidth) * depthScale;
        Vec3 n{v.normal.x, -v.normal.y, v.normal.z};
        if (curved) {
            float a = x * angle, s = std::sin(a), c = std::cos(a);
            v.position = {s * (radius - z), y + .5f, radius * (1 - c) + c * z};
            v.normal = {n.x * c - n.z * s, n.y, n.x * s + n.z * c};
        } else {
            v.position = {x, y, z};
            v.normal = n;
        }
        return v;
    };
    for (size_t at = 0; at < source.size(); at += 6) {
        const auto &a = source[at], &b = source[at + 1], &c = source[at + 2], &d = source[at + 5];
        int nu = curved ? std::clamp(int(std::ceil(std::abs(b.position.x - a.position.x) /
                                                   f.coverageWidth * 96)),
                                     1, 96)
                        : 1,
            nv = curved ? std::clamp(int(std::ceil(std::abs(d.position.x - a.position.x) /
                                                   f.coverageWidth * 96)),
                                     1, 96)
                        : 1;
        auto point = [&](float u, float v) {
            Vertex result = a;
            result.position = {a.position.x + (b.position.x - a.position.x) * u +
                                   (d.position.x - a.position.x) * v,
                               a.position.y + (b.position.y - a.position.y) * u +
                                   (d.position.y - a.position.y) * v,
                               a.position.z + (b.position.z - a.position.z) * u +
                                   (d.position.z - a.position.z) * v};
            result.u = a.u + (b.u - a.u) * u + (d.u - a.u) * v;
            result.v = a.v + (b.v - a.v) * u + (d.v - a.v) * v;
            return project(result);
        };
        for (int yy = 0; yy < nv; ++yy)
            for (int xx = 0; xx < nu; ++xx) {
                std::array<Vertex, 4> q{{point(float(xx) / nu, float(yy) / nv),
                                         point(float(xx + 1) / nu, float(yy) / nv),
                                         point(float(xx + 1) / nu, float(yy + 1) / nv),
                                         point(float(xx) / nu, float(yy + 1) / nv)}};
                // Flipping screen Y changes handedness. Reverse each triangle once.
                for (int index : {0, 2, 1, 0, 3, 2})
                    out.push_back(q[index]);
            }
    }
}
} // namespace scu::voxel
