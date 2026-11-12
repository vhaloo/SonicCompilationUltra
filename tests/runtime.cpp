#include "core.h"
#include "touch_controls.h"
#include <SDL.h>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

using namespace scu;
static_assert(touch::actionMask(touch::Action::A)==(1u<<RETRO_DEVICE_ID_JOYPAD_Y));
static_assert(touch::actionMask(touch::Action::B)==(1u<<RETRO_DEVICE_ID_JOYPAD_B));
static_assert(touch::actionMask(touch::Action::C)==(1u<<RETRO_DEVICE_ID_JOYPAD_A));
static void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

#include "fixture.h"

int main() {
    try {
        SDL_SetMainReady();
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
        require(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO) == 0, "SDL timer/audio initialization");
        const auto tempRoot = fs::canonical(fs::temp_directory_path());
        const auto dir =
            tempRoot / ("SCU-runtime-test-" + std::to_string(SDL_GetPerformanceCounter()));
        require(fs::create_directory(dir), "Unique private test directory");
        const auto path = dir / "original-test-program.bin";
        writeBytesAtomic(path, testProgram(true));
        Library library(dir);
        require(library.importFile(path) == 1, "Synthetic cartridge import");
        const auto portableRoot = dir / "portable";
        const auto relativeRom = fs::path("ROMs") / "original-test-program.bin";
        writeBytesAtomic(portableRoot / relativeRom, testProgram(true));
        Library portable(portableRoot / "UserData");
        portable.entries.push_back(library.entries.front());
        portable.entries.front().path = relativeRom.generic_u8string();
        portable.save();
        portable.load();
        require(portable.bytes(portable.entries.front()) == testProgram(true),
                "A portable cartridge path must resolve from the package folder");
        Settings defaults;
        require(defaults.motionMode==0&&defaults.motionJump==1&&defaults.motionSwipe,
                "Motion must remain opt-in with lift and swipe ready when enabled");
        const auto legacyMotion=dir/"legacy-motion";
        fs::create_directory(legacyMotion);
        const std::string legacy="motionMode=1\nmotionFlick=1\nmotionFlickThreshold=4\n";
        writeBytesAtomic(legacyMotion/"settings.ini",{legacy.begin(),legacy.end()});
        Settings migrated;migrated.load(legacyMotion);
        require(migrated.motionMode==1&&migrated.motionJump==2&&migrated.motionGestureSensitivity==200,
                "Legacy enabled flick did not migrate");
        const std::string explicitMotion="motionJump=0\nmotionGestureSensitivity=80\nmotionSwipe=0\n"+legacy;
        writeBytesAtomic(legacyMotion/"settings.ini",{explicitMotion.begin(),explicitMotion.end()});
        migrated=Settings{};migrated.load(legacyMotion);
        require(migrated.motionJump==0&&migrated.motionGestureSensitivity==80&&!migrated.motionSwipe,
                "Legacy settings overwrote an explicit new gesture preference");
        migrated.save(legacyMotion);Settings restoredMotion;restoredMotion.load(legacyMotion);
        require(restoredMotion.motionJump==0&&restoredMotion.motionGestureSensitivity==80&&!restoredMotion.motionSwipe,
                "New gesture options did not survive a save/load");
        require(defaults.rewind && defaults.rewindAudio && defaults.rewindButton == 0,
                "New installs must default to L2/LT with reverse sound");
        defaults.rewind = false;
        defaults.rewindAudio = false;
        defaults.rewindButton = 2;
        defaults.rewindMemory = 0;
        defaults.hintCorner = 3;
        defaults.hintScale = 60;
        defaults.hintOpacity = 35;
        defaults.hintMargin = 24;
        defaults.hintDetail = 2;
        defaults.hintSeconds = 7;
        defaults.noticeSeconds = 12;
        defaults.rewindIndicator = false;
        defaults.save(dir);
        Settings persisted;
        persisted.load(dir);
        require(!persisted.rewind && !persisted.rewindAudio && persisted.rewindButton == 2 &&
                    persisted.rewindMemory == 0,
                "Rewind options or existing opt-out preference did not persist");
        require(persisted.hintCorner == 3 && persisted.hintScale == 60 &&
                    persisted.hintOpacity == 35 && persisted.hintMargin == 24 &&
                    persisted.hintDetail == 2 && persisted.hintSeconds == 7 &&
                    persisted.noticeSeconds == 12 && !persisted.rewindIndicator,
                "Corner overlay preferences did not persist");
        const std::string invalidHints = "hintCorner=99\nhintScale=-100\nhintOpacity=101\n"
                                         "hintMargin=999\nhintDetail=-1\nhintSeconds=999\n"
                                         "noticeSeconds=-5\n";
        writeBytesAtomic(dir / "settings.ini", {invalidHints.begin(), invalidHints.end()});
        persisted.load(dir);
        require(persisted.hintCorner == 3 && persisted.hintScale == 60 &&
                    persisted.hintOpacity == 100 && persisted.hintMargin == 64 &&
                    persisted.hintDetail == 0 && persisted.hintSeconds == 30 &&
                    persisted.noticeSeconds == 0,
                "Out-of-range corner options were not bounded");
        Settings settings;
        settings.autosave = false;
        settings.fastBoot = false;
        settings.rewind = true;
        auto core = std::make_unique<Core>(settings, library);
        core->headless = true;
        require(core->load(library.entries.front().id), "Native core load");
        SDL_AudioSpec want{};
        want.freq = (int)core->sampleRate;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        core->audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
        require(core->audioDevice != 0, "Dummy audio device for real queue checks");
        // The device stays paused so tests can inspect exact queued byte counts.
        core->headless = false;
        for (int n = 0; n < 60; ++n)
            core->tick();
        require(core->snapshot.ram[0x1234] == 0x42, "68000 program did not execute");
        require(!core->native.empty(), "No native video frame");
        bool red = false;
        for (auto pixel : core->native)
            red |= (pixel & 0xFF0000) >= 0xB00000 && (pixel & 0xFFFF) == 0;
        require(red, "VDP backdrop did not reach host framebuffer");
        require(std::any_of(core->audioSamples().begin(), core->audioSamples().end(),
                            [](int16_t s) { return s != 0; }),
                "Authored PSG tone did not reach sound callback");
        auto state = core->state();
        const auto savedVideo = core->native;
        const auto savedBackdrop = core->snapshot.background[0][0];
        const auto counter = core->snapshot.ram[0x1235];
        const auto savedFrame = core->frame;
        for (int n = 0; n < 15; ++n)
            core->tick();
        core->native.assign(core->native.size(), 0);
        core->snapshot.background[0][0] ^= 0xFFFFFF;
        require(core->restore(state), "State restore");
        require(core->frame == savedFrame && core->snapshot.ram[0x1235] == counter,
                "State failed to restore native RAM and host time");
        require(core->native == savedVideo && core->snapshot.background[0][0] == savedBackdrop,
                "State failed to restore the paused presentation raster");
        require(core->rewindSeconds() == 0, "External restore retained a different timeline");
        auto damaged = state;
        damaged.back() ^= 1;
        require(!core->restore(damaged), "Corrupt state accepted");
        auto other = state;
        other[4] ^= 1;
        require(!core->restore(other), "Wrong cartridge state accepted");
        core->save(1);
        core->tick();
        core->loadState(1);
        require(core->frame == savedFrame, "File-backed state round trip");
        std::vector<std::vector<int16_t>> sounds;
        std::vector<uint8_t> counters;
        for (int n = 0; n < 16; ++n) {
            counters.push_back(core->snapshot.ram[0x1235]);
            core->tick();
            sounds.push_back(core->audioSamples());
        }
        const auto later = core->frame;
        require(SDL_GetQueuedAudioSize(core->audioDevice) > 0, "Forward sound not queued");
        for (int n = 15; n >= 0; --n) {
            require(core->rewind() && core->frame == savedFrame + n,
                    "Rewind must restore exactly one preceding native frame");
            require(core->snapshot.ram[0x1235] == counters[n], "Rewind native RAM mismatch");
            auto expected = sounds[n];
            reverseStereo(expected);
            require(core->audioSamples() == expected, "Reverse sound must match that tick exactly");
            if (n == 15)
                require(SDL_GetQueuedAudioSize(core->audioDevice) ==
                            expected.size() * sizeof(int16_t),
                        "Direction change retained forward audio or erased reversed audio");
        }
        require(later - core->frame == 16, "Rewind skipped native frames");
        require(!core->rewind() && core->frame == savedFrame,
                "Exhausted history must hold the oldest moment");
        core->clearAudio();
        core->tick();
        require(!core->isRewinding() && core->frame == savedFrame + 1,
                "Release must resume from the restored moment");
        require(core->rewind() && !core->rewind(), "Undone future survived the new branch");
        require(core->frame == savedFrame, "Branch returned to a future frame");
        core->tick();
        core->settings.mute = true;
        require(core->rewind() && SDL_GetQueuedAudioSize(core->audioDevice) == 0,
                "Mute must apply to reversed sound");
        core->settings.mute = false;
        core->tick();
        core->settings.rewindAudio = false;
        require(core->rewind() && SDL_GetQueuedAudioSize(core->audioDevice) == 0,
                "Silent rewind option must suppress playback");
        core->settings.rewindAudio = true;
        core->settings.volume = 0;
        core->tick();
        require(core->rewind() && SDL_GetQueuedAudioSize(core->audioDevice) == 0,
                "Zero volume must apply to reversed sound");
        core->settings.volume = 70;
        core->tick();
        core->settings.rewind = false;
        core->tick();
        require(core->rewindSeconds() == 0 && !core->rewind(),
                "Disabling rewind must clear history");
        core->settings.rewind = true;
        core->tick();
        core->reset();
        require(core->rewindSeconds() == 0 && !core->rewind(), "Reset must clear history");

        std::vector<int16_t> stereo{1, 101, 2, 102, 3, 103};
        reverseStereo(stereo);
        require(stereo == std::vector<int16_t>({3, 103, 2, 102, 1, 101}),
                "Stereo channel identities were reversed");
        RewindHistory history;
        std::vector<uint8_t> smallState(4096, 0x42);
        const std::vector<int16_t> audioBlock(2048, 0x15);
        constexpr size_t compressedBudget = 384 * 1024;
        for (int n = 0; n < 180; ++n) {
            smallState[0] = uint8_t(n);
            history.push(smallState, audioBlock, compressedBudget);
        }
        require(history.size() > 1 && history.size() < 180 && history.bytes() <= compressedBudget,
                "Rewind exceeded its allocation budget");
        std::vector<uint8_t> restored;
        std::vector<int16_t> reversed;
        require(history.pop(restored, reversed) && restored == smallState,
                "Compressed history did not round trip");
        smallState[0] = 178;
        require(history.pop(restored, reversed) && restored == smallState,
                "Eviction broke the preceding delta state");
        history.trim(4096);
        require(history.bytes() <= 4096, "Changing memory budget did not immediately trim history");
        history.push(smallState, stereo, 1);
        require(history.size() == 0 && history.bytes() == 0, "Oversized entry exceeded budget");
        history.push(smallState, stereo, 8192);
        smallState[0] = 179;
        history.push(smallState, stereo, 8192);
        require(history.size() == 1 && history.bytes() <= 8192 && history.pop(restored, reversed) &&
                    restored == smallState, "A tiny budget should still keep a standalone checkpoint");
        const std::vector<uint8_t> larger(12000, 0xA5), smaller(2000, 0x5A);
        history.push(larger, stereo, compressedBudget);
        history.push(smaller, stereo, compressedBudget);
        history.push(smallState, stereo, compressedBudget);
        require(history.pop(restored, reversed) && restored == smallState &&
                    history.pop(restored, reversed) && restored == smaller &&
                    history.pop(restored, reversed) && restored == larger && history.bytes() == 0,
                "Variable-sized delta history did not restore its complete states");
        core->unload();
        core->headless = true;
        core->settings.region = 2;
        require(core->load(library.entries.front().id) && core->fps > 49 && core->fps < 51,
                "PAL timing was not selected");
        size_t palSamples = 0;
        for (int n = 0; n < 50; ++n) {
            core->tick();
            palSamples += core->audioSamples().size();
        }
        require(std::abs(palSamples / (2 * core->sampleRate) - 50 / core->fps) < 0.003,
                "PAL sound duration does not match native frame timing");
        for (int n = 49; n >= 0; --n)
            require(core->rewind() && core->frame == uint64_t(n), "PAL rewind skipped frames");
        core->unload(false);
        const auto controllerRom=dir/"controller.bin";
        writeBytesAtomic(controllerRom,testProgram(false,true));
        require(library.importFile(controllerRom)==1,"controller probe import");
        core->settings.region=1;core->settings.rewind=false;
        const auto inputEntry=std::find_if(library.entries.begin(),library.entries.end(),[&](auto &e){return library.bytes(e)==testProgram(false,true);});
        require(inputEntry!=library.entries.end()&&core->load(inputEntry->id),"controller probe load");
        for(int n=0;n<3;++n)core->tick();
        auto consoleProbe=[&](uint16_t buttons){
            core->buttons[0]=buttons;
            for(int n=0;n<3;++n)core->tick();
            return std::pair<uint8_t,uint8_t>{core->snapshot.ram[0x1236],core->snapshot.ram[0x1237]};
        };
        const auto idle=consoleProbe(0);
        require((idle.first&0x30)==0x30&&(idle.second&0x30)==0x30,"probe did not read released native port");
        for(auto profile:{touch::Profile::Jump,touch::Profile::ABC,touch::Profile::TwoActions,touch::Profile::Pinball})
          for(bool watch:{false,true}){
            touch::Options o;o.profile=profile;o.watch=watch;o.circular=watch;
            const auto l=touch::arrange({0,0,438,438},o);
            touch::Input finger;
            for(auto action:{touch::Action::A,touch::Action::B,touch::Action::C}) {
                auto button=std::find_if(l.buttons.begin(),l.buttons.end(),[&](auto b){return b.action==action;});
                if(button==l.buttons.end())continue;
                finger.clear();finger.down(1,{button->rect.x+button->rect.w*.5f,button->rect.y+button->rect.h*.5f},l,1000);
                finger.up(1); // a complete Android tap can arrive between native ticks
                const auto sampled=finger.state(l,.25f,1030).buttons;
                const auto actual=consoleProbe(sampled);
                require((actual.first&0x10)==(action==touch::Action::A?0:0x10),"touch A did not reach the native Genesis A pin");
                require((actual.second&0x10)==(action==touch::Action::B?0:0x10),"touch B did not reach the native Genesis B pin");
                require((actual.second&0x20)==(action==touch::Action::C?0:0x20),"touch C did not reach the native Genesis C pin");
                require(finger.state(l,.25f,1100).buttons==0,"short tap remained stuck");
            }
          }
        // This exact old mask was internally consistent in 0.13.0 but never
        // pressed the three-button controller's A pin. Keep the negative control.
        require((consoleProbe(1u<<RETRO_DEVICE_ID_JOYPAD_X).first&0x10)!=0,"negative control unexpectedly presses A");
        const auto both=consoleProbe(touch::actionMask(touch::Action::A)|touch::actionMask(touch::Action::B));
        require(!(both.first&0x10)&&!(both.second&0x10),"simultaneous native A/B flippers failed");
        const auto released=consoleProbe(0);
        require((released.first&0x30)==0x30&&(released.second&0x30)==0x30,"native input did not release");
        core.reset();
        // This path was created exclusively above; it is never supplied by a user.
        require(dir.parent_path() == tempRoot &&
                    dir.filename().string().rfind("SCU-runtime-test-", 0) == 0,
                "Test cleanup boundary");
        fs::remove_all(dir);
        SDL_Quit();
        std::cout << "PASS: original synthetic 68000 program, native VDP, state integrity, file "
                     "saves, exact-frame rewind, reverse stereo sound, queue direction/mute, "
                     "timeline branching and memory limits\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Runtime test: " << e.what() << '\n';
        SDL_Quit();
        return 1;
    }
}
