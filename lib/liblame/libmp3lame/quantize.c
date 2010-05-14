/*
 * MP3 quantization
 *
 *      Copyright (c) 1999-2000 Mark Taylor
 *      Copyright (c) 1999-2003 Takehiro Tominaga
 *      Copyright (c) 2000-2007 Robert Hegemann
 *      Copyright (c) 2001-2005 Gabriel Bouvigne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: quantize.c,v 1.201.2.1 2008/08/05 14:16:07 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "quantize_pvt.h"
#include "lame_global_flags.h"
#include "reservoir.h"
#include "bitstream.h"
#include "vbrquantize.h"
#include "quantize.h"
#ifdef HAVE_XMMINTRIN_H
#include "vector/lame_intrin.h"
#endif




/* convert from L/R <-> Mid/Side */
static void
ms_convert(III_side_info_t * l3_side, int gr)
{
    int     i;
    for (i = 0; i < 576; ++i) {
        FLOAT   l, r;
        l = l3_side->tt[gr][0].xr[i];
        r = l3_side->tt[gr][1].xr[i];
        l3_side->tt[gr][0].xr[i] = (l + r) * (FLOAT) (SQRT2 * 0.5);
        l3_side->tt[gr][1].xr[i] = (l - r) * (FLOAT) (SQRT2 * 0.5);
    }
}

/************************************************************************
 *
 *      init_outer_loop()
 *  mt 6/99
 *
 *  initializes cod_info, scalefac and xrpow
 *
 *  returns 0 if all energies in xr are zero, else 1
 *
 ************************************************************************/

static void
init_xrpow_core_c(gr_info * const cod_info, FLOAT xrpow[576], int upper, FLOAT * sum)
{
    int     i;
    FLOAT   tmp;
    *sum = 0;
    for (i = 0; i <= upper; ++i) {
        tmp = fabs(cod_info->xr[i]);
        *sum += tmp;
        xrpow[i] = sqrt(tmp * sqrt(tmp));

        if (xrpow[i] > cod_info->xrpow_max)
            cod_info->xrpow_max = xrpow[i];
    }
}





void
init_xrpow_core_init(lame_internal_flags * const gfc)
{
    gfc->init_xrpow_core = init_xrpow_core_c;

#if defined(HAVE_XMMINTRIN_H)
    if (gfc->CPU_features.SSE)
        gfc->init_xrpow_core = init_xrpow_core_sse;
#endif
}



static int
init_xrpow(lame_internal_flags * gfc, gr_info * const cod_info, FLOAT xrpow[576])
{
    FLOAT   sum = 0;
    int     i;
    int const upper = cod_info->max_nonzero_coeff;

    assert(xrpow != NULL);
    cod_info->xrpow_max = 0;

    /*  check if there is some energy we have to quantize
     *  and calculate xrpow matching our fresh scalefactors
     */
    assert(0 <= upper && upper <= 575);
    memset(&(xrpow[upper]), 0, (576 - upper) * sizeof(xrpow[0]));


    gfc->init_xrpow_core(cod_info, xrpow, upper, &sum);

    /*  return 1 if we have something to quantize, else 0
     */
    if (sum > (FLOAT) 1E-20) {
        int     j = 0;
        if (gfc->substep_shaping & 2)
            j = 1;

        for (i = 0; i < cod_info->psymax; i++)
            gfc->pseudohalf[i] = j;

        return 1;
    }

    memset(&cod_info->l3_enc[0], 0, sizeof(int) * 576);
    return 0;
}





extern FLOAT athAdjust(FLOAT a, FLOAT x, FLOAT athFloor);



/*
Gabriel Bouvigne feb/apr 2003
Analog silence detection in partitionned sfb21
or sfb12 for short blocks

From top to bottom of sfb, changes to 0
coeffs which are below ath. It stops on the first
coeff higher than ath.
*/
static void
psfb21_analogsilence(lame_internal_flags const *gfc, gr_info * const cod_info)
{
    ATH_t const *const ATH = gfc->ATH;
    FLOAT  *const xr = cod_info->xr;

    if (cod_info->block_type != SHORT_TYPE) { /* NORM, START or STOP type, but not SHORT blocks */
        int     gsfb;
        int     stop = 0;
        for (gsfb = PSFB21 - 1; gsfb >= 0 && !stop; gsfb--) {
            int const start = gfc->scalefac_band.psfb21[gsfb];
            int const end = gfc->scalefac_band.psfb21[gsfb + 1];
            int     j;
            FLOAT   ath21;
            ath21 = athAdjust(ATH->adjust, ATH->psfb21[gsfb], ATH->floor);

            if (gfc->nsPsy.longfact[21] > 1e-12f)
                ath21 *= gfc->nsPsy.longfact[21];

            for (j = end - 1; j >= start; j--) {
                if (fabs(xr[j]) < ath21)
                    xr[j] = 0;
                else {
                    stop = 1;
                    break;
                }
            }
        }
    }
    else {
        /*note: short blocks coeffs are reordered */
        int     block;
        for (block = 0; block < 3; block++) {

            int     gsfb;
            int     stop = 0;
            for (gsfb = PSFB12 - 1; gsfb >= 0 && !stop; gsfb--) {
                int const start = gfc->scalefac_band.s[12] * 3 +
                    (gfc->scalefac_band.s[13] - gfc->scalefac_band.s[12]) * block +
                    (gfc->scalefac_band.psfb12[gsfb] - gfc->scalefac_band.psfb12[0]);
                int const end =
                    start + (gfc->scalefac_band.psfb12[gsfb + 1] - gfc->scalefac_band.psfb12[gsfb]);
                int     j;
                FLOAT   ath12;
                ath12 = athAdjust(ATH->adjust, ATH->psfb12[gsfb], ATH->floor);

                if (gfc->nsPsy.shortfact[12] > 1e-12f)
                    ath12 *= gfc->nsPsy.shortfact[12];

                for (j = end - 1; j >= start; j--) {
                    if (fabs(xr[j]) < ath12)
                        xr[j] = 0;
                    else {
                        stop = 1;
                        break;
                    }
                }
            }
        }
    }

}





