#include "terrain_atlas.h"
#include "setting_help.h"
#include "localization.h"
#include "world.h"
#include <cstring>
#include <memory>
#include <stdexcept>
using namespace scu;
namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void put(std::vector<uint8_t> &v, size_t at, unsigned value, int count = 2) {
    for (int i = count - 1; i >= 0; --i) {
        v.at(at + i) = value & 255;
        value >>= 8;
    }
}
unsigned crc(const std::vector<uint8_t> &v) {
    unsigned result = 0;
    for (auto b : v) {
        result ^= b;
        for (int i = 0; i < 8; ++i)
            result = (result >> 1) ^ ((result & 1) ? 0xA001 : 0);
    }
    return result;
}
struct Bits {
    std::vector<uint8_t> bytes;
    size_t control = 0;
    unsigned used = 8;
    void bit(unsigned b) {
        if (used == 8) {
            control = bytes.size();
            bytes.push_back(0);
            used = 0;
        }
        bytes[control] |= uint8_t(b << (7 - used++));
    }
    void raw(unsigned value) {
        bytes.push_back(uint8_t(value));
    }
    std::vector<uint8_t> row(const std::vector<uint8_t> &expected) {
        for (int i = 0; i < 4; ++i)
            bit(1);
        raw(0);
        bit(0);
        std::vector<uint8_t> result(10);
        put(result, 0, unsigned(bytes.size()), 4);
        put(result, 4, crc(expected));
        put(result, 6, crc(bytes));
        result.insert(result.end(), bytes.begin(), bytes.end());
        return result;
    }
};
} // namespace
void testTerrain() {
    std::vector<uint8_t> result, expected(768);
    Bits literal;
    literal.bit(0);
    literal.bit(0);
    for (unsigned i = 0; i < 768; ++i) {
        expected[i] = uint8_t(i * 17);
        literal.bit(0);
        literal.raw(expected[i]);
    }
    auto packed = literal.row(expected);
    check(decodeRNC2Row(packed, 0, result) && result == expected,
          "RNC literals and interleaved bit buffers");
    auto bad = packed;
    bad.back() ^= 128;
    check(!decodeRNC2Row(bad, 0, result) && result.empty(),
          "RNC packed CRC must reject corruption");
    packed.pop_back();
    check(!decodeRNC2Row(packed, 0, result), "RNC truncation accepted");
    check(!decodeRNC2Row({}, size_t(-1), result), "RNC invalid offset");
    for (bool shortCopies : {true, false}) {
        Bits run;
        run.bit(0);
        run.bit(0);
        expected.assign(768, 0x55);
        run.bit(0);
        run.raw(0x55);
        if (shortCopies) {
            run.bit(0);
            run.raw(0x55);
            for (int i = 0; i < 383; ++i) {
                run.bit(1);
                run.bit(1);
                run.bit(0);
                run.raw(0);
            }
        } else {
            for (int length : {255, 256, 256}) {
                for (int i = 0; i < 4; ++i)
                    run.bit(1);
                run.raw(length - 8);
                run.bit(0);
                run.raw(0);
            }
        }
        auto data = run.row(expected);
        check(decodeRNC2Row(data, 0, result) && result == expected, "RNC overlapping matches");
        data[4] ^= 1;
        check(!decodeRNC2Row(data, 0, result) && result.empty(), "RNC output CRC");
    }
    check(decodeSSC({1, 0x5A, 1, 15}, 0, 18, result) && result == std::vector<uint8_t>(18, 0x5A),
          "SSC overlap/dictionary origin");
    check(!decodeSSC({0, 0, 0}, 0, 8, result) && result.empty(), "SSC premature end");
    check(!decodeSSC({1, 0x5A, 1, 15}, 0, 17, result), "SSC overflow");
    check(!decodeSSC({255}, 0, 8, result), "SSC short literals");
    // Synthetic two-layer pinball board: upper plane transparent, lower
    // plane uses its own tile bank. A later VRAM change must update far tiles.
    std::vector<uint8_t> rom(1048576);
    put(rom, 0xBFC70, 320);
    put(rom, 0xBFC72, 2048);
    for (int l = 0; l < 2; ++l) {
        unsigned tiles = 0x1000 + l * 0x100;
        put(rom, 0xBFBE0 + l * 16, tiles, 4);
        put(rom, tiles, 1);
        for (int k = 0; k < 4; ++k) {
            rom[tiles + 2 + k * 9] = 255;
            for (int n = 0; n < 8; ++n)
                rom[tiles + 3 + k * 9 + n] = l ? 0x22 : 0;
        }
        put(rom, 0xBFC00 + l * 16, 0x3000 + l * 0x1000, 4);
        put(rom, 0xBFC20 + l * 16, 0x6000 + l * 0x100, 4);
    }
    auto snapshot = std::make_unique<SCUSnapshot>();
    *snapshot = {};
    for (int i = 0; i < 32; ++i)
        snapshot->vram[32 + i] = 0x22;
    TerrainAtlas atlas;
    check(atlas.prepare(rom, *snapshot, 4, 0), "synthetic pinball atlas");
    check((atlas.sample(319, 2047) & 63) == 2 && atlas.sample(-1, 0) == 0,
          "pinball background bank / far extent");
    for (int i = 0; i < 32; ++i)
        snapshot->vram[32 + i] = 0x33;
    check(atlas.prepare(rom, *snapshot, 4, 0) && (atlas.sample(319, 2047) & 63) == 3,
          "distant animated terrain cache update");
    // The final board's parallax layer stops after 48 brush rows. Poison the
    // following bytes so treating it as a 64-row layer cannot pass silently.
    for (unsigned table : {0xBFBE0u, 0xBFBF0u, 0xBFC00u, 0xBFC10u, 0xBFC20u, 0xBFC30u})
        for (int i = 0; i < 4; ++i)
            rom[table + 12 + i] = rom[table + i];
    put(rom, 0xBFC7C, 320);
    put(rom, 0xBFC7E, 2048);
    for (int row = 48; row < 64; ++row)
        for (int col = 0; col < 10; ++col)
            put(rom, 0x4000 + (row * 10 + col) * 2, 2047);
    check(atlas.prepare(rom, *snapshot, 4, 3) && (atlas.sample(319, 2047) & 63) == 2,
          "final board shorter parallax layer");
    atlas.clear();
    check(!atlas.bytes() && !atlas.sample(0, 0), "atlas reset");
    check(!atlas.prepare({}, *snapshot, 4, 0), "empty cartridge atlas accepted");
    size_t count = 0;
    auto help = allSettingHelp(count);
    check(count >= 70, "illustrated setting coverage");
    for (size_t i = 0; i < count; ++i) {
        check(settingHelp(help[i].key) == &help[i], "duplicate guide key");
        check(std::strcmp(tr(help[i].key, 0), help[i].key), "missing setting title");
        for (int l = 0; l < 4; ++l)
            check(std::strlen(help[i].text[l]) > 15, "missing localized setting explanation");
    }
    Settings settings;
    settings.stepZoom(-1);
    check(settings.pixelScale() == 0.5, "quick zoom below 1:1");
    settings.stepZoom(1);
    check(settings.pixelScale() == 1, "quick zoom return to 1:1");
}
