#include "voxel_gpu.h"
#include "texture_sampling.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <set>
#include <stdexcept>
using namespace scu;

static void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(std::string(message) + " " + SDL_GetError());
}
static void checkSamplerSwitch(SDL_Renderer *renderer) {
    auto texture = [&](uint32_t color) {
        auto *t =
            SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        require(t && SDL_UpdateTexture(t, nullptr, &color, sizeof color) == 0, "solid texture");
        return t;
    };
    auto *level = texture(0xff1188cc), *native = texture(0xffff6622);
    SDL_Rect pixel{0, 0, 1, 1};
    for (auto filter : {SDL_ScaleModeNearest, SDL_ScaleModeLinear, SDL_ScaleModeNearest}) {
        require(SDL_RenderCopy(renderer, level, nullptr, nullptr) == 0, "draw surround");
        SDL_RenderFlush(renderer);
        setTextureSampling(renderer, native, filter);
        require(SDL_RenderCopy(renderer, level, nullptr, nullptr) == 0, "redraw surround");
        uint32_t value = 0;
        require(SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_ARGB8888, &value,
                                     sizeof value) == 0 &&
                    (value & 0xffffff) == 0x1188cc,
                "native filter cannot replace bound surround");
    }
    SDL_DestroyTexture(native);
    SDL_DestroyTexture(level);
}
int main() {
    SDL_SetMainReady();
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    int result = 1;
    try {
        require(SDL_Init(SDL_INIT_VIDEO) == 0, "SDL video");
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
        window = SDL_CreateWindow("Volume rendering test", 0, 0, 480, 240,
                                  SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
        require(window, "OpenGL window");
        renderer =
            SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        require(renderer, "OpenGL renderer");
        SDL_RendererInfo info{};
        SDL_GetRendererInfo(renderer, &info);
        require(std::string(info.name) == "opengl", "OpenGL backend");
        checkSamplerSwitch(renderer);
        voxel::Frame f;
        f.configure(48, 32, 0, 0, 48, 32);
        f.planes = voxel::Frame::allPlanes;
        for (int y = 3; y < 30; ++y)
            for (int x = 3; x < 45; ++x) {
                if (y > 21 || (x > 25 && x < 31 && y > 11))
                    f.at(voxel::Plane::TerrainLow, x, y) = 0xff167846;
                if ((x - 12) * (x - 12) + (y - 14) * (y - 14) < 42)
                    f.at(voxel::Plane::ActorsHigh, x, y) = x < 12 ? 0xffee3040 : 0xffffb730;
                if (x > 32 && x < 42 && y > 6 && y < 11)
                    f.at(voxel::Plane::Preview, x, y) = 0x9270c8ff;
            }
        VoxelGpu gpu;
        voxel::Mesher mesh;
        Settings settings;
        for (int mode = 1; mode <= 3; ++mode) {
            voxel::Options opts;
            opts.mode = mode;
            require(mesh.update(f, opts), "closed mesh");
            settings.voxelMode = mode;
            settings.voxelDetail = 3;
            for (auto size : {std::array<int, 2>{320, 240}, {480, 120}, {160, 144}}) {
                auto sample = [&](int yaw, int pitch) {
                    settings.voxelYaw = yaw;
                    settings.voxelPitch = pitch;
                    auto image = gpu.render(renderer, f, mesh, settings, size[0], size[1]);
                    require(image, "actual GL voxel picture");
                    SDL_SetRenderDrawColor(renderer, 3, 5, 7, 255);
                    require(SDL_RenderClear(renderer) == 0 &&
                                SDL_RenderCopy(renderer, image, nullptr, nullptr) == 0,
                            "display texture");
                    std::vector<uint32_t> pixels(480 * 240);
                    require(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888,
                                                 pixels.data(), 480 * 4) == 0,
                            "read pixels");
                    size_t drawn = 0;
                    uint64_t hash = 1469598103934665603ULL;
                    for (auto p : pixels) {
                        drawn += (p & 0xffffff) != 0x030507;
                        hash = (hash ^ (p & 0xffffff)) * 1099511628211ULL;
                    }
                    require(drawn > 70, "visible mesh");
                    return hash;
                };
                auto front = sample(0, 0);
                std::set<uint64_t> views{
                    front,           sample(-30, 22), sample(90, 0),    sample(180, 0),
                    sample(-90, 35), sample(65, 85),  sample(-150, -85)};
                require(views.size() >= 6, "distinct front, side, rear and elevated views");
                gpu.release();
                require(sample(0, 0) == front, "pixel-identical front after resource recreation");
            }
        }
        gpu.release();
        std::puts("PASS actual desktop OpenGL: three volume modes, three aspect ratios, seven "
                  "camera positions and resource restoration.");
        result = 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
    }
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
