#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace scu::voxel {
// Presentation planes follow the console's observed priority. This is a
// reconstruction of pixels, never a collision mesh or an inferred game object.
enum class Plane : uint8_t {
    Background,
    TerrainLow,
    ActorsLow,
    BackgroundHigh,
    TerrainHigh,
    ActorsHigh,
    Preview,
    Count
};
constexpr size_t planeCount = size_t(Plane::Count);
struct Vec3 {
    float x = 0, y = 0, z = 0;
};
struct Vertex {
    Vec3 position, normal;
    float u = 0, v = 0;
};
struct Bounds {
    Vec3 min, max;
};
struct Options {
    int mode = 2; // 1: one relief plane, 2: priority planes, 3: stepped surface.
    float thickness = 6, spacing = 8, relief = 3;
    size_t faceLimit = 50000;
    int geometryStep = 1;
};
struct Frame {
    static constexpr int maxWidth = 3072, maxHeight = 768;
    static constexpr unsigned allPlanes = (1u << planeCount) - 1;
    int width = 0, height = 0;
    double left = 0, top = 0, coverageWidth = 0, coverageHeight = 0;
    bool hardwareLayers = false, nativeOnly = false;
    unsigned planes = allPlanes;
    // Each plane occupies one slot of a two-column, four-row texture atlas.
    std::vector<uint32_t> pixels;
    std::array<uint64_t, planeCount> planeRevisions{};
    uint64_t revision = 0;
    void configure(int w, int h, double x, double y, double cw, double ch);
    uint32_t &at(Plane plane, int x, int y);
    uint32_t at(Plane plane, int x, int y) const;
    uint32_t *row(Plane plane, int y);
    const uint32_t *row(Plane plane, int y) const;
    int textureWidth() const {
        return width * 2;
    }
    int textureHeight() const {
        return height * 4;
    }
    bool valid() const;
    void clear();
    void discard();
};
struct LayerMesh {
    std::vector<Vertex> opaque, translucent;
    uint64_t signature = 0, pixelSignature = 0;
    uint64_t sourceRevision = 0, configuration = 0;
};
// Geometry is keyed by occupancy, alpha class, priority and (for sculpted mode)
// quantized relief, not palette colours themselves.
class Mesher {
    std::array<LayerMesh, planeCount> layers_;
    std::vector<uint8_t> visited;
    std::vector<uint32_t> shapePixels, shapeTexels;
    bool valid_ = false;
    Bounds bounds_{};
    size_t faces_ = 0;
    bool buildLayer(const Frame &, size_t plane, const Options &, LayerMesh &, size_t limit);

  public:
    uint64_t revision = 0, cacheHits = 0, rebuiltLayers = 0;
    int geometryStep = 1;
    bool update(const Frame &, const Options &);
    void clear();
    void discard();
    bool valid() const {
        return valid_;
    }
    const std::array<LayerMesh, planeCount> &layers() const {
        return layers_;
    }
    const Bounds &bounds() const {
        return bounds_;
    }
    size_t faces() const {
        return faces_;
    }
    size_t bytes() const;
};
// Column extents never cross the next priority plane. Black is an opaque
// colour, not a transparency heuristic. Alpha zero alone means no voxel.
std::array<float, 2> column(Plane, uint32_t color, const Options &);
uint64_t shapeSignature(const Frame &, Plane, const Options &, uint64_t *pixelSignature = nullptr);
size_t cellBudget(int detail, bool watch, bool mobile, int effects, bool adaptive);
std::array<int, 2> grid(double coverageWidth, double coverageHeight, int sourceWidth,
                        int sourceHeight, size_t budget, int textureLimit = 2048);
std::array<int, 2> pictureSize(int width, int height, int quality, int detail, int scale,
                               int maximum, bool mobile, bool watch, int sourceWidth = 0,
                               int sourceHeight = 0);
struct Matrix {
    std::array<float, 16> v{};
    static Matrix identity();
};
Matrix multiply(const Matrix &, const Matrix &);
Vec3 transform(const Matrix &, Vec3);
Matrix pictureCamera(const Bounds &, int width, int height, float yaw, float pitch);
// Convert world pixels into a unit presentation panel. Curved panels subdivide
// wide faces before bending, so a merged rectangle cannot cut across the arc.
void panelTriangles(const std::vector<Vertex> &, const Frame &, const Options &, bool curved,
                    float arcDegrees, std::vector<Vertex> &output);
} // namespace scu::voxel
