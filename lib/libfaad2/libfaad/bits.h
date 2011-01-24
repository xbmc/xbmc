/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: bits.h,v 1.45 2007/11/01 12:33:29 menno Exp $
**/

#ifndef __BITS_H__
#define __BITS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "analysis.h"
#ifdef ANALYSIS
#include <stdio.h>
#endif

#define BYTE_NUMBIT     8
#define BYTE_NUMBIT_LD  3
//#define bit2byte(a) ((a+7)/BYTE_NUMBIT)
#define bit2byte(a) ((a+7)>>BYTE_NUMBIT_LD)

typedef struct _bitfile
{
    /* bit input */
    uint32_t bufa;
    uint32_t bufb;
    uint32_t bits_left;
    uint32_t buffer_size; /* size of the buffer in bytes */
    uint32_t bytes_left;
    uint8_t error;
    uint32_t *tail;
    uint32_t *start;
    const void *buffer;
} bitfile;


#if 0
static uint32_t const bitmask[] = {
    0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF,
    0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
    0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF,
    0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF,
    0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF
    /* added bitmask 32, correct?!?!?! */
    , 0xFFFFFFFF
};
#endif

void faad_initbits(bitfile *ld, const void *buffer, const uint32_t buffer_size);
void faad_endbits(bitfile *ld);
void faad_initbits_rev(bitfile *ld, void *buffer,
                       uint32_t bits_in_buffer);
uint8_t faad_byte_align(bitfile *ld);
uint32_t faad_get_processed_bits(bitfile *ld);
void faad_flushbits_ex(bitfile *ld, uint32_t bits);
void faad_rewindbits(bitfile *ld);
void faad_resetbits(bitfile *ld, int bits);
uint8_t *faad_getbitbuffer(bitfile *ld, uint32_t bits
                       DEBUGDEC);
#ifdef DRM
void *faad_origbitbuffer(bitfile *ld);
uint32_t faad_origbitbuffer_size(bitfile *ld);
#endif

/* circumvent memory alignment errors on ARM */
static INLINE uint32_t getdword(void *mem)
{
    uint32_t tmp;
#ifndef ARCH_IS_BIG_ENDIAN
    ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[3];
    ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
    ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
    ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
#else
    ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
    ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
    ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
    ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[3];
#endif

    return tmp;
}

/* reads only n bytes from the stream instead of the standard 4 */
static /*INLINE*/ uint32_t getdword_n(void *mem, int n)
{
    uint32_t tmp = 0;
#ifndef ARCH_IS_BIG_ENDIAN
    switch (n)
    {
    case 3:
        ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[2];
    case 2:
        ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[1];
    case 1:
        ((uint8_t*)&tmp)[3] = ((uint8_t*)mem)[0];
    default:
        break;
    }
#else
    switch (n)
    {
    case 3:
        ((uint8_t*)&tmp)[2] = ((uint8_t*)mem)[2];
    case 2:
        ((uint8_t*)&tmp)[1] = ((uint8_t*)mem)[1];
    case 1:
        ((uint8_t*)&tmp)[0] = ((uint8_t*)mem)[0];
    default:
        break;
    }
#endif

    return tmp;
}

static INLINE uint32_t faad_showbits(bitfile *ld, uint32_t bits)
{
    if (bits <= ld->bits_left)
    {
        //return (ld->bufa >> (ld->bits_left - bits)) & bitmask[bits];
        return (ld->bufa << (32 - ld->bits_left)) >> (32 - bits);
    }

    bits -= ld->bits_left;
    //return ((ld->bufa & bitmask[ld->bits_left]) << bits) | (ld->bufb >> (32 - bits));
    return ((ld->bufa & ((1<<ld->bits_left)-1)) << bits) | (ld->bufb >> (32 - bits));
}

