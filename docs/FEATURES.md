# Views and options

The app separates cartridge simulation from presentation. Resizing, panning, blur, HUD placement and ordinary zoom do not change the game's collision data or its native clock. Gameplay assistance is opt-in.

## Screen

| Option | Behaviour |
|---|---|
| Pixel world | Exact physical 1:1 with magnification 1× / divisor 1×; magnification 1–8 and divisor 1/2/4/8 |
| Exact pixel 1:1 | One action restores Pixel world, magnification 1× and divisor 1× |
| Panorama | Adjustable 224–4096 world-pixel height; horizontal coverage follows the window aspect |
| Whole-level overview (default) | Fit the entire installed current-stage foreground into a bounded image, with a live native player inset |
| Overview terrain | Automatic clean map for Sonic 3-era streamed layouts; original artwork or schematic map can be selected for any supported platform stage |
| Original framing | Preserve the cartridge's native framebuffer and projection |
| Full Spinball / 3D Blast terrain | Decode complete cartridge maps beyond the native camera; disable to use native framing |
| Smooth reduced maps | Average terrain samples and interpolate reduced views to limit shimmer; exact 1:1 and magnified pixels stay sharp |
| Fullscreen / span | Borderless window over one display or the combined display rectangle |
| Native-screen scale | Independent scale for title screens, special stages and puzzle games |
| Options layout / text | Fullscreen by default; compact overlay, automatic/small/large/extra-large text and high contrast |
| Fullscreen display | Current display, a selected display or an optional span across all displays |
| Performance presets | Preserve the chosen view; overview sampling is 1/3 in Lightweight, 1/2 in Balanced and full output in Ultra |
| Rendering workers | Automatic or 1–16, where parallel rendering is available |
| Software rendering | CPU renderer fallback; restart the application to change backend |
| Vertical sync | Presentation synchronisation; simulation retains native cartridge timing |
| Scanlines | Adjustable strength; foreground remains nearest-sampled |
| Performance overlay | Separate cartridge ticks/s, presented frames/s and drawable dimensions |

Pixel 1:1 uses physical framebuffer dimensions, including on Retina displays. Extremely large output requests are bounded by texture and memory limits; there is no fixed aspect-ratio ceiling. Overview samples a large world instead of allocating a stage-sized, aspect-padded framebuffer.

The render-size cap preserves the requested world coverage and aspect ratio. Native-only games also respect Pixel world magnification rather than silently filling a larger screen. A native image that cannot fit a tiny window is reduced to fit.

## Illustrated settings

Every option has an explanation and an original geometric illustration, localized into English, French, Spanish and Japanese. On larger screens, focus or hover an option to update the side panel. Press **Y / Triangle / top face button** or **F1 in the menu** to open a scrollable detail guide at any size. **B / Esc / Select** closes that guide first. The help button is also reachable with directional navigation. The guide’s illustrations are diagrams, not fabricated gameplay captures.

## Corner reminder and status

The default options reminder is a single compact line in the bottom-right corner, with 14-pixel text and an 8-pixel margin. It uses the outside screen edge on ultrawide displays. It is clickable and does not reserve a footer or shrink native gameplay. The native player close-up moves clear of overlapping cards.

Find these controls under **Camera & HUD**. Every control supports directional gamepad navigation and its illustrated guide.

| Control | Choices |
|---|---|
| Show compact options reminder | Show or hide; Select/View, Esc and F10 always open options |
| Reminder corner | Bottom right (default), bottom left, top right or top left |
| Reminder content | Both menu shortcuts (default), current input device, or details with rewind bindings/history |
| Reminder size | 60–180%, independent of menu text and game HUD size |
| Overlay background opacity | 20–100%; text remains opaque, high contrast uses a nearly solid backing |
| Overlay edge spacing | 0–64 window pixels, bounded on tiny windows |
| Hide reminder after | Always visible (0, default), or 1–30 seconds; returning from the menu restarts the timer |
| Notification duration | 6 seconds by default, 1–30 seconds, or until dismissed (0) |
| Show status while rewinding | Independent active rewind status with remaining history |

Performance counters and notifications work when the reminder is hidden. An expired notification remains readable and dismissible in the menu; clicking a visible notification opens the menu. Very long notification overlays are bounded to the screen, with full text on mouse hover or in the menu. Larger text and crowded tiny layouts wrap or clip inside their allotted cards instead of extending off-screen. The input-device content follows keyboard/controller use and the selected controller label style.

## Map coverage

| Cartridge / scene | Extended view |
|---|---|
| Sonic 1/2/3, S&K and standard platform lock-ons | Current installed act layout, native actors and supported spawn previews; Sonic 3 can use the schematic terrain style |
| Sonic Spinball | Four complete 2048-pixel-tall boards, 1280/1536/1472/1792 pixels wide; both tile banks, native camera/player tracking and authentic score strip |
| Sonic 3D Blast | All 22 cartridge terrain maps, each 8192×4096, in the original isometric projection; native rings/lives and live player tracking |
| Sonic Classics / Compilation | Installed Sonic 1/2 RAM layouts and hardware sprites; menus and Mean Bean retain native framing; separate-cartridge spawn prediction is not applied to this banked edition |
| Mean Bean Machine | Complete native puzzle screen, adaptive fitting and physical Pixel world scaling |
| Blue Sphere, special stages, bonus scenes, competition and titles | Original projection and complete native screen; no invented planar terrain |

