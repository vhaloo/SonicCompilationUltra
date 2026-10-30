# Changelog

## 0.8.0 Mobile / XR development preview — 2026-10-29

- Separate non-XR Android phone/tablet application, Android 7.0+, ARM64/ARMv7/x86-64, GLES 2.0 and 16 KB page alignment for 64-bit libraries.
- Adaptive portrait deck / landscape overlays, safe display/gesture insets, density-aware menus, native multitouch joystick and buttons, rewind, Spinball flippers and gamepad auto-hide.
- Touch navigation dock, four-language illustrated mobile settings, handedness, control-group offsets, opacity/size/dead zone, orientation, immersive display, refresh preferences and panorama memory budgets.
- Modern Android Back callback and scoped USB receiver integration; atomic bundled-file seeding preserves existing app data and recovers interrupted first-run copies.
- 864 pure layout combinations plus input ownership/release checks. All release builds use compile/lint/static validation only; no application, cartridge or headset execution. macOS remains experimental.

## 0.7.0 XR development preview — 2026-10-29

- 90 Hz default with one-time backed-up migration; transparent AR scenery by default.
- Movable 3D remote with seven pages, nine direct view presets, large ray/poke/pinch buttons, every advanced setting section and fine placement buttons.
- One middle-finger grip moves/rotates stage, native CRT, options or remote; two grips resize the stage. Optional hand tracking and automatic large virtual game controller.
- Independent native CRT, three original enclosure styles, and flat coplanar TV/world composition with a native-viewport cutout.
- Shared fixed transparent previews for 152 named scenery types; other placed origins can use markers. Spinball trigger flippers and distinct rewind bindings/hints.
- Smaller stereo/terrain budgets, direct stereo canvas sampling, reusable image uploads, retained priority buffer, cached geometry/text and bounded preview caches.
- Compile/lint, pure helper and static package checks only. No 0.7 Quest or application launch; hardware and gameplay qualification deferred. macOS remains experimental.


## 0.6.0 XR preview — Stereo furniture and startup repair

- Repair partial UI shutdown, use app-owned internal storage with external-data migration, identify the app as immersive and preserve SDL graphics state around the full OpenXR frame sequence.
- Add stereo 3D pinball cabinets, automatic complete-board Spinball presentation, curved CRT enclosures/shaders, aligned surrounding terrain, a repeated 360° cartridge backdrop and curved stage galleries.
- Separate resizing from placement locking. Add two-grip/stick resizing, 5–20,000% scale, half/double/reset, cabinet dimensions/slope/glass, CRT presets/mask/curvature and VR surroundings.
- Add portable desktop cabinet/CRT frames and illustrated guides in four languages. Expand the renamed OFL font subset and load every translated glyph.
- Verify Quest 3 library/options and original-pattern stereo previews with cartridge execution disabled at the core boundary; statically check all 12 personal cartridge entries. No gameplay was launched or tested.
- Publish ROM-free software/source only. Personal cartridges, saves, signing keys and APK remain local. macOS stays experimental; desktop companion CI compiles/packages without running games.

## 0.5.0 Quest preview — Fixed wall AR and maximum refresh preference

- Add scanned-wall fit, a whole-stage mural rising two metres from the floor, and a horizontal floor atlas. Stage bounds stay fixed while the player and your head move; the mural can extend past the physical wall without stretching game geometry.
- Load wall/floor planes with optional spatial permission. Add Meta room capture, wall selection, manual virtual-wall placement, floor calibration and on-device anchor save/restore with failure recovery.
- Keep a separate nearby options screen, placement locking and a right-stick menu recovery path. Tracking loss hides an attached wall and pauses by default. Cinema, tabletop and head-following screens remain available.
- Request the highest runtime-reported headset refresh by default, with System and explicit supported-rate options. Reuse unchanged game images, bound mural textures and optionally adapt their render budget under CPU pressure. Original game speed is unchanged.
- Add original wall diagrams and four-language setting guides. Compilation/lint/static inspection only; no game or headset session is run, and sustained Quest 3 performance remains unverified.
- Accept Windows CRLF and UTF-8 BOM when reading copied library/settings files on Android or Linux; personal asset packages use UTF-8 LF so ZIP member names stay exact. The private preset also applies once on an existing Quest installation while preserving a settings backup.

## 0.4.0 Quest preview — Standalone VR and passthrough AR

- Add an Android ARM64 OpenXR edition that runs on Meta Quest without PC streaming. Play on a room-pinned cinema, tilted tabletop or head-following screen; optionally show the room through runtime passthrough.
- Add Quest Touch gameplay, pointing and clicking, menu navigation, recentering, two-hand screen movement/scaling and left-trigger rewind with reverse sound. Bluetooth gamepads remain available.
- Add spatial-screen size, distance, height, tilt, resolution, input, vibration and orientation preferences with illustrated English, French, Spanish and Japanese guides.
- Import personally supplied cartridges through Android's document picker. The public APK contains no ROMs, saves or signing keys.
- Validation is compilation, lint and static APK inspection only. No game or headset session was executed. Tracking, visuals, input, passthrough, comfort and performance remain unverified; this is an experimental sideload preview.

## 0.3.1 — Compact corner controls

