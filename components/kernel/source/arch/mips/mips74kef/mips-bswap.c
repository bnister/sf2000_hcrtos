#include <stdint.h>

uint32_t ________swap32(uint32_t n)
{
	return ((((n) & (uint32_t)0x000000ffUL) << 24) |
		(((n) & (uint32_t)0x0000ff00UL) << 8) |
		(((n) & (uint32_t)0x00ff0000UL) >> 8) |
		(((n) & (uint32_t)0xff000000UL) >> 24));
}

uint16_t ________swap16(uint16_t n)
{
	return ((uint16_t)((((n) & (uint16_t)0x00ffU) << 8) |
			   (((n) & (uint16_t)0xff00U) >> 8)));
}
