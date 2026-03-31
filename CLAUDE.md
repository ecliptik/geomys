# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Geomys is a Gopher browser for classic Macintosh (68000/Macintosh Plus) written in C, targeting System 6.0.8 with MacTCP. Implements RFC 1436 (Gopher) and RFC 4266 (Gopher URI scheme).

## Target Platform Constraints

- Motorola 68000 CPU only (no PowerPC, no 68020+ instructions)
- 4 MiB RAM on Macintosh Plus
- System 6.0.8 primary target
- MacTCP for networking
- Monochrome only for MVP
- Latency and responsiveness are critical priorities

### Multi-Window Memory Constraints (System 7 Color)

On System 7 with Color QuickDraw, a shared 8-bit GWorld offscreen buffer (~300KB at 640x480) is used for flicker-free rendering. Combined with per-page item arrays (298 bytes × item count with Gopher+), memory is the primary constraint for multi-window support:

- **System 6 (monochrome)**: 1-bit offscreen (~22KB). Memory is rarely an issue.
- **System 7 (color)**: 8-bit shared GWorld (~300KB) + per-window item arrays.
- **GopherItem size**: ~298 bytes each (display[100] + selector[128] + host[64] + Gopher+ fields). A large directory (1500+ items) uses ~435KB.
- **Practical window limits**: 2-3 windows with large directories on 4MB. Item array growth (128→256→512→1024→2000) fails silently when heap is exhausted — second/third windows may show fewer items than the first.
- Allocation failures show as "Connection failed" or truncated item lists — the item array `NewPtr` returns NULL and stops adding items at the current capacity boundary (256, 512, 1024).

### Tuning for More Memory

The SIZE resource controls how much memory MultiFinder/System 7 gives Geomys. Default values by preset:

| Preset | Preferred | Minimum | Max Windows |
|--------|-----------|---------|-------------|
| Minimal | 512KB | 256KB | 1 |
| Lite | 1024KB | 768KB | 2 |
| Full | 2560KB | 1536KB | 3 |

On machines with more RAM (8MB, 32MB+), increase the SIZE resource clamp in `scripts/build.sh` (lines 216-221) to give Geomys a larger heap partition:

```bash
# Example: raise full preset to 8MB preferred / 4MB minimum
local max_pref=8192
local max_min=4096
```

Users can also adjust memory after building via Finder's "Get Info" on the Geomys application — change "Application Memory Size" to the desired value. This does not require rebuilding.

## Build System

