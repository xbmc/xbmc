/* 
 * layer1.c: Mpeg Layer-1 audio decoder 
 *
 * Copyright (C) 1999-2010 The L.A.M.E. project
 *
 * Initially written by Michael Hipp, see also AUTHORS and README.
 *  
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: layer1.c,v 1.23.2.2 2010/03/22 14:17:14 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include "common.h"
#include "decode_i386.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "layer1.h"

static void
I_step_one(PMPSTR mp, unsigned int balloc[], unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
    unsigned int *ba = balloc;
    unsigned int *sca = (unsigned int *) scale_index;

    assert(fr->stereo == 1 || fr->stereo == 2);
    if (fr->stereo == 2) {
        int     i;
        int     jsbound = fr->jsbound;
        for (i = 0; i < jsbound; i++) {
            *ba++ = getbits(mp, 4);
            *ba++ = getbits(mp, 4);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            *ba++ = getbits(mp, 4);

        ba = balloc;

        for (i = 0; i < jsbound; i++) {
            if ((*ba++))
                *sca++ = getbits(mp, 6);
            if ((*ba++))
                *sca++ = getbits(mp, 6);
        }
        for (i = jsbound; i < SBLIMIT; i++)
            if ((*ba++)) {
                *sca++ = getbits(mp, 6);
                *sca++ = getbits(mp, 6);
            }
    }
    else {
        int     i;
        for (i = 0; i < SBLIMIT; i++)
            *ba++ = getbits(mp, 4);
        ba = balloc;
        for (i = 0; i < SBLIMIT; i++)
            if ((*ba++))
                *sca++ = getbits(mp, 6);
    }
}

static void
I_step_two(PMPSTR mp, real fraction[2][SBLIMIT], unsigned int balloc[2 * SBLIMIT],
           unsigned int scale_index[2][SBLIMIT], struct frame *fr)
{
    int     i, n;
    int     smpb[2 * SBLIMIT]; /* values: 0-65535 */
    int    *sample;
    unsigned int *ba;
    unsigned int *sca = (unsigned int *) scale_index;

    assert(fr->stereo == 1 || fr->stereo == 2);
    if (fr->stereo == 2) {
        int     jsbound = fr->jsbound;
        real   *f0 = fraction[0];
        real   *f1 = fraction[1];
        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            n = *ba++;
            if (n)
                *sample++ = getbits(mp, n + 1);
            n = *ba++;
            if (n)
                *sample++ = getbits(mp, n + 1);
        }
        for (i = jsbound; i < SBLIMIT; i++) {
            n = *ba++;
            if (n)
                *sample++ = getbits(mp, n + 1);
        }
        ba = balloc;
        for (sample = smpb, i = 0; i < jsbound; i++) {
            n = *ba++;
            if (n)
                *f0++ = (real) (((-1) << n) + (*sample++) + 1) * muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
            n = *ba++;
            if (n)
                *f1++ = (real) (((-1) << n) + (*sample++) + 1) * muls[n + 1][*sca++];
            else
                *f1++ = 0.0;
        }
        for (i = jsbound; i < SBLIMIT; i++) {
            n = *ba++;
            if (n) {
                real    samp = (real) (((-1) << n) + (*sample++) + 1);
                *f0++ = samp * muls[n + 1][*sca++];
                *f1++ = samp * muls[n + 1][*sca++];
            }
            else
                *f0++ = *f1++ = 0.0;
        }
        for (i = fr->down_sample_sblimit; i < 32; i++)
            fraction[0][i] = fraction[1][i] = 0.0;
    }
    else {
        real   *f0 = fraction[0];
        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++) {
            n = *ba++;
            if (n)
                *sample++ = getbits(mp, n + 1);
        }
        ba = balloc;
        for (sample = smpb, i = 0; i < SBLIMIT; i++) {
            n = *ba++;
            if (n)
                *f0++ = (real) (((-1) << n) + (*sample++) + 1) * muls[n + 1][*sca++];
            else
                *f0++ = 0.0;
        }
        for (i = fr->down_sample_sblimit; i < 32; i++)
            fraction[0][i] = 0.0;
    }
}

/*int do_layer1(struct frame *fr,int outmode,struct audio_info_struct *ai) */
int
do_layer1(PMPSTR mp, unsigned char *pcm_sample, int *pcm_point)
{
    int     clip = 0;
    unsigned int balloc[2 * SBLIMIT];
    unsigned int scale_index[2][SBLIMIT];
    real    fraction[2][SBLIMIT];
    struct frame *fr = &(mp->fr);
    int     i, stereo = fr->stereo;
    int     single = fr->single;

    fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ? (fr->mode_ext << 2) + 4 : 32;

    if (stereo == 1 || single == 3)
        single = 0;

    I_step_one(mp, balloc, scale_index, fr);

    for (i = 0; i < SCALE_BLOCK; i++) {
        I_step_two(mp, fraction, balloc, scale_index, fr);

        if (single >= 0) {
            clip += synth_1to1_mono(mp, (real *) fraction[single], pcm_sample, pcm_point);
        }
        else {
            int     p1 = *pcm_point;
            clip += synth_1to1(mp, (real *) fraction[0], 0, pcm_sample, &p1);
            clip += synth_1to1(mp, (real *) fraction[1], 1, pcm_sample, pcm_point);
        }
    }

    return clip;
}
