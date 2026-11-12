#pragma once
#include <cstdint>
namespace scu::motion {
struct Vec { float x = 0, y = 0, z = 0; };
struct Quaternion { float x = 0, y = 0, z = 0, w = 1; };
struct Sample {
    Quaternion orientation;
    Vec gravity, acceleration;
    uint64_t milliseconds = 0;
    int source = 0, rotation = 0; // 1: fused orientation, 2: gravity, 3: accelerometer
};
struct Options {
    int mode = 0, sensitivity = 100, deadzone = 20, jumpGesture = 1, gestureSensitivity = 100;
    bool invertX = false, invertY = false;
};
struct State { float x = 0, y = 0; uint16_t directions = 0; bool jump = false, ready = false; uint64_t jumpSequence = 0; };
class Controller {
    Options options;
    Quaternion neutral;
    Vec neutralGravity;
    uint64_t lastSample = 0, calibratedAt = 0, lastJump = 0, pulseUntil = 0, sequence = 0, lastLowering = 0;
    int source = 0, rotation = 0;
    bool calibrated = false, gestureArmed = true;
    float filteredX = 0, filteredY = 0;
    float vertical = 0, velocity = 0, quietTime = 0, previousLift = 0;
    State current;
  public:
    void configure(Options value);
    void recenter();
    void update(const Sample &sample);
    State state(uint64_t now) const;
    uint64_t jumpSequence() const { return sequence; }
};
// Deliberate touch/gamepad directions take priority on each axis; actions combine.
uint16_t combine(uint16_t motion, uint16_t direct);
} // namespace scu::motion