Spinball and 3D Blast cache palette indices for the entire current terrain and update affected cells when native VRAM animation changes. Palettes remain live. One atlas is retained at a time: about 3–5 MB for Spinball and 37 MB for 3D Blast, plus the output canvas and normal emulation/history memory. Turning off their map extension releases its cache. The final Spinball board’s shorter background follows an anchored version of the cartridge’s vertical parallax curve; the native window preserves its exact local composition.

These maps expose the **terrain**, not an expanded enemy simulation or every moving object in the whole world. The exact native camera patch and optional inset preserve local moving terrain, depth effects, score and animations. Distant scripted changes or native moving objects can remain absent outside the active area. Spinball/3D Blast enemy and boss forecasting is not implemented.

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
- Frame-by-frame 1x rewind with backwards stereo music/effects; hold Backspace or L2 / Xbox LT by default. Enabled on new installs, with gamepad button choice, reverse-sound toggle and compressed 24 / 96 / 192 MB history budgets (Lightweight caps at 24 MB). Duration depends on game data. In-memory history clears on loading, restarting or switching games. Release continues from the restored moment and discards the undone future.
- Game volume, mute and pause on loss of focus. Opening the app menu pauses simulation and clears queued audio.
- Two controllers or keyboard players, analogue dead-zone adjustment and hot-plug support.
- English, French, Spanish and Japanese; automatic machine language with English fallback.
- Spawn previews enabled by default in expanded platform views: transparent original sprites, amber enemy markers, larger red boss markers and purple script-origin markers; adjustable 10–80% opacity. Independent boss/script switches, optional tint and labels, and overview framing that includes distant spawns. Every control works with a gamepad. [Details and coverage](ENEMY_PREVIEWS.md).

## Controller-first menus

All nine sections and their settings support D-pad/left-stick navigation, bottom-face confirm and right-face back. Shoulder buttons change sections; triggers scroll long pages and file lists. Start resumes a loaded game. The focus outline is always visible when opening the menu or switching sections. Back closes an active edit or dialog before returning to play. The menu pauses native simulation and suppresses held confirm buttons until released.

The ROM browser has parent-folder navigation, a drives/locations chooser, a filtered file list and an on-screen path keyboard. Folder import and the ROM guide remain reachable on tiny screens. Library filtering can show only imported games. Missing cartridges leave the menu available to re-import.

Controller hints support automatic/Xbox/PlayStation/Nintendo labels. Gameplay offers three face-button layouts; the menu keeps consistent confirm/back positions. The stick dead zone applies to gameplay and menu navigation. Either connected pad can take over menu navigation by pressing a button; the first two pads retain their native player assignments.

Optional gameplay shortcuts use the right shoulder as a modifier: Up/Down zooms, the top button toggles overview, the left button freezes/follows, the right button focuses the player, and Start captures a screenshot. Modified inputs are suppressed from the cartridge. Rewind uses L2 / LT by default, independently of these shortcuts. Trigger scrolling stays active only in menus.

Save slots show occupied/empty status and dates. Empty-slot loading is disabled. Overwrites, loads, game restarts, game switches and default resets ask for confirmation, with Cancel selected first. Confirmations and quit confirmation are independently configurable. Restoring defaults preserves cartridge paths, saved files, language and mute preference. Updating does not automatically erase earlier preferences or save files.

## Known limits

The expanded compositor is experimental. It reconstructs **terrain, static rings and spawn previews**, while actual enemies and dynamic objects follow the cartridge's original rules. Fixed placements and supported creation scripts supply previews; variable attacks mark their source. Scripted objects can include boss parts and scenery. Conditional routes show possible appearances. Unsupported runtime calculations, random outcomes and full trajectories cannot be forecast exactly. Original object-pool pressure and gameplay timing are preserved. See [supported patterns and limits](ENEMY_PREVIEWS.md).

Only the current act's installed layout is shown. Connected acts, interiors or art banks that have not been loaded are not an atlas of the whole campaign. Sonic 3's streamed edits, burning scenery, palette changes and unusual scrolling can make distant scenery imperfect. The native visible region is retained where possible; Original framing is always available.

Special-stage projections and native split-screen are not flattened into a world map. Mean Bean, Blue Sphere and unsupported scenes use their native view. Spinball and 3D Blast support the terrain atlases described above; native framing remains available for all games.

Displays with different densities/heights and physical bezels cannot form a mathematically perfect single physical surface. The rectangular desktop canvas adapts, but no bezel correction is performed. Low-end performance depends on CPU, drivers, display size and settings; Lightweight and software rendering provide alternatives, not a guarantee for every machine ever made.

This release does not claim full-game/boss-route certification, verified physical macOS/Linux gameplay, network play, Sonic CD disc support, every ROM hack, new levels or new character artwork.
