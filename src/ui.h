#pragma once
#include "core.h"
#include "localization.h"
#include "setting_help.h"
#include <imgui.h>
#ifdef SCU_UI_TESTS
#include <map>
#endif
namespace scu {
class App {
    SDL_Texture *worldTexture = nullptr, *nativeTexture = nullptr, *bgTexture = nullptr;
    int textureW = 0, textureH = 0, nativeTW = 0, nativeTH = 0;
    std::array<uint32_t, 320 * 224> frozenBackground{};
    int backgroundStage = -1;
    bool backgroundFrozen = false;
    uint64_t lastImageVersion = ~uint64_t(0), lastPresentationHash = 0;
    bool cachedExpanded = false;
    bool browserOpen = false, folderMode = false;
    bool pathEditor = false, focusMenu = true, focusPage = false, focusBrowser = false;
    bool backRequested = false, selectRequested = false;
    bool padUsed = false, previousFullscreen = false, releaseInput = false;
    bool rewindTrigger = false;
    bool guideRequested = false, guideOpen = false;
    std::string guideKey = "view";
    uint64_t presentationClock = 0;
    unsigned presentationFrames = 0;
    double presentationFps = 0;
    double gameAccumulator = 0;
    bool releaseNavigation = true;
    int actionPopup = 0, actionSlot = 1, activePad = 0;
    std::string actionGame;
    float uiFactor = 1;
    ImGuiStyle baseStyle;
    SDL_Rect windowedBounds{SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720};
    std::string browsePath;
    char pathBuffer[4096]{};
    int tab = 0;
    void ui();
    void libraryUi();
    void optionsUi(int section);
    void browserUi();
    void hud();
    void error(const std::exception &e);
    void menuInput(bool editing);
    void syncNavigationController();
    void updateNavigationController();
    void scrollPage();
    void pathEditorUi();
    void changeDirectory(const fs::path &path);
    void requestAction(int action, int slot = 1, const std::string &game = {});
    void commitAction();
    bool button(const char *key, const ImVec2 &size = {0, 0});
    bool tick(const char *key, bool &value);
    void nextButton(const char *key);
    bool choice(const char *key, int &value, std::initializer_list<const char *> keys);
    bool slider(const char *key, int &value, int lo, int hi, const char *format = "%d");
    void control(const char *key);
    void settingGuide(bool compact = false);
    std::string navigationHint() const;

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
    bool rewindHeld = false;
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
    unsigned advanceGame(double elapsed, bool unpaced = false);
    void pan();
    void present(bool capture = false);
    void screenshot(const fs::path &path);
    void flushSettings();
    int menuTab() const {
        return tab;
    }
    void showSection(int n) {
        tab = n >= 0 && n < 9 ? n : 0;
        menu = true;
        focusPage = true;
    }
    bool browsing() const {
        return browserOpen;
    }
    bool editingPath() const {
        return pathEditor;
    }
    int confirmation() const {
        return actionPopup;
    }
    bool guideVisible() const {
        return guideOpen;
    }
    const std::string &guideTopic() const {
        return guideKey;
    }
    void showGuide(const std::string &key) {
        if (settingHelp(key.c_str())) {
            guideKey = key;
            guideRequested = true;
            menu = true;
        }
    }
#ifdef SCU_UI_TESTS
    std::map<std::string, ImGuiID> controls;
    std::map<std::string, ImVec4> controlRects;
#endif
};
} // namespace scu
