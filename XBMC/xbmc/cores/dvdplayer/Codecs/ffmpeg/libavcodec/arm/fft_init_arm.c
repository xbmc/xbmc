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

#include "libavcodec/dsputil.h"

void ff_fft_permute_neon(FFTContext *s, FFTComplex *z);
void ff_fft_calc_neon(FFTContext *s, FFTComplex *z);

void ff_imdct_calc_neon(FFTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_neon(FFTContext *s, FFTSample *output, const FFTSample *input);
void ff_mdct_calc_neon(FFTContext *s, FFTSample *output, const FFTSample *input);

av_cold void ff_fft_init_arm(FFTContext *s)
{
    if (HAVE_NEON) {
        s->fft_permute  = ff_fft_permute_neon;
        s->fft_calc     = ff_fft_calc_neon;
        s->imdct_calc   = ff_imdct_calc_neon;
        s->imdct_half   = ff_imdct_half_neon;
        s->mdct_calc    = ff_mdct_calc_neon;
        s->permutation  = FF_MDCT_PERM_INTERLEAVE;
    }
}
