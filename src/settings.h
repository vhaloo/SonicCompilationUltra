#pragma once
#include "rom.h"
#include <algorithm>
namespace scu {
struct Settings {
    int language = -1, view = 2, zoom = 1, camera = 0, background = 0, blur = 7, brightness = 60;
    int hud = 0, hudScale = 0, nativeScale = 0, workers = 0, volume = 70, quality = 1, uiScale = 0;
    int slot = 1, region = 0, renderer = 0, scanlines = 0, panSpeed = 6, deadzone = 12000;
    int menuLayout = 0, display = -1, padLabels = 0, jumpLayout = 0;
    int rewindButton = 0, rewindMemory = 1;
    int overviewStyle = 0;
    int enemyOpacity = 35, enemyStyle = 0;
    int zoomOut = 0, panoramaHeight = 224;
    int hintCorner = 0, hintScale = 100, hintOpacity = 80, hintMargin = 8;
    int hintDetail = 0, hintSeconds = 0, noticeSeconds = 6;
    bool rewindIndicator = true;
    int questWidth = 320, questDistance = 220, questHeight = 0, questTilt = 0;
    int questResolution = 1, questAnchor = 0, questHaptics = 30;
    int questMode = 0, questWallHeight = 200, questWallGap = 0, questRefresh = 90;
    int questWallDetail = 1, questMenuDistance = 100;
    int questScale = 100, questResizeSpeed = 100;
    int questCabinetWidth = 140, questCabinetHeight = 85, questCabinetTilt = 12;
    int questTvWidth = 160, questCurve = 90;
    int questCrt = 2, questCrtMask = 1, questCrtStrength = 80, questCrtCurve = 35;
    int questGlass = 35, questSurroundBrightness = 100, questEnvironment = 1;
    int questDefaultsRevision = 7, questEyeDetail = 1, questRemoteScale = 100,
        questOptionsScale = 100;
    int questInsetWidth = 60, questHandJoystick = 0, questTvModel = 0;
    bool questTransparent = true, questRemote = true, questHandTracking = true;
    bool questInsetCrt = true, questCrtCoplanar = true, spinballTriggers = true;
    bool questMoveRotate = true, questControlHints = true;
    bool objectPreview = true, objectOrigins = true, frozenPreviews = true;
    // Frame-only alpha mode; never persisted and never edits native game pixels.
    bool projectionTransparent = false;
    int desktopPresentation = 0, desktopFrameScale = 100, desktopCrtStrength = 40,
        desktopCabinetAngle = 35;
    bool desktopSurround = true, desktopCrtMask = true;
    bool questSpinballAuto = true, questSurround = true, questWorldBackdrop = true;
    bool questAutoWall = true, questWallLocked = true, questAdaptive = true;
    bool questPauseLost = true, questCompanion = true;
    // Per-frame projection bounds, deliberately excluded from saved preferences.
    int fixedMapWidth = 0, fixedMapHeight = 0;
    bool projectionUiOnly = false;
    bool projectionCleanScene = false;
    bool questPassthrough = false, questTouch = true, questFlip = false;
    bool mapExtension = true;
    bool smoothMap = true;
    double pixelScale() const {
        return double(std::clamp(zoom, 1, 8)) / (1 << std::clamp(zoomOut, 0, 3));
    }
    void stepZoom(int direction) {
        if (direction > 0) {
            if (zoomOut)
                --zoomOut;
            else if (zoom < 8)
                ++zoom;
        } else {
            if (zoom > 1)
                --zoom;
            else if (zoomOut < 3)
                ++zoomOut;
        }
    }
    bool fullscreen = true, span = false, vsync = true, parallax = true, locator = true,
         minimap = false;
    bool help = true, spindash = false, noSpriteLimit = false, integerScale = true;
    bool mute = false, pauseUnfocused = true, performance = false, reduceMotion = false;
    bool autosave = true, rewind = true, ghost = false, invincible = false, infiniteAir = false;
    bool rewindAudio = true;
    bool fastBoot = true;
    bool lockon = false;
    bool safeEffects = true, confirmQuit = true;
    bool confirmActions = true, quickControls = true, readyOnly = false, highContrast = false;
    bool playerInset = true;
    bool enemyPreview = true;
    bool bossPreview = true, scriptPreview = true, spawnTint = true, spawnLabels = true;
    bool fitSpawns = true;
    int left = 0, top = 0;
    std::string lastGame = "sonic1", lastDirectory;
    void load(const fs::path &dir);
    void save(const fs::path &dir) const;
    void preset(int n);
};
} // namespace scu
