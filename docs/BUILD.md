# Build from source

The source builds without a game ROM. A C99/C++17 compiler and CMake 3.20+ are required. SDL2 2.28+ can be supplied by the system; otherwise CMake obtains a SHA-256-pinned SDL2 2.32.10 source archive. Genesis Plus GX, Dear ImGui and zlib sources are included.

## Windows

Install Visual Studio 2022 Build Tools with the C++ desktop workload and Windows SDK. In a developer terminal:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix dist/SonicCompilationUltra
```

Use the installed folder; it includes SDL2, the font and required runtime libraries. The app uses the static MSVC CRT; OpenMP's redistributable is staged when required. No ROM is copied during compilation or installation.

## Linux

On Ubuntu, install a compiler, CMake, Ninja and the SDL platform-development dependencies:

```sh
sudo apt install build-essential cmake ninja-build libsdl2-dev libasound2-dev \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev \
  libxkbcommon-dev libwayland-dev libegl1-mesa-dev libgl1-mesa-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/SonicCompilationUltra
```

## macOS — experimental

Intel and Apple Silicon builds receive automated checks. They have not been tested on a physical Mac, so macOS packages remain experimental.

Install Xcode Command Line Tools, CMake and Ninja. A Homebrew SDL2 is optional; the pinned source fallback also works.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/SonicCompilationUltra
```

The release packager assembles the `.app`, resources and SDL library. Intel and Apple Silicon builds are separate. Release builds target macOS 10.15 on Intel and 11.0 on Apple Silicon; use `-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15` or `11.0` when reproducing them. No physical Mac gameplay certification is implied by a successful build.

The downloadable corresponding-source archive includes the complete, unmodified SDL tarball under `vendor/`. CMake verifies and extracts this local archive, so rebuilding requires no SDL download.

## Development and validation

`scu_tests` uses synthetic tile/layout data, never copyrighted game fixtures. `scu_runtime_tests` generates an original minimal 68000 program and PSG tone at runtime to verify CPU execution, VDP output, saves, integrity checks, exact-frame rewind, reverse stereo sound, audio queue direction/mute, PAL timing and bounded delta history. It contains no commercial cartridge code, art or audio. `scu_ui_tests` feeds a real SDL virtual gamepad through the importer, path keyboard, every options page, save slots, dialogs, shortcuts and resizing; it also verifies reconnect and L2/LT rewind pacing at 10/30/60/144 Hz display intervals. The software-rendered ultrawide menu cases can take a few minutes. A ROM-free UI smoke test can run with `SDL_VIDEODRIVER=dummy` and `SDL_AUDIODRIVER=dummy` plus `--software --headless --frames 10 --menu`.

```sh
SonicCompilationUltra --help
SonicCompilationUltra --import /path/to/owned-cartridge.zip --game sonic1
SonicCompilationUltra --data-dir /private/test-data --headless --game sonic1 \
  --size 1920x1080 --frames 1000 --script /private/input.timeline \
  --capture /private/capture.bmp --report /private/report.json
```

Timeline commands support `tab 0..8`, held controller buttons, presentation settings, save/load, captures and assertions. `rewindhold on` / `rewindhold off` exercises the same held-rewind cadence as gameplay; `rewind` restores one frame. A timeline step is a host-loop iteration; use non-real-time test runs for repeatable input. `--no-fast-boot` retains the original boot timing. Headless throughput is not a display-frame-rate measurement. ROMs, snapshots, logs and captures used for game tests belong outside the public source tree.

Original project sources follow `.clang-format`. Vendored sources preserve upstream licensing and are not reformatted. The Genesis core patches are identified in [VENDOR.md](VENDOR.md).

For reproducible menu captures, the timeline command `guide zoomout` opens the localized illustrated explanation for that setting. `tools/smoke_ui.py` captures every page and representative guide in four languages at three window sizes. Cartridge terrain formats and cache boundaries are described in [TERRAIN_ATLASES.md](TERRAIN_ATLASES.md).

To audit a locally owned, supported raw cartridge without initializing video, audio or a window:

```sh
SonicCompilationUltra --spawn-audit /private/cartridge.bin --audit-stage 0002 --report /private/spawns.json
```

The stage number is hexadecimal. The report contains positions, categories and exact/origin flags, never extracted sprite pixels. It is a static catalogue check with an empty native snapshot; it does not prove completion of that stage.
