#pragma once
#ifdef SCU_MOBILE
#include "touch_controls.h"
#include <SDL.h>
#include <cstdint>
namespace scu {
class App;
class MobilePresentation {
    App &app;
    SDL_Texture *canvas = nullptr;
    touch::Layout layout;
    touch::Input input;
    int width = 0, height = 0, textureW = 0, textureH = 0;
    float density = 1;
    float displayHz = 60;
    bool wasMenu = true, wasControls = true;
    int64_t pointer = 0;
    bool pointerDown = false;
    int displayConfig = -1;
    uint64_t frameStart = 0;
    int scroll = 0;
    void clearInput();
    void action(touch::Action action);
    void drawControls();

  public:
    explicit MobilePresentation(App &app);
    ~MobilePresentation();
    void beginFrame();
    void endFrame();
    bool event(const SDL_Event &e);
    void prepareUi();
    void logicalSize(int &width, int &height) const;
    void options();
    void chooseRom();
    void poll();
    float scrollDirection() const {
        return float(scroll);
    }
};
} // namespace scu
#endif
