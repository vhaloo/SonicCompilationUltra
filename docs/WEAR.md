# Wear OS preview

**A separate standalone watch APK**, with a round-screen layout, crown/bezel zoom and an optional fisheye. It does not require the phone app. The build targets Android 8.0 / API 26 or newer, OpenGL ES 2.0 and ARM64, ARMv7 or x86-64. Compatible Android rotary input is detected automatically; no model-name whitelist is used.

This is an **experimental development preview**. Version 0.13.0 extends the round picture to the screen rim, adds discreet game-specific controls and optional combined motion input. The colored menu uses the original palm backdrop and a single navigation row. The ARMv7 scanline crash fix from 0.12.3 is retained. [Physical checks and limits](TEST_REPORT.md) distinguish completed sessions, generated checks and build-only coverage; other models, sustained 60 fps and battery endurance remain unqualified.

## Get ready

1. Download **Android-Wear-ROM-free.apk**, then sideload it using your usual Wear OS ADB or companion sideload tool. The phone/tablet and Quest APKs are separate downloads.
2. Open the library and choose your own supported cartridge. Public packages contain no ROMs. If the watch has no system document picker, copy your own cartridge files into the app's ROM inbox below, then tap **Choose ROM…**. The app copies supported files into private storage and imports them; return to the library when ready. Originals and existing private copies are kept.
3. Tap a colored game row to play. **…** or Android **Back** opens options. Swipe vertically or rotate the crown to scroll. Tap the section name beside **Back** to choose another page or switch between Simple and Advanced.

Local USB/Wi-Fi ADB inbox, after installing the app:

```sh
adb shell mkdir -p /sdcard/Android/data/com.vhaloo.soniccompilationultra.wear/files/ROMs
adb push YOUR_OWN_CARTRIDGE.zip /sdcard/Android/data/com.vhaloo.soniccompilationultra.wear/files/ROMs/
```

Only local cartridge files up to 32 MB are accepted. The app neither downloads ROMs nor requests network/storage-wide permissions. See the [cartridge guide](ROMS.md).

## Zoom without losing the level

- Rotate the crown or compatible rotating bezel while playing. **0%** fits the complete known stage; **100%** reaches the original console viewport and follows its camera. The transition is continuous.
- **Watch / crown** contains sensitivity, reverse direction, the two endpoint buttons, fisheye and lens strength. **Reduce motion** makes zoom changes immediate.
- The optional lens reaches the actual screen rim at every strength. Strength changes center magnification without shrinking the picture; presets preserve the choice. Native zoom fills its source canvas without integer-scale margins. Touch targets, overview statistics and menus remain flat. Disable the lens for a conventional aspect-correct rectangle.
- Tap **+** to switch between game controls and the map/zoom/rewind bank. Regular Sonic games have one **Jump** button; Spinball has independent **L/R** flippers, with both together to launch and a central directional pad. 3D Blast and Mean Bean have two actions. **Advanced → Controls** can restore full A/B/C for a custom in-game mapping. Direction and face buttons accept simultaneous fingers. A connected gamepad can hide the touch controls; options remain reachable.
- On a square watch the game keeps a flat, aspect-correct picture. Scenes without a supported map decoder retain their native view. “Whole stage” means the currently supported map, not undiscovered scripted content.

The new installation targets 60 fps with automatic scene sampling, effects and a 30% work reserve. It uses a small scene ceiling, a retained lens mesh and a 24 MB rewind profile. The system and native simulation still set practical limits. [Performance choices](AUTOMATIC_QUALITY.md) explain the tradeoffs.

**Controls → Motion + touch** optionally adds calibrated left/right or all-direction tilt and a flick action. Android fuses available motion sensors; gravity or filtered acceleration provides a fallback. Touch/gamepad directions take priority on the axis being used. Sampling stops in the background. Motion starts off; use **Center now** in a comfortable position before playing. [Touch and movement guide](TOUCH_MOTION.md).

**Video → Depth and voxels** adds optional pixel volumes, including with the crown and round lens. Watch geometry and texture limits are smaller. For the lowest drawing cost, keep the default Flat mode. [Depth guide](VOXELS.md).

![Original illustrative examples, not watch captures](illustrations/settings-preview-gallery.png)

The menu uses the same English/French/Spanish/Japanese setting explanations as desktop and XR. Tap a setting's icon for the illustrated guide. The examples are original demo art, not cartridge or device screenshots.

Keep save states on the device architecture that created them. The native emulator's older save format is not portable between a 64-bit PC and an ARMv7 watch. Cartridge files can be imported independently.

Platform references: [Android rotary input](https://developer.android.com/training/wearables/compose/rotary-input), [standalone Wear OS manifest and storage](https://developer.android.com/training/wearables/apps/standalone-apps). The lens follows the round-display design used in the owner's OrbitEmu project; its C++ implementation and controls are independent.
