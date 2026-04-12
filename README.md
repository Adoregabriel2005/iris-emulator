# Íris Emulator

<p align="center">
  <img src="src/resources/iris_logo.svg" width="220" alt="Íris Emulator Logo"/>
</p>

<p align="center">
  <b>Multi-system Atari emulator — Atari 2600 + Atari Lynx + Atari Jaguar</b><br>
  Built with C++17 · Qt 6 · SDL2 · OpenGL 3.3<br>
  Developed by <b>Gorigamia</b> &amp; <b>Aleister93MarkV</b> · Open Source · Constantly updated
</p>

<p align="center">
  <a href="https://github.com/Adoregabriel2005/iris-emulator/releases/tag/v1.18.1">
    <img src="https://img.shields.io/badge/release-v1.18.1-blue?style=flat-square"/>
  </a>
  <img src="https://img.shields.io/badge/platform-Windows%2010%2F11%2064--bit-lightgrey?style=flat-square"/>
  <img src="https://img.shields.io/badge/license-GPL--3.0-green?style=flat-square"/>
  <img src="https://img.shields.io/badge/Discord-RPC-5865F2?style=flat-square&logo=discord&logoColor=white"/>
  <img src="https://img.shields.io/badge/OpenGL-3.3_Core-red?style=flat-square"/>
</p>

---

## What is Íris?

Íris is an open source multi-system Atari emulator that runs **Atari 2600**, **Atari Lynx**, and **Atari Jaguar** games on modern Windows hardware. The interface is inspired by **PCSX2**, **DuckStation**, and **Dolphin Emulator** — clean, professional, and easy to use without sacrificing power user features.

The project is in **active development** and receives constant updates. New mappers, better game compatibility, Jaguar improvements, and quality of life features are always in progress.

> **ROMs and BIOS files are NOT included and cannot be provided.**
> You must own the original hardware to legally use ROM files.
> The Atari Lynx Boot ROM (`lynxboot.img`) must be obtained from your own Lynx hardware.

---

## Platform Support

| Platform | Status |
|---|---|
| Windows 10 64-bit | ✅ Supported |
| Windows 11 64-bit | ✅ Supported |
| Linux | ⚠️ Not officially supported yet |
| macOS | ❌ Not supported |

> **Linux contributors welcome!**
> The codebase uses Qt6, SDL2, OpenGL and CMake — all cross-platform. Pull requests are very welcome. The main things that would need attention are the build system paths, icon theme loading, and SDL audio device enumeration.

---

## Credits

This emulator would not exist without the work of these incredible people and projects.

### Developers
- **Gorigamia** — Lead developer, Atari 2600 core, UI, architecture
- **Aleister93MarkV** — Co-developer, testing, contributions

### Atari 2600 — Hardware History
The **Atari 2600** (originally Atari VCS) was developed by **Atari, Inc.**, under the leadership of engineer **Jay Miner** — who later went on to design the Amiga chipset. Released in 1977, it became one of the best-selling consoles of all time. The 2600 core was built from scratch, heavily informed by the **Stella** emulator team and the **AtariAge** community's decades of reverse engineering work on the TIA, RIOT, and 6507 chips.

### Atari Lynx — Hardware History
The **Atari Lynx** was originally developed by the software company **Epyx** in 1987, initially named *"Handy Game"*. The principal designers were **Dave Needle** and **R.J. Mical**, known for their work on the Amiga computer. Due to financial difficulties at Epyx, **Atari Corp.** took over production and launched the Lynx in 1989 — making it the world's first color handheld console. The Lynx core is powered by **[Gearlynx](https://github.com/drhelius/Gearlynx)** by **Ignacio Sanchez (drhelius)** — without Gearlynx, Lynx support in Íris would not be possible.

### Atari Jaguar — Hardware History
The **Atari Jaguar** was developed primarily by **Flare Technology** (Martin Brennan and John Mathieson) in partnership with Atari Corp. Released in 1993, it was marketed as the first 64-bit home console. The Jaguar core is powered by the **Virtual Jaguar** engine by Shamus / James L. Hammons.

