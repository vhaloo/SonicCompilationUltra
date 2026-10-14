#include "rewind.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <zlib.h>

namespace scu {
size_t RewindHistory::Entry::bytes() const {
    return sizeof(Entry) + state.capacity() + audio.capacity() * sizeof(int16_t);
}
void RewindHistory::clear() {
    entries.clear();
    std::vector<uint8_t>().swap(latest);
    used = 0;
}
void RewindHistory::trim(size_t limit) {
    while (!entries.empty() && bytes() > limit) {
        used -= entries.front().bytes();
        entries.pop_front();
    }
    if (entries.empty())
        clear();
}
void RewindHistory::push(const std::vector<uint8_t> &state, const std::vector<int16_t> &audio,
                         size_t limit) {
    if (state.empty() || state.size() > 20 * 1024 * 1024 || audio.size() % 2)
        throw std::runtime_error("Invalid rewind frame");
    Entry entry;
    entry.previousSize = latest.size();
    if (!latest.empty()) {
        // Adjacent states share most CPU memory and pixels. Store a reversible
        // XOR difference, anchored by one complete latest state, then compress.
        std::vector<uint8_t> delta(std::max(state.size(), latest.size()));
        const size_t common = std::min(state.size(), latest.size());
        for (size_t i = 0; i < common; ++i)
            delta[i] = state[i] ^ latest[i];
        if (state.size() > common)
            std::memcpy(delta.data() + common, state.data() + common, state.size() - common);
        if (latest.size() > common)
            std::memcpy(delta.data() + common, latest.data() + common, latest.size() - common);
        uLongf n = compressBound((uLong)delta.size());
        std::vector<uint8_t> compressed(n);
        if (compress2(compressed.data(), &n, delta.data(), (uLong)delta.size(), Z_BEST_SPEED) !=
            Z_OK)
            throw std::runtime_error("Rewind compression failed");
        entry.state.assign(compressed.begin(), compressed.begin() + n);
    }
    latest = state;
    entry.audio = audio;
    const size_t bytes = entry.bytes();
    while (!entries.empty() && used + bytes + latest.capacity() > limit) {
        used -= entries.front().bytes();
        entries.pop_front();
    }
    if (bytes + latest.capacity() <= limit) {
        used += bytes;
        entries.push_back(std::move(entry));
    } else
        clear();
}
bool RewindHistory::pop(std::vector<uint8_t> &state, std::vector<int16_t> &audio) {
    if (entries.empty())
        return false;
    auto entry = std::move(entries.back());
    entries.pop_back();
    used -= entry.bytes();
    state = latest;
    if (!entries.empty()) {
        std::vector<uint8_t> delta(std::max(latest.size(), entry.previousSize));
        uLongf n = (uLongf)delta.size();
        if (uncompress(delta.data(), &n, entry.state.data(), (uLong)entry.state.size()) != Z_OK ||
            n != delta.size()) {
            clear();
            throw std::runtime_error("Invalid compressed rewind frame");
        }
        latest.resize(delta.size());
        for (size_t i = 0; i < delta.size(); ++i)
            latest[i] ^= delta[i];
        latest.resize(entry.previousSize);
    } else {
        std::vector<uint8_t>().swap(latest);
    }
    audio = std::move(entry.audio);
    reverseStereo(audio);
    return true;
}
void reverseStereo(std::vector<int16_t> &samples) {
    if (samples.size() % 2)
        throw std::runtime_error("Incomplete stereo frame");
    const size_t frames = samples.size() / 2;
    for (size_t i = 0; i < frames / 2; ++i) {
        std::swap(samples[i * 2], samples[(frames - 1 - i) * 2]);
        std::swap(samples[i * 2 + 1], samples[(frames - 1 - i) * 2 + 1]);
    }
}
} // namespace scu
