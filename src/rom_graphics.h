#pragma once
#include <cstdint>
#include <vector>
namespace scu {
// Bounded, independent format readers. Input is always the locally supplied ROM.
std::vector<uint8_t> decodeNemesis(const std::vector<uint8_t> &rom, unsigned address);
std::vector<uint8_t> decodeKosinskiModules(const std::vector<uint8_t> &rom, unsigned address);
} // namespace scu
