#pragma once
#include "world.h"
#include <SDL.h>
namespace scu {
// Renderer-neutral furniture frames for SDL software, D3D and OpenGL backends.
class DesktopFrame {
    std::vector<SDL_Vertex> effects;
    std::array<int, 7> effectKey{};

  public:
    void draw(SDL_Renderer *renderer, SDL_Texture *level, SDL_Texture *native, int nativeWidth,
              int nativeHeight, const Scene &scene, const Canvas &canvas, const Settings &settings,
              int width, int height);
};
} // namespace scu
