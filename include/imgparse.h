/*
 * imgparse.h - Image header detection and dimension parsing
 */

#ifndef IMGPARSE_H
#define IMGPARSE_H

#include <Types.h>

/* Image format constants */
#define IMG_FMT_UNKNOWN  0
#define IMG_FMT_GIF      1
#define IMG_FMT_PNG      2

/* Minimum header bytes needed for detection + dimension parsing */
#define IMG_HEADER_SIZE  26

/*
 * Detect image format from first bytes of header.
 * Returns IMG_FMT_GIF, IMG_FMT_PNG, or IMG_FMT_UNKNOWN.
 */
short img_detect_format(const char *buf, short len);

/*
 * Parse image dimensions from header buffer.
 * fmt must be IMG_FMT_GIF or IMG_FMT_PNG.
 * Returns true if dimensions were successfully parsed.
 * Uses byte-by-byte reads to avoid 68000 alignment traps.
 */
Boolean img_parse_dimensions(const char *buf, short len,
    short fmt, unsigned short *width, unsigned short *height);

/*
 * Get human-readable format name string.
 */
const char *img_format_name(const char *buf, short len, short fmt);

#endif /* IMGPARSE_H */
