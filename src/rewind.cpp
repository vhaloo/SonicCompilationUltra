#include "rewind.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <zlib.h>
#include <cstdlib>
#include <limits>

namespace scu {
struct RewindHistory::Workspace {
    z_stream compressor{}, decompressor{};
    bool compressReady = false, decompressReady = false;
    size_t allocated = 0;
    struct alignas(std::max_align_t) Allocation {
        size_t bytes;
    };
    std::vector<uint8_t> delta, compressed;
    static voidpf allocate(voidpf opaque, uInt count, uInt size) {
        auto &self = *static_cast<Workspace *>(opaque);
        if (size &&
            size_t(count) > (std::numeric_limits<size_t>::max() - sizeof(Allocation)) / size)
            return nullptr;
        size_t bytes = size_t(count) * size + sizeof(Allocation);
        auto p = static_cast<Allocation *>(std::calloc(1, bytes));
        if (!p)
            return nullptr;
        p->bytes = bytes;
        self.allocated += bytes;
        return p + 1;
    }
    static void release(voidpf opaque, voidpf address) {
        if (!address)
            return;
        auto &self = *static_cast<Workspace *>(opaque);
        auto p = static_cast<Allocation *>(address) - 1;
        self.allocated -= p->bytes;
        std::free(p);
    }
    void initialise(z_stream &stream) {
        stream.zalloc = allocate;
        stream.zfree = release;
        stream.opaque = this;
    }
    size_t compress() {
        if (!compressReady) {
            initialise(compressor);
            // XOR deltas are mostly zero runs. RLE avoids dictionary search;
            // a smaller hash workspace leaves more memory for actual history.
            if (deflateInit2(&compressor, Z_BEST_SPEED, Z_DEFLATED, 15, 5, Z_RLE) != Z_OK)
                throw std::runtime_error("Rewind compression setup failed");
            compressReady = true;
        }
        if (deflateReset(&compressor) != Z_OK)
            throw std::runtime_error("Rewind compressor reset failed");
        // compressBound assumes zlib's default memory parameters. This stream
        // uses a smaller workspace, so ask it for the correct worst-case bound.
        compressed.resize(deflateBound(&compressor, uLong(delta.size())));
        compressor.next_in = delta.data();
        compressor.avail_in = uInt(delta.size());
        compressor.next_out = compressed.data();
        compressor.avail_out = uInt(compressed.size());
        if (deflate(&compressor, Z_FINISH) != Z_STREAM_END)
            throw std::runtime_error("Rewind compression failed");
        return compressor.total_out;
    }
    void expand(const std::vector<uint8_t> &source) {
        if (!decompressReady) {
            initialise(decompressor);
            if (inflateInit(&decompressor) != Z_OK)
                throw std::runtime_error("Rewind decoder setup failed");
            decompressReady = true;
        }
        if (inflateReset(&decompressor) != Z_OK)
            throw std::runtime_error("Rewind decoder reset failed");
        decompressor.next_in = const_cast<Bytef *>(source.data());
        decompressor.avail_in = uInt(source.size());
        decompressor.next_out = delta.data();
        decompressor.avail_out = uInt(delta.size());
        if (inflate(&decompressor, Z_FINISH) != Z_STREAM_END ||
            decompressor.total_out != delta.size() || decompressor.avail_in)
            throw std::runtime_error("Invalid compressed rewind frame");
    }
    size_t bytes() const {
        return sizeof(Workspace) + allocated + delta.capacity() + compressed.capacity();
    }
    ~Workspace() {
        if (compressReady)
            deflateEnd(&compressor);
        if (decompressReady)
            inflateEnd(&decompressor);
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
    if (bytes() + entryBytes <= limit) {
        used += entryBytes;
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
