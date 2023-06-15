/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __GZIP_H
#define __GZIP_H

/**
 * gzip_parse_header() - Parse a header from a gzip file
 *
 * This returns the length of the header.
 *
 * @src: Pointer to gzip file
 * @len: Length of data
 * @return length of header in bytes, or -1 if not enough data
 */
int gzip_parse_header(const unsigned char *src, unsigned long len);

/**
 * gunzip() - Decompress gzipped data
 *
 * @dst: Destination for uncompressed data
 * @dstlen: Size of destination buffer
 * @src: Source data to decompress
 * @lenp: Returns length of uncompressed data
 * @return 0 if OK, -1 on error
 */
int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp);

/**
 * zunzip() - Uncompress blocks compressed with zlib without headers
 *
 * @dst: Destination for uncompressed data
 * @dstlen: Size of destination buffer
 * @src: Source data to decompress
 * @lenp: On entry, length data at @src. On exit, number of bytes used from @src
 * @stoponerr: 0 to continue when a decode error is found, 1 to stop
 * @offset: start offset within the src buffer
 * @return 0 if OK, -1 on error
 */
int zunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp,
	   int stoponerr, int offset);
#endif
