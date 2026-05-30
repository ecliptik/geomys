# UTF-8 Support — Implementation Plan

Status: **proposed** (not yet implemented)
Target: System 6.0.8 / Mac Plus (68000), System 7 color
Feature flag: `GEOMYS_UTF8` (new) — ON in `full`/`lite`, OFF in `minimal`

## Problem

Geomys renders text by handing bytes straight to QuickDraw, which interprets
them as **Mac Roman**. Modern Gopher servers serve **UTF-8**. A character like
`ä` is two UTF-8 bytes (`0xC3 0xA4`); QuickDraw draws two Mac Roman glyphs
(`Ã¤`) instead of one umlaut. The result: umlauts and accented Latin look
corrupted.

This is **not** a System 6/7 limitation. Mac Roman contains `ä ö ü ß Ä Ö Ü`
and the rest of Western European Latin-1. The fix is to decode incoming UTF-8
to a Unicode code point and map that code point to the correct Mac Roman byte.

## What already exists

- **CP437 path** (`GEOMYS_CP437`): `cp437_translate_if_needed()`
  (`src/content.c:411`) translates high bytes through `cp437_table[256]`
  (`src/cp437.c`) at draw time, in the **text-file view only**. Glyphs that
  have no Mac Roman equivalent collapse to an ASCII `copy_char` fallback.
- **Glyph table** (`GEOMYS_GLYPHS`): `glyph_lookup(codepoint)`
  (`src/glyphs.c:889`) maps a Unicode code point to a curated rendering index
  (180 QuickDraw primitives + 57 sextants + 18 bitmap emoji). **This is the
  limitation to keep in mind:** the glyph set is finite and hand-built, so code
  points outside Mac Roman *and* outside this table cannot be drawn — they fall
  back. Umlauts/accented Latin do **not** need glyphs; they are in Mac Roman.
- **Menu items (directory listings):** display strings go straight from
  `GopherItem.display[]` (`src/gopher.c:1066`) to `DrawString`
  (`src/content.c` ~1119–1158) with **no translation at all** today.

## Reference implementation: Flynn

Flynn (`/home/claude/git/flynn`) already has a complete, proven UTF-8 pipeline
in `src/terminal.c`. Geomys should **borrow these pure functions verbatim**
rather than reinvent them:

| Function | Location | Reuse |
|----------|----------|-------|
| `utf8_decode(buf, len)` | `terminal.c:2855` | decode 2/3/4-byte sequence → code point, with overlong/surrogate rejection. Copy as-is. |
| `latin1_to_macroman[128]` | `terminal.c:2921` | U+0080–U+00FF → Mac Roman byte. Copy as-is. |
| `unicode_to_macroman(cp)` | `terminal.c:2942` | wraps the table. Copy as-is. |
| `unicode_symbol_to_macroman(cp)` | `terminal.c:2953` | curves quotes, dashes, bullet, euro, ≠ ≤ ≥, etc. Copy as-is. |
| `term_put_unicode(cp)` logic | `terminal.c:2983` | the *resolution order* (invisible-absorb → glyph → Latin-1 → symbols → spaces → fallback). Adapt, dropping the terminal-cell calls. |

Flynn's stateful continuation-byte handler (`terminal.c:463–557`) and its
"2-byte UTF-8 to U+0100–U+07FF with no glyph is probably raw CP437" heuristic
(`terminal.c:476`) are the model for our **auto-detection** below.

## Decisions (confirmed)

1. **Detection: auto, no UI.** Validate each line as UTF-8. If it is valid
   multi-byte UTF-8, transcode it; otherwise fall back to the existing
   CP437/Mac Roman path. No preference setting.
2. **Scope: menu items + text files.** Both draw paths get the transcode.
3. **Fallback order:** Mac Roman → glyph `copy_char` → ASCII transliteration
   (accented Latin → base letter) → `'?'`.

## Design

### New shared module: `src/utf8.c` / `src/utf8.h`

