#include "touch_controls.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace scu::touch;
static void require(bool v, const char *message) {
    if (!v) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}
static bool inside(Rect r, Rect s) {
    return r.w > 0 && r.h > 0 && r.x >= s.x - .01f && r.y >= s.y - .01f &&
           r.x + r.w <= s.x + s.w + .01f && r.y + r.h <= s.y + s.h + .01f;
}
static Point center(Rect r) {
    return {r.x + r.w / 2, r.y + r.h / 2};
}
static Point button(const Layout &l, Action a) {
    for (auto &b : l.buttons)
        if (b.action == a)
            return center(b.rect);
    require(false, "missing button");
    return {};
}
int main() {
    int cases = 0;
    for (auto size : {Point{240, 240}, Point{320, 568}, Point{568, 320}, Point{412, 915},
                      Point{915, 412}, Point{600, 960}, Point{960, 600}, Point{800, 1280},
                      Point{1600, 900}, Point{1440, 420}, Point{280, 600}, Point{1200, 500}})
        for (bool menu : {false, true})
            for (bool left : {false, true})
                for (bool controls : {false, true})
                    for (int mode : {0, 1, 2})
                        for (float scale : {.65f, 1.f, 1.6f}) {
                            Options o;
                            o.menu = menu;
                            o.leftHanded = left;
                            o.controls = controls;
                            o.layout = mode;
                            o.scale = scale;
                            Rect safe{24, 32, size.x - 48, size.y - 64};
                            auto l = arrange(safe, o);
                            require(inside(l.canvas, safe), "canvas outside safe area");
                            if (l.stick.w)
                                require(inside(l.stick, safe), "stick outside safe area");
                            for (auto &b : l.buttons)
                                require(inside(b.rect, safe), "button outside safe area");
                            if (menu)
                                require(l.buttons.size() == 5, "menu dock is not complete");
                            else {
                                require(button(l, Action::Menu).x >= safe.x, "menu unavailable");
                                if (controls)
                                    require(l.stick.w > 0, "missing stick");
                                else
                                    require(l.stick.w == 0, "hidden stick still has a hit target");
                                if (mode == 0 && controls)
                                    require(l.deck == (safe.h > safe.w * 1.1f),
                                            "auto portrait split");
                            }
                            ++cases;
                        }
    auto l = arrange({0, 0, 412, 915}, {});
    Input in;
    auto stick = center(l.stick);
    stick.x += l.stick.w * .45f;
    require(in.down(1, stick, l) == Action::Stick, "stick capture");
    require(in.down(2, center(l.stick), l) == Action::None, "second stick capture stole first");
    in.down(3, button(l, Action::B), l);
    in.down(4, button(l, Action::Rewind), l);
    auto s = in.state(l, .25f);
    require((s.buttons & (1 << 7)) && (s.buttons & 1) && s.rewind, "simultaneous move/jump/rewind");
    in.up(3);
    s = in.state(l, .25f);
    require(!(s.buttons & 1) && s.rewind, "finger release changed another finger");
    in.move(4, {-100, -100});
    require(!in.state(l, .25f).rewind, "sliding off rewind kept it pressed");
    in.move(1, {-1000, -1000});
    s = in.state(l, .25f);
    require(std::abs(s.stick.x) <= 1 && std::abs(s.stick.y) <= 1, "stick not clamped");
    in.clear();
    s = in.state(l, .25f);
    require(s.buttons == 0 && !s.rewind, "rotation/focus clear stuck input");
    in.down(5, button(l, Action::A), l);
    in.down(6, button(l, Action::B), l);
    require((in.state(l, .25f).buttons & 513) == 513, "Spinball left+right");
    in.up(5);
    require(in.state(l, .25f).buttons == 1, "left release affected right flipper");
    Options menu;
    menu.menu = true;
    auto m = arrange({0, 0, 320, 568}, menu);
    in.clear();
    in.down(9, button(m, Action::ScrollDown), m);
    require(in.state(m, .25f).scroll == 1, "held scroll down");
    in.up(9);
    require(in.state(m, .25f).scroll == 0, "scroll release");
    std::printf("PASS %d adaptive layouts plus multitouch ownership, simultaneous flippers, "
                "rewind, release and scrolling. No game linked or executed.\n",
                cases);
}
