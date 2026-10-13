# Your first game

Download the package for your computer from [Releases](https://github.com/vhaloo/SonicCompilationUltra/releases/latest). Extract the whole folder before opening the app. Keep the executable, libraries and `assets` folder together.

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
