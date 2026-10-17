# Terrain atlas implementation

The atlas is a read-only presentation feature. Cartridge emulation, object activation, native video/audio timing and saved-state formats remain unchanged. Supported ROM identities are listed in [ROMS.md](ROMS.md); unknown editions keep native framing.

## Spinball

Four cartridge pointer tables describe foreground/background graphics, 32×32 brush layouts and 4×4 tile brushes. A bounded dictionary decoder expands the SSC graphics into the two cartridge tile banks. Background indices are relative to the second bank; instance and tile flips are composed independently. Map dimensions are read from the cartridge.

The final board has a 2048-pixel foreground and a shorter 1536-pixel background. The cartridge changes vertical parallax slope at 1440. The atlas anchors that curve over the complete terrain and never reads the unused trailing background rows. The live native patch preserves the precise current-camera composition.

## Sonic 3D Blast

Each of the 22 act headers identifies raw terrain graphics, two block-mapping tables and the row layout. The 128 row offsets lead to header-stripped RNC method-2 streams; packed and unpacked CRC16 checks must pass, and each row must contain exactly 768 bytes. Those three-byte cells select the two 32×32 mappings and their flips/priority. Primary graphics use an extended tile-index bit; secondary graphics use the native preloaded VRAM bank. The atlas retains the original 8192×4096 isometric plane rather than flattening it into a top-down map.

The native X/Z/height coordinates are projected into the same plane for camera tracking and the player marker. Hardware sprites and an exact native-camera patch preserve local actors, depth, animation and dynamic foreground changes. This does not activate distant actors or forecast their scripts.

## Cache and reduced views

Only the current map is cached. Each atlas pixel stores a palette index and priority; palettes remain live. A tile-use index updates only affected cells when native VRAM graphics change. Native pattern decoding also skips unchanged tile bytes. A separate composed-terrain layer can be reused when geometry, used patterns, palettes and camera coverage are unchanged; actors and the native camera patch are drawn afterwards. Empty margins are culled before sampling. Reduced maps can average terrain samples and use linear presentation; exact 1:1 and magnified views remain nearest-sampled. Reduced Pixel world and tall Panorama render at no more than display resolution. The output allocation cap preserves world coverage instead of cropping it.

Decode failures return to the native frame. All input reads, output sizes and decompression loops are bounded. Synthetic tests cover interleaved control/literal bytes, overlapping copies, truncation, packed/output CRC failure, output overflow, the second Spinball tile bank, its shorter final background and far-away animated tiles. Local cartridge audits check all 26 layouts without including the cartridges in public tests.

## Format references

Research references include the [Sonic 3D Blast disassembly annotations](https://github.com/sonicretro/s3ddisasm) and [SonicSpintool](https://github.com/Eggplant891/SonicSpintool). They helped identify structures; native cartridge instructions and rendered snapshots were used to verify offsets and behaviour. No ROM, extracted tile bank, disassembly output or external tool source is incorporated into this repository. The bounded C++ readers and cache are original project code.
