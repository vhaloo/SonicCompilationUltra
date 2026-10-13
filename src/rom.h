#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace scu {
namespace fs = std::filesystem;
struct Game {
    const char *id, *name;
    uint32_t crc;
    size_t size;
    int family;
    const char *revision;
};
const std::vector<Game> &games();
const Game *identify(const std::vector<uint8_t> &bytes);
const Game *gameById(const std::string &id);
uint32_t checksum(const std::vector<uint8_t> &bytes);
struct RomEntry {
    std::string id, path, member;
    uint32_t crc = 0;
    bool derived = false;
};
struct RomData {
    std::vector<uint8_t> bytes;
    std::string member;
};
std::vector<RomData> readRoms(const fs::path &path);
std::vector<uint8_t> readBytes(const fs::path &path, size_t limit = 32 * 1024 * 1024);
void writeBytesAtomic(const fs::path &path, const std::vector<uint8_t> &bytes);
class Library {
  public:
    fs::path dataDir;
    std::vector<RomEntry> entries;
    explicit Library(fs::path dir) : dataDir(std::move(dir)) {}
    void load();
    void save() const;
    int importFile(const fs::path &path);
    int importFolder(const fs::path &path);
    const RomEntry *find(const std::string &id) const;
    std::vector<uint8_t> bytes(const RomEntry &entry) const;
    void refreshLockons();
};
} // namespace scu
