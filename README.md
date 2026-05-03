# M88M - PC-8801 Series Emulator

> [!WARNING]
> **Work In Progress**: This project is currently under active development. Features and stability may vary.

M88M is a modern, cross-platform port of the classic PC-8801 emulator **M88**, originally developed by **cisc**. 

While the original M88 was tightly coupled with the Win32 API and DirectX, M88M leverages **raylib** and **raygui** for its frontend, making it natively compatible with **macOS (Intel/Apple Silicon)**, **Linux**, and **Windows** via a single CMake-based build system.

## Key Features

- **Cross-Platform:** Native support for macOS, Linux, and Windows.
- **Raylib Frontend:** Modern, lightweight hardware-accelerated rendering and audio.
- **Core Integrity:** Retains the highly accurate emulation core of the original M88 while replacing the platform-dependent layers.
- **Modern Build System:** Uses CMake for easy compilation with modern compilers (Clang, GCC, MSVC).
- **Dual-Threaded Architecture:** Maintains the original high-performance design with separate threads for emulation and UI/rendering.
- **Enhanced UI:** Includes a built-in overlay for disk management and system configuration.

## Status

M88M is currently in a "Playable Prototype" stage. The core emulation (Z80, OPNA/PSG, CRTC, FDC) is fully functional. 

**Working Features:**
- N88-BASIC (V1/V2) and compatible modes.
- Soundboard II (OPNA) emulation (FM, PSG, Rhythm, ADPCM).
- D88 disk image support (Mount/Unmount).
- Keyboard matrix emulation.
- Modern window scaling and aspect ratio maintenance.

## Prerequisites

To run the emulator, you must provide the necessary PC-8801 ROM files. Place the following files in the same directory as the executable or in a folder named `roms`:

- `N88.ROM` (or `N88.ROM` + `N88_0.ROM`, etc.)
- `DISK.ROM`
- `FONT.ROM`
- (Optional) `KANJI1.ROM`, `KANJI2.ROM`

*Note: You must own the original hardware to legally use these ROM files.*

## Building

### macOS / Linux
#### Dependencies (Linux)
On Debian-based systems (Ubuntu, etc.), you will need the following packages:
```bash
sudo apt-get install build-essential cmake libasound2-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev libgl1-mesa-dev libgtk-3-dev
```

#### Build
```bash
# Clone the repository
git clone <repository-url>
cd M88M

# Configure and build
cmake -S . -B build
cmake --build build -j
```
The executable will be generated at `./build/m88m`.

### Windows (CMake)
You can use CMake with Visual Studio or MinGW.
```cmd
cmake -S . -B build
cmake --build build --config Release
```

## Usage

- **F1 / F2:** Open file dialog to mount disk to Drive 1 or Drive 2.
- **F10 / Right Click:** Toggle the Main Menu / Settings overlay.
- **F12:** Reset the emulator.
- **Drag & Drop:** Drop a `.d88` file onto the window to mount it.

## License

- The original **M88** core is copyright (C) **cisc**. Please refer to `docs/README.md` for the original license terms.
- New code, porting layers, and Raylib integration are provided under the **2-Clause BSD License**.
- `c86ctl.h` is provided under the **2-Clause BSD License**.

---

*This project is a fan-made port and is not affiliated with the original author cisc.*