static void
init_outer_loop(lame_internal_flags const *gfc, gr_info * const cod_info)
{
    int     sfb, j;
    /*  initialize fresh cod_info
     */
    cod_info->part2_3_length = 0;
    cod_info->big_values = 0;
    cod_info->count1 = 0;
    cod_info->global_gain = 210;
    cod_info->scalefac_compress = 0;
    /* mixed_block_flag, block_type was set in psymodel.c */
    cod_info->table_select[0] = 0;
    cod_info->table_select[1] = 0;
    cod_info->table_select[2] = 0;
    cod_info->subblock_gain[0] = 0;
    cod_info->subblock_gain[1] = 0;
    cod_info->subblock_gain[2] = 0;
    cod_info->subblock_gain[3] = 0; /* this one is always 0 */
    cod_info->region0_count = 0;
    cod_info->region1_count = 0;
    cod_info->preflag = 0;
    cod_info->scalefac_scale = 0;
    cod_info->count1table_select = 0;
    cod_info->part2_length = 0;
    cod_info->sfb_lmax = SBPSY_l;
    cod_info->sfb_smin = SBPSY_s;
    cod_info->psy_lmax = gfc->sfb21_extra ? SBMAX_l : SBPSY_l;
    cod_info->psymax = cod_info->psy_lmax;
    cod_info->sfbmax = cod_info->sfb_lmax;
    cod_info->sfbdivide = 11;
    for (sfb = 0; sfb < SBMAX_l; sfb++) {
        cod_info->width[sfb]
            = gfc->scalefac_band.l[sfb + 1] - gfc->scalefac_band.l[sfb];
        cod_info->window[sfb] = 3; /* which is always 0. */
    }
    if (cod_info->block_type == SHORT_TYPE) {
        FLOAT   ixwork[576];
        FLOAT  *ix;

        cod_info->sfb_smin = 0;
        cod_info->sfb_lmax = 0;
        if (cod_info->mixed_block_flag) {
            /*
             *  MPEG-1:      sfbs 0-7 long block, 3-12 short blocks
             *  MPEG-2(.5):  sfbs 0-5 long block, 3-12 short blocks
             */
            cod_info->sfb_smin = 3;
            cod_info->sfb_lmax = gfc->mode_gr * 2 + 4;
        }
        cod_info->psymax
            = cod_info->sfb_lmax
            + 3 * ((gfc->sfb21_extra ? SBMAX_s : SBPSY_s) - cod_info->sfb_smin);
        cod_info->sfbmax = cod_info->sfb_lmax + 3 * (SBPSY_s - cod_info->sfb_smin);
        cod_info->sfbdivide = cod_info->sfbmax - 18;
        cod_info->psy_lmax = cod_info->sfb_lmax;
        /* re-order the short blocks, for more efficient encoding below */
        /* By Takehiro TOMINAGA */
        /*
           Within each scalefactor band, data is given for successive
           time windows, beginning with window 0 and ending with window 2.
           Within each window, the quantized values are then arranged in
           order of increasing frequency...
         */
        ix = &cod_info->xr[gfc->scalefac_band.l[cod_info->sfb_lmax]];
        memcpy(ixwork, cod_info->xr, 576 * sizeof(FLOAT));
        for (sfb = cod_info->sfb_smin; sfb < SBMAX_s; sfb++) {
            int const start = gfc->scalefac_band.s[sfb];
            int const end = gfc->scalefac_band.s[sfb + 1];
            int     window, l;
            for (window = 0; window < 3; window++) {
                for (l = start; l < end; l++) {
                    *ix++ = ixwork[3 * l + window];
                }
            }
        }

        j = cod_info->sfb_lmax;
        for (sfb = cod_info->sfb_smin; sfb < SBMAX_s; sfb++) {
            cod_info->width[j] = cod_info->width[j + 1] = cod_info->width[j + 2]
                = gfc->scalefac_band.s[sfb + 1] - gfc->scalefac_band.s[sfb];
            cod_info->window[j] = 0;
            cod_info->window[j + 1] = 1;
            cod_info->window[j + 2] = 2;
            j += 3;
        }
    }

    cod_info->count1bits = 0;
    cod_info->sfb_partition_table = nr_of_sfb_block[0][0];
    cod_info->slen[0] = 0;
    cod_info->slen[1] = 0;
    cod_info->slen[2] = 0;
    cod_info->slen[3] = 0;

    cod_info->max_nonzero_coeff = 575;

    /*  fresh scalefactors are all zero
     */
    memset(cod_info->scalefac, 0, sizeof(cod_info->scalefac));

    psfb21_analogsilence(gfc, cod_info);
}



/************************************************************************
 *
 *      bin_search_StepSize()
 *
 *  author/date??
 *
 *  binary step size search
 *  used by outer_loop to get a quantizer step size to start with
 *
 ************************************************************************/

typedef enum {
    BINSEARCH_NONE,
    BINSEARCH_UP,
    BINSEARCH_DOWN
} binsearchDirection_t;

static int
bin_search_StepSize(lame_internal_flags * const gfc, gr_info * const cod_info,
                    int desired_rate, const int ch, const FLOAT xrpow[576])
{
    int     nBits;
    int     CurrentStep = gfc->CurrentStep[ch];
    int     flag_GoneOver = 0;
    int const start = gfc->OldValue[ch];
    binsearchDirection_t Direction = BINSEARCH_NONE;
    cod_info->global_gain = start;
    desired_rate -= cod_info->part2_length;

    assert(CurrentStep);
    for (;;) {
        int     step;
        nBits = count_bits(gfc, xrpow, cod_info, 0);

        if (CurrentStep == 1 || nBits == desired_rate)
            break;      /* nothing to adjust anymore */

        if (nBits > desired_rate) {
            /* increase Quantize_StepSize */
            if (Direction == BINSEARCH_DOWN)
                flag_GoneOver = 1;

            if (flag_GoneOver)
                CurrentStep /= 2;
            Direction = BINSEARCH_UP;
            step = CurrentStep;
        }
        else {
            /* decrease Quantize_StepSize */
            if (Direction == BINSEARCH_UP)
                flag_GoneOver = 1;

            if (flag_GoneOver)
                CurrentStep /= 2;
            Direction = BINSEARCH_DOWN;
            step = -CurrentStep;
        }
        cod_info->global_gain += step;
        if (cod_info->global_gain < 0) {
            cod_info->global_gain = 0;
            flag_GoneOver = 1;
        }
        if (cod_info->global_gain > 255) {
            cod_info->global_gain = 255;
            flag_GoneOver = 1;
        }
    }

    assert(cod_info->global_gain >= 0);
    assert(cod_info->global_gain < 256);

    while (nBits > desired_rate && cod_info->global_gain < 255) {
        cod_info->global_gain++;
        nBits = count_bits(gfc, xrpow, cod_info, 0);
    }
    gfc->CurrentStep[ch] = (start - cod_info->global_gain >= 4) ? 4 : 2;
    gfc->OldValue[ch] = cod_info->global_gain;
    cod_info->part2_3_length = nBits;
    return nBits;
}




/************************************************************************
 *
 *      trancate_smallspectrums()
 *
 *  Takehiro TOMINAGA 2002-07-21
 *
 *  trancate smaller nubmers into 0 as long as the noise threshold is allowed.
 *
 ************************************************************************/
static int
floatcompare(const void *v1, const void *v2)
{
    const FLOAT *const a = v1, *const b = v2;
    if (*a > *b)
        return 1;
    if (*a < *b)
        return -1;
    return 0;
}

void
trancate_smallspectrums(lame_internal_flags const *gfc,
                        gr_info * const gi, const FLOAT * const l3_xmin, FLOAT * const work)
{
    int     sfb, j, width;
    FLOAT   distort[SFBMAX];
    calc_noise_result dummy;

    if ((!(gfc->substep_shaping & 4) && gi->block_type == SHORT_TYPE)
        || gfc->substep_shaping & 0x80)
        return;
    (void) calc_noise(gi, l3_xmin, distort, &dummy, 0);
    for (j = 0; j < 576; j++) {
        FLOAT   xr = 0.0;
        if (gi->l3_enc[j] != 0)
            xr = fabs(gi->xr[j]);
        work[j] = xr;
    }

    j = 0;
    sfb = 8;
    if (gi->block_type == SHORT_TYPE)
        sfb = 6;
    do {
        FLOAT   allowedNoise, trancateThreshold;
        int     nsame, start;

        width = gi->width[sfb];
        j += width;
        if (distort[sfb] >= 1.0)
            continue;

        qsort(&work[j - width], width, sizeof(FLOAT), floatcompare);
        if (EQ(work[j - 1], 0.0))
            continue;   /* all zero sfb */

        allowedNoise = (1.0 - distort[sfb]) * l3_xmin[sfb];
        trancateThreshold = 0.0;
        start = 0;
        do {
            FLOAT   noise;
            for (nsame = 1; start + nsame < width; nsame++)
                if (NEQ(work[start + j - width], work[start + j + nsame - width]))
                    break;

            noise = work[start + j - width] * work[start + j - width] * nsame;
            if (allowedNoise < noise) {
                if (start != 0)
                    trancateThreshold = work[start + j - width - 1];
                break;
            }
            allowedNoise -= noise;
            start += nsame;
        } while (start < width);
        if (EQ(trancateThreshold, 0.0))
            continue;

/*      printf("%e %e %e\n", */
/*             trancateThreshold/l3_xmin[sfb], */
/*             trancateThreshold/(l3_xmin[sfb]*start), */
/*             trancateThreshold/(l3_xmin[sfb]*(start+width)) */
/*          ); */
/*      if (trancateThreshold > 1000*l3_xmin[sfb]*start) */
/*          trancateThreshold = 1000*l3_xmin[sfb]*start; */

        do {
            if (fabs(gi->xr[j - width]) <= trancateThreshold)
                gi->l3_enc[j - width] = 0;
        } while (--width > 0);
    } while (++sfb < gi->psymax);

    gi->part2_3_length = noquant_count_bits(gfc, gi, 0);
}


/*************************************************************************
 *
 *      loop_break()
 *
 *  author/date??
 *
 *  Function: Returns zero if there is a scalefac which has not been
 *            amplified. Otherwise it returns one.
 *
 *************************************************************************/

