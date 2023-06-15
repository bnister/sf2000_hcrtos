/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVUTIL_MIPS_BSWAP_H
#define AVUTIL_MIPS_BSWAP_H

#if 0 //def __MIPS32R1__

#include <stdint.h>
#include "config.h"
#include "libavutil/attributes.h"

#define av_bswap16 av_bswap16
static av_always_inline av_const uint16_t av_bswap16(uint16_t x)
{
    uint16_t v1;
    __asm__ volatile(
        "move  $4,   %1  \n\t"
        ".word 0x70801210    \n\t"
        "move   %0,  $2     \n\t"
        : "=r"(v1)
        : "r"(x)
        : "$4", "$2");
    return v1;
}

#define av_bswap32 av_bswap32
static av_always_inline av_const uint32_t av_bswap32(uint32_t x)
{
    uint32_t v1;
    __asm__ volatile(
        "move  $4,   %1  \n\t"
        ".word 0x70801010    \n\t"
        "move   %0,  $2     \n\t"
        : "=r"(v1)
        : "r"(x)
        : "$4", "$2");
    return v1;
}
#endif

#endif /* AVUTIL_MIPS_BSWAP_H */
