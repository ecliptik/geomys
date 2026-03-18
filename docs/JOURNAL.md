# Development Journal

## 2026-03-18 — Phase 1: Project Scaffolding

Started Geomys, a Gopher protocol browser for classic Macintosh Plus.

Adapted the project structure and build system from the sibling project [Flynn](https://github.com/ecliptik/flynn) (telnet/finger client). Flynn's infrastructure — MacTCP wrapper, DNS resolver, utility functions, build scripts, and Rez resource patterns — provided a solid foundation. Roughly 60% of the low-level infrastructure was copied verbatim; the rest was adapted for Gopher's simpler stateless request-response model.

Phase 1 deliverables:
- Directory structure: `src/`, `include/`, `resources/`, `scripts/`, `docs/`
- CMakeLists.txt with feature flags for compile-time configuration
- `scripts/build.sh` adapted from Flynn with Geomys presets
- Rez resources: menu bar (Apple/File/Edit/Favorites/Options), About dialog, SIZE, BNDL, FREF, ICN#
- `main.c` with Toolbox initialization and event loop
- `menus.c` with menu setup and dispatch
- `dialogs.c` with About Geomys dialog (version, build SHA, machine name)
- Utility modules from Flynn: `macutil.c`, `sysutil.c`, `MacTCP.h`
- Documentation: README, CHANGELOG, BUILD, JOURNAL, ROADMAP, About Geomys

Key decisions:
- **Creator code:** `GEOM`
- **Default home page:** `gopher://sdf.org` (user-changeable in Options)
- **C standard:** C89/C90 throughout (matches era, native to Retro68)
- **Memory budget:** ~270KB of 384KB partition, ~30% headroom
- **License:** ISC (same as Flynn)

Next: Phase 2 — Networking & Gopher Protocol Core (TCP, DNS, RFC 1436 parser).