inline static int
loop_break(const gr_info * const cod_info)
{
    int     sfb;

    for (sfb = 0; sfb < cod_info->sfbmax; sfb++)
        if (cod_info->scalefac[sfb]
            + cod_info->subblock_gain[cod_info->window[sfb]] == 0)
            return 0;

    return 1;
}




/*  mt 5/99:  Function: Improved calc_noise for a single channel   */

/*************************************************************************
 *
 *      quant_compare()
 *
 *  author/date??
 *
 *  several different codes to decide which quantization is better
 *
 *************************************************************************/

static double
penalties(double noise)
{
    return FAST_LOG10(0.368 + 0.632 * noise * noise * noise);
}

static double
get_klemm_noise(const FLOAT * distort, const gr_info * const gi)
{
    int     sfb;
    double  klemm_noise = 1E-37;
    for (sfb = 0; sfb < gi->psymax; sfb++)
        klemm_noise += penalties(distort[sfb]);

    return Max(1e-20, klemm_noise);
}

inline static int
quant_compare(const int quant_comp,
              const calc_noise_result * const best,
              calc_noise_result * const calc, const gr_info * const gi, const FLOAT * distort)
{
    /*
       noise is given in decibels (dB) relative to masking thesholds.

       over_noise:  ??? (the previous comment is fully wrong)
       tot_noise:   ??? (the previous comment is fully wrong)
       max_noise:   max quantization noise

     */
    int     better;

    switch (quant_comp) {
    default:
    case 9:{
            if (best->over_count > 0) {
                /* there are distorted sfb */
                better = calc->over_SSD <= best->over_SSD;
                if (calc->over_SSD == best->over_SSD)
                    better = calc->bits < best->bits;
            }
            else {
                /* no distorted sfb */
                better = ((calc->max_noise < 0) &&
                          ((calc->max_noise * 10 + calc->bits) <=
                           (best->max_noise * 10 + best->bits)));
            }
            break;
        }

    case 0:
        better = calc->over_count < best->over_count
            || (calc->over_count == best->over_count && calc->over_noise < best->over_noise)
            || (calc->over_count == best->over_count &&
                EQ(calc->over_noise, best->over_noise) && calc->tot_noise < best->tot_noise);
        break;

    case 8:
        calc->max_noise = get_klemm_noise(distort, gi);
        /*lint --fallthrough */
    case 1:
        better = calc->max_noise < best->max_noise;
        break;
    case 2:
        better = calc->tot_noise < best->tot_noise;
        break;
    case 3:
        better = (calc->tot_noise < best->tot_noise)
            && (calc->max_noise < best->max_noise);
        break;
    case 4:
        better = (calc->max_noise <= 0.0 && best->max_noise > 0.2)
            || (calc->max_noise <= 0.0 &&
                best->max_noise < 0.0 &&
                best->max_noise > calc->max_noise - 0.2 && calc->tot_noise < best->tot_noise)
            || (calc->max_noise <= 0.0 &&
                best->max_noise > 0.0 &&
                best->max_noise > calc->max_noise - 0.2 &&
                calc->tot_noise < best->tot_noise + best->over_noise)
            || (calc->max_noise > 0.0 &&
                best->max_noise > -0.05 &&
                best->max_noise > calc->max_noise - 0.1 &&
                calc->tot_noise + calc->over_noise < best->tot_noise + best->over_noise)
            || (calc->max_noise > 0.0 &&
                best->max_noise > -0.1 &&
                best->max_noise > calc->max_noise - 0.15 &&
                calc->tot_noise + calc->over_noise + calc->over_noise <
                best->tot_noise + best->over_noise + best->over_noise);
        break;
    case 5:
        better = calc->over_noise < best->over_noise
            || (EQ(calc->over_noise, best->over_noise) && calc->tot_noise < best->tot_noise);
        break;
    case 6:
        better = calc->over_noise < best->over_noise
            || (EQ(calc->over_noise, best->over_noise) &&
                (calc->max_noise < best->max_noise
                 || (EQ(calc->max_noise, best->max_noise) && calc->tot_noise <= best->tot_noise)
                ));
        break;
    case 7:
        better = calc->over_count < best->over_count || calc->over_noise < best->over_noise;
        break;
    }


    if (best->over_count == 0) {
        /*
           If no distorted bands, only use this quantization
           if it is better, and if it uses less bits.
           Unfortunately, part2_3_length is sometimes a poor
           estimator of the final size at low bitrates.
         */
        better = better && calc->bits < best->bits;
    }


    return better;
}



/*************************************************************************
 *
 *          amp_scalefac_bands()
 *
 *  author/date??
 *
 *  Amplify the scalefactor bands that violate the masking threshold.
 *  See ISO 11172-3 Section C.1.5.4.3.5
 *
 *  distort[] = noise/masking
 *  distort[] > 1   ==> noise is not masked
 *  distort[] < 1   ==> noise is masked
 *  max_dist = maximum value of distort[]
 *
 *  Three algorithms:
 *  noise_shaping_amp
 *        0             Amplify all bands with distort[]>1.
 *
 *        1             Amplify all bands with distort[] >= max_dist^(.5);
 *                     ( 50% in the db scale)
 *
 *        2             Amplify first band with distort[] >= max_dist;
 *
 *
 *  For algorithms 0 and 1, if max_dist < 1, then amplify all bands
 *  with distort[] >= .95*max_dist.  This is to make sure we always
 *  amplify at least one band.
 *
 *
 *************************************************************************/
static void
amp_scalefac_bands(lame_global_flags const *gfp,
                   gr_info * const cod_info, FLOAT const *distort, FLOAT xrpow[576], int bRefine)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     j, sfb;
    FLOAT   ifqstep34, trigger;
    int     noise_shaping_amp;

    if (cod_info->scalefac_scale == 0) {
        ifqstep34 = 1.29683955465100964055; /* 2**(.75*.5) */
    }
    else {
        ifqstep34 = 1.68179283050742922612; /* 2**(.75*1) */
    }

    /* compute maximum value of distort[]  */
    trigger = 0;
    for (sfb = 0; sfb < cod_info->sfbmax; sfb++) {
        if (trigger < distort[sfb])
            trigger = distort[sfb];
    }

    noise_shaping_amp = gfc->noise_shaping_amp;
    if (noise_shaping_amp == 3) {
        if (bRefine == 1)
            noise_shaping_amp = 2;
        else
            noise_shaping_amp = 1;
    }
    switch (noise_shaping_amp) {
    case 2:
        /* amplify exactly 1 band */
        break;

    case 1:
        /* amplify bands within 50% of max (on db scale) */
        if (trigger > 1.0)
            trigger = pow(trigger, .5);
        else
            trigger *= .95;
        break;

    case 0:
    default:
        /* ISO algorithm.  amplify all bands with distort>1 */
        if (trigger > 1.0)
            trigger = 1.0;
        else
            trigger *= .95;
        break;
    }

    j = 0;
    for (sfb = 0; sfb < cod_info->sfbmax; sfb++) {
        int const width = cod_info->width[sfb];
        int     l;
        j += width;
        if (distort[sfb] < trigger)
            continue;

        if (gfc->substep_shaping & 2) {
            gfc->pseudohalf[sfb] = !gfc->pseudohalf[sfb];
            if (!gfc->pseudohalf[sfb] && gfc->noise_shaping_amp == 2)
                return;
        }
        cod_info->scalefac[sfb]++;
        for (l = -width; l < 0; l++) {
            xrpow[j + l] *= ifqstep34;
            if (xrpow[j + l] > cod_info->xrpow_max)
                cod_info->xrpow_max = xrpow[j + l];
        }

        if (gfc->noise_shaping_amp == 2)
            return;
    }
}

/*************************************************************************
 *
 *      inc_scalefac_scale()
 *
 *  Takehiro Tominaga 2000-xx-xx
 *
 *  turns on scalefac scale and adjusts scalefactors
 *
 *************************************************************************/

static void
inc_scalefac_scale(gr_info * const cod_info, FLOAT xrpow[576])
{
    int     l, j, sfb;
    const FLOAT ifqstep34 = 1.29683955465100964055;

    j = 0;
    for (sfb = 0; sfb < cod_info->sfbmax; sfb++) {
        int const width = cod_info->width[sfb];
        int     s = cod_info->scalefac[sfb];
        if (cod_info->preflag)
            s += pretab[sfb];
        j += width;
        if (s & 1) {
            s++;
            for (l = -width; l < 0; l++) {
                xrpow[j + l] *= ifqstep34;
                if (xrpow[j + l] > cod_info->xrpow_max)
                    cod_info->xrpow_max = xrpow[j + l];
            }
        }
        cod_info->scalefac[sfb] = s >> 1;
    }
    cod_info->preflag = 0;
    cod_info->scalefac_scale = 1;
}



