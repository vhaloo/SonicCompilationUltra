# Automatic performance

**For current 0.10 quality/reserve controls and Wear OS, read [Fluidity and automatic quality](AUTOMATIC_QUALITY.md).** The material below records the original 0.9 performance pass and its historical measurements.

**0.9 targets 60 fps presentation by default, with adaptive scene resolution enabled on every platform and view mode.** It preserves console timing, camera coverage, the native game image, rewind and spawn previews. Quest keeps the selected headset refresh rate (90 Hz by default) and updates stereo tracking at that rate. A PAL title retains its original timing; rendering more display frames does not speed up the game.

Open **Video** or the Quest options to change:

- **Automatic performance / resolution:** reduce scene sampling under sustained CPU/GPU load and restore detail slowly after headroom returns. Short expensive frames are retained in a peak envelope; menu/idle waiting and long pauses do not count as active rendering work.
- **Presentation target:** 60 by default; 30–120 available. Desktop vsync uses a compatible integer display divisor where possible (for example 72 on a 144 Hz panel). Mobile's default follows the common target; explicit 30/90/120/maximum choices override it. Actual refresh remains controlled by the device, window mode and operating system.
- **Minimum / maximum scene resolution:** 35–100% by default, adjustable within 25–100%. Percentages scale the sampling dimensions, so 50% uses a quarter as many scene pixels. Camera framing is unchanged. Quality presets and GPU/memory limits still bound the initial scene ceiling.
- **Lock literal 1:1 Pixel view:** protect source sampling in Pixel mode. This overrides scene downsampling in that view and can limit the automatic performance gain. The native screen and UI stay separate from adaptive world sampling in all modes.
- **React to Android thermal pressure:** locally read the operating system's thermal state. Moderate, severe and critical pressure cap scene detail. No new permission, network connection, camera, microphone, device setting or system-wide performance override is used.
- **Stereo shader quality:** Automatic, Efficient or Reference. Efficient retains texture colors, gamma and the CRT edge vignette. Its scanline wave and material highlights are cheaper approximations; bloom/fresnel use equivalent multiplication/square-root forms. Automatic begins efficient, uses Reference only after sustained measured GPU headroom, and holds the efficient variant after an overload to prevent rapid switching. Both programs compile at startup.

On Quest, the spatial remote's Performance page also has direct **Auto resolution** and **CRT shader** buttons. Keyboard, gamepad, touch and the full VR options panel remain supported. The performance readout reports the last active work measurements, current adaptive factor and actual CPU scene dimensions. GPU time is shown only after a valid measurement. The native simulation is identified separately when lowering scene resolution cannot solve a CPU/core bottleneck.

## What changed

| Work | 0.8 | 0.9 |
| --- | --- | --- |
| Automatic resolution | Fixed-wall Quest-only heuristic; mobile static scene/UI target budget | Shared CPU/GPU/thermal governor for all modes and platforms; native/UI stay separate |
| Terrain sampling | Repeated tile-address decoding per sampled pixel | Whole-domain tile-attribute cache keyed by actual geometry references |
| Retaining terrain | Copy every terrain pixel for each changed game frame | Restore conservative damaged regions, with dense-frame fallback |
| Texture uploads | Complete world image upload | Merged dirty regions, bounded call count and full upload when required |
| Stereo resolution | User detail selection recreates eye swapchains | Stable swapchain ceiling; adaptive viewport and submitted image rectangle |
| GPU feedback | Wall-frame elapsed time, including submission wait | Optional disjoint-safe timer-query ring, read only after availability; CPU fallback |
| Frame pacing | Desktop redraws could exceed the useful game rate; mobile rounded each period | Monotonic fractional deadlines, display-aware vsync pacing and independent simulation |
| Other work | Repeated frozen backdrop copies / individual scanline draw calls | Retained backdrop buffers, one batched scanline call, retained actor allocation and bounded mobile metrics polling |

Sparse restoration preserves the original draw order and pixel values. Native foreground, actors, enemy/boss/script/moving-object previews and their previous bounds all participate in damage marking. Dense animation, palette changes and broadly distributed previews can still require a full update; the governor then adjusts scene sampling rather than changing gameplay or hiding previews. Sonic 1 geometry keys include referenced high chunk/block banks and exclude unrelated actor state.

## Validation and limits

The standalone checks in `tests/performance` link only generated-data math/cache/input helpers. They cover CPU/GPU overload, intermittent XR overruns, minimum/maximum bounds, thermal caps, manual opt-out, native-core bottlenecks, invalid GPU input, pause recovery, fractional deadlines, display divisors, terrain revision sensitivity, sparse restoration and retained-texture equivalence at tiny, normal, 4K and ultrawide sizes. A separate executable in `tests/rendering` compares the actual retained compositor and its upload regions against fresh full rendering for 144 generated cases, without SDL, a cartridge or an emulator. Existing spatial and touch checks remain separate.

On the development PC, one **generated 4K restoration microbenchmark** copied **76,185,600 bytes** instead of **6,635,520,000 bytes** across 200 iterations, a **98.85% reduction** for that sparse workload. These figures quantify helper work only. Dense frames can fall back to full copies. They are not a game benchmark or a promise of a corresponding FPS increase.

Both exact GLES shader variants compile/link with the offline GLSL compiler. The efficient scanline approximation's maximum scalar error is about 0.01001 before strength/anti-aliasing; at the maximum native CRT strength the brightness difference is about 0.00431. Texture colors/gamma/vignette remain unchanged; highlight appearance is an approximation and the Reference option is retained.

**No application, game, emulator, installer or headset session is executed for this release.** Compilation, lint, static APK/package inspection, shader compilation and generated-data checks cannot certify sustained 60 fps, GPU output, startup, thermal behavior, controller ergonomics or every scene/device combination. A slow native emulator/core, a sub-60-Hz display, hardware throttling or a literal-pixel lock can prevent the requested rate. macOS remains experimental. Physical phone/tablet/Quest and real-game profiling remain a later, user-authorized step.

Platform references: [Android power/performance guidance](https://developer.android.com/games/optimize/power), [Android thermal API](https://developer.android.com/games/optimize/adpf/thermal), [Meta missed-frame guidance](https://developers.meta.com/vr/documentation/native/android/os-missed-frames/), [Khronos disjoint timer queries](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_disjoint_timer_query.txt). They inform the implementation, not hardware qualification.
