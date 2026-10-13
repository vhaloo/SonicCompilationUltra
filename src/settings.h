#pragma once
#include "rom.h"
namespace scu {
struct Settings {
    int language = -1, view = 2, zoom = 1, camera = 0, background = 0, blur = 7, brightness = 60;
    int hud = 0, hudScale = 0, nativeScale = 0, workers = 0, volume = 70, quality = 1, uiScale = 0;
    int slot = 1, region = 0, renderer = 0, scanlines = 0, panSpeed = 6, deadzone = 12000;
    int menuLayout = 0, display = -1, padLabels = 0, jumpLayout = 0;
    int overviewStyle = 0;
    bool fullscreen = true, span = false, vsync = true, parallax = true, locator = true,
         minimap = false;
    bool help = true, spindash = false, noSpriteLimit = false, integerScale = true;
    bool mute = false, pauseUnfocused = true, performance = false, reduceMotion = false;
    bool autosave = true, rewind = false, ghost = false, invincible = false, infiniteAir = false;
    bool fastBoot = true;
    bool lockon = false;
    bool safeEffects = true, confirmQuit = true;
    bool confirmActions = true, quickControls = true, readyOnly = false, highContrast = false;
    bool playerInset = true;
    int left = 0, top = 0;
    std::string lastGame = "sonic1", lastDirectory;
    void load(const fs::path &dir);
    void save(const fs::path &dir) const;
    void preset(int n);
};
} // namespace scu