/*************************************************************************
 *
 *      inc_subblock_gain()
 *
 *  Takehiro Tominaga 2000-xx-xx
 *
 *  increases the subblock gain and adjusts scalefactors
 *
 *************************************************************************/

static int
inc_subblock_gain(const lame_internal_flags * const gfc, gr_info * const cod_info, FLOAT xrpow[576])
{
    int     sfb, window;
    int    *const scalefac = cod_info->scalefac;

    /* subbloc_gain can't do anything in the long block region */
    for (sfb = 0; sfb < cod_info->sfb_lmax; sfb++) {
        if (scalefac[sfb] >= 16)
            return 1;
    }

    for (window = 0; window < 3; window++) {
        int     s1, s2, l, j;
        s1 = s2 = 0;

        for (sfb = cod_info->sfb_lmax + window; sfb < cod_info->sfbdivide; sfb += 3) {
            if (s1 < scalefac[sfb])
                s1 = scalefac[sfb];
        }
        for (; sfb < cod_info->sfbmax; sfb += 3) {
            if (s2 < scalefac[sfb])
                s2 = scalefac[sfb];
        }

        if (s1 < 16 && s2 < 8)
            continue;

        if (cod_info->subblock_gain[window] >= 7)
            return 1;

        /* even though there is no scalefactor for sfb12
         * subblock gain affects upper frequencies too, that's why
         * we have to go up to SBMAX_s
         */
        cod_info->subblock_gain[window]++;
        j = gfc->scalefac_band.l[cod_info->sfb_lmax];
        for (sfb = cod_info->sfb_lmax + window; sfb < cod_info->sfbmax; sfb += 3) {
            FLOAT   amp;
            int const width = cod_info->width[sfb];
            int     s = scalefac[sfb];
            assert(s >= 0);
            s = s - (4 >> cod_info->scalefac_scale);
            if (s >= 0) {
                scalefac[sfb] = s;
                j += width * 3;
                continue;
            }

            scalefac[sfb] = 0;
            {
                int const gain = 210 + (s << (cod_info->scalefac_scale + 1));
                amp = IPOW20(gain);
            }
            j += width * (window + 1);
            for (l = -width; l < 0; l++) {
                xrpow[j + l] *= amp;
                if (xrpow[j + l] > cod_info->xrpow_max)
                    cod_info->xrpow_max = xrpow[j + l];
            }
            j += width * (3 - window - 1);
        }

        {
            FLOAT const amp = IPOW20(202);
            j += cod_info->width[sfb] * (window + 1);
            for (l = -cod_info->width[sfb]; l < 0; l++) {
                xrpow[j + l] *= amp;
                if (xrpow[j + l] > cod_info->xrpow_max)
                    cod_info->xrpow_max = xrpow[j + l];
            }
        }
    }
    return 0;
}



/********************************************************************
 *
 *      balance_noise()
 *
 *  Takehiro Tominaga /date??
 *  Robert Hegemann 2000-09-06: made a function of it
 *
 *  amplifies scalefactor bands,
 *   - if all are already amplified returns 0
 *   - if some bands are amplified too much:
 *      * try to increase scalefac_scale
 *      * if already scalefac_scale was set
 *          try on short blocks to increase subblock gain
 *
 ********************************************************************/
inline static int
balance_noise(lame_global_flags const *const gfp,
              gr_info * const cod_info, FLOAT const *distort, FLOAT xrpow[576], int bRefine)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     status;

    amp_scalefac_bands(gfp, cod_info, distort, xrpow, bRefine);

    /* check to make sure we have not amplified too much
     * loop_break returns 0 if there is an unamplified scalefac
     * scale_bitcount returns 0 if no scalefactors are too large
     */

    status = loop_break(cod_info);

    if (status)
        return 0;       /* all bands amplified */

    /* not all scalefactors have been amplified.  so these
     * scalefacs are possibly valid.  encode them:
     */
    if (gfc->mode_gr == 2)
        status = scale_bitcount(cod_info);
    else
        status = scale_bitcount_lsf(gfc, cod_info);

    if (!status)
        return 1;       /* amplified some bands not exceeding limits */

    /*  some scalefactors are too large.
     *  lets try setting scalefac_scale=1
     */
    if (gfc->noise_shaping > 1) {
        memset(&gfc->pseudohalf[0], 0, sizeof(gfc->pseudohalf));
        if (!cod_info->scalefac_scale) {
            inc_scalefac_scale(cod_info, xrpow);
            status = 0;
        }
        else {
            if (cod_info->block_type == SHORT_TYPE && gfc->subblock_gain > 0) {
                status = inc_subblock_gain(gfc, cod_info, xrpow)
                    || loop_break(cod_info);
            }
        }
    }

    if (!status) {
        if (gfc->mode_gr == 2)
            status = scale_bitcount(cod_info);
        else
            status = scale_bitcount_lsf(gfc, cod_info);
    }
    return !status;
}



/************************************************************************
 *
 *  outer_loop ()
 *
 *  Function: The outer iteration loop controls the masking conditions
 *  of all scalefactorbands. It computes the best scalefac and
 *  global gain. This module calls the inner iteration loop
 *
 *  mt 5/99 completely rewritten to allow for bit reservoir control,
 *  mid/side channels with L/R or mid/side masking thresholds,
 *  and chooses best quantization instead of last quantization when
 *  no distortion free quantization can be found.
 *
 *  added VBR support mt 5/99
 *
 *  some code shuffle rh 9/00
 ************************************************************************/

