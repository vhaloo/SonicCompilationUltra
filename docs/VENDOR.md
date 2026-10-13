# Pinned components and local changes

| Component | Pin | Licence |
|---|---|---|
| Genesis Plus GX | `393f70cd64c7334564c19a424c383f53986c13f4` | Noncommercial core licence and per-file notices |
| Dear ImGui | `f5befd2d29e66809cd1110a152e375a7f1981f06` / 1.91.9b | MIT |
| Musashi disassembler | `313ebf1bd9f4d0d93341eb5ce21fd8a119e9dbdd` | MIT-style upstream notice |
| SDL2 fallback | 2.32.10, archive SHA-256 `5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165` | zlib licence |
| zlib used by SCU | `51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf` / 1.3.1 | zlib licence |
| Noto Sans JP source | Google Fonts commit `295d98a7a0c17c68f1341eaeea354e7960ea70d3` | SIL OFL 1.1 |

The font source SHA-256 is `c2f3b4d463500a2ddcd3849cded1fceeb9fd6d1c32e6cbecd568453ba50fc68f`. The shipped subset is renamed **SCU Interface** and contains the application's Latin/Japanese UI characters. Original copyright and licence notices are retained.

The pinned SDL Windows build receives a small, idempotent patch in `cmake/SDLLegacyKeys.cmake`: a virtual keyboard message with no hardware scan code is resolved through the Windows key map before normal SDL translation. This supports virtual/on-screen and automation keyboards while preserving messages that already contain scan codes. The source archive retains the original pinned tarball and the patch script. A system SDL library is used as supplied by that system.

The vendored Genesis directories are the upstream `core` and `libretro` sources plus notices. SCU adds a read-only palette/background/scroll observer to `vdp_render.c`, fixes the omitted Boolean return in `update_geometry`, and builds through its own CMake source list with zlib 1.3.1. CPU, collision, native spawning and normal game timing are not rewritten by the presentation observer.

The frontend and compositor are new project code. Sonic Retro disassemblies and the mstan renderer projects were consulted for documented memory/layout formats; their game ROMs, generated game-program C, box art and extracted artwork are not included.

The standalone Musashi disassembler reads padded host copies of ROM instructions. Its entry points are renamed at compilation to avoid the cartridge CPU namespace; its unused memory callbacks return zero. It does not execute a second CPU or access cartridge RAM. The bounded host walker follows local branches, supported dispatch tables and continuations to distinguish executable instructions from adjacent data. Original source and hashes are retained in `vendor/musashi-disassembler/SOURCE.json`.
