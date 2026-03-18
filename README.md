# Geomys

A Gopher browser for classic 68000 Macintosh systems, targeting the Macintosh Plus. Implements RFC 1436 (Gopher) and RFC 4266 (Gopher URI scheme) with a full Macintosh GUI for System 6. Cross-compiled on Linux using [Retro68](https://github.com/autc04/Retro68).

This project is 100% vibe coded using [Claude Code](https://docs.anthropic.com/en/docs/claude-code).

<p align="center">
<a href="#features">Features</a> · <a href="#requirements">Requirements</a> · <a href="#building">Building</a> · <a href="#testing">Testing</a> · <a href="#acknowledgments">Acknowledgments</a> · <a href="#license">License</a>
</p>

## Requirements

- Macintosh Plus or later (4MB RAM, 68000 CPU)
- System 6.0.8 with MacTCP
- MacTCP for networking

## Features

**Gopher Protocol**
- RFC 1436 (Gopher) and RFC 4266 (Gopher URI scheme)
- All canonical types (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, g, I, T)
- Non-canonical types (d, h, i, p, r, s)

**User Interface**
- Full Macintosh GUI with mouse support
- Web browser-style experience: address bar, back/forward/refresh/home buttons
- Bottom status bar
- Monochrome display
- Menus: File, Edit, Favorites, Options
- Aligned with Apple Human Interface Guidelines

**Networking**
- MacTCP support
- Works well within 4MB of RAM

## Building

Requires the [Retro68](https://github.com/autc04/Retro68) cross-compilation toolchain. Build it from source (68k only):

```bash
git clone https://github.com/autc04/Retro68.git
cd Retro68 && git submodule update --init && cd ..
mkdir Retro68-build && cd Retro68-build
bash ../Retro68/build-toolchain.bash --no-ppc --no-carbon --prefix=$(pwd)/toolchain
```

Then build Geomys:

```bash
./scripts/build.sh
```

## Testing

Uses [Snow](https://snowemu.com/) emulator with a Mac Plus ROM and System 6.0.8 SCSI hard drive image. Snow supports DaynaPORT SCSI/Link Ethernet emulation for MacTCP networking.

## Acknowledgments

- **[Claude Code](https://claude.ai/code)** by [Anthropic](https://www.anthropic.com/)
- **[Retro68](https://github.com/autc04/Retro68)** by Wolfgang Thaller
- **[Snow](https://snowemu.com/)** emulator
- **[Flynn](https://github.com/ecliptik/flynn)** — Telnet client for classic Macintosh, sibling project and architectural reference

## License

ISC License. See [LICENSE](LICENSE) for full details.
