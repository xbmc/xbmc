/*
 * presets.c -- Apply presets
 *
 *	Copyright (c) 2002-2008 Gabriel Bouvigne
 *	Copyright (c) 2007-2008 Robert Hegemann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "set_get.h"
#include "encoder.h"
#include "util.h"
#include "lame_global_flags.h"

#define SET_OPTION(opt, val, def) if (enforce) \
    (void) lame_set_##opt(gfp, val); \
    else if (!(fabs(lame_get_##opt(gfp) - def) > 0)) \
    (void) lame_set_##opt(gfp, val);

#undef Min
#undef Max

static inline int
min_int(int a, int b)
{
    if (a < b) {
        return a;
    }
    return b;
}

static inline int
max_int(int a, int b)
{
    if (a > b) {
        return a;
    }
    return b;
}



typedef struct {
    int     vbr_q;
    int     quant_comp;
    int     quant_comp_s;
    int     expY;
    FLOAT   st_lrm;          /*short threshold */
    FLOAT   st_s;
    FLOAT   masking_adj;
    FLOAT   masking_adj_short;
    FLOAT   ath_lower;
    FLOAT   ath_curve;
    FLOAT   ath_sensitivity;
    FLOAT   interch;
    int     safejoint;
    int     sfb21mod;
    FLOAT   msfix;
} vbr_presets_t;

    /* *INDENT-OFF* */
    
    /* Switch mappings for VBR mode VBR_RH */
    static const vbr_presets_t vbr_old_switch_map[] = {
    /*vbr_q  qcomp_l  qcomp_s  expY  st_lrm   st_s  mask adj_l  adj_s  ath_lower  ath_curve  ath_sens  interChR  safejoint sfb21mod  msfix */
        {0,       9,       9,    0,   5.20, 125.0,      -4.2,   -6.3,       4.8,       1,          0,   0,              2,      21,  0.97},
        {1,       9,       9,    0,   5.30, 125.0,      -3.6,   -5.6,       4.5,       1.5,        0,   0,              2,      21,  1.35},
        {2,       9,       9,    0,   5.60, 125.0,      -2.2,   -3.5,       2.8,       2,          0,   0,              2,      21,  1.49},
        {3,       9,       9,    1,   5.80, 130.0,      -1.8,   -2.8,       2.6,       3,         -4,   0,              2,      20,  1.64},
        {4,       9,       9,    1,   6.00, 135.0,      -0.7,   -1.1,       1.1,       3.5,       -8,   0,              2,       0,  1.79},
        {5,       9,       9,    1,   6.40, 140.0,       0.5,    0.4,      -7.5,       4,        -12,   0.0002,         0,       0,  1.95},
        {6,       9,       9,    1,   6.60, 145.0,       0.67,   0.65,    -14.7,       6.5,      -19,   0.0004,         0,       0,  2.30},
        {7,       9,       9,    1,   6.60, 145.0,       0.8,    0.75,    -19.7,       8,        -22,   0.0006,         0,       0,  2.70},
        {8,       9,       9,    1,   6.60, 145.0,       1.2,    1.15,    -27.5,      10,        -23,   0.0007,         0,       0,  0   },
        {9,       9,       9,    1,   6.60, 145.0,       1.6,    1.6,     -36,        11,        -25,   0.0008,         0,       0,  0   },
        {10,      9,       9,    1,   6.60, 145.0,       2.0,    2.0,     -36,        12,        -25,   0.0008,         0,       0,  0   }
    };
    
    static const vbr_presets_t vbr_psy_switch_map[] = {
    /*vbr_q  qcomp_l  qcomp_s  expY  st_lrm   st_s  mask adj_l  adj_s  ath_lower  ath_curve  ath_sens  interChR  safejoint sfb21mod  msfix */
        {0,       9,       9,    0,   4.20,  25.0,      -7.0,   -4.0,       7.5,       1,          0,   0,              2,      26,  0.97},
        {1,       9,       9,    0,   4.20,  25.0,      -5.6,   -3.6,       4.5,       1.5,        0,   0,              2,      21,  1.35},
        {2,       9,       9,    0,   4.20,  25.0,      -4.4,   -1.8,       2,         2,          0,   0,              2,      18,  1.49},
        {3,       9,       9,    1,   4.20,  25.0,      -3.4,   -1.25,      1.1,       3,         -4,   0,              2,      15,  1.64},
        {4,       9,       9,    1,   4.20,  25.0,      -2.2,    0.1,       0,         3.5,       -8,   0,              2,       0,  1.79},
        {5,       9,       9,    1,   4.20,  25.0,      -1.0,    1.65,     -7.7,       4,        -12,   0.0002,         0,       0,  1.95},
        {6,       9,       9,    1,   4.20,  25.0,      -0.0,    2.47,     -7.7,       6.5,      -19,   0.0004,         0,       0,  2   },
        {7,       9,       9,    1,   4.20,  25.0,       0.5,    2.0,     -14.5,       8,        -22,   0.0006,         0,       0,  2   },
        {8,       9,       9,    1,   4.20,  25.0,       1.0,    2.4,     -22.0,      10,        -23,   0.0007,         0,       0,  2   },
        {9,       9,       9,    1,   4.20,  25.0,       1.5,    2.95,    -30.0,      11,        -25,   0.0008,         0,       0,  2   },
        {10,      9,       9,    1,   4.20,  25.0,       2.0,    2.95,    -36.0,      12,        -30,   0.0008,         0,       0,  2   }
    };
    
    /* *INDENT-ON* */

