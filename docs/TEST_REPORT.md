# Validation report — 13 October 2026

This report separates successful checks from coverage that has not been measured. A boot test or source build is not a full-game completion certificate.

## 0.2.2 rewind additions

- Authored 68000/PSG fixtures verify one-frame native restoration, backwards PCM with stereo channels preserved, queue flushing on direction changes, mute/zero volume/silent rewind, exhaustion, branching, load/reset invalidation, option persistence and bounded allocation. Variable-sized compressed delta states round-trip after history eviction. PAL picture/audio timing is also exercised.
- A real SDL virtual gamepad verifies L2/LT hold/release, analogue hysteresis, alternative LB/RT bindings, safe menu return and separation from trigger scrolling. The same game loop restores the expected number of frames over one second at simulated display rates of 30, 60 and 144 Hz.
- Eleven locally supplied cartridge entries each restore 360 consecutive frames: **3,960 exact complete snapshots and native rasters**. Concatenated reversed stereo PCM matches the forward audio in reverse frame order, sample for sample. Source save files remain unchanged. These are sampled sessions, not full playthroughs.
- On this Windows machine, short real-time forward/rewind/resume samples with active expanded rendering reach approximately 59.2 ticks/s at 7680×1440 Balanced, 59.1 at full-resolution Ultra, and 59.5 at 640×360 software/Lightweight. These measurements include rewind recording and restoration; they do not certify every GPU or audio device.
- The disk save container remains SCU2, with SCU1 loading retained. Rewind differences and audio stay in memory and are excluded from releases. Four-language page captures and portable package validation remain part of CI.

## 0.2.1 boss/script additions

- Synthetic fixtures cover event bosses, parent-relative and variable scripts, all ten supported Sonic 3-era child-factory formats, boss art before VRAM loading, Nemesis dictionary/inline/XOR decoding, Kosinski short/extended copies and descriptor-boundary prefetch. Truncated art fails closed.
- Boss/script filtering, remembered source defeat, Sonic 1 boss defeat, small-view markers, arena-inclusive framing and unchanged native snapshots pass. The virtual gamepad changes and restores all five new controls.
- A display-free catalogue audit reads 113 game/act combinations from six locally supplied cartridge profiles, checking bounded coordinates and categories. This includes empty/unused lists; it is static coverage, not 113 completed stages.
- Seven private native-stage entries render successfully: Sonic 1 GHZ3 and Final Zone, Sonic 2 HTZ2 and Death Egg, Sonic 3 MGZ2, Sonic 3 & Knuckles SOZ1, and Sonic & Knuckles Doomsday. These test copies use controlled level restart state; they are not natural route-completion evidence.
- Six equal-input, equal-tick comparisons across those games produce byte-identical complete native snapshots with previews enabled/disabled, including RAM, VRAM, palettes, background and scroll observations.
- Render captures at 640×360, 1920×1080 and 7680×1440 verify the actual composition. Bosses retain priority when labels are decluttered; missing or variable art remains a marker.

Random and player-dependent attacks report supported origins; unknown global destinations are omitted. Full-campaign boss completion and every indirect event path remain unverified. See [precise scope](ENEMY_PREVIEWS.md).

## 0.2.0 additions

- SDL virtual gamepad reaches every setting and all nine pages. Confirm/back, shoulder tabs, trigger scrolling, sliders, choices, ROM import, on-screen path entry, save/load, safe dialogs and controller shortcuts are exercised through the SDL input path.
- Controller menu resizing checks cover 320×240, 640×360, portrait 1080×1920, 1920×1080 and 7680×1440, plus disconnect/reconnect.
- ROM-free rendering checks capture all nine pages in four languages at three sizes: **108 captures**, repeated against each portable package in CI.
- All eleven cartridge entries retain the overview setting, produce their appropriate expanded/native frame and save/load successfully. The six supported platform-game layouts fit within the reported world coverage. Puzzle/3D/special-stage/Compilation projections retain native framing.
- The live player inset preserves the native view when the world is zoomed out. Sonic 3-era overview can use a clean schematic map; it does not claim unloaded art or collision accuracy.
- Enemy-preview tests cover all six platform address profiles, lock-on/mirrored art, spawn coordinates, remembered defeat, waiting/visible native sprites, ROM-backed dynamic art, configurable transparency, tiny-view markers and unchanged guest snapshots. Six locally supplied opening-stage runs produce preview sprites and markers. The virtual gamepad changes the toggle, style and opacity.

These are virtual-controller and sampled game checks, not certification of every physical controller or a full campaign.

## Earlier Windows baseline (0.1.0)

Physical host: Windows, AMD Ryzen 7 5800X, Nvidia GeForce RTX 5070. The three-screen desktop is exposed as one **7680 × 1440** display. Test runs kept application audio muted and did not alter system volume.

