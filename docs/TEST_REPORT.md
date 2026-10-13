# Validation report — 13 October 2026

This report separates successful checks from coverage that has not been measured. A boot test or source build is not a full-game completion certificate.

## Windows checks

Physical host: Windows, AMD Ryzen 7 5800X, Nvidia GeForce RTX 5070. The three-screen desktop is exposed as one **7680 × 1440** display. Test runs kept application audio muted and did not alter system volume.

| Check | Result |
|---|---|
| Original-source compilation | MSVC Release build succeeds with pinned SDL2 2.32.10 |
| Unit checks | Pass: ROM identities, layout/flip addressing, read-only compositor, 1:1, zoom, overview, portrait, tiny and original-view fallback |
| ROM-free language/UI checks | 12 cases pass: EN/FR/ES/JA × 320×240, 640×360, 1280×720; software rendering and exact captured dimensions |
| Pixel 1:1 | Drawable and world canvas both **7680×1440** |
| Real-time wide run | **59.57 simulation fps**, Direct3D 11, about 15.1 s measured host loop |
| Real-time small/software run | **59.77 simulation fps**, 640×360, Lightweight, about 14.4 s measured host loop |
| Screenshot review | Actual 7680×1440, 1920×1080 Panorama, 640×360, original view and overview captures reviewed |
| Collection cartridge smoke | All 11 listed entries boot, produce a frame and save/load a state successfully |
| Platform-game entry | Sonic 1, Sonic 2, Sonic 3, S&K, Sonic 3 & Knuckles and Knuckles in Sonic 2 opening-stage entries reached; native movement/jump exercised |
| S3-family scenes | Original title/save/intro and installed level layouts observed; native visible-region composition retained; distant art limits remain |
| Puzzle and pinball | Mean Bean scenario board and Spinball opening playfield reached with native inputs |
| Cartridge lock-on | S1 → Blue Sphere; S3 → S3&K; S2 → Knuckles using either separate UPMEM or a combined dump; Mean Bean lock-on produces its Blue Sphere stage |
| Blue Sphere | Original NO WAY entry advances through native A+B+C input into the special-stage mode |
| State files | Corrupt payload and wrong-cartridge states rejected; charged S1 spin dash survives save/load; previous files preserved as `.bak` |
| Spin dash option | Identical Down/jump inputs: disabled Sonic stays near the start; enabled charge reaches 3, then launches forward and clears on release |
| App-menu input | Injected SDL Select/View and Esc events toggle the menu; native frame stays unchanged while paused and resumes when closed |
| Rewind | Native checkpoint restoration exercised after a save/load |

Headless test throughput exceeding hundreds of ticks per second is deliberately not presented as display performance. Real-time measurements above count executed native ticks during the measured loop; neutral startup fast-forward is excluded from that rate.

The screenshots are real renderer output from a locally supplied cartridge, not concept art or a mock-up. Documentation captures preserve the source pixels as lossless PNGs. See [screenshot credits](screenshots/README.md).

## Portable build checks

The repository workflow builds Windows x64, Linux x64, macOS Intel and macOS Apple Silicon, runs synthetic tests, validates the four-language UI without ROM data, and prepares corresponding-source packages. The workflow result is the authority for each submitted revision; inspect the [current Actions run](https://github.com/vhaloo/SonicCompilationUltra/actions).

Successful CI means compiler, linkage and the tested software-rendering UI paths passed. It does **not** prove physical Mac/Linux controllers, audio devices, Retina rendering or full gameplay on those hosts. Physical integrated-GPU gameplay is also not certified by a software-renderer test on this Windows CPU.

## Limits of the game tests

The engine executes the original cartridges. These tests exercise boot/menu/gameplay samples, selected native inputs and state round trips. They do not complete every act, boss, ending, character, multiplayer route or special stage. Mean Bean, Spinball and 3D Blast use their native projection. Mean Bean's scenario board and Spinball's playfield were observed; 3D Blast's title/options and story were exercised. None is a complete campaign playthrough.

The host world renderer is experimental. Far-away geometry/rings are shown without expanding the native enemy simulation. Sonic 3 art banks, streamed edits, fire transitions and unusual scroll modes can require Original framing. [Feature boundaries](FEATURES.md#known-limits) remain part of the release description.
