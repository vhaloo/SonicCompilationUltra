# Fluidity and automatic quality

**0.10 aims for the highest picture quality that leaves a useful performance reserve.** Ordinary screens target 60 fps by default. Quest retains 90 Hz by default for stereo presentation and tracking. Console timing, level coverage, rewind audio and spawn previews remain independent of display quality.

Open **Video → Fluidity and quality**, or the Quest options:

| Preference | Time kept in reserve | Best for |
| --- | --- | --- |
| Prefer fluidity | 35% | Extra room for busy scenes, tracking and system work |
| Balanced | 25% | The default balance between detail and reserve |
| Prefer detail | 15% | More detail on a capable device |

At 90 Hz, a 25% reserve leaves about **8.3 ms** of each 11.1 ms frame for app work. This is a budget, not a device guarantee. Watches start with 30% reserve. The readout shows measured CPU work, available GPU timings, estimated margin, scene resolution and effect detail.

## What adjusts

- **CPU scene resolution and stereo resolution adjust independently.** CPU terrain load does not automatically blur the eye buffers, and GPU load does not unnecessarily reduce CPU terrain detail. Automatic overview starts from the full selected pixel budget, rather than permanently halving each dimension.
- **Optional effects reduce first:** CRT mask, glass, floor decoration and, at the lowest detail, map filtering. Original texture colors and native game timing are preserved. Disable **Adjust effects automatically** to retain selected effects.
- **Short spikes count.** A decaying peak and a bounded 95th-percentile history complement average work measurements. Detail returns only after sustained spare time; upward probes remain small. Intentional swap/frame waits and long inactivity are excluded where they can be separated from work. Active options rendering also participates.
- **Android heat limits detail** using local thermal status. No new permission or system clock override is needed. The operating system controls actual refresh, clocks and throttling.
- **The menu and native source image stay separate from scene downsampling.** A watch lens deliberately transforms its game canvas; its controls and options remain flat.

**Advanced performance limits** contains minimum/maximum sampling (35–100% by default, adjustable within 25–100%), thermal adaptation and **literal 1:1 Pixel lock**. Pixel lock preserves source sampling in Pixel view and can restrict automatic gains. Existing quality choices bound scene pixels and memory. Manual mode retains full selected resolution/effects. No setting changes PAL/NTSC simulation speed.

## Quest VR/AR

**Stereo shader quality** offers Auto, Efficient and Reference. Auto begins efficient and upgrades only after measured GPU room persists. The efficient CRT retains texture color, gamma and edge shading, with cheaper scanline/highlight calculations. Both shaders are compiled in advance; switching does not compile a new program.

**Peripheral rendering** optionally uses the runtime's OpenXR foveation profiles. Auto follows effect detail; Off/Light/Balanced/Strong are available. The center stays detailed while peripheral fragment work can be reduced. This is fixed peripheral rendering, not eye tracking. Profiles are cached and only changed when needed. Missing extensions/functions or a rejected foveated swapchain fall back to ordinary rendering. Sharp compositor menu quads are excluded.

Eye swapchains/depth storage stay allocated while viewport size changes. Opaque geometry avoids blending. Framebuffer completeness is checked once per swapchain image. Optional GPU queries are read only when available, with disjoint/stale-result rejection; no query is synchronously awaited. The Performance remote page gives direct resolution/shader buttons and fluidity/balanced/detail choices.

## Less repeated work on every platform

| Work | Implementation |
| --- | --- |
| Palette animation | Retain the exact four palette-index taps, then recolor without terrain decoding/filter resampling; capped at 2.5 million samples |
| Terrain and actors | Cached tile attributes, conservative damage restoration and merged texture uploads; dense changes use a complete update |
| Rewind | Reuse state buffers, XOR/compression scratch and zlib streams, with fast zero-run compression; track workspace memory and preserve the state format and reversed stereo pairs |
| Mobile controls | Retain draw buffers; rebuild layout only when the window, safe area, device or settings change |
| Round lens | One cached indexed mesh; no steady-frame geometry allocation |
| Pacing | Fractional monotonic deadlines, display-aware desktop divisors and independent console timing |

Desktop vsync can use a compatible divisor, such as 72 on a 144 Hz panel. Mobile's explicit 30/90/120/maximum selections override the common target. The operating system and display remain authoritative.

## Verification and limits

Generated-data checks cover retained/full compositor and upload equivalence, including palette alpha changes; independent CPU/stereo pressure; reserve budgets, thermal/manual/core-limit behavior; continuous crown zoom, no-fold round meshes and round-safe simultaneous touch; and byte-exact rewind/audio round trips with memory limits. GLSL variants are compiled offline. Platform builds, Android lint and static APK/dependency/signature/alignment audits are separate checks.

**No app, game, emulator, installer, watch or headset session was executed for 0.10.** These checks do not certify startup, GPU output, physical controls, thermal behavior or sustained 60/90 fps on any particular device. Hardware profiling remains a later step. macOS and Wear OS remain experimental. A CPU-limited console core, sub-60-Hz display, throttling or literal-pixel lock can constrain the result.

References: [Android game performance](https://developer.android.com/games/optimize/power), [thermal feedback](https://developer.android.com/games/optimize/adpf/thermal), [OpenXR foveation](https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/extensions/fb/fb_foveation.adoc), [profile configuration](https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/extensions/fb/fb_foveation_configuration.adoc), [swapchain state](https://github.com/KhronosGroup/OpenXR-Docs/blob/main/specification/sources/chapters/extensions/fb/fb_swapchain_update_state.adoc), [disjoint timer queries](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt).
