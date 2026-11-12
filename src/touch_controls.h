#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <string_view>

namespace scu::touch {
struct Point {
    float x = 0, y = 0;
};
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(Point p) const;
    bool containsCircle(Point p) const;
};
enum class Profile { ABC, Jump, TwoActions, Pinball };
Profile profileForGame(std::string_view id, int family = 0);
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
    ScrollDown,
    Utility
};
// Genesis Plus GX maps console A/B/C to libretro Y/B/A. libretro X (9)
// is the console's six-button Y and is ignored by three-button games.
constexpr uint16_t actionMask(Action action) {
    return action == Action::A ? uint16_t(1u << 1) :
           action == Action::B ? uint16_t(1u << 0) :
           action == Action::C ? uint16_t(1u << 8) :
           action == Action::Start ? uint16_t(1u << 3) : 0;
}
struct Button {
    Rect rect;
    Action action = Action::None;
    bool circular = false;
    bool contains(Point p) const { return circular ? rect.containsCircle(p) : rect.contains(p); }
};
struct Options {
    int layout = 0; // Auto, overlay, separate deck
    float scale = 1, margin = 8;
    bool leftHanded = false, controls = true, menu = false, pinball = false;
    bool watch = false, circular = false, utilityBank = false;
    Profile profile = Profile::ABC;
    float stickX = 0, stickY = 0, buttonsX = 0, buttonsY = 0; // offsets in dp
};
struct Layout {
    Rect safe, canvas, stick;
    std::vector<Button> buttons;
    bool deck = false;
    bool circularControls = false;
    Profile profile = Profile::ABC;
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
    std::array<uint64_t, 16> pulseUntil{};

  public:
    Action down(int64_t id, Point p, const Layout &layout, uint64_t now = 0);
    void move(int64_t id, Point p);
    void up(int64_t id);
    void clear();
    State state(const Layout &layout, float deadzone, uint64_t now = 0) const;
};
class SwipeJump {
    struct Finger { int64_t id=0; Point origin{}; uint64_t started=0; bool active=false, fired=false; };
    std::array<Finger, 8> fingers{};
    uint64_t pulseUntil=0;
    float distance=24;
  public:
    void down(int64_t id, Point p, uint64_t now, float threshold=24);
    void move(int64_t id, Point p, uint64_t now);
    void up(int64_t id);
    void clear();
    bool pressed(uint64_t now) const { return now && now < pulseUntil; }
};
} // namespace scu::touch
