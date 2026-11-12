# Touch and movement

On an Android phone, tablet or Wear OS watch, open **Controls** from the options menu. Touch and gamepad controls continue to work when movement controls are enabled.

## Just the actions your game needs

The automatic touch layout uses one **Jump** button for Sonic 1, 2, 3, Sonic & Knuckles, their supported lock-ons and Blue Sphere. Sonic Classics changes the layout when its Sonic 1 or 2 engine is detected. The directional pad still supports looking, crouching and the optional Sonic 1 spin dash.

**Spinball** has independent **L** and **R** flippers. Touch both together to launch; use the center directional pad to guide Sonic or move on foot. Left-handed mode keeps the flippers on their physical left/right sides. In the original default control configuration, A is the left flipper, B the right and C both. [SEGA's Spinball manual](https://manuals.sega.com/genesismini/pdf/SONIC_SPINBALL.pdf).

3D Blast and Mean Bean use two actions. For a custom in-game button configuration, switch to **Advanced → Controls → Touch actions → Full A / B / C**. The compact automatic layouts are intended for the games' default mappings. [SEGA's 3D Blast manual](https://www.sega.jp/genesismini2/assets/manual/pdf/US_Sonic-3D-Blast.pdf).

The watch has thin neutral outlines and almost clear button interiors. **…** opens options; **+** switches to the map, zoom and rewind bank. The crown still zooms, and the native Start button remains available in the game bank. Button opacity changes the drawing, while the touch targets keep their size.

## Optional tilt and flick

1. Select **Motion + touch → Tilt left / right**, or **Tilt in all directions** for up/down too.
2. Hold a comfortable pose and choose **Center now**. The live dot shows filtered movement. Resuming play also takes a fresh neutral pose.
3. Optionally enable **Flick: jump / both flippers**. A short movement triggers Jump, or both flippers in Spinball. A release threshold and cooldown prevent one long shake from repeating.
4. Advanced settings adjust sensitivity, neutral zone, gesture strength and each axis direction. Direct touch/gamepad directions override motion on the axis being used; buttons combine.

The bridge prefers Android's game rotation vector, then rotation vector, gravity or a filtered accelerometer. Fused sensors use the motion hardware available on the device; linear acceleration supplies gestures where available. The request is 50 Hz while playing or previewing the Controls page. Listeners stop when leaving the app, and no motion samples are stored. Unsupported devices retain touch/gamepad controls. [Android motion sensor documentation](https://developer.android.com/develop/sensors-and-location/sensors/sensors_motion).

## Watch picture

The round lens reaches the actual screen rim. Lens strength adjusts center magnification without shrinking the picture. Appearance presets preserve the lens choice. The original-screen endpoint fills its source canvas without integer-scale margins; turning the lens off explicitly restores an aspect-correct rectangle. Whole-stage statistics stay small and flat after the lens. The game can contain its own dark artwork or empty stage space; these are different from a presentation margin.

The controls and layout are independently implemented in C++ with the owner's OrbitEmu as the visual and space-use reference. Physical sensor delivery has been checked on Galaxy Watch8 Classic; generated tests cover mapping, neutral poses, rotation, invalid/stale input, gesture rearming and input priority. Wrist comfort, gesture sensitivity and other hardware models require human use; these checks are not a certification of sustained 60 fps or every stage.
