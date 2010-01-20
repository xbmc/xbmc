/*
 * audio_out_internal.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef LIBDTS_DOUBLE
typedef float convert_t;
#else
typedef sample_t convert_t;
#endif

#ifdef LIBDTS_FIXED
#define CONVERT_LEVEL (1 << 26)
#define CONVERT_BIAS 0
#else
#define CONVERT_LEVEL 1
#define CONVERT_BIAS 384
#endif

void convert2s16_1 (convert_t * f, int16_t * s16);
void convert2s16_2 (convert_t * f, int16_t * s16);
void convert2s16_3 (convert_t * f, int16_t * s16);
void convert2s16_4 (convert_t * f, int16_t * s16);
void convert2s16_5 (convert_t * f, int16_t * s16);
int channels_multi (int flags);
void convert2s16_multi (convert_t * f, int16_t * s16, int flags);
void convert2s16_wav (convert_t * f, int16_t * s16, int flags);
void s16_swap (int16_t * s16, int channels);
void s32_swap (int32_t * s32, int channels);

#ifdef WORDS_BIGENDIAN
#define s16_LE(s16,channels) s16_swap (s16, channels)
#define s16_BE(s16,channels) do {} while (0)
#define s32_LE(s32,channels) s32_swap (s32, channels)
#define s32_BE(s32,channels) do {} while (0)
#else
#define s16_LE(s16,channels) do {} while (0)
#define s16_BE(s16,channels) s16_swap (s16, channels)
#define s32_LE(s32,channels) do {} while (0)
#define s32_BE(s32,channels) s32_swap (s32, channels)
#endif