static int
outer_loop(lame_global_flags const *gfp, gr_info * const cod_info, const FLOAT * const l3_xmin, /* allowed distortion */
           FLOAT xrpow[576], /* coloured magnitudes of spectral */
           const int ch, const int targ_bits)
{                       /* maximum allowed bits */
    lame_internal_flags *const gfc = gfp->internal_flags;
    gr_info cod_info_w;
    FLOAT   save_xrpow[576];
    FLOAT   distort[SFBMAX];
    calc_noise_result best_noise_info;
    int     huff_bits;
    int     better;
    int     age;
    calc_noise_data prev_noise;
    int     best_part2_3_length = 9999999;
    int     bEndOfSearch = 0;
    int     bRefine = 0;
    int     best_ggain_pass1 = 0;

    (void) bin_search_StepSize(gfc, cod_info, targ_bits, ch, xrpow);

    if (!gfc->noise_shaping)
        /* fast mode, no noise shaping, we are ready */
        return 100;     /* default noise_info.over_count */

    memset(&prev_noise, 0, sizeof(calc_noise_data));


    /* compute the distortion in this quantization */
    /* coefficients and thresholds both l/r (or both mid/side) */
    (void) calc_noise(cod_info, l3_xmin, distort, &best_noise_info, &prev_noise);
    best_noise_info.bits = cod_info->part2_3_length;

    cod_info_w = *cod_info;
    age = 0;
    /* if (gfp->VBR == vbr_rh || gfp->VBR == vbr_mtrh) */
    memcpy(save_xrpow, xrpow, sizeof(FLOAT) * 576);

    while (!bEndOfSearch) {
        /* BEGIN MAIN LOOP */
        do {
            calc_noise_result noise_info;
            int     search_limit;
            int     maxggain = 255;

            /* When quantization with no distorted bands is found,
             * allow up to X new unsuccesful tries in serial. This
             * gives us more possibilities for different quant_compare modes.
             * Much more than 3 makes not a big difference, it is only slower.
             */

            if (gfc->substep_shaping & 2) {
                search_limit = 20;
            }
            else {
                search_limit = 3;
            }



            /* Check if the last scalefactor band is distorted.
             * in VBR mode we can't get rid of the distortion, so quit now
             * and VBR mode will try again with more bits.
             * (makes a 10% speed increase, the files I tested were
             * binary identical, 2000/05/20 Robert Hegemann)
             * distort[] > 1 means noise > allowed noise
             */
            if (gfc->sfb21_extra) {
                if (distort[cod_info_w.sfbmax] > 1.0)
                    break;
                if (cod_info_w.block_type == SHORT_TYPE
                    && (distort[cod_info_w.sfbmax + 1] > 1.0
                        || distort[cod_info_w.sfbmax + 2] > 1.0))
                    break;
            }

            /* try a new scalefactor conbination on cod_info_w */
            if (balance_noise(gfp, &cod_info_w, distort, xrpow, bRefine) == 0)
                break;
            if (cod_info_w.scalefac_scale)
                maxggain = 254;

            /* inner_loop starts with the initial quantization step computed above
             * and slowly increases until the bits < huff_bits.
             * Thus it is important not to start with too large of an inital
             * quantization step.  Too small is ok, but inner_loop will take longer
             */
            huff_bits = targ_bits - cod_info_w.part2_length;
            if (huff_bits <= 0)
                break;

            /*  increase quantizer stepsize until needed bits are below maximum
             */
            while ((cod_info_w.part2_3_length
                    = count_bits(gfc, xrpow, &cod_info_w, &prev_noise)) > huff_bits
                   && cod_info_w.global_gain <= maxggain)
                cod_info_w.global_gain++;

            if (cod_info_w.global_gain > maxggain)
                break;

            if (best_noise_info.over_count == 0) {

                while ((cod_info_w.part2_3_length
                        = count_bits(gfc, xrpow, &cod_info_w, &prev_noise)) > best_part2_3_length
                       && cod_info_w.global_gain <= maxggain)
                    cod_info_w.global_gain++;

                if (cod_info_w.global_gain > maxggain)
                    break;
            }

            /* compute the distortion in this quantization */
            (void) calc_noise(&cod_info_w, l3_xmin, distort, &noise_info, &prev_noise);
            noise_info.bits = cod_info_w.part2_3_length;

            /* check if this quantization is better
             * than our saved quantization */
            if (cod_info->block_type != SHORT_TYPE) /* NORM, START or STOP type */
                better = gfp->quant_comp;
            else
                better = gfp->quant_comp_short;


            better = quant_compare(better, &best_noise_info, &noise_info, &cod_info_w, distort);


            /* save data so we can restore this quantization later */
            if (better) {
                best_part2_3_length = cod_info->part2_3_length;
                best_noise_info = noise_info;
                *cod_info = cod_info_w;
                age = 0;
                /* save data so we can restore this quantization later */
                /*if (gfp->VBR == vbr_rh || gfp->VBR == vbr_mtrh) */  {
                    /* store for later reuse */
                    memcpy(save_xrpow, xrpow, sizeof(FLOAT) * 576);
                }
            }
            else {
                /* early stop? */
                if (gfc->full_outer_loop == 0) {
                    if (++age > search_limit && best_noise_info.over_count == 0)
                        break;
                    if ((gfc->noise_shaping_amp == 3) && bRefine && age > 30)
                        break;
                    if ((gfc->noise_shaping_amp == 3) && bRefine &&
                        (cod_info_w.global_gain - best_ggain_pass1) > 15)
                        break;
                }
            }
        }
        while ((cod_info_w.global_gain + cod_info_w.scalefac_scale) < 255);

        if (gfc->noise_shaping_amp == 3) {
            if (!bRefine) {
                /* refine search */
                cod_info_w = *cod_info;
                memcpy(xrpow, save_xrpow, sizeof(FLOAT) * 576);
                age = 0;
                best_ggain_pass1 = cod_info_w.global_gain;

                bRefine = 1;
            }
            else {
                /* search already refined, stop */
                bEndOfSearch = 1;
            }

        }
        else {
            bEndOfSearch = 1;
        }
    }

    assert((cod_info->global_gain + cod_info->scalefac_scale) <= 255);
    /*  finish up
     */
    if (gfp->VBR == vbr_rh || gfp->VBR == vbr_mtrh)
        /* restore for reuse on next try */
        memcpy(xrpow, save_xrpow, sizeof(FLOAT) * 576);
    /*  do the 'substep shaping'
     */
    else if (gfc->substep_shaping & 1)
        trancate_smallspectrums(gfc, cod_info, l3_xmin, xrpow);

    return best_noise_info.over_count;
}





/************************************************************************
 *
 *      iteration_finish_one()
 *
 *  Robert Hegemann 2000-09-06
 *
 *  update reservoir status after FINAL quantization/bitrate
 *
 ************************************************************************/

static void
iteration_finish_one(lame_internal_flags * gfc, int gr, int ch)
{
    III_side_info_t *const l3_side = &gfc->l3_side;
    gr_info *const cod_info = &l3_side->tt[gr][ch];

    /*  try some better scalefac storage
     */
    best_scalefac_store(gfc, gr, ch, l3_side);

    /*  best huffman_divide may save some bits too
     */
    if (gfc->use_best_huffman == 1)
        best_huffman_divide(gfc, cod_info);

    /*  update reservoir status after FINAL quantization/bitrate
     */
    ResvAdjust(gfc, cod_info);
}



/*********************************************************************
 *
 *      VBR_encode_granule()
 *
 *  2000-09-04 Robert Hegemann
 *
 *********************************************************************/

static void
VBR_encode_granule(lame_global_flags const *gfp, gr_info * const cod_info, const FLOAT * const l3_xmin, /* allowed distortion of the scalefactor */
                   FLOAT xrpow[576], /* coloured magnitudes of spectral values */
                   const int ch, int min_bits, int max_bits)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    gr_info bst_cod_info;
    FLOAT   bst_xrpow[576];
    int const Max_bits = max_bits;
    int     real_bits = max_bits + 1;
    int     this_bits = (max_bits + min_bits) / 2;
    int     dbits, over, found = 0;
    int const sfb21_extra = gfc->sfb21_extra;

    assert(Max_bits <= MAX_BITS_PER_CHANNEL);
    memset(bst_cod_info.l3_enc, 0, sizeof(bst_cod_info.l3_enc));

    /*  search within round about 40 bits of optimal
     */
    do {
        assert(this_bits >= min_bits);
        assert(this_bits <= max_bits);
        assert(min_bits <= max_bits);

        if (this_bits > Max_bits - 42)
            gfc->sfb21_extra = 0;
        else
            gfc->sfb21_extra = sfb21_extra;

        over = outer_loop(gfp, cod_info, l3_xmin, xrpow, ch, this_bits);

        /*  is quantization as good as we are looking for ?
         *  in this case: is no scalefactor band distorted?
         */
        if (over <= 0) {
            found = 1;
            /*  now we know it can be done with "real_bits"
             *  and maybe we can skip some iterations
             */
            real_bits = cod_info->part2_3_length;

            /*  store best quantization so far
             */
            bst_cod_info = *cod_info;
            memcpy(bst_xrpow, xrpow, sizeof(FLOAT) * 576);

            /*  try with fewer bits
             */
            max_bits = real_bits - 32;
            dbits = max_bits - min_bits;
            this_bits = (max_bits + min_bits) / 2;
        }
        else {
            /*  try with more bits
             */
            min_bits = this_bits + 32;
            dbits = max_bits - min_bits;
            this_bits = (max_bits + min_bits) / 2;

            if (found) {
                found = 2;
                /*  start again with best quantization so far
                 */
                *cod_info = bst_cod_info;
                memcpy(xrpow, bst_xrpow, sizeof(FLOAT) * 576);
            }
        }
    } while (dbits > 12);

    gfc->sfb21_extra = sfb21_extra;

    /*  found=0 => nothing found, use last one
     *  found=1 => we just found the best and left the loop
     *  found=2 => we restored a good one and have now l3_enc to restore too
     */
    if (found == 2) {
        memcpy(cod_info->l3_enc, bst_cod_info.l3_enc, sizeof(int) * 576);
    }
    assert(cod_info->part2_3_length <= Max_bits);

}



/************************************************************************
 *
 *      get_framebits()
 *
 *  Robert Hegemann 2000-09-05
 *
 *  calculates
 *  * how many bits are available for analog silent granules
 *  * how many bits to use for the lowest allowed bitrate
 *  * how many bits each bitrate would provide
 *
 ************************************************************************/

