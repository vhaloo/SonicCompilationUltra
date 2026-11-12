#include "wear_display.h"
#include "touch_controls.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace scu;
static void check(bool value, const char *message) {
    if (!value) {
        std::fprintf(stderr, "FAIL %s\n", message);
        std::exit(1);
    }
}
int main() {
    unsigned cases = 0;
    for (int size : {160, 192, 240, 320, 384, 438, 454, 480, 800})
        for (bool round : {false, true})
            for (float blend : {0.f, .35f, .75f, 1.f})
                for (float aspect : {.5f, 320.f / 224, 16.f / 9, 8.f}) {
                    wear::LensMesh mesh;
                    check(mesh.update(size, size, aspect, round, blend, blend * 1.25f),
                          "initial mesh missing");
                    check(!mesh.update(size, size, aspect, round, blend, blend * 1.25f),
                          "steady frame rebuilt lens");
                    for (auto v : mesh.vertices) {
                        check(std::isfinite(v.x) && std::isfinite(v.y), "non-finite vertex");
                        check(v.x >= 0 && v.y >= 0 && v.x <= size && v.y <= size,
                              "lens clips a source edge");
                        if (round)
                            check(std::hypot(v.x - size * .5, v.y - size * .5) <= size * .48001,
                                  "vertex outside round screen");
                    }
                    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                        auto a = mesh.vertices[mesh.indices[i]],
                             b = mesh.vertices[mesh.indices[i + 1]],
                             c = mesh.vertices[mesh.indices[i + 2]];
                        check((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) < .0001,
                              "lens folded a triangle");
                    }
                    ++cases;
                }
    double last = 1e20;
    for (int z = 0; z <= 1000; ++z) {
        auto v = wear::camera(z / 1000., 16384, 2048, 320, 224, 8000, 700, 320. / 224);
        check(v.width <= last + .001 && v.width >= 319.999, "crown coverage not monotone");
        last = v.width;
        if (z == 0)
            check(v.left <= 0 && v.top <= 0 && v.left + v.width >= 16384 &&
                      v.top + v.height >= 2048,
                  "whole stage not visible");
        if (z == 1000)
            check(v.left == 8000 && v.top == 700 && std::abs(v.width - 320) < .001 &&
                      std::abs(v.height - 224) < .001,
                  "native endpoint changed");
    }
    for (int cx : {0, 80, 8000, 16064})
        for (int cy : {0, 12, 1824})
            for (int z = 0; z <= 1000; z += 5) {
                auto view = wear::camera(z / 1000., 16384, 2048, 320, 224, cx, cy, 320. / 224);
                check(view.left <= cx + .001 && view.left + view.width >= cx + 320 - .001 &&
                          view.top <= cy + .001 && view.top + view.height >= cy + 224 - .001,
                      "zoom lost the native viewport at a stage edge");
            }
    check(wear::crownZoom(0, 100, 2, false) == 1000, "crown upper bound");
    check(wear::crownZoom(1000, 100, 2, true) == 0, "reverse crown lower bound");
    check(wear::crownZoom(100, std::numeric_limits<double>::quiet_NaN(), 1, false) == 100,
          "invalid crown input");
    unsigned layouts = 0;
    for (int size : {160, 192, 240, 320, 438, 480})
        for (bool left : {false, true})
            for (bool menu : {false, true})
                for (bool bank : {false, true}) {
                    touch::Options o;
                    o.watch = o.circular = true;
                    o.menu = menu;
                    o.leftHanded = left;
                    o.utilityBank = bank;
                    auto l = touch::arrange({0, 0, float(size), float(size)}, o);
                    if (menu)
                        check(l.canvas.w == size && l.canvas.h == size && l.buttons.empty(),
                              "watch menu still reserves space for the control dock");
                    for (auto b : l.buttons)
                        for (float x : {b.rect.x, b.rect.x + b.rect.w})
                            for (float y : {b.rect.y, b.rect.y + b.rect.h})
                                check(std::hypot(x - size * .5, y - size * .5) <= size * .5001,
                                      "touch target clipped by round bezel");
                    touch::Input input;
                    if (!menu && !bank) {
                        input.down(1, {l.stick.x + l.stick.w * .9f, l.stick.y + l.stick.h * .5f},
                                   l);
                        for (auto b : l.buttons)
                            if (b.action == touch::Action::A)
                                input.down(
                                    2, {b.rect.x + b.rect.w * .5f, b.rect.y + b.rect.h * .5f}, l);
                        auto s = input.state(l, .25f);
                        check((s.buttons & (1 << 9)) && (s.buttons & (1 << 7)),
                              "simultaneous watch movement/action failed");
                    }
                    ++layouts;
                }
    std::printf("PASS %u generated lens variants, 1001 zoom positions, %u round layouts; no app, "
                "ROM, emulator or device.\n",
                cases, layouts);
}
