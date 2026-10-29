#include "core.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <zlib.h>
namespace scu {
Core *Core::active = nullptr;
void Core::log(enum retro_log_level, const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
}
bool Core::environment(unsigned cmd, void *data) {
    auto &c = *active;
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        ((retro_log_callback *)data)->log = log;
        return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *(unsigned *)data = 2;
        return true;
    case RETRO_ENVIRONMENT_GET_LANGUAGE:
        *(unsigned *)data = RETRO_LANGUAGE_ENGLISH;
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2: {
        auto d = ((retro_core_options_v2 *)data)->definitions;
        for (; d && d->key; d++)
            if (!c.options.count(d->key))
                c.options[d->key] = d->default_value ? d->default_value : d->values[0].value;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL: {
        auto d = ((retro_core_options_v2_intl *)data)->us;
        return environment(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, d);
    }
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS: {
        auto d = (retro_core_option_definition *)data;
        for (; d && d->key; d++)
            if (!c.options.count(d->key))
                c.options[d->key] = d->default_value ? d->default_value : d->values[0].value;
        return true;
    }
    case RETRO_ENVIRONMENT_SET_VARIABLES: {
        auto d = (retro_variable *)data;
        for (; d && d->key; d++) {
            std::string val = d->value ? d->value : "";
            auto p = val.find(';');
            if (p != std::string::npos)
                val = val.substr(p + 2);
            p = val.find('|');
            if (p != std::string::npos)
                val.resize(p);
            if (!c.options.count(d->key))
                c.options[d->key] = val;
        }
        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        auto v = (retro_variable *)data;
        auto i = c.options.find(v->key);
        v->value = i == c.options.end() ? nullptr : i->second.c_str();
        return v->value != nullptr;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data = c.changed;
        c.changed = false;
        return true;
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *(const char **)data = c.systemDir.c_str();
        return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = c.saveDir.c_str();
        return true;
    case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
        *(retro_game_info_ext **)data = &c.infoExt;
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *(unsigned *)data == RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
        return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
        auto av = (retro_system_av_info *)data;
        c.fps = av->timing.fps;
        c.sampleRate = av->timing.sample_rate;
        return true;
    }
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true;
        return true;
    case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        return true;
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
    case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
    case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
    case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
        return true;
    case RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE:
        *(int *)data = 3;
        return true;
    case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        *(bool *)data = c.headless;
        return true;
    default:
        return false;
    }
}
void Core::video(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data || !w || !h || w > 720 || h > 576)
        return;
    auto &c = *active;
    c.nativeW = w;
    c.nativeH = h;
    c.native.resize((size_t)w * h);
    for (unsigned y = 0; y < h; y++) {
        auto p = (const uint16_t *)((const uint8_t *)data + y * pitch);
        for (unsigned x = 0; x < w; x++) {
            int v = p[x], r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
            c.native[y * w + x] =
                0xFF000000u | ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
        }
    }
}
size_t Core::audio(const int16_t *data, size_t frames) {
    auto &c = *active;
    if (c.captureAudio && data)
        c.frameAudio.insert(c.frameAudio.end(), data, data + frames * 2);
    c.queueAudio(data, frames);
    return frames;
}
void Core::queueAudio(const int16_t *data, size_t frames) {
    if (!data || !frames || !audioDevice || headless || settings.mute || !settings.volume)
        return;
    if (SDL_GetQueuedAudioSize(audioDevice) > sampleRate * 4 / 5)
        return;
    std::vector<int16_t> samples(frames * 2);
    for (size_t i = 0; i < samples.size(); i++)
        samples[i] = (int16_t)(data[i] * settings.volume / 100);
    SDL_QueueAudio(audioDevice, samples.data(), (Uint32)(samples.size() * sizeof(int16_t)));
}
int16_t Core::input(unsigned port, unsigned device, unsigned, unsigned id) {
    auto &c = *active;
    if (port >= 2 || device != RETRO_DEVICE_JOYPAD)
        return 0;
    uint16_t b = c.buttons[port];
    if (!port && (c.currentId == "sphere" || c.locked) && (c.snapshot.ram[0xF600] & 127) == 0x2C)
        b |= (1 << RETRO_DEVICE_ID_JOYPAD_A) | (1 << RETRO_DEVICE_ID_JOYPAD_B) |
             (1 << RETRO_DEVICE_ID_JOYPAD_Y);
    if (!port && c.spinCharge && (b & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)))
        b &= ~((1 << RETRO_DEVICE_ID_JOYPAD_A) | (1 << RETRO_DEVICE_ID_JOYPAD_B) |
               (1 << RETRO_DEVICE_ID_JOYPAD_Y));
    return id == RETRO_DEVICE_ID_JOYPAD_MASK ? (int16_t)b : id < 16 ? ((b >> id) & 1) : 0;
}
void Core::updateOptions() {
    options["genesis_plus_gx_region_detect"] = settings.region == 1   ? "ntsc-u"
                                               : settings.region == 2 ? "pal"
                                               : settings.region == 3 ? "ntsc-j"
                                                                      : "auto";
    options["genesis_plus_gx_overscan"] = "disabled";
    options["genesis_plus_gx_ntsc_filter"] = "disabled";
    options["genesis_plus_gx_no_sprite_limit"] = settings.noSpriteLimit ? "enabled" : "disabled";
    options["genesis_plus_gx_render"] = "single field";
    options["genesis_plus_gx_audio_filter"] = settings.quality == 0 ? "disabled" : "low-pass";
    options["genesis_plus_gx_ym2612"] = settings.quality == 2 ? "mame (ym2612)" : "mame (ym2612)";
    changed = true;
}
bool Core::load(const std::string &id) {
    if (executionDisabled) {
        SDL_Log("SCU verification guard blocked cartridge load");
        status = "Cartridge execution is disabled during menu verification.";
        return false;
    }
    unload();
    status.clear();
    auto entry = library.find(id);
    if (!entry) {
        status = "Cartridge not imported";
        return false;
    }
    try {
        romBytes = library.bytes(*entry);
        game = identify(romBytes);
        currentId = id;
        locked = false;
        if (settings.lockon && id != "sandk" && id != "sonic3k" && id != "sonic2k" &&
            id != "sphere") {
            auto skEntry = library.find("sandk");
            if (!skEntry)
                throw std::runtime_error("Import Sonic & Knuckles first");
            auto combined = library.bytes(*skEntry);
            combined.insert(combined.end(), romBytes.begin(), romBytes.end());
            if (id == "sonic2") {
                auto chip = library.find("skchip");
                std::vector<uint8_t> bytes;
                if (chip)
                    bytes = library.bytes(*chip);
                else if (auto full = library.find("sonic2k")) {
                    auto image = library.bytes(*full);
                    bytes.assign(image.begin() + 0x300000, image.end());
                } else
                    throw std::runtime_error(
                        "Import the S&K UPMEM chip or a combined Knuckles in Sonic 2 ROM");
                combined.insert(combined.end(), bytes.begin(), bytes.end());
            }
            romBytes.swap(combined);
            game = identify(romBytes);
            if (!game)
                game = gameById("sphere");
            locked = true;
        }
        auto completeImage = romBytes;
        runningCrc = checksum(completeImage);
        saveKey = locked
                      ? (identify(completeImage) ? identify(completeImage)->id : (id + "-lockon"))
                      : id;
        systemDir = (library.dataDir / "bios").u8string();
        saveDir = (library.dataDir / "saves").u8string();
        romDir = library.dataDir.u8string();
        contentName = id;
        contentPath = (library.dataDir / (id + ".md")).u8string();
        fs::create_directories(fs::u8path(systemDir));
        fs::create_directories(fs::u8path(saveDir));
        options.clear();
        options["genesis_plus_gx_lock_on"] = "disabled";
        // Recreate the cartridge's lock-on mapping from the user's own two ROMs.
        if (game && game->crc == 0x2AC1E7C6) {
            std::vector<uint8_t> sk(romBytes.begin(), romBytes.begin() + 0x200000),
                up(romBytes.begin() + 0x300000, romBytes.end()),
                s2(romBytes.begin() + 0x200000, romBytes.begin() + 0x300000);
            writeBytesAtomic(fs::u8path(systemDir) / "sk.bin", sk);
            writeBytesAtomic(fs::u8path(systemDir) / "sk2chip.bin", up);
            romBytes.swap(s2);
            options["genesis_plus_gx_lock_on"] = "sonic & knuckles";
        }
        active = this;
        retro_set_environment(environment);
        retro_set_video_refresh(video);
        retro_set_audio_sample_batch(audio);
        retro_set_audio_sample([](int16_t, int16_t) {});
        retro_set_input_poll([]() {});
        retro_set_input_state(input);
        retro_init();
        initialised = true;
        updateOptions();
        infoExt = {};
        infoExt.full_path = contentPath.c_str();
        infoExt.dir = romDir.c_str();
        infoExt.name = contentName.c_str();
        infoExt.ext = "md";
        infoExt.data = romBytes.data();
        infoExt.size = romBytes.size();
        infoExt.persistent_data = true;
        retro_game_info info{contentPath.c_str(), romBytes.data(), romBytes.size(), nullptr};
        if (!retro_load_game(&info))
            throw std::runtime_error("Core rejected cartridge");
        loaded = true;
        retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
        retro_set_controller_port_device(1, RETRO_DEVICE_JOYPAD);
        retro_system_av_info av{};
        retro_get_system_av_info(&av);
        fps = av.timing.fps;
        sampleRate = av.timing.sample_rate;
        if (!headless) {
            SDL_AudioSpec want{}, got{};
            want.freq = (int)sampleRate;
            want.format = AUDIO_S16SYS;
            want.channels = 2;
            want.samples = 1024;
            audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
            if (audioDevice)
                SDL_PauseAudioDevice(audioDevice, 0);
        }
        loadSram();
        frame = 0;
        spinCharge = 0;
        lastButtons = 0;
        native.clear();
        ++imageVersion;
        std::memset(&snapshot, 0, sizeof snapshot);
        world.setRom(completeImage);
        history.clear();
        rewinding = false;
        frameAudio.clear();
        settings.lastGame = id;
        bool resumed = false;
        if (settings.autosave) {
            auto path = library.dataDir / "saves" / (saveKey + "-auto.state");
            if (fs::exists(path)) {
                try {
                    resumed = restore(readBytes(path));
                } catch (...) {
                }
            }
        }
        if (settings.fastBoot && !resumed && game) {
            bool silent = headless;
            headless = true;
            int frames = game->crc == 0x2AC1E7C6 ? 750
                         : game->family == 1     ? 600
                         : game->family == 2     ? 1120
                         : game->family == 3     ? 850
                         : id == "sphere"        ? 1000
                                                 : 600;
            for (int i = 0; i < frames; i++) {
                retro_run();
                ++frame;
            }
            scu_snapshot(&snapshot);
            ++imageVersion;
            headless = silent;
            clearAudio();
        }
        return true;
    } catch (const std::exception &e) {
        status = e.what();
        unload();
        return false;
    }
}
void Core::saveSram() {
    if (!loaded)
        return;
    size_t n = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    auto p = (uint8_t *)retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if (n && p) {
        std::vector<uint8_t> b(p, p + n);
        writeBytesAtomic(library.dataDir / "saves" / (saveKey + ".srm"), b);
    }
}
void Core::loadSram() {
    size_t n = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    auto p = (uint8_t *)retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    auto path = library.dataDir / "saves" / (saveKey + ".srm");
    if (n && p && fs::exists(path)) {
        auto b = readBytes(path);
        if (b.size() == n)
            std::copy(b.begin(), b.end(), p);
    }
}
void Core::unload() {
    if (loaded) {
        try {
            saveSram();
            if (settings.autosave && frame > 30)
                writeBytesAtomic(library.dataDir / "saves" / (saveKey + "-auto.state"), state());
        } catch (const std::exception &e) {
            status = e.what();
        }
        retro_unload_game();
        loaded = false;
    }
    if (initialised) {
        retro_deinit();
        initialised = false;
    }
    if (audioDevice) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    history.clear();
    rewinding = false;
    captureAudio = false;
    frameAudio.clear();
    native.clear();
    game = nullptr;
    spinCharge = 0;
}
Core::~Core() {
    unload();
}
void Core::clearAudio() {
    if (audioDevice)
        SDL_ClearQueuedAudio(audioDevice);
}
void Core::reset() {
    if (!loaded)
        return;
    retro_reset();
    world.reset();
    ++imageVersion;
    frame = 0;
    spinCharge = 0;
    history.clear();
    rewinding = false;
    frameAudio.clear();
    clearAudio();
}
void Core::gameplayMods(uint16_t pressed) {
    if (!game || !loaded)
        return;
    auto &r = snapshot.ram;
    if (game->family == 1 && settings.spindash && r[0xD000] == 1 && r[0xD024] == 2 &&
        !(r[0xD022] & 2)) {
        bool down = (buttons[0] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) != 0,
             jump = (pressed & ((1 << RETRO_DEVICE_ID_JOYPAD_A) | (1 << RETRO_DEVICE_ID_JOYPAD_B) |
                                (1 << RETRO_DEVICE_ID_JOYPAD_Y))) != 0;
        if (!spinCharge && down && jump && std::abs((int16_t)be16(r, 0xD014)) < 256) {
            spinCharge = 1;
            spinFacing = (r[0xD022] & 1) ? -1 : 1;
        } else if (spinCharge) {
            if (down) {
                if (jump)
                    spinCharge = std::min(8, spinCharge + 1);
                scu_ram_write16(0xD010, 0);
                scu_ram_write16(0xD014, 0);
            } else {
                int speed = spinFacing * (0x800 + spinCharge * 0x80);
                scu_ram_write16(0xD010, (uint16_t)speed);
                scu_ram_write16(0xD014, (uint16_t)speed);
                scu_ram_write8(0xD022, r[0xD022] | 4);
                scu_ram_write8(0xD016, 14);
                scu_ram_write8(0xD017, 7);
                scu_ram_write16(0xD00C, be16(r, 0xD00C) + 5);
                scu_ram_write8(0xD01C, 2);
                spinCharge = 0;
            }
        }
    } else
        spinCharge = 0;
    if (game->family == 1 && ((r[0xF600] & 127) == 12)) {
        unsigned player = 0xD000;
        if (settings.invincible)
            scu_ram_write16(player + 0x30, 120);
        if (settings.infiniteAir)
            scu_ram_write16(0xFE14, 30);
    }
}
void Core::tick() {
    if (!loaded)
        return;
    setRewinding(false);
    std::vector<uint8_t> before;
    if (settings.rewind)
        before = state();
    else
        history.clear();
    frameAudio.clear();
    captureAudio = true;
    uint16_t pressed = buttons[0] & ~lastButtons;
    gameplayMods(pressed);
    retro_run();
    captureAudio = false;
    scu_snapshot(&snapshot);
    ++imageVersion;
    ++frame;
    lastButtons = buttons[0];
    if (!before.empty()) {
        try {
            history.push(before, frameAudio, rewindLimit());
        } catch (const std::exception &e) {
            history.clear();
            status = e.what();
        }
    }
}
static void put32(std::vector<uint8_t> &b, uint32_t v) {
    for (int i = 0; i < 4; i++)
        b.push_back((uint8_t)(v >> (i * 8)));
}
static uint32_t get32(const std::vector<uint8_t> &b, size_t p) {
    if (p + 4 > b.size())
        throw std::runtime_error("Invalid state header");
    return b[p] | (uint32_t(b[p + 1]) << 8) | (uint32_t(b[p + 2]) << 16) |
           (uint32_t(b[p + 3]) << 24);
}
std::vector<uint8_t> Core::state() const {
    if (!loaded)
        throw std::runtime_error("No running game");
    size_t n = retro_serialize_size();
    if (n > 16 * 1024 * 1024)
        throw std::runtime_error("State too large");
    // Preserve the completed raster as well as guest state. Otherwise a paused
    // load or rewind can briefly mix the previous scene with restored geometry.
    const auto *image = reinterpret_cast<const uint8_t *>(&snapshot);
    unsigned width = native.empty() ? 0 : nativeW, height = native.empty() ? 0 : nativeH;
    const auto *pixels = reinterpret_cast<const uint8_t *>(native.data());
    std::vector<uint8_t> b{'S', 'C', 'U', '2'};
    b.reserve(40 + n + sizeof snapshot + native.size() * sizeof(uint32_t));
    put32(b, runningCrc);
    put32(b, (uint32_t)n);
    put32(b, 0);
    put32(b, (uint32_t)frame);
    put32(b, spinCharge);
    put32(b, (uint32_t)spinFacing);
    put32(b, lastButtons);
    put32(b, width);
    put32(b, height);
    b.resize(40 + n);
    if (!retro_serialize(b.data() + 40, n))
        throw std::runtime_error("State save failed");
    b.insert(b.end(), image, image + sizeof snapshot);
    if (!native.empty())
        b.insert(b.end(), pixels, pixels + native.size() * sizeof(uint32_t));
    const uint32_t crc = (uint32_t)crc32(0, b.data() + 40, (uInt)(b.size() - 40));
    for (int i = 0; i < 4; ++i)
        b[12 + i] = uint8_t(crc >> (i * 8));
    return b;
}
bool Core::restore(const std::vector<uint8_t> &b) {
    return restoreState(b, false);
}
bool Core::restoreState(const std::vector<uint8_t> &b, bool rewindFrame) {
    if (!loaded || b.size() < 32 || get32(b, 4) != runningCrc)
        return false;
    bool raster = std::memcmp(b.data(), "SCU2", 4) == 0;
    if (!raster && std::memcmp(b.data(), "SCU1", 4))
        return false;
    size_t header = raster ? 40 : 32, n = get32(b, 8), pixels = 0;
    unsigned width = 0, height = 0;
    if (n > 16 * 1024 * 1024 || b.size() < header)
        return false;
    if (raster) {
        width = get32(b, 32);
        height = get32(b, 36);
        if (width > 720 || height > 576 || (!width != !height))
            return false;
        pixels = size_t(width) * height;
    }
    size_t expected = n + (raster ? sizeof snapshot + pixels * sizeof(uint32_t) : 0);
    if (b.size() - header != expected)
        return false;
    const auto *body = b.data() + header;
    if (crc32(0, body, (uInt)expected) != get32(b, 12))
        return false;
    if (!retro_unserialize(body, n))
        return false;
    frame = get32(b, 16);
    spinCharge = std::min(8, (int)get32(b, 20));
    spinFacing = (int32_t)get32(b, 24) < 0 ? -1 : 1;
    lastButtons = (uint16_t)get32(b, 28);
    scu_snapshot(&snapshot);
    native.clear();
    if (raster) {
        std::memcpy(&snapshot, body + n, sizeof snapshot);
        nativeW = width;
        nativeH = height;
        native.resize(pixels);
        if (pixels)
            std::memcpy(native.data(), body + n + sizeof snapshot, pixels * sizeof(uint32_t));
    }
    ++imageVersion;
    if (!rewindFrame) {
        history.clear();
        frameAudio.clear();
        setRewinding(false);
        world.reset();
        clearAudio();
    }
    return true;
}
void Core::save(int slot) {
    auto p = library.dataDir / "saves" / (saveKey + "-" + std::to_string(slot) + ".state");
    writeBytesAtomic(p, state());
    status = "Saved";
}
void Core::loadState(int slot) {
    auto p = library.dataDir / "saves" / (saveKey + "-" + std::to_string(slot) + ".state");
    if (!restore(readBytes(p)))
        throw std::runtime_error("State is damaged or belongs to another cartridge");
    status = "Loaded";
}
void Core::setRewinding(bool on) {
    if (settings.rewind)
        history.trim(rewindLimit());
    else
        history.clear();
    on = on && loaded && settings.rewind;
    if (on == rewinding)
        return;
    rewinding = on;
    // Flush the old direction once. Clearing on every restored frame would
    // discard reverse sound before the device could play it.
    clearAudio();
    if (on)
        world.reset();
}
size_t Core::rewindLimit() const {
    static const size_t budgets[] = {24, 96, 192};
    return budgets[settings.quality == 0 ? 0 : std::clamp(settings.rewindMemory, 0, 2)] * 1024 *
           1024;
}
bool Core::rewind() {
    if (!loaded || !settings.rewind)
        return false;
    setRewinding(true);
    std::vector<uint8_t> before;
    if (!history.pop(before, frameAudio)) {
        frameAudio.clear();
        clearAudio();
        return false;
    }
    if (!restoreState(before, true)) {
        history.clear();
        frameAudio.clear();
        clearAudio();
        return false;
    }
    if (settings.rewindAudio)
        queueAudio(frameAudio.data(), frameAudio.size() / 2);
    return true;
}
} // namespace scu
