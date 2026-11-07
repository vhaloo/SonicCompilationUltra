#include "voxel.h"
#include "voxel_export.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace scu::voxel;
static void require(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
static void validate(const Frame &frame, const Mesher &mesh) {
    require(mesh.valid(), "complete mesh");
    for (const auto &layer : mesh.layers())
        for (const auto *vertices : {&layer.opaque, &layer.translucent}) {
            require(vertices->size() % 6 == 0, "whole quads");
            for (size_t i = 0; i < vertices->size(); i += 3) {
                const auto &a = (*vertices)[i], &b = (*vertices)[i + 1], &c = (*vertices)[i + 2];
                Vec3 ab{b.position.x - a.position.x, b.position.y - a.position.y,
                        b.position.z - a.position.z},
                    ac{c.position.x - a.position.x, c.position.y - a.position.y,
                       c.position.z - a.position.z};
                Vec3 cross{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z,
                           ab.x * ac.y - ab.y * ac.x};
                require(cross.x * a.normal.x + cross.y * a.normal.y + cross.z * a.normal.z > 0,
                        "outward non-degenerate triangles");
            }
            for (const auto &v : *vertices) {
                require(std::isfinite(v.position.x) && std::isfinite(v.position.y) &&
                            std::isfinite(v.position.z),
                        "finite positions");
                require(v.u >= 0 && v.u <= 1 && v.v >= 0 && v.v <= 1,
                        "bounded texture coordinates");
            }
        }
    for (auto size :
         {std::array<int, 2>{320, 180}, {1920, 1080}, {5760, 1080}, {438, 438}, {128, 96}})
        for (float yaw : {-60.f, 0.f, 37.f, 60.f})
            for (float pitch : {-60.f, 0.f, 41.f, 60.f}) {
                auto camera = pictureCamera(mesh.bounds(), size[0], size[1], yaw, pitch);
                for (int i = 0; i < 8; ++i) {
                    const auto &b = mesh.bounds();
                    auto p =
                        transform(camera, {i & 1 ? b.max.x : b.min.x, i & 2 ? b.max.y : b.min.y,
                                           i & 4 ? b.max.z : b.min.z});
                    require(std::abs(p.x) <= 1.0001f && std::abs(p.y) <= 1.0001f &&
                                std::abs(p.z) <= 1.0001f,
                            "camera never crops volume");
                }
            }
}
int main() {
    size_t scenes = 0;
    for (int mode : {1, 2, 3})
        for (int width : {1, 7, 32, 65})
            for (int height : {1, 9, 32})
                for (int pattern : {0, 1, 2, 3, 4}) {
                    Frame f;
                    f.configure(width, height, -17, 23, width * 1.25, height * .75);
                    for (int y = 0; y < height; ++y)
                        for (int x = 0; x < width; ++x) {
                            uint32_t c =
                                pattern == 0
                                    ? 0xFF000000
                                    : 0xFF000000 |
                                          uint32_t((x * 43 + y * 101) * 0x010203) & 0xFFFFFF;
                            if (pattern == 1 && (x + y) % 2)
                                c = 0;
                            if (pattern == 2 && x > width / 3 && x < 2 * width / 3 &&
                                y > height / 3 && y < 2 * height / 3)
                                c = 0;
                            if (pattern == 3)
                                c = (c & 0xFFFFFF) | 0x68000000;
                            if (pattern == 4)
                                c = 0;
                            f.at(Plane::TerrainLow, x, y) = c;
                        }
                    Mesher mesh;
                    Options options;
                    options.mode = mode;
                    options.faceLimit = 100000;
                    require(mesh.update(f, options), "generated scene fits budget");
                    validate(f, mesh);
                    if (pattern == 0)
                        require(mesh.faces() == 6,
                                "solid black merges to six faces, never disappears");
                    if (pattern == 3 && mode != 3)
                        require(mesh.faces() == 6, "transparent volume has no internal faces");
                    if (pattern == 4)
                        require(mesh.faces() == 0, "empty scene has no artificial solid screen");
                    auto revision = mesh.revision;
                    require(mesh.update(f, options) && revision == mesh.revision,
                            "unchanged frame reuses complete geometry");
                    if (mode != 3) {
                        std::array<uint64_t, planeCount> before;
                        for (size_t p = 0; p < planeCount; ++p)
                            before[p] = mesh.layers()[p].pixelSignature;
                        for (auto &c : f.pixels)
                            if (c >> 24)
                                c = (c & 0xFF000000) | ((c ^ 0x00E38AB1) & 0xFFFFFF);
                        require(mesh.update(f, options) && revision == mesh.revision,
                                "palette-only change preserves geometry");
                        for (size_t p = 0; p < planeCount; ++p)
                            require((before[p] != mesh.layers()[p].pixelSignature) ==
                                        (p == size_t(Plane::TerrainLow) && pattern != 4),
                                    "only the recoloured atlas plane needs an upload");
                    }
                    for (bool curved : {false, true})
                        for (float angle : {10.f, 90.f, 150.f, 220.f}) {
                            std::vector<Vertex> panel;
                            for (const auto &layer : mesh.layers())
                                for (const auto *vertices : {&layer.opaque, &layer.translucent}) {
                                    panelTriangles(*vertices, f, options, curved, angle, panel);
                                    require(panel.size() % 6 == 0,
                                            "curved panel preserves complete faces");
                                    for (size_t i = 0; i < panel.size(); i += 3) {
                                        const auto &a = panel[i], &b = panel[i + 1],
                                                   &c = panel[i + 2];
                                        Vec3 ab{b.position.x - a.position.x,
                                                b.position.y - a.position.y,
                                                b.position.z - a.position.z},
                                            ac{c.position.x - a.position.x,
                                               c.position.y - a.position.y,
                                               c.position.z - a.position.z};
                                        Vec3 n{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z,
                                               ab.x * ac.y - ab.y * ac.x};
                                        require(
                                            n.x * a.normal.x + n.y * a.normal.y + n.z * a.normal.z >
                                                0,
                                            "panel winding and outward normals survive bending");
                                    }
                                    for (const auto &v : panel) {
                                        require(std::isfinite(v.position.x) &&
                                                    std::isfinite(v.position.y) &&
                                                    std::isfinite(v.position.z),
                                                "curved panel coordinates finite");
                                        require(std::abs(v.normal.x * v.normal.x +
                                                         v.normal.y * v.normal.y +
                                                         v.normal.z * v.normal.z - 1) < .001f,
                                                "curved normals remain unit length");
                                    }
                                }
                        }
                    ++scenes;
                }
    for (int thickness : {1, 6, 32})
        for (int spacing : {0, 8, 64})
            for (int relief : {0, 3, 12}) {
                Options o;
                o.mode = 3;
                o.thickness = float(thickness);
                o.spacing = float(spacing);
                o.relief = float(relief);
                for (size_t p = 0; p + 1 < planeCount; ++p)
                    require(column(Plane(p), 0xFFFFFFFF, o)[1] <
                                column(Plane(p + 1), 0xFF000000, o)[0],
                            "priority bands never intersect");
            }
    for (bool watch : {false, true})
        for (bool mobile : {false, true})
            for (int detail = 0; detail < 4; ++detail)
                for (int effects = 0; effects < 4; ++effects)
                    for (auto size :
                         {std::array<int, 2>{320, 224}, {5760, 1080}, {32768, 8192}, {1, 8192}}) {
                        auto budget = cellBudget(detail, watch, mobile, effects, true);
                        auto d = grid(size[0], size[1], size[0], size[1], budget);
                        require(d[0] > 0 && d[1] > 0 && d[0] <= 1024 && d[1] <= 512 &&
                                    size_t(d[0]) * d[1] <= budget,
                                "all platform grid budgets bounded");
                    }
    Frame f;
    f.configure(32, 32, 0, 0, 32, 32);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x)
            if ((x + y) % 2 == 0)
                f.at(Plane::ActorsHigh, x, y) = 0xFFFFFFFF;
    Mesher mesh;
    Options o;
    o.faceLimit = 6;
    require(!mesh.update(f, o) && !mesh.valid(), "over-budget geometry is rejected as a whole");
    f.configure(1, 1, 0, 0, 32, 32);
    f.at(Plane::ActorsHigh, 0, 0) = 0xFFFFFFFF;
    require(mesh.update(f, o) && mesh.valid() && mesh.faces() == 6,
            "coarser complete scene recovers after budget rejection");
    const auto exportDir =
        std::filesystem::current_path() /
        ("generated-export-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto pixels = f.pixels;
    exportScene(f, mesh, exportDir);
    require(pixels == f.pixels, "export never mutates the source picture");
    std::ifstream model(exportDir / "scene.obj");
    std::string line;
    size_t faces = 0, positions = 0, uvs = 0, normals = 0;
    while (std::getline(model, line)) {
        faces += line.rfind("f ", 0) == 0;
        positions += line.rfind("v ", 0) == 0;
        uvs += line.rfind("vt ", 0) == 0;
        normals += line.rfind("vn ", 0) == 0;
    }
    require(faces == 12 && positions == 36 && uvs == positions && normals == positions,
            "export includes complete indexed mesh and UV/normal data");
    require(std::filesystem::file_size(exportDir / "pixels.tga") == 18 + f.pixels.size() * 4,
            "export texture preserves RGBA atlas size");
    bool refused = false;
    try {
        exportScene(f, mesh, exportDir);
    } catch (...) {
        refused = true;
    }
    require(refused, "existing exports are never overwritten");
    f.configure(0, 2, 0, 0, 1, 1);
    require(!f.valid() && !mesh.update(f, o), "invalid dimensions rejected");
    f.configure(2, 2, 0, 0, std::numeric_limits<double>::infinity(), 1);
    require(!f.valid(), "non-finite input rejected");
    std::printf("PASS %zu generated voxel scenes; winding, holes, alpha, colour cache, priority "
                "bands, platform budgets, eight flat/curved panel transforms and 80 camera "
                "projections per scene. No SDL, emulator, ROM or device.\n",
                scenes);
}
