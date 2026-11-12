#include "settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <tuple>
namespace scu {
#define INT_FIELDS(X)                                                                                            \
    X(voxelMode, 0, 3) X(voxelDetail, 0, 3) X(voxelFraming, 0, 1) X(voxelThickness, 1, 32) X(voxelSpacing, 0, 64) \
    X(voxelRelief, 0, 12) X(voxelLight, 0, 100) X(voxelYaw, -60, 60) X(voxelPitch, -60, 60)                      \
    X(performanceTarget, 30, 120)                                                                                \
    X(performanceHeadroom, 10, 45)                                                                               \
    X(wearZoom, 0, 1000)                                                                                         \
    X(mobileActions, 0, 1) X(motionMode, 0, 2) X(motionSensitivity, 40, 240) X(motionDeadzone, 5, 50) \
    X(motionJump, 0, 2) X(motionGestureSensitivity, 40, 200) \
    X(wearCrownSpeed, 0, 2) X(wearLensStrength, 0, 100) X(questFoveation, 0, 4) X(                               \
        minimumRenderScale, 25, 100) X(maximumRenderScale, 25, 100) X(questShaderQuality, 0, 2)                  \
        X(mobileLayout, 0, 2) X(mobileOrientation, 0, 2) X(mobileTouch, 0, 2) X(                                 \
            mobileTouchScale, 65, 160) X(mobileOpacity, 20, 100) X(mobileDeadzone, 10,                           \
                                                                   65) X(mobileSafeMargin, 4, 40)                \
            X(mobileRefresh, 0, 4) X(mobileRenderQuality, 0, 2) X(mobileStickX, -160, 160) X(                    \
                mobileStickY,                                                                                    \
                -160,                                                                                            \
                160) X(mobileButtonsX, -160,                                                                     \
                       160) X(mobileButtonsY, -160, 160)                                                         \
                X(questDefaultsRevision, 0, 7) X(questEyeDetail, 0, 2) X(                                        \
                    questRemoteScale,                                                                            \
                    50,                                                                                          \
                    180) X(questOptionsScale, 50,                                                                \
                           180) X(questInsetWidth, 25, 160)                                                      \
                    X(questHandJoystick, 0, 2) X(questTvModel, 0, 2) X(                                          \
                        desktopPresentation,                                                                     \
                        0,                                                                                       \
                        2) X(desktopFrameScale, 50,                                                              \
                             130) X(desktopCrtStrength, 0, 100)                                                  \
                        X(desktopCabinetAngle, 10, 60) X(questMode, 0, 6) X(                                     \
                            questScale,                                                                          \
                            5,                                                                                   \
                            20000) X(questResizeSpeed, 25,                                                       \
                                     300) X(questCabinetWidth, 40, 600)                                          \
                            X(questCabinetHeight, 30, 160) X(questCabinetTilt, 0, 40) X(                         \
                                questTvWidth,                                                                    \
                                40,                                                                              \
                                800) X(questCurve, 10,                                                           \
                                       220) X(questCrt, 0, 3)                                                    \
                                X(questCrtMask, 0, 2) X(questCrtStrength, 0, 100) X(questCrtCurve, 0, 100) X(    \
                                    questGlass,                                                                  \
                                    0, 100) X(questSurroundBrightness, 0, 100)                                   \
                                    X(questEnvironment, 0, 1) X(questWallHeight, 50, 500) X(                     \
                                        questWallGap,                                                            \
                                        0,                                                                       \
                                        100) X(questRefresh, -1,                                                 \
                                               240) X(questWallDetail, 0, 2)                                     \
                                        X(questMenuDistance, 60, 250) X(questWidth, 80, 800) X(                  \
                                            questDistance,                                                       \
                                            80,                                                                  \
                                            600) X(questHeight, -150,                                            \
                                                   150) X(questTilt, -60, 80)                                    \
                                            X(questResolution, 0, 3) X(questAnchor, 0, 1) X(                     \
                                                questHaptics,                                                    \
                                                0,                                                               \
                                                100) X(language, -1,                                             \
                                                       3) X(view, 0, 4)                                          \
                                                X(zoom, 1, 8) X(zoomOut, 0, 3) X(                                \
                                                    hintCorner,                                                  \
                                                    0,                                                           \
                                                    3) X(hintScale, 60,                                          \
                                                         180) X(hintOpacity, 20, 100)                            \
                                                    X(hintMargin, 0, 64) X(hintDetail, 0, 2) X(                  \
                                                        hintSeconds,                                             \
                                                        0, 30) X(noticeSeconds, 0, 30)                           \
                                                        X(panoramaHeight, 224, 4096) X(                          \
                                                            camera,                                              \
                                                            0, 4) X(background, 0, 6)                            \
                                                            X(blur, 0, 16) X(brightness, 10, 100) X(             \
                                                                hud, 0,                                          \
                                                                3) X(hudScale, 0, 6)                             \
                                                                X(nativeScale, 0,                                \
                                                                  8) X(workers, 0,                               \
                                                                       16) X(volume, 0, 100)                     \
                                                                    X(quality, 0,                                \
                                                                      2) X(uiScale, 0,                           \
                                                                           3) X(slot, 1, 9)                      \
                                                                        X(region, 0,                             \
                                                                          3) X(renderer, 0,                      \
                                                                               1) X(scanlines, 0, 60)            \
                                                                            X(panSpeed, 1, 32) X(                \
                                                                                deadzone,                        \
                                                                                3000,                            \
                                                                                28000) X(left,                   \
                                                                                         -32768,                 \
                                                                                         32768)                  \
                                                                                X(top, -16384, 16384) X(         \
                                                                                    menuLayout,                  \
                                                                                    0,                           \
                                                                                    1) X(display,                \
                                                                                         -1,                     \
                                                                                         31)                     \
                                                                                    X(padLabels,                 \
                                                                                      0,                         \
                                                                                      3) X(jumpLayout,           \
                                                                                           0,                    \
                                                                                           2)                    \
                                                                                        X(overviewStyle,         \
                                                                                          0,                     \
                                                                                          2) X(enemyOpacity,     \
                                                                                               10,               \
                                                                                               80)               \
                                                                                            X(enemyStyle,        \
                                                                                              0,                 \
                                                                                              2) X(rewindButton, \
                                                                                                   0,            \
                                                                                                   2)            \
                                                                                                X(rewindMemory,  \
                                                                                                  0,             \
                                                                                                  2)
#define BOOL_FIELDS(X)                                                                             \
    X(voxelBackground) X(voxelCrt)                                                                 \
    X(autoPerformance)                                                                             \
    X(adaptiveEffects)                                                                             \
    X(advancedOptions)                                                                             \
    X(wearFisheye) X(wearCrownReverse) X(wearFillMigrated) X(motionSwipe) X(motionInvertX) X(motionInvertY) X(performancePixelLock) X(thermalAware) X(mobileLeftHanded) \
        X(mobileImmersive) X(mobileHints) X(questTransparent) X(questRemote) X(questHandTracking)  \
            X(questInsetCrt) X(questCrtCoplanar) X(spinballTriggers) X(questMoveRotate)            \
                X(questControlHints) X(objectPreview) X(objectOrigins) X(frozenPreviews)           \
                    X(questWorldBackdrop) X(desktopSurround) X(desktopCrtMask) X(questAutoWall) X( \
                        questSpinballAuto) X(questSurround) X(questWallLocked) X(questAdaptive)    \
                        X(questPauseLost) X(questCompanion) X(questPassthrough) X(questTouch)      \
                            X(questFlip) X(fullscreen) X(span) X(vsync) X(parallax) X(locator)     \
                                X(minimap) X(help) X(spindash) X(noSpriteLimit) X(integerScale)    \
                                    X(mute) X(pauseUnfocused) X(performance) X(reduceMotion) X(    \
                                        autosave) X(rewind) X(ghost) X(invincible) X(infiniteAir)  \
                                        X(safeEffects) X(confirmQuit) X(fastBoot) X(lockon)        \
                                            X(confirmActions) X(quickControls) X(readyOnly)        \
                                                X(highContrast) X(playerInset) X(enemyPreview)     \
                                                    X(bossPreview) X(scriptPreview) X(spawnTint)   \
                                                        X(spawnLabels) X(fitSpawns) X(rewindAudio) \
                                                            X(mapExtension) X(smoothMap)           \
                                                                X(rewindIndicator)
void Settings::load(const fs::path &dir) {
    std::ifstream f(dir / "settings.ini");
    bool versionedXrDefaults = false, xrConfiguration = false;
    bool explicitJump=false,explicitGestureSensitivity=false;
    int legacyFlick=-1,legacyThreshold=0;
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
        if(k=="motionFlick"){legacyFlick=n;continue;}
        if(k=="motionFlickThreshold"){legacyThreshold=n;continue;}
        explicitJump |= k=="motionJump";
        explicitGestureSensitivity |= k=="motionGestureSensitivity";
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
    if(!explicitJump&&legacyFlick>0)motionJump=2;
    if(!explicitGestureSensitivity&&legacyThreshold>0)
        motionGestureSensitivity=std::clamp(800/legacyThreshold,40,200);
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
    quality = std::clamp(n, 0, 2);
}
namespace {
template <class T> auto appearance(T &s) {
    return std::tie(s.view, s.camera, s.left, s.top, s.zoom, s.zoomOut, s.panoramaHeight,
                    s.nativeScale, s.integerScale, s.overviewStyle, s.mapExtension, s.smoothMap,
                    s.background, s.blur, s.brightness, s.parallax, s.scanlines, s.voxelMode,
                    s.voxelFraming, s.voxelThickness, s.voxelSpacing, s.voxelRelief, s.voxelLight,
                    s.voxelYaw, s.voxelPitch, s.voxelBackground, s.voxelCrt, s.desktopPresentation,
                    s.desktopFrameScale, s.desktopCrtStrength, s.desktopCabinetAngle,
                    s.desktopSurround, s.desktopCrtMask, s.wearZoom);
}
template <class T> auto spatialAppearance(T &s) {
    return std::tie(s.questMode, s.questPassthrough, s.questTransparent, s.questCrt,
                    s.questSurroundBrightness, s.questSpinballAuto);
}
Settings pictureForLook(int n, const Settings &defaults) {
    Settings picture = defaults;
    if (n >= 1 && n <= 4) {
        picture.questMode = 0;
        picture.questPassthrough = false;
    }
    switch (n) {
    case 1: // Original cartridge framing
        picture.view = 4;
        picture.wearZoom = 1000;
        picture.background = 5;
        picture.questSpinballAuto = false;
        break;
    case 2: // Panorama
        picture.view = 1;
        picture.panoramaHeight = 384;
        picture.wearZoom = 500;
        break;
    case 3: // Voxel diorama
        picture.view = 1;
        picture.panoramaHeight = 320;
        picture.voxelMode = 2;
        picture.wearZoom = 650;
        break;
    case 4: // CRT
        picture.view = 4;
        picture.wearZoom = 1000;
        picture.desktopPresentation = 2;
        picture.questMode = 5;
        picture.questSpinballAuto = false;
        break;
    case 5: // Full level on an AR wall
        picture.questMode = 1;
        picture.questPassthrough = true;
        picture.questTransparent = true;
        picture.questSpinballAuto = false;
        break;
    }
    return picture;
}
} // namespace
void Settings::applyLook(int n, const Settings &defaults) {
    auto picture = pictureForLook(n, defaults);
    appearance(*this) = appearance(picture);
    spatialAppearance(*this) = spatialAppearance(picture);
    if (n == 5)
        questWallLocked = true;
}
int Settings::matchingLook(const Settings &defaults, bool spatial) const {
    for (int n = 0; n <= (spatial ? 5 : 4); ++n) {
        const auto picture = pictureForLook(n, defaults);
        if (appearance(*this) == appearance(picture) &&
            (!spatial || spatialAppearance(*this) == spatialAppearance(picture)))
            return n;
    }
    return -1;
}
void Settings::restoreDefaults(const Settings &defaults, int section) {
    auto reset = [&](auto... fields) { ((this->*fields = defaults.*fields), ...); };
#define F(field) &Settings::field
    switch (section) {
    case 0: {
        auto game = lastGame;
        auto directory = lastDirectory;
        *this = defaults;
        lastGame = std::move(game);
        lastDirectory = std::move(directory);
        break;
    }
    case 1: // Video and performance
        reset(F(voxelMode), F(voxelDetail), F(voxelFraming), F(voxelThickness), F(voxelSpacing), F(voxelRelief),
              F(voxelLight), F(voxelYaw), F(voxelPitch), F(voxelBackground), F(voxelCrt),
              F(language), F(quality), F(view), F(overviewStyle), F(wearZoom), F(wearFisheye),
              F(zoom), F(zoomOut), F(panoramaHeight), F(mapExtension), F(smoothMap), F(fullscreen),
              F(span), F(display), F(menuLayout), F(uiScale), F(highContrast), F(vsync),
              F(integerScale), F(desktopPresentation), F(desktopFrameScale), F(desktopCrtStrength),
              F(desktopCabinetAngle), F(desktopSurround), F(desktopCrtMask),
              F(nativeScale), F(workers), F(renderer), F(scanlines), F(performance), F(autoPerformance),
              F(adaptiveEffects), F(performanceHeadroom), F(performanceTarget), F(minimumRenderScale),
              F(maximumRenderScale), F(performancePixelLock), F(thermalAware));
        break;
    case 2: // Camera, HUD and previews
        reset(F(camera), F(left), F(top), F(panSpeed), F(hud), F(hudScale), F(locator), F(minimap),
              F(ghost), F(help), F(playerInset), F(enemyPreview), F(bossPreview), F(scriptPreview),
              F(objectPreview), F(objectOrigins), F(frozenPreviews), F(enemyStyle), F(enemyOpacity),
              F(spawnTint), F(spawnLabels), F(fitSpawns), F(hintCorner), F(hintScale), F(hintOpacity),
              F(hintMargin), F(hintDetail), F(hintSeconds), F(noticeSeconds), F(rewindIndicator));
        break;
    case 3:
        reset(F(background), F(blur), F(brightness), F(parallax), F(reduceMotion));
        break;
    case 4:
        reset(F(volume), F(mute), F(pauseUnfocused));
        break;
    case 5:
        reset(F(padLabels), F(spinballTriggers), F(jumpLayout), F(rewindButton), F(quickControls),
              F(deadzone),F(mobileActions),F(motionMode),F(motionSensitivity),F(motionDeadzone),
              F(motionJump),F(motionGestureSensitivity),F(motionSwipe),F(motionInvertX),F(motionInvertY));
        break;
    case 6:
        reset(F(lockon), F(spindash), F(fastBoot), F(noSpriteLimit), F(rewind), F(rewindAudio),
              F(rewindMemory), F(invincible), F(infiniteAir), F(region), F(confirmActions), F(confirmQuit));
        break;
    case 7:
        reset(F(slot), F(autosave));
        break;
    case 9: // Phone, tablet and watch
        reset(F(wearZoom), F(wearFisheye), F(wearLensStrength), F(wearCrownSpeed), F(wearCrownReverse),
              F(mobileLayout), F(mobileOrientation), F(mobileTouch), F(mobileTouchScale),
              F(mobileOpacity), F(mobileDeadzone), F(mobileSafeMargin), F(mobileRefresh),
              F(mobileRenderQuality), F(mobileStickX), F(mobileStickY), F(mobileButtonsX),
              F(mobileButtonsY), F(mobileLeftHanded), F(mobileImmersive), F(mobileHints),
              F(mobileActions),F(motionMode),F(motionSensitivity),F(motionDeadzone),
              F(motionJump),F(motionGestureSensitivity),F(motionSwipe),F(motionInvertX),F(motionInvertY));
        break;
    case 10: // Quest
        reset(F(questWidth), F(questDistance), F(questHeight), F(questTilt), F(questResolution),
              F(questAnchor), F(questHaptics), F(questMode), F(questWallHeight), F(questWallGap),
              F(questRefresh), F(questWallDetail), F(questMenuDistance), F(questScale),
              F(questResizeSpeed), F(questCabinetWidth), F(questCabinetHeight), F(questCabinetTilt),
              F(questTvWidth), F(questCurve), F(questCrt), F(questCrtMask), F(questCrtStrength),
              F(questCrtCurve), F(questGlass), F(questSurroundBrightness), F(questEnvironment),
              F(questEyeDetail), F(questRemoteScale), F(questOptionsScale), F(questInsetWidth),
              F(questHandJoystick), F(questTvModel), F(questTransparent), F(questRemote),
              F(questHandTracking), F(questInsetCrt), F(questCrtCoplanar), F(questMoveRotate),
              F(questControlHints), F(questSpinballAuto), F(questSurround), F(questWorldBackdrop),
              F(questAutoWall), F(questWallLocked), F(questAdaptive), F(questPauseLost),
              F(questCompanion), F(questPassthrough), F(questTouch), F(questFlip), F(questShaderQuality),
              F(questFoveation));
        break;
    }
#undef F
}
} // namespace scu