Pure, Toolbox-free, dependency-light (may call `glyph_lookup`/`glyph_get_info`
when `GEOMYS_GLYPHS` is on). Guarded by `GEOMYS_UTF8`; stubs when off.

```c
/* Decode one 2/3/4-byte sequence. Returns code point, or -1 if malformed. */
long  utf8_decode(const unsigned char *buf, short len);

/* U+0080-U+00FF -> Mac Roman byte, or 0 if unmapped. (ported from Flynn) */
unsigned char unicode_to_macroman(unsigned short cp);

/* Common punctuation/symbols -> Mac Roman, or 0. (ported from Flynn) */
unsigned char unicode_symbol_to_macroman(unsigned short cp);

/* NEW for Geomys: Latin Extended-A/B & friends -> base ASCII letter, or 0.
   e.g. U+0101 a-macron -> 'a', U+015F s-cedilla -> 's', U+0151 o-dbl-acute -> 'o'. */
unsigned char unicode_transliterate(long cp);

/*
 * Transcode one whole line of possibly-UTF-8 bytes into Mac Roman + ASCII.
 *   - Returns output length (>=0) if the line was valid UTF-8 and transcoded.
 *   - Returns -1 if the line is NOT valid multi-byte UTF-8 (caller falls back
 *     to CP437/raw). A pure-ASCII line also returns -1 (nothing to do).
 * dst must hold at least `len` bytes (output never grows).
 */
short utf8_transcode_line(char *dst, short dstcap, const char *src, short len);
```

**`utf8_transcode_line` algorithm**

1. Scan `src` for high bytes (`>= 0x80`).
   - None → return `-1` (pure ASCII, fast path, caller draws raw).
2. Walk the line as UTF-8. For each high lead byte, require the correct number
   of `0x80–0xBF` continuation bytes. If any byte is a stray continuation, a
   malformed sequence, or a `0xF8–0xFF` lead → **not UTF-8**, return `-1`.
3. Apply Flynn's CP437-ambiguity guard: a 2-byte sequence decoding to
   `U+0100–U+07FF` with `glyph_lookup() < 0` is almost certainly two raw CP437
   bytes, not UTF-8 → treat the **whole line** as not-UTF-8, return `-1`.
4. Otherwise the line is UTF-8. For each decoded code point, resolve to one
   output byte (the resolution chain, mirroring `term_put_unicode`):
   1. Invisible/formatting (soft hyphen, ZWSP, BOM, variation selectors, …)
      → emit nothing (absorb).
   2. Unicode spaces (U+2000–U+200A, U+3000, …) → `' '`.
   3. `unicode_to_macroman()` (Latin-1) → that byte. **This covers all
      umlauts and Western European accents.**
   4. `unicode_symbol_to_macroman()` (smart quotes, dashes, €, ≠, ≤, ≥, …)
      → that byte.
   5. `glyph_lookup()` ≥ 0 → `glyph_get_info()->copy_char` (an ASCII
      approximation; Geomys content draws flat strings, so we use the glyph's
      copy character rather than its bitmap — see "Out of scope" below).
   6. `unicode_transliterate()` → base ASCII letter.
   7. else → `'?'`.

### Draw-path integration: one unified hook

Replace the CP437-only hook in the text-file draw path, and add the same call
to the menu-item path, via one function in `content.c`:

```c
/* Try UTF-8 first; fall back to CP437; else signal "use raw". */
static short
text_transcode_if_needed(char *dst, short dstcap, const char *src, short len)
{
#ifdef GEOMYS_UTF8
    short n = utf8_transcode_line(dst, dstcap, src, len);
    if (n >= 0) return n;          /* was valid UTF-8 */
#endif
#ifdef GEOMYS_CP437
    return cp437_translate_if_needed(dst, src, len);   /* existing */
#endif
    return 0;                       /* caller draws raw bytes */
}
```

- **Text view** (`src/content.c:1428`): swap `cp437_translate_if_needed` for
  `text_transcode_if_needed`.