static INLINE void faad_flushbits(bitfile *ld, uint32_t bits)
{
    /* do nothing if error */
    if (ld->error != 0)
        return;

    if (bits < ld->bits_left)
    {
        ld->bits_left -= bits;
    } else {
        faad_flushbits_ex(ld, bits);
    }
}

/* return next n bits (right adjusted) */
static /*INLINE*/ uint32_t faad_getbits(bitfile *ld, uint32_t n DEBUGDEC)
{
    uint32_t ret;

    if (n == 0)
        return 0;

    ret = faad_showbits(ld, n);
    faad_flushbits(ld, n);

#ifdef ANALYSIS
    if (print)
        fprintf(stdout, "%4d %2d bits, val: %4d, variable: %d %s\n", dbg_count++, n, ret, var, dbg);
#endif

    return ret;
}

static INLINE uint8_t faad_get1bit(bitfile *ld DEBUGDEC)
{
    uint8_t r;

    if (ld->bits_left > 0)
    {
        ld->bits_left--;
        r = (uint8_t)((ld->bufa >> ld->bits_left) & 1);
        return r;
    }

    /* bits_left == 0 */
#if 0
    r = (uint8_t)(ld->bufb >> 31);
    faad_flushbits_ex(ld, 1);
#else
    r = (uint8_t)faad_getbits(ld, 1);
#endif
    return r;
}

/* reversed bitreading routines */
static INLINE uint32_t faad_showbits_rev(bitfile *ld, uint32_t bits)
{
    uint8_t i;
    uint32_t B = 0;

    if (bits <= ld->bits_left)
    {
        for (i = 0; i < bits; i++)
        {
            if (ld->bufa & (1 << (i + (32 - ld->bits_left))))
                B |= (1 << (bits - i - 1));
        }
        return B;
    } else {
        for (i = 0; i < ld->bits_left; i++)
        {
            if (ld->bufa & (1 << (i + (32 - ld->bits_left))))
                B |= (1 << (bits - i - 1));
        }
        for (i = 0; i < bits - ld->bits_left; i++)
        {
            if (ld->bufb & (1 << (i + (32-ld->bits_left))))
                B |= (1 << (bits - ld->bits_left - i - 1));
        }
        return B;
    }
}

static INLINE void faad_flushbits_rev(bitfile *ld, uint32_t bits)
{
    /* do nothing if error */
    if (ld->error != 0)
        return;

    if (bits < ld->bits_left)
    {
        ld->bits_left -= bits;
    } else {
        uint32_t tmp;

        ld->bufa = ld->bufb;
        tmp = getdword(ld->start);
        ld->bufb = tmp;
        ld->start--;
        ld->bits_left += (32 - bits);

        if (ld->bytes_left < 4)
        {
            ld->error = 1;
            ld->bytes_left = 0;
        } else {
            ld->bytes_left -= 4;
        }
//        if (ld->bytes_left == 0)
//            ld->no_more_reading = 1;
    }
}

static /*INLINE*/ uint32_t faad_getbits_rev(bitfile *ld, uint32_t n
                                        DEBUGDEC)
{
    uint32_t ret;

    if (n == 0)
        return 0;

    ret = faad_showbits_rev(ld, n);
    faad_flushbits_rev(ld, n);

#ifdef ANALYSIS
    if (print)
        fprintf(stdout, "%4d %2d bits, val: %4d, variable: %d %s\n", dbg_count++, n, ret, var, dbg);
#endif

    return ret;
}

