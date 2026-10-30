#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace scu::touch {
struct Point {
    float x = 0, y = 0;
};
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(Point p) const;
};
enum class Action {
    None,
    Stick,
    A,
    B,
    C,
    Start,
    Rewind,
    Menu,
    Overview,
    ZoomIn,
    ZoomOut,
    Back,
    Previous,
    Next,
    ScrollUp,
    ScrollDown
};
struct Button {
    Rect rect;
    Action action = Action::None;
};
struct Options {
    int layout = 0; // Auto, overlay, separate deck
    float scale = 1, margin = 8;
    bool leftHanded = false, controls = true, menu = false, pinball = false;
    float stickX = 0, stickY = 0, buttonsX = 0, buttonsY = 0; // offsets in dp
};
struct Layout {
    Rect safe, canvas, stick;
    std::vector<Button> buttons;
    bool deck = false;
};
Layout arrange(Rect safe, const Options &options);
struct State {
    uint16_t buttons = 0;
    bool rewind = false;
    Point stick{};
    int scroll = 0;
};
class Input {
    struct Finger {
        bool used = false;
        int64_t id = 0;
        Action action = Action::None;
        Point p;
    };
    std::array<Finger, 16> fingers{};

  public:
    Action down(int64_t id, Point p, const Layout &layout);
    void move(int64_t id, Point p);
    void up(int64_t id);
    void clear();
    State state(const Layout &layout, float deadzone) const;
};
} // namespace scu::touch
