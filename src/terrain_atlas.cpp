#include "terrain_atlas.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <cstring>
namespace scu {
namespace {
struct Reader {
    const std::vector<uint8_t> &data;
    uint8_t byte(size_t p) const {
        if (p >= data.size())
            throw std::out_of_range("cartridge terrain");
        return data[p];
    }
    unsigned word(size_t p) const {
        return (unsigned(byte(p)) << 8) | byte(p + 1);
    }
    unsigned longword(size_t p) const {
        return (word(p) << 16) | word(p + 2);
    }
};
uint16_t crc16(const uint8_t *p, size_t n) {
    uint16_t crc = 0;
    while (n--) {
        crc ^= *p++;
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
    return crc;
}
} // namespace
bool decodeSSC(const std::vector<uint8_t> &input, size_t offset, size_t size,
               std::vector<uint8_t> &output) {
    output.clear();
    if (!size || size > 131072 || offset >= input.size())
        return false;
    Reader r{input};
    std::array<uint8_t, 4096> dictionary{};
    auto emit = [&](uint8_t value) {
        if (output.size() >= size)
            throw std::out_of_range("SSC output");
        output.push_back(value);
        dictionary[output.size() & 4095] = value;
    };
    try {
        // Every token emits data or terminates, so this also bounds the work.
        while (output.size() < size) {
            unsigned flags = r.byte(offset++);
            for (unsigned i = 0; i < 8 && output.size() < size; ++i) {
                if (flags & (1u << i))
                    emit(r.byte(offset++));
                else {
                    unsigned source = r.byte(offset++), control = r.byte(offset++);
                    source |= (control & 240) << 4;
                    if (!source)
                        throw std::out_of_range("short SSC output");
                    for (unsigned n = 0; n < (control & 15) + 2; ++n)
                        emit(dictionary[(source + n) & 4095]);
                }
            }
        }
        return true;
    } catch (const std::out_of_range &) {
        output.clear();
        return false;
    }
}
bool decodeRNC2Row(const std::vector<uint8_t> &input, size_t offset, std::vector<uint8_t> &output) {
    output.clear();
    Reader r{input};
    try {
        unsigned packed = r.longword(offset);
        if (!packed || packed > 8192 || offset > input.size() ||
            input.size() - offset < size_t(packed) + 10)
            return false;
        size_t pos = offset + 10, end = pos + packed;
        if (crc16(input.data() + pos, packed) != r.word(offset + 6))
            return false;
        unsigned remaining = 0, control = 0;
        auto raw = [&]() -> unsigned {
            if (pos >= end)
                throw std::out_of_range("RNC input");
            return input[pos++];
        };
        auto bit = [&]() -> unsigned {
            if (!remaining) {
                control = raw();
                remaining = 8;
            }
            return (control >> --remaining) & 1;
        };
        auto emit = [&](unsigned v) {
            if (output.size() >= 768)
                throw std::out_of_range("RNC row");
            output.push_back(uint8_t(v));
        };
        bit();
        bit();
        for (unsigned operations = 0; operations < 4096; ++operations) {
            if (!bit()) {
                emit(raw());
                continue;
            }
            unsigned length = 2, high = 0;
            bool shortOffset = false;
            if (!bit()) {
                length = length * 2 + bit();
                if (bit()) {
                    length = (length - 1) * 2 + bit();
                    if (length == 9) {
                        unsigned count = 0;
                        for (int n = 0; n < 4; ++n)
                            count = count * 2 + bit();
                        for (unsigned n = 0; n < (count + 3) * 4; ++n)
                            emit(raw());
                        continue;
                    }
                }
            } else if (!bit())
                shortOffset = true;
            else {
                length = 3;
                if (bit()) {
                    length = raw();
                    if (!length) {
                        if (!bit()) {
                            if (output.size() == 768 &&
                                crc16(output.data(), output.size()) == r.word(offset + 4))
                                return true;
                            break;
                        }
                        continue;
                    }
                    length += 8;
                }
            }
            if (!shortOffset && bit()) {
                high = bit();
                if (bit()) {
                    high = (high * 2 + bit()) | 4;
                    if (!bit())
                        high = high * 2 + bit();
                } else if (!high)
                    high = 2 + bit();
            }
            unsigned distance = (high << 8) + raw() + 1;
            if (distance > output.size())
                break;
            for (unsigned n = 0; n < length; ++n)
                emit(output[output.size() - distance]);
        }
    } catch (const std::out_of_range &) {
    }
    output.clear();
    return false;
}
void TerrainAtlas::clear() {
    ++revision;
    family = stage = -1;
    width = height = 0;
    std::vector<uint8_t>().swap(pixels);
    std::vector<std::array<uint16_t, 2>>().swap(cells);
    std::vector<uint8_t>().swap(spinArt);
    for (auto &uses : tileUses)
        std::vector<uint32_t>().swap(uses);
}
int TerrainAtlas::backgroundY(int y) const {
    // The final board has a 1536-pixel background and a 2048-pixel foreground.
    // Its original camera changes slope at 1440. Anchor that parallax curve
    // over the complete board instead of reading beyond its shorter layout.
    return family == 4 && stage == 3 ? (y < 1440 ? y * 29 / 45 : y - 512) : y;
}
void TerrainAtlas::drawCell(uint32_t cell, const std::vector<uint8_t> &rom, const SCUSnapshot &s,
                            bool live) {
    int x = int(cell % (width / 8)) * 8, y = int(cell / (width / 8)) * 8;
    Reader r{rom};
    for (int py = 0; py < 8; ++py)
        for (int px = 0; px < 8; ++px) {
            uint8_t color = 0;
            for (int layer = 1; layer >= 0; --layer) {
                int sourceY = layer ? backgroundY(y + py) : y + py;
                unsigned a = cells[size_t(sourceY / 8) * (width / 8) + x / 8][layer],
                         tile = a & 2047, pal = (a >> 9) & 48;
                unsigned ix = px ^ ((a & 0x800) ? 7 : 0),
                         iy = (sourceY & 7) ^ ((a & 0x1000) ? 7 : 0);
                if (family == 5 && !layer) {
                    tile |= (a & 0x4000) >> 3;
                    pal &= 16;
                }
                size_t address = size_t(tile) * 32 + iy * 4 + ix / 2;
                uint8_t packed;
                if (family == 5 && !layer)
                    packed = r.byte(terrainArt + address);
                else if (family == 4 && !live) {
                    if (address >= spinArt.size())
                        throw std::out_of_range("board tile bank");
                    packed = spinArt[address];
                } else
                    packed = s.vram[address & 65535];
                unsigned p = (ix & 1) ? packed & 15 : packed >> 4;
                if (p)
                    color = uint8_t(p | pal | ((a & 0x8000) ? 128 : 0));
            }
            pixels[size_t(y + py) * width + x + px] = color;
        }
}
void TerrainAtlas::animate(const std::vector<uint8_t> &rom, const SCUSnapshot &s) {
    std::vector<uint32_t> dirty;
    for (unsigned t = 0; t < 2048; ++t) {
        if (!std::memcmp(previousVram.data() + t * 32, s.vram + t * 32, 32))
            continue;
        std::memcpy(previousVram.data() + t * 32, s.vram + t * 32, 32);
        dirty.insert(dirty.end(), tileUses[t].begin(), tileUses[t].end());
    }
    if (family == 4) {
        std::sort(dirty.begin(), dirty.end());
        dirty.erase(std::unique(dirty.begin(), dirty.end()), dirty.end());
    }
    for (uint32_t cell : dirty)
        drawCell(cell, rom, s, true);
    if (!dirty.empty())
        ++revision;
}
bool TerrainAtlas::prepare(const std::vector<uint8_t> &rom, const SCUSnapshot &s, int kind,
                           int act) {
    if (family == kind && stage == act) {
        if (!pixels.empty())
            animate(rom, s);
        return !pixels.empty();
    }
    clear();
    family = kind;
    stage = act;
    Reader r{rom};
    try {
        unsigned layout[2]{}, mapping[2]{};
        unsigned artBase = 0, foregroundTiles = 0;
        if (kind == 4 && act >= 0 && act < 4) {
            width = r.word(0xBFC70 + act * 4);
            height = r.word(0xBFC72 + act * 4);
            if (width < 320 || width > 2048 || height != 2048)
                throw std::out_of_range("board dimensions");
            for (unsigned layer = 0; layer < 2; ++layer) {
                unsigned tiles = r.longword(0xBFBE0 + layer * 16 + act * 4);
                unsigned count = r.word(tiles);
                std::vector<uint8_t> bank;
                if (!decodeSSC(rom, tiles + 2, count * 32, bank))
                    throw std::out_of_range("board tiles");
                if (!layer)
                    foregroundTiles = count;
                spinArt.insert(spinArt.end(), bank.begin(), bank.end());
                layout[layer] = r.longword(0xBFC00 + layer * 16 + act * 4);
                mapping[layer] = r.longword(0xBFC20 + layer * 16 + act * 4);
            }
        } else if (kind == 5 && act >= 1 && act <= 22) {
            unsigned header = 0x1AB62 + (act - 1) * 42;
            width = 8192;
            height = 4096;
            artBase = r.longword(header + 12);
            mapping[0] = r.longword(header + 20);
            mapping[1] = r.longword(header + 16);
            layout[0] = r.longword(header + 24);
        } else
            throw std::out_of_range("terrain identity");
        pixels.assign(size_t(width) * height, 0);
        cells.resize(size_t(width / 8) * (height / 8));
        std::memcpy(previousVram.data(), s.vram, 65536);
        terrainArt = artBase;
        std::vector<uint8_t> row;
        for (int cy = 0; cy < height / 32; ++cy) {
            if (kind == 5 && !decodeRNC2Row(rom, layout[0] + r.word(layout[0] + cy * 2), row))
                throw std::out_of_range("terrain row CRC");
            for (int cx = 0; cx < width / 32; ++cx) {
                unsigned codes[2]{};
                if (kind == 4) {
                    for (int l = 0; l < 2; ++l)
                        if (!(act == 3 && l == 1 && cy >= 48))
                            codes[l] = r.word(layout[l] + (cy * (width / 32) + cx) * 2);
                } else {
                    codes[0] = (unsigned(row[cx * 3]) << 8) | (row[cx * 3 + 2] & 240);
                    codes[1] = (unsigned(row[cx * 3 + 1]) << 8) | ((row[cx * 3 + 2] & 15) << 4);
                }
                for (int layer = 1; layer >= 0; --layer) {
                    if (kind == 4 && act == 3 && layer == 1 && cy >= 48)
                        continue;
                    bool flipX =
                        kind == 4 ? (codes[layer] & 0x800) != 0 : layer == 0 && (codes[1] & 16);
                    bool flipY = kind == 4 && (codes[layer] & 0x1000);
                    unsigned offset = kind == 4 ? (codes[layer] & 2047) * 32 : codes[layer] & ~16u;
                    for (int ty = 0; ty < 4; ++ty)
                        for (int tx = 0; tx < 4; ++tx) {
                            unsigned a =
                                r.word(mapping[layer] + offset +
                                       ((ty ^ (flipY ? 3 : 0)) * 4 + (tx ^ (flipX ? 3 : 0))) * 2);
                            a ^= (flipX ? 0x800 : 0) | (flipY ? 0x1000 : 0);
                            if (kind == 5 && !layer && (codes[0] & 16))
                                a ^= 0x8000;
                            unsigned tile = a & 2047;
                            if (kind == 4 && layer)
                                tile += foregroundTiles;
                            size_t cellIndex = size_t(cy * 4 + ty) * (width / 8) + cx * 4 + tx;
                            cells[cellIndex][layer] = uint16_t(kind == 4 ? (a & ~2047u) | tile : a);
                        }
                }
            }
        }
        for (uint32_t cell = 0; cell < cells.size(); ++cell) {
            int x = int(cell % (width / 8)), y = int(cell / (width / 8)) * 8;
            if (kind == 4)
                tileUses[cells[cell][0] & 2047].push_back(cell);
            int previous = -1;
            for (int py = 0; py < 8; ++py) {
                int sourceY = backgroundY(y + py);
                int tile = cells[size_t(sourceY / 8) * (width / 8) + x][1] & 2047;
                if (tile != previous)
                    tileUses[tile].push_back(cell);
                previous = tile;
            }
            drawCell(cell, rom, s, false);
        }
        return true;
    } catch (const std::out_of_range &) {
        pixels.clear();
        width = height = 0;
        return false;
    }
}
} // namespace scu
