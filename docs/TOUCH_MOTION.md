# Touch and movement

On an Android phone, tablet or Wear OS watch, open **Controls** from the options menu. Touch and gamepad controls continue to work when movement controls are enabled.

## Just the actions your game needs

The automatic touch layout uses one **Jump** button for Sonic 1, 2, 3, Sonic & Knuckles, their supported lock-ons and Blue Sphere. Sonic Classics changes the layout when its Sonic 1 or 2 engine is detected. The directional pad still supports looking, crouching and the optional Sonic 1 spin dash.

**Spinball** has independent **L** and **R** flippers. Touch both together to launch; use the center directional pad to guide Sonic or move on foot. Left-handed mode keeps the flippers on their physical left/right sides. In the original default control configuration, A is the left flipper, B the right and C both. [SEGA's Spinball manual](https://manuals.sega.com/genesismini/pdf/SONIC_SPINBALL.pdf).

3D Blast and Mean Bean use two actions. For a custom in-game button configuration, switch to **Advanced → Controls → Touch actions → Full A / B / C**. The compact automatic layouts are intended for the games' default mappings. [SEGA's 3D Blast manual](https://www.sega.jp/genesismini2/assets/manual/pdf/US_Sonic-3D-Blast.pdf).

The watch has thin neutral outlines and almost clear button interiors. **…** opens options; **+** switches to the map, zoom and rewind bank. The crown still zooms, and the native Start button remains available in the game bank. Button opacity changes the drawing, while the touch targets keep their size.

## Tilt, wrist lift and swipe

1. Select **Motion + touch → Tilt left / right**, or **Tilt in all directions** for up/down too.
2. Hold a comfortable pose and choose **Center now**. The live dot shows filtered movement. Resuming play also takes a fresh neutral pose.
3. **Jump gesture → Raise wrist** is the default once movement controls are enabled. Lift the wrist briefly, or tilt upward quickly from the centered pose. It detects a gesture, not absolute hand height. Return to rest before repeating. **Quick flick** is a stronger all-direction alternative; **Off** disables sensor jumping.
4. **Swipe up to jump** is also enabled in motion mode. Swipe upward on a clear area of the game, away from buttons and the directional pad. Lift your finger before another swipe. Menus, the pad and utility buttons keep their own actions. Spinball gestures activate both flippers.
5. Advanced settings adjust tilt sensitivity, neutral zone, gesture sensitivity and axis direction. Direct touch/gamepad directions override motion on the axis being used; buttons combine. A higher gesture sensitivity requires a smaller movement. The live preview shows **Jump detected** when a sensor gesture triggers.

The bridge prefers Android's game rotation vector, then rotation vector, gravity or a filtered accelerometer. Fused sensors use the motion hardware available on the device; linear acceleration supplies gestures where available. Upward acceleration is interpreted in the world's vertical direction using the current posture. Filtering, a short velocity estimate, a resting threshold and cooldown reject tremors, repeated holds and braking while lowering the wrist. The request is 50 Hz while playing or previewing Controls. A bounded 32-sample memory queue retains events between display frames; original sensor timestamps keep the two sensor streams from repeatedly recalibrating each other. Samples are never written to disk. Listeners stop when leaving the app. Unsupported devices retain touch/gamepad controls. [Android motion sensor documentation](https://developer.android.com/develop/sensors-and-location/sensors/sensors_motion).

## Jump reliability

Version **0.13.1 fixes the console A mapping** used by touch and wrist jumping. The old mask selected an extra six-button action that regular Sonic games ignored; the same error also affected Spinball's left touch flipper. All touch and gesture paths now share the console A/B/C mapping. A brief tap is retained for at least 90 ms, including a down/up pair received between two game frames. Holding Jump keeps its pointer capture when the thumb drifts beyond the drawing, and releasing or leaving the game clears it.

The regression test runs an original 68000 cartridge that configures and reads the native controller port, checking actual A/B/C electrical states for the phone/watch layouts. It includes the old wrong mask as a negative control. This goes beyond checking an internal button variable. Android input checks separately exercise actual touch events and Sonic's airborne state.

## Watch picture

The round lens reaches the actual screen rim. Lens strength adjusts center magnification without shrinking the picture. Appearance presets preserve the lens choice. The original-screen endpoint fills its source canvas without integer-scale margins; turning the lens off explicitly restores an aspect-correct rectangle. Whole-stage statistics stay small and flat after the lens. The game can contain its own dark artwork or empty stage space; these are different from a presentation margin.

The controls and layout are independently implemented in C++ with the owner's OrbitEmu as the visual and space-use reference. Historical physical sensor delivery was checked on Galaxy Watch8 Classic in 0.13.0. The 0.13.1 checks distinguish native-controller probes, generated sensor streams and Android emulator input from human wrist use. Wrist comfort, gesture sensitivity and other hardware models still require human use; these checks do not certify sustained 60 fps or every stage.
