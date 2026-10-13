# Bring your own cartridge

The app is software, not a source of game licences. Supply a cartridge dump or authorised backup that you are entitled to use. It provides no game-download service or pirate-site links.

To learn about making a dump from your own cartridge, search for **“dump my Sega Genesis cartridge”** and the exact game title. Look for documentation from the cartridge-reader manufacturer. A mobile `Data.rsdk`, a CD image and a Genesis cartridge are different formats; they cannot be substituted for one another here.

Owning a cartridge does not automatically permit every copying, circumvention or distribution activity in every country. Check the rights applicable to your copy. Do not upload your ROM to this repository or to an issue.

## Recognised editions

File names can be changed; the **size and CRC32 of the uncompressed, normalised cartridge** identify an edition. The importer shows the recognised game and accepts ordinary ZIP archives. Headered/interleaved SMD dumps and byte-swapped dumps are normalised in memory.

| Game | Edition | Bytes | CRC32 |
|---|---|---:|---|
| Sonic the Hedgehog | USA / Europe, revision 00 | 524288 | `F9394E97` |
| Sonic the Hedgehog 2 | World, revision A | 1048576 | `7B905383` |
| Sonic the Hedgehog 3 | USA | 2097152 | `9BC192CE` |
| Sonic & Knuckles | World, main cartridge | 2097152 | `0658F691` |
| Sonic 3 & Knuckles | USA lock-on | 4194304 | `63522553` |
| Knuckles in Sonic 2 | World revision A lock-on, with UPMEM | 3407872 | `2AC1E7C6` |
| Blue Sphere (S&K + Sonic 1) | USA / Europe lock-on | 2621440 | `E01F6ED5` |
| Dr. Robotnik’s Mean Bean Machine | USA | 1048576 | `C7CA517F` |
| Sonic Spinball | USA | 1048576 | `677206CB` |
| Sonic 3D Blast | USA / Europe / Korea | 4194304 | `44A2CA44` |
| Sonic Compilation / Sonic Classics | USA / Europe / Korea, revision A | 3145728 | `95B5E8D7` |
| S&K UPMEM chip | Lock-on chip, separate from main cartridge | 262144 | `4DCFD55C` |

These identities enable game-specific features. Other valid Mega Drive cartridge revisions may be imported individually with original framing. They are not claimed to have expanded-view coverage. Changing a ROM after import is detected; select it again to update the library.

## Lock-on

Import Sonic & Knuckles and a matching partner. The library automatically makes these combinations available without rewriting either original file:

- **S&K + Sonic 3:** Sonic 3 & Knuckles.
- **S&K + Sonic 2 revision A + UPMEM:** Knuckles in Sonic 2.
- **S&K + Sonic 1 revision 00:** Blue Sphere.

A combined dump can also be selected directly. The 256 KiB UPMEM image is part of the lock-on cartridge hardware; a 2 MiB S&K main-ROM dump alone does not contain it. The app can import both members when your own S&K ZIP contains both chips.

The optional **Gameplay → Attach Sonic & Knuckles** feature uses the original lock-on behaviour with the selected cartridge. Other game cartridges expose Blue Sphere rather than inserting Knuckles into their normal gameplay. **Restart or reopen the game after changing this option.** Saves are separated from the unattached game and checked against the effective combined checksum.

Small mapper helper copies, when needed for Knuckles in Sonic 2, are made only in your private user-data directory. They are never included in releases or uploaded.
