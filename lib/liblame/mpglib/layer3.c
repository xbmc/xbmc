/* 
 * layer3.c: Mpeg Layer-3 audio decoder 
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* $Id: layer3.c,v 1.51.2.3 2010/03/22 14:17:14 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include "common.h"
#include "huffman.h"
#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "lame-analysis.h"
#include "decode_i386.h"
#include "layer3.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif



static real ispow[8207];
static real aa_ca[8], aa_cs[8];
static real COS1[12][6];
static real win[4][36];
static real win1[4][36];
static real gainpow2[256 + 118 + 4];
static real COS9[9];
static real COS6_1, COS6_2;
static real tfcos36[9];
static real tfcos12[3];

struct bandInfoStruct {
    short   longIdx[23];
    short   longDiff[22];
    short   shortIdx[14];
    short   shortDiff[13];
};

static int longLimit[9][23];
static int shortLimit[9][14];

/* *INDENT-OFF* */

const struct bandInfoStruct bandInfo[9] = { 

/* MPEG 1.0 */
 { {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
   {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
   {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
   {4,4,4,4,6,8,10,12,14,18,22,30,56} } ,

 { {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
   {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
   {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
   {4,4,4,4,6,6,10,12,14,16,20,26,66} } ,

 { {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576} ,
   {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26} ,
   {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3} ,
   {4,4,4,4,6,8,12,16,20,26,34,42,12} }  ,

/* MPEG 2.0 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
   {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
   {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,
                                             /* docs: 332. mpg123: 330 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,332,394,464,540,576},
   {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,54,62,70,76,36 } ,
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3} ,
   {4,4,4,6,8,10,12,14,18,24,32,44,12 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
   {4,4,4,6,8,10,12,14,18,24,30,40,18 } } ,
/* MPEG 2.5 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576},
   {12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2},
   {0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576},
   {8,8,8,12,16,20,24,28,36,2,2,2,26} } ,
};
/* *INDENT-ON* */

static int mapbuf0[9][152];
static int mapbuf1[9][156];
static int mapbuf2[9][44];
static int *map[9][3];
static int *mapend[9][3];

static unsigned int n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
static unsigned int i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

static real tan1_1[16], tan2_1[16], tan1_2[16], tan2_2[16];
static real pow1_1[2][16], pow2_1[2][16], pow1_2[2][16], pow2_2[2][16];

static unsigned int
get1bit(PMPSTR mp)
{
    unsigned char rval;
    rval = *mp->wordpointer << mp->bitindex;

    mp->bitindex++;
    mp->wordpointer += (mp->bitindex >> 3);
    mp->bitindex &= 7;

    return rval >> 7;
}




/* 
 * init tables for layer-3 
 */
void
init_layer3(int down_sample_sblimit)
{
    int     i, j, k;

    for (i = -256; i < 118 + 4; i++)
        gainpow2[i + 256] = pow((double) 2.0, -0.25 * (double) (i + 210));

    for (i = 0; i < 8207; i++)
        ispow[i] = pow((double) i, (double) 4.0 / 3.0);

    for (i = 0; i < 8; i++) {
        static double Ci[8] = { -0.6, -0.535, -0.33, -0.185, -0.095, -0.041, -0.0142, -0.0037 };
        double  sq = sqrt(1.0 + Ci[i] * Ci[i]);
        aa_cs[i] = 1.0 / sq;
        aa_ca[i] = Ci[i] / sq;
    }

    for (i = 0; i < 18; i++) {
        win[0][i] = win[1][i] =
            0.5 * sin(M_PI / 72.0 * (double) (2 * (i + 0) + 1)) / cos(M_PI *
                                                                      (double) (2 * (i + 0) +
                                                                                19) / 72.0);
        win[0][i + 18] = win[3][i + 18] =
            0.5 * sin(M_PI / 72.0 * (double) (2 * (i + 18) + 1)) / cos(M_PI *
                                                                       (double) (2 * (i + 18) +
                                                                                 19) / 72.0);
    }
    for (i = 0; i < 6; i++) {
        win[1][i + 18] = 0.5 / cos(M_PI * (double) (2 * (i + 18) + 19) / 72.0);
        win[3][i + 12] = 0.5 / cos(M_PI * (double) (2 * (i + 12) + 19) / 72.0);
        win[1][i + 24] =
            0.5 * sin(M_PI / 24.0 * (double) (2 * i + 13)) / cos(M_PI *
                                                                 (double) (2 * (i + 24) +
                                                                           19) / 72.0);
        win[1][i + 30] = win[3][i] = 0.0;
        win[3][i + 6] =
            0.5 * sin(M_PI / 24.0 * (double) (2 * i + 1)) / cos(M_PI * (double) (2 * (i + 6) + 19) /
                                                                72.0);
    }

    for (i = 0; i < 9; i++)
        COS9[i] = cos(M_PI / 18.0 * (double) i);

    for (i = 0; i < 9; i++)
        tfcos36[i] = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 36.0);
    for (i = 0; i < 3; i++)
        tfcos12[i] = 0.5 / cos(M_PI * (double) (i * 2 + 1) / 12.0);

    COS6_1 = cos(M_PI / 6.0 * (double) 1);
    COS6_2 = cos(M_PI / 6.0 * (double) 2);

    for (i = 0; i < 12; i++) {
        win[2][i] =
            0.5 * sin(M_PI / 24.0 * (double) (2 * i + 1)) / cos(M_PI * (double) (2 * i + 7) / 24.0);
        for (j = 0; j < 6; j++)
            COS1[i][j] = cos(M_PI / 24.0 * (double) ((2 * i + 7) * (2 * j + 1)));
    }

    for (j = 0; j < 4; j++) {
        static int len[4] = { 36, 36, 12, 36 };
        for (i = 0; i < len[j]; i += 2)
            win1[j][i] = +win[j][i];
        for (i = 1; i < len[j]; i += 2)
            win1[j][i] = -win[j][i];
    }

    for (i = 0; i < 16; i++) {
        double  t = tan((double) i * M_PI / 12.0);
        tan1_1[i] = t / (1.0 + t);
        tan2_1[i] = 1.0 / (1.0 + t);
        tan1_2[i] = M_SQRT2 * t / (1.0 + t);
        tan2_2[i] = M_SQRT2 / (1.0 + t);

        for (j = 0; j < 2; j++) {
            double  base = pow(2.0, -0.25 * (j + 1.0));
            double  p1 = 1.0, p2 = 1.0;
            if (i > 0) {
                if (i & 1)
                    p1 = pow(base, (i + 1.0) * 0.5);
                else
                    p2 = pow(base, i * 0.5);
            }
            pow1_1[j][i] = p1;
            pow2_1[j][i] = p2;
            pow1_2[j][i] = M_SQRT2 * p1;
            pow2_2[j][i] = M_SQRT2 * p2;
        }
    }

    for (j = 0; j < 9; j++) {
        struct bandInfoStruct const *bi = (struct bandInfoStruct const *) &bandInfo[j];
        int    *mp;
        int     cb, lwin;
        short const *bdf;

        mp = map[j][0] = mapbuf0[j];
        bdf = bi->longDiff;
        for (i = 0, cb = 0; cb < 8; cb++, i += *bdf++) {
            *mp++ = (*bdf) >> 1;
            *mp++ = i;
            *mp++ = 3;
            *mp++ = cb;
        }
        bdf = bi->shortDiff + 3;
        for (cb = 3; cb < 13; cb++) {
            int     l = (*bdf++) >> 1;
            for (lwin = 0; lwin < 3; lwin++) {
                *mp++ = l;
                *mp++ = i + lwin;
                *mp++ = lwin;
                *mp++ = cb;
            }
            i += 6 * l;
        }
        mapend[j][0] = mp;

        mp = map[j][1] = mapbuf1[j];
        bdf = bi->shortDiff + 0;
        for (i = 0, cb = 0; cb < 13; cb++) {
            int     l = (*bdf++) >> 1;
            for (lwin = 0; lwin < 3; lwin++) {
                *mp++ = l;
                *mp++ = i + lwin;
                *mp++ = lwin;
                *mp++ = cb;
            }
            i += 6 * l;
        }
        mapend[j][1] = mp;

        mp = map[j][2] = mapbuf2[j];
        bdf = bi->longDiff;
        for (cb = 0; cb < 22; cb++) {
            *mp++ = (*bdf++) >> 1;
            *mp++ = cb;
        }
        mapend[j][2] = mp;

    }

    for (j = 0; j < 9; j++) {
        for (i = 0; i < 23; i++) {
            longLimit[j][i] = (bandInfo[j].longIdx[i] - 1 + 8) / 18 + 1;
            if (longLimit[j][i] > (down_sample_sblimit))
                longLimit[j][i] = down_sample_sblimit;
        }
        for (i = 0; i < 14; i++) {
            shortLimit[j][i] = (bandInfo[j].shortIdx[i] - 1) / 18 + 1;
            if (shortLimit[j][i] > (down_sample_sblimit))
                shortLimit[j][i] = down_sample_sblimit;
        }
    }

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 6; j++) {
            for (k = 0; k < 6; k++) {
                int     n = k + j * 6 + i * 36;
                i_slen2[n] = i | (j << 3) | (k << 6) | (3 << 12);
            }
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 4; k++) {
                int     n = k + j * 4 + i * 16;
                i_slen2[n + 180] = i | (j << 3) | (k << 6) | (4 << 12);
            }
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            int     n = j + i * 3;
            i_slen2[n + 244] = i | (j << 3) | (5 << 12);
            n_slen2[n + 500] = i | (j << 3) | (2 << 12) | (1 << 15);
        }
    }

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 4; k++) {
                int     l;
                for (l = 0; l < 4; l++) {
                    int     n = l + k * 4 + j * 16 + i * 80;
                    n_slen2[n] = i | (j << 3) | (k << 6) | (l << 9) | (0 << 12);
                }
            }
        }
    }
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            for (k = 0; k < 4; k++) {
                int     n = k + j * 4 + i * 20;
                n_slen2[n + 400] = i | (j << 3) | (k << 6) | (1 << 12);
            }
        }
    }
}

