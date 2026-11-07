#pragma once
#include "voxel.h"
#include "settings.h"
#include <SDL.h>
#include <memory>
namespace scu {
// A depth-tested GL/ES offscreen picture shared by SDL desktop and mobile.
// All entry points are loaded from the existing SDL context; no new window,
// graphics SDK, process, readback or display permission is required.
class VoxelGpu {
    struct Impl;
    std::unique_ptr<Impl> impl;

  public:
    VoxelGpu();
    ~VoxelGpu();
    SDL_Texture *render(SDL_Renderer *, const voxel::Frame &, const voxel::Mesher &,
                        const Settings &, int width, int height);
    void release();
    void invalidate();
    const char *status() const;
};
} // namespace scu
