# Geomys

A [Gopher protocol](https://en.wikipedia.org/wiki/Gopher_(protocol)) browser for classic 68000 Macintosh systems, targeting the Macintosh Plus. Implements [RFC 1436](https://datatracker.ietf.org/doc/html/rfc1436) (Gopher) and [RFC 4266](https://datatracker.ietf.org/doc/html/rfc4266) (Gopher URI scheme) with a full Macintosh GUI for System 6. Cross-compiled on Linux using [Retro68](https://github.com/autc04/Retro68).

This project is 100% vibe coded using [Claude Code](https://docs.anthropic.com/en/docs/claude-code).

<p align="center">
<a href="#features">Features</a> · <a href="#requirements">Requirements</a> · <a href="#building">Building</a> · <a href="#testing">Testing</a> · <a href="#acknowledgments">Acknowledgments</a> · <a href="#license">License</a>
</p>

## Requirements

- Macintosh Plus or later (4MB RAM, 68000 CPU)
- System 6.0.8 with MacTCP
- Network connection (Ethernet or compatible)

## Features

**Gopher Protocol**
- [RFC 1436](https://datatracker.ietf.org/doc/html/rfc1436) (Gopher) and [RFC 4266](https://datatracker.ietf.org/doc/html/rfc4266) (Gopher URI scheme)
- All canonical types (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, g, I, T)
- Non-canonical types (d, h, i, p, r, s)
- [Gopher+](https://en.wikipedia.org/wiki/Gopher%2B) protocol support (`+ADMIN`, `+VIEWS` parsing)
- Default home page: `gopher://sdf.org` (user-changeable)

**User Interface**
- Full Macintosh GUI with mouse support
- Web browser-style experience: address bar, back/forward/refresh/home buttons
- Page styles: Traditional, Plain, Markdown
- Show Details toggle: show/hide server metadata (dates, sizes) in directory listings
- Font selection: Chicago 12, Monaco 9/12, Courier 10, Geneva 9/10
- Hand cursor on navigable items
- Bottom status bar with connection info
- Monochrome display
- Menus: File, Edit, Favorites, Options
- Aligned with Apple Human Interface Guidelines (1992)

**Networking**
- MacTCP for TCP/IP connectivity
- Built-in DNS resolver
- Local page cache (3-page LRU) for instant back/forward
- Works well within 4MB of RAM

**Character Support**
- CP437 character set translation (box-drawing, special characters)
- Unicode glyph rendering

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

Output: `build/Geomys.dsk` (800K floppy image) and `build/Geomys.bin` (BinHex).

See [docs/BUILD.md](docs/BUILD.md) for build presets, feature flags, and detailed instructions.

## Testing

Uses [Snow](https://snowemu.com/) emulator with a Mac Plus ROM and System 6.0.8 SCSI hard drive image. Snow supports DaynaPORT SCSI/Link Ethernet emulation for MacTCP networking.

## Acknowledgments

- **[wallops](https://github.com/jcs/wallops)** by joshua stein — MacTCP wrapper (`tcp.c`/`tcp.h`), DNS resolution (`dns.c`/`dns.h`), and utility functions. ISC license.
- **[subtext](https://github.com/jcs/subtext)** by joshua stein — Additional utility and networking code. ISC license.
- **[Flynn](https://github.com/ecliptik/flynn)** — Telnet client for classic Macintosh, sibling project and architectural reference. ISC license.
- **University of Illinois Board of Trustees** — TCP networking code (`tcp.c`, 1990-1992)
- **Apple Computer, Inc.** — MacTCP header definitions (`MacTCP.h`, 1984-1995)
- **Todd C. Miller and The Regents of the University of California** — Utility functions
- **[Retro68](https://github.com/autc04/Retro68)** by Wolfgang Thaller — Cross-compilation toolchain for 68k Macintosh
- **[Snow](https://snowemu.com/)** — Macintosh emulator used for development and testing
- **[Claude Code](https://claude.ai/code)** by [Anthropic](https://www.anthropic.com/)

## License

ISC License. See [LICENSE](LICENSE) for full details.
