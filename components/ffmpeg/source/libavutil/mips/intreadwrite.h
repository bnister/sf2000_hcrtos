/*
 * Copyright (c) 2009 Mans Rullgard <mans@mansr.com>
 *
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

#ifndef AVUTIL_MIPS_INTREADWRITE_H
#define AVUTIL_MIPS_INTREADWRITE_H

#include <stdint.h>
#include "config.h"

#if ARCH_MIPS64 && HAVE_INLINE_ASM && !HAVE_MIPS64R6

#define AV_RN32 AV_RN32
static av_always_inline uint32_t AV_RN32(const void *p)
{
    struct __attribute__((packed)) u32 { uint32_t v; };
    const uint8_t *q = p;
    const struct u32 *pl = (const struct u32 *)(q + 3 * !HAVE_BIGENDIAN);
    const struct u32 *pr = (const struct u32 *)(q + 3 *  HAVE_BIGENDIAN);
    uint32_t v;
    __asm__ ("lwl %0, %1  \n\t"
             "lwr %0, %2  \n\t"
             : "=&r"(v)
             : "m"(*pl), "m"(*pr));
    return v;
}

#endif /* ARCH_MIPS64 && HAVE_INLINE_ASM && !HAVE_MIPS64R6 */

#if 0 //def __MIPS32R1__
#define AV_RB16 AV_RB16
static av_always_inline uint16_t AV_RB16(const void *p)
{
    uint16_t v1, v2;
    __asm__ volatile(
        "lbu    %0,   0(%2)  \n\t"
        "sll    %0, %0, 0x8  \n\t"
        "lbu    %1,   1(%2)  \n\t"
        "or     %0, %1, %0   \n\t"
        : "+r"(v1), "=r"(v2)
        : "r"(p));
    return v1;
}

#define AV_RB32 AV_RB32
static av_always_inline uint32_t AV_RB32(const void *p)
{
    uint32_t v1;
    __asm__ volatile(
        "ulw    $4,   0(%1)  \n\t"
        ".word 0x70801010    \n\t"
        "move   %0,   $2     \n\t"
        : "=r"(v1)
        : "r"(p)
        : "$4", "$2");
    return v1;
}
#endif

#endif /* AVUTIL_MIPS_INTREADWRITE_H */
