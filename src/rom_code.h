#pragma once
#include "rom_view.h"
#include <vector>
namespace scu {
// Decode only reachable instructions. Never execute cartridge code or read
// the emulator's memory. Bounds and a work budget fail closed on foreign data.
std::vector<unsigned> codeInstructions(const RomView &r, unsigned start, unsigned end);
} // namespace scu
