/*
 * video_out_pgm.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 2003      Regis Duchesne <hpreg@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * MD5 code derived from linux kernel GPL implementation
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "video_out.h"
#include "vo_internal.h"

typedef struct pgm_instance_s {
    vo_instance_t vo;
    int framenum;
    int width;
    int height;
    int chroma_width;
    int chroma_height;
    char header[1024];
    void (* writer) (struct pgm_instance_s *, uint8_t *, size_t);
    FILE * file;
    uint32_t md5_hash[4];
    uint32_t md5_block[16];
    uint32_t md5_bytes;
} pgm_instance_t;

static void file_writer (pgm_instance_t * instance, uint8_t * ptr, size_t size)
{
    fwrite (ptr, size, 1, instance->file);
}

static void internal_draw_frame (pgm_instance_t * instance,
				 uint8_t * const * buf)
{
    static uint8_t black[16384] = { 0 };
    int i;

    instance->writer (instance, (uint8_t *)instance->header,
		      strlen (instance->header));
    for (i = 0; i < instance->height; i++) {
	instance->writer (instance, buf[0] + i * instance->width,
			  instance->width);
	instance->writer (instance, black,
			  2 * instance->chroma_width - instance->width);
    }
    for (i = 0; i < instance->chroma_height; i++) {
	instance->writer (instance, buf[1] + i * instance->chroma_width,
			  instance->chroma_width);
	instance->writer (instance, buf[2] + i * instance->chroma_width,
			  instance->chroma_width);
    }
}

static void pgm_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance = (pgm_instance_t *) _instance;
    char filename[128];

    sprintf (filename, "%d.pgm", instance->framenum++);
    instance->file = fopen (filename, "wb");
    if (instance->file == NULL)
	return;
    internal_draw_frame (instance, buf);
    fclose (instance->file);
}

static int pgm_setup (vo_instance_t * _instance, unsigned int width,
		      unsigned int height, unsigned int chroma_width,
		      unsigned int chroma_height, vo_setup_result_t * result)
{
    pgm_instance_t * instance;

    instance = (pgm_instance_t *) _instance;

    /*
     * Layout of the Y, U, and V buffers in our pgm image
     *
     *      YY        YY        YY
     * 420: YY   422: YY   444: YY
     *      UV        UV        UUVV
     *                UV        UUVV
     */
    if (width > 2 * chroma_width)
	return 1;

    instance->width = width;
    instance->height = height;
    instance->chroma_width = chroma_width;
    instance->chroma_height = chroma_height;
    sprintf (instance->header, "P5\n%d %d\n255\n", 2 * chroma_width,
	     height + chroma_height);
    result->convert = NULL;
    return 0;
}

static vo_instance_t * internal_open (void draw (vo_instance_t *,
						 uint8_t * const *, void *),
				      void writer (pgm_instance_t *,
						   uint8_t *, size_t))
{
    pgm_instance_t * instance;

    instance = (pgm_instance_t *) malloc (sizeof (pgm_instance_t));
    if (instance == NULL)
        return NULL;

    instance->vo.setup = pgm_setup;
    instance->vo.setup_fbuf = NULL;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = NULL;
    instance->vo.draw = draw;
    instance->vo.discard = NULL;
    instance->vo.close = (void (*) (vo_instance_t *)) free;
    instance->framenum = 0;
    instance->writer = writer;
    instance->file = stdout;

    return (vo_instance_t *) instance;
}

vo_instance_t * vo_pgm_open (void)
{
    return internal_open (pgm_draw_frame, file_writer);
}

static void pgmpipe_draw_frame (vo_instance_t * _instance,
				uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance = (pgm_instance_t *) _instance;
    internal_draw_frame (instance, buf);
}

vo_instance_t * vo_pgmpipe_open (void)
{
    return internal_open (pgmpipe_draw_frame, file_writer);
}

#define F1(x,y,z)	(z ^ (x & (y ^ z)))
#define F2(x,y,z)	F1 (z, x, y)
#define F3(x,y,z)	(x ^ y ^ z)
#define F4(x,y,z)	(y ^ (x | ~z))