static void
get_framebits(lame_global_flags const *gfp, int frameBits[15])
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    int     bitsPerFrame, i;

    /*  always use at least this many bits per granule per channel
     *  unless we detect analog silence, see below
     */
    gfc->bitrate_index = gfc->VBR_min_bitrate;
    bitsPerFrame = getframebits(gfp);

    /*  bits for analog silence
     */
    gfc->bitrate_index = 1;
    bitsPerFrame = getframebits(gfp);

    for (i = 1; i <= gfc->VBR_max_bitrate; i++) {
        gfc->bitrate_index = i;
        frameBits[i] = ResvFrameBegin(gfp, &bitsPerFrame);
    }
}



/*********************************************************************
 *
 *      VBR_prepare()
 *
 *  2000-09-04 Robert Hegemann
 *
 *  * converts LR to MS coding when necessary
 *  * calculates allowed/adjusted quantization noise amounts
 *  * detects analog silent frames
 *
 *  some remarks:
 *  - lower masking depending on Quality setting
 *  - quality control together with adjusted ATH MDCT scaling
 *    on lower quality setting allocate more noise from
 *    ATH masking, and on higher quality setting allocate
 *    less noise from ATH masking.
 *  - experiments show that going more than 2dB over GPSYCHO's
 *    limits ends up in very annoying artefacts
 *
 *********************************************************************/

/* RH: this one needs to be overhauled sometime */

static int
VBR_old_prepare(lame_global_flags const *gfp,
                FLOAT pe[2][2], FLOAT const ms_ener_ratio[2],
                III_psy_ratio ratio[2][2],
                FLOAT l3_xmin[2][2][SFBMAX],
                int frameBits[16], int min_bits[2][2], int max_bits[2][2], int bands[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;


    FLOAT   masking_lower_db, adjust = 0.0;
    int     gr, ch;
    int     analog_silence = 1;
    int     avg, mxb, bits = 0;

    gfc->bitrate_index = gfc->VBR_max_bitrate;
    avg = ResvFrameBegin(gfp, &avg) / gfc->mode_gr;

    get_framebits(gfp, frameBits);

    for (gr = 0; gr < gfc->mode_gr; gr++) {
        mxb = on_pe(gfp, pe, max_bits[gr], avg, gr, 0);
        if (gfc->mode_ext == MPG_MD_MS_LR) {
            ms_convert(&gfc->l3_side, gr);
            reduce_side(max_bits[gr], ms_ener_ratio[gr], avg, mxb);
        }
        for (ch = 0; ch < gfc->channels_out; ++ch) {
            gr_info *const cod_info = &gfc->l3_side.tt[gr][ch];

            if (cod_info->block_type != SHORT_TYPE) { /* NORM, START or STOP type */
                adjust = 1.28 / (1 + exp(3.5 - pe[gr][ch] / 300.)) - 0.05;
                masking_lower_db = gfc->PSY->mask_adjust - adjust;
            }
            else {
                adjust = 2.56 / (1 + exp(3.5 - pe[gr][ch] / 300.)) - 0.14;
                masking_lower_db = gfc->PSY->mask_adjust_short - adjust;
            }
            gfc->masking_lower = pow(10.0, masking_lower_db * 0.1);

            init_outer_loop(gfc, cod_info);
            bands[gr][ch] = calc_xmin(gfp, &ratio[gr][ch], cod_info, l3_xmin[gr][ch]);
            if (bands[gr][ch])
                analog_silence = 0;

            min_bits[gr][ch] = 126;

            bits += max_bits[gr][ch];
        }
    }
    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            if (bits > frameBits[gfc->VBR_max_bitrate]) {
                max_bits[gr][ch] *= frameBits[gfc->VBR_max_bitrate];
                max_bits[gr][ch] /= bits;
            }
            if (min_bits[gr][ch] > max_bits[gr][ch])
                min_bits[gr][ch] = max_bits[gr][ch];

        }               /* for ch */
    }                   /* for gr */

    return analog_silence;
}

static void
bitpressure_strategy(lame_internal_flags const *gfc,
                     FLOAT l3_xmin[2][2][SFBMAX], int min_bits[2][2], int max_bits[2][2])
{
    int     gr, ch, sfb;
    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            gr_info const *const gi = &gfc->l3_side.tt[gr][ch];
            FLOAT  *pxmin = l3_xmin[gr][ch];
            for (sfb = 0; sfb < gi->psy_lmax; sfb++)
                *pxmin++ *= 1. + .029 * sfb * sfb / SBMAX_l / SBMAX_l;

            if (gi->block_type == SHORT_TYPE) {
                for (sfb = gi->sfb_smin; sfb < SBMAX_s; sfb++) {
                    *pxmin++ *= 1. + .029 * sfb * sfb / SBMAX_s / SBMAX_s;
                    *pxmin++ *= 1. + .029 * sfb * sfb / SBMAX_s / SBMAX_s;
                    *pxmin++ *= 1. + .029 * sfb * sfb / SBMAX_s / SBMAX_s;
                }
            }
            max_bits[gr][ch] = Max(min_bits[gr][ch], 0.9 * max_bits[gr][ch]);
        }
    }
}

/************************************************************************
 *
 *      VBR_iteration_loop()
 *
 *  tries to find out how many bits are needed for each granule and channel
 *  to get an acceptable quantization. An appropriate bitrate will then be
 *  choosed for quantization.  rh 8/99
 *
 *  Robert Hegemann 2000-09-06 rewrite
 *
 ************************************************************************/

void
VBR_old_iteration_loop(lame_global_flags const *gfp, FLOAT pe[2][2],
                       FLOAT ms_ener_ratio[2], III_psy_ratio ratio[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    FLOAT   l3_xmin[2][2][SFBMAX];

    FLOAT   xrpow[576];
    int     bands[2][2];
    int     frameBits[15];
    int     used_bits;
    int     bits;
    int     min_bits[2][2], max_bits[2][2];
    int     mean_bits;
    int     ch, gr, analog_silence;
    III_side_info_t *const l3_side = &gfc->l3_side;

    analog_silence = VBR_old_prepare(gfp, pe, ms_ener_ratio, ratio,
                                     l3_xmin, frameBits, min_bits, max_bits, bands);

    /*---------------------------------*/
    for (;;) {

        /*  quantize granules with lowest possible number of bits
         */

        used_bits = 0;

        for (gr = 0; gr < gfc->mode_gr; gr++) {
            for (ch = 0; ch < gfc->channels_out; ch++) {
                int     ret;
                gr_info *const cod_info = &l3_side->tt[gr][ch];

                /*  init_outer_loop sets up cod_info, scalefac and xrpow
                 */
                ret = init_xrpow(gfc, cod_info, xrpow);
                if (ret == 0 || max_bits[gr][ch] == 0) {
                    /*  xr contains no energy
                     *  l3_enc, our encoding data, will be quantized to zero
                     */
                    continue; /* with next channel */
                }

                VBR_encode_granule(gfp, cod_info, l3_xmin[gr][ch], xrpow,
                                   ch, min_bits[gr][ch], max_bits[gr][ch]);

                /*  do the 'substep shaping'
                 */
                if (gfc->substep_shaping & 1) {
                    trancate_smallspectrums(gfc, &l3_side->tt[gr][ch], l3_xmin[gr][ch], xrpow);
                }

                ret = cod_info->part2_3_length + cod_info->part2_length;
                used_bits += ret;
            }           /* for ch */
        }               /* for gr */

        /*  find lowest bitrate able to hold used bits
         */
        if (analog_silence && !gfp->VBR_hard_min)
            /*  we detected analog silence and the user did not specify
             *  any hard framesize limit, so start with smallest possible frame
             */
            gfc->bitrate_index = 1;
        else
            gfc->bitrate_index = gfc->VBR_min_bitrate;

        for (; gfc->bitrate_index < gfc->VBR_max_bitrate; gfc->bitrate_index++) {
            if (used_bits <= frameBits[gfc->bitrate_index])
                break;
        }
        bits = ResvFrameBegin(gfp, &mean_bits);

        if (used_bits <= bits)
            break;

        bitpressure_strategy(gfc, l3_xmin, min_bits, max_bits);

    }                   /* breaks adjusted */
    /*--------------------------------------*/

    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            iteration_finish_one(gfc, gr, ch);
        }               /* for ch */
    }                   /* for gr */
    ResvFrameEnd(gfc, mean_bits);
}



