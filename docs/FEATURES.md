# Views and options

The app separates cartridge simulation from presentation. Resizing, panning, blur, HUD placement and ordinary zoom do not change the game's collision data or its native clock. Gameplay assistance is opt-in.

## Screen

| Option | Behaviour |
|---|---|
| Pixel world | One game pixel per physical display pixel at 1×; integer zoom 1–8 |
| Panorama | Original 224-pixel world height, adapted to the window's aspect |
| Whole-level overview | Sample the entire installed foreground layout into a bounded output image |
| Original framing | Preserve the cartridge's native framebuffer and projection |
| Fullscreen / span | Borderless window over one display or the combined display rectangle |
| Native-screen scale | Independent scale for title screens, special stages and puzzle games |
| Menu text size | Automatic, smaller, larger or largest |
| Rendering workers | Automatic or 1–16, where parallel rendering is available |
| Software rendering | CPU renderer fallback; restart the application to change backend |
| Vertical sync | Presentation synchronisation; simulation retains native cartridge timing |
| Scanlines | Adjustable strength; foreground remains nearest-sampled |
| Performance overlay | Live simulation frame rate and drawable dimensions |

Pixel 1:1 uses physical framebuffer dimensions, including on Retina displays. Extremely large output requests are bounded by texture and memory limits; there is no fixed aspect-ratio ceiling. Overview samples a large world instead of allocating a stage-sized, aspect-padded framebuffer.

## Camera and HUD

Follow inside the level, centre on the player, freeze, pan freely, or centre the whole level. Free-camera controls are independent of player movement. The locator remains useful when the player leaves the view.

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

## Known limits

The expanded compositor is experimental. It reconstructs **terrain and static rings**, while enemies and other dynamic objects follow the cartridge's original spawn/culling rules. A distant enemy is not simulated merely because the large view can show its terrain. This avoids changing object-pool pressure and gameplay timing.

Only the current act's installed layout is shown. Connected acts, interiors or art banks that have not been loaded are not an atlas of the whole campaign. Sonic 3's streamed edits, burning scenery, palette changes and unusual scrolling can make distant scenery imperfect. The native visible region is retained where possible; Original framing is always available.

Special-stage projections and native split-screen are not flattened into a world map. Mean Bean, Spinball, 3D Blast, Blue Sphere and the original Compilation cartridge use the native view. The original UI inside those scenes is retained.

Displays with different densities/heights and physical bezels cannot form a mathematically perfect single physical surface. The rectangular desktop canvas adapts, but no bezel correction is performed. Low-end performance depends on CPU, drivers, display size and settings; Lightweight and software rendering provide alternatives, not a guarantee for every machine ever made.

This release does not claim full-game/boss-route certification, verified physical macOS/Linux gameplay, network play, Sonic CD disc support, every ROM hack, new levels or new character artwork.
