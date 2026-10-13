#pragma once
#include "rom_view.h"
#include "observer.h"
#include <vector>
namespace scu {
enum class SpawnKind { Enemy, Boss, Scripted };
struct SpawnPlacement {
    int x = 0, y = 0;
    unsigned id = 0, subtype = 0, flags = 0, state = 0;
    unsigned function = 0, attributes = 0, frame = 0, codeEnd = 0;
    SpawnKind kind = SpawnKind::Enemy;
    bool event = false, relative = false, relativeX = false, relativeY = false, variable = false;
    int dx = 0, dy = 0;
    unsigned parentId = 0, parentFunction = 0, parentState = 0;
    unsigned sourceId = 0, sourceFunction = 0, sourceState = 0;
    unsigned bossOrigin = 0, bossOriginFunction = 0;
};
bool isBoss(int family, bool originalS3, bool skSet, unsigned id);
unsigned objectFunction(const RomView &r, unsigned index, int family, unsigned id);
unsigned objectEnd(const RomView &r, unsigned index, int family, unsigned function);
void addEventBosses(const RomView &r, int family, int stage, unsigned index,
                    std::vector<SpawnPlacement> &points);
void addScreenEventSpawns(const RomView &r, bool originalS3, int stage, unsigned index,
                          std::vector<SpawnPlacement> &points);
void addScriptSpawns(const RomView &r, int family, bool originalS3, unsigned index,
                     const std::vector<SpawnPlacement> &roots, std::vector<SpawnPlacement> &points);
} // namespace scu
