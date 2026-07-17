/*
 * utf8.c - UTF-8 to Mac Roman transcoding for Gopher text
 *
 * See utf8.h and docs/UTF8_PLAN.md.  The pure code-point mappers are ported
 * from Flynn's terminal.c reference implementation; the line-level detection
 * and resolution driver (utf8_transcode_line) is new, since Geomys transcodes
 * whole buffered lines rather than a streaming terminal.
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

#include "utf8.h"

#ifdef GEOMYS_UTF8

#include "glyphs.h"

/*
 * utf8_decode - decode a complete 2/3/4-byte UTF-8 sequence to a code point.
 * Returns the code point, or -1 on malformed/overlong/surrogate input.
 * (Ported from Flynn terminal.c, returning -1 instead of '?' so the caller
 * can distinguish "not UTF-8" from a literal question mark.)
 */
static long
utf8_decode(const unsigned char *buf, short len)
{
	long cp;

	switch (len) {
	case 2:
		if ((buf[0] & 0xE0) != 0xC0 || (buf[1] & 0xC0) != 0x80)
			return -1;
		cp = ((long)(buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
		if (cp < 0x80) return -1;			/* overlong */
		break;
	case 3:
		if ((buf[0] & 0xF0) != 0xE0 || (buf[1] & 0xC0) != 0x80 ||
		    (buf[2] & 0xC0) != 0x80)
			return -1;
		cp = ((long)(buf[0] & 0x0F) << 12) |
		     ((long)(buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
		if (cp < 0x800) return -1;			/* overlong */
		if (cp >= 0xD800 && cp <= 0xDFFF) return -1;	/* surrogate */
		break;
	case 4:
		if ((buf[0] & 0xF8) != 0xF0 || (buf[1] & 0xC0) != 0x80 ||
		    (buf[2] & 0xC0) != 0x80 || (buf[3] & 0xC0) != 0x80)
			return -1;
		cp = ((long)(buf[0] & 0x07) << 18) |
		     ((long)(buf[1] & 0x3F) << 12) |
		     ((long)(buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);
		if (cp < 0x10000 || cp > 0x10FFFF) return -1;	/* overlong */
		break;
	default:
		return -1;
	}

	return cp;
}

/* Index: codepoint - 0x80. Value: Mac Roman byte, or 0 if unmapped.
 * (Ported verbatim from Flynn terminal.c.) */
static const unsigned char latin1_to_macroman[128] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,		/* 0x80-0x8F: C1 */
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,		/* 0x90-0x9F: C1 */
	0xCA,0xC1,0xA2,0xA3, 0xB4,0xB4,0x7C,0xA4,	/* 0xA0-0xA7 */
	0xAC,0xA9,0xBB,0xC7, 0xC2,0,0xA8,0xF8,		/* 0xA8-0xAF */
	0xA1,0xB1,0,0, 0xAB,0xB5,0xA6,0xE1,		/* 0xB0-0xB7 */
	0xFC,0,0xBC,0xC8, 0,0,0,0xC0,			/* 0xB8-0xBF */
	0xCB,0xE7,0xE5,0xCC, 0x80,0x81,0xAE,0x82,	/* 0xC0-0xC7 */
	0xE9,0x83,0xE6,0xE8, 0xED,0xEA,0xEB,0xEC,	/* 0xC8-0xCF */
	0,0x84,0xF1,0xEE, 0xEF,0xCD,0x85,0,		/* 0xD0-0xD7 */
	0xAF,0xF4,0xF2,0xF3, 0x86,0,0,0xA7,		/* 0xD8-0xDF */
	0x88,0x87,0x89,0x8B, 0x8A,0x8C,0xBE,0x8D,	/* 0xE0-0xE7 */
	0x8F,0x8E,0x90,0x91, 0x93,0x92,0x94,0x95,	/* 0xE8-0xEF */
	0,0x96,0x98,0x97, 0x99,0x9B,0x9A,0xD6,		/* 0xF0-0xF7 */
	0xBF,0x9D,0x9C,0x9E, 0x9F,0,0,0xD8,		/* 0xF8-0xFF */
};

/*
 * unicode_to_macroman - translate U+0080-U+00FF to Mac Roman.
 * (Ported from Flynn terminal.c.)
 */
static unsigned char
unicode_to_macroman(unsigned short cp)
{
	if (cp < 0x80 || cp > 0xFF)
		return 0;
	return latin1_to_macroman[cp - 0x80];
}

/*
 * unicode_symbol_to_macroman - translate common Unicode symbols that have a
 * Mac Roman equivalent.  Returns the Mac Roman byte, or 0 if unmapped.
 * (Ported from Flynn terminal.c, extended with the OE ligature, Y-diaeresis
 * and florin, which Mac Roman also contains.)
 */
static unsigned char
unicode_symbol_to_macroman(unsigned short cp)
{
	switch (cp) {
	case 0x2013: return 0xD0;	/* en dash */
	case 0x2014: return 0xD1;	/* em dash */
	case 0x2018: return 0xD4;	/* left single quote */
	case 0x2019: return 0xD5;	/* right single quote */
	case 0x201C: return 0xD2;	/* left double quote */
	case 0x201D: return 0xD3;	/* right double quote */
	case 0x2022: return 0xA5;	/* bullet */
	case 0x2026: return 0xC9;	/* ellipsis */
	case 0x2122: return 0xAA;	/* trademark */
	case 0x20AC: return 0xDB;	/* euro sign */
	case 0x2260: return 0xAD;	/* not equal */
	case 0x2264: return 0xB2;	/* less or equal */
	case 0x2265: return 0xB3;	/* greater or equal */
	case 0x03C0: return 0xB9;	/* pi */
	case 0x2206: return 0xC6;	/* delta */
	case 0x2219: return 0xE1;	/* bullet operator -> middle dot */
	case 0x22C5: return 0xE1;	/* dot operator -> middle dot */
	case 0x0387: return 0xE1;	/* greek ano teleia -> middle dot */
	case 0x2027: return 0xE1;	/* hyphenation point -> middle dot */
	case 0x0152: return 0xCE;	/* OE ligature */
	case 0x0153: return 0xCF;	/* oe ligature */
	case 0x0178: return 0xD9;	/* Y with diaeresis */
	case 0x0192: return 0xC4;	/* florin */
	default:     return 0;
	}
}

/* Latin Extended-A (U+0100-U+017F) -> base ASCII letter, or 0 if none.
 * Used as a readability fallback for accented Latin that Mac Roman lacks
 * (Central/Eastern European).  Index: codepoint - 0x100.  Entries that Mac
 * Roman covers (OE/oe at 0x152/0x153, Y-diaeresis at 0x178) are left 0 so
 * unicode_symbol_to_macroman handles them first. */
static const unsigned char latinext_a_base[128] = {
	'A','a','A','a', 'A','a','C','c', 'C','c','C','c', 'C','c','D','d', /* 0x100 */
	'D','d','E','e', 'E','e','E','e', 'E','e','E','e', 'G','g','G','g', /* 0x110 */
	'G','g','G','g', 'H','h','H','h', 'I','i','I','i', 'I','i','I','i', /* 0x120 */
	'I','i','I','i', 'J','j','K','k', 'k','L','l','L', 'l','L','l','L', /* 0x130 */
	'l','L','l','N', 'n','N','n','N', 'n','n','N','n', 'O','o','O','o', /* 0x140 */
	'O','o',0,0,     'R','r','R','r', 'R','r','S','s', 'S','s','S','s', /* 0x150 */
	'S','s','T','t', 'T','t','T','t', 'U','u','U','u', 'U','u','U','u', /* 0x160 */
	'U','u','U','u', 'W','w','Y','y', 0,'Z','z','Z',   'z','Z','z','s', /* 0x170 */
};

/*
 * unicode_transliterate - accented Latin Extended-A -> base ASCII letter.
 * Returns the base letter, or 0 if there is no transliteration.
 */
static unsigned char
unicode_transliterate(long cp)
{
	if (cp >= 0x100 && cp <= 0x17F)
		return latinext_a_base[cp - 0x100];
	return 0;
}

/*
 * utf8_resolve - resolve one decoded code point to a single output byte.
 * Returns the byte to emit (0x00-0xFF), or -1 to absorb (emit nothing).
 */
static short
utf8_resolve(long cp)
{
	unsigned char ch;

	/* Invisible / formatting code points: absorb silently. */
	if (cp == 0x00AD ||			/* soft hyphen */
	    (cp >= 0x200B && cp <= 0x200F) ||	/* ZWSP,ZWNJ,ZWJ,LRM,RLM */
	    (cp >= 0x2028 && cp <= 0x202E) ||	/* line/para sep, bidi */
	    (cp >= 0x2060 && cp <= 0x2069) ||	/* word joiner, bidi iso */
	    (cp >= 0xFE00 && cp <= 0xFE0F) ||	/* variation selectors */
	    cp == 0xFEFF ||			/* BOM / ZWNBSP */
	    (cp >= 0xE0001L && cp <= 0xE007FL))	/* tag characters */
		return -1;

	/* Unicode spaces -> ASCII space. */
	if ((cp >= 0x2000 && cp <= 0x200A) || cp == 0x205F || cp == 0x3000)
		return ' ';

	/* Latin-1 Supplement -> Mac Roman (covers umlauts and accents).
	 * Codepoints Mac Roman lacks (superscripts ², ³, ¹, ...) fall
	 * through to the glyph table and transliteration below before '?'. */
	if (cp >= 0x80 && cp <= 0xFF) {
		ch = unicode_to_macroman((unsigned short)cp);
		if (ch)
			return (short)ch;
	}

	/* Common punctuation/symbols -> Mac Roman.  Guard the cast so
	 * supplementary-plane code points (cp > 0xFFFF) don't alias a
	 * mapped BMP symbol (e.g. U+12013 truncating to U+2013 en dash). */
	if (cp <= 0xFFFF) {
		ch = unicode_symbol_to_macroman((unsigned short)cp);
		if (ch)
			return (short)ch;
	}

#ifdef GEOMYS_GLYPHS
	/* Curated glyph -> its ASCII copy character.  Content view draws flat
	 * strings, so we approximate the glyph rather than blitting a bitmap. */
	{
		short gid = glyph_lookup(cp);
		if (gid >= 0) {
			const GlyphInfo *gi =
			    glyph_get_info((unsigned char)gid);
			if (gi && gi->copy_char)
				return (short)gi->copy_char;
		}
	}
#endif

	/* Accented Latin Extended -> base letter. */
	ch = unicode_transliterate(cp);
	if (ch)
		return (short)ch;

	/* Everything else (CJK, emoji, braille, ...) -> fallback. */
	return '?';
}

short
utf8_transcode_line(char *dst, short dstcap, const char *src, short len)
{
	short i, out;

	/* Fast path: a line with no high bytes is plain ASCII; let the caller
	 * draw the original bytes untouched. */
	for (i = 0; i < len; i++) {
		if ((unsigned char)src[i] >= 0x80)
			break;
	}
	if (i >= len)
		return -1;

	/* Validate and transcode in a single pass.  Any byte that is not part
	 * of a well-formed UTF-8 sequence means this line is not UTF-8 (more
	 * likely CP437 / Mac Roman), so bail and let the caller fall back. */
	out = 0;
	i = 0;
	while (i < len) {
		unsigned char c = (unsigned char)src[i];
		short seqlen, k, r;
		long cp;

		if (c < 0x80) {
			if (out < dstcap)
				dst[out++] = (char)c;
			i++;
			continue;
		}

		if ((c & 0xE0) == 0xC0)
			seqlen = 2;
		else if ((c & 0xF0) == 0xE0)
			seqlen = 3;
		else if ((c & 0xF8) == 0xF0)
			seqlen = 4;
		else
			return -1;	/* stray continuation or 0xF8-0xFF */

		if (i + seqlen > len)
			return -1;	/* truncated sequence */
		for (k = 1; k < seqlen; k++) {
			if (((unsigned char)src[i + k] & 0xC0) != 0x80)
				return -1;	/* bad continuation byte */
		}

		cp = utf8_decode((const unsigned char *)src + i, seqlen);
		if (cp < 0)
			return -1;	/* overlong / surrogate / invalid */

		/* CP437-ambiguity guard (Flynn heuristic): a 2-byte sequence
		 * decoding to U+0100-U+07FF with no glyph is almost certainly
		 * two raw CP437 bytes misread as UTF-8. */
		if (seqlen == 2 && cp > 0xFF && cp < 0x800 &&
		    glyph_lookup(cp) < 0)
			return -1;

		r = utf8_resolve(cp);
		if (r >= 0 && out < dstcap)
			dst[out++] = (char)r;
		i += seqlen;
	}

	return out;
}

#endif /* GEOMYS_UTF8 */
