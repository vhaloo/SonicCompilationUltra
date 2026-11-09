#pragma once
#ifdef SCU_MOBILE
#include "touch_controls.h"
#include "performance.h"
#include "wear_display.h"
#include <imgui.h>
#include <memory>
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
    uint64_t metricsAt = 0;
    float safeLeft = 0, safeTop = 0, safeRight = 0, safeBottom = 0;
    FrameDeadline pacing;
    wear::LensMesh lens;
    std::unique_ptr<ImDrawList> controlsDraw;
    uint64_t layoutKey = 0;
    bool utilityBank = false, zoomInitialised = false, crownDetected = false;
    double smoothedZoom = 0;
    uint64_t zoomAt = 0, zoomHintUntil = 0;
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
    float topControlsBottom() const;
    void options();
    bool chooseRom();
    void poll();
    double refreshLimit() const;
    bool isWatch() const {
        return appIsWatch;
    }
    bool appIsWatch = false;
    double lastSubmissionMs = 0;
    float scrollDirection() const {
        return float(scroll);
    }
};
} // namespace scu
#endif