#ifdef DRM
/* CRC lookup table for G8 polynome in DRM standard */
static const uint8_t crc_table_G8[256] = {
    0x0, 0x1d, 0x3a, 0x27, 0x74, 0x69, 0x4e, 0x53,
    0xe8, 0xf5, 0xd2, 0xcf, 0x9c, 0x81, 0xa6, 0xbb,
    0xcd, 0xd0, 0xf7, 0xea, 0xb9, 0xa4, 0x83, 0x9e,
    0x25, 0x38, 0x1f, 0x2, 0x51, 0x4c, 0x6b, 0x76,
    0x87, 0x9a, 0xbd, 0xa0, 0xf3, 0xee, 0xc9, 0xd4,
    0x6f, 0x72, 0x55, 0x48, 0x1b, 0x6, 0x21, 0x3c,
    0x4a, 0x57, 0x70, 0x6d, 0x3e, 0x23, 0x4, 0x19,
    0xa2, 0xbf, 0x98, 0x85, 0xd6, 0xcb, 0xec, 0xf1,
    0x13, 0xe, 0x29, 0x34, 0x67, 0x7a, 0x5d, 0x40,
    0xfb, 0xe6, 0xc1, 0xdc, 0x8f, 0x92, 0xb5, 0xa8,
    0xde, 0xc3, 0xe4, 0xf9, 0xaa, 0xb7, 0x90, 0x8d,
    0x36, 0x2b, 0xc, 0x11, 0x42, 0x5f, 0x78, 0x65,
    0x94, 0x89, 0xae, 0xb3, 0xe0, 0xfd, 0xda, 0xc7,
    0x7c, 0x61, 0x46, 0x5b, 0x8, 0x15, 0x32, 0x2f,
    0x59, 0x44, 0x63, 0x7e, 0x2d, 0x30, 0x17, 0xa,
    0xb1, 0xac, 0x8b, 0x96, 0xc5, 0xd8, 0xff, 0xe2,
    0x26, 0x3b, 0x1c, 0x1, 0x52, 0x4f, 0x68, 0x75,
    0xce, 0xd3, 0xf4, 0xe9, 0xba, 0xa7, 0x80, 0x9d,
    0xeb, 0xf6, 0xd1, 0xcc, 0x9f, 0x82, 0xa5, 0xb8,
    0x3, 0x1e, 0x39, 0x24, 0x77, 0x6a, 0x4d, 0x50,
    0xa1, 0xbc, 0x9b, 0x86, 0xd5, 0xc8, 0xef, 0xf2,
    0x49, 0x54, 0x73, 0x6e, 0x3d, 0x20, 0x7, 0x1a,
    0x6c, 0x71, 0x56, 0x4b, 0x18, 0x5, 0x22, 0x3f,
    0x84, 0x99, 0xbe, 0xa3, 0xf0, 0xed, 0xca, 0xd7,
    0x35, 0x28, 0xf, 0x12, 0x41, 0x5c, 0x7b, 0x66,
    0xdd, 0xc0, 0xe7, 0xfa, 0xa9, 0xb4, 0x93, 0x8e,
    0xf8, 0xe5, 0xc2, 0xdf, 0x8c, 0x91, 0xb6, 0xab,
    0x10, 0xd, 0x2a, 0x37, 0x64, 0x79, 0x5e, 0x43,
    0xb2, 0xaf, 0x88, 0x95, 0xc6, 0xdb, 0xfc, 0xe1,
    0x5a, 0x47, 0x60, 0x7d, 0x2e, 0x33, 0x14, 0x9,
    0x7f, 0x62, 0x45, 0x58, 0xb, 0x16, 0x31, 0x2c,
    0x97, 0x8a, 0xad, 0xb0, 0xe3, 0xfe, 0xd9, 0xc4,
};

static uint8_t faad_check_CRC(bitfile *ld, uint16_t len)
{
    int bytes, rem;
    unsigned int CRC;
    unsigned int r=255;  /* Initialize to all ones */

    /* CRC polynome used x^8 + x^4 + x^3 + x^2 +1 */
#define GPOLY 0435

    faad_rewindbits(ld);

    CRC = (unsigned int) ~faad_getbits(ld, 8
        DEBUGVAR(1,999,"faad_check_CRC(): CRC")) & 0xFF;          /* CRC is stored inverted */

    bytes = len >> 3;
    rem = len & 0x7;

    for (; bytes > 0; bytes--)
    {
        r = crc_table_G8[( r ^ faad_getbits(ld, 8 DEBUGVAR(1,998,"")) ) & 0xFF];
    }
    for (; rem > 0; rem--)
    {
        r = ( (r << 1) ^ (( ( faad_get1bit(ld
            DEBUGVAR(1,998,""))  & 1) ^ ((r >> 7) & 1)) * GPOLY )) & 0xFF;
    }

    if (r != CRC)
  //  if (0)
    {
        return 28;
    } else {
        return 0;
    }
}