/*
 * read additional side information
 */

static void
III_get_side_info_1(PMPSTR mp, struct III_sideinfo *si, int stereo,
                    int ms_stereo, long sfreq, int single)
{
    int     ch, gr;
    int     powdiff = (single == 3) ? 4 : 0;

    si->main_data_begin = getbits(mp, 9);
    if (stereo == 1)
        si->private_bits = getbits_fast(mp, 5);
    else
        si->private_bits = getbits_fast(mp, 3);

    for (ch = 0; ch < stereo; ch++) {
        si->ch[ch].gr[0].scfsi = -1;
        si->ch[ch].gr[1].scfsi = getbits_fast(mp, 4);
    }

    for (gr = 0; gr < 2; gr++) {
        for (ch = 0; ch < stereo; ch++) {
            struct gr_info_s *gr_infos = &(si->ch[ch].gr[gr]);

            gr_infos->part2_3_length = getbits(mp, 12);
            gr_infos->big_values = getbits_fast(mp, 9);
            if (gr_infos->big_values > 288) {
                fprintf(stderr, "big_values too large! %i\n", gr_infos->big_values);
                gr_infos->big_values = 288;
            }
            {
                unsigned int qss = getbits_fast(mp, 8);
                gr_infos->pow2gain = gainpow2 + 256 - qss + powdiff;
                if (mp->pinfo != NULL) {
                    mp->pinfo->qss[gr][ch] = qss;
                }
            }
            if (ms_stereo)
                gr_infos->pow2gain += 2;
            gr_infos->scalefac_compress = getbits_fast(mp, 4);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
            if (get1bit(mp)) {
                int     i;
                gr_infos->block_type = getbits_fast(mp, 2);
                gr_infos->mixed_block_flag = get1bit(mp);
                gr_infos->table_select[0] = getbits_fast(mp, 5);
                gr_infos->table_select[1] = getbits_fast(mp, 5);


                /*
                 * table_select[2] not needed, because there is no region2,
                 * but to satisfy some verifications tools we set it either.
                 */
                gr_infos->table_select[2] = 0;
                for (i = 0; i < 3; i++) {
                    unsigned int sbg = (getbits_fast(mp, 3) << 3);
                    gr_infos->full_gain[i] = gr_infos->pow2gain + sbg;
                    if (mp->pinfo != NULL)
                        mp->pinfo->sub_gain[gr][ch][i] = sbg / 8;
                }

                if (gr_infos->block_type == 0) {
                    fprintf(stderr, "Blocktype == 0 and window-switching == 1 not allowed.\n");
                    /* error seems to be very good recoverable, so don't exit */
                    /* exit(1); */
                }
                /* region_count/start parameters are implicit in this case. */
                gr_infos->region1start = 36 >> 1;
                gr_infos->region2start = 576 >> 1;
            }
            else {
                int     i, r0c, r1c;
                for (i = 0; i < 3; i++)
                    gr_infos->table_select[i] = getbits_fast(mp, 5);
                r0c = getbits_fast(mp, 4);
                r1c = getbits_fast(mp, 3);
                gr_infos->region1start = bandInfo[sfreq].longIdx[r0c + 1] >> 1;
                gr_infos->region2start = bandInfo[sfreq].longIdx[r0c + 1 + r1c + 1] >> 1;
                gr_infos->block_type = 0;
                gr_infos->mixed_block_flag = 0;
            }
            gr_infos->preflag = get1bit(mp);
            gr_infos->scalefac_scale = get1bit(mp);
            gr_infos->count1table_select = get1bit(mp);
        }
    }
}

/*
 * Side Info for MPEG 2.0 / LSF
 */
static void
III_get_side_info_2(PMPSTR mp, struct III_sideinfo *si, int stereo,
                    int ms_stereo, long sfreq, int single)
{
    int     ch;
    int     powdiff = (single == 3) ? 4 : 0;

    si->main_data_begin = getbits(mp, 8);

    if (stereo == 1)
        si->private_bits = get1bit(mp);
    else
        si->private_bits = getbits_fast(mp, 2);

    for (ch = 0; ch < stereo; ch++) {
        struct gr_info_s *gr_infos = &(si->ch[ch].gr[0]);
        unsigned int qss;

        gr_infos->part2_3_length = getbits(mp, 12);
        gr_infos->big_values = getbits_fast(mp, 9);
        if (gr_infos->big_values > 288) {
            fprintf(stderr, "big_values too large! %i\n", gr_infos->big_values);
            gr_infos->big_values = 288;
        }
        qss = getbits_fast(mp, 8);
        gr_infos->pow2gain = gainpow2 + 256 - qss + powdiff;
        if (mp->pinfo != NULL) {
            mp->pinfo->qss[0][ch] = qss;
        }


        if (ms_stereo)
            gr_infos->pow2gain += 2;
        gr_infos->scalefac_compress = getbits(mp, 9);
/* window-switching flag == 1 for block_Type != 0 .. and block-type == 0 -> win-sw-flag = 0 */
        if (get1bit(mp)) {
            int     i;
            gr_infos->block_type = getbits_fast(mp, 2);
            gr_infos->mixed_block_flag = get1bit(mp);
            gr_infos->table_select[0] = getbits_fast(mp, 5);
            gr_infos->table_select[1] = getbits_fast(mp, 5);
            /*
             * table_select[2] not needed, because there is no region2,
             * but to satisfy some verifications tools we set it either.
             */
            gr_infos->table_select[2] = 0;
            for (i = 0; i < 3; i++) {
                unsigned int sbg = (getbits_fast(mp, 3) << 3);
                gr_infos->full_gain[i] = gr_infos->pow2gain + sbg;
                if (mp->pinfo != NULL)
                    mp->pinfo->sub_gain[0][ch][i] = sbg / 8;

            }

            if (gr_infos->block_type == 0) {
                fprintf(stderr, "Blocktype == 0 and window-switching == 1 not allowed.\n");
                /* error seems to be very good recoverable, so don't exit */
                /* exit(1); */
            }
            /* region_count/start parameters are implicit in this case. */
/* check this again! */
            if (gr_infos->block_type == 2) {
                if (sfreq == 8)
                    gr_infos->region1start = 36;
                else
                    gr_infos->region1start = 36 >> 1;
            }
            else if (sfreq == 8)
/* check this for 2.5 and sfreq=8 */
                gr_infos->region1start = 108 >> 1;
            else
                gr_infos->region1start = 54 >> 1;
            gr_infos->region2start = 576 >> 1;
        }
        else {
            int     i, r0c, r1c;
            for (i = 0; i < 3; i++)
                gr_infos->table_select[i] = getbits_fast(mp, 5);
            r0c = getbits_fast(mp, 4);
            r1c = getbits_fast(mp, 3);
            gr_infos->region1start = bandInfo[sfreq].longIdx[r0c + 1] >> 1;
            gr_infos->region2start = bandInfo[sfreq].longIdx[r0c + 1 + r1c + 1] >> 1;
            gr_infos->block_type = 0;
            gr_infos->mixed_block_flag = 0;
        }
        gr_infos->scalefac_scale = get1bit(mp);
        gr_infos->count1table_select = get1bit(mp);
    }
}

