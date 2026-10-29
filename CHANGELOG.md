# Changelog

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