static uint8_t tabFlipbits[256] = {
    0,128,64,192,32,160,96,224,16,144,80,208,48,176,112,240,
    8,136,72,200,40,168,104,232,24,152,88,216,56,184,120,248,
    4,132,68,196,36,164,100,228,20,148,84,212,52,180,116,244,
    12,140,76,204,44,172,108,236,28,156,92,220,60,188,124,252,
    2,130,66,194,34,162,98,226,18,146,82,210,50,178,114,242,
    10,138,74,202,42,170,106,234,26,154,90,218,58,186,122,250,
    6,134,70,198,38,166,102,230,22,150,86,214,54,182,118,246,
    14,142,78,206,46,174,110,238,30,158,94,222,62,190,126,254,
    1,129,65,193,33,161,97,225,17,145,81,209,49,177,113,241,
    9,137,73,201,41,169,105,233,25,153,89,217,57,185,121,249,
    5,133,69,197,37,165,101,229,21,149,85,213,53,181,117,245,
    13,141,77,205,45,173,109,237,29,157,93,221,61,189,125,253,
    3,131,67,195,35,163,99,227,19,147,83,211,51,179,115,243,
    11,139,75,203,43,171,107,235,27,155,91,219,59,187,123,251,
    7,135,71,199,39,167,103,231,23,151,87,215,55,183,119,247,
    15,143,79,207,47,175,111,239,31,159,95,223,63,191,127,255
};
#endif

#ifdef ERROR_RESILIENCE

/* Modified bit reading functions for HCR */

typedef struct
{
    /* bit input */
    uint32_t bufa;
    uint32_t bufb;
    int8_t len;
} bits_t;


static INLINE uint32_t showbits_hcr(bits_t *ld, uint8_t bits)
{
    if (bits == 0) return 0;
    if (ld->len <= 32)
    {
        /* huffman_spectral_data_2 needs to read more than may be available, bits maybe
           > ld->len, deliver 0 than */
        if (ld->len >= bits)
            return ((ld->bufa >> (ld->len - bits)) & (0xFFFFFFFF >> (32 - bits)));
        else
            return ((ld->bufa << (bits - ld->len)) & (0xFFFFFFFF >> (32 - bits)));
    } else {
        if ((ld->len - bits) < 32)
        {
            return ( (ld->bufb & (0xFFFFFFFF >> (64 - ld->len))) << (bits - ld->len + 32)) |
                (ld->bufa >> (ld->len - bits));
        } else {
            return ((ld->bufb >> (ld->len - bits - 32)) & (0xFFFFFFFF >> (32 - bits)));
        }
    }
}

/* return 1 if position is outside of buffer, 0 otherwise */
static INLINE int8_t flushbits_hcr( bits_t *ld, uint8_t bits)
{
    ld->len -= bits;

    if (ld->len <0)
    {
        ld->len = 0;
        return 1;
    } else {
        return 0;
    }
}

static INLINE int8_t getbits_hcr(bits_t *ld, uint8_t n, uint32_t *result)
{
    *result = showbits_hcr(ld, n);
    return flushbits_hcr(ld, n);
}

static INLINE int8_t get1bit_hcr(bits_t *ld, uint8_t *result)
{
    uint32_t res;
    int8_t ret;

    ret = getbits_hcr(ld, 1, &res);
    *result = (int8_t)(res & 1);
    return ret;
}

#endif


#ifdef __cplusplus
}
#endif
#endif
