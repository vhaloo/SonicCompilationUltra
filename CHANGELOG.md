# Changelog

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
