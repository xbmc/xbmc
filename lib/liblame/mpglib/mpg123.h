/*
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
#ifndef MPG123_H_INCLUDED
#define MPG123_H_INCLUDED

#include        <stdio.h>

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#include        <signal.h>


#if defined(__riscos__) && defined(FPA10)
#include "ymath.h"
#else
#include <math.h>
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2    1.41421356237309504880
#endif

#ifndef FALSE
#define         FALSE                   0
#endif
#ifndef TRUE
#define         TRUE                    1
#endif

#undef REAL_IS_FLOAT
#define REAL_IS_FLOAT

#ifdef REAL_IS_FLOAT
#  define real float
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#else
#  define real double
#endif

#define         FALSE                   0
#define         TRUE                    1

#define         SBLIMIT                 32
#define         SSLIMIT                 18

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define MAXFRAMESIZE 2880

/* AF: ADDED FOR LAYER1/LAYER2 */
#define         SCALE_BLOCK             12


/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)

struct frame {
    int     stereo;
    int     jsbound;
    int     single;          /* single channel (monophonic) */
    int     lsf;             /* 0 = MPEG-1, 1 = MPEG-2/2.5 */
    int     mpeg25;          /* 1 = MPEG-2.5, 0 = MPEG-1/2 */
    int     header_change;
    int     lay;             /* Layer */
    int     error_protection; /* 1 = CRC-16 code following header */
    int     bitrate_index;
    int     sampling_frequency; /* sample rate of decompressed audio in Hz */
    int     padding;
    int     extension;
    int     mode;
    int     mode_ext;
    int     copyright;
    int     original;
    int     emphasis;
    int     framesize;       /* computed framesize */

    /* AF: ADDED FOR LAYER1/LAYER2 */
    int     II_sblimit;
    struct al_table2 const *alloc;
    int     down_sample_sblimit;
    int     down_sample;


};

struct gr_info_s {
    int     scfsi;
    unsigned part2_3_length;
    unsigned big_values;
    unsigned scalefac_compress;
    unsigned block_type;
    unsigned mixed_block_flag;
    unsigned table_select[3];
    unsigned subblock_gain[3];
    unsigned maxband[3];
    unsigned maxbandl;
    unsigned maxb;
    unsigned region1start;
    unsigned region2start;
    unsigned preflag;
    unsigned scalefac_scale;
    unsigned count1table_select;
    real   *full_gain[3];
    real   *pow2gain;
};

struct III_sideinfo {
    unsigned main_data_begin;
    unsigned private_bits;
    struct {
        struct gr_info_s gr[2];
    } ch[2];
};


#endif
