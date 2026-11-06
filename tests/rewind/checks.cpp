#include "rewind.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace scu;
static void check(bool v, const char *why) {
    if (!v) {
        std::fprintf(stderr, "FAIL %s\n", why);
        std::exit(1);
    }
}
int main() {
    std::mt19937 rng(1062026);
    unsigned verified = 0;
    for (bool dense : {false, true})
        for (bool variable : {false, true}) {
            RewindHistory history;
            std::vector<std::vector<uint8_t>> frames;
            std::vector<std::vector<int16_t>> sounds;
            std::vector<uint8_t> bytes(65536);
            for (int tick = 0; tick < 160; ++tick) {
                if (variable)
                    bytes.resize(50000 + (tick % 9) * 4000);
                if (dense)
                    for (auto &b : bytes)
                        b = uint8_t(rng());
                else
                    for (int n = 0; n < 70; ++n)
                        bytes[rng() % bytes.size()] ^= uint8_t(rng());
                std::vector<int16_t> audio(2 * (tick % 91));
                for (auto &n : audio)
                    n = int16_t(rng());
                frames.push_back(bytes);
                sounds.push_back(audio);
                history.push(bytes, audio, 24 * 1024 * 1024);
                check(history.bytes() <= 24 * 1024 * 1024, "workspace escaped memory limit");
            }
            check(history.size() == 160, "ample budget lost history");
            for (int tick = 159; tick >= 0; --tick) {
                std::vector<uint8_t> state;
                std::vector<int16_t> audio;
                check(history.pop(state, audio), "missing rewind frame");
                check(state == frames[tick], "state differs after reused zlib streams");
                reverseStereo(sounds[tick]);
                check(audio == sounds[tick], "reverse stereo order changed");
                check(history.bytes() <= 24 * 1024 * 1024, "inflate workspace escaped budget");
                ++verified;
            }
            check(history.bytes() == 0, "empty history retained working memory");
        }
    RewindHistory small;
    std::vector<uint8_t> bytes(65536);
    std::vector<int16_t> audio(64, 13);
    for (int i = 0; i < 500; ++i) {
        for (auto &b : bytes)
            b = uint8_t(rng());
        small.push(bytes, audio, 1024 * 1024);
        check(small.bytes() <= 1024 * 1024, "small budget overflow");
    }
    small.trim(700000);
    while (small.size()) {
        std::vector<uint8_t> restored;
        std::vector<int16_t> reversed;
        check(small.pop(restored, reversed), "trimmed history lost its anchor");
        check(small.bytes() <= 700000, "post-trim inflate exceeded new limit");
    }
    small.trim(100);
    check(small.bytes() == 0, "trim did not release streams and buffers");
    small.push(bytes, audio, 0);
    check(small.bytes() == 0, "zero budget retained history");
    std::printf("PASS %u exact generated state/audio round trips; sparse/dense, variable lengths, "
                "stream resets and memory limits. No game/core/device execution.\n",
                verified);
}
