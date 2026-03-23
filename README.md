# Geomys

A [Gopher protocol](https://en.wikipedia.org/wiki/Gopher_(protocol)) browser for classic 68000 Macintosh systems, targeting the Macintosh Plus. Implements [RFC 1436](https://datatracker.ietf.org/doc/html/rfc1436) (Gopher) and [RFC 4266](https://datatracker.ietf.org/doc/html/rfc4266) (Gopher URI scheme) with a full Macintosh GUI for System 6 and System 7. Cross-compiled on Linux using [Retro68](https://github.com/autc04/Retro68).

This project is 100% vibe coded using [Claude Code](https://docs.anthropic.com/en/docs/claude-code).

<p align="center">
<a href="#features">Features</a> · <a href="#requirements">Requirements</a> · <a href="#building">Building</a> · <a href="#testing">Testing</a> · <a href="#acknowledgments">Acknowledgments</a> · <a href="#license">License</a>
</p>

## Requirements

- Macintosh Plus or later (4MB RAM, 68000 CPU)
- System 6.0.8 or System 7 with MacTCP
- Network connection (Ethernet or compatible)
- Color themes require Mac II or later with Color QuickDraw (monochrome themes work on all systems)

## Features

**Gopher Protocol**
- [RFC 1436](https://datatracker.ietf.org/doc/html/rfc1436) (Gopher) and [RFC 4266](https://datatracker.ietf.org/doc/html/rfc4266) (Gopher URI scheme)
- All canonical types (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, g, I, T)
- Non-canonical types (d, h, i, p, r, s)
- [Gopher+](https://en.wikipedia.org/wiki/Gopher%2B) protocol support (`+ADMIN`, `+VIEWS` parsing)
- Default home page: `gopher://sdf.org` (user-changeable)

**User Interface**
- Full Macintosh GUI with mouse support
- Multi-window browsing: up to 4 simultaneous windows with background loading and Notification Manager alerts
- Web browser-style experience: address bar, back/forward/refresh/home buttons
- Window menu for switching between open windows
- Page styles: Traditional, Plain, Markdown
- Show Details toggle: show/hide server metadata (dates, sizes) in directory listings
- Font selection: Chicago 12, Monaco 9/12, Courier 10, Geneva 9/10
- Save Page As (Cmd-S): save current page as TeachText-readable TEXT file
- Text selection and copy: highlight text in content area or address bar, copy to clipboard (Cmd-C)
- 9 built-in themes: Light, Dark, Solarized Light/Dark, Tokyo Night Light/Dark, Green Screen, Classic, Platinum
- 256-color support on Mac II and later (Color QuickDraw); monochrome themes on Mac Plus
- Themed chrome: nav bar, buttons, address bar, and status bar colored per theme
- Themed content colors by Gopher item type (text, links, search, errors, external)
- Color icon family (icl4, icl8, ics4, ics8) for Finder on color systems
- Horizontal scrollbar for wide content
- Find in Page (Cmd-F) with Find Again (Cmd-G): case-insensitive text search with match highlighting
- Keyboard navigation: arrow keys, Page Up/Down, Home/End, Cmd-L (address bar), Cmd-R (refresh)
- Hand cursor on navigable items
- Bottom status bar with connection info
- Menus: File, Edit, Favorites, Window, Options
- Movable modal dialogs on System 7 (fixed position on System 6)
- Aligned with Apple Human Interface Guidelines (1992)

**Networking**
- MacTCP for TCP/IP connectivity
- Built-in DNS resolver
- Local page cache for instant back/forward with scroll position preservation
- Per-window history, scroll position, and text selection
- Dirty-row tracking and shadow buffer for optimized redraws
- GWorld color offscreen buffer for flicker-free color rendering
- Background session throttling for idle windows
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
./scripts/build.sh                    # full preset (4 windows)
./scripts/build.sh --preset lite      # lite preset (2 windows)
./scripts/build.sh --preset minimal   # minimal preset (1 window)
./scripts/build.sh --max-windows 2    # override window count
```

Output: `build/Geomys.dsk` (800K floppy image) and `build/Geomys.bin` (BinHex).

| Preset | Windows | Features | SIZE (preferred) |
|--------|---------|----------|-----------------|
| Full (default) | 4 | All features + color themes | ~1024 KB |
| Lite | 2 | Core + offscreen + favorites + clipboard + mono themes | ~505 KB |
| Minimal | 1 | Core only | ~297 KB |

See [docs/BUILD.md](docs/BUILD.md) for feature flags and detailed instructions.

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
