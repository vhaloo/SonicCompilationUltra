#include "settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>
namespace scu {
#define INT_FIELDS(X)                                                                              \
    X(questDefaultsRevision, 0, 7)                                                                 \
    X(questEyeDetail, 0, 2)                                                                        \
    X(questRemoteScale, 50, 180)                                                                   \
    X(questOptionsScale, 50, 180) X(questInsetWidth, 25, 160) X(questHandJoystick, 0, 2) X(        \
        questTvModel, 0, 2) X(desktopPresentation, 0, 2) X(desktopFrameScale, 50, 130)             \
        X(desktopCrtStrength, 0, 100) X(desktopCabinetAngle, 10, 60) X(questMode, 0, 6) X(         \
            questScale, 5, 20000) X(questResizeSpeed, 25, 300) X(questCabinetWidth, 40, 600)       \
            X(questCabinetHeight, 30, 160) X(questCabinetTilt, 0, 40) X(questTvWidth, 40, 800) X(  \
                questCurve,                                                                        \
                10, 220) X(questCrt, 0, 3)                                                         \
                X(questCrtMask, 0, 2) X(questCrtStrength, 0, 100) X(questCrtCurve, 0, 100) X(      \
                    questGlass,                                                                    \
                    0, 100) X(questSurroundBrightness, 0, 100)                                     \
                    X(questEnvironment, 0, 1) X(questWallHeight, 50, 500) X(                       \
                        questWallGap,                                                              \
                        0,                                                                         \
                        100) X(questRefresh, -1,                                                   \
                               240) X(questWallDetail, 0, 2)                                       \
                        X(questMenuDistance, 60, 250) X(questWidth, 80, 800) X(                    \
                            questDistance,                                                         \
                            80,                                                                    \
                            600) X(questHeight, -150,                                              \
                                   150) X(questTilt, -60, 80)                                      \
                            X(questResolution, 0, 3) X(questAnchor, 0, 1) X(questHaptics, 0, 100)  \
                                X(language, -1, 3) X(view, 0, 4) X(zoom, 1, 8) X(zoomOut, 0, 3) X( \
                                    hintCorner,                                                    \
                                    0,                                                             \
                                    3) X(hintScale, 60,                                            \
                                         180) X(hintOpacity, 20, 100)                              \
                                    X(hintMargin, 0, 64) X(hintDetail, 0, 2) X(                    \
                                        hintSeconds,                                               \
                                        0, 30) X(noticeSeconds, 0, 30)                             \
                                        X(panoramaHeight, 224, 4096) X(camera, 0, 4) X(            \
                                            background,                                            \
                                            0, 6) X(blur, 0,                                       \
                                                    16) X(brightness, 10, 100)                     \
                                            X(hud, 0, 3) X(hudScale, 0, 6) X(nativeScale, 0, 8) X( \
                                                workers, 0,                                        \
                                                16) X(volume, 0, 100)                              \
                                                X(quality, 0, 2) X(uiScale, 0, 3) X(slot, 1, 9) X( \
                                                    region,                                        \
                                                    0, 3)                                          \
                                                    X(renderer, 0, 1) X(scanlines, 0, 60) X(       \
                                                        panSpeed,                                  \
                                                        1, 32) X(deadzone, 3000,                   \
                                                                 28000) X(left, -32768, 32768)     \
                                                        X(top, -16384, 16384) X(menuLayout, 0, 1)  \
                                                            X(display, -1, 31) X(padLabels, 0, 3)  \
                                                                X(jumpLayout, 0,                   \
                                                                  2) X(overviewStyle, 0,           \
                                                                       2) X(enemyOpacity, 10, 80)  \
                                                                    X(enemyStyle, 0, 2)            \
                                                                        X(rewindButton, 0, 2)      \
                                                                            X(rewindMemory, 0, 2)
#define BOOL_FIELDS(X)                                                                             \
    X(questTransparent)                                                                            \
    X(questRemote)                                                                                 \
    X(questHandTracking)                                                                           \
    X(questInsetCrt)                                                                               \
    X(questCrtCoplanar) X(spinballTriggers) X(questMoveRotate) X(questControlHints)                \
        X(objectPreview) X(objectOrigins) X(frozenPreviews) X(questWorldBackdrop)                  \
            X(desktopSurround) X(desktopCrtMask) X(questAutoWall) X(questSpinballAuto)             \
                X(questSurround) X(questWallLocked) X(questAdaptive) X(questPauseLost)             \
                    X(questCompanion) X(questPassthrough) X(questTouch) X(questFlip) X(fullscreen) \
                        X(span) X(vsync) X(parallax) X(locator) X(minimap) X(help) X(spindash)     \
                            X(noSpriteLimit) X(integerScale) X(mute) X(pauseUnfocused)             \
                                X(performance) X(reduceMotion) X(autosave) X(rewind) X(ghost)      \
                                    X(invincible) X(infiniteAir) X(safeEffects) X(confirmQuit)     \
                                        X(fastBoot) X(lockon) X(confirmActions) X(quickControls)   \
                                            X(readyOnly) X(highContrast) X(playerInset)            \
                                                X(enemyPreview) X(bossPreview) X(scriptPreview)    \
                                                    X(spawnTint) X(spawnLabels) X(fitSpawns)       \
                                                        X(rewindAudio) X(mapExtension)             \
                                                            X(smoothMap) X(rewindIndicator)
void Settings::load(const fs::path &dir) {
    std::ifstream f(dir / "settings.ini");
    bool versionedXrDefaults = false, xrConfiguration = false;
    std::string s;
    while (std::getline(f, s)) {
        if (!s.empty() && s.back() == '\r')
            s.pop_back();
        if (s.compare(0, 3, "\xEF\xBB\xBF") == 0)
            s.erase(0, 3);
        auto eq = s.find('=');
        if (eq == std::string::npos)
            continue;
        auto k = s.substr(0, eq), v = s.substr(eq + 1);
        versionedXrDefaults |= k == "questDefaultsRevision";
        xrConfiguration |= k == "questMode";
        if (k == "lastGame") {
            lastGame = v;
            continue;
        }
        if (k == "lastDirectory") {
            lastDirectory = v;
            continue;
        }
        int n;
        try {
            n = std::stoi(v);
        } catch (...) {
            continue;
        }
#define LOAD_I(name, lo, hi)                                                                       \
    if (k == #name)                                                                                \
        name = std::clamp(n, lo, hi);
        INT_FIELDS(LOAD_I)
#undef LOAD_I
#define LOAD_B(name)                                                                               \
    if (k == #name)                                                                                \
        name = n != 0;
        BOOL_FIELDS(LOAD_B)
#undef LOAD_B
    }
    // One migration, then retain the user's explicit refresh/transparency choices.
    if (xrConfiguration && (!versionedXrDefaults || questDefaultsRevision < 7)) {
        std::error_code ec;
        if (fs::exists(dir / "settings.ini") && !fs::exists(dir / "settings-before-xr-0700.ini"))
            fs::copy_file(dir / "settings.ini", dir / "settings-before-xr-0700.ini", ec);
        if (ec) {
            questDefaultsRevision = 0;
            return;
        } // Keep old preferences if backup failed.
        questRefresh = 90;
        questTransparent = true;
        questCrtCoplanar = true;
        questSurroundBrightness = 100;
        questDefaultsRevision = 7;
    }
}
void Settings::save(const fs::path &dir) const {
    std::ostringstream out;
    out << "; Sonic Compilation Ultra settings\n";
#define SAVE_I(name, lo, hi) out << #name << "=" << name << "\n";
    INT_FIELDS(SAVE_I)
#undef SAVE_I
#define SAVE_B(name) out << #name << "=" << name << "\n";
    BOOL_FIELDS(SAVE_B)
#undef SAVE_B
    out << "lastGame=" << lastGame << "\nlastDirectory=" << lastDirectory << "\n";
    auto text = out.str();
    writeBytesAtomic(dir / "settings.ini", std::vector<uint8_t>(text.begin(), text.end()));
}
void Settings::preset(int n) {
    quality = n;
    // Performance changes should not move the camera or replace a chosen view.
    background = n == 0 ? 3 : 0;
    blur = n == 0 ? 0 : n == 1 ? 5 : 9;
    brightness = 60;
    parallax = n > 0;
    locator = true;
    minimap = false;
    noSpriteLimit = n > 0;
    scanlines = 0;
    workers = 0;
    smoothMap = n > 0;
}
} // namespace scu