/*
 * read scalefactors
 */

static int
III_get_scale_factors_1(PMPSTR mp, int *scf, struct gr_info_s *gr_infos)
{
    static const unsigned char slen[2][16] = {
        {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
        {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
    };
    int     numbits;
    int     num0 = slen[0][gr_infos->scalefac_compress];
    int     num1 = slen[1][gr_infos->scalefac_compress];

    if (gr_infos->block_type == 2) {
        int     i = 18;
        numbits = (num0 + num1) * 18;

        if (gr_infos->mixed_block_flag) {
            for (i = 8; i; i--)
                *scf++ = getbits_fast(mp, num0);
            i = 9;
            numbits -= num0; /* num0 * 17 + num1 * 18 */
        }

        for (; i; i--)
            *scf++ = getbits_fast(mp, num0);
        for (i = 18; i; i--)
            *scf++ = getbits_fast(mp, num1);
        *scf++ = 0;
        *scf++ = 0;
        *scf++ = 0;     /* short[13][0..2] = 0 */
    }
    else {
        int     i;
        int     scfsi = gr_infos->scfsi;

        if (scfsi < 0) { /* scfsi < 0 => granule == 0 */
            for (i = 11; i; i--)
                *scf++ = getbits_fast(mp, num0);
            for (i = 10; i; i--)
                *scf++ = getbits_fast(mp, num1);
            numbits = (num0 + num1) * 10 + num0;
        }
        else {
            numbits = 0;
            if (!(scfsi & 0x8)) {
                for (i = 6; i; i--)
                    *scf++ = getbits_fast(mp, num0);
                numbits += num0 * 6;
            }
            else {
                scf += 6;
            }

            if (!(scfsi & 0x4)) {
                for (i = 5; i; i--)
                    *scf++ = getbits_fast(mp, num0);
                numbits += num0 * 5;
            }
            else {
                scf += 5;
            }

            if (!(scfsi & 0x2)) {
                for (i = 5; i; i--)
                    *scf++ = getbits_fast(mp, num1);
                numbits += num1 * 5;
            }
            else {
                scf += 5;
            }

            if (!(scfsi & 0x1)) {
                for (i = 5; i; i--)
                    *scf++ = getbits_fast(mp, num1);
                numbits += num1 * 5;
            }
            else {
                scf += 5;
            }
        }

        *scf++ = 0;     /* no l[21] in original sources */
    }
    return numbits;
}


static int
III_get_scale_factors_2(PMPSTR mp, int *scf, struct gr_info_s *gr_infos, int i_stereo)
{
    unsigned char const *pnt;
    int     i, j;
    unsigned int slen;
    int     n = 0;
    int     numbits = 0;

  /* *INDENT-OFF* */
  static const unsigned char stab[3][6][4] = {
   { { 6, 5, 5,5 } , { 6, 5, 7,3 } , { 11,10,0,0} ,
     { 7, 7, 7,0 } , { 6, 6, 6,3 } , {  8, 8,5,0} } ,
   { { 9, 9, 9,9 } , { 9, 9,12,6 } , { 18,18,0,0} ,
     {12,12,12,0 } , {12, 9, 9,6 } , { 15,12,9,0} } ,
   { { 6, 9, 9,9 } , { 6, 9,12,6 } , { 15,18,0,0} ,
     { 6,15,12,0 } , { 6,12, 9,6 } , {  6,18,9,0} } }; 
  /* *INDENT-ON* */

    if (i_stereo)       /* i_stereo AND second channel -> do_layer3() checks this */
        slen = i_slen2[gr_infos->scalefac_compress >> 1];
    else
        slen = n_slen2[gr_infos->scalefac_compress];

    gr_infos->preflag = (slen >> 15) & 0x1;

    n = 0;
    if (gr_infos->block_type == 2) {
        n++;
        if (gr_infos->mixed_block_flag)
            n++;
    }

    pnt = (unsigned char const *) stab[n][(slen >> 12) & 0x7];

    for (i = 0; i < 4; i++) {
        int     num = slen & 0x7;
        slen >>= 3;
        if (num) {
            for (j = 0; j < (int) (pnt[i]); j++)
                *scf++ = getbits_fast(mp, num);
            numbits += pnt[i] * num;
        }
        else {
            for (j = 0; j < (int) (pnt[i]); j++)
                *scf++ = 0;
        }
    }

    n = (n << 1) + 1;
    for (i = 0; i < n; i++)
        *scf++ = 0;

    return numbits;
}

/* *INDENT-OFF* */
static const int pretab1 [22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0}; /* char enough ? */
static const int pretab2 [22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/* *INDENT-ON* */

/*
 * don't forget to apply the same changes to III_dequantize_sample_ms() !!!
 */
static int
III_dequantize_sample(PMPSTR mp, real xr[SBLIMIT][SSLIMIT], int *scf,
                      struct gr_info_s *gr_infos, int sfreq, int part2bits)
{
    int     shift = 1 + gr_infos->scalefac_scale;
    real   *xrpnt = (real *) xr;
    int     l[3], l3;
    int     part2remain = gr_infos->part2_3_length - part2bits;
    int    *me;

    /* fprintf(stderr,"part2remain = %d, gr_infos->part2_3_length = %d, part2bits = %d\n",
       part2remain, gr_infos->part2_3_length, part2bits); */

    {
        int     i;

        for (i = (&xr[SBLIMIT][0] - xrpnt) >> 1; i > 0; i--) {
            *xrpnt++ = 0.0;
            *xrpnt++ = 0.0;
        }

        xrpnt = (real *) xr;
    }

    {
        int     bv = gr_infos->big_values;
        int     region1 = gr_infos->region1start;
        int     region2 = gr_infos->region2start;

        l3 = ((576 >> 1) - bv) >> 1;
/*
 * we may lose the 'odd' bit here !! 
 * check this later again 
 */
        if (bv <= region1) {
            l[0] = bv;
            l[1] = 0;
            l[2] = 0;
        }
        else {
            l[0] = region1;
            if (bv <= region2) {
                l[1] = bv - l[0];
                l[2] = 0;
            }
            else {
                l[1] = region2 - l[0];
                l[2] = bv - region2;
            }
        }
    }
    /* MDH crash fix */
    {
        int     i;
        for (i = 0; i < 3; i++) {
            if (l[i] < 0) {
                fprintf(stderr, "hip: Bogus region length (%d)\n", l[i]);
                l[i] = 0;
            }
        }
    }
    /* end MDH crash fix */

    if (gr_infos->block_type == 2) {
        /*
         * decoding with short or mixed mode BandIndex table 
         */
        int     i, max[4];
        int     step = 0, lwin = 0, cb = 0;
        real    v = 0.0;
        int    *m, mc;

        if (gr_infos->mixed_block_flag) {
            max[3] = -1;
            max[0] = max[1] = max[2] = 2;
            m = map[sfreq][0];
            me = mapend[sfreq][0];
        }
        else {
            max[0] = max[1] = max[2] = max[3] = -1;
            /* max[3] not really needed in this case */
            m = map[sfreq][1];
            me = mapend[sfreq][1];
        }

        mc = 0;
        for (i = 0; i < 2; i++) {
            int     lp = l[i];
            struct newhuff const *h = (struct newhuff const *) (ht + gr_infos->table_select[i]);
            for (; lp; lp--, mc--) {
                int     x, y;
                if ((!mc)) {
                    mc = *m++;
                    xrpnt = ((real *) xr) + (*m++);
                    lwin = *m++;
                    cb = *m++;
                    if (lwin == 3) {
                        v = gr_infos->pow2gain[(*scf++) << shift];
                        step = 1;
                    }
                    else {
                        v = gr_infos->full_gain[lwin][(*scf++) << shift];
                        step = 3;
                    }
                }
                {
                    short const *val = (short const *) h->table;
                    while ((y = *val++) < 0) {
                        if (get1bit(mp))
                            val -= y;
                        part2remain--;
                    }
                    x = y >> 4;
                    y &= 0xf;
                }
                if (x == 15) {
                    max[lwin] = cb;
                    part2remain -= h->linbits + 1;
                    x += getbits(mp, (int) h->linbits);
                    if (get1bit(mp))
                        *xrpnt = -ispow[x] * v;
                    else
                        *xrpnt = ispow[x] * v;
                }
                else if (x) {
                    max[lwin] = cb;
                    if (get1bit(mp))
                        *xrpnt = -ispow[x] * v;
                    else
                        *xrpnt = ispow[x] * v;
                    part2remain--;
                }
                else
                    *xrpnt = 0.0;
                xrpnt += step;
                if (y == 15) {
                    max[lwin] = cb;
                    part2remain -= h->linbits + 1;
                    y += getbits(mp, (int) h->linbits);
                    if (get1bit(mp))
                        *xrpnt = -ispow[y] * v;
                    else
                        *xrpnt = ispow[y] * v;
                }
                else if (y) {
                    max[lwin] = cb;
                    if (get1bit(mp))
                        *xrpnt = -ispow[y] * v;
                    else
                        *xrpnt = ispow[y] * v;
                    part2remain--;
                }
                else
                    *xrpnt = 0.0;
                xrpnt += step;
            }
        }
        for (; l3 && (part2remain > 0); l3--) {
            struct newhuff const *h = (struct newhuff const *) (htc + gr_infos->count1table_select);
            short const *val = (short const *) h->table;
            short   a;

            while ((a = *val++) < 0) {
                part2remain--;
                if (part2remain < 0) {
                    part2remain++;
                    a = 0;
                    break;
                }
                if (get1bit(mp))
                    val -= a;
            }
            for (i = 0; i < 4; i++) {
                if (!(i & 1)) {
                    if (!mc) {
                        mc = *m++;
                        xrpnt = ((real *) xr) + (*m++);
                        lwin = *m++;
                        cb = *m++;
                        if (lwin == 3) {
                            v = gr_infos->pow2gain[(*scf++) << shift];
                            step = 1;
                        }
                        else {
                            v = gr_infos->full_gain[lwin][(*scf++) << shift];
                            step = 3;
                        }
                    }
                    mc--;
                }
                if ((a & (0x8 >> i))) {
                    max[lwin] = cb;
                    part2remain--;
                    if (part2remain < 0) {
                        part2remain++;
                        break;
                    }
                    if (get1bit(mp))
                        *xrpnt = -v;
                    else
                        *xrpnt = v;
                }
                else
                    *xrpnt = 0.0;
                xrpnt += step;
            }
        }

        while (m < me) {
            if (!mc) {
                mc = *m++;
                xrpnt = ((real *) xr) + *m++;
                if ((*m++) == 3)
                    step = 1;
                else
                    step = 3;
                m++;    /* cb */
            }
            mc--;
            *xrpnt = 0.0;
            xrpnt += step;
            *xrpnt = 0.0;
            xrpnt += step;
/* we could add a little opt. here:
 * if we finished a band for window 3 or a long band
 * further bands could copied in a simple loop without a
 * special 'map' decoding
 */
        }

        gr_infos->maxband[0] = max[0] + 1;
        gr_infos->maxband[1] = max[1] + 1;
        gr_infos->maxband[2] = max[2] + 1;
        gr_infos->maxbandl = max[3] + 1;

        {
            int     rmax = max[0] > max[1] ? max[0] : max[1];
            rmax = (rmax > max[2] ? rmax : max[2]) + 1;
            gr_infos->maxb = rmax ? shortLimit[sfreq][rmax] : longLimit[sfreq][max[3] + 1];
        }

    }
    else {
        /*
         * decoding with 'long' BandIndex table (block_type != 2)
         */
        int const *pretab = (int const *) (gr_infos->preflag ? pretab1 : pretab2);
        int     i, max = -1;
        int     cb = 0;
        int    *m = map[sfreq][2];
        real    v = 0.0;
        int     mc = 0;

        /*
         * long hash table values
         */
        for (i = 0; i < 3; i++) {
            int     lp = l[i];
            struct newhuff const *h = (struct newhuff const *) (ht + gr_infos->table_select[i]);

            for (; lp; lp--, mc--) {
                int     x, y;

                if (!mc) {
                    mc = *m++;
                    v = gr_infos->pow2gain[((*scf++) + (*pretab++)) << shift];
                    cb = *m++;
                }
                {
                    short const *val = (short const *) h->table;
                    while ((y = *val++) < 0) {
                        if (get1bit(mp))
                            val -= y;
                        part2remain--;
                    }
                    x = y >> 4;
                    y &= 0xf;
                }
                if (x == 15) {
                    max = cb;
                    part2remain -= h->linbits + 1;
                    x += getbits(mp, (int) h->linbits);
                    if (get1bit(mp))
                        *xrpnt++ = -ispow[x] * v;
                    else
                        *xrpnt++ = ispow[x] * v;
                }
                else if (x) {
                    max = cb;
                    if (get1bit(mp))
                        *xrpnt++ = -ispow[x] * v;
                    else
                        *xrpnt++ = ispow[x] * v;
                    part2remain--;
                }
                else
                    *xrpnt++ = 0.0;

                if (y == 15) {
                    max = cb;
                    part2remain -= h->linbits + 1;
                    y += getbits(mp, (int) h->linbits);
                    if (get1bit(mp))
                        *xrpnt++ = -ispow[y] * v;
                    else
                        *xrpnt++ = ispow[y] * v;
                }
                else if (y) {
                    max = cb;
                    if (get1bit(mp))
                        *xrpnt++ = -ispow[y] * v;
                    else
                        *xrpnt++ = ispow[y] * v;
                    part2remain--;
                }
                else
                    *xrpnt++ = 0.0;
            }
        }

        /*
         * short (count1table) values
         */
        for (; l3 && (part2remain > 0); l3--) {
            struct newhuff const *h = (struct newhuff const *) (htc + gr_infos->count1table_select);
            short const *val = (short const *) h->table;
            short   a;

            while ((a = *val++) < 0) {
                part2remain--;
                if (part2remain < 0) {
                    part2remain++;
                    a = 0;
                    break;
                }
                if (get1bit(mp))
                    val -= a;
            }
            for (i = 0; i < 4; i++) {
                if (!(i & 1)) {
                    if (!mc) {
                        mc = *m++;
                        cb = *m++;
                        v = gr_infos->pow2gain[((*scf++) + (*pretab++)) << shift];
                    }
                    mc--;
                }
                if ((a & (0x8 >> i))) {
                    max = cb;
                    part2remain--;
                    if (part2remain < 0) {
                        part2remain++;
                        break;
                    }
                    if (get1bit(mp))
                        *xrpnt++ = -v;
                    else
                        *xrpnt++ = v;
                }
                else
                    *xrpnt++ = 0.0;
            }
        }

        /* 
         * zero part
         */
        for (i = (&xr[SBLIMIT][0] - xrpnt) >> 1; i; i--) {
            *xrpnt++ = 0.0;
            *xrpnt++ = 0.0;
        }

        gr_infos->maxbandl = max + 1;
        gr_infos->maxb = longLimit[sfreq][gr_infos->maxbandl];
    }

    while (part2remain > 16) {
        getbits(mp, 16); /* Dismiss stuffing Bits */
        part2remain -= 16;
    }
    if (part2remain > 0)
        getbits(mp, part2remain);
    else if (part2remain < 0) {
        fprintf(stderr, "hip: Can't rewind stream by %d bits!\n", -part2remain);
        return 1;       /* -> error */
    }
    return 0;
}


/* 
 * III_stereo: calculate real channel values for Joint-I-Stereo-mode
 */
static void
III_i_stereo(real xr_buf[2][SBLIMIT][SSLIMIT], int *scalefac,
             struct gr_info_s *gr_infos, int sfreq, int ms_stereo, int lsf)
{
    real(*xr)[SBLIMIT * SSLIMIT] = (real(*)[SBLIMIT * SSLIMIT]) xr_buf;
    struct bandInfoStruct const *bi = (struct bandInfoStruct const *) &bandInfo[sfreq];
    real   *tabl1, *tabl2;

    if (lsf) {
        int     p = gr_infos->scalefac_compress & 0x1;
        if (ms_stereo) {
            tabl1 = pow1_2[p];
            tabl2 = pow2_2[p];
        }
        else {
            tabl1 = pow1_1[p];
            tabl2 = pow2_1[p];
        }
    }
    else {
        if (ms_stereo) {
            tabl1 = tan1_2;
            tabl2 = tan2_2;
        }
        else {
            tabl1 = tan1_1;
            tabl2 = tan2_1;
        }
    }

    if (gr_infos->block_type == 2) {
        int     lwin, do_l = 0;
        if (gr_infos->mixed_block_flag)
            do_l = 1;

        for (lwin = 0; lwin < 3; lwin++) { /* process each window */
            /* get first band with zero values */
            int     is_p, sb, idx, sfb = gr_infos->maxband[lwin]; /* sfb is minimal 3 for mixed mode */
            if (sfb > 3)
                do_l = 0;

            for (; sfb < 12; sfb++) {
                is_p = scalefac[sfb * 3 + lwin - gr_infos->mixed_block_flag]; /* scale: 0-15 */
                if (is_p != 7) {
                    real    t1, t2;
                    sb = bi->shortDiff[sfb];
                    idx = bi->shortIdx[sfb] + lwin;
                    t1 = tabl1[is_p];
                    t2 = tabl2[is_p];
                    for (; sb > 0; sb--, idx += 3) {
                        real    v = xr[0][idx];
                        xr[0][idx] = v * t1;
                        xr[1][idx] = v * t2;
                    }
                }
            }

#if 1
/* in the original: copy 10 to 11 , here: copy 11 to 12 
maybe still wrong??? (copy 12 to 13?) */
            is_p = scalefac[11 * 3 + lwin - gr_infos->mixed_block_flag]; /* scale: 0-15 */
            sb = bi->shortDiff[12];
            idx = bi->shortIdx[12] + lwin;
#else
            is_p = scalefac[10 * 3 + lwin - gr_infos->mixed_block_flag]; /* scale: 0-15 */
            sb = bi->shortDiff[11];
            idx = bi->shortIdx[11] + lwin;
#endif
            if (is_p != 7) {
                real    t1, t2;
                t1 = tabl1[is_p];
                t2 = tabl2[is_p];
                for (; sb > 0; sb--, idx += 3) {
                    real    v = xr[0][idx];
                    xr[0][idx] = v * t1;
                    xr[1][idx] = v * t2;
                }
            }
        }               /* end for(lwin; .. ; . ) */

        if (do_l) {
/* also check l-part, if ALL bands in the three windows are 'empty'
 * and mode = mixed_mode 
 */
            int     sfb = gr_infos->maxbandl;
            int     idx = bi->longIdx[sfb];

            for (; sfb < 8; sfb++) {
                int     sb = bi->longDiff[sfb];
                int     is_p = scalefac[sfb]; /* scale: 0-15 */
                if (is_p != 7) {
                    real    t1, t2;
                    t1 = tabl1[is_p];
                    t2 = tabl2[is_p];
                    for (; sb > 0; sb--, idx++) {
                        real    v = xr[0][idx];
                        xr[0][idx] = v * t1;
                        xr[1][idx] = v * t2;
                    }
                }
                else
                    idx += sb;
            }
        }
    }
    else {              /* ((gr_infos->block_type != 2)) */

        int     sfb = gr_infos->maxbandl;
        int     is_p, idx = bi->longIdx[sfb];
        for (; sfb < 21; sfb++) {
            int     sb = bi->longDiff[sfb];
            is_p = scalefac[sfb]; /* scale: 0-15 */
            if (is_p != 7) {
                real    t1, t2;
                t1 = tabl1[is_p];
                t2 = tabl2[is_p];
                for (; sb > 0; sb--, idx++) {
                    real    v = xr[0][idx];
                    xr[0][idx] = v * t1;
                    xr[1][idx] = v * t2;
                }
            }
            else
                idx += sb;
        }

        is_p = scalefac[20]; /* copy l-band 20 to l-band 21 */
        if (is_p != 7) {
            int     sb;
            real    t1 = tabl1[is_p], t2 = tabl2[is_p];

            for (sb = bi->longDiff[21]; sb > 0; sb--, idx++) {
                real    v = xr[0][idx];
                xr[0][idx] = v * t1;
                xr[1][idx] = v * t2;
            }
        }
    }                   /* ... */
}

static void
III_antialias(real xr[SBLIMIT][SSLIMIT], struct gr_info_s *gr_infos)
{
    int     sblim;

    if (gr_infos->block_type == 2) {
        if (!gr_infos->mixed_block_flag)
            return;
        sblim = 1;
    }
    else {
        sblim = gr_infos->maxb - 1;
    }

    /* 31 alias-reduction operations between each pair of sub-bands */
    /* with 8 butterflies between each pair                         */

    {
        int     sb;
        real   *xr1 = (real *) xr[1];

        for (sb = sblim; sb; sb--, xr1 += 10) {
            int     ss;
            real   *cs = aa_cs, *ca = aa_ca;
            real   *xr2 = xr1;

            for (ss = 7; ss >= 0; ss--) { /* upper and lower butterfly inputs */
                real    bu = *--xr2, bd = *xr1;
                *xr2 = (bu * (*cs)) - (bd * (*ca));
                *xr1++ = (bd * (*cs++)) + (bu * (*ca++));
            }
        }
    }
}


/* *INDENT-OFF* */

/*
 DCT insipired by Jeff Tsay's DCT from the maplay package
 this is an optimized version with manual unroll.

 References:
 [1] S. Winograd: "On Computing the Discrete Fourier Transform",
     Mathematics of Computation, Volume 32, Number 141, January 1978,
     Pages 175-199
*/

static void dct36(real *inbuf,real *o1,real *o2,real *wintab,real *tsbuf)
{
  {
    real *in = inbuf;

    in[17]+=in[16]; in[16]+=in[15]; in[15]+=in[14];
    in[14]+=in[13]; in[13]+=in[12]; in[12]+=in[11];
    in[11]+=in[10]; in[10]+=in[9];  in[9] +=in[8];
    in[8] +=in[7];  in[7] +=in[6];  in[6] +=in[5];
    in[5] +=in[4];  in[4] +=in[3];  in[3] +=in[2];
    in[2] +=in[1];  in[1] +=in[0];

    in[17]+=in[15]; in[15]+=in[13]; in[13]+=in[11]; in[11]+=in[9];
    in[9] +=in[7];  in[7] +=in[5];  in[5] +=in[3];  in[3] +=in[1];

  {

#define MACRO0(v) { \
    real tmp; \
    out2[9+(v)] = (tmp = sum0 + sum1) * w[27+(v)]; \
    out2[8-(v)] = tmp * w[26-(v)];  } \
    sum0 -= sum1; \
    ts[SBLIMIT*(8-(v))] = out1[8-(v)] + sum0 * w[8-(v)]; \
    ts[SBLIMIT*(9+(v))] = out1[9+(v)] + sum0 * w[9+(v)]; 
#define MACRO1(v) { \
    real sum0,sum1; \
    sum0 = tmp1a + tmp2a; \
    sum1 = (tmp1b + tmp2b) * tfcos36[(v)]; \
    MACRO0(v); }
#define MACRO2(v) { \
    real sum0,sum1; \
    sum0 = tmp2a - tmp1a; \
    sum1 = (tmp2b - tmp1b) * tfcos36[(v)]; \
    MACRO0(v); }

    const real *c = COS9;
    real *out2 = o2;
    real *w = wintab;
    real *out1 = o1;
    real *ts = tsbuf;

    real ta33,ta66,tb33,tb66;

    ta33 = in[2*3+0] * c[3];
    ta66 = in[2*6+0] * c[6];
    tb33 = in[2*3+1] * c[3];
    tb66 = in[2*6+1] * c[6];

    { 
      real tmp1a,tmp2a,tmp1b,tmp2b;
      tmp1a =             in[2*1+0] * c[1] + ta33 + in[2*5+0] * c[5] + in[2*7+0] * c[7];
      tmp1b =             in[2*1+1] * c[1] + tb33 + in[2*5+1] * c[5] + in[2*7+1] * c[7];
      tmp2a = in[2*0+0] + in[2*2+0] * c[2] + in[2*4+0] * c[4] + ta66 + in[2*8+0] * c[8];
      tmp2b = in[2*0+1] + in[2*2+1] * c[2] + in[2*4+1] * c[4] + tb66 + in[2*8+1] * c[8];

      MACRO1(0);
      MACRO2(8);
    }

    {
      real tmp1a,tmp2a,tmp1b,tmp2b;
      tmp1a = ( in[2*1+0] - in[2*5+0] - in[2*7+0] ) * c[3];
      tmp1b = ( in[2*1+1] - in[2*5+1] - in[2*7+1] ) * c[3];
      tmp2a = ( in[2*2+0] - in[2*4+0] - in[2*8+0] ) * c[6] - in[2*6+0] + in[2*0+0];
      tmp2b = ( in[2*2+1] - in[2*4+1] - in[2*8+1] ) * c[6] - in[2*6+1] + in[2*0+1];

      MACRO1(1);
      MACRO2(7);
    }

    {
      real tmp1a,tmp2a,tmp1b,tmp2b;
      tmp1a =             in[2*1+0] * c[5] - ta33 - in[2*5+0] * c[7] + in[2*7+0] * c[1];
      tmp1b =             in[2*1+1] * c[5] - tb33 - in[2*5+1] * c[7] + in[2*7+1] * c[1];
      tmp2a = in[2*0+0] - in[2*2+0] * c[8] - in[2*4+0] * c[2] + ta66 + in[2*8+0] * c[4];
      tmp2b = in[2*0+1] - in[2*2+1] * c[8] - in[2*4+1] * c[2] + tb66 + in[2*8+1] * c[4];

      MACRO1(2);
      MACRO2(6);
    }

    {
      real tmp1a,tmp2a,tmp1b,tmp2b;
      tmp1a =             in[2*1+0] * c[7] - ta33 + in[2*5+0] * c[1] - in[2*7+0] * c[5];
      tmp1b =             in[2*1+1] * c[7] - tb33 + in[2*5+1] * c[1] - in[2*7+1] * c[5];
      tmp2a = in[2*0+0] - in[2*2+0] * c[4] + in[2*4+0] * c[8] + ta66 - in[2*8+0] * c[2];
      tmp2b = in[2*0+1] - in[2*2+1] * c[4] + in[2*4+1] * c[8] + tb66 - in[2*8+1] * c[2];

      MACRO1(3);
      MACRO2(5);
    }

    {
      real sum0,sum1;
      sum0 =  in[2*0+0] - in[2*2+0] + in[2*4+0] - in[2*6+0] + in[2*8+0];
      sum1 = (in[2*0+1] - in[2*2+1] + in[2*4+1] - in[2*6+1] + in[2*8+1] ) * tfcos36[4];
      MACRO0(4);
    }
  }

  }
}


