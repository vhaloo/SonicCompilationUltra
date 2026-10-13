# See enemy spawns before they surprise you

In an expanded platform view, enemies outside the original visible area appear as translucent previews at their cartridge-authored spawn points. The setting starts enabled at **35% opacity**, with ghost sprites and small amber diamond markers.

Open **Esc / F10 / gamepad Select or View**, choose **Camera & HUD**, then adjust **Preview distant enemy spawns**, **Enemy preview style** and **Enemy preview opacity**. Every control works with the D-pad or stick, confirm/back buttons and triggers for scrolling.

| Style | What you see |
|---|---|
| Ghost sprites + spawn markers | Transparent cartridge sprites and readable position markers |
| Ghost sprites | Sprites, with a marker when the image cannot be shown |
| Spawn markers | Small amber diamonds that remain visible in a whole-stage overview |

The preview disappears when the native enemy sprite is visible. A defeated enemy is hidden when the cartridge remembers its defeat; enemies that the original game allows to respawn can return. Waiting enemies with an empty native sprite can retain their warning until they actually appear.

These are **spawn points**, not live enemy positions or predictions of attacks. They do not start enemy AI early or change collisions, spawning, object capacity, cartridge RAM or save-state contents. Initial sprites can differ from an enemy’s later animation or multipart attack. Floor settling and scripted motion can move an enemy after its authored placement.

The supported ROM editions of **Sonic 1, Sonic 2, Sonic 3, Sonic & Knuckles, Sonic 3 & Knuckles and Knuckles in Sonic 2** have placement previews in their expanded ordinary platform stages. Original framing, special-stage projections, competition, Mean Bean, Spinball, 3D Blast and the original Compilation cartridge keep their native display. Bosses, generated projectiles and enemies created only by event scripts are outside this preview.

Sprite pixels come from the locally imported cartridge’s mappings and loaded art. Supported dynamic sprite slots use the cartridge’s original uncompressed art and frame-loading descriptions, rather than borrowing another enemy’s current VRAM slot. No extracted enemy artwork is shipped. If an image is unavailable, the marker still reports the validated placement.

The placement formats were checked against the primary [Sonic 1](https://github.com/sonicretro/s1disasm), [Sonic 2](https://github.com/sonicretro/s2disasm) and [Sonic 3 / Knuckles](https://github.com/sonicretro/skdisasm) disassembly references. The application contains its own presentation parser and uses the user’s ROM; those disassemblies and their game assets are not bundled.

Synthetic tests cover all six address profiles, lock-on banking, remembered defeat, native visibility, waiting sprites, dynamic art, opacity, tiny-screen markers, disabled previews and read-only snapshots. Opening-stage captures from locally supplied cartridges check the complete renderer. This is sampled coverage, not full-campaign certification.