#define MD5STEP(f,w,x,y,z,in,s) do {					\
    w += f (x, y, z) + in; w = ((w << s) | (w >> (32 - s))) + x;	\
} while (0)

static void md5_transform (uint32_t * hash, uint32_t const * in)
{
    uint32_t a, b, c, d;

    a = hash[0];
    b = hash[1];
    c = hash[2];
    d = hash[3];

    MD5STEP (F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP (F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP (F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP (F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP (F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP (F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP (F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP (F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP (F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP (F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP (F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP (F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP (F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP (F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP (F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP (F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP (F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP (F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP (F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP (F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP (F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP (F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP (F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP (F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP (F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP (F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP (F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP (F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP (F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP (F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP (F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP (F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP (F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP (F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP (F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP (F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP (F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP (F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP (F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP (F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP (F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP (F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP (F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP (F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP (F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP (F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP (F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP (F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP (F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP (F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP (F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP (F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP (F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP (F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP (F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP (F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP (F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP (F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP (F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP (F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP (F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP (F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP (F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP (F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
}

static inline uint32_t swap (uint32_t x) 
{
    return (((x & 0xff) << 24) | ((x & 0xff00) << 8) |
	    ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24));
}

static inline void little_endian (uint32_t * buf, unsigned int words)
{
#ifdef WORDS_BIGENDIAN
    while (words--)
	buf[words] = swap (buf[words]);
#endif
}

static void md5_writer (pgm_instance_t * instance, uint8_t * ptr, size_t size)
{
    const unsigned int offset = instance->md5_bytes & 0x3f;

    instance->md5_bytes += size;

    if (offset + size < 64) {
	memcpy ((char *)instance->md5_block + offset, ptr, size);
	return;
    } else if (offset) {
	const int avail = 64 - offset;
	memcpy ((char *)instance->md5_block + offset, ptr, avail);
	little_endian (instance->md5_block, 16);
	md5_transform (instance->md5_hash, instance->md5_block);
	ptr += avail;
	size -= avail;
    }

    while (size >= 64) {
#ifndef ARCH_X86
	memcpy (instance->md5_block, ptr, 64);
	little_endian (instance->md5_block, 16);
	md5_transform (instance->md5_hash, instance->md5_block);
#else
	md5_transform (instance->md5_hash, (uint32_t *)ptr);
#endif
	ptr += 64;
	size -= 64;
    }

    memcpy (instance->md5_block, ptr, size);
}

static void md5_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    pgm_instance_t * instance = (pgm_instance_t *) _instance;
    unsigned int offset;
    uint8_t * p;
    int padding;

    instance->md5_hash[0] = 0x67452301;
    instance->md5_hash[1] = 0xefcdab89;
    instance->md5_hash[2] = 0x98badcfe;
    instance->md5_hash[3] = 0x10325476;
    instance->md5_bytes = 0;

    internal_draw_frame (instance, buf);

    offset = instance->md5_bytes & 0x3f;
    p = (uint8_t *)instance->md5_block + offset;
    padding = 55 - offset;

    *p++ = 0x80;
    if (padding < 0) {
	memset (p, 0, padding + 8);
	little_endian (instance->md5_block, 16);
	md5_transform (instance->md5_hash, instance->md5_block);
	p = (uint8_t *)instance->md5_block;
	padding = 56;
    }

    memset (p, 0, padding);
    instance->md5_block[14] = instance->md5_bytes << 3;
    instance->md5_block[15] = instance->md5_bytes >> 29;
    little_endian (instance->md5_block, 14);
    md5_transform (instance->md5_hash, instance->md5_block);

    printf ("%08x%08x%08x%08x *%d.pgm\n", swap (instance->md5_hash[0]),
	    swap (instance->md5_hash[1]) , swap (instance->md5_hash[2]),
	    swap (instance->md5_hash[3]), instance->framenum++);
}

vo_instance_t * vo_md5_open (void)
{
    return internal_open (md5_draw_frame, md5_writer);
}
