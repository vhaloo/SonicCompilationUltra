# Standalone Meta Quest VR / AR preview

This experimental Android ARM64 edition runs the collection on the headset. It presents the original games on a tracked spatial screen: a room-pinned cinema, a tilted whole-stage tabletop, or a head-following panel. Passthrough AR shows the room around that screen. The game simulation, cartridge graphics, saves, rewind and illustrated options use the same collection engine as the desktop app.

**0.5 wall AR:** choose a scanned wall, a fixed two-metre stage mural or a floor atlas. Local anchors, manual placement and a separate nearby menu provide several ways to use the same games. Maximum available headset refresh is the default preference. See the [wall placement, modes and performance guide](QUEST_WALLS.md).

**Status:** built for Quest 2, Quest Pro, Quest 3 and Quest 3S with OpenGL ES 3.2 and OpenXR. Physical headset behavior, image orientation/color, tracking, passthrough, input, comfort and performance have not been verified. No game or headset session is run as part of this delivery. A successful build or APK signature check does not establish those runtime properties.

## Install

1. Enable developer mode for your Quest using [Meta's supported process](https://developers.meta.com/vr/documentation/android-apps/enable-developer-mode/) and approve USB debugging on your own headset.
2. Sideload the APK with SideQuest or `adb install -r path/to/collection.apk`.
3. Open **Sonic Compilation Ultra XR** in the headset's Unknown Sources / developer app list.

Meta also documents the [ADB installation commands](https://developers.meta.com/vr/documentation/native/android/ts-adb/). Installing and running on a headset are owner-controlled steps; this delivery performs neither.

The ordinary APK contains no ROMs. **Choose ROM…** opens Android's document picker; select a cartridge dump or ZIP that you are entitled to use. The app copies it into its own local ROM folder. No storage-wide permission, microphone access, network service or camera-image access is requested. Passthrough is composed by the headset runtime.

A separately prepared **private personal APK** can contain the owner's cartridges. Its first launch copies those files into local app storage. Subsequent launches and APK updates preserve files that already exist. Private ROM-containing APKs and archives stay outside the public repository and public releases.

The personal 0.5 test APK opts into a one-time wall profile, including on upgrade: two-metre AR mural, whole-stage framing, locked placement, nearby options, adaptive Balanced detail and maximum-available refresh preference. Existing settings are backed up before this change; later user choices are retained. The ROM-free APK has no opt-in flag.

## Touch controls

| Action | Quest Touch |
|---|---|
| Move | Left stick |
| Jump / game action | A or right trigger |
| Original Start / pause | B during play |
| Open / close app options | Y |
| Switch whole-stage / pixel view; toggle close-up in fixed wall modes | X during play |
| Confirm / back in options | A / B |
| Explain the focused setting | X in options |
| Point and click | Right controller aim + right trigger |
| Scroll menus | Right stick |
| Previous / next section | Left / right grip separately |
| Recenter screen | Right stick click |
| Move and resize a room-pinned screen | Hold both grips; move hands together or apart |
| Rewind with reverse sound | Hold left trigger |

The system Meta button remains the headset's control. A connected Bluetooth gamepad can use the existing menus; turn **Use Quest Touch controls** off under **Quest VR / AR** to give the gamepad the first player slot. Physical controller compatibility remains unverified.

The right-controller menu pointer and Y menu access stay available when Touch gameplay is switched off, so you can turn it back on. Touch is routed to player one when enabled, including when a Bluetooth pad was connected first. Headset session suspension resets the game-loop time reference instead of accumulating a catch-up burst.

Touch always uses its left trigger for rewind. The ordinary gamepad rewind binding is retained for Bluetooth-pad mode, so changing it does not leave Touch without rewind.

## Make the screen yours

The **Quest VR / AR** section includes passthrough, room/head anchoring, width (80–800 cm), distance (80–600 cm), vertical offset, tabletop tilt, resolution, Touch input, pointer-click vibration, an orientation compatibility switch, recentering and cinema/tabletop presets. Each control has an original diagram and explanations in English, French, Spanish and Japanese.

- **Cinema:** 320 cm wide, 240 cm ahead, at eye height.
- **Tabletop:** 220 cm wide, 140 cm ahead, 60 cm below the eyes, tilted 65°, with whole-stage view.
- **Canvas choices:** 1280×720, 1920×1080, 2560×1440 or ultra-wide 3072×576 (48:9).

Ordinary room-pinned placement is local to the session. Wall modes can optionally store a local spatial anchor and restore it in a recognizable space. Auto placement reads a captured room after spatial-data permission; manual placement remains available without it. Original whole-map, panorama, 1/8 zoom and native framing remain available in Spatial screen mode; fixed modes retain whole-stage framing. Special projections keep their native screen.

**Pixel 1:1 refers to game pixels in the virtual canvas.** The headset compositor and optics resample the final image. Start with Lightweight rendering or the 720p canvas if needed; larger canvases increase GPU and memory load. No Quest framerate or battery-life measurements are claimed.

## Local saves and updates

ROMs, settings and saves use the app's external-files directory, normally under `Android/data/com.vhaloo.soniccompilationultra.quest/files/`. Android may restrict direct access; use your own authorized file/ADB tools to back up that directory. Uninstalling the app can remove its data. Updating the same package with the same signing key preserves its app data under Android's normal update behavior.

Private builds copy raw cartridge SRAM where supplied. Windows emulator snapshots are preserved in a separate desktop backup; Quest creates its own native snapshots. Cross-architecture snapshot compatibility is unverified. Keep the original PC data.

## Build without game execution

Use JDK 21, Android SDK platform 36, CMake 3.31.6 and NDK 29.0.14206865. Gradle and Android Gradle Plugin are pinned by the project. The application targets API 32 for the standalone Quest preview; it is a sideload build, not a Meta Store submission.

```sh
python tools/prepare_quest.py
cd android
./gradlew --no-daemon --max-workers=2 :app:assembleRelease :app:lintRelease
```

On Windows use `gradlew.bat`. Configure the Android SDK in an ignored `android/local.properties` or `ANDROID_HOME`. SDL 2.32.10 is downloaded with a SHA-256 check. The OpenXR Android loader is pinned to Maven Central version 1.1.65. The SDL renderer draws once into an offscreen canvas; a GPU framebuffer blit transfers it to an OpenXR quad swapchain. The compositor handles tracked stereo placement and optional `XR_FB_passthrough` reconstruction.

Without a supplied signing key, local/CI builds use Android's development key. For repeatable personal updates, retain your own key and set `SCU_QUEST_KEYSTORE` and `SCU_QUEST_KEY_PASSWORD` in the build environment. Keep keys and passwords private. `-PscuPrivateAssets=/outside/repository/assets` adds a local `scu/ROMs` and `scu/UserData` tree; the default build excludes it.

Optional wall features use `XR_FB_scene`, spatial-entity query/storage, scene capture and local floor reference spaces when reported by the runtime. The manifest declares Meta Scene and anchor permissions; the app requests Scene consent for wall discovery. Its saved anchor UUID and bounds stay in local `UserData/quest-wall-anchor.txt`. No cloud anchor or camera-image API is used. `XR_FB_display_refresh_rate` enumerates rates and requests the selected preference, with a system fallback if declined.

Lint exceptions are limited to three pinned SDL files implementing optional Bluetooth Steam-controller and microphone-capture paths. The application requests neither permission; SDL guards Bluetooth initialization, and this frontend never opens audio capture. The app's own Activity and manifest remain checked. Upstream compiler warnings are recorded separately from app build errors.

Static APK inspection should check its signature, ARM64 libraries, SDL entry point, manifest, packaged fonts/loader, ROM-free default boundary and the hashes of any private cartridges. None of these checks validates headset runtime behavior.
