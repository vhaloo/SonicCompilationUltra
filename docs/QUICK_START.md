# Your first game

Download the package for your computer from [Releases](https://github.com/vhaloo/SonicCompilationUltra/releases/latest). Extract the whole folder before opening the app. Keep the executable, libraries and `assets` folder together.

| Download | Designed for |
|---|---|
| Windows x64 | Windows 10/11 on a 64-bit Intel or AMD PC |
| macOS x64 | Intel Macs; deployment target 10.15 |
| macOS arm64 | Apple Silicon Macs; deployment target 11.0 |
| Linux x64 | Ubuntu 22.04 or a compatible newer 64-bit desktop; run `SonicCompilationUltra.sh` |

The macOS app has an ad-hoc signature, not Apple notarization. If Gatekeeper asks, follow Apple's [instructions for opening an app from an identified download](https://support.apple.com/en-us/102445) after checking you downloaded the package from this repository. The Windows app is unsigned; its publisher cannot be verified by a commercial signing certificate. These packages do not change your computer's security settings.

Click **Choose ROM…**. Navigate to your own cartridge dump and select it. ZIP files can stay zipped. **Import folder…** adds recognised collection editions from a folder; other Mega Drive revisions can be chosen individually and played with original framing.

The app remembers where the original file is. Moving or deleting it later means you must choose it again. Import does not alter your cartridge file.

Click **Play**, then press **Enter** or **gamepad Start** on the original title screen. Some original title animations briefly ignore input; wait for the menu and press Start again. Sonic 3 has its original save-file and character selection.

| Do this | Keyboard | Gamepad |
|---|---|---|
| Move | Arrow keys | D-pad / left stick |
| Jump / action | Z, X, C; Space also jumps | X, A, B |
| Original Start / pause | Enter | Start |
| Open app options | Esc / F10 | Select / View |
| Confirm in options | Enter | A |
| Back in options | Esc | B |
| Zoom | + / − or Ctrl + wheel | Choose in Screen options |
| Free camera | WASD; Shift moves faster | Right stick |
| Save / load | Shift + F1…F9 / F1…F9 | Choose in Saves |
| Rewind, when enabled | Hold Backspace | Hold left shoulder |
| Screenshot | F12 | Choose in Saves |
| Fullscreen | Alt + Enter | Choose in Screen |

The first two controllers drive players 1 and 2. A second keyboard player uses **I/J/K/L**, **U/O/P** and **right Shift**. The games retain their original multiplayer rules.

You can also click the menu reminder at the bottom of the game. Short keyboard presses are kept until the next game tick, so a fast display refresh does not swallow a quick Start or jump.

## Pick a comfortable view

- **Balanced:** a useful first choice on ordinary PCs.
- **Lightweight:** less rendering work; try this if motion or sound struggles.
- **Ultra:** pixel 1:1, a player-centred camera and a soft landscape; suited to large displays.
- **Panorama:** familiar original-height play with extra width.
- **Whole-level overview:** fit all currently installed foreground geometry into the window.
- **Original framing:** preserve the original display, including special stages and puzzle boards.

On separate monitors, turn on **Fullscreen** and **Span all displays**. A unified Nvidia Surround / AMD Eyefinity desktop is treated as one wide display. Different monitor heights create a rectangular desktop canvas; the app cannot correct physical bezels or unequal screen densities.

**Camera & HUD** offers follow, fixed and free views. If you lose sight of Sonic, press **R**, enable the locator or open the minimap. The locator points toward a player outside the view.

## Keep your progress

Save slots belong to a particular cartridge checksum. A damaged state or a different ROM is rejected. Previous files are preserved as `.bak`. Normal cartridge saves and app states are separate. Auto-resume keeps a local checkpoint when you close or switch games.

Settings and saves live in your user profile, not inside the downloaded package:

| System | Default location |
|---|---|
| Windows | `%APPDATA%\vhaloo\SonicCompilationUltra\` |
| macOS | `~/Library/Application Support/vhaloo/SonicCompilationUltra/` |
| Linux | `~/.local/share/vhaloo/SonicCompilationUltra/` |

The advanced `--data-dir` option selects a portable data folder. Updating the application does not require deleting your data. Keep backups before changing builds; cross-build state compatibility is not guaranteed.

For a direct-play desktop shortcut, add `--resume-last` to its arguments. It opens the last selected cartridge and uses your saved presentation settings. The regular app opens the collection library.
