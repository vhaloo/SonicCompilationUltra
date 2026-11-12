#include "touch_controls.h"
#include <algorithm>
#include <cmath>

namespace scu::touch {
bool Rect::contains(Point p) const {
    return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
}
bool Rect::containsCircle(Point p) const {
    if (w <= 0 || h <= 0)
        return false;
    const float dx = (p.x - x - w * .5f) / (w * .5f);
    const float dy = (p.y - y - h * .5f) / (h * .5f);
    return dx * dx + dy * dy <= 1;
}
Profile profileForGame(std::string_view id, int family) {
    if (id == "spinball") return Profile::Pinball;
    if (id == "sonic1" || id == "sonic2" || id == "sonic3" || id == "sandk" ||
        id == "sonic3k" || id == "sonic2k" || id == "sphere" ||
        (id == "compilation" && (family == 1 || family == 2)))
        return Profile::Jump;
    if (id == "3dblast" || id == "meanbean" || id == "compilation")
        return Profile::TwoActions;
    return Profile::ABC;
}
static Rect keep(Rect r, Rect safe) {
    r.w = std::min(r.w, safe.w);
    r.h = std::min(r.h, safe.h);
    r.x = std::clamp(r.x, safe.x, safe.x + safe.w - r.w);
    r.y = std::clamp(r.y, safe.y, safe.y + safe.h - r.h);
    return r;
}
Layout arrange(Rect safe, const Options &o) {
    safe.w = std::max(1.f, safe.w);
    safe.h = std::max(1.f, safe.h);
    Layout l;
    l.safe = l.canvas = safe;
    l.profile = o.pinball ? Profile::Pinball : o.profile;
    l.circularControls = !o.menu;
    if (o.watch) {
        const float d = std::min(safe.w, safe.h);
        const float ox = safe.x + (safe.w - d) * .5f, oy = safe.y + (safe.h - d) * .5f;
        auto r = [&](float x, float y, float w, float h) {
            return Rect{ox + x * d, oy + y * d, w * d, h * d};
        };
        auto add = [&](Action a, float x, float y, float w, float h) {
            if (o.leftHanded && !o.menu &&
                (l.profile != Profile::Pinball || (a != Action::A && a != Action::B)))
                x = 1 - x - w;
            l.buttons.push_back({r(x, y, w, h), a, !o.menu});
        };
        if (o.menu) {
            // The menu owns the full display. Its content is inset for the bezel,
            // and both a swipe and the crown can scroll without a control dock.
            l.canvas = r(0, 0, 1, 1);
        } else {
            l.canvas = r(0, 0, 1, 1);
            add(Action::Menu, .732f, .152f, .136f, .136f);
            if (o.controls) {
                add(Action::Utility, l.profile == Profile::Pinball ? .132f : .252f,
                    l.profile == Profile::Pinball ? .152f : .772f, .136f, .136f);
                if (o.utilityBank) {
                    add(Action::Overview, .384f, .224f, .232f, .232f);
                    add(Action::ZoomOut, .144f, .474f, .232f, .232f);
                    add(Action::ZoomIn, .624f, .474f, .232f, .232f);
                    add(Action::Rewind, .384f, .514f, .232f, .232f);
                } else if (l.profile == Profile::Pinball) {
                    l.stick = r(.364f, .284f, .272f, .272f);
                    add(Action::A, .124f, .604f, .232f, .232f);
                    add(Action::B, .644f, .604f, .232f, .232f);
                    add(Action::Start, .422f, .822f, .156f, .156f);
                } else {
                    l.stick = r(o.leftHanded ? .624f : .104f, .504f, .272f, .272f);
                    if (l.profile == Profile::Jump)
                        add(Action::A, .644f, .534f, .232f, .232f);
                    else {
                        add(Action::A, .484f, .584f, .204f, .204f);
                        add(Action::B, .708f, .494f, .204f, .204f);
                        if (l.profile == Profile::ABC)
                            add(Action::C, .484f, .354f, .204f, .204f);
                    }
                    add(Action::Start, .422f, .822f, .156f, .156f);
                }
            }
        }
        return l;
    }
    const float pad = std::min({std::max(4.f, o.margin), safe.w * .03f, safe.h * .03f});
    if (o.menu) {
        float dock = std::min(54.f, safe.h * .2f);
        l.canvas.h -= dock;
        const Action actions[] = {Action::Back, Action::Previous, Action::Next, Action::ScrollUp,
                                  Action::ScrollDown};
        float w = safe.w / 5;
        for (int i = 0; i < 5; ++i)
            l.buttons.push_back(
                {{safe.x + i * w + 2, safe.y + safe.h - dock + 2, std::max(1.f, w - 4), dock - 4},
                 actions[i]});
        return l;
    }
    float utility = std::min(48.f, safe.h * .14f);
    float utilityW = std::min(66.f, safe.w / 5);
    const Action utilities[] = {Action::Menu, Action::Overview, Action::ZoomOut, Action::ZoomIn};
    for (int i = 0; i < 4; ++i)
        l.buttons.push_back(
            {{safe.x + pad + i * utilityW, safe.y + pad, utilityW - 3, utility - pad},
             utilities[i]});
    if (!o.controls)
        return l; // Menu remains reachable with a connected gamepad.
    l.deck = o.layout == 2 || (o.layout == 0 && safe.h > safe.w * 1.1f);
    float deckH = std::min(290.f * std::clamp(o.scale, .65f, 1.6f), safe.h * .42f);
    if (l.deck)
        l.canvas.h -= deckH;
    Rect controls = safe;
    if (l.deck) {
        controls.y += safe.h - deckH;
        controls.h = deckH;
    }
    float side = std::min({140.f * o.scale, safe.w * .34f, controls.h * .68f});
    float y = controls.y + controls.h - side - pad;
    float left = safe.x + pad, right = safe.x + safe.w - side - pad;
    float sx = o.leftHanded ? right : left, ax = o.leftHanded ? left : right;
    l.stick = keep({sx + o.stickX, y + o.stickY, side, side}, controls);
    Rect cluster = keep({ax + o.buttonsX, y + o.buttonsY, side, side}, controls);
    // Keep only the actions needed by the selected game's default mapping.
    float b = side * .42f;
    if (l.profile == Profile::Pinball) {
        l.buttons.push_back({{safe.x + pad, y, side * .5f, side * .5f}, Action::A, true});
        l.buttons.push_back({{safe.x + safe.w - pad - side * .5f, y, side * .5f, side * .5f}, Action::B, true});
        l.stick = keep({safe.x + (safe.w - side * .7f) * .5f, y - side * .35f, side * .7f, side * .7f}, controls);
    } else if (l.profile == Profile::Jump)
        l.buttons.push_back({{cluster.x + side * .15f, cluster.y + side * .15f, side * .7f, side * .7f}, Action::A, true});
    else {
        l.buttons.push_back({{cluster.x, cluster.y + side - b, b, b}, Action::A, true});
        l.buttons.push_back({{cluster.x + side - b, cluster.y + side - b, b, b}, Action::B, true});
        if (l.profile == Profile::ABC)
            l.buttons.push_back({{cluster.x + (side - b) * .5f, cluster.y, b, b}, Action::C, true});
    }
    float centerW = std::min(64.f, safe.w * .19f), centerH = std::min(44.f, controls.h * .23f);
    float cx = safe.x + (safe.w - centerW) * .5f;
    l.buttons.push_back(
        {{cx, controls.y + controls.h - centerH - pad, centerW, centerH}, Action::Start});
    l.buttons.push_back(
        {{cx, controls.y + controls.h - centerH * 2 - pad * 2, centerW, centerH}, Action::Rewind});
    return l;
}
Action Input::down(int64_t id, Point p, const Layout &l) {
    up(id);
    Action a = (l.circularControls ? l.stick.containsCircle(p) : l.stick.contains(p)) ? Action::Stick : Action::None;
    if (a == Action::None)
        for (const auto &b : l.buttons)
            if (b.contains(p)) {
                a = b.action;
                break;
            }
    if (a == Action::None)
        return a;
    // A second finger cannot take over the movement stick from the first one.
    if (a == Action::Stick)
        for (const auto &f : fingers)
            if (f.used && f.action == a)
                return Action::None;
    for (auto &f : fingers)
        if (!f.used) {
            f = {true, id, a, p};
            return a;
        }
    return Action::None;
}
void Input::move(int64_t id, Point p) {
    for (auto &f : fingers)
        if (f.used && f.id == id)
            f.p = p;
}
void Input::up(int64_t id) {
    for (auto &f : fingers)
        if (f.used && f.id == id)
            f.used = false;
}
void Input::clear() {
    for (auto &f : fingers)
        f.used = false;
}
State Input::state(const Layout &l, float deadzone) const {
    State s;
    for (const auto &f : fingers)
        if (f.used) {
            // Match libretro's stable joypad ABI without bringing an emulator into layout tests.
            int bit = f.action == Action::A       ? 9
                      : f.action == Action::B     ? 0
                      : f.action == Action::C     ? 8
                      : f.action == Action::Start ? 3
                                                  : -1;
            bool inside = false;
            for (const auto &b : l.buttons)
                if (b.action == f.action && b.contains(f.p))
                    inside = true;
            if (bit >= 0 && inside)
                s.buttons |= uint16_t(1u << bit);
            if (f.action == Action::Rewind && inside)
                s.rewind = true;
            if (inside && f.action == Action::ScrollUp)
                --s.scroll;
            if (inside && f.action == Action::ScrollDown)
                ++s.scroll;
            if (f.action == Action::Stick && l.stick.w > 0) {
                float x = (f.p.x - l.stick.x - l.stick.w * .5f) / (l.stick.w * .5f);
                float y = (f.p.y - l.stick.y - l.stick.h * .5f) / (l.stick.h * .5f);
                float length = std::sqrt(x * x + y * y);
                if (length > 1) {
                    x /= length;
                    y /= length;
                }
                s.stick = {x, y};
                deadzone = std::clamp(deadzone, .1f, .8f);
                if (std::abs(x) > deadzone)
                    s.buttons |= uint16_t(1u << (x < 0 ? 6 : 7));
                if (std::abs(y) > deadzone)
                    s.buttons |= uint16_t(1u << (y < 0 ? 4 : 5));
            }
        }
    return s;
}
} // namespace scu::touch