/*
 * new DCT12
 */
static void dct12(real *in,real *rawout1,real *rawout2,real *wi,real *ts)
{
#define DCT12_PART1 \
             in5 = in[5*3];  \
     in5 += (in4 = in[4*3]); \
     in4 += (in3 = in[3*3]); \
     in3 += (in2 = in[2*3]); \
     in2 += (in1 = in[1*3]); \
     in1 += (in0 = in[0*3]); \
                             \
     in5 += in3; in3 += in1; \
                             \
     in2 *= COS6_1; \
     in3 *= COS6_1; \

#define DCT12_PART2 \
     in0 += in4 * COS6_2; \
                          \
     in4 = in0 + in2;     \
     in0 -= in2;          \
                          \
     in1 += in5 * COS6_2; \
                          \
     in5 = (in1 + in3) * tfcos12[0]; \
     in1 = (in1 - in3) * tfcos12[2]; \
                         \
     in3 = in4 + in5;    \
     in4 -= in5;         \
                         \
     in2 = in0 + in1;    \
     in0 -= in1;


   {
     real in0,in1,in2,in3,in4,in5;
     real *out1 = rawout1;
     ts[SBLIMIT*0] = out1[0]; ts[SBLIMIT*1] = out1[1]; ts[SBLIMIT*2] = out1[2];
     ts[SBLIMIT*3] = out1[3]; ts[SBLIMIT*4] = out1[4]; ts[SBLIMIT*5] = out1[5];
 
     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = (in1 - in5) * tfcos12[1];
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       ts[(17-1)*SBLIMIT] = out1[17-1] + tmp0 * wi[11-1];
       ts[(12+1)*SBLIMIT] = out1[12+1] + tmp0 * wi[6+1];
       ts[(6 +1)*SBLIMIT] = out1[6 +1] + tmp1 * wi[1];
       ts[(11-1)*SBLIMIT] = out1[11-1] + tmp1 * wi[5-1];
     }

     DCT12_PART2

     ts[(17-0)*SBLIMIT] = out1[17-0] + in2 * wi[11-0];
     ts[(12+0)*SBLIMIT] = out1[12+0] + in2 * wi[6+0];
     ts[(12+2)*SBLIMIT] = out1[12+2] + in3 * wi[6+2];
     ts[(17-2)*SBLIMIT] = out1[17-2] + in3 * wi[11-2];

     ts[(6+0)*SBLIMIT]  = out1[6+0] + in0 * wi[0];
     ts[(11-0)*SBLIMIT] = out1[11-0] + in0 * wi[5-0];
     ts[(6+2)*SBLIMIT]  = out1[6+2] + in4 * wi[2];
     ts[(11-2)*SBLIMIT] = out1[11-2] + in4 * wi[5-2];
  }

  in++;

  {
     real in0,in1,in2,in3,in4,in5;
     real *out2 = rawout2;
 
     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = (in1 - in5) * tfcos12[1];
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       out2[5-1] = tmp0 * wi[11-1];
       out2[0+1] = tmp0 * wi[6+1];
       ts[(12+1)*SBLIMIT] += tmp1 * wi[1];
       ts[(17-1)*SBLIMIT] += tmp1 * wi[5-1];
     }

     DCT12_PART2

     out2[5-0] = in2 * wi[11-0];
     out2[0+0] = in2 * wi[6+0];
     out2[0+2] = in3 * wi[6+2];
     out2[5-2] = in3 * wi[11-2];

     ts[(12+0)*SBLIMIT] += in0 * wi[0];
     ts[(17-0)*SBLIMIT] += in0 * wi[5-0];
     ts[(12+2)*SBLIMIT] += in4 * wi[2];
     ts[(17-2)*SBLIMIT] += in4 * wi[5-2];
  }

  in++; 

  {
     real in0,in1,in2,in3,in4,in5;
     real *out2 = rawout2;
     out2[12]=out2[13]=out2[14]=out2[15]=out2[16]=out2[17]=0.0;

     DCT12_PART1

     {
       real tmp0,tmp1 = (in0 - in4);
       {
         real tmp2 = (in1 - in5) * tfcos12[1];
         tmp0 = tmp1 + tmp2;
         tmp1 -= tmp2;
       }
       out2[11-1] = tmp0 * wi[11-1];
       out2[6 +1] = tmp0 * wi[6+1];
       out2[0+1] += tmp1 * wi[1];
       out2[5-1] += tmp1 * wi[5-1];
     }

     DCT12_PART2

     out2[11-0] = in2 * wi[11-0];
     out2[6 +0] = in2 * wi[6+0];
     out2[6 +2] = in3 * wi[6+2];
     out2[11-2] = in3 * wi[11-2];

     out2[0+0] += in0 * wi[0];
     out2[5-0] += in0 * wi[5-0];
     out2[0+2] += in4 * wi[2];
     out2[5-2] += in4 * wi[5-2];
  }
}
/* *INDENT-ON* */

