# Building Geomys

## Prerequisites

- Linux host (cross-compilation)
- [Retro68](https://github.com/autc04/Retro68) toolchain built from source (68k only)
- CMake 3.1+
- A working C compiler for the host (GCC or Clang)

## Building the Retro68 Toolchain

Clone and build Retro68 (68k target only — no PowerPC or Carbon):

```bash
git clone https://github.com/autc04/Retro68.git
cd Retro68 && git submodule update --init && cd ..
mkdir Retro68-build && cd Retro68-build
bash ../Retro68/build-toolchain.bash --no-ppc --no-carbon --prefix=$(pwd)/toolchain
```

The toolchain is expected at `Retro68-build/toolchain/` relative to the repository root.

## Building Geomys

```bash
./scripts/build.sh
```

This produces:

- `build/Geomys.dsk` — 800K floppy disk image
- `build/Geomys.bin` — BinHex encoded binary

The full path of created disk images is printed on completion.

## Build Presets

Geomys supports build presets for different configurations:

| Preset | Description |
|--------|-------------|
| `full` | All features enabled (default) |
| `lite` | Recommended for Mac Plus — core browsing features |
| `minimal` | Bare-bones — smallest possible binary |

Select a preset:

```bash
./scripts/build.sh --preset minimal
./scripts/build.sh --preset lite
./scripts/build.sh --preset full
```

## Feature Flags

The build system uses CMake feature flags to enable or disable components at compile time:

| Flag | Description | Default |
|------|-------------|---------|
| `GEOMYS_OFFSCREEN` | Double-buffered rendering | OFF |
| `GEOMYS_STATUS_BAR` | Bottom status bar | ON |
| `GEOMYS_FAVORITES` | Favorites system | OFF |
| `GEOMYS_COLOR` | 256-color support (future) | OFF |
| `GEOMYS_DOWNLOAD` | File download support (future) | OFF |

## Compiler Flags

The build targets Motorola 68000 with size optimization:

```
-m68000 -Os -ffunction-sections -fdata-sections -Wl,-gc-sections
```

- `-m68000` — Target 68000 CPU (Mac Plus compatible, no 68020+ instructions)
- `-Os` — Optimize for size
- `-ffunction-sections -fdata-sections -Wl,-gc-sections` — Dead code elimination

## Clean Build

```bash
rm -rf build/
./scripts/build.sh
```

## Disk Image Output

The build produces an 800K `.dsk` floppy image suitable for use with emulators (Snow, Mini vMac) or writing to physical floppy disks. The `.bin` (BinHex) format can be transferred via serial or network to a real Macintosh.
