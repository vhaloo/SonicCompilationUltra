#pragma once
#include <SDL.h>
#include <cstring>

namespace scu {
inline void setTextureSampling(SDL_Renderer *renderer, SDL_Texture *texture, SDL_ScaleMode mode) {
    SDL_ScaleMode current;
    if (SDL_GetTextureScaleMode(texture, &current) != 0 || current == mode)
        return;
    // SDL2 changes the GL binding immediately, even with pending draws, and
    // its filter setter does not invalidate the backend's last-bound texture.
    SDL_RenderFlush(renderer);
    SDL_SetTextureScaleMode(texture, mode);
    SDL_RendererInfo info{};
    if (SDL_GetRendererInfo(renderer, &info) == 0 && info.name &&
        std::strncmp(info.name, "opengl", 6) == 0)
        SDL_GL_UnbindTexture(texture);
}
} // namespace scu