#define NOOP(m) (void)p.m
#define LERP(m) p.m = p.m + x * (q.m - p.m)

static void
apply_vbr_preset(lame_global_flags * gfp, int a, int enforce)
{
    vbr_presets_t const *vbr_preset = lame_get_VBR(gfp) == vbr_rh ? &vbr_old_switch_map[0]
        : &vbr_psy_switch_map[0];
    float   x = gfp->VBR_q_frac;
    vbr_presets_t p = vbr_preset[a];
    vbr_presets_t q = vbr_preset[a + 1];
    vbr_presets_t const *set = &p;

    NOOP(vbr_q);
    NOOP(quant_comp);
    NOOP(quant_comp_s);
    NOOP(expY);
    LERP(st_lrm);
    LERP(st_s);
    LERP(masking_adj);
    LERP(masking_adj_short);
    LERP(ath_lower);
    LERP(ath_curve);
    LERP(ath_sensitivity);
    LERP(interch);
    NOOP(safejoint);
    NOOP(sfb21mod);
    LERP(msfix);

    (void) lame_set_VBR_q(gfp, set->vbr_q);
    SET_OPTION(quant_comp, set->quant_comp, -1);
    SET_OPTION(quant_comp_short, set->quant_comp_s, -1);
    if (set->expY) {
        (void) lame_set_experimentalY(gfp, set->expY);
    }
    SET_OPTION(short_threshold_lrm, set->st_lrm, -1);
    SET_OPTION(short_threshold_s, set->st_s, -1);
    SET_OPTION(maskingadjust, set->masking_adj, 0);
    SET_OPTION(maskingadjust_short, set->masking_adj_short, 0);
    SET_OPTION(ATHlower, set->ath_lower, 0);
    SET_OPTION(ATHcurve, set->ath_curve, -1);
    SET_OPTION(athaa_sensitivity, set->ath_sensitivity, 0);
    if (set->interch > 0) {
        SET_OPTION(interChRatio, set->interch, -1);
    }

    /* parameters for which there is no proper set/get interface */
    if (set->safejoint > 0) {
        (void) lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | set->safejoint);
    }
    if (set->sfb21mod > 0) {
        (void) lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (set->sfb21mod << 20));
    }
    SET_OPTION(msfix, set->msfix, -1);

    if (enforce == 0) {
        gfp->VBR_q = a;
        gfp->VBR_q_frac = x;
    }
}

