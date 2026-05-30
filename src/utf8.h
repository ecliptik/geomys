/*
 * utf8.h - UTF-8 to Mac Roman transcoding for Gopher text
 *
 * Modern Gopher servers serve UTF-8.  Classic QuickDraw renders Mac Roman.
 * This module detects UTF-8 in a buffered line and transcodes it to Mac
 * Roman (plus ASCII fallbacks) so umlauts and accented Latin render
 * correctly instead of as mojibake.
 *
 * The pure code-point mappers (utf8_decode, latin1_to_macroman,
 * unicode_to_macroman, unicode_symbol_to_macroman) are ported from Flynn's
 * terminal.c, which is the reference UTF-8 implementation.  See
 * docs/UTF8_PLAN.md.
 *
 * Copyright (c) 2024-2026 Flynn project
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef UTF8_H
#define UTF8_H

#ifdef GEOMYS_UTF8

/*
 * utf8_transcode_line - detect and transcode one line of UTF-8 bytes.
 *
 * Walks src as UTF-8, resolving each code point to a single Mac Roman / ASCII
 * byte (Latin-1 -> Mac Roman, common symbols -> Mac Roman, curated glyph ->
 * its copy character, accented Latin -> base letter, else '?').  Invisible
 * formatting code points are absorbed.
 *
 * Returns the output length (>= 0) when src is valid multi-byte UTF-8 and was
 * transcoded into dst.  Returns -1 when src is pure ASCII, or is NOT clean
 * UTF-8 (a stray continuation byte, a truncated/overlong sequence, or a
 * 2-byte sequence that is more likely raw CP437); the caller should then fall
 * back to CP437/raw rendering.  Output never grows past the input length;
 * dst must hold at least min(len, dstcap) bytes.
 */
short utf8_transcode_line(char *dst, short dstcap, const char *src, short len);

#else /* !GEOMYS_UTF8 */

/* Stub: always "not UTF-8" so callers fall through to CP437/raw. */
#define utf8_transcode_line(dst, dstcap, src, len)  (-1)

#endif /* GEOMYS_UTF8 */

#endif /* UTF8_H */