- Replace the large bottom menu bar with a small, clickable bottom-right options badge. Its default 14-pixel text stays small independently of menu scaling, including 48:9 displays.
- Add eight controller-accessible preferences: corner, compact/device/detailed content, size, backing opacity, edge spacing, reminder timeout, notification duration and a separate active-rewind indicator. All have illustrated explanations in four languages.
- Remove the old gameplay footer reservation. Original framing uses the full available height; the player close-up moves out of the corner cards’ way.
- Keep performance counters, rewind status and notifications independent of the menu reminder. Repeated notifications restart their timer; expired messages remain readable in the menu. Select/View, Esc and F10 work with the badge hidden.
- Add bounded corner, timer, input, persistence, crowded tiny-window and native-framing checks, plus a fifth illustrated-guide topic in the cross-platform UI smoke tests.

## 0.3.0 — Full terrain atlases and illustrated settings

- Complete cartridge terrain for all four Spinball boards and all 22 Sonic 3D Blast maps, including the original isometric projection and the final Spinball board’s shorter parallax layer. Native actors, counters and camera tracking stay live.
- Sonic 1/2 installed layouts inside Sonic Classics / Compilation also use the extended renderer. Puzzle boards and special projections retain their native screen with correct Pixel world scaling.
- Every setting has a four-language illustrated explanation that follows mouse or controller focus. Y/Triangle or F1 opens a scrollable detail guide at every screen size; B/Esc closes it safely.
- Zoom-out divisors down to 1/8, one-action exact 1:1, adjustable 224–4096-pixel Panorama height, and optional smoothing for reduced terrain. Output limits preserve world coverage.
- Cached full terrain with affected-cell animation updates, dirty VRAM pattern decoding, less background/upload work, and separate displayed FPS/native tick counters. Lightweight disables map smoothing; existing view choices are kept.
- New bounded codec, map, animation, localization, coverage and controller-guide checks. Disk save format, cartridge activation rules and local-only game-data boundaries are preserved. macOS remains experimental.

## 0.2.2 — L2 / LT rewind with reverse sound

- Hold L2 / Xbox LT or Backspace for frame-by-frame rewind at normal cartridge speed; music and effects play backwards with stereo channels preserved. Release resumes from the restored moment.
- Forward and reverse simulation share the same fixed cadence, independent of display refresh. Direction changes flush stale sound once; exhausted history holds still.
- Compressed in-memory history with 24 / 96 / 192 MB budgets; Lightweight caps history at 24 MB. Rewind and reverse sound are enabled on new installs; existing settings are retained.
- Controller-friendly button choice and reverse-sound toggle, four-language help, and an on-screen hint showing remaining history. Trigger scrolling remains available in options.
- Tests cover exact frame restoration, reverse PCM order, sound queuing/mute, branching after rewind, bounded memory, SDL trigger input and display-rate-independent pacing. Disk save format is unchanged.
- Intel and Apple Silicon Mac downloads are explicitly marked experimental; physical Mac testing remains outstanding. Linux keeps its runtime and portable-launcher checks.

## 0.2.1 — Bosses and scripted spawn previews

- Boss placements and supported event spawns are visible before native activation, with larger red markers and optional labels.
- Script-created objects and projectiles have purple previews; uncertain attacks show labelled origins instead of an invented trajectory or sprite.
- Parent-relative and nested spawn descriptions follow their source. Remembered defeated sources hide future attacks; Sonic 1/2 boss defeat and Death Egg's successive fights are handled separately.
- Five gamepad-accessible controls: bosses, scripted spawns, ghost tint, labels and overview framing that includes distant spawns. English, French, Spanish and Japanese.
- Cartridge-backed Nemesis/Kosinski boss art, bounded read-only instruction traversal, and fixes for locally unsorted object lists and Sonic 2 Crawl badniks.
- Additional synthetic decoding/compositor/input tests and a display-free spawn-audit utility. No ROM or extracted artwork added to the release.

## 0.2.0 — Fullscreen and gamepad preview

- Fullscreen options by default, readable centred content, responsive small-screen section chooser, adaptive text and high contrast.

- Clean schematic overview for streamed Sonic 3-era terrain, with original artwork available.

- Whole current-stage overview on new installs, native framing for other projections, optional live player inset and lower-cost overview quality presets.

- Complete gamepad menus: focus, shoulder tabs, trigger scrolling, consistent confirm/back, nested dialogs and release protection.

- Controller ROM browsing, drive/location selection and on-screen path keyboard; folder import remains available on tiny screens.

- Button-label hints, three gameplay button layouts, menu/gameplay dead zones and optional camera/screenshot shortcuts.

- Dated save-slot status and safe overwrite/load/reset/switch/default confirmations.

- ROM-free SDL virtual-controller UI tests, alongside native-core tests and four-platform packaging checks.

## 0.1.0 — Preview

- Local cartridge importer, collection library and standard S&K lock-on recipes.

- Portable Genesis Plus GX runtime with separate host world presentation.

- Physical pixel 1:1, integer zoom, panorama, sampled overview and original framing.

- Camera/HUD choices, six backgrounds, blur/brightness and performance presets.

- English, French, Spanish and Japanese UI; keyboard and gamepad menu navigation.

- Optional Sonic 1 spin dash, assistance, rewind, nine states and auto-resume.

- ROM-free source/binary packaging, corresponding source, licensed fonts and notices.

Expanded views are experimental. See the validation report and known limits.
