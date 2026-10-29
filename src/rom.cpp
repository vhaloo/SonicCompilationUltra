#include "rom.h"
#include <zlib.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstring>
namespace scu {
const std::vector<Game> &games() {
    static const std::vector<Game> all = {
        {"sonic1", "Sonic the Hedgehog", 0xF9394E97, 524288, 1, "USA / Europe · revision 00"},
        {"sonic2", "Sonic the Hedgehog 2", 0x7B905383, 1048576, 2, "World · revision A"},
        {"sonic3", "Sonic the Hedgehog 3", 0x9BC192CE, 2097152, 3, "USA"},
        {"sandk", "Sonic & Knuckles", 0x0658F691, 2097152, 3, "World"},
        {"sonic3k", "Sonic 3 & Knuckles", 0x63522553, 4194304, 3, "USA lock-on"},
        {"sonic2k", "Knuckles in Sonic 2", 0x2AC1E7C6, 3407872, 2,
         "World revision A lock-on · includes UPMEM"},
        {"sphere", "Blue Sphere (S&K + Sonic 1)", 0xE01F6ED5, 2621440, 0, "USA / Europe lock-on"},
        {"meanbean", "Dr. Robotnik's Mean Bean Machine", 0xC7CA517F, 1048576, 0, "USA"},
        {"spinball", "Sonic Spinball", 0x677206CB, 1048576, 0, "USA"},
        {"3dblast", "Sonic 3D Blast", 0x44A2CA44, 4194304, 0, "USA / Europe / Korea"},
        {"compilation", "Sonic Compilation / Sonic Classics", 0x95B5E8D7, 3145728, 0,
         "USA / Europe / Korea · revision A"},
        {"skchip", "S&K lock-on UPMEM chip", 0x4DCFD55C, 262144, 0, "256 KiB cartridge chip"}};
    return all;
}
const Game *gameById(const std::string &id) {
    for (auto &g : games())
        if (id == g.id)
            return &g;
    return nullptr;
}
uint32_t checksum(const std::vector<uint8_t> &b) {
    return (uint32_t)crc32(0, b.data(), (uInt)b.size());
}
const Game *identify(const std::vector<uint8_t> &b) {
    auto c = checksum(b);
    for (auto &g : games())
        if (b.size() == g.size && c == g.crc)
            return &g;
    return nullptr;
}
std::vector<uint8_t> readBytes(const fs::path &p, size_t limit) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f)
        throw std::runtime_error("Cannot open file");
    auto n = f.tellg();
    if (n < 0 || (uint64_t)n > limit)
        throw std::runtime_error("File exceeds size limit");
    std::vector<uint8_t> b((size_t)n);
    f.seekg(0);
    f.read((char *)b.data(), b.size());
    if (!f && !b.empty())
        throw std::runtime_error("Incomplete read");
    return b;
}
void writeBytesAtomic(const fs::path &p, const std::vector<uint8_t> &b) {
    fs::create_directories(p.parent_path());
    auto tmp = p;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            throw std::runtime_error("Cannot write file");
        f.write((const char *)b.data(), b.size());
        if (!f)
            throw std::runtime_error("Incomplete write");
    }
    std::error_code ec;
    if (fs::exists(p)) {
        auto bak = p;
        bak += ".bak";
        fs::copy_file(p, bak, fs::copy_options::overwrite_existing, ec);
        if (ec)
            throw std::runtime_error("Cannot preserve previous file");
        fs::remove(p, ec);
        if (ec)
            throw std::runtime_error("Cannot replace file");
    }
    fs::rename(tmp, p);
}
static uint16_t le16(const std::vector<uint8_t> &b, size_t p) {
    if (p + 2 > b.size())
        throw std::runtime_error("Invalid ZIP");
    return b[p] | (b[p + 1] << 8);
}
static uint32_t le32(const std::vector<uint8_t> &b, size_t p) {
    return le16(b, p) | (uint32_t(le16(b, p + 2)) << 16);
}
static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}
std::vector<RomData> readRoms(const fs::path &path) {
    auto file = readBytes(path);
    std::vector<RomData> out;
    if (lower(path.extension().u8string()) != ".zip") {
        if (file.size() < 256 * 1024 || file.size() > 10 * 1024 * 1024)
            throw std::runtime_error("Unsupported cartridge size");
        // Common .smd copier format: 512-byte header + interleaved 16 KiB banks.
        if ((file.size() % 16384) == 512) {
            std::vector<uint8_t> raw(file.size() - 512);
            for (size_t k = 0; k < raw.size(); k += 16384)
                for (size_t i = 0; i < 8192; i++) {
                    raw[k + i * 2] = file[k + 512 + 8192 + i];
                    raw[k + i * 2 + 1] = file[k + 512 + i];
                }
            file.swap(raw);
        }
        if (file.size() > 0x104 && std::memcmp(file.data() + 0x100, "ESAG", 4) == 0)
            for (size_t i = 0; i + 1 < file.size(); i += 2)
                std::swap(file[i], file[i + 1]);
        out.push_back({std::move(file), ""});
        return out;
    }
    if (file.size() < 22)
        throw std::runtime_error("Invalid ZIP");
    size_t end = file.size() - 22, low = file.size() > 65557 ? file.size() - 65557 : 0;
    while (le32(file, end) != 0x06054B50) {
        if (end == low)
            throw std::runtime_error("ZIP directory missing");
        --end;
    }
    unsigned count = le16(file, end + 10);
    size_t p = le32(file, end + 16);
    if (count > 256)
        throw std::runtime_error("Too many archive members");
    size_t expandedTotal = 0;
    for (unsigned e = 0; e < count; e++) {
        if (le32(file, p) != 0x02014B50)
            throw std::runtime_error("Invalid ZIP directory");
        unsigned flags = le16(file, p + 8), method = le16(file, p + 10),
                 nameLen = le16(file, p + 28);
        size_t packed = le32(file, p + 20), size = le32(file, p + 24), local = le32(file, p + 42);
        uint32_t crc = le32(file, p + 16);
        if (p + 46 + nameLen > file.size())
            throw std::runtime_error("Truncated ZIP");
        std::string name((char *)file.data() + p + 46, nameLen);
        size_t next = p + 46 + nameLen + le16(file, p + 30) + le16(file, p + 32);
        p = next;
        if (size < 256 * 1024 || size > 10 * 1024 * 1024 || (!name.empty() && name.back() == '/'))
            continue;
        expandedTotal += size;
        if (expandedTotal > 32 * 1024 * 1024)
            throw std::runtime_error("Archive expands beyond the cartridge import limit");
        if (flags & 1)
            throw std::runtime_error("Encrypted ZIP unsupported");
        if (le32(file, local) != 0x04034B50)
            throw std::runtime_error("Invalid ZIP member");
        size_t data = local + 30 + le16(file, local + 26) + le16(file, local + 28);
        if (data + packed > file.size())
            throw std::runtime_error("Truncated ZIP member");
        std::vector<uint8_t> raw(size);
        if (method == 0) {
            if (packed != size)
                throw std::runtime_error("Invalid stored size");
            std::copy_n(file.data() + data, size, raw.data());
        } else if (method == 8) {
            z_stream z{};
            z.next_in = file.data() + data;
            z.avail_in = (uInt)packed;
            z.next_out = raw.data();
            z.avail_out = (uInt)size;
            if (inflateInit2(&z, -15) != Z_OK)
                throw std::runtime_error("ZIP decoder failed");
            int status = inflate(&z, Z_FINISH);
            bool ok = status == Z_STREAM_END && z.total_out == size;
            inflateEnd(&z);
            if (!ok)
                throw std::runtime_error("Invalid compressed member");
        } else
            continue;
        if (checksum(raw) != crc)
            throw std::runtime_error("ZIP checksum mismatch");
        out.push_back({std::move(raw), std::move(name)});
    }
    if (out.empty())
        throw std::runtime_error("No cartridge in ZIP");
    return out;
}
const RomEntry *Library::find(const std::string &id) const {
    for (auto &r : entries)
        if (r.id == id)
            return &r;
    return nullptr;
}
void Library::load() {
    entries.clear();
    std::ifstream f(dataDir / "library.tsv");
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0)
            line.erase(0, 3);
        std::istringstream in(line);
        RomEntry r;
        std::string crc;
        std::getline(in, r.id, '\t');
        std::getline(in, crc, '\t');
        std::getline(in, r.path, '\t');
        std::getline(in, r.member);
        try {
            r.crc = std::stoul(crc, nullptr, 16);
        } catch (...) {
            continue;
        }
        if (gameById(r.id) || r.id.rfind("custom-", 0) == 0)
            entries.push_back(std::move(r));
    }
    refreshLockons();
}
void Library::save() const {
    std::string s;
    for (auto &r : entries)
        if (!r.derived) {
            char c[16];
            snprintf(c, sizeof c, "%08X", r.crc);
            s += r.id + "\t" + c + "\t" + r.path + "\t" + r.member + "\n";
        }
    writeBytesAtomic(dataDir / "library.tsv", std::vector<uint8_t>(s.begin(), s.end()));
}
int Library::importFile(const fs::path &path) {
    int added = 0;
    for (auto &raw : readRoms(path)) {
        const Game *g = identify(raw.bytes);
        std::string id;
        if (g)
            id = g->id;
        else if (raw.bytes.size() > 0x200 &&
                 std::memcmp(raw.bytes.data() + 0x100, "SEGA", 4) == 0) {
            char c[20];
            snprintf(c, sizeof c, "custom-%08X", checksum(raw.bytes));
            id = c;
        } else
            continue;
        auto file = fs::absolute(path).u8string();
        if (file.find_first_of("\t\n\r") != std::string::npos ||
            raw.member.find_first_of("\t\n\r") != std::string::npos)
            continue;
        auto it = std::find_if(entries.begin(), entries.end(), [&](auto &r) { return r.id == id; });
        RomEntry value{id, file, raw.member, checksum(raw.bytes), false};
        if (it == entries.end())
            entries.push_back(std::move(value));
        else
            *it = std::move(value);
        ++added;
    }
    refreshLockons();
    save();
    return added;
}
int Library::importFolder(const fs::path &path) {
    int n = 0;
    for (auto &p : fs::directory_iterator(path)) {
        if (!p.is_regular_file())
            continue;
        auto ext = lower(p.path().extension().u8string());
        if (ext != ".bin" && ext != ".md" && ext != ".gen" && ext != ".smd" && ext != ".zip")
            continue;
        try {
            bool known = false;
            for (auto &r : readRoms(p.path()))
                if (identify(r.bytes))
                    known = true;
            if (known)
                n += importFile(p.path());
        } catch (...) {
        }
    }
    return n;
}
void Library::refreshLockons() {
    entries.erase(std::remove_if(entries.begin(), entries.end(), [](auto &r) { return r.derived; }),
                  entries.end());
    if (!find("sandk"))
        return;
    for (auto p : {std::pair<const char *, const char *>{"sonic3", "sonic3k"},
                   {"sonic1", "sphere"},
                   {"sonic2", "sonic2k"}}) {
        if (!find(p.second) && find(p.first) &&
            (std::string(p.second) != "sonic2k" || find("skchip"))) {
            auto g = gameById(p.second);
            entries.push_back({g->id, "", "", g->crc, true});
        }
    }
}
std::vector<uint8_t> Library::bytes(const RomEntry &r) const {
    if (r.derived) {
        auto sk = bytes(*find("sandk"));
        auto other = bytes(*find(r.id == "sonic3k"  ? "sonic3"
                                 : r.id == "sphere" ? "sonic1"
                                                    : "sonic2"));
        sk.insert(sk.end(), other.begin(), other.end());
        if (r.id == "sonic2k") {
            auto chip = bytes(*find("skchip"));
            sk.insert(sk.end(), chip.begin(), chip.end());
        }
        if (checksum(sk) != r.crc)
            throw std::runtime_error("Lock-on identity mismatch");
        return sk;
    }
    for (auto &raw : readRoms(fs::u8path(r.path)))
        if (raw.member == r.member) {
            if (checksum(raw.bytes) != r.crc)
                throw std::runtime_error("Cartridge changed since import");
            return std::move(raw.bytes);
        }
    throw std::runtime_error("Imported member missing");
}
} // namespace scu
