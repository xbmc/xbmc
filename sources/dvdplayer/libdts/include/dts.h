/*
 * dts.h
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 *
 * This file is part of dtsdec, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/dtsdec.html for updates.
 *
 * dtsdec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dtsdec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DTS_H
#define DTS_H

/* x86 accelerations */
#define MM_ACCEL_X86_MMX	0x80000000
#define MM_ACCEL_X86_3DNOW	0x40000000
#define MM_ACCEL_X86_MMXEXT	0x20000000

uint32_t mm_accel (void);

#if defined(LIBDTS_FIXED)
typedef int32_t sample_t;
typedef int32_t level_t;
#elif defined(LIBDTS_DOUBLE)
typedef double sample_t;
typedef double level_t;
#else
typedef float sample_t;
typedef float level_t;
#endif

typedef struct dts_state_s dts_state_t;

#define DTS_MONO 0
#define DTS_CHANNEL 1
#define DTS_STEREO 2
#define DTS_STEREO_SUMDIFF 3
#define DTS_STEREO_TOTAL 4
#define DTS_3F 5
#define DTS_2F1R 6
#define DTS_3F1R 7
#define DTS_2F2R 8
#define DTS_3F2R 9
#define DTS_4F2R 10

#define DTS_DOLBY 101 /* FIXME */

#define DTS_CHANNEL_MAX  DTS_3F2R /* We don't handle anything above that */
#define DTS_CHANNEL_BITS 6
#define DTS_CHANNEL_MASK 0x3F

#define DTS_LFE 0x80
#define DTS_ADJUST_LEVEL 0x100

dts_state_t * dts_init (uint32_t mm_accel);

int dts_syncinfo (dts_state_t *state, uint8_t * buf, int * flags,
                  int * sample_rate, int * bit_rate, int *frame_length);

int dts_frame (dts_state_t * state, uint8_t * buf, int * flags,
               level_t * level, sample_t bias);

void dts_dynrng (dts_state_t * state,
                 level_t (* call) (level_t, void *), void * data);

int dts_blocks_num (dts_state_t * state);
int dts_block (dts_state_t * state);

sample_t * dts_samples (dts_state_t * state);

void dts_free (dts_state_t * state);

#endif /* DTS_H */