static int
VBR_new_prepare(lame_global_flags const *gfp,
                FLOAT pe[2][2], III_psy_ratio ratio[2][2],
                FLOAT l3_xmin[2][2][SFBMAX], int frameBits[16], int max_bits[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;

    int     gr, ch;
    int     analog_silence = 1;
    int     avg, bits = 0;
    int     maximum_framebits;

    if (!gfp->free_format) {
        gfc->bitrate_index = gfc->VBR_max_bitrate;
        (void) ResvFrameBegin(gfp, &avg);

        get_framebits(gfp, frameBits);
        maximum_framebits = frameBits[gfc->VBR_max_bitrate];
    }
    else {
        gfc->bitrate_index = 0;
        maximum_framebits = ResvFrameBegin(gfp, &avg);
        frameBits[0] = maximum_framebits;
    }

    for (gr = 0; gr < gfc->mode_gr; gr++) {
        (void) on_pe(gfp, pe, max_bits[gr], avg, gr, 0);
        if (gfc->mode_ext == MPG_MD_MS_LR) {
            ms_convert(&gfc->l3_side, gr);
        }
        for (ch = 0; ch < gfc->channels_out; ++ch) {
            gr_info *const cod_info = &gfc->l3_side.tt[gr][ch];

            gfc->masking_lower = pow(10.0, gfc->PSY->mask_adjust * 0.1);

            init_outer_loop(gfc, cod_info);
            if (0 != calc_xmin(gfp, &ratio[gr][ch], cod_info, l3_xmin[gr][ch]))
                analog_silence = 0;

            bits += max_bits[gr][ch];
        }
    }
    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            if (bits > maximum_framebits) {
                max_bits[gr][ch] *= maximum_framebits;
                max_bits[gr][ch] /= bits;
            }

        }               /* for ch */
    }                   /* for gr */

    return analog_silence;
}


#if 0
static int
getFramesizeInBytesPerSecond(lame_global_flags const *gfp, int bits_used)
{
    lame_internal_flags const *gfc = gfp->internal_flags;
    int const tmp = (gfp->version == 0) ? 72 : 144;
    int const bytes_used = (bits_used + 7) / 8;
    int const bytes_per_second =
        (gfp->out_samplerate * (bytes_used + gfc->sideinfo_len) + (tmp - 1)) / tmp;
    return bytes_per_second;
}

static int
getFramesize_kbps(lame_global_flags const *gfp, int bits_used)
{
    int const bytes_per_second = getFramesizeInBytesPerSecond(gfp, bits_used);
    int const kbps = (bytes_per_second + 999) / 1000;
    return kbps;
}
#endif


void
VBR_new_iteration_loop(lame_global_flags const *gfp, FLOAT pe[2][2],
                       FLOAT ms_ener_ratio[2], III_psy_ratio ratio[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    FLOAT   l3_xmin[2][2][SFBMAX];

    FLOAT   xrpow[2][2][576];
    int     frameBits[15];
    int     used_bits;
    int     max_bits[2][2];
    int     ch, gr, analog_silence;
    III_side_info_t *const l3_side = &gfc->l3_side;

    (void) ms_ener_ratio; /* not used */

    analog_silence = VBR_new_prepare(gfp, pe, ratio, l3_xmin, frameBits, max_bits);

    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            gr_info *const cod_info = &l3_side->tt[gr][ch];

            /*  init_outer_loop sets up cod_info, scalefac and xrpow
             */
            if (0 == init_xrpow(gfc, cod_info, xrpow[gr][ch])) {
                max_bits[gr][ch] = 0; /* silent granule needs no bits */
            }
        }               /* for ch */
    }                   /* for gr */

    /*  quantize granules with lowest possible number of bits
     */

    used_bits = VBR_encode_frame(gfc, xrpow, l3_xmin, max_bits);

    if (!gfp->free_format) {
        /*  find lowest bitrate able to hold used bits
         */
        if (analog_silence && !gfp->VBR_hard_min) {
            /*  we detected analog silence and the user did not specify
             *  any hard framesize limit, so start with smallest possible frame
             */
            gfc->bitrate_index = 1;
        }
        else {
            gfc->bitrate_index = gfc->VBR_min_bitrate;
        }

        for (; gfc->bitrate_index < gfc->VBR_max_bitrate; gfc->bitrate_index++) {
            if (used_bits <= frameBits[gfc->bitrate_index])
                break;
        }
        if (gfc->bitrate_index > gfc->VBR_max_bitrate) {
            gfc->bitrate_index = gfc->VBR_max_bitrate;
        }
    }
    else {
#if 0
        static int mmm = 0;
        int     fff = getFramesize_kbps(gfp, used_bits);
        int     hhh = getFramesize_kbps(gfp, MAX_BITS_PER_GRANULE * gfc->mode_gr);
        if (mmm < fff)
            mmm = fff;
        printf("demand=%3d kbps  max=%3d kbps   limit=%3d kbps\n", fff, mmm, hhh);
#endif
        gfc->bitrate_index = 0;
    }
    if (used_bits <= frameBits[gfc->bitrate_index]) {
        /* update Reservoire status */
        int     mean_bits, fullframebits;
        fullframebits = ResvFrameBegin(gfp, &mean_bits);
        assert(used_bits <= fullframebits);
        for (gr = 0; gr < gfc->mode_gr; gr++) {
            for (ch = 0; ch < gfc->channels_out; ch++) {
                gr_info const *const cod_info = &l3_side->tt[gr][ch];
                ResvAdjust(gfc, cod_info);
            }
        }
        ResvFrameEnd(gfc, mean_bits);
    }
    else {
        /* SHOULD NOT HAPPEN INTERNAL ERROR
         */
        ERRORF(gfc, "INTERNAL ERROR IN VBR NEW CODE, please send bug report\n");
        exit(-1);
    }
}





/********************************************************************
 *
 *  calc_target_bits()
 *
 *  calculates target bits for ABR encoding
 *
 *  mt 2000/05/31
 *
 ********************************************************************/