/*
 * III_hybrid
 */
static void
III_hybrid(PMPSTR mp, real fsIn[SBLIMIT][SSLIMIT], real tsOut[SSLIMIT][SBLIMIT],
           int ch, struct gr_info_s *gr_infos)
{
    real   *tspnt = (real *) tsOut;
    real(*block)[2][SBLIMIT * SSLIMIT] = mp->hybrid_block;
    int    *blc = mp->hybrid_blc;
    real   *rawout1, *rawout2;
    int     bt;
    int     sb = 0;

    {
        int     b = blc[ch];
        rawout1 = block[b][ch];
        b = -b + 1;
        rawout2 = block[b][ch];
        blc[ch] = b;
    }


    if (gr_infos->mixed_block_flag) {
        sb = 2;
        dct36(fsIn[0], rawout1, rawout2, win[0], tspnt);
        dct36(fsIn[1], rawout1 + 18, rawout2 + 18, win1[0], tspnt + 1);
        rawout1 += 36;
        rawout2 += 36;
        tspnt += 2;
    }

    bt = gr_infos->block_type;
    if (bt == 2) {
        for (; sb < (int) gr_infos->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36) {
            dct12(fsIn[sb], rawout1, rawout2, win[2], tspnt);
            dct12(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, win1[2], tspnt + 1);
        }
    }
    else {
        for (; sb < (int) gr_infos->maxb; sb += 2, tspnt += 2, rawout1 += 36, rawout2 += 36) {
            dct36(fsIn[sb], rawout1, rawout2, win[bt], tspnt);
            dct36(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, win1[bt], tspnt + 1);
        }
    }

    for (; sb < SBLIMIT; sb++, tspnt++) {
        int     i;
        for (i = 0; i < SSLIMIT; i++) {
            tspnt[i * SBLIMIT] = *rawout1++;
            *rawout2++ = 0.0;
        }
    }
}