### Third-Party Engines & Libraries
- **[Gearlynx](https://github.com/drhelius/Gearlynx)** by Ignacio Sanchez (drhelius) — Atari Lynx core (GPL-3.0)
- **Virtual Jaguar** by Shamus / James L. Hammons — Atari Jaguar core (GPL-3.0)
- **Stella** team & **AtariAge** community — 2600 documentation
- **Qt 6** — UI framework
- **SDL2** — Audio & input
- **VST3 SDK** by Steinberg — Audio plugin support
- **discord-rpc** — Discord Rich Presence

### Interface Inspiration
- **PCSX2** — Overall layout, settings dialog, game list, setup wizard style
- **DuckStation** — Toolbar style, overlay menu
- **Dolphin Emulator** — Save state slots, cover art grid

---

## Features

### Atari 2600
- Cycle-accurate 6507 CPU (all illegal opcodes)
- TIA chip at color-clock level — 15 collisions, HMOVE, WSYNC, VDEL, NUSIZ
- 2-channel audio with 16 waveforms
- RIOT 6532 (timers + I/O)
- NTSC / PAL / SECAM support
- Mappers: None (≤4KB), F8 (8KB), F6 (16KB), F4 (32KB), E0 (Parker Bros), F8SC/F6SC/F4SC (Superchip), 3F, 3E, FA, FE

### Atari Lynx (powered by Gearlynx)
- WDC 65C02 CPU @ 4 MHz
- Mikey chip — 8 timers, 4-channel stereo audio, display DMA
- Suzy sprite engine with scaling
- Math coprocessor (multiply/divide)
- 160×102 4bpp display with palette
- LNX cartridge format + HLE boot (no BIOS required for homebrew)
- Boot ROM support (`lynxboot.img`) for commercial games

### Atari Jaguar (powered by Virtual Jaguar)
- Motorola 68000 @ 13.295 MHz (main CPU)
- Tom chip — GPU, blitter, object processor
- Jerry chip — DSP, audio, timers
- NTSC / PAL support
- Cartridge format `.j64` / `.jag`
- **RF Cable filter** — chroma bleed, animated dot crawl, luminance noise, vignette
- **S-Video filter** — sharp luma, soft chroma (Y/C separation), CRT scanlines, saturation boost
- **Upscaling** — Native / 480p / 720p / 1080p
- **Aspect ratio** — Auto / 4:3 / 16:9

### Audio — VST3 Plugin Chain
- Real-time VST3 plugin processing on emulator audio output
- Enable/disable chain toggle (bypass without removing plugins)
- Double-click plugin to open its native graphical editor
- Drag to reorder plugins in the chain
- Master gain slider (0–200%)
- Persisted chain (saved/loaded with settings)

### Interface
- Game list with cover art (table + grid view)
- Built-in cover art downloader (TheGamesDB)
- **Setup wizard** on first launch (PCSX2-style, multi-page with sidebar)
- **Returning user greeting** with "don't show again" option
- **About dialog** with Credits, License (GPL-3.0) and hardware history tabs
- CRT Scanlines, CRT Full, LCD Grid, LCD Ghosting, RF Cable, S-Video filters
- Dark theme (inspired by PCSX2)
- Fully remappable controls — keyboard and gamepad
- **10 save state slots** — slot 1 = quick save (F5/F7), slots 2–10 = manual
- Save state folders auto-created on first launch (`savestates/quick/` + `savestates/manual/`)
- Drag & drop ROM loading
- Fullscreen support
- Pause overlay menu
- Discord Rich Presence (game name, console, time playing, status)
- Debug window (F12)

---

## Game Compatibility

### Compatibility Summary

#### Atari 2600

| Category | Count | Percentage |
|---|---|---|
| ✅ Fully playable — no issues | ~29 tested titles | ~65% of tested library |
| ⚠️ Opens but has graphical/timing bugs | ~4 tested titles | ~9% of tested library |
| ❌ Does not open / crashes | ~12 titles (missing mappers) | ~26% of tested library |
| 🏆 100% completable | Pitfall!, River Raid, Space Invaders, Pac-Man, Enduro, Adventure, Asteroids, Missile Command | ~18% of tested library |

> The 2600 library has ~900 commercial titles. The missing mapper list (3F, 3E, FE, FA, F0, AR, DPC, DPC+, UA, CV) blocks roughly 15–20% of the full commercial library.

#### Atari Lynx

| Category | Count | Percentage |
|---|---|---|
| ✅ Fully playable | ~13 tested titles | ~50% of tested library |
| ⚠️ Opens with graphical/sprite bugs | ~4 tested titles | ~15% of tested library |
| ❌ Does not open / black screen | ~9 titles | ~35% of tested library |
| 🏆 100% completable | California Games, Chip's Challenge, Gates of Zendocon | ~12% of tested library |

> The Lynx core is in **alpha (~35%)** and improving with every release. Only ~26 titles have been tested out of ~73 commercial releases.

#### Atari Jaguar

| Category | Count | Percentage |
|---|---|---|
| ✅ Fully playable | ~8 tested titles | ~40% of tested library |
| ⚠️ Opens with graphical/audio bugs | ~6 tested titles | ~30% of tested library |
| ❌ Does not open / crashes | ~6 titles | ~30% of tested library |
| 🏆 100% completable | Tempest 2000, Rayman (partial) | ~10% of tested library |

> The Jaguar core is in **alpha (~40%)**. The Jaguar library has ~67 commercial titles. GPU/blitter accuracy and CD-ROM support are the main remaining challenges.

---

### Atari 2600 — Works perfectly ✅

| Game | Status |
|---|---|
| Pitfall! | ✅ Perfect |
| River Raid | ✅ Perfect |
| Space Invaders | ✅ Perfect |
| Pac-Man | ✅ Perfect |
| Enduro | ✅ Perfect |
| Frostbite | ✅ Perfect |
| Atlantis | ✅ Perfect |
| Demon Attack | ✅ Perfect |
| Megamania | ✅ Perfect |
| Keystone Kapers | ✅ Perfect |
| Berzerk | ✅ Perfect |
| Adventure | ✅ Perfect |
| Combat | ✅ Perfect |
| Asteroids | ✅ Perfect |
| Missile Command | ✅ Perfect |
| Centipede | ✅ Perfect |
| Freeway | ✅ Perfect |
| Donkey Kong | ✅ Perfect |
| Frogger | ✅ Perfect |
| H.E.R.O. | ✅ Perfect |
| Solaris | ✅ Perfect |
| Jr. Pac-Man | ✅ Perfect |
| Crystal Castles | ✅ Perfect |
| Montezuma's Revenge | ✅ Perfect |
| Q*bert | ✅ Perfect |
| Popeye | ✅ Perfect |
| Spider-Man | ✅ Perfect |
| Star Wars: The Empire Strikes Back | ✅ Perfect |
| The Activision Decathlon | ✅ Perfect |

### Atari 2600 — Has issues ⚠️

| Game | Issue |
|---|---|
| Cosmic Ark | Stars don't appear (timing edge case) |
| Yars' Revenge | Shield flicker more intense than hardware |
| BurgerTime | E7 mapper simplified — some graphics glitch |
| Masters of the Universe | Minor visual artifacts |

### Atari 2600 — Not working ❌

| Game | Reason |
|---|---|
| Pitfall II | DPC mapper not implemented |
| Miner 2049er | 3F mapper not implemented |
| Robot Tank | FA mapper not implemented |
| Supercharger games | AR mapper not implemented |
| Kaboom! | Paddle controller not implemented |
| Night Driver | Paddle controller not implemented |
| Indy 500 | Driving controller not implemented |
| Decathlon (paddle) | Paddle controller not implemented |

**Missing mappers:** 3F, 3E, FE, FA, F0, AR, DPC, DPC+, UA, CV

---

### Atari Lynx — Works well ✅

| Game | Status |
|---|---|
| California Games | ✅ Playable |
| Chip's Challenge | ✅ Playable |
| Gates of Zendocon | ✅ Playable |
| Xenophobe | ✅ Playable |
| Batman Returns | ✅ Playable |
| Lemmings | ✅ Playable |
| Toki | ✅ Playable |
| Alien vs Predator (Prototype) | ✅ Playable |
| Awesome Golf | ✅ Playable |
| Double Dragon (Telegames) | ✅ Playable |
| Paperboy | ✅ Playable |
| RoadBlasters | ✅ Playable |
| Simple homebrew titles | ✅ Playable |

### Atari Lynx — Has issues ⚠️

| Game | Issue |
|---|---|
| Gauntlet: The Third Encounter | Heavy graphical bugs, mostly unplayable |
| Games with heavy sprite overlap | Collision detection incomplete |
| Games using ComLynx | Multiplayer not implemented |
| Games with sprite tilt/stretch | Advanced sprite transforms missing |

> **Note:** Only a portion of the Lynx library has been tested. If a game not listed here runs well or has issues, feel free to open an issue or pull request.

### Atari Lynx — Not working ❌

| Game | Reason |
|---|---|
| Games requiring CPUSLEEP | Not implemented |
| Games using shadow/XOR/boundary sprites | Advanced sprite types missing |

---

### Atari Jaguar — Works well ✅

| Game | Status |
|---|---|
| Tempest 2000 | ✅ Fully playable |
| Doom | ✅ Fully playable |
| Wolfenstein 3D | ✅ Fully playable |
| Alien vs Predator | ✅ Fully playable |
| Rayman | ✅ Mostly playable (minor sprite glitches) |
| Checkered Flag | ✅ Fully playable |
| Cybermorph | ✅ Fully playable |
| Club Drive | ✅ Fully playable |

### Atari Jaguar — Has issues ⚠️

| Game | Issue |
|---|---|
| Kasumi Ninja | Graphical corruption in some stages |
| Brutal Sports Football | Audio desync |
| Val d'Isère Skiing | Geometry glitches |
| Iron Soldier | Occasional frame drops |
| Zool 2 | Sprite priority errors |
| Bubsy in Fractured Furry Tales | Minor graphical artifacts |

### Atari Jaguar — Not working ❌

| Game | Reason |
|---|---|
| All Jaguar CD titles | CD-ROM not implemented |
| Battlemorph | CD-ROM not implemented |
| Blue Lightning (CD) | CD-ROM not implemented |
| Games requiring heavy DSP tricks | Jerry DSP accuracy incomplete |

---

## Hardware Requirements

Rendering uses **OpenGL 3.3 Core Profile** (GPU-accelerated shaders for all filters).
Emulation is **single-threaded** — 1 core for emulation + 1 for SDL audio.

> **OpenGL 3.3 is required.** Intel GMA 950 and older integrated graphics do not support OpenGL 3.3 and will not run the emulator at all.

### Atari 2600 — Minimum (60 FPS stable)

| Component | Minimum | Recommended |
|---|---|---|
| **CPU (Intel)** | Core 2 Duo E6300 @ 1.86 GHz (2006) | Core i3-2100 @ 3.1 GHz (2011) |
| **CPU (AMD)** | Athlon 64 X2 3800+ @ 2.0 GHz (2006) | FX-6300 @ 3.5 GHz (2012) |
| **RAM** | 2 GB DDR2-667 | 4 GB DDR3-1333 |
| **GPU (dedicated)** | NVIDIA GeForce 8400 GS (2007) | NVIDIA GTX 550 Ti (2011) |
| **GPU (dedicated AMD)** | AMD Radeon HD 2400 (2007) | AMD Radeon HD 6790 (2011) |
| **GPU (integrated)** | Intel GMA X4500 — G45 chipset (2008) | Intel HD Graphics 2000 — Sandy Bridge (2011) |
| **Storage** | 80 MB | 200 MB (with covers) |
| **OS** | Windows 10 64-bit | Windows 10 / 11 64-bit |
| **Display** | 1024×768 | 1280×720 or higher |
| **OpenGL** | 3.3 Core Profile | 3.3 Core Profile |

### Atari Lynx — Minimum (75 FPS stable)

| Component | Minimum | Recommended |
|---|---|---|
| **CPU (Intel)** | Core 2 Duo E8400 @ 3.0 GHz (2008) | Core i3-2100 @ 3.1 GHz (2011) |
| **CPU (AMD)** | Phenom II X2 550 @ 3.1 GHz (2009) | FX-6300 @ 3.5 GHz (2012) |
| **RAM** | 2 GB DDR2-800 | 4 GB DDR3-1333 |
| **GPU (dedicated)** | NVIDIA GeForce 8400 GS (2007) | NVIDIA GTX 550 Ti (2011) |
| **GPU (dedicated AMD)** | AMD Radeon HD 2400 (2007) | AMD Radeon HD 6790 (2011) |
| **GPU (integrated)** | Intel GMA X4500 — G45 chipset (2008) | Intel HD Graphics 2000 — Sandy Bridge (2011) |

### Atari Jaguar — Minimum (50+ FPS)

The Jaguar is **significantly more demanding** than the 2600 and Lynx. It emulates three processors simultaneously (68000 + Tom + Jerry) and the RF/S-Video filters run as real GPU shaders every frame.

| Component | Minimum | Recommended |
|---|---|---|
| **CPU (Intel)** | Core i3-2100 @ 3.1 GHz — Sandy Bridge (2011) | Core i5-3470 @ 3.2 GHz — Ivy Bridge (2012) |
| **CPU (AMD)** | FX-4300 @ 3.8 GHz (2012) | FX-8350 @ 4.0 GHz (2012) |
| **RAM** | 4 GB DDR3-1333 | 8 GB DDR3-1600 |
| **GPU (dedicated)** | NVIDIA GeForce GTX 460 (2010) | NVIDIA GTX 760 (2013) |
| **GPU (dedicated AMD)** | AMD Radeon HD 6850 (2010) | AMD Radeon R9 270 (2013) |
| **GPU (integrated)** | Intel HD Graphics 4000 — Ivy Bridge (2012) | Intel HD Graphics 4600 — Haswell (2013) |
| **OpenGL** | 3.3 Core Profile | 3.3 Core Profile |

> **Why does Jaguar need a GTX 460 / HD 6850 minimum?**
> The RF Cable and S-Video filters are real GLSL fragment shaders running on the GPU at 60 Hz. Combined with 1080p upscaling, the GPU needs to sustain a full-screen shader pass every frame. The GTX 460 and HD 6850 are the oldest cards tested that do this without dropping frames.
> Intel HD Graphics 3000 (Sandy Bridge) may work at native resolution with no filter, but will drop frames with RF/S-Video or at 720p+ upscaling.

### Maximum tested (no bottleneck at any setting)

| Component | Value |
|---|---|
| **CPU** | Intel Core i9-13900K / AMD Ryzen 9 7950X |
| **GPU** | NVIDIA RTX 4090 / AMD RX 7900 XTX |
| **RAM** | 64 GB DDR5 |

> At recommended or higher hardware, all three consoles run at full speed with all filters enabled simultaneously — RF, S-Video, 1080p upscaling and VST3 plugin chains.

---

## Default Controls

| Action | 2600 Keyboard | 2600 Gamepad | Lynx Keyboard | Lynx Gamepad | Jaguar Keyboard | Jaguar Gamepad |
|---|---|---|---|---|---|---|
| Directions | Arrow keys | D-pad / Left stick | Arrow keys | D-pad / Left stick | Arrow keys | D-pad / Left stick |
| Button A / Fire | Z | A | Z | A | Z | A |
| Button B | — | — | X | B | X | B |
| Button C | — | — | — | — | C | X |
| L Shoulder | — | — | A | LB | — | — |
| R Shoulder | — | — | S | RB | — | — |
| Option | — | — | — | — | A | LB |
| Select | Tab | Back | — | — | — | — |
| Reset / Start / Pause | Backspace | Start | Enter | Start | Enter | Start |
| Quick Save | F5 | — | F5 | — | F5 | — |
| Quick Load | F7 | — | F7 | — | F7 | — |
| Pause overlay | Escape | — | Escape | — | Escape | — |

All bindings are remappable in Settings → Controls.

---

## Running (Windows)

1. Download the latest release from the [Releases page](https://github.com/Adoregabriel2005/iris-emulator/releases)
2. Extract the zip to any folder
3. Run `irisemulator.exe`
4. The setup wizard will guide you through adding a ROM folder
5. Double-click any game to play

**Requirements:**
- Windows 10 or 11 64-bit
- GPU with OpenGL 3.3 Core Profile (GeForce 8 series / Radeon HD 2000 / Intel HD 2000 or newer for 2600+Lynx — GTX 460 / HD 6850 / Intel HD 4000 or newer for Jaguar)
- [Visual C++ Redistributable 2022 x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) ← install this if the emulator doesn't open
- Discord desktop app (optional, for Rich Presence)

**Atari Lynx BIOS:** Place `lynxboot.img` next to the `.exe` or configure the path in Settings → Lynx Controls → Boot ROM file. ROMs with the HLE magic byte will boot without a BIOS.

---

## Build from Source

### Requirements
- Windows 10/11 64-bit
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ workload
- [Qt 6.8+](https://www.qt.io/download) (Widgets, Svg, Network, OpenGL, OpenGLWidgets)
- [SDL2](https://github.com/libsdl-org/SDL/releases)
- [vcpkg](https://github.com/microsoft/vcpkg) with `discord-rpc:x64-windows`
- CMake ≥ 3.16

### Steps

```powershell
git clone https://github.com/Adoregabriel2005/iris-emulator.git
cd iris-emulator

cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release

# Output: build/Release/irisemulator.exe
```

---

## License

Íris Emulator is open source software licensed under the **GNU General Public License v3.0**.
See [LICENSE](LICENSE) for details.

The Gearlynx engine is copyright © 2025 Ignacio Sanchez, licensed under GPL-3.0.
The Virtual Jaguar engine is copyright © its respective authors, licensed under GPL-3.0.

---

## About

Developed by **Gorigamia** & **Aleister93MarkV** · Inspired by PCSX2, DuckStation, and Dolphin Emulator
