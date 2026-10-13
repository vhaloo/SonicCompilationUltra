<p align="center"><img src="assets/brand.svg" width="760" alt="Sonic Compilation Ultra — your cartridges, a much bigger view"></p>

<p align="center">
  <a href="https://github.com/vhaloo/SonicCompilationUltra/releases/latest">Download</a> ·
  <a href="docs/QUICK_START.md">First steps</a> ·
  <a href="docs/ROMS.md">ROM guide</a> ·
  <a href="docs/FEATURES.md">All options</a>
</p>

**Play your own Mega Drive / Genesis Sonic cartridges with an adaptable PC interface, huge views and backgrounds that look good at any aspect ratio.** No ROMs are included.

Windows, macOS and Linux · English, French, Spanish and Japanese · Keyboard and gamepads

## Start playing

1. **[Download your platform’s package](https://github.com/vhaloo/SonicCompilationUltra/releases/latest)** and extract it completely. On Windows, open `SonicCompilationUltra.exe`; on macOS, open the app; on Linux, run the included launcher.
2. Choose **Choose ROM…** using a mouse or gamepad or drop your own `.bin`, `.md`, `.gen`, `.smd` or `.zip` into the window. The app checks the file’s contents. [Which edition do I need?](docs/ROMS.md)
3. Choose **Play**. Use **Enter / gamepad Start** on the original title screen. Move with the arrows or left stick; jump with **Z, X, C / gamepad A, X, B**.
4. Open the fullscreen options whenever you like: **Esc, F10 or gamepad Select / View**. Use **A / bottom face button** to confirm, **B / right face button** to go back, **LB/RB** to switch sections and **LT/RT** to scroll. Gameplay pauses in the menu.

No installer, administrator access or game-data upload is required. English is the fallback language; a supported machine language is selected automatically. You can change it under **Screen → Language**.

## A small screen or three huge ones

**New installations start in borderless fullscreen with a Whole-level overview.** The current platform stage fits on screen, with a player marker and live close-up. Sonic 3-era scenes can use a clean terrain map; other game projections retain their native image. Fullscreen menus keep their content readable in the centre, even across three monitors.

**Pixel world at 1×** shows one game pixel per physical display pixel. **Balanced** and **Lightweight** lower overview sampling to reduce rendering work; quality presets preserve your chosen view. Resize freely, choose a fullscreen display or span all displays.

Choose a familiar **Panorama**, a very wide **Pixel world**, a **Whole-level overview**, or the **Original game framing**. Follow the player, freeze the view or explore with a free camera. Keep the HUD on the centre monitor, attach it to the player, move it to an edge or hide it.

<img src="docs/screenshots/ultrawide-48x9.png" alt="Real 7680 × 1440 gameplay capture, 48:9 pixel view">

*48:9 · 7680 × 1440 · actual game rendering at pixel 1:1. Open the image to inspect the full resolution.*

<table><tr>
<td width="66%"><img src="docs/screenshots/normal-16x9.png" alt="Real 16:9 gameplay capture"><br><em>16:9 · 1920 × 1080 · stage overview</em></td>
<td width="34%"><img src="docs/screenshots/tiny-640x360.png" alt="Real small-window gameplay capture"><br><em>Small window · 640 × 360</em></td>
</tr></table>

<details><summary>48:9 whole-stage view</summary>
<img src="docs/screenshots/overview-48x9.png" alt="Real 7680 × 1440 whole-stage view with centred HUD and native player inset">
</details>

<img src="docs/screenshots/fullscreen-options.png" alt="Fullscreen options with controller hints and clear stage-view choices">

## The collection

| Platform games and lock-ons | More cartridge games |
|---|---|
| Sonic the Hedgehog, Sonic 2, Sonic 3 | Dr. Robotnik’s Mean Bean Machine |
| Sonic & Knuckles, Sonic 3 & Knuckles | Sonic Spinball, Sonic 3D Blast |
| Knuckles in Sonic 2, Blue Sphere | Sonic Compilation / Sonic Classics |

Import separate cartridges and the app can assemble the standard lock-ons locally. **Attach Sonic & Knuckles** also exposes its original lock-on behaviour with other imported cartridges. Knuckles in Sonic 2 requires the cartridge’s UPMEM chip or the supported combined dump. [Details](docs/ROMS.md#lock-on).

The main platform games have an experimental world renderer. Puzzle games, special stages and native competition retain their original framing. The cartridge engine is **Genesis Plus GX**; this project is an enhanced collection, not a new decompilation of every game.

## Make it yours

Six backgrounds include a softly blurred landscape stretched from the game’s own scenery. Blur and brightness are adjustable; characters and terrain stay sharp. You also get a locator, minimap, scanlines, a camera-frame guide, optional **Sonic 1 spin dash**, damage/air assistance, rewind, nine save slots per game, automatic resume, controller dead zones, three gameplay button layouts, button-label hints, large text, high contrast, safe confirmations and rendering presets. The ROM browser includes an on-screen path keyboard.

**Hold the right shoulder** for quick zoom, overview, freeze/follow, player focus or screenshots. **+ / −** changes zoom, **F** freezes the camera, **R** restores following, **Shift + F1…F9** saves and **F1…F9** loads. The menu’s **Controls** page has the complete guide.

## Before you download

This is a **preview release**. Original framing provides the native cartridge display; expanded views can expose art-bank and scrolling limitations, especially during Sonic 3 transitions. Full-game completion and every GPU/controller combination are not certified. See the [tested coverage](docs/TEST_REPORT.md) and [known limits](docs/FEATURES.md#known-limits).

Use game data you are entitled to use. The executable supplies no ROM, music, BIOS or extracted game-art pack. Gameplay screenshots document the app; game graphics and names remain the property of their rights holders. This independent, noncommercial fan project is not affiliated with SEGA. [Licences and distribution boundaries](docs/LEGAL.md).

For contributors: [build from source](docs/BUILD.md), [Sonic 1 research catalogue](docs/RESEARCH.md), [third-party notices](THIRD_PARTY_NOTICES.md).