static int
apply_abr_preset(lame_global_flags * gfp, int preset, int enforce)
{
    int     k;

    typedef struct {
        int     abr_kbps;
        int     quant_comp;
        int     quant_comp_s;
        int     safejoint;
        FLOAT   nsmsfix;
        FLOAT   st_lrm;      /*short threshold */
        FLOAT   st_s;
        FLOAT   nsbass;
        FLOAT   scale;
        FLOAT   masking_adj;
        FLOAT   ath_lower;
        FLOAT   ath_curve;
        FLOAT   interch;
        int     sfscale;
    } abr_presets_t;


    /* *INDENT-OFF* */

    /* 
     *  Switch mappings for ABR mode
     */
    const abr_presets_t abr_switch_map[] = {        
    /* kbps  quant q_s safejoint nsmsfix st_lrm  st_s  ns-bass scale   msk ath_lwr ath_curve  interch , sfscale */
      {  8,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -30.0,     11,    0.0012,        1}, /*   8, impossible to use in stereo */
      { 16,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -25.0,     11,    0.0010,        1}, /*  16 */
      { 24,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -20.0,     11,    0.0010,        1}, /*  24 */
      { 32,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -15.0,     11,    0.0010,        1}, /*  32 */
      { 40,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -10.0,     11,    0.0009,        1}, /*  40 */
      { 48,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,  -10.0,     11,    0.0009,        1}, /*  48 */
      { 56,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,   -6.0,     11,    0.0008,        1}, /*  56 */
      { 64,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,   -2.0,     11,    0.0008,        1}, /*  64 */
      { 80,     9,  9,        0,      0,  6.60,  145,       0, 0.95,    0,     .0,      8,    0.0007,        1}, /*  80 */
      { 96,     9,  9,        0,   2.50,  6.60,  145,       0, 0.95,    0,    1.0,      5.5,  0.0006,        1}, /*  96 */
      {112,     9,  9,        0,   2.25,  6.60,  145,       0, 0.95,    0,    2.0,      4.5,  0.0005,        1}, /* 112 */
      {128,     9,  9,        0,   1.95,  6.40,  140,       0, 0.95,    0,    3.0,      4,    0.0002,        1}, /* 128 */
      {160,     9,  9,        1,   1.79,  6.00,  135,       0, 0.95,   -2,    5.0,      3.5,  0,             1}, /* 160 */
      {192,     9,  9,        1,   1.49,  5.60,  125,       0, 0.97,   -4,    7.0,      3,    0,             0}, /* 192 */
      {224,     9,  9,        1,   1.25,  5.20,  125,       0, 0.98,   -6,    9.0,      2,    0,             0}, /* 224 */
      {256,     9,  9,        1,   0.97,  5.20,  125,       0, 1.00,   -8,   10.0,      1,    0,             0}, /* 256 */
      {320,     9,  9,        1,   0.90,  5.20,  125,       0, 1.00,  -10,   12.0,      0,    0,             0}  /* 320 */
    };

    /* *INDENT-ON* */

    /* Variables for the ABR stuff */
    int     r;
    int     actual_bitrate = preset;

    r = nearestBitrateFullIndex(preset);


    (void) lame_set_VBR(gfp, vbr_abr);
    (void) lame_set_VBR_mean_bitrate_kbps(gfp, (actual_bitrate));
    (void) lame_set_VBR_mean_bitrate_kbps(gfp, min_int(lame_get_VBR_mean_bitrate_kbps(gfp), 320));
    (void) lame_set_VBR_mean_bitrate_kbps(gfp, max_int(lame_get_VBR_mean_bitrate_kbps(gfp), 8));
    (void) lame_set_brate(gfp, lame_get_VBR_mean_bitrate_kbps(gfp));


    /* parameters for which there is no proper set/get interface */
    if (abr_switch_map[r].safejoint > 0)
        (void) lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | 2); /* safejoint */

    if (abr_switch_map[r].sfscale > 0)
        (void) lame_set_sfscale(gfp, 1);

    /* ns-bass tweaks */
    if (fabs(abr_switch_map[r].nsbass) > 0) {
        k = (int) (abr_switch_map[r].nsbass * 4);
        if (k < 0)
            k += 64;
        (void) lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 2));
    }




    SET_OPTION(quant_comp, abr_switch_map[r].quant_comp, -1);
    SET_OPTION(quant_comp_short, abr_switch_map[r].quant_comp_s, -1);

    SET_OPTION(msfix, abr_switch_map[r].nsmsfix, -1);

    SET_OPTION(short_threshold_lrm, abr_switch_map[r].st_lrm, -1);
    SET_OPTION(short_threshold_s, abr_switch_map[r].st_s, -1);

    /* ABR seems to have big problems with clipping, especially at low bitrates */
    /* so we compensate for that here by using a scale value depending on bitrate */
    SET_OPTION(scale, abr_switch_map[r].scale, -1);

    SET_OPTION(maskingadjust, abr_switch_map[r].masking_adj, 0);
    if (abr_switch_map[r].masking_adj > 0) {
        SET_OPTION(maskingadjust_short, abr_switch_map[r].masking_adj * .9, 0);
    }
    else {
        SET_OPTION(maskingadjust_short, abr_switch_map[r].masking_adj * 1.1, 0);
    }


    SET_OPTION(ATHlower, abr_switch_map[r].ath_lower, 0);
    SET_OPTION(ATHcurve, abr_switch_map[r].ath_curve, -1);

    SET_OPTION(interChRatio, abr_switch_map[r].interch, -1);


    return preset;
}



