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

Geomys supports build presets for different configurations. The default preset is `full`.

| Preset | Windows | Memory | Description |
|--------|---------|--------|-------------|
| `full` | 3 | 2560 KB | All features enabled (default) |
| `lite` | 2 | 512 KB | Core browsing features, recommended for Mac Plus (`macplus` is an alias) |
| `minimal` | 1 | 512 KB | Bare-bones — smallest possible binary, single window |

Select a preset:

```bash
./scripts/build.sh                    # full (default)
./scripts/build.sh --preset minimal
./scripts/build.sh --preset lite
./scripts/build.sh --preset full
```

## Feature Flags

The build system uses CMake feature flags to enable or disable components at compile time. Each flag can be toggled independently via command-line options (see below), or set in bulk via a preset.

| Flag | Purpose | Default | Minimal | Lite | Full |
|------|---------|---------|---------|------|------|
| `GEOMYS_MAX_WINDOWS` | Max simultaneous windows | 3 | 1 | 2 | 3 |
| `GEOMYS_OFFSCREEN` | Offscreen double-buffer | ON | ON* | ON | ON |
| `GEOMYS_STATUS_BAR` | Status bar | ON | ON | ON | ON |
| `GEOMYS_FAVORITES` | Favorites system | ON | OFF | ON | ON |
| `GEOMYS_COLOR` | 256-color support | ON | OFF | OFF | ON |
| `GEOMYS_THEMES` | Theme system (9 themes) | ON | ON | ON | ON |
| `GEOMYS_DOWNLOAD` | File downloads and image save | ON | OFF | OFF | ON |
| `GEOMYS_GOPHER_PLUS` | Gopher+ protocol support | ON | OFF | OFF | ON |
| `GEOMYS_GLYPHS` | Unicode glyph rendering | ON | OFF | ON | ON |
| `GEOMYS_CP437` | CP437 character set | ON | OFF | ON | ON |
| `GEOMYS_STYLES` | Page format styles (text/icons) | ON | ON | ON | ON |
| `GEOMYS_CACHE` | Local page caching | ON | OFF | OFF | ON |
| `GEOMYS_CLIPBOARD` | Text selection and clipboard | ON | ON | ON | ON |
| `GEOMYS_HTML` | HTML content stripping | ON | ON | ON | ON |
| `GEOMYS_TELNET` | Telnet session info dialog | ON | ON | ON | ON |

\* Minimal sets `OFFSCREEN=OFF` but it is auto-enabled because `THEMES` requires it.

### Dependencies

- **CP437 requires GLYPHS** — enabling `--cp437` automatically enables `--glyphs` if it is not already on.
- **THEMES requires OFFSCREEN** — enabling `--themes` automatically enables `--offscreen` for flicker-free redraw.

## Command-Line Flags

Individual feature flags can be toggled on or off after a preset is applied. Use `--flag` to enable or `--no-flag` to disable:

| Enable | Disable | Feature |
|--------|---------|---------|
| `--offscreen` | `--no-offscreen` | Double-buffered rendering |
| `--statusbar` | `--no-statusbar` | Status bar |
| `--favorites` | `--no-favorites` | Favorites system |
| `--color` | `--no-color` | 256-color support |
| `--themes` | `--no-themes` | Theme system |
| `--download` | `--no-download` | File downloads and image save |
| `--gopher-plus` | `--no-gopher-plus` | Gopher+ protocol |
| `--glyphs` | `--no-glyphs` | Unicode glyph rendering |
| `--cp437` | `--no-cp437` | CP437 character set |
| `--styles` | `--no-styles` | Page format styles |
| `--cache` | `--no-cache` | Local page caching |
| `--clipboard` | `--no-clipboard` | Text selection and clipboard |
| `--html` | `--no-html` | HTML content stripping |
| `--telnet` | `--no-telnet` | Telnet session info dialog |

### Window Count

Use `--max-windows N` to set the maximum number of simultaneous windows (overrides preset):

```bash
./scripts/build.sh --max-windows 3                # custom window limit
./scripts/build.sh --preset lite --max-windows 4   # lite features, 4 windows
```

When `GEOMYS_MAX_WINDOWS` is 1, multi-window code compiles with zero overhead (no Window menu, no session switching). Cache slots scale automatically: 3 slots for 1 window, 4 for 2, 6 for 3+.

Flags are applied after presets, so you can start from a preset and override individual features:

```bash
# Start from lite, but add Gopher+ and downloads
./scripts/build.sh --preset lite --gopher-plus --download

# Full preset without caching
./scripts/build.sh --no-cache
```

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
