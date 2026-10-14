#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace scu {
// Each entry describes one completed tick: its starting state and its sound.
// Compression is private to this in-memory history; disk saves keep their format.
class RewindHistory {
    struct Entry {
        std::vector<uint8_t> state;
        std::vector<int16_t> audio;
        size_t previousSize = 0;
        size_t bytes() const;
    };
    std::deque<Entry> entries;
    std::vector<uint8_t> latest;
    size_t used = 0;

  public:
    void clear();
    void trim(size_t limit);
    void push(const std::vector<uint8_t> &state, const std::vector<int16_t> &audio, size_t limit);
    bool pop(std::vector<uint8_t> &state, std::vector<int16_t> &audio);
    size_t size() const {
        return entries.size();
    }
    size_t bytes() const {
        return used + latest.capacity();
    }
};

// Reverse time, keeping the left/right samples of each stereo frame together.
void reverseStereo(std::vector<int16_t> &samples);
} // namespace scu
