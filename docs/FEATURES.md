# Views and options

The app separates cartridge simulation from presentation. Resizing, panning, blur, HUD placement and ordinary zoom do not change the game's collision data or its native clock. Gameplay assistance is opt-in.

## Screen

| Option | Behaviour |
|---|---|
| Pixel world | One game pixel per physical display pixel at 1×; integer zoom 1–8 |
| Panorama | Original 224-pixel world height, adapted to the window's aspect |
| Whole-level overview (default) | Fit the entire installed current-stage foreground into a bounded image, with a live native player inset |
| Overview terrain | Automatic clean map for Sonic 3-era streamed layouts; original artwork or schematic map can be selected for any supported platform stage |
| Original framing | Preserve the cartridge's native framebuffer and projection |
| Fullscreen / span | Borderless window over one display or the combined display rectangle |
| Native-screen scale | Independent scale for title screens, special stages and puzzle games |
| Options layout / text | Fullscreen by default; compact overlay, automatic/small/large/extra-large text and high contrast |
| Fullscreen display | Current display, a selected display or an optional span across all displays |
| Performance presets | Preserve the chosen view; overview sampling is 1/3 in Lightweight, 1/2 in Balanced and full output in Ultra |
| Rendering workers | Automatic or 1–16, where parallel rendering is available |
| Software rendering | CPU renderer fallback; restart the application to change backend |
| Vertical sync | Presentation synchronisation; simulation retains native cartridge timing |
| Scanlines | Adjustable strength; foreground remains nearest-sampled |
| Performance overlay | Live simulation frame rate and drawable dimensions |

Pixel 1:1 uses physical framebuffer dimensions, including on Retina displays. Extremely large output requests are bounded by texture and memory limits; there is no fixed aspect-ratio ceiling. Overview samples a large world instead of allocating a stage-sized, aspect-padded framebuffer.

## Camera and HUD

Follow inside the level, centre on the player, freeze, pan freely, or centre the whole level. Free-camera controls are independent of player movement. **Show whole stage** and **Focus on player** provide one-action view changes. Choosing an integer zoom or a follow/free camera switches out of the fixed overview, so the option takes effect immediately. The optional live player inset in overview shows the authoritative native camera. The clean terrain map is a schematic of loaded tile geometry, not a collision map or reconstruction of unloaded artwork. The locator remains useful when the player leaves the view.

HUD choices: centre monitor / safe 16:9 frame, near player, screen edge, hidden. Choose automatic or manual size. A minimap and a guide to the original camera are available. Original-framing scenes retain their native HUD.

Hotkeys: **Ctrl + 1/2/3** selects pixel zoom 1/2/3, **Ctrl + 4** selects Panorama, **Ctrl + 5** selects Overview. **H** cycles HUD position; **M** toggles the minimap.

## Background

1. Soft stretched landscape — the native background without characters or foreground, blurred and stretched uniformly.
2. Sharp stretched landscape.
3. Original background tiled with its live parallax.
4. Gradient sampled from the game's palette.
5. Quiet midnight.
6. Black.

Adjust blur and brightness. Freeze the background independently, reduce its motion, or let it follow the game. The original foreground and characters stay sharp. In original-framing scenes, a softened copy of the native composition supplies the surrounding backdrop.

## Gameplay, sound and progress

- Optional Sonic 1 spin dash: hold Down, tap jump to charge, release Down. This is a host-added ability using the original rolling physics/art, not a claim of bit-identical Sonic 2 spin-dash behaviour.
- Optional Sonic 1 damage protection and unlimited air. Pits/crushing can still be dangerous.
- Original S&K lock-on, including separate save identities.
- Optional removal of the hardware sprite-per-line limit.
- Optional startup-logo skip, nine state slots, automatic resume and cartridge SRAM.
- Optional bounded rewind history; hold Backspace or the left shoulder.
- Game volume, mute and pause on loss of focus. Opening the app menu pauses simulation and clears queued audio.
- Two controllers or keyboard players, analogue dead-zone adjustment and hot-plug support.
- English, French, Spanish and Japanese; automatic machine language with English fallback.

## Controller-first menus

All nine sections and their settings support D-pad/left-stick navigation, bottom-face confirm and right-face back. Shoulder buttons change sections; triggers scroll long pages and file lists. Start resumes a loaded game. The focus outline is always visible when opening the menu or switching sections. Back closes an active edit or dialog before returning to play. The menu pauses native simulation and suppresses held confirm buttons until released.

The ROM browser has parent-folder navigation, a drives/locations chooser, a filtered file list and an on-screen path keyboard. Folder import and the ROM guide remain reachable on tiny screens. Library filtering can show only imported games. Missing cartridges leave the menu available to re-import.

Controller hints support automatic/Xbox/PlayStation/Nintendo labels. Gameplay offers three face-button layouts; the menu keeps consistent confirm/back positions. The stick dead zone applies to gameplay and menu navigation. Either connected pad can take over menu navigation by pressing a button; the first two pads retain their native player assignments.

Optional gameplay shortcuts use the right shoulder as a modifier: Up/Down zooms, the top button toggles overview, the left button freezes/follows, the right button focuses the player, and Start captures a screenshot. Modified inputs are suppressed from the cartridge. The left shoulder remains rewind when enabled.

Save slots show occupied/empty status and dates. Empty-slot loading is disabled. Overwrites, loads, game restarts, game switches and default resets ask for confirmation, with Cancel selected first. Confirmations and quit confirmation are independently configurable. Restoring defaults preserves cartridge paths, saved files, language and mute preference. Updating does not automatically erase earlier preferences or save files.

## Known limits

The expanded compositor is experimental. It reconstructs **terrain and static rings**, while enemies and other dynamic objects follow the cartridge's original spawn/culling rules. A distant enemy is not simulated merely because the large view can show its terrain. This avoids changing object-pool pressure and gameplay timing.

Only the current act's installed layout is shown. Connected acts, interiors or art banks that have not been loaded are not an atlas of the whole campaign. Sonic 3's streamed edits, burning scenery, palette changes and unusual scrolling can make distant scenery imperfect. The native visible region is retained where possible; Original framing is always available.

Special-stage projections and native split-screen are not flattened into a world map. Mean Bean, Spinball, 3D Blast, Blue Sphere and the original Compilation cartridge use the native view. The original UI inside those scenes is retained.

Displays with different densities/heights and physical bezels cannot form a mathematically perfect single physical surface. The rectangular desktop canvas adapts, but no bezel correction is performed. Low-end performance depends on CPU, drivers, display size and settings; Lightweight and software rendering provide alternatives, not a guarantee for every machine ever made.

This release does not claim full-game/boss-route certification, verified physical macOS/Linux gameplay, network play, Sonic CD disc support, every ROM hack, new levels or new character artwork.
