#pragma once
#include "core.h"
#include "localization.h"
#include <imgui.h>
namespace scu {
class App {
    SDL_Texture *worldTexture = nullptr, *nativeTexture = nullptr, *bgTexture = nullptr;
    int textureW = 0, textureH = 0, nativeTW = 0, nativeTH = 0;
    std::array<uint32_t, 320 * 224> frozenBackground{};
    int backgroundStage = -1;
    bool backgroundFrozen = false;
    uint64_t lastImageVersion = ~uint64_t(0), lastPresentationHash = 0;
    bool cachedExpanded = false;
    bool browserOpen = false, folderMode = false, quitPopup = false;
    std::string browsePath;
    char pathBuffer[4096]{};
    int tab = 0;
    void ui();
    void libraryUi();
    void optionsUi(int section);
    void browserUi();
    void hud();
    void error(const std::exception &e);

  public:
    Settings settings;
    Library library;
    Core core;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_GameController *pads[2]{};
    uint16_t pendingButtons[2]{};
    bool running = true, menu = true, focused = true, headless = false, dirty = false;
    bool screenshotRequested = false;
    int language = 0, drawableW = 1280, drawableH = 720;
    double measuredFps = 0;
    std::string notice, capturePath;
    fs::path basePath;
    explicit App(const fs::path &dir) : library(dir), core(settings, library) {}
    ~App();
    void initialise(const fs::path &base, bool hidden = false);
    void refreshControllers();
    void toggleMenu();
    void applyDisplay();
    void beginBrowser(bool folder);
    void import(const fs::path &path, bool folder = false);
    bool play(const std::string &id);
    void event(const SDL_Event &e);
    void pollInput();
    void pan();
    void present(bool capture = false);
    void screenshot(const fs::path &path);
    void flushSettings();
};
} // namespace scu
