/*
 * rgb_vis.c
 * Copyright (C) 2003 David S. Miller <davem@redhat.com>
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

#ifdef ARCH_SPARC

#include <stddef.h>
#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2convert.h"
#include "convert_internal.h"
#include "attributes.h"
#include "vis.h"

/* Based partially upon the MMX yuv2rgb code, see there for credits.
 *
 * The difference here is that since we have enough registers we
 * process both even and odd scanlines in one pass.
 */

static const uint16_t const_2048[] ATTR_ALIGN(8) = {2048, 2048, 2048, 2048};
static const uint16_t const_1024[] ATTR_ALIGN(8) = {1024, 1024, 1024, 1024};
static const uint16_t const_128[] ATTR_ALIGN(8) = {128, 128, 128, 128};
static const uint8_t const_Ugreen[] ATTR_ALIGN(8) =
	{0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00};
static const uint8_t const_Vgreen[] ATTR_ALIGN(8) =
	{0xe6, 0x00, 0xe6, 0x00, 0xe6, 0x00, 0xe6, 0x00};
static const uint8_t const_Ublue_Vred[] ATTR_ALIGN(8) =
	{0x41, 0x41, 0x41, 0x41, 0x33, 0x33, 0x33, 0x33};
static const uint8_t const_Ycoeff[] ATTR_ALIGN(4) = {0x25, 0x25, 0x25, 0x25};

#define TMP0		0
#define TMP1		1
#define TMP2		2
#define TMP3		3
#define TMP4		4
#define TMP5		5
#define TMP6		6
#define TMP7		7
#define TMP8		8
#define TMP9		9
#define TMP10		10
#define TMP11		11
#define TMP12		12
#define TMP13		13

#define CONST_UBLUE	14
#define CONST_VRED	15
#define CONST_2048	16

#define BLUE8_EVEN	18
#define BLUE8_ODD	19
#define RED8_EVEN	20
#define RED8_ODD	21
#define GREEN8_EVEN	22
#define GREEN8_ODD	23

#define BLUE8_2_EVEN	24
#define BLUE8_2_ODD	25
#define RED8_2_EVEN	26
#define RED8_2_ODD	27
#define GREEN8_2_EVEN	28
#define GREEN8_2_ODD	29

#define CONST_YCOEFF	30
#define ZEROS		31

#define PU_0		32
#define PU_2		34
#define PV_0		36
#define PV_2		38
#define PY_0		40
#define PY_2		42
#define PY_4		44
#define PY_6		46

#define CONST_128	56
#define CONST_1024	58
#define CONST_VGREEN	60
#define CONST_UGREEN	62

static inline void vis_init_consts(void)
{
	vis_set_gsr(7 << VIS_GSR_SCALEFACT_SHIFT);

	vis_ld64(const_2048[0], CONST_2048);
	vis_ld64(const_1024[0], CONST_1024);
	vis_ld64(const_Ugreen[0], CONST_UGREEN);
	vis_ld64(const_Vgreen[0], CONST_VGREEN);
	vis_fzeros(ZEROS);
	vis_ld64(const_Ublue_Vred[0], CONST_UBLUE);
	vis_ld32(const_Ycoeff[0], CONST_YCOEFF);
	vis_ld64(const_128[0],  CONST_128);
}