int
apply_preset(lame_global_flags * gfp, int preset, int enforce)
{
    /*translate legacy presets */
    switch (preset) {
    case R3MIX:
        {
            preset = V3;
            (void) lame_set_VBR(gfp, vbr_mtrh);
            break;
        }
    case MEDIUM:
        {
            preset = V4;
            (void) lame_set_VBR(gfp, vbr_rh);
            break;
        }
    case MEDIUM_FAST:
        {
            preset = V4;
            (void) lame_set_VBR(gfp, vbr_mtrh);
            break;
        }
    case STANDARD:
        {
            preset = V2;
            (void) lame_set_VBR(gfp, vbr_rh);
            break;
        }
    case STANDARD_FAST:
        {
            preset = V2;
            (void) lame_set_VBR(gfp, vbr_mtrh);
            break;
        }
    case EXTREME:
        {
            preset = V0;
            (void) lame_set_VBR(gfp, vbr_rh);
            break;
        }
    case EXTREME_FAST:
        {
            preset = V0;
            (void) lame_set_VBR(gfp, vbr_mtrh);
            break;
        }
    case INSANE:
        {
            preset = 320;
            gfp->preset = preset;
            (void) apply_abr_preset(gfp, preset, enforce);
            lame_set_VBR(gfp, vbr_off);
            return preset;
        }
    }

    gfp->preset = preset;
    {
        switch (preset) {
        case V9:
            apply_vbr_preset(gfp, 9, enforce);
            return preset;
        case V8:
            apply_vbr_preset(gfp, 8, enforce);
            return preset;
        case V7:
            apply_vbr_preset(gfp, 7, enforce);
            return preset;
        case V6:
            apply_vbr_preset(gfp, 6, enforce);
            return preset;
        case V5:
            apply_vbr_preset(gfp, 5, enforce);
            return preset;
        case V4:
            apply_vbr_preset(gfp, 4, enforce);
            return preset;
        case V3:
            apply_vbr_preset(gfp, 3, enforce);
            return preset;
        case V2:
            apply_vbr_preset(gfp, 2, enforce);
            return preset;
        case V1:
            apply_vbr_preset(gfp, 1, enforce);
            return preset;
        case V0:
            apply_vbr_preset(gfp, 0, enforce);
            return preset;
        default:
            break;
        }
    }
    if (8 <= preset && preset <= 320) {
        return apply_abr_preset(gfp, preset, enforce);
    }

    gfp->preset = 0;    /*no corresponding preset found */
    return preset;
}
