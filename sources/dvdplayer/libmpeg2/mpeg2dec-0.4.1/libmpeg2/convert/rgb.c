/*
 * rgb.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
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
#include "attributes.h"

#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2convert.h"
#include "convert_internal.h"

static int matrix_coefficients = 6;

static const int Inverse_Table_6_9[8][4] = {
    {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
    {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
    {104597, 132201, 25675, 53279}, /* unspecified */
    {104597, 132201, 25675, 53279}, /* reserved */
    {104448, 132798, 24759, 53109}, /* FCC */
    {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
    {104597, 132201, 25675, 53279}, /* SMPTE 170M */
    {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};

static const uint8_t dither[] ATTR_ALIGN(32) = {
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35,
     3,  9, 27, 63,  1,  4, 25, 59,  5, 12, 28, 67,  3,  7, 26, 62,
     3,  9, 27, 63,  1,  4, 25, 59,  5, 12, 28, 67,  3,  7, 26, 62,
     3,  9, 27, 63,  1,  4, 25, 59,  5, 12, 28, 67,  3,  7, 26, 62,
     3,  9, 27, 63,  1,  4, 25, 59,  5, 12, 28, 67,  3,  7, 26, 62,
    19, 45, 11, 27, 17, 41,  9, 22, 21, 49, 13, 30, 19, 44, 11, 26,
    19, 45, 11, 27, 17, 41,  9, 22, 21, 49, 13, 30, 19, 44, 11, 26,
    19, 45, 11, 27, 17, 41,  9, 22, 21, 49, 13, 30, 19, 44, 11, 26,
    19, 45, 11, 27, 17, 41,  9, 22, 21, 49, 13, 30, 19, 44, 11, 26,
     0,  2, 24, 57,  6, 15, 30, 70,  0,  1, 23, 55,  6, 14, 29, 69,
     0,  2, 24, 57,  6, 15, 30, 70,  0,  1, 23, 55,  6, 14, 29, 69,
     0,  2, 24, 57,  6, 15, 30, 70,  0,  1, 23, 55,  6, 14, 29, 69,
     0,  2, 24, 57,  6, 15, 30, 70,  0,  1, 23, 55,  6, 14, 29, 69,
    16, 38,  8, 20, 22, 52, 14, 34, 16, 37,  8, 19, 21, 51, 14, 33,
    16, 38,  8, 20, 22, 52, 14, 34, 16, 37,  8, 19, 21, 51, 14, 33,
    16, 38,  8, 20, 22, 52, 14, 34, 16, 37,  8, 19, 21, 51, 14, 33,
    16, 38,  8, 20, 22, 52, 14, 34, 16, 37,  8, 19, 21, 51, 14, 33,
     4, 11, 28, 66,  2,  6, 26, 61,  4, 10, 27, 65,  2,  5, 25, 60,
     4, 11, 28, 66,  2,  6, 26, 61,  4, 10, 27, 65,  2,  5, 25, 60,
     4, 11, 28, 66,  2,  6, 26, 61,  4, 10, 27, 65,  2,  5, 25, 60,
     4, 11, 28, 66,  2,  6, 26, 61,  4, 10, 27, 65,  2,  5, 25, 60,
    20, 47, 12, 29, 18, 43, 10, 25, 20, 46, 12, 28, 18, 42, 10, 23,
    20, 47, 12, 29, 18, 43, 10, 25, 20, 46, 12, 28, 18, 42, 10, 23,
    20, 47, 12, 29, 18, 43, 10, 25, 20, 46, 12, 28, 18, 42, 10, 23,
    20, 47, 12, 29, 18, 43, 10, 25, 20, 46, 12, 28, 18, 42, 10, 23,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
     0,  0, 23, 54,  5, 13, 29, 68,  1,  3, 24, 58,  7, 17, 30, 71,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35,
    15, 36,  7, 18, 21, 50, 13, 31, 17, 39,  9, 21, 22, 53, 15, 35
};

static const uint8_t dither_temporal[64] = {
    0x00, 0x20, 0x21, 0x01, 0x40, 0x60, 0x61, 0x41,
    0x42, 0x62, 0x63, 0x43, 0x02, 0x22, 0x23, 0x03,
    0x80, 0xa0, 0xa1, 0x81, 0xc0, 0xe0, 0xe1, 0xc1,
    0xc2, 0xe2, 0xe3, 0xc3, 0x82, 0xa2, 0xa3, 0x83,
    0x84, 0xa4, 0xa5, 0x85, 0xc4, 0xe4, 0xe5, 0xc5,
    0xc6, 0xe6, 0xe7, 0xc7, 0x86, 0xa6, 0xa7, 0x87,
    0x04, 0x24, 0x25, 0x05, 0x44, 0x64, 0x65, 0x45,
    0x46, 0x66, 0x67, 0x47, 0x06, 0x26, 0x27, 0x07
};

typedef struct {
    convert_rgb_t base;
    void * table_rV[256];
    void * table_gU[256];
    int table_gV[256];
    void * table_bU[256];
} convert_rgb_c_t;

#define RGB(type,i)							\
    U = pu[i];								\
    V = pv[i];								\
    r = (type *) id->table_rV[V];					\
    g = (type *) (((uint8_t *)id->table_gU[U]) + id->table_gV[V]);	\
    b = (type *) id->table_bU[U];

#define DST(py,dst,i,j)			\
    Y = py[i];				\
    dst[i] = r[Y] + g[Y] + b[Y];

#define DSTRGB(py,dst,i,j)					\
    Y = py[i];							\
    dst[3*i] = r[Y]; dst[3*i+1] = g[Y]; dst[3*i+2] = b[Y];

#define DSTBGR(py,dst,i,j)					\
    Y = py[i];							\
    dst[3*i] = b[Y]; dst[3*i+1] = g[Y]; dst[3*i+2] = r[Y];

#define DSTDITHER(py,dst,i,j)						  \
    Y = py[i];								  \
    dst[i] = r[Y+pd[2*i+96*j]] + g[Y-pd[2*i+96*j]] + b[Y+pd[2*i+1+96*j]];

#define DO(x) x
#define SKIP(x)

#define DECLARE_420(func,type,num,DST,DITHER)				\
static void func (void * _id, uint8_t * const * src,			\
		  unsigned int v_offset)				\
{									\
    const convert_rgb_c_t * const id = (convert_rgb_c_t *) _id;		\
    type * dst_1;							\
    const uint8_t * py_1, * pu, * pv;					\
    int i;								\
    DITHER(uint8_t dithpos = id->base.dither_offset;)			\
									\
    dst_1 = (type *)(id->base.rgb_ptr + id->base.rgb_stride * v_offset);\
    py_1 = src[0];	pu = src[1];	pv = src[2];			\
									\
    i = 8;								\
    do {								\
	const uint8_t * py_2;						\
	int j, U, V, Y;							\
	const type * r, * g, * b;					\
	type * dst_2;							\
	DITHER(const uint8_t * const pd = dither + 2 * dithpos;)	\
									\
	dst_2 = (type *)((char *)dst_1 + id->base.rgb_stride);		\
	py_2 = py_1 + id->base.y_stride;				\
	j = id->base.width;						\
	do {								\
	    RGB (type, 0)						\
	    DST (py_1, dst_1, 0, 0)					\
	    DST (py_1, dst_1, 1, 0)					\
	    DST (py_2, dst_2, 0, 1)					\
	    DST (py_2, dst_2, 1, 1)					\
									\
	    RGB (type, 1)						\
	    DST (py_2, dst_2, 2, 1)					\
	    DST (py_2, dst_2, 3, 1)					\
	    DST (py_1, dst_1, 2, 0)					\
	    DST (py_1, dst_1, 3, 0)					\
									\
	    RGB (type, 2)						\
	    DST (py_1, dst_1, 4, 0)					\
	    DST (py_1, dst_1, 5, 0)					\
	    DST (py_2, dst_2, 4, 1)					\
	    DST (py_2, dst_2, 5, 1)					\
									\
	    RGB (type, 3)						\
	    DST (py_2, dst_2, 6, 1)					\
	    DST (py_2, dst_2, 7, 1)					\
	    DST (py_1, dst_1, 6, 0)					\
	    DST (py_1, dst_1, 7, 0)					\
									\
	    pu += 4;							\
	    pv += 4;							\
	    py_1 += 8;							\
	    py_2 += 8;							\
	    dst_1 += 8 * num;						\
	    dst_2 += 8 * num;						\
	} while (--j);							\
	py_1 += id->base.y_increm;					\
	pu += id->base.uv_increm;					\
	pv += id->base.uv_increm;					\
	dst_1 = (type *)((char *)dst_1 + id->base.rgb_increm);		\
	DITHER(dithpos += id->base.dither_stride;)			\
    } while (--i);							\
}

DECLARE_420 (rgb_c_32_420, uint32_t, 1, DST, SKIP)
DECLARE_420 (rgb_c_24_rgb_420, uint8_t, 3, DSTRGB, SKIP)
DECLARE_420 (rgb_c_24_bgr_420, uint8_t, 3, DSTBGR, SKIP)
DECLARE_420 (rgb_c_16_420, uint16_t, 1, DST, SKIP)
DECLARE_420 (rgb_c_8_420, uint8_t, 1, DSTDITHER, DO)

#define DECLARE_422(func,type,num,DST,DITHER)				\
static void func (void * _id, uint8_t * const * src,			\
		  unsigned int v_offset)				\
{									\
    const convert_rgb_c_t * const id = (convert_rgb_c_t *) _id;		\
    type * dst;								\
    const uint8_t * py, * pu, * pv;					\
    int i;								\
    DITHER(uint8_t dithpos = id->base.dither_offset;)			\
									\
    dst = (type *)(id->base.rgb_ptr + id->base.rgb_stride * v_offset);	\
    py = src[0];	pu = src[1];	pv = src[2];			\
									\
    i = 16;								\
    do {								\
	int j, U, V, Y;							\
	const type * r, * g, * b;					\
	DITHER(const uint8_t * const pd = dither + 2 * dithpos;)	\
									\
	j = id->base.width;						\
	do {								\
	    RGB (type, 0)						\
	    DST (py, dst, 0, 0)						\
	    DST (py, dst, 1, 0)						\
									\
	    RGB (type, 1)						\
	    DST (py, dst, 2, 0)						\
	    DST (py, dst, 3, 0)						\
									\
	    RGB (type, 2)						\
	    DST (py, dst, 4, 0)						\
	    DST (py, dst, 5, 0)						\
									\
	    RGB (type, 3)						\
	    DST (py, dst, 6, 0)						\
	    DST (py, dst, 7, 0)						\
									\
	    pu += 4;							\
	    pv += 4;							\
	    py += 8;							\
	    dst += 8 * num;						\
	} while (--j);							\
	py += id->base.y_increm;					\
	pu += id->base.uv_increm;					\
	pv += id->base.uv_increm;					\
	dst = (type *)((char *)dst + id->base.rgb_increm);		\
	DITHER(dithpos += id->base.dither_stride;)			\
    } while (--i);							\
}

DECLARE_422 (rgb_c_32_422, uint32_t, 1, DST, SKIP)
DECLARE_422 (rgb_c_24_rgb_422, uint8_t, 3, DSTRGB, SKIP)
DECLARE_422 (rgb_c_24_bgr_422, uint8_t, 3, DSTBGR, SKIP)
DECLARE_422 (rgb_c_16_422, uint16_t, 1, DST, SKIP)
DECLARE_422 (rgb_c_8_422, uint8_t, 1, DSTDITHER, DO)

#define DECLARE_444(func,type,num,DST,DITHER)				\
static void func (void * _id, uint8_t * const * src,			\
		  unsigned int v_offset)				\
{									\
    const convert_rgb_c_t * const id = (convert_rgb_c_t *) _id;		\
    type * dst;								\
    const uint8_t * py, * pu, * pv;					\
    int i;								\
    DITHER(uint8_t dithpos = id->base.dither_offset;)			\
									\
    dst = (type *)(id->base.rgb_ptr + id->base.rgb_stride * v_offset);	\
    py = src[0];	pu = src[1];	pv = src[2];			\
									\
    i = 16;								\
    do {								\
	int j, U, V, Y;							\
	const type * r, * g, * b;					\
	DITHER(const uint8_t * const pd = dither + 2 * dithpos;)	\
									\
	j = id->base.width;						\
	do {								\
	    RGB (type, 0)						\
	    DST (py, dst, 0, 0)						\
	    RGB (type, 1)						\
	    DST (py, dst, 1, 0)						\
	    RGB (type, 2)						\
	    DST (py, dst, 2, 0)						\
	    RGB (type, 3)						\
	    DST (py, dst, 3, 0)						\
	    RGB (type, 4)						\
	    DST (py, dst, 4, 0)						\
	    RGB (type, 5)						\
	    DST (py, dst, 5, 0)						\
	    RGB (type, 6)						\
	    DST (py, dst, 6, 0)						\
	    RGB (type, 7)						\
	    DST (py, dst, 7, 0)						\
									\
	    pu += 8;							\
	    pv += 8;							\
	    py += 8;							\
	    dst += 8 * num;						\
	} while (--j);							\
	py += id->base.y_increm;				   	\
	pu += id->base.y_increm;				   	\
	pv += id->base.y_increm;				   	\
	dst = (type *)((char *)dst + id->base.rgb_increm);		\
	DITHER(dithpos += id->base.dither_stride;)			\
    } while (--i);							\
}

DECLARE_444 (rgb_c_32_444, uint32_t, 1, DST, SKIP)
DECLARE_444 (rgb_c_24_rgb_444, uint8_t, 3, DSTRGB, SKIP)
DECLARE_444 (rgb_c_24_bgr_444, uint8_t, 3, DSTBGR, SKIP)
DECLARE_444 (rgb_c_16_444, uint16_t, 1, DST, SKIP)
DECLARE_444 (rgb_c_8_444, uint8_t, 1, DSTDITHER, DO)

static void rgb_start (void * _id, const mpeg2_fbuf_t * fbuf,
		       const mpeg2_picture_t * picture,
		       const mpeg2_gop_t * gop)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;
    int uv_stride = id->uv_stride_frame;
    id->y_stride = id->y_stride_frame;
    id->rgb_ptr = fbuf->buf[0];
    id->rgb_stride = id->rgb_stride_frame;
    id->dither_stride = 32;
    id->dither_offset = dither_temporal[picture->temporal_reference & 63];
    if (picture->nb_fields == 1) {
	uv_stride <<= 1;
	id->y_stride <<= 1;
	id->rgb_stride <<= 1;
	id->dither_stride <<= 1;
	id->dither_offset += 16;
	if (!(picture->flags & PIC_FLAG_TOP_FIELD_FIRST)) {
	    id->rgb_ptr += id->rgb_stride_frame;
	    id->dither_offset += 32;
	}
    }
    id->y_increm = (id->y_stride << id->convert420) - id->y_stride_frame;
    id->uv_increm = uv_stride - id->uv_stride_frame;
    id->rgb_increm = (id->rgb_stride << id->convert420) - id->rgb_stride_min;
    id->dither_stride <<= id->convert420;
}