- Cross-compile on Linux using [Retro68](https://github.com/autc04/Retro68) toolchain
- Toolchain built from source, installed at `Retro68-build/toolchain/` (in-repo, gitignored)
- Source cloned at `Retro68/` (in-repo, gitignored)
- Build: `./scripts/build.sh` (re-use build.sh and release.sh patterns from Flynn)
- CMake flag: `-m68000` for Mac Plus compatibility
- Target artifacts: 800K `.dsk` floppy images and `.hqx` (BinHex) compressed binaries
- Retro68 API quirks vs classic Toolbox: `qd.thePort` not `thePort`, `GetMenuHandle` not `GetMHandle`, `AppendResMenu` not `AddResMenu`, `LMGetApplLimit()` not `GetApplLimit`

### Debug Build Flag

`--debug` enables `GEOMYS_DEBUG` which adds keyboard shortcuts for QA automation:

| Shortcut | Action |
|----------|--------|
| Cmd+D | Toggle dark/light theme |
| Cmd+T | Cycle through all themes |
| Cmd+B | Toggle status bar visibility |
| Cmd+I | Toggle details panel |
| Cmd+K | Show/hide clipboard window |

**QA builds must always use `--debug`** — XTEST automation cannot reliably interact with hierarchical submenus in Snow/Basilisk. The debug shortcuts provide reliable keyboard-driven alternatives. Example: `./scripts/build.sh --preset minimal --debug`

## Testing

### Emulator: Snow

[Snow](https://snowemu.com/) emulator with Mac Plus ROM and System 6.0.8 SCSI hard drive image.

**IMPORTANT: Do NOT launch Snow, deploy to disk images, or run any automated testing unless the user explicitly asks. All testing and QA is done by the human. Only build and deploy when asked.**

- **Networking**: DaynaPORT SCSI/Link Ethernet emulation, NAT mode
- **Launch**: `DISPLAY=:0 tools/snow/snowemu diskimages/geomys.snoww &`
- Always kill Snow before doing a new build or adding to the Snow disk image

## Repository Conventions

- Git remote (primary): `ssh://git@forgejo.ecliptik.com/ecliptik/geomys.git`
- Codeberg mirror: `https://codeberg.org/ecliptik/geomys` (auto-mirrored from Forgejo)
- GitHub mirror: `https://github.com/ecliptik/geomys` (read-only, never push directly)
- Use feature branches for new features, squash commits when merging to main
- Never use git worktrees when working with agent teams
- Always include `Co-Authored-By: Claude Code` in commits (enforced by prepare-commit-msg hook)
- Always create a plan for implementing a feature and break into multiple phases
- Include short SHA in About Geomys for build identification
- Do NOT commit: disk images, GEOMYS.md, or other non-source artifacts
- Maintain: README.md, CHANGELOG.md
- Always print the full path of created disk images
- When taking screenshots of Snow, always crop to show only the full System 6 desktop

## Agent Teams

When creating a team, always include:
- **UI/UX reviewer** — reviews and offers guidance to keep Geomys aligned with Apple HIG
- **Macintosh 68000 and C expert** — reviews and offers guidance on performance, architecture, and feature implementation
- **Technical writer** — updates documentation as development progresses (CHANGELOG.md, README.md, docs/, About Geomys)

## Architecture

- Use C, optimize hot paths with assembly when needed
- Use native Macintosh toolkits (QuickDraw)
- Double buffering for smooth, flicker-free GUI
- Fully modularized build with feature flags (similar to Flynn)
- Code should be easy to understand and extend — break into modules, avoid large files
- Align with Apple Human Interface Guidelines (see references)
- Create About Geomys as TeachText document in docs/ (resource type `ttro`, reference Flynn's `docs/About Flynn`)
- When building the MVP, keep future features in mind to make them easier to implement later (keyboard nav, favorites, 256 color, themes, multi-window, System 7)

## UI Design

- Web browser-style experience: address bar, back/forward/refresh/home, status bar
- Menus: File, Edit, Favorites, Options
- Monochrome for MVP
- Think like a 1980s Macintosh UI/UX designer
- Reference: Apple Human Interface Guidelines 1992

## Reference Material

- Flynn (sibling project): `/home/claude/git/flynn` — follow similar patterns for build, docs, architecture
- Macintosh Human Interface Guidelines: `/home/claude/git/flynn/references/Human_Interface_Guidelines_1992.pdf`
- How to Write Macintosh Software: `/home/claude/git/flynn/references/How_To_Write_Macintosh_Software_1988.pdf`
- C Programming Techniques for Macintosh: `/home/claude/git/flynn/references/C_Programming_Techniques_for_the_Macintosh_1989.pdf`

## Gopher Protocol

### Canonical Types (MVP)
- 0: Text file
- 1: Directory (submenu)
- 2: CSO phone-book server
- 3: Error
- 4: BinHex encoded file
- 5: DOS binary archive
- 6: UUEncoded text file
- 7: Search query (full-text index)
- 8: Telnet session pointer
- 9: Binary file
- g: GIF image
- I: Generic image
- T: TN3270 telnet session

### Non-Canonical Types (MVP)
- d: Document (PDF/Word)
- h: HTML
- i: Informational message (display only)
- p: PNG image
- r: RTF document
- s: Sound file
