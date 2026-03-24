/*
 * imgparse.c - Image header detection and dimension parsing
 *
 * All reads are byte-by-byte to avoid 68000 alignment traps.
 */

#ifdef GEOMYS_DOWNLOAD

#include <Types.h>
#include <string.h>
#include "imgparse.h"

/* GIF magic: "GIF87a" or "GIF89a" */
static const char gif_magic[3] = { 'G', 'I', 'F' };

/* PNG magic: 8 bytes */
static const unsigned char png_magic[8] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};

short
img_detect_format(const char *buf, short len)
{
	if (len >= 6 && memcmp(buf, gif_magic, 3) == 0)
		return IMG_FMT_GIF;

	if (len >= 8 && memcmp(buf, png_magic, 8) == 0)
		return IMG_FMT_PNG;

	return IMG_FMT_UNKNOWN;
}

Boolean
img_parse_dimensions(const char *buf, short len, short fmt,
    unsigned short *width, unsigned short *height)
{
	if (fmt == IMG_FMT_GIF && len >= 10) {
		/* GIF: width at bytes 6-7 (little-endian),
		 * height at bytes 8-9 (little-endian) */
		*width = (unsigned char)buf[6] |
		    ((unsigned short)(unsigned char)buf[7] << 8);
		*height = (unsigned char)buf[8] |
		    ((unsigned short)(unsigned char)buf[9] << 8);
		return true;
	}

	if (fmt == IMG_FMT_PNG && len >= 24) {
		/* PNG: IHDR chunk at offset 8, width at 16-19
		 * (big-endian), height at 20-23 (big-endian).
		 * Images > 65535 in either dimension are clamped. */
		unsigned long w, h;

		w = ((unsigned long)(unsigned char)buf[16] << 24) |
		    ((unsigned long)(unsigned char)buf[17] << 16) |
		    ((unsigned long)(unsigned char)buf[18] << 8) |
		     (unsigned long)(unsigned char)buf[19];
		h = ((unsigned long)(unsigned char)buf[20] << 24) |
		    ((unsigned long)(unsigned char)buf[21] << 16) |
		    ((unsigned long)(unsigned char)buf[22] << 8) |
		     (unsigned long)(unsigned char)buf[23];

		*width = (w > 65535UL) ? 65535U : (unsigned short)w;
		*height = (h > 65535UL) ? 65535U : (unsigned short)h;
		return true;
	}

	*width = 0;
	*height = 0;
	return false;
}

const char *
img_format_name(const char *buf, short len, short fmt)
{
	if (fmt == IMG_FMT_GIF && len >= 6) {
		/* Return "GIF87a" or "GIF89a" based on version */
		if (buf[3] == '8' && buf[4] == '7')
			return "GIF87a";
		if (buf[3] == '8' && buf[4] == '9')
			return "GIF89a";
		return "GIF";
	}

	if (fmt == IMG_FMT_PNG)
		return "PNG";

	return "Unknown";
}

#endif /* GEOMYS_DOWNLOAD */