static inline int div_round (int dividend, int divisor)
{
    if (dividend > 0)
	return (dividend + (divisor>>1)) / divisor;
    else
	return -((-dividend + (divisor>>1)) / divisor);
}

static unsigned int rgb_c_init (convert_rgb_c_t * id,
				mpeg2convert_rgb_order_t order,
				unsigned int bpp)
{
    int i;
    uint8_t table_Y[1024];
    uint32_t * table_32 = 0;
    uint16_t * table_16 = 0;
    uint8_t * table_8 = 0;
    uint8_t * table_332 = 0;
    int entry_size = 0;
    void * table_r = 0;
    void * table_g = 0;
    void * table_b = 0;

    int crv = Inverse_Table_6_9[matrix_coefficients][0];
    int cbu = Inverse_Table_6_9[matrix_coefficients][1];
    int cgu = -Inverse_Table_6_9[matrix_coefficients][2];
    int cgv = -Inverse_Table_6_9[matrix_coefficients][3];

    for (i = 0; i < 1024; i++) {
	int j;

	j = (76309 * (i - 384 - 16) + 32768) >> 16;
	table_Y[i] = (j < 0) ? 0 : ((j > 255) ? 255 : j);
    }

    switch (bpp) {
    case 32:
	if (!id)
	    return (197 + 2*682 + 256 + 132) * sizeof (uint32_t);
	table_32 = (uint32_t *) (id + 1);
	entry_size = sizeof (uint32_t);
	table_r = table_32 + 197;
	table_b = table_32 + 197 + 685;
	table_g = table_32 + 197 + 2*682;

	for (i = -197; i < 256+197; i++)
	    ((uint32_t *) table_r)[i] =
		table_Y[i+384] << ((order == MPEG2CONVERT_RGB) ? 16 : 0);
	for (i = -132; i < 256+132; i++)
	    ((uint32_t *) table_g)[i] = table_Y[i+384] << 8;
	for (i = -232; i < 256+232; i++)
	    ((uint32_t *) table_b)[i] =
		table_Y[i+384] << ((order == MPEG2CONVERT_RGB) ? 0 : 16);
	break;

    case 24:
	if (!id)
	    return (256 + 2*232) * sizeof (uint8_t);
	table_8 = (uint8_t *) (id + 1);
	entry_size = sizeof (uint8_t);
	table_r = table_g = table_b = table_8 + 232;

	for (i = -232; i < 256+232; i++)
	    ((uint8_t * )table_b)[i] = table_Y[i+384];
	break;

    case 15:
    case 16:
	if (!id)
	    return (197 + 2*682 + 256 + 132) * sizeof (uint16_t);
	table_16 = (uint16_t *) (id + 1);
	entry_size = sizeof (uint16_t);
	table_r = table_16 + 197;
	table_b = table_16 + 197 + 685;
	table_g = table_16 + 197 + 2*682;

	for (i = -197; i < 256+197; i++) {
	    int j = table_Y[i+384] >> 3;

	    if (order == MPEG2CONVERT_RGB)
		j <<= ((bpp==16) ? 11 : 10);

	    ((uint16_t *)table_r)[i] = j;
	}
	for (i = -132; i < 256+132; i++) {
	    int j = table_Y[i+384] >> ((bpp==16) ? 2 : 3);

	    ((uint16_t *)table_g)[i] = j << 5;
	}
	for (i = -232; i < 256+232; i++) {
	    int j = table_Y[i+384] >> 3;

	    if (order == MPEG2CONVERT_BGR)
		j <<= ((bpp==16) ? 11 : 10);

	    ((uint16_t *)table_b)[i] = j;
	}
	break;

    case 8:
	if (!id)
	    return (197 + 2*682 + 256 + 232 + 71) * sizeof (uint8_t);
	table_332 = (uint8_t *) (id + 1);
	entry_size = sizeof (uint8_t);
	table_r = table_332 + 197;
	table_g = table_332 + 197 + 682 + 30;
	table_b = table_332 + 197 + 2*682;

	for (i = -197; i < 256+197+30; i++)
	    ((uint8_t *)table_r)[i] = ((table_Y[i+384] * 7 / 255) <<
				       (order == MPEG2CONVERT_RGB ? 5 : 0));
	for (i = -132; i < 256+132+30; i++)
	    ((uint8_t *)table_g)[i-30] = ((table_Y[i+384] * 7 / 255) <<
					  (order == MPEG2CONVERT_RGB ? 2 : 3));
	for (i = -232; i < 256+232+71; i++)
	    ((uint8_t *)table_b)[i] = ((table_Y[i+384] / 85) <<
				       (order == MPEG2CONVERT_RGB ? 0 : 6));
	break;
    }

    for (i = 0; i < 256; i++) {
	id->table_rV[i] = (((uint8_t *)table_r) +
			   entry_size * div_round (crv * (i-128), 76309));
	id->table_gU[i] = (((uint8_t *)table_g) +
			   entry_size * div_round (cgu * (i-128), 76309));
	id->table_gV[i] = entry_size * div_round (cgv * (i-128), 76309);
	id->table_bU[i] = (((uint8_t *)table_b) +
			   entry_size * div_round (cbu * (i-128), 76309));
    }

    return 0;
}

