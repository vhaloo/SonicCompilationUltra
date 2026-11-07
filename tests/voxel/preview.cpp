// Original fixture art rendered by the actual procedural mesh. This executable
// has no SDL, emulator or cartridge dependency and is never part of the app.
#include "voxel.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
using namespace scu::voxel;
namespace fs = std::filesystem;
constexpr int outputW = 960, outputH = 540;
static uint32_t over(uint32_t a, uint32_t b) {
    unsigned aa = a >> 24, bb = (b >> 24) * (255 - aa) / 255, total = aa + bb;
    if (!total)
        return 0;
    uint32_t out = total << 24;
    for (unsigned s : {0u, 8u, 16u})
        out |= (((((a >> s) & 255) * aa + ((b >> s) & 255) * bb) / total)) << s;
    return out;
}
static Frame original() {
    Frame f;
    f.configure(256, 144, 0, 0, 256, 144);
    f.hardwareLayers = true;
    auto rectangle = [&](Plane p, int x, int y, int w, int h, uint32_t color) {
        for (int yy = std::max(0, y); yy < std::min(144, y + h); ++yy)
            for (int xx = std::max(0, x); xx < std::min(256, x + w); ++xx)
                f.at(p, xx, yy) = color;
    };
    // Deliberately original colours and geometry, without a game silhouette.
    for (int y = 0; y < 144; ++y)
        for (int x = 0; x < 256; ++x) {
            bool city = (x > 18 && x < 50 && y > 46) || (x > 64 && x < 97 && y > 35) ||
                        (x > 113 && x < 139 && y > 57) || (x > 157 && x < 190 && y > 40) ||
                        (x > 209 && x < 244 && y > 25);
            if (city && y < 120)
                f.at(Plane::Background, x, y) = 0xFF1E415C + uint32_t(y / 8) * 0x00020201;
            if (city && y < 118 && x % 13 < 3 && y % 11 < 3)
                f.at(Plane::Background, x, y) = 0xFF50849B;
        }
    for (int x = 0; x < 256; ++x) {
        int floor = 113 - (x / 32 % 3) * 6;
        for (int y = floor; y < 144; ++y)
            f.at(Plane::TerrainLow, x, y) =
                y < floor + 3 ? 0xFFA6E8CE : ((x / 8 + y / 8) % 2 ? 0xFF276F72 : 0xFF245C69);
        if (x % 8 == 0)
            for (int y = floor + 4; y < 144; ++y)
                f.at(Plane::TerrainLow, x, y) = 0xFF1C4F60;
    }
    for (auto r : {std::array<int, 4>{19, 81, 49, 6}, {86, 62, 45, 6}, {165, 86, 61, 6}}) {
        rectangle(Plane::TerrainLow, r[0], r[1] + 3, r[2], r[3], 0xFF2B8E91);
        rectangle(Plane::TerrainLow, r[0], r[1], r[2], 3, 0xFF99E4CE);
        for (int x = r[0] + 3; x < r[0] + r[2] - 3; x += 9)
            rectangle(Plane::TerrainLow, x, r[1] + 5, 4, 3, 0xFF235B6D);
    }
    // A pierced foreground arch and a bridge prove that holes stay holes.
    for (int y = 85; y < 123; ++y)
        for (int x = 126; x < 161; ++x)
            if (x < 133 || x > 153 || y < 92)
                f.at(Plane::TerrainHigh, x, y) =
                    y % 7 == 0 ? 0xFFE8A965 : ((x + y) % 9 < 3 ? 0xFF885A75 : 0xFFAB7584);
    for (int x = 23; x < 86; ++x) {
        rectangle(Plane::TerrainHigh, x, 122, 1, 3, x % 7 < 5 ? 0xFFD9AD6B : 0xFF75586D);
        if (x % 13 == 0)
            rectangle(Plane::TerrainHigh, x, 116, 2, 10, 0xFFD9AD6B);
    }
    auto gem = [&](Plane p, int cx, int cy, uint32_t color) {
        for (int y = -7; y <= 7; ++y)
            for (int x = -5; x <= 5; ++x) {
                if (std::abs(x) * 7 + std::abs(y) * 5 > 35)
                    continue;
                f.at(p, cx + x, cy + y) =
                    x < 0 ? color
                          : ((color & 0xFF000000) | (((color & 0x00FEFEFE) >> 1) + 0x00202020));
            }
    };
    gem(Plane::ActorsLow, 43, 71, 0xFFFFD27E);
    gem(Plane::ActorsLow, 110, 52, 0xFFFFD27E);
    gem(Plane::ActorsHigh, 188, 75, 0xFF9BDCEC);
    rectangle(Plane::ActorsHigh, 75, 97, 12, 12, 0xFFCF8CB6);
    rectangle(Plane::ActorsHigh, 77, 95, 8, 3, 0xFFE9B9D3);
    rectangle(Plane::ActorsHigh, 77, 101, 2, 2, 0xFF152D4A);
    rectangle(Plane::ActorsHigh, 83, 101, 2, 2, 0xFF152D4A);
    gem(Plane::Preview, 222, 65, 0x60D8B4FF);
    return f;
}
static void save(const fs::path &path, const std::vector<uint32_t> &pixels) {
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << outputW << ' ' << outputH << "\n255\n";
    for (auto color : pixels)
        for (unsigned shift : {16u, 8u, 0u})
            file.put(char(color >> shift));
    if (!file)
        throw std::runtime_error("Cannot save generated fixture");
}
static std::vector<uint32_t> background() {
    std::vector<uint32_t> image(outputW * outputH);
    for (int y = 0; y < outputH; ++y)
        for (int x = 0; x < outputW; ++x) {
            float glow = std::exp(-((x - outputW * .5f) * (x - outputW * .5f) +
                                    (y - outputH * .55f) * (y - outputH * .55f)) /
                                  180000.f);
            image[size_t(y) * outputW + x] = 0xFF000000 | uint32_t(7 + glow * 8) << 16 |
                                             uint32_t(16 + glow * 16) << 8 |
                                             uint32_t(31 + glow * 22);
        }
    return image;
}
struct Point {
    float x, y, z, u, v;
    Vec3 normal;
};
static void render(const Frame &frame, const Mesher &mesh, float yaw, float pitch, int light,
                   const fs::path &path) {
    auto image = background();
    std::vector<float> depth(outputW * outputH, 1e20f);
    auto camera = pictureCamera(mesh.bounds(), outputW, outputH, yaw, pitch);
    const float inverse = 1 / std::sqrt(.35f * .35f + .7f * .7f + .85f * .85f);
    auto point = [&](const Vertex &v) {
        auto p = transform(camera, v.position);
        return Point{
            (p.x * .5f + .5f) * outputW, (.5f - p.y * .5f) * outputH, p.z, v.u, v.v, v.normal};
    };
    for (int alpha = 0; alpha < 2; ++alpha)
        for (const auto &layer : mesh.layers()) {
            const auto &vertices = alpha ? layer.translucent : layer.opaque;
            for (size_t at = 0; at < vertices.size(); at += 3) {
                auto a = point(vertices[at]), b = point(vertices[at + 1]),
                     c = point(vertices[at + 2]);
                float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                if (area <= 1e-8f)
                    continue;
                int x0 = std::max(0, int(std::floor(std::min({a.x, b.x, c.x})))),
                    x1 = std::min(outputW - 1, int(std::ceil(std::max({a.x, b.x, c.x})))),
                    y0 = std::max(0, int(std::floor(std::min({a.y, b.y, c.y})))),
                    y1 = std::min(outputH - 1, int(std::ceil(std::max({a.y, b.y, c.y}))));
                float shade =
                    (1 - light * .01f) +
                    (light * .01f) *
                        (.48f + .52f * std::max(0.f, (-a.normal.x * .35f - a.normal.y * .7f +
                                                      a.normal.z * .85f) *
                                                         inverse));
                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x) {
                        float px = x + .5f, py = y + .5f,
                              wa = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) / area,
                              wb = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) / area,
                              wc = 1 - wa - wb;
                        if (wa < 0 || wb < 0 || wc < 0)
                            continue;
                        float z = wa * a.z + wb * b.z + wc * c.z;
                        size_t index = size_t(y) * outputW + x;
                        if (z > depth[index] + 1e-6f)
                            continue;
                        float u = wa * a.u + wb * b.u + wc * c.u,
                              v = wa * a.v + wb * b.v + wc * c.v;
                        int tx = std::clamp(int(u * frame.textureWidth()), 0,
                                            frame.textureWidth() - 1),
                            ty = std::clamp(int(v * frame.textureHeight()), 0,
                                            frame.textureHeight() - 1);
                        auto source = frame.pixels[size_t(ty) * frame.textureWidth() + tx];
                        if (!(source >> 24))
                            continue;
                        uint32_t color = source & 0xFF000000;
                        for (unsigned shift : {0u, 8u, 16u})
                            color |=
                                uint32_t(std::clamp(int(((source >> shift) & 255) * shade), 0, 255))
                                << shift;
                        image[index] = over(color, image[index]);
                        if (!alpha)
                            depth[index] = z;
                    }
            }
        }
    save(path, image);
}
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Provide a local output directory for original generated fixtures.\n";
        return 2;
    }
    try {
        fs::path out = fs::u8path(argv[1]);
        fs::create_directories(out);
        auto frame = original();
        auto flat = background();
        for (int y = 0; y < outputH; ++y)
            for (int x = 0; x < outputW; ++x) {
                int sx = x * frame.width / outputW, sy = y * frame.height / outputH;
                auto &pixel = flat[size_t(y) * outputW + x];
                for (size_t p = 0; p < planeCount; ++p)
                    pixel = over(frame.at(Plane(p), sx, sy), pixel);
            }
        save(out / "flat.ppm", flat);
        for (int mode : {2, 3}) {
            Mesher mesh;
            Options options;
            options.mode = mode;
            options.thickness = 6;
            options.spacing = 8;
            options.relief = 5;
            options.faceLimit = 100000;
            if (!mesh.update(frame, options))
                return 3;
            render(frame, mesh, -20, 16, 70, out / (mode == 2 ? "layered.ppm" : "sculpted.ppm"));
        }
        std::cout << "Generated original fixture views from the procedural mesh; no app, SDL, "
                     "emulator, ROM or device.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