static void
calc_target_bits(lame_global_flags const *gfp,
                 FLOAT pe[2][2],
                 FLOAT const ms_ener_ratio[2],
                 int targ_bits[2][2], int *analog_silence_bits, int *max_frame_bits)
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    III_side_info_t const *const l3_side = &gfc->l3_side;
    FLOAT   res_factor;
    int     gr, ch, totbits, mean_bits;

    gfc->bitrate_index = gfc->VBR_max_bitrate;
    *max_frame_bits = ResvFrameBegin(gfp, &mean_bits);

    gfc->bitrate_index = 1;
    mean_bits = getframebits(gfp) - gfc->sideinfo_len * 8;
    *analog_silence_bits = mean_bits / (gfc->mode_gr * gfc->channels_out);

    mean_bits = gfp->VBR_mean_bitrate_kbps * gfp->framesize * 1000;
    if (gfc->substep_shaping & 1)
        mean_bits *= 1.09;
    mean_bits /= gfp->out_samplerate;
    mean_bits -= gfc->sideinfo_len * 8;
    mean_bits /= (gfc->mode_gr * gfc->channels_out);

    /*
       res_factor is the percentage of the target bitrate that should
       be used on average.  the remaining bits are added to the
       bitreservoir and used for difficult to encode frames.

       Since we are tracking the average bitrate, we should adjust
       res_factor "on the fly", increasing it if the average bitrate
       is greater than the requested bitrate, and decreasing it
       otherwise.  Reasonable ranges are from .9 to 1.0

       Until we get the above suggestion working, we use the following
       tuning:
       compression ratio    res_factor
       5.5  (256kbps)         1.0      no need for bitreservoir
       11   (128kbps)         .93      7% held for reservoir

       with linear interpolation for other values.

     */
    res_factor = .93 + .07 * (11.0 - gfp->compression_ratio) / (11.0 - 5.5);
    if (res_factor < .90)
        res_factor = .90;
    if (res_factor > 1.00)
        res_factor = 1.00;

    for (gr = 0; gr < gfc->mode_gr; gr++) {
        int     sum = 0;
        for (ch = 0; ch < gfc->channels_out; ch++) {
            targ_bits[gr][ch] = res_factor * mean_bits;

            if (pe[gr][ch] > 700) {
                int     add_bits = (pe[gr][ch] - 700) / 1.4;

                gr_info const *const cod_info = &l3_side->tt[gr][ch];
                targ_bits[gr][ch] = res_factor * mean_bits;

                /* short blocks use a little extra, no matter what the pe */
                if (cod_info->block_type == SHORT_TYPE) {
                    if (add_bits < mean_bits / 2)
                        add_bits = mean_bits / 2;
                }
                /* at most increase bits by 1.5*average */
                if (add_bits > mean_bits * 3 / 2)
                    add_bits = mean_bits * 3 / 2;
                else if (add_bits < 0)
                    add_bits = 0;

                targ_bits[gr][ch] += add_bits;
            }
            if (targ_bits[gr][ch] > MAX_BITS_PER_CHANNEL) {
                targ_bits[gr][ch] = MAX_BITS_PER_CHANNEL;
            }
            sum += targ_bits[gr][ch];
        }               /* for ch */
        if (sum > MAX_BITS_PER_GRANULE) {
            for (ch = 0; ch < gfc->channels_out; ++ch) {
                targ_bits[gr][ch] *= MAX_BITS_PER_GRANULE;
                targ_bits[gr][ch] /= sum;
            }
        }
    }                   /* for gr */

    if (gfc->mode_ext == MPG_MD_MS_LR)
        for (gr = 0; gr < gfc->mode_gr; gr++) {
            reduce_side(targ_bits[gr], ms_ener_ratio[gr], mean_bits * gfc->channels_out,
                        MAX_BITS_PER_GRANULE);
        }

    /*  sum target bits
     */
    totbits = 0;
    for (gr = 0; gr < gfc->mode_gr; gr++) {
        for (ch = 0; ch < gfc->channels_out; ch++) {
            if (targ_bits[gr][ch] > MAX_BITS_PER_CHANNEL)
                targ_bits[gr][ch] = MAX_BITS_PER_CHANNEL;
            totbits += targ_bits[gr][ch];
        }
    }

    /*  repartion target bits if needed
     */
    if (totbits > *max_frame_bits) {
        for (gr = 0; gr < gfc->mode_gr; gr++) {
            for (ch = 0; ch < gfc->channels_out; ch++) {
                targ_bits[gr][ch] *= *max_frame_bits;
                targ_bits[gr][ch] /= totbits;
            }
        }
    }
}






/********************************************************************
 *
 *  ABR_iteration_loop()
 *
 *  encode a frame with a disired average bitrate
 *
 *  mt 2000/05/31
 *
 ********************************************************************/

void
ABR_iteration_loop(lame_global_flags const *gfp, FLOAT pe[2][2],
                   FLOAT ms_ener_ratio[2], III_psy_ratio ratio[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    FLOAT   l3_xmin[SFBMAX];
    FLOAT   xrpow[576];
    int     targ_bits[2][2];
    int     mean_bits, max_frame_bits;
    int     ch, gr, ath_over;
    int     analog_silence_bits;
    gr_info *cod_info;
    III_side_info_t *const l3_side = &gfc->l3_side;

    mean_bits = 0;

    calc_target_bits(gfp, pe, ms_ener_ratio, targ_bits, &analog_silence_bits, &max_frame_bits);

    /*  encode granules
     */
    for (gr = 0; gr < gfc->mode_gr; gr++) {

        if (gfc->mode_ext == MPG_MD_MS_LR) {
            ms_convert(&gfc->l3_side, gr);
        }
        for (ch = 0; ch < gfc->channels_out; ch++) {
            FLOAT   adjust, masking_lower_db;
            cod_info = &l3_side->tt[gr][ch];

            if (cod_info->block_type != SHORT_TYPE) { /* NORM, START or STOP type */
                /* adjust = 1.28/(1+exp(3.5-pe[gr][ch]/300.))-0.05; */
                adjust = 0;
                masking_lower_db = gfc->PSY->mask_adjust - adjust;
            }
            else {
                /* adjust = 2.56/(1+exp(3.5-pe[gr][ch]/300.))-0.14; */
                adjust = 0;
                masking_lower_db = gfc->PSY->mask_adjust_short - adjust;
            }
            gfc->masking_lower = pow(10.0, masking_lower_db * 0.1);


            /*  cod_info, scalefac and xrpow get initialized in init_outer_loop
             */
            init_outer_loop(gfc, cod_info);
            if (init_xrpow(gfc, cod_info, xrpow)) {
                /*  xr contains energy we will have to encode
                 *  calculate the masking abilities
                 *  find some good quantization in outer_loop
                 */
                ath_over = calc_xmin(gfp, &ratio[gr][ch], cod_info, l3_xmin);
                if (0 == ath_over) /* analog silence */
                    targ_bits[gr][ch] = analog_silence_bits;

                (void) outer_loop(gfp, cod_info, l3_xmin, xrpow, ch, targ_bits[gr][ch]);
            }
            iteration_finish_one(gfc, gr, ch);
        }               /* ch */
    }                   /* gr */

    /*  find a bitrate which can refill the resevoir to positive size.
     */
    for (gfc->bitrate_index = gfc->VBR_min_bitrate;
         gfc->bitrate_index <= gfc->VBR_max_bitrate; gfc->bitrate_index++) {
        if (ResvFrameBegin(gfp, &mean_bits) >= 0)
            break;
    }
    assert(gfc->bitrate_index <= gfc->VBR_max_bitrate);

    ResvFrameEnd(gfc, mean_bits);
}






/************************************************************************
 *
 *      CBR_iteration_loop()
 *
 *  author/date??
 *
 *  encodes one frame of MP3 data with constant bitrate
 *
 ************************************************************************/

void
CBR_iteration_loop(lame_global_flags const *gfp, FLOAT pe[2][2],
                   FLOAT ms_ener_ratio[2], III_psy_ratio ratio[2][2])
{
    lame_internal_flags *const gfc = gfp->internal_flags;
    FLOAT   l3_xmin[SFBMAX];
    FLOAT   xrpow[576];
    int     targ_bits[2];
    int     mean_bits, max_bits;
    int     gr, ch;
    III_side_info_t *const l3_side = &gfc->l3_side;
    gr_info *cod_info;

    (void) ResvFrameBegin(gfp, &mean_bits);

    /* quantize! */
    for (gr = 0; gr < gfc->mode_gr; gr++) {

        /*  calculate needed bits
         */
        max_bits = on_pe(gfp, pe, targ_bits, mean_bits, gr, gr);

        if (gfc->mode_ext == MPG_MD_MS_LR) {
            ms_convert(&gfc->l3_side, gr);
            reduce_side(targ_bits, ms_ener_ratio[gr], mean_bits, max_bits);
        }

        for (ch = 0; ch < gfc->channels_out; ch++) {
            FLOAT   adjust, masking_lower_db;
            cod_info = &l3_side->tt[gr][ch];

            if (cod_info->block_type != SHORT_TYPE) { /* NORM, START or STOP type */
                /* adjust = 1.28/(1+exp(3.5-pe[gr][ch]/300.))-0.05; */
                adjust = 0;
                masking_lower_db = gfc->PSY->mask_adjust - adjust;
            }
            else {
                /* adjust = 2.56/(1+exp(3.5-pe[gr][ch]/300.))-0.14; */
                adjust = 0;
                masking_lower_db = gfc->PSY->mask_adjust_short - adjust;
            }
            gfc->masking_lower = pow(10.0, masking_lower_db * 0.1);

            /*  init_outer_loop sets up cod_info, scalefac and xrpow
             */
            init_outer_loop(gfc, cod_info);
            if (init_xrpow(gfc, cod_info, xrpow)) {
                /*  xr contains energy we will have to encode
                 *  calculate the masking abilities
                 *  find some good quantization in outer_loop
                 */
                (void) calc_xmin(gfp, &ratio[gr][ch], cod_info, l3_xmin);
                (void) outer_loop(gfp, cod_info, l3_xmin, xrpow, ch, targ_bits[ch]);
            }

            iteration_finish_one(gfc, gr, ch);
            assert(cod_info->part2_3_length <= MAX_BITS_PER_CHANNEL);
            assert(cod_info->part2_3_length <= targ_bits[ch]);
        }               /* for ch */
    }                   /* for gr */

    ResvFrameEnd(gfc, mean_bits);
}
