# Standalone Meta Quest XR preview 0.6

Play your own cartridges on a giant AR mural, a complete-board pinball cabinet, a 3D CRT, a curved gallery, a floor atlas or a simple spatial screen. **Y opens the options.** Every control has an illustrated explanation in English, French, Spanish and Japanese.

**Experimental preview.** Library/options startup and stereo preview rendering have been checked on a USB-connected Quest 3 with cartridge execution locked. No ROM was launched or played for this release. Gameplay, physical wall alignment, room relocalization, real-controller coverage and sustained performance remain unqualified. [What was verified](TEST_REPORT.md).

## Install and add your cartridges

1. Enable developer mode using [Meta's instructions](https://developers.meta.com/vr/documentation/android-apps/enable-developer-mode/), connect USB and approve debugging in your headset.
2. Install the ROM-free APK with SideQuest or `adb install -r collection.apk`.
3. Open **Sonic Compilation Ultra XR** from Unknown Sources. Choose **Choose ROM…** and select your own supported cartridge file or ZIP. [Required editions](ROMS.md).
4. Choose a game and **Play**. Use **B** for the original Start button; **Y** returns to the collection options.

The public APK contains no cartridges, saves, extracted game artwork or private signing keys. A personal APK may package its owner's files locally; that edition must stay private. The document picker copies selected files into app-owned storage. No storage-wide, microphone, internet or camera-image permission is requested. Passthrough is composed by the headset runtime.

## Choose how to play

| Presentation | Use it for |
|---|---|
| Full-stage AR mural | Platformers fixed on a wall, initially two metres high. Turn or move to inspect the supported current stage without camera scrolling. |
| Fit a scanned wall | Keep the display inside a captured wall's bounds. Manual placement is also available. |
| Floor atlas | Lay the supported whole stage horizontally. |
| 3D full-board pinball | Spinball's complete board inside an original 3D cabinet with rails, legs, glass and native backglass view. Selected automatically for Spinball unless disabled. |
| 3D CRT + surrounding world | A curved tube in a 3D enclosure. The television shows the authentic native viewport; supported terrain extends around it at the matching pixel scale. |
| Curved level gallery | Wrap the supported stage through 10–220 degrees. |
| Spatial screen | Cinema, tilted tabletop or head-following screen with the collection's normal camera choices. |

New Quest installations start with the AR mural, automatic Spinball cabinet, nearby options and **Maximum available** refresh preference. Existing settings are retained. Personal 0.6 builds can opt into these defaults once, keeping a backup of previous settings. Later choices are preserved.

Spinball uses its existing four-board cartridge terrain decoder; the 3D cabinet is presentation furniture around the authentic 2D playfield and original physics. Distant objects still obey cartridge activation rules. Unsupported scenes, puzzle boards and special projections keep their native image. This is an independent fan project, not an official remake.

## Resize and control it

| Action | Quest Touch |
|---|---|
| Open / close options | Y |
| Confirm / back | A / B |
| Explain the focused setting | X in options |
| Point / click | Right controller aim / trigger |
| Navigate / scroll | Left stick / right stick |
| Previous / next section | Left / right grip separately |
| Recover nearby menu | Right stick click (R3) |
| Resize | Hold both grips and spread or close your hands; release and regrip to continue |
| Continuous large resizing | Both grips + right-stick up/down |
| Move the placement | Unlock placement, then move both gripped hands together |
| Rewind with reverse sound | Hold left trigger |
| Jump / action | A or right trigger |
| Original Start | B during play |

**Placement locking does not prevent resizing.** The physical scale slider spans 5–20,000%, with half/double/reset buttons. Physical size is independent of texture allocation. Wall-fit mode remains bounded by its wall; use Mural for unrestricted expansion. Options keep their own readable size, and R3 brings them near you. The Meta button keeps its system function.

Pinball width, front height, slope and glass reflections are adjustable. CRT choices include clean, living-room and arcade presets, effect strength, phosphor mask, real tube curvature, picture width, surrounding terrain and its brightness. Choose a quiet studio floor or dark void in VR, or passthrough AR for your room. Cinema, tabletop, head-follow, cabinet and curved-gallery presets are available.

A Bluetooth gamepad can use the existing controller menus. Disable **Use Quest Touch controls** to assign that pad to player one; menu pointing and Y access remain available. Physical controller behavior still needs user validation.

## Walls, detail and refresh

See the [wall guide](QUEST_WALLS.md) for captured rooms, manual/outdoor placement, floor calibration, local anchors and tracking recovery. Room recognition needs suitable spatial permission and tracking; outdoor wall recognition is not guaranteed. Manual placement does not require Scene permission.

The app requests the highest runtime-reported display rate by default; System and explicit supported rates are available. Quest 3 menu checks observed a 120 Hz display target. This does not guarantee sustained 120 FPS in games. The original simulation retains its native cadence. Bounded mural textures, cached game images and optional adaptive detail control work independently of physical scale. The stereo furniture uses bounded eye buffers; the options retain a separate sharp compositor quad.

**Pixel 1:1 describes game pixels in the virtual canvas.** Headset projection, optics and composition resample that canvas. Extremely large murals have finite stage bounds and finite detail.

## Local files and updates

Version 0.6 keeps runtime files in **private internal app storage**. It attempts a one-time copy of readable `UserData` and `ROMs` from the older external-files location, preserving originals. APK asset copying preserves existing cartridges and saves. Update with the same package/signing key; uninstalling can erase app data. Keep your original PC saves and private cartridge archives as independent backups. Cross-architecture emulator snapshots remain unverified; raw SRAM is supplied where available.

The startup path now reports its original error and safely cleans up partial UI initialization. The immersive manifest and complete OpenXR graphics-state guard prevent the observed startup/window-placement and black-menu failures.

## Build and verification boundary

Build with JDK 21, Android SDK 36, NDK 29.0.14206865 and CMake 3.31.6:

```sh
python tools/prepare_quest.py
cd android
./gradlew --no-daemon --max-workers=2 :app:assembleRelease :app:lintRelease
```

Windows uses `gradlew.bat`. OpenXR loader 1.1.65 and SDL 2.32.10 are pinned. `SCU_QUEST_KEYSTORE` and `SCU_QUEST_KEY_PASSWORD` select your private update key. Keep it outside the repository. `-PscuPrivateAssets=/outside/repo/assets` adds a private `scu/ROMs` and `scu/UserData` tree; ordinary builds omit it.

For an owner-authorized frontend check, start the activity with `--ez scu_menu_only true`. This disables cartridge loading at the core boundary. Optional `--ei scu_preview_mode 4`, `--ez scu_options true`, `--ez scu_capture true` and `--es scu_language ja` select an empty preview/options screen and diagnostic captures. These sessions never establish gameplay qualification. Preview captures use an original pattern and a viewer-aligned empty scene.

The desktop companion adds renderer-neutral 2D cabinet/CRT frames under **Screen → Desktop frame**. Actual stereo furniture and GPU CRT tube shaders are Quest features. macOS remains experimental. The 0.6 companion workflow compiles/packages Windows, Linux and macOS without launching the application.

Design references: [Meta compositor sample](https://github.com/meta-quest/Meta-OpenXR-SDK/tree/main/Samples/XrSamples/XrCompositor_NativeActivity), [Meta OpenXR samples](https://developers.meta.com/vr/documentation/native/native-openxr-sdk-sample/) and the [Khronos OpenXR specification](https://registry.khronos.org/OpenXR/specs/1.0-khr/html/xrspec.html). Their sample code, room models and artwork are not bundled.
