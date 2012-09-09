/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

 /* Endian_SwapXX functions taken from SDL (SDL_endian.h) */

#ifndef __ENDIAN_SWAP_H__
#define __ENDIAN_SWAP_H__

/* Include config.h to define (or not) WORDS_BIGENDIAN
   File created by configure */
#if defined(__linux__) || defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
#include "config.h"
#include <inttypes.h>
#endif
#ifdef _WIN32
#define __inline__ __inline
#include <stdint.h>
#endif


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
static __inline__ uint16_t Endian_Swap16(uint16_t x)
{
        uint16_t result;

        __asm__("rlwimi %0,%2,8,16,23" : "=&r" (result) : "0" (x >> 8), "r" (x));
        return result;
}

static __inline__ uint32_t Endian_Swap32(uint32_t x)
{
        uint32_t result;

        __asm__("rlwimi %0,%2,24,16,23" : "=&r" (result) : "0" (x>>24), "r" (x));
        __asm__("rlwimi %0,%2,8,8,15"   : "=&r" (result) : "0" (result),    "r" (x));
        __asm__("rlwimi %0,%2,24,0,7"   : "=&r" (result) : "0" (result),    "r" (x));
        return result;
}
#else
static __inline__ uint16_t Endian_Swap16(uint16_t x) {
        return((x<<8)|(x>>8));
}

static __inline__ uint32_t Endian_Swap32(uint32_t x) {
        return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}
#endif

static __inline__ uint64_t Endian_Swap64(uint64_t x) {
        uint32_t hi, lo;

        /* Separate into high and low 32-bit values and swap them */
        lo = (uint32_t)(x&0xFFFFFFFF);
        x >>= 32;
        hi = (uint32_t)(x&0xFFFFFFFF);
        x = Endian_Swap32(lo);
        x <<= 32;
        x |= Endian_Swap32(hi);
        return(x);

}

void Endian_Swap16_buf(uint16_t *dst, uint16_t *src, int w);

#ifndef WORDS_BIGENDIAN
#define Endian_SwapLE16(X) (X)
#define Endian_SwapLE32(X) (X)
#define Endian_SwapLE64(X) (X)
#define Endian_SwapBE16(X) Endian_Swap16(X)
#define Endian_SwapBE32(X) Endian_Swap32(X)
#define Endian_SwapBE64(X) Endian_Swap64(X)
#else
#define Endian_SwapLE16(X) Endian_Swap16(X)
#define Endian_SwapLE32(X) Endian_Swap32(X)
#define Endian_SwapLE64(X) Endian_Swap64(X)
#define Endian_SwapBE16(X) (X)
#define Endian_SwapBE32(X) (X)
#define Endian_SwapBE64(X) (X)
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif  /* __ENDIAN_SWAP_H__ */