- **Menu items** (`src/content.c`, the `format_row_text`/`DrawString` region
  ~862 and ~1119–1158): transcode the `item->display` bytes into a temp buffer
  before composing the row (style prefix + name). Style prefixes are ASCII and
  unaffected.

This keeps a single code path, mirrors the proven CP437 architecture, and is
low-risk (draw-time, no change to stored bytes).

### Why draw-time (and the one caveat)

Mirrors CP437 exactly: stored bytes stay as received; only visible rows are
transcoded each redraw (~30 rows, cheap). Known minor caveat: clipboard copy,
find/search, and horizontal-scroll width measurement still operate on the raw
UTF-8 bytes. **Phase 5** addresses copy/find; width error is cosmetic (a couple
of pixels on lines with many umlauts) and accepted for now.

*Alternative considered:* normalize to Mac Roman once at parse time
(`src/gopher.c`) so every downstream consumer sees correct bytes. Cleaner for
copy/find/width, but diverges from the draw-time CP437 model and touches the
parser. Deferred; revisit if copy/find correctness becomes a priority.

## Out of scope (future)

- **Inline glyph *bitmaps* in content view.** Flynn bitmap-renders glyphs
  because it has a fixed-cell terminal grid. Geomys draws proportional flat
  strings via `DrawText`/`DrawString`, so injecting a `CopyBits` glyph
  mid-line requires pen-position measurement per glyph. Not needed for the
  umlaut fix; the `copy_char` fallback is used instead.
- **CJK / emoji.** Out of Mac Roman and (mostly) out of the glyph table →
  `'?'`. Same as Flynn's terminal.
- **Encoding preference UI.** Explicitly declined; auto-detect only.

## Phases

1. **Module + flag.** Add `src/utf8.c`/`src/utf8.h` with the ported Flynn
   functions and stubs-when-off. Add `GEOMYS_UTF8` to `CMakeLists.txt`
   (option, `target_sources`, `target_compile_definitions`) and to
   `scripts/build.sh` (defaults per preset, `--utf8`/`--no-utf8`, feature
   summary line). Mirror the `GEOMYS_CP437` wiring exactly. Build all presets,
   confirm no link errors with the flag on and off.
2. **Transcoder.** Implement `utf8_transcode_line` (detection + resolution
   chain) and `unicode_transliterate` (small Latin-Extended → ASCII table).
   Unit-reason against fixtures: pure ASCII, `ä ö ü ß`, smart quotes, an
   em-dash, a stray `0xC3` (must fall through to CP437), raw CP437 accents
   (must fall through), a CJK char (→ `'?'`).
3. **Text view.** Introduce `text_transcode_if_needed`; wire it into
   `src/content.c:1428`. Verify CP437 still works when a line isn't UTF-8.
4. **Menu items.** Transcode `item->display` in the row-draw path so directory
   titles with umlauts render. Confirm style prefixes/type icons unaffected.
5. **Copy/find consistency.** Run the same transcode in the clipboard-copy
   builder (`GEOMYS_CLIPBOARD`) and in find/search so selected/searched text
   matches what is drawn.
6. **Docs + changelog.** Update `docs/BUILD.md` feature table, `CLAUDE.md`
   feature-flag note, `README.md`, and add a CHANGELOG entry. Update "About
   Geomys" if feature list is shown there.

## Test material (per project convention, use sdf.org)

- A Gopher menu whose item titles contain umlauts (German/Nordic content on
  sdf.org or a self-hosted test selector).
- A UTF-8 text file with: umlauts, `ß`, smart quotes, en/em dashes, `€`, a
  math symbol (`≠`), and a CJK character (expected `'?'`).
- A legacy CP437 ASCII-art text file (must still render via the CP437 path,
  i.e. UTF-8 detection must *not* false-positive on it).

Per repo rules: build/deploy only when asked; the human runs QA in Snow
(System 6) and Basilisk II (System 7).
