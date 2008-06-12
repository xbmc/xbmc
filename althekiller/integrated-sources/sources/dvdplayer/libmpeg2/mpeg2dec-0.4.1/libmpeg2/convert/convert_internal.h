/*
 * convert_internal.h
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

typedef struct {
    uint8_t * rgb_ptr;
    int width;
    int y_stride, rgb_stride, y_increm, uv_increm, rgb_increm;
    int chroma420, convert420;
    int dither_offset, dither_stride;
    int y_stride_frame, uv_stride_frame, rgb_stride_frame, rgb_stride_min;
} convert_rgb_t;

typedef void mpeg2convert_copy_t (void * id, uint8_t * const * src,
				  unsigned int v_offset);

mpeg2convert_copy_t * mpeg2convert_rgb_mmxext (int bpp, int mode,
					       const mpeg2_sequence_t * seq);
mpeg2convert_copy_t * mpeg2convert_rgb_mmx (int bpp, int mode,
					    const mpeg2_sequence_t * seq);
mpeg2convert_copy_t * mpeg2convert_rgb_vis (int bpp, int mode,
					    const mpeg2_sequence_t * seq);