static inline void vis_yuv2rgb(uint8_t *py, uint8_t *pu, uint8_t *pv,
			       int y_stride)
{
	vis_ld32(pu[0], TMP0);

	vis_ld32(pv[0], TMP2);

	vis_ld64(py[0], TMP4);
	vis_mul8x16au(TMP0, CONST_2048, PU_0);

	vis_ld64_2(py, y_stride, TMP8);
	vis_mul8x16au(TMP2, CONST_2048, PV_0);

	vis_pmerge(TMP4, TMP5, TMP6);

	vis_pmerge(TMP6, TMP7, TMP4);

	vis_pmerge(TMP8, TMP9, TMP10);

	vis_pmerge(TMP10, TMP11, TMP8);
	vis_mul8x16au(TMP4, CONST_2048, PY_0);

	vis_psub16(PU_0, CONST_1024, PU_0);
	vis_mul8x16au(TMP5, CONST_2048, PY_2);

	vis_psub16(PV_0, CONST_1024, PV_0);
	vis_mul8x16au(TMP8, CONST_2048, PY_4);

	vis_psub16(PY_0, CONST_128, PY_0);
	vis_mul8x16au(TMP9, CONST_2048, PY_6);

	vis_psub16(PY_2, CONST_128, PY_2);
	vis_mul8x16(CONST_YCOEFF, PY_0, PY_0);

	vis_psub16(PY_4, CONST_128, PY_4);
	vis_mul8x16(CONST_YCOEFF, PY_2, PY_2);

	vis_psub16(PY_6, CONST_128, PY_6);
	vis_mul8x16(CONST_YCOEFF, PY_4, PY_4);

	vis_mul8x16(CONST_YCOEFF, PY_6, PY_6);

	vis_mul8sux16(CONST_UGREEN, PU_0, TMP0);

	vis_mul8sux16(CONST_VGREEN, PV_0, TMP2);

	vis_mul8x16(CONST_UBLUE, PU_0, TMP4);

	vis_mul8x16(CONST_VRED, PV_0, TMP6);
	vis_padd16(TMP0, TMP2, TMP10);

	vis_padd16(PY_0, TMP4, TMP0);

	vis_padd16(PY_2, TMP4, TMP2);
	vis_pack16(TMP0, BLUE8_EVEN);

	vis_padd16(PY_4, TMP4, TMP0);
	vis_pack16(TMP2, BLUE8_ODD);

	vis_padd16(PY_6, TMP4, TMP2);
	vis_pack16(TMP0, BLUE8_2_EVEN);

	vis_padd16(PY_0, TMP6, TMP0);
	vis_pack16(TMP2, BLUE8_2_ODD);

	vis_padd16(PY_2, TMP6, TMP2);
	vis_pack16(TMP0, RED8_EVEN);

	vis_padd16(PY_4, TMP6, TMP0);
	vis_pack16(TMP2, RED8_ODD);

	vis_padd16(PY_6, TMP6, TMP2);
	vis_pack16(TMP0, RED8_2_EVEN);

	vis_padd16(PY_0, TMP10, TMP0);
	vis_pack16(TMP2, RED8_2_ODD);

	vis_padd16(PY_2, TMP10, TMP2);
	vis_pack16(TMP0, GREEN8_EVEN);

	vis_padd16(PY_4, TMP10, TMP0);
	vis_pack16(TMP2, GREEN8_ODD);

	vis_padd16(PY_6, TMP10, TMP2);
	vis_pack16(TMP0, GREEN8_2_EVEN);

	vis_pack16(TMP2, GREEN8_2_ODD);
	vis_pmerge(BLUE8_EVEN, BLUE8_ODD, BLUE8_EVEN);

	vis_pmerge(BLUE8_2_EVEN, BLUE8_2_ODD, BLUE8_2_EVEN);

	vis_pmerge(RED8_EVEN, RED8_ODD, RED8_EVEN);

	vis_pmerge(RED8_2_EVEN, RED8_2_ODD, RED8_2_EVEN);

	vis_pmerge(GREEN8_EVEN, GREEN8_ODD, GREEN8_EVEN);

	vis_pmerge(GREEN8_2_EVEN, GREEN8_2_ODD, GREEN8_2_EVEN);
}

static inline void vis_unpack_32rgb(uint8_t *image, int stride)
{
	vis_pmerge(ZEROS, GREEN8_EVEN, TMP0);
	vis_pmerge(RED8_EVEN, BLUE8_EVEN, TMP2);

	vis_pmerge(TMP0, TMP2, TMP4);
	vis_st64(TMP4, image[0]);

	vis_pmerge(TMP1, TMP3, TMP6);
	vis_st64_2(TMP6, image, 8);

	vis_pmerge(ZEROS, GREEN8_ODD, TMP8);
	vis_pmerge(RED8_ODD, BLUE8_ODD, TMP10);

	vis_pmerge(TMP8, TMP10, TMP0);
	vis_st64_2(TMP0, image, 16);

	vis_pmerge(TMP9, TMP11, TMP2);
	vis_st64_2(TMP2, image, 24);

	image += stride;

	vis_pmerge(ZEROS, GREEN8_2_EVEN, TMP0);
	vis_pmerge(RED8_2_EVEN, BLUE8_2_EVEN, TMP2);

	vis_pmerge(TMP0, TMP2, TMP4);
	vis_st64(TMP4, image[0]);

	vis_pmerge(TMP1, TMP3, TMP6);
	vis_st64_2(TMP6, image, 8);

	vis_pmerge(ZEROS, GREEN8_2_ODD, TMP8);
	vis_pmerge(RED8_2_ODD, BLUE8_2_ODD, TMP10);

	vis_pmerge(TMP8, TMP10, TMP0);
	vis_st64_2(TMP0, image, 16);

	vis_pmerge(TMP9, TMP11, TMP2);
	vis_st64_2(TMP2, image, 24);
}

