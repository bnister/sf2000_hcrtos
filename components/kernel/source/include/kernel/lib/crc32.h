#ifndef _CRC32_H_
#define _CRC32_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel/types.h>

uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len);

/*
 * Calculate the crc32 checksum triggering the watchdog every 'chunk_sz' bytes
 * of input.
 */
uint32_t crc32_wd(uint32_t crc, const unsigned char *buf, uint32_t len,
		  uint32_t chunk_sz);

uint32_t nuttx_crc32(const uint8_t *src, size_t len);
uint32_t nuttx_crc32part(const uint8_t *src, size_t len, uint32_t crc32val);

#ifdef __cplusplus
}
#endif

#endif
