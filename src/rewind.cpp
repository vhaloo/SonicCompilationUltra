#include "rewind.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <lz4.h>

namespace scu {
struct RewindHistory::Workspace {
    LZ4_stream_t *encoder = nullptr;
    std::vector<uint8_t> delta, compressed;
    size_t compress() {
        if (!encoder) {
            encoder = LZ4_createStream();
            if (!encoder)
                throw std::runtime_error("Rewind compression setup failed");
        }
        // Reuse an aligned workspace, with independent bounded blocks. Disk
        // saves keep their format; every guest tick and its sound are retained.
        compressed.resize(LZ4_compressBound(int(delta.size())));
        const int size = LZ4_compress_fast_extState(
            encoder, reinterpret_cast<const char *>(delta.data()),
            reinterpret_cast<char *>(compressed.data()), int(delta.size()),
            int(compressed.size()), 1);
        if (size <= 0)
            throw std::runtime_error("Rewind compression failed");
        return size;
    }
    void expand(const std::vector<uint8_t> &source) {
        const int size = LZ4_decompress_safe(
            reinterpret_cast<const char *>(source.data()),
            reinterpret_cast<char *>(delta.data()), int(source.size()), int(delta.size()));
        if (size != int(delta.size()))
            throw std::runtime_error("Invalid compressed rewind frame");
    }
    size_t bytes() const {
        return sizeof(Workspace) + (encoder ? LZ4_sizeofState() : 0) + delta.capacity() +
               compressed.capacity();
    }
    ~Workspace() {
        LZ4_freeStream(encoder);
    }
};
RewindHistory::RewindHistory() = default;
RewindHistory::~RewindHistory() = default;
size_t RewindHistory::bytes() const {
    return used + latest.capacity() + (workspace ? workspace->bytes() : 0);
}
size_t RewindHistory::Entry::bytes() const {
    return sizeof(Entry) + state.capacity() + audio.capacity() * sizeof(int16_t);
}
void RewindHistory::clear() {
    entries.clear();
    std::vector<uint8_t>().swap(latest);
    used = 0;
    workspace.reset();
}
void RewindHistory::trim(size_t limit) {
    memoryLimit = limit;
    while (!entries.empty() && bytes() > limit) {
        used -= entries.front().bytes();
        entries.pop_front();
    }
    if (entries.empty())
        clear();
}
void RewindHistory::push(const std::vector<uint8_t> &state, const std::vector<int16_t> &audio,
                         size_t limit) {
    memoryLimit = limit;
    if (state.empty() || state.size() > 20 * 1024 * 1024 || audio.size() % 2)
        throw std::runtime_error("Invalid rewind frame");
    Entry entry;
    entry.previousSize = latest.size();
    if (!latest.empty()) {
        // Adjacent states share most CPU memory and pixels. Store a reversible
        // XOR difference, anchored by one complete latest state, then compress.
        if (!workspace)
            workspace = std::make_unique<Workspace>();
        auto &delta = workspace->delta;
        delta.resize(std::max(state.size(), latest.size()));
        const size_t common = std::min(state.size(), latest.size());
        for (size_t i = 0; i < common; ++i)
            delta[i] = state[i] ^ latest[i];
        if (state.size() > common)
            std::memcpy(delta.data() + common, state.data() + common, state.size() - common);
        if (latest.size() > common)
            std::memcpy(delta.data() + common, latest.data() + common, latest.size() - common);
        size_t n = workspace->compress();
        entry.state.assign(workspace->compressed.begin(), workspace->compressed.begin() + n);
    }
    latest = state;
    entry.audio = audio;
    const size_t entryBytes = entry.bytes();
    while (!entries.empty() && bytes() + entryBytes > limit) {
        used -= entries.front().bytes();
        entries.pop_front();
    }
    if (entries.empty() && bytes() + entryBytes > limit) {
        // One standalone checkpoint can still fit when the codec cannot.
        workspace.reset();
        std::vector<uint8_t>().swap(entry.state);
        entry.previousSize = 0;
    }
    if (bytes() + entry.bytes() <= limit) {
        used += entry.bytes();
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
        auto &delta = workspace->delta;
        delta.resize(std::max(latest.size(), entry.previousSize));
        try {
            workspace->expand(entry.state);
        } catch (...) {
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
    if (entries.empty())
        clear();
    else
        trim(memoryLimit);
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