static inline void vis_unpack_32bgr(uint8_t *image, int stride)
{
	vis_pmerge(ZEROS, GREEN8_EVEN, TMP0);
	vis_pmerge(BLUE8_EVEN, RED8_EVEN, TMP2);

	vis_pmerge(TMP0, TMP2, TMP4);
	vis_st64(TMP4, image[0]);

	vis_pmerge(TMP1, TMP3, TMP6);
	vis_st64_2(TMP6, image, 8);

	vis_pmerge(ZEROS, GREEN8_ODD, TMP8);
	vis_pmerge(BLUE8_ODD, RED8_ODD, TMP10);

	vis_pmerge(TMP8, TMP10, TMP0);
	vis_st64_2(TMP0, image, 16);

	vis_pmerge(TMP9, TMP11, TMP2);
	vis_st64_2(TMP2, image, 24);

	image += stride;

	vis_pmerge(ZEROS, GREEN8_2_EVEN, TMP0);
	vis_pmerge(BLUE8_2_EVEN, RED8_2_EVEN, TMP2);

	vis_pmerge(TMP0, TMP2, TMP4);
	vis_st64(TMP4, image[0]);

	vis_pmerge(TMP1, TMP3, TMP6);
	vis_st64_2(TMP6, image, 8);

	vis_pmerge(ZEROS, GREEN8_2_ODD, TMP8);
	vis_pmerge(BLUE8_2_ODD, RED8_2_ODD, TMP10);

	vis_pmerge(TMP8, TMP10, TMP0);
	vis_st64_2(TMP0, image, 16);

	vis_pmerge(TMP9, TMP11, TMP2);
	vis_st64_2(TMP2, image, 24);
}

static inline void vis_yuv420_argb32(uint8_t *image,
				     uint8_t *py, uint8_t *pu, uint8_t *pv,
				     int width, int height, int rgb_stride,
				     int y_stride, int uv_stride)
{
	height >>= 1;
	uv_stride -= width >> 1;
	do {
		int i = width >> 3;
		do {
			vis_yuv2rgb(py, pu, pv, y_stride);
			vis_unpack_32rgb(image, rgb_stride);
			py += 8;
			pu += 4;
			pv += 4;
			image += 32;
		} while (--i);

		py    += (y_stride << 1) - width;
		image += (rgb_stride << 1) - 4 * width;
		pu    += uv_stride;
		pv    += uv_stride;
	} while (--height);
}

static inline void vis_yuv420_abgr32(uint8_t *image,
				     uint8_t *py, uint8_t *pu, uint8_t *pv,
				     int width, int height, int rgb_stride,
				     int y_stride, int uv_stride)
{
	height >>= 1;
	uv_stride -= width >> 1;
	do {
		int i = width >> 3;
		do {
			vis_yuv2rgb(py, pu, pv, y_stride);
			vis_unpack_32bgr(image, rgb_stride);
			py += 8;
			pu += 4;
			pv += 4;
			image += 32;
		} while (--i);

		py    += (y_stride << 1) - width;
		image += (rgb_stride << 1) - 4 * width;
		pu    += uv_stride;
		pv    += uv_stride;
	} while (--height);
}

static void vis_argb32(void *_id, uint8_t * const *src,
		       unsigned int v_offset)
{
	convert_rgb_t *id = (convert_rgb_t *) _id;

	vis_init_consts();
	vis_yuv420_argb32(id->rgb_ptr + id->rgb_stride * v_offset,
			  src[0], src[1], src[2], id->width, 16,
			  id->rgb_stride, id->y_stride, id->y_stride >> 1);
}

static void vis_abgr32(void *_id, uint8_t * const *src,
		       unsigned int v_offset)
{
	convert_rgb_t *id = (convert_rgb_t *) _id;

	vis_init_consts();
	vis_yuv420_abgr32(id->rgb_ptr + id->rgb_stride * v_offset,
			  src[0], src[1], src[2], id->width, 16,
			  id->rgb_stride, id->y_stride, id->y_stride >> 1);
}

mpeg2convert_copy_t *mpeg2convert_rgb_vis(int order, int bpp,
					  const mpeg2_sequence_t * seq)
{
	if (bpp == 32 && seq->chroma_height < seq->height) {
		if (order == MPEG2CONVERT_RGB)
			return vis_argb32;
		if (order == MPEG2CONVERT_BGR)
			return vis_abgr32;
	}

	return NULL;	/* Fallback to C */
}

#endif /* ARCH_SPARC */
