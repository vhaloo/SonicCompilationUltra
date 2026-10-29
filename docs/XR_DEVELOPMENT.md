# XR 0.7: interaction, rendering and verification

**Development release; no 0.7 headset session has been started.** Compilation, lint, pure CPU interaction checks and static archive inspection are the available evidence. The previous release's menu captures do not validate the new controls or rendering.

## Decisions from primary documentation

| Reference | Implementation decision |
|---|---|
| [Meta passthrough](https://developers.meta.com/vr/documentation/native/android/mobile-passthrough/) | Keep Quest's OPAQUE environment blend mode. Compose the room under source-alpha layers. Clear empty AR scenery to zero alpha and retain premultiplied alpha through the stereo shaders. |
| [Khronos hand aim state](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrHandTrackingAimStateFB.html) and [gesture flags](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrHandTrackingAimFlagBitsFB.html) | Enumerate optional EXT hand tracking and FB aim support; use their validity and system-gesture flags. Index pinch presses; middle pinch grips. Unsupported tracking leaves controller/pad input available. Hand data stays in memory. |
| [Meta hand tracking](https://developers.meta.com/vr/documentation/native/android/mobile-hand-tracking/) and [native samples](https://github.com/meta-quest/Meta-OpenXR-SDK/tree/main/Samples/XrSamples/XrHandsFB) | Declare optional hand support. Supply ray/pinch and nearby poke input, not just a drawn joystick. Release controls when tracking/focus is lost. Precision must be checked on the headset later. |
| [OpenXR display refresh](https://registry.khronos.org/OpenXR/specs/1.1/man/html/XR_FB_display_refresh_rate.html) | Request 90 Hz by default, enumerate supported rates and choose the closest reported rate for an unsupported explicit request. Preserve native cartridge simulation speed. A one-time settings migration retains a backup. |
| [Meta FFR guidance](https://developers.meta.com/horizon/documentation/unreal/os-fixed-foveated-rendering/) and [OpenXR foveation swapchains](https://registry.khronos.org/OpenXR/specs/1.0/man/html/XrSwapchainCreateInfoFoveationFB.html) | Keep small text on an independent compositor panel and bound stereo resolution. Do not enable a new unqualified foveation path in this release; text legibility and real GPU bottlenecks still need hardware measurements. |
| [Sony's KV-27FS120 reference and manuals](https://www.sony.com/electronics/support/product/kv-27fs120/manuals) | Use original meshes informed by a real 4:3 consumer CRT: deep housing, rear taper, vents, speaker grilles and front controls. Also provide compact and studio variants. No Sony model, logo, image or manual is bundled. |
| [Sonic 1](https://github.com/sonicretro/s1disasm), [Sonic 2](https://github.com/sonicretro/s2disasm) and [Sonic 3 & Knuckles](https://github.com/sonicretro/skdisasm/tree/73de97c6b55055bcb8089c1d55d006b02136f163/Levels/Misc) object-table metadata | Add 152 named scenery types to the existing read-only spawn catalogue. [The inventory](MOVING_OBJECTS.csv) records identifiers/names, not cartridge code or artwork. The original S3 table's unclassified objects use origin markers rather than guessed art. |

## Spatial controls

The remote has seven pages: modes, placement, virtual play controls, picture, performance, all settings, and fine placement. The mode page directly selects wall fit, mural, floor atlas, pinball, CRT, curved gallery, cinema, tabletop and head follow. The existing full options panel remains available, including its illustrated explanations and four languages.

Ray intersection, direct poke and drawing use the same button rectangles. Index input captures a button until release. Pinch uses different press/release thresholds; focus/tracking loss clears captures and native input. A captured joystick becomes neutral when its ray no longer intersects the control plane. Grip manipulation suppresses gameplay inputs.

Choose a placement target or aim at a remote/native-CRT handle. One middle-finger grip translates and optionally rotates it, using a relative six-degree-of-freedom transform. Two grips resize the stage. Fine placement provides held buttons for XYZ movement and yaw/pitch. Select Move stage to unlock it. Control poses are session-local; dimensions and preferences persist. R3 recovers the nearby controls and native monitor.

With no tracked Touch controller or external gamepad, Auto exposes the large virtual joystick after a short debounce. One hand can hold/drag direction while the other presses A/B/C, Start, both flippers or rewind. The idle virtual Touch pad is detached when Touch pose actions are inactive so an external pad can occupy player one. Hand tracking must be enabled in the headset. This fallback is implemented and compiled, but its ergonomics and runtime hand-source switching are unqualified.

## Transparent and coplanar presentation

AR starts with a transparent scene background. Terrain, real actors and transparent spawn previews remain; empty areas reveal passthrough. The native CRT always retains the authentic complete framebuffer. Spinball's board is retained rather than being removed as "background." This does not make an ordinary desktop OS window transparent.

In the main CRT mode, coplanar presentation makes the tube picture flat. Surrounding terrain uses the same Z plane and native pixel scale. A UV cutout removes the duplicated native viewport from the terrain layer. The bezel can still physically occlude its border. Turning coplanar presentation off restores the curved legacy composition. The optional native close-up CRT has its own independent pose and size in other expanded modes.

## Allocation and work reductions

- Balanced eye buffers cap their longest dimension at 1280, compared with 1600 before: `1280² / 1600² = 0.64` at equal aspect. This is a 36% pixel-budget reduction, not a measured FPS improvement. Lightweight and detailed caps are 1024/1600.
- Mural/board canvases use separate 1/2/4-megapixel budgets, subject to aspect and texture limits. Physical scaling does not enlarge them.
- Stereo scenes sample their SDL canvas directly. They no longer create or blit an unused game compositor swapchain.
- Native/backdrop conversion reuses scratch storage. Backdrops use subimage updates after initial allocation. Transparent mode skips backdrop synthesis and uploads.
- Terrain retains one priority buffer, avoiding its former per-frame copy and duplicate allocation. Frozen preview sprites skip repeat pixel construction; appearance/art caches have bounded entry counts.
- Uniform locations and furniture geometry are cached. The presenter supplies one complete GL state guard around submission rather than another nested guard in the stereo renderer.
- Remote lettering is rerendered only when its labels/state change; moving a panel does not rebuild its lettering texture.

## What remains to verify later

No new runtime, image, frame-rate, comfort, hand/controller, passthrough, wall alignment or full-game certification is claimed. The pure helper checks cover button hit regions/bounds, invalid coordinates, joystick deadzone/normalization, pinch hysteresis/release and viewport alignment. They do not emulate the headset. macOS packages are experimental, compiled/packaged only, ad-hoc signed and unnotarized.

On a later user-authorized hardware pass, check the remote in every mode, two-hand play, tracking loss/source switching, grip priority and clutched resizing, front/back alpha, CRT seam alignment and sustained CPU/GPU/memory behavior. Scene/Guardian prompts and hand-tracking settings remain system responsibilities; the application does not disable them.
