#include "spatial_controls.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace scu;
void require(bool b) {
    if (!b)
        std::abort();
}
int main() {
    for (int page = 0; page < 7; ++page) {
        auto buttons = spatialButtons(page);
        for (size_t i = 0; i < buttons.size(); ++i) {
            auto b = buttons[i];
            require(spatialHit(buttons, b.x, b.y) == int(i));
            require(std::abs(b.x) + b.width / 2 < spatialWidth / 2);
            require(std::abs(b.y) + b.height / 2 < spatialHeight / 2);
        }
        require(spatialHit(buttons, 8, 8) == -1);
        require(spatialHit(buttons, std::numeric_limits<float>::quiet_NaN(), 0) == -1);
    }
    require(!spatialPress(.6f, false, true));
    require(spatialPress(.8f, false, true));
    require(spatialPress(.6f, true, true));
    require(!spatialPress(.3f, true, true));
    require(!spatialPress(.9f, true, false));
    require(!spatialPress(2, true, true));
    require(spatialJoystick(.01f, .01f).x == 0);
    auto axis = spatialJoystick(1, 1);
    require(std::abs(std::hypot(axis.x, axis.y) - 1) < .0001f);
    auto v = coplanarViewport(-100, 50, 2000, 1000, 900, 250, 320, 224);
    require(std::abs(v.x - .5f) < .00001f && std::abs(v.y - .2f) < .00001f);
    require(std::abs(v.width - .16f) < .00001f && std::abs(v.height - .224f) < .00001f);
    require(coplanarViewport(0, 0, 0, 0, 0, 0, 320, 224).width == 0);
    std::cout << "Pure interaction checks passed; no SDL, OpenXR session or cartridge execution.\n";
}