/*
 * main layer3 handler
 */
struct III_sideinfo sideinfo;

int
layer3_audiodata_precedesframes(PMPSTR mp)
{
    int     audioDataInFrame;
    int     framesToBacktrack;

    /* specific to Layer 3, since Layer 1 & 2 the audio data starts at the frame that describes it. */
    /* determine how many bytes and therefore bitstream frames the audio data precedes it's matching frame */
    /* fprintf(stderr, "hip: main_data_begin = %d, mp->bsize %d, mp->fsizeold %d, mp->ssize %d\n",
       sideinfo.main_data_begin, mp->bsize, mp->fsizeold, mp->ssize); */
    /* compute the number of frames to backtrack, 4 for the header, ssize already holds the CRC */
    /* TODO Erroneously assumes current frame is same as previous frame. */
    audioDataInFrame = mp->bsize - 4 - mp->ssize;
    framesToBacktrack = (sideinfo.main_data_begin + audioDataInFrame - 1) / audioDataInFrame;
    /* fprintf(stderr, "hip: audioDataInFrame %d framesToBacktrack %d\n", audioDataInFrame, framesToBacktrack); */
    return framesToBacktrack;
}

int
do_layer3_sideinfo(PMPSTR mp)
{
    struct frame *fr = &mp->fr;
    int     stereo = fr->stereo;
    int     single = fr->single;
    int     ms_stereo;
    int     sfreq = fr->sampling_frequency;
    int     granules;
    int     ch, gr, databits;

    if (stereo == 1) {  /* stream is mono */
        single = 0;
    }

    if (fr->mode == MPG_MD_JOINT_STEREO) {
        ms_stereo = fr->mode_ext & 0x2;
    }
    else
        ms_stereo = 0;


    if (fr->lsf) {
        granules = 1;
        III_get_side_info_2(mp, &sideinfo, stereo, ms_stereo, sfreq, single);
    }
    else {
        granules = 2;
        III_get_side_info_1(mp, &sideinfo, stereo, ms_stereo, sfreq, single);
    }

    databits = 0;
    for (gr = 0; gr < granules; ++gr) {
        for (ch = 0; ch < stereo; ++ch) {
            struct gr_info_s *gr_infos = &(sideinfo.ch[ch].gr[gr]);
            databits += gr_infos->part2_3_length;
        }
    }
    return databits - 8 * sideinfo.main_data_begin;
}



