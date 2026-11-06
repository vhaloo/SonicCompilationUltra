#pragma once
#include <array>
#include <cstdint>

namespace scu::wear {
struct View {
    double left = 0, top = 0, width = 320, height = 224;
};
// Zero is the complete known stage; one is the untouched console viewport.
View camera(double zoom, int stageW, int stageH, int nativeW, int nativeH, int cameraX, int cameraY,
            double aspect);
double crownZoom(double current, double delta, int speed, bool reverse);
struct Vertex {
    float x, y, u, v;
};
// A cached indexed mesh, with no per-frame heap allocation. Only game pixels
// pass through the lens; touch targets and the options menu remain flat.
class LensMesh {
    int width = 0, height = 0;
    float aspect = 0, amount = -1, strength = -1;
    bool round = false;

  public:
    static constexpr int segments = 24, side = segments + 1;
    std::array<Vertex, side * side> vertices{};
    std::array<int, segments * segments * 6> indices{};
    uint64_t revision = 0;
    LensMesh();
    bool update(int w, int h, float sourceAspect, bool circular, float blend, float magnification);
    Vertex project(float u, float v) const;
};
} // namespace scu::wear
