#pragma once
#include "voxel.h"
#include <filesystem>
namespace scu::voxel {
// Writes only into a new directory chosen by the caller. Exported pixels may
// come from the owner's cartridge and must never enter public release assets.
void exportScene(const Frame &, const Mesher &, const std::filesystem::path &directory);
} // namespace scu::voxel
