# Wall AR on Quest 3

Open **Y → Quest VR / AR → Presentation mode**. The new modes keep a whole supported stage fixed in the room while you turn your head or move to inspect it. The original cartridge simulation keeps its normal speed.

| Mode | What you get |
|---|---|
| Spatial screen | The existing cinema, tilted tabletop and personal head-following presets, with normal camera choices. |
| Fit a scanned wall | A fixed screen inside the selected wall's bounds, up to the chosen height. The full stage fits without stretching its geometry. |
| Two-metre level mural | The stage's natural aspect ratio at **200 cm high from the floor** by default. The virtual plane can extend beyond the physical wall instead of shrinking the stage to its width. |
| Floor atlas | The full stage laid horizontally above the floor, with the same fixed framing and an adjustable physical scale. |

## Place it

**Automatic:** enable **Find a scanned wall automatically** and grant Quest's spatial-data permission when asked. The app loads the saved room model, prefers the wall you are looking at and retains that selection as you move. **Next wall** selects another nearby wall. **Capture / update room** opens Meta's own Space Setup flow.

**Manual:** when there is no usable room model, use **Place virtual wall in front of me**. It creates a fixed virtual plane at the chosen distance. The floor comes from a tracked floor origin when available; otherwise it starts from an explicit standing-height estimate. You can put the left controller at floor level and activate **Use left controller height as floor**. This is a manual calibration, not automatic recognition of a physical surface.

Scene recognition depends on a captured room, spatial permission and usable headset tracking. **An outdoor wall is not guaranteed to be detected.** Manual placement remains available without Scene permission. Camera images are composed by the Quest runtime; the app does not request camera frames, scene upload or a cloud-anchor service.

The two-metre mural is deliberately allowed to be wider than the real wall. The level has finite bounds. An infinitely large physical display, unlimited tracking range and headset-pixel 1:1 are not implied.

## Keep it still

**Lock wall placement** starts enabled. The map does not follow Sonic or your head, and newly previewed spawns do not resize the framing. Within the same stage, its map bounds remain fixed. Stage changes replace the map; native-only screens and unsupported special projections retain the original game image.

- **Y:** open the companion menu near you, leaving the wall in place.
- **Right stick click:** recenter the companion menu while the wall is locked.
- **X during mural play:** toggle the live player close-up.
- **Left trigger:** rewind with reverse sound.
- Unlock placement to move/scale it using both grips. The base stays at floor height; lock it again afterward.

**Save wall placement locally** creates and stores an on-device spatial anchor. **Restore saved placement** attempts to locate it in a later session. Saving is optional and depends on runtime support. A saved anchor may be unavailable after room changes or when tracking cannot recognize the space. The app preserves the last stored record if a new save fails.

If tracking of an attached wall is lost, the wall is hidden and the default **Pause when wall tracking is lost** pauses play. The companion menu stays reachable when head tracking is valid. The app does not silently attach the game to a different wall.

## Refresh rate and detail

**Maximum available** is the default headset refresh preference. The app enumerates the runtime's supported rates and requests the highest one; you can select System or a reported rate instead. The options display the active runtime rate and the requested rate. The runtime may limit or reject a request, including for its own thermal or power policy. This is not a measured sustained-FPS guarantee.

The original games keep their normal native cadence. Tracked composition continues at the headset rate; an unchanged game image reuses its submitted swapchain instead of repeating map rendering and framebuffer copies. Menu and game use separate compositor layers, so a long mural does not turn the controls into a distant strip.

Choose **Fast**, **Balanced** or **Sharp** mural detail. Texture dimensions are bounded by the runtime and GPU limits. Larger maps may be sampled down. **Adaptive mural resolution** adjusts the render budget after sustained CPU pressure and restores detail conservatively. It never changes the physical map bounds, game speed or requested headset rate. Turn it off for a fixed render budget.

## Status

This is an experimental standalone APK. Validation for this delivery is compilation, Android lint, source review and static APK/signature/dependency/content inspection only. No game, installer or headset session is executed. Physical alignment, relocalization, image quality, controllers, passthrough, comfort, sustained FPS and battery/thermal behavior require Quest 3 hardware validation.

## Official examples and design sources

The implementation uses the public Khronos APIs directly. The following official examples informed the design; their application code and room assets are not bundled:

- [Meta OpenXR XrSceneModel](https://github.com/meta-quest/Meta-OpenXR-SDK/tree/main/Samples/XrSamples/XrSceneModel): semantic wall/floor planes, asynchronous scene queries and Space Setup.
- [Meta OpenXR XrSpatialAnchor](https://github.com/meta-quest/Meta-OpenXR-SDK/tree/main/Samples/XrSamples/XrSpatialAnchor): asynchronous anchor creation, local storage and restoration.
- [Meta Scene overview](https://developers.meta.com/vr/documentation/native/android/openxr-scene-overview/): room models and surface placement.
- [Meta Scene best practices](https://developers.meta.com/vr/documentation/native/scene-best-practices/): load scenes when needed, reject distant rooms, refresh selected-anchor poses and retain interactive controls despite drift.
- [Meta spatial-data permission](https://developers.meta.com/vr/documentation/native/native-spatial-data-perm/): request Scene access when used and retain a fallback.
- [Meta Spatial SDK MRUK sample](https://developers.meta.com/vr/documentation/spatial-sdk/spatial-sdk-sample-mruk/): room-aware placement and explicit room setup controls.
- [Khronos display refresh request](https://registry.khronos.org/OpenXR/specs/1.1/man/html/xrRequestDisplayRefreshRateFB.html): enumerate available rates, request a preference and distinguish it from a guarantee.
- [Meta compositor sample](https://github.com/meta-quest/Meta-OpenXR-SDK/tree/main/Samples/XrSamples/XrCompositor_NativeActivity): tracked composition layers with bounded texture resources.