int
do_layer3(PMPSTR mp, unsigned char *pcm_sample, int *pcm_point,
          int (*synth_1to1_mono_ptr) (PMPSTR, real *, unsigned char *, int *),
          int (*synth_1to1_ptr) (PMPSTR, real *, int, unsigned char *, int *))
{
    int     gr, ch, ss, clip = 0;
    int     scalefacs[2][39]; /* max 39 for short[13][3] mode, mixed: 38, long: 22 */
    /*  struct III_sideinfo sideinfo; */
    struct frame *fr = &(mp->fr);
    int     stereo = fr->stereo;
    int     single = fr->single;
    int     ms_stereo, i_stereo;
    int     sfreq = fr->sampling_frequency;
    int     stereo1, granules;

    if (set_pointer(mp, (int) sideinfo.main_data_begin) == MP3_ERR)
        return 0;

    if (stereo == 1) {  /* stream is mono */
        stereo1 = 1;
        single = 0;
    }
    else if (single >= 0) /* stream is stereo, but force to mono */
        stereo1 = 1;
    else
        stereo1 = 2;

    if (fr->mode == MPG_MD_JOINT_STEREO) {
        ms_stereo = fr->mode_ext & 0x2;
        i_stereo = fr->mode_ext & 0x1;
    }
    else
        ms_stereo = i_stereo = 0;


    if (fr->lsf) {
        granules = 1;
    }
    else {
        granules = 2;
    }

    for (gr = 0; gr < granules; gr++) {
        static real hybridIn[2][SBLIMIT][SSLIMIT];
        static real hybridOut[2][SSLIMIT][SBLIMIT];

        {
            struct gr_info_s *gr_infos = &(sideinfo.ch[0].gr[gr]);
            long    part2bits;

            if (fr->lsf)
                part2bits = III_get_scale_factors_2(mp, scalefacs[0], gr_infos, 0);
            else {
                part2bits = III_get_scale_factors_1(mp, scalefacs[0], gr_infos);
            }

            if (mp->pinfo != NULL) {
                int     i;
                mp->pinfo->sfbits[gr][0] = part2bits;
                for (i = 0; i < 39; i++)
                    mp->pinfo->sfb_s[gr][0][i] = scalefacs[0][i];
            }

            /* fprintf(stderr, "calling III dequantize sample 1 gr_infos->part2_3_length %d\n", gr_infos->part2_3_length); */
            if (III_dequantize_sample(mp, hybridIn[0], scalefacs[0], gr_infos, sfreq, part2bits))
                return clip;
        }
        if (stereo == 2) {
            struct gr_info_s *gr_infos = &(sideinfo.ch[1].gr[gr]);
            long    part2bits;
            if (fr->lsf)
                part2bits = III_get_scale_factors_2(mp, scalefacs[1], gr_infos, i_stereo);
            else {
                part2bits = III_get_scale_factors_1(mp, scalefacs[1], gr_infos);
            }
            if (mp->pinfo != NULL) {
                int     i;
                mp->pinfo->sfbits[gr][1] = part2bits;
                for (i = 0; i < 39; i++)
                    mp->pinfo->sfb_s[gr][1][i] = scalefacs[1][i];
            }

            /* fprintf(stderr, "calling III dequantize sample 2  gr_infos->part2_3_length %d\n", gr_infos->part2_3_length); */
            if (III_dequantize_sample(mp, hybridIn[1], scalefacs[1], gr_infos, sfreq, part2bits))
                return clip;

            if (ms_stereo) {
                int     i;
                for (i = 0; i < SBLIMIT * SSLIMIT; i++) {
                    real    tmp0, tmp1;
                    tmp0 = ((real *) hybridIn[0])[i];
                    tmp1 = ((real *) hybridIn[1])[i];
                    ((real *) hybridIn[1])[i] = tmp0 - tmp1;
                    ((real *) hybridIn[0])[i] = tmp0 + tmp1;
                }
            }

            if (i_stereo)
                III_i_stereo(hybridIn, scalefacs[1], gr_infos, sfreq, ms_stereo, fr->lsf);

            if (ms_stereo || i_stereo || (single == 3)) {
                if (gr_infos->maxb > sideinfo.ch[0].gr[gr].maxb)
                    sideinfo.ch[0].gr[gr].maxb = gr_infos->maxb;
                else
                    gr_infos->maxb = sideinfo.ch[0].gr[gr].maxb;
            }

            switch (single) {
            case 3:
                {
                    int     i;
                    real   *in0 = (real *) hybridIn[0], *in1 = (real *) hybridIn[1];
                    for (i = 0; i < (int) (SSLIMIT * gr_infos->maxb); i++, in0++)
                        *in0 = (*in0 + *in1++); /* *0.5 done by pow-scale */
                }
                break;
            case 1:
                {
                    int     i;
                    real   *in0 = (real *) hybridIn[0], *in1 = (real *) hybridIn[1];
                    for (i = 0; i < (int) (SSLIMIT * gr_infos->maxb); i++)
                        *in0++ = *in1++;
                }
                break;
            }
        }

        if (mp->pinfo != NULL) {
            int     i, sb;
            float   ifqstep;

            mp->pinfo->bitrate = tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index];
            mp->pinfo->sampfreq = freqs[sfreq];
            mp->pinfo->emph = fr->emphasis;
            mp->pinfo->crc = fr->error_protection;
            mp->pinfo->padding = fr->padding;
            mp->pinfo->stereo = fr->stereo;
            mp->pinfo->js = (fr->mode == MPG_MD_JOINT_STEREO);
            mp->pinfo->ms_stereo = ms_stereo;
            mp->pinfo->i_stereo = i_stereo;
            mp->pinfo->maindata = sideinfo.main_data_begin;

            for (ch = 0; ch < stereo1; ch++) {
                struct gr_info_s *gr_infos = &(sideinfo.ch[ch].gr[gr]);
                mp->pinfo->big_values[gr][ch] = gr_infos->big_values;
                mp->pinfo->scalefac_scale[gr][ch] = gr_infos->scalefac_scale;
                mp->pinfo->mixed[gr][ch] = gr_infos->mixed_block_flag;
                mp->pinfo->mpg123blocktype[gr][ch] = gr_infos->block_type;
                mp->pinfo->mainbits[gr][ch] = gr_infos->part2_3_length;
                mp->pinfo->preflag[gr][ch] = gr_infos->preflag;
                if (gr == 1)
                    mp->pinfo->scfsi[ch] = gr_infos->scfsi;
            }


            for (ch = 0; ch < stereo1; ch++) {
                struct gr_info_s *gr_infos = &(sideinfo.ch[ch].gr[gr]);
                ifqstep = (mp->pinfo->scalefac_scale[gr][ch] == 0) ? .5 : 1.0;
                if (2 == gr_infos->block_type) {
                    for (i = 0; i < 3; i++) {
                        for (sb = 0; sb < 12; sb++) {
                            int     j = 3 * sb + i;
                            /*
                               is_p = scalefac[sfb*3+lwin-gr_infos->mixed_block_flag]; 
                             */
                            /* scalefac was copied into pinfo->sfb_s[] above */
                            mp->pinfo->sfb_s[gr][ch][j] =
                                -ifqstep * mp->pinfo->sfb_s[gr][ch][j - gr_infos->mixed_block_flag];
                            mp->pinfo->sfb_s[gr][ch][j] -= 2 * (mp->pinfo->sub_gain[gr][ch][i]);
                        }
                        mp->pinfo->sfb_s[gr][ch][3 * sb + i] =
                            -2 * (mp->pinfo->sub_gain[gr][ch][i]);
                    }
                }
                else {
                    for (sb = 0; sb < 21; sb++) {
                        /* scalefac was copied into pinfo->sfb[] above */
                        mp->pinfo->sfb[gr][ch][sb] = mp->pinfo->sfb_s[gr][ch][sb];
                        if (gr_infos->preflag)
                            mp->pinfo->sfb[gr][ch][sb] += pretab1[sb];
                        mp->pinfo->sfb[gr][ch][sb] *= -ifqstep;
                    }
                    mp->pinfo->sfb[gr][ch][21] = 0;
                }
            }



            for (ch = 0; ch < stereo1; ch++) {
                int     j = 0;
                for (sb = 0; sb < SBLIMIT; sb++)
                    for (ss = 0; ss < SSLIMIT; ss++, j++)
                        mp->pinfo->mpg123xr[gr][ch][j] = hybridIn[ch][sb][ss];
            }
        }


        for (ch = 0; ch < stereo1; ch++) {
            struct gr_info_s *gr_infos = &(sideinfo.ch[ch].gr[gr]);
            III_antialias(hybridIn[ch], gr_infos);
            III_hybrid(mp, hybridIn[ch], hybridOut[ch], ch, gr_infos);
        }

        for (ss = 0; ss < SSLIMIT; ss++) {
            if (single >= 0) {
                clip += (*synth_1to1_mono_ptr) (mp, hybridOut[0][ss], pcm_sample, pcm_point);
            }
            else {
                int     p1 = *pcm_point;
                clip += (*synth_1to1_ptr) (mp, hybridOut[0][ss], 0, pcm_sample, &p1);
                clip += (*synth_1to1_ptr) (mp, hybridOut[1][ss], 1, pcm_sample, pcm_point);
            }
        }
    }

    return clip;
}
