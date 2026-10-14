#include "settings.h"
#include <fstream>
#include <sstream>
#include <algorithm>
namespace scu {
#define INT_FIELDS(X)                                                                              \
    X(language, -1, 3)                                                                             \
    X(view, 0, 4)                                                                                  \
    X(zoom, 1, 8)                                                                                  \
    X(camera, 0, 4)                                                                                \
    X(background, 0, 5)                                                                            \
    X(blur, 0, 16)                                                                                 \
    X(brightness, 10, 100) X(hud, 0, 3) X(hudScale, 0, 6) X(nativeScale, 0, 8) X(workers, 0, 16)   \
        X(volume, 0, 100) X(quality, 0, 2) X(uiScale, 0, 3) X(slot, 1, 9) X(region, 0, 3)          \
            X(renderer, 0, 1) X(scanlines, 0, 60) X(panSpeed, 1, 32) X(deadzone, 3000, 28000)      \
                X(left, -32768, 32768) X(top, -16384, 16384) X(menuLayout, 0, 1)                   \
                    X(display, -1, 31) X(padLabels, 0, 3) X(jumpLayout, 0, 2)                      \
                        X(overviewStyle, 0, 2) X(enemyOpacity, 10, 80) X(enemyStyle, 0, 2)         \
                            X(rewindButton, 0, 2) X(rewindMemory, 0, 2)
#define BOOL_FIELDS(X)                                                                             \
    X(fullscreen)                                                                                  \
    X(span)                                                                                        \
    X(vsync)                                                                                       \
    X(parallax)                                                                                    \
    X(locator)                                                                                     \
    X(minimap)                                                                                     \
    X(help) X(spindash) X(noSpriteLimit) X(integerScale) X(mute) X(pauseUnfocused) X(performance)  \
        X(reduceMotion) X(autosave) X(rewind) X(ghost) X(invincible) X(infiniteAir) X(safeEffects) \
            X(confirmQuit) X(fastBoot) X(lockon) X(confirmActions) X(quickControls) X(readyOnly)   \
                X(highContrast) X(playerInset) X(enemyPreview) X(bossPreview) X(scriptPreview)     \
                    X(spawnTint) X(spawnLabels) X(fitSpawns) X(rewindAudio)
void Settings::load(const fs::path &dir) {
    std::ifstream f(dir / "settings.ini");
    std::string s;
    while (std::getline(f, s)) {
        auto eq = s.find('=');
        if (eq == std::string::npos)
            continue;
        auto k = s.substr(0, eq), v = s.substr(eq + 1);
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
}
} // namespace scu