static int rgb_internal (mpeg2convert_rgb_order_t order, unsigned int bpp,
			 int stage, void * _id, const mpeg2_sequence_t * seq,
			 int stride, uint32_t accel, void * arg,
			 mpeg2_convert_init_t * result)
{
    convert_rgb_t * id = (convert_rgb_t *) _id;
    mpeg2convert_copy_t * copy = (mpeg2convert_copy_t *) 0;
    unsigned int id_size = sizeof (convert_rgb_t);
    int chroma420 = (seq->chroma_height < seq->height);
    int convert420 = 0;
    int rgb_stride_min = ((bpp + 7) >> 3) * seq->width;

#ifdef ARCH_X86
    if (!copy && (accel & MPEG2_ACCEL_X86_MMXEXT)) {
	convert420 = 0;
	copy = mpeg2convert_rgb_mmxext (order, bpp, seq);
    }
    if (!copy && (accel & MPEG2_ACCEL_X86_MMX)) {
	convert420 = 0;
	copy = mpeg2convert_rgb_mmx (order, bpp, seq);
    }
#endif
#ifdef ARCH_SPARC
    if (!copy && (accel & MPEG2_ACCEL_SPARC_VIS)) {
	convert420 = chroma420;
	copy = mpeg2convert_rgb_vis (order, bpp, seq);
    }
#endif
    if (!copy) {
	int src, dest;
	static void (* rgb_c[3][5]) (void *, uint8_t * const *,
				     unsigned int) =
	    {{rgb_c_24_bgr_420, rgb_c_8_420, rgb_c_16_420,
	      rgb_c_24_rgb_420, rgb_c_32_420},
	     {rgb_c_24_bgr_422, rgb_c_8_422, rgb_c_16_422,
	      rgb_c_24_rgb_422, rgb_c_32_422},
	     {rgb_c_24_bgr_444, rgb_c_8_444, rgb_c_16_444,
	      rgb_c_24_rgb_444, rgb_c_32_444}};

	convert420 = chroma420;
	id_size = (sizeof (convert_rgb_c_t) +
		   rgb_c_init ((convert_rgb_c_t *) id, order, bpp));
	src = ((seq->chroma_width == seq->width) +
	       (seq->chroma_height == seq->height));
	dest = ((bpp == 24 && order == MPEG2CONVERT_BGR) ? 0 : (bpp + 7) >> 3);
	copy = rgb_c[src][dest];
    }

    result->id_size = id_size;

    if (stride < rgb_stride_min)
	stride = rgb_stride_min;

    if (stage == MPEG2_CONVERT_STRIDE)
	return stride;
    else if (stage == MPEG2_CONVERT_START) {
	id->width = seq->width >> 3;
	id->y_stride_frame = seq->width;
	id->uv_stride_frame = seq->chroma_width;
	id->rgb_stride_frame = stride;
	id->rgb_stride_min = rgb_stride_min;
	id->chroma420 = chroma420;
	id->convert420 = convert420;
	result->buf_size[0] = stride * seq->height;
	result->buf_size[1] = result->buf_size[2] = 0;
	result->start = rgb_start;
	result->copy = copy;
    }
    return 0;
}

