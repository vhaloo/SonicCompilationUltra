# Sonic 1 source-project survey

Survey date: **13 October 2026**. The starting question was how to build a large, adaptable view of the first Sonic using a Genesis cartridge. The collection later expanded to the other Mega Drive games.

This catalogue separates decompilations, hand ports, static recompilers, disassemblies and engine mods. Forks are useful for platform support, but are not counted as independent decompilations. It covers publicly discoverable projects; private, deleted or unindexed work cannot be proven absent.

## Projects that run the 16-bit original or remake

| Project | What it is | Required game data / practical boundary |
|---|---|---|
| [RSDKv4 Decompilation](https://github.com/RSDKModding/RSDKv4-Decompilation) | Complete decompilation of the Retro Engine used by Sonic 1/2 (2013) | Requires the appropriate `Data.rsdk`; a Genesis ROM does not supply the remake's data pack |
| [RSDKv4 Script Decompilation](https://github.com/RSDKModding/RSDKv4-Script-Decompilation) | Reconstructed game scripts | Complements the engine; not a standalone ROM-based game |
| [rsdkv4-plus](https://github.com/prodnw/rsdkv4-plus) | Extended RSDKv4 engine fork | Remake data required; feature branch with its own compatibility limits |
| [Sonic 1 Forever](https://teamforeveronline.wixsite.com/home/sonic-1-forever) | Team Forever's expanded version of the 2013 game | Uses original mobile data, not the Genesis cartridge |
| [Forever Decomp 1.3](https://github.com/LittlePlanetCD/Forever-Decomp-1.3) | RSDKv4 engine variant for Forever projects | Relevant engine distribution; does not make the official game data freely redistributable |
| [TeamForeverDev/Sonic1Forever](https://github.com/TeamForeverDev/Sonic1Forever) | Named official repository | Empty at the time of this survey; not evidence of a publicly complete game-source release |
| [foreverWIP/s1-port](https://github.com/foreverWIP/s1-port) | Full C reimplementation / hand port, still WIP | Genesis revision 01; upstream describes a playable alpha, with renderer and platform limitations |
| [mstan/SonicTheHedgehogRecomp](https://github.com/mstan/SonicTheHedgehogRecomp) | Static 68000-to-C recompiler runner with experimental adaptive scenery reconstruction | Genesis revision 00; native route coverage remains partial and expanded rendering is experimental |
| [cuckydev/SoniCPort](https://github.com/cuckydev/SoniCPort) | Earlier Sonic 1 native port | Superseded work; inspect successor and current build status before choosing it |
| [cuckydev/Sonic1C](https://github.com/cuckydev/Sonic1C) | C conversion close to the original assembly | Small, early project; not equivalent to a validated turnkey cross-platform release |

### Useful RSDK platform branches

[JeffRuLz's New 3DS port](https://github.com/JeffRuLz/Sonic-1-2-2013-Decompilation),
[SonicMastr's Vita port](https://github.com/SonicMastr/Sonic-1-2-Vita),
[mattConn's WebAssembly port](https://github.com/mattConn/Sonic-Decompilation-WASM),
[Sappharad's Mac branch](https://github.com/Sappharad/Sonic-1-2-2013-Decompilation),
[leorelic/RSDKv4](https://github.com/leorelic/RSDKv4), and
[123TOSG's iOS branch](https://github.com/123TOSG/RSDKv4-Decompilation-iOS)
are relevant variants of the same engine family. Their current compatibility and release instructions should be checked at the linked source. They do not turn a Genesis ROM into a mobile remake data pack.

## Disassemblies and analysis

| Project | Scope / use |
|---|---|
| [sonicretro/s1disasm](https://github.com/sonicretro/s1disasm) | Community disassembly of the Genesis original; essential memory, object, tile and layout documentation; builds cartridge code rather than a standalone PC renderer |
| [cvghivebrain/s1disasm](https://github.com/cvghivebrain/s1disasm) | Related maintained disassembly branch/toolchain |
| [Iso-Kilo/Sonic-1-Mega-Play-Disassembly](https://github.com/Iso-Kilo/Sonic-1-Mega-Play-Disassembly) | Mega Play arcade adaptation; an assembly route, not a standalone modern PC renderer |
| [Sonic-1-Prototype-Disassembly](https://github.com/Iso-Kilo/Sonic-1-Prototype-Disassembly) and [Sonic-1-Prototype](https://github.com/Iso-Kilo/Sonic-1-Prototype) | Prototype analysis and restoration work; distinct from a completed retail-game C decompilation |
| [OrionNavattan/Sonic1](https://github.com/OrionNavattan/Sonic1) | Additional original-game assembly work / branch |
| [JarrettBillingsley/DecompilingSonic1](https://github.com/JarrettBillingsley/DecompilingSonic1) | Educational decompilation/analysis in C++; the author does not present it as a guaranteed runnable port |
| [Wilps93/sonic1-genesis-decompilation](https://github.com/Wilps93/sonic1-genesis-decompilation) | Pseudo-C reverse-engineering analysis; not a drop-in playable engine |

Disassemblies sometimes contain extracted game data. Their existence on a public host is not permission to mirror those assets in a new application. This project publishes memory/format readers and software, not their game-data trees.

## Other versions called Sonic 1

| Project | Version / distinction |
|---|---|
| [iamgreaser/s1smsdisasm-gm](https://github.com/iamgreaser/s1smsdisasm-gm) | 8-bit Master System disassembly, WIP; a different game/engine from the Genesis original |
| [maxim-zhao/STH1EDWV](https://github.com/maxim-zhao/STH1EDWV) | Master System disassembly/tooling branch |
| [TheRavenfreak/Sonic-1-Game-Gear-Disassembly](https://github.com/TheRavenfreak/Sonic-1-Game-Gear-Disassembly) | Game Gear split disassembly, WIP |
| [mstan/SonicTheHedgehogSMSRecomp](https://github.com/mstan/SonicTheHedgehogSMSRecomp) and [smsggrecomp](https://github.com/mstan/smsggrecomp) | Native Z80 recompiler ecosystem; early Sonic SMS bring-up, not full-game certification; the engine's README did not declare a project-wide licence at survey time, so it is not redistributed here |
| [Iso-Kilo/Sonic-1-J2ME-Decompilation](https://github.com/Iso-Kilo/Sonic-1-J2ME-Decompilation) | Decompiled 2005 Java mobile parts; upstream describes obfuscation/unbuildable work, includes several original phone resolutions |
| [yowari/sonic-j2me-decompilation](https://github.com/yowari/sonic-j2me-decompilation) | Buildable-workflow effort for the J2ME Part One game; separate phone implementation |
| [josepilas/sonic-j2me-decomp](https://github.com/josepilas/sonic-j2me-decomp) | Related Java mobile work |
| [IsoKilo/Sonic-1-EB-VS3-Decomp](https://github.com/IsoKilo/Sonic-1-EB-VS3-Decomp) | Phone/device-specific version analysis |
| [ArchiverXP/sncdecomp](https://github.com/ArchiverXP/sncdecomp) | Didj “Sonic the Hedgehog” project; not the 1991 Genesis game |

## Origins and the expanded collection

[RSDKv5u-Origins](https://github.com/thesupersonic16/RSDKv5u-Origins) concerns the Origins runtime and its own original data requirements. It is a different route from directly reading a Genesis cartridge.

[SonicTheHedgehog2Recomp](https://github.com/mstan/SonicTheHedgehog2Recomp) has substantial game-specific mods and a wide renderer;
[Sonic3AndKnucklesRecomp](https://github.com/mstan/Sonic3AndKnucklesRecomp) covers the three Sonic 3-family modes and explicitly describes bring-up/experimental boundaries. These are useful sources for formats and comparison, not evidence that every campaign route is certified.

## Decision for this collection

The first prototype compiled and tested the mstan Sonic 1 route locally, including adaptive ratios, 1:1 rendering and backgrounds. Its generated game program and private cartridge-derived captures were kept outside the public release.

For a single collection including Mean Bean and all standard lock-ons, the released app uses [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX) to run the original cartridge program, with a separate host world compositor. This keeps native game execution available across the collection and avoids distributing ROM-derived generated C. The enhanced renderer remains experimental; this survey does not present it as a newly completed decompilation.

## Search trail and limits

The survey used primary repository READMEs, source/configuration files and release pages, plus GitHub repository searches for **Sonic decompilation**, **sonic1**, and **Sonic disassembly**. Additional pages completed the initial `sonic1` and disassembly query results. Direct fork inventories were retrieved for the main RSDK, Sonic Retro, C-port and recompilation families.

[The repository/fork inventory](research-repositories.csv) records names, public URLs, descriptions, parent families and retrieval metadata. It includes forks and search noise; it is a retrieval appendix, **not a count of independently completed decompilations**. A project's README claim is distinguished from locally measured results in [TEST_REPORT.md](TEST_REPORT.md).
