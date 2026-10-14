# Your first game

Download the package for your computer from [Releases](https://github.com/vhaloo/SonicCompilationUltra/releases/latest). Extract the whole folder before opening the app. Keep the executable, libraries and `assets` folder together.

| Download | Designed for |
|---|---|
| Windows x64 | Windows 10/11 on a 64-bit Intel or AMD PC |
| macOS x64 — **experimental** | Intel Macs; deployment target 10.15; no physical Mac testing |
| macOS arm64 — **experimental** | Apple Silicon Macs; deployment target 11.0; no physical Mac testing |
| Linux x64 | Ubuntu 22.04 or a compatible newer 64-bit desktop; run `SonicCompilationUltra.sh` |

The macOS app has an ad-hoc signature, not Apple notarization. If Gatekeeper asks, follow Apple's [instructions for opening an app from an identified download](https://support.apple.com/en-us/102445) after checking you downloaded the package from this repository. The Windows app is unsigned; its publisher cannot be verified by a commercial signing certificate. These packages do not change your computer's security settings.

**macOS is experimental.** The two Mac packages receive automated compilation, synthetic runtime/controller tests and portable UI checks. Physical Mac display, audio, Retina and controller behaviour remain untested. Linux packages are validated on Ubuntu 22.04, including execution through their bundled launcher; physical Linux display/audio hardware is a separate check.

The app starts in fullscreen. Choose **Choose ROM…** with a mouse or the bottom gamepad face button. Navigate to your own cartridge dump and select it. ZIP files can stay zipped. **Import folder…** adds recognised collection editions from a folder; other Mega Drive revisions can be chosen individually and played with original framing.

The app remembers where the original file is. Moving or deleting it later means you must choose it again. Import does not alter your cartridge file.

Click **Play**, then press **Enter** or **gamepad Start** on the original title screen. Some original title animations briefly ignore input; wait for the menu and press Start again. Sonic 3 has its original save-file and character selection.

| Do this | Keyboard | Gamepad |
|---|---|---|
| Move | Arrow keys | D-pad / left stick |
| Jump / action | Z, X, C; Space also jumps | X, A, B |
| Original Start / pause | Enter | Start |
| Open app options | Esc / F10 | Select / View |
| Confirm in options | Enter | A |
| Back in options | Esc | B / right face button |
| Change options section | Ctrl + Tab; Shift reverses | LB / RB |
| Scroll options / files | Mouse wheel; Page Up/Down | LT / RT |
| Adjust a value | Enter, then arrows; Esc finishes | A, then D-pad / left stick; B finishes |
| Resume from options | Resume button | Start |
| Zoom | + / − or Ctrl + wheel | Hold RB + Up/Down |
| Free camera | WASD; Shift moves faster | Right stick |
| Save / load | Shift + F1…F9 / F1…F9 | Choose in Saves |
| Rewind at normal speed with reversed sound | Hold Backspace | Hold L2 / Xbox LT (default) |
| Screenshot | F12 | Hold RB + Start |
| Fullscreen | Alt + Enter | Choose in Screen |

The first two controllers drive players 1 and 2. A second keyboard player uses **I/J/K/L**, **U/O/P** and **right Shift**. The games retain their original multiplayer rules.

You can also click the menu reminder at the bottom of the game. Short keyboard presses are kept until the next game tick, so a fast display refresh does not swallow a quick Start or jump.

## Pick a comfortable view

- **Whole-level overview (default):** the current platform stage fits, with a player marker and live player inset. Other game projections keep their native frame.
- **Balanced:** the default quality; lower overview sampling reduces work without cropping the stage.
- **Lightweight:** less rendering work; try this if motion or sound struggles.
- **Ultra:** render the overview at full output resolution with a soft landscape; suited to large displays. For pixel 1:1 choose **Pixel world** and **1×**.
- **Panorama:** familiar original-height play with extra width.
- **Original framing:** preserve the original display, including special stages and puzzle boards.

On separate monitors, turn on **Fullscreen** and **Span all displays**. A unified Nvidia Surround / AMD Eyefinity desktop is treated as one wide display. Different monitor heights create a rectangular desktop canvas; the app cannot correct physical bezels or unequal screen densities.

**Camera & HUD** offers follow, fixed and free views. Choose **Focus on player** to move closer, or **Show whole stage** to return to the overview. Hold **RB + top face button** to switch between those views. The **Live player view in overview** option keeps the native scene readable while zoomed out. The locator points toward a player outside the view.

## Keep your progress

Save slots belong to a particular cartridge checksum. A damaged state or a different ROM is rejected. Previous files are preserved as `.bak`. Normal cartridge saves and app states are separate. Auto-resume keeps a local checkpoint when you close or switch games.

The Saves page shows all nine slots and their dates. Loads, overwrites, resets, switching games and restoring defaults have gamepad-friendly confirmations; Cancel receives focus first. You can turn these off under Gameplay.

**Rewind** is enabled on new installs. Hold **L2 / LT** or **Backspace** to go backwards at the cartridge’s normal speed, including music and effects; release to continue from that moment. The on-screen reminder shows the available seconds. At the oldest retained moment, holding rewind keeps the game still. **Gameplay** lets you disable rewind or its reverse sound, and choose a 24 / 96 / 192 MB history budget. Lightweight limits history to 24 MB. **Controls** lets you choose LT, LB or RT. Existing preferences are retained, so enable Rewind under Gameplay if you previously disabled it.

History builds as you play, stays in memory, and clears when loading a save, restarting, switching games or closing. Its duration depends on the game and selected memory budget. Releasing rewind discards the undone future; saving then keeps the new point in time. Volume and mute apply to reverse sound too. In options, triggers continue to scroll and never rewind gameplay.

Settings and saves live in your user profile, not inside the downloaded package:

| System | Default location |
|---|---|
| Windows | `%APPDATA%\vhaloo\SonicCompilationUltra\` |
| macOS | `~/Library/Application Support/vhaloo/SonicCompilationUltra/` |
| Linux | `~/.local/share/vhaloo/SonicCompilationUltra/` |

The advanced `--data-dir` option selects a portable data folder. Updating the application does not require deleting your data. Keep backups before changing builds; cross-build state compatibility is not guaranteed.

For a direct-play desktop shortcut, add `--resume-last` to its arguments. It opens the last selected cartridge and uses your saved presentation settings. The regular app opens the collection library.

## Comfortable options

**Screen** offers a full-screen menu or compact overlay, automatic/large/extra-large text, high contrast and display selection. Every setting is reachable by gamepad. The **Controls** page offers automatic Xbox/PlayStation/Nintendo hints, three gameplay button layouts, an adjustable menu/gameplay stick dead zone and optional in-game shortcuts. Hot-plugging does not require restarting.

In the ROM browser, select folders and files with the D-pad and confirm button. **LB** opens the parent folder; **Drives / locations** switches disks. **Edit path** opens a navigable on-screen keyboard; **Start** accepts the path. No typed path is needed for ordinary browsing. **B** backs out one dialog or edit at a time.