#define DECLARE(func,order,bpp)						\
int func (int stage, void * id,						\
	  const mpeg2_sequence_t * sequence, int stride,		\
	  uint32_t accel, void * arg, mpeg2_convert_init_t * result)	\
{									\
    return rgb_internal (order, bpp, stage, id, sequence, stride,	\
			 accel, arg, result);				\
}

DECLARE (mpeg2convert_rgb32, MPEG2CONVERT_RGB, 32)
DECLARE (mpeg2convert_rgb24, MPEG2CONVERT_RGB, 24)
DECLARE (mpeg2convert_rgb16, MPEG2CONVERT_RGB, 16)
DECLARE (mpeg2convert_rgb15, MPEG2CONVERT_RGB, 15)
DECLARE (mpeg2convert_rgb8, MPEG2CONVERT_RGB, 8)
DECLARE (mpeg2convert_bgr32, MPEG2CONVERT_BGR, 32)
DECLARE (mpeg2convert_bgr24, MPEG2CONVERT_BGR, 24)
DECLARE (mpeg2convert_bgr16, MPEG2CONVERT_BGR, 16)
DECLARE (mpeg2convert_bgr15, MPEG2CONVERT_BGR, 15)
DECLARE (mpeg2convert_bgr8, MPEG2CONVERT_BGR, 8)

mpeg2_convert_t * mpeg2convert_rgb (mpeg2convert_rgb_order_t order,
				    unsigned int bpp)
{
    static mpeg2_convert_t * table[5][2] =
	{{mpeg2convert_rgb15, mpeg2convert_bgr15},
	 {mpeg2convert_rgb8, mpeg2convert_bgr8},
	 {mpeg2convert_rgb16, mpeg2convert_bgr16},
	 {mpeg2convert_rgb24, mpeg2convert_bgr24},
	 {mpeg2convert_rgb32, mpeg2convert_bgr32}};

    if (order == MPEG2CONVERT_RGB || order == MPEG2CONVERT_BGR) {
	if (bpp == 15)
	    return table[0][order == MPEG2CONVERT_BGR];
	else if (bpp >= 8 && bpp <= 32 && (bpp & 7) == 0)
	    return table[bpp >> 3][order == MPEG2CONVERT_BGR];
    }
    return (mpeg2_convert_t *) 0;
}