| Check | Result |
|---|---|
| Original-source compilation | MSVC Release build succeeds with pinned SDL2 2.32.10 |
| Unit checks | Pass: ROM identities, layout/flip addressing, read-only compositor, 1:1, zoom, overview, portrait, tiny and original-view fallback |
| Original synthetic runtime | Pass: our own 68000 program executes, VDP backdrop reaches the framebuffer, native RAM/time restore, damaged/foreign states reject and rewind restores an earlier frame |
| ROM-free language/UI checks | 12 cases pass: EN/FR/ES/JA × 320×240, 640×360, 1280×720; software rendering and exact captured dimensions |
| Pixel 1:1 | Drawable and world canvas both **7680×1440** |
| Real-time wide run | **59.57 simulation fps**, Direct3D 11, about 15.1 s measured host loop |
| Real-time small/software run | **59.77 simulation fps**, 640×360, Lightweight, about 14.4 s measured host loop |
| Screenshot review | Actual 7680×1440, 1920×1080 Panorama, 640×360, original view and overview captures reviewed |
| Corresponding source | A clean extracted source archive configures and rebuilds on Windows using the included pinned SDL archive; unit checks pass |
| Collection cartridge smoke | All 11 listed entries boot, produce a frame and save/load a state successfully |
| Platform-game entry | Sonic 1, Sonic 2, Sonic 3, S&K, Sonic 3 & Knuckles and Knuckles in Sonic 2 opening-stage entries reached; native movement/jump exercised |
| S3-family scenes | Original title/save/intro and installed level layouts observed; native visible-region composition retained; distant art limits remain |
| Puzzle and pinball | Mean Bean scenario board and Spinball opening playfield reached with native inputs |
| Cartridge lock-on | S1 → Blue Sphere; S3 → S3&K; S2 → Knuckles using either separate UPMEM or a combined dump; Mean Bean lock-on produces its Blue Sphere stage |
| Blue Sphere | Original NO WAY entry advances through native A+B+C input into the special-stage mode |
| State files | Corrupt payload and wrong-cartridge states rejected; charged S1 spin dash survives save/load; previous files preserved as `.bak` |
| Spin dash option | Identical Down/jump inputs: disabled Sonic stays near the start; enabled charge reaches 3, then launches forward and clears on release |
| App-menu input | Injected SDL Select/View and Esc events toggle the menu; native frame stays unchanged while paused and resumes when closed |
| Native Windows interface | Mouse library/options/resume/quit exercised; F10, Esc, Ctrl+4 and F1 verified through the desktop tester after adding zero-scan-code virtual-key support |
| Rewind | Native checkpoint restoration exercised after a save/load |
| Paused visual restore | State format includes the presentation raster; restoring guest RAM/time also restores the native image and backdrop |

Headless test throughput exceeding hundreds of ticks per second is deliberately not presented as display performance. Real-time measurements above count executed native ticks during the measured loop; neutral startup fast-forward is excluded from that rate.

The screenshots are real renderer output from a locally supplied cartridge, not concept art or a mock-up. Documentation captures preserve the source pixels as lossless PNGs. See [screenshot credits](screenshots/README.md).

## Portable build checks

The repository workflow builds Windows x64, Linux x64, macOS Intel and macOS Apple Silicon, runs synthetic addressing, native-runtime and virtual-controller UI tests, validates the four-language UI without commercial ROM data, and prepares corresponding-source packages. It repeats all 108 page captures against the portable launcher or app bundle itself. The workflow result is the authority for each submitted revision; inspect the [current Actions run](https://github.com/vhaloo/SonicCompilationUltra/actions).

Successful CI means compiler, linkage and the tested software-rendering UI paths passed. It does **not** prove physical Mac/Linux controllers, audio devices, Retina rendering or full gameplay on those hosts. Physical integrated-GPU gameplay is also not certified by a software-renderer test on this Windows CPU.

**Both macOS downloads are experimental.** No physical Mac validation has been performed. Windows, Linux and Mac build results are listed separately in Actions; Mac CI success does not change that experimental status.

## Limits of the game tests

The engine executes the original cartridges. These tests exercise boot/menu/gameplay samples, selected native inputs and state round trips. They do not complete every act, boss, ending, character, multiplayer route or special stage. Mean Bean, Spinball and 3D Blast use their native projection. Mean Bean's scenario board, Spinball's playfield and 3D Blast's opening isometric stage were observed. None is a complete campaign playthrough.

The host world renderer is experimental. Far-away geometry/rings are shown without expanding the native enemy simulation. Sonic 3 art banks, streamed edits, fire transitions and unusual scroll modes can require Original framing. [Feature boundaries](FEATURES.md#known-limits) remain part of the release description.
