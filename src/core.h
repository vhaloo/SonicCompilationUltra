#pragma once
#include "world.h"
#include "rewind.h"
#include <SDL.h>
#include <libretro.h>
#include <map>
namespace scu {
class Core {
    static Core *active;
    static bool environment(unsigned cmd, void *data);
    static void video(const void *data, unsigned w, unsigned h, size_t pitch);
    static size_t audio(const int16_t *data, size_t frames);
    static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id);
    static void log(enum retro_log_level, const char *, ...);
    std::map<std::string, std::string> options;
    bool changed = false, initialised = false;
    std::string systemDir, saveDir, romDir, contentPath, contentName;
    retro_game_info_ext infoExt{};
    std::vector<uint8_t> romBytes;
    RewindHistory history;
    std::vector<int16_t> frameAudio;
    bool captureAudio = false, rewinding = false;
    void queueAudio(const int16_t *data, size_t frames);
    bool restoreState(const std::vector<uint8_t> &data, bool rewindFrame);
    size_t rewindLimit() const;
    void saveSram();
    void loadSram();
    void gameplayMods(uint16_t pressed);

  public:
    Settings &settings;
    Library &library;
    World world;
    SCUSnapshot snapshot{};
    std::vector<uint32_t> native;
    int nativeW = 320, nativeH = 224;
    bool loaded = false, headless = false;
    bool executionDisabled = false;
    std::string currentId, status;
    std::string saveKey;
    uint32_t runningCrc = 0;
    bool locked = false;
    const Game *game = nullptr;
    uint64_t frame = 0;
    uint64_t imageVersion = 0;
    double fps = 59.92274, sampleRate = 44100;
    SDL_AudioDeviceID audioDevice = 0;
    uint16_t buttons[2]{}, lastButtons = 0;
    int spinCharge = 0, spinFacing = 1;
    explicit Core(Settings &s, Library &l) : settings(s), library(l) {}
    ~Core();
    bool load(const std::string &id);
    void unload();
    void reset();
    void tick();
    void updateOptions();
    std::vector<uint8_t> state() const;
    bool restore(const std::vector<uint8_t> &data);
    void save(int slot);
    void loadState(int slot);
    bool rewind();
    void setRewinding(bool on);
    bool isRewinding() const {
        return rewinding;
    }
    double rewindSeconds() const {
        return history.size() / fps;
    }
    size_t rewindBytes() const {
        return history.bytes();
    }
    const std::vector<int16_t> &audioSamples() const {
        return frameAudio;
    }
    void clearAudio();
};
} // namespace scu
