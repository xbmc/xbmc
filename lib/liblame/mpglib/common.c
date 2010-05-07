/*
 * common.c: some common bitstream operations
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

/* $Id: common.c,v 1.32.8.4 2010/03/22 14:32:36 robert Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef macintosh
#include   <types.h>
#include   <stat.h>
#else
#include  <sys/types.h>
#include  <sys/stat.h>
#endif

#include "common.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

/* In C++ the array first must be prototyped, why ? */


    /* *INDENT-OFF* */
const int tabsel_123 [2] [3] [16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

const long freqs[9] = { 44100, 48000, 32000,
                        22050, 24000, 16000,
                        11025, 12000,  8000 };

    /* *INDENT-ON* */


real    muls[27][64];

#if 0
static void
get_II_stuff(struct frame *fr)
{
    /* *INDENT-OFF* */
  static const int translate [3] [2] [16] =   /* char ? */
   { { { 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 } ,
       { 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 } } ,
     { { 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 } ,
       { 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 } } ,
     { { 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 } ,
       { 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 } } };
    /* *INDENT-ON* */

    int     table, sblim;
    static const struct al_table2 *tables[5] = { alloc_0, alloc_1, alloc_2, alloc_3, alloc_4 };
    static int sblims[5] = { 27, 30, 8, 12, 30 };

    if (fr->lsf)
        table = 4;
    else
        table = translate[fr->sampling_frequency][2 - fr->stereo][fr->bitrate_index];
    sblim = sblims[table];

    fr->alloc = tables[table];
    fr->II_sblimit = sblim;
}
#endif

#define HDRCMPMASK 0xfffffd00

#define MAX_INPUT_FRAMESIZE 4096

int
head_check(unsigned long head, int check_layer)
{
    /*
       look for a valid header.  
       if check_layer > 0, then require that
       nLayer = check_layer.  
     */

    /* bits 13-14 = layer 3 */
    int     nLayer = 4 - ((head >> 17) & 3);

    if ((head & 0xffe00000) != 0xffe00000) {
        /* syncword */
        return FALSE;
    }

    if (nLayer == 4)
        return FALSE;

    if (check_layer > 0 && nLayer != check_layer)
        return FALSE;

    if (((head >> 12) & 0xf) == 0xf) {
        /* bits 16,17,18,19 = 1111  invalid bitrate */
        return FALSE;
    }
    if (((head >> 10) & 0x3) == 0x3) {
        /* bits 20,21 = 11  invalid sampling freq */
        return FALSE;
    }
    if ((head & 0x3) == 0x2)
        /* invalid emphasis */
        return FALSE;
    return TRUE;
}


/*
 * decode a header and write the information
 * into the frame structure
 */
int
decode_header(struct frame *fr, unsigned long newhead)
{


    if (newhead & (1 << 20)) {
        fr->lsf = (newhead & (1 << 19)) ? 0x0 : 0x1;
        fr->mpeg25 = 0;
    }
    else {
        fr->lsf = 1;
        fr->mpeg25 = 1;
    }


    fr->lay = 4 - ((newhead >> 17) & 3);
    if (((newhead >> 10) & 0x3) == 0x3) {
        fprintf(stderr, "Stream error\n");
        exit(1);
    }
    if (fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
    }
    else
        fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->lsf * 3);

    fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

    if (fr->mpeg25)     /* allow Bitrate change for 2.5 ... */
        fr->bitrate_index = ((newhead >> 12) & 0xf);

    fr->bitrate_index = ((newhead >> 12) & 0xf);
    fr->padding = ((newhead >> 9) & 0x1);
    fr->extension = ((newhead >> 8) & 0x1);
    fr->mode = ((newhead >> 6) & 0x3);
    fr->mode_ext = ((newhead >> 4) & 0x3);
    fr->copyright = ((newhead >> 3) & 0x1);
    fr->original = ((newhead >> 2) & 0x1);
    fr->emphasis = newhead & 0x3;

    fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    switch (fr->lay) {
    case 1:
        fr->framesize = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
        fr->down_sample = 0;
        fr->down_sample_sblimit = SBLIMIT >> (fr->down_sample);
        break;

    case 2:
        fr->framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        fr->framesize /= freqs[fr->sampling_frequency];
        fr->framesize += fr->padding - 4;
        fr->down_sample = 0;
        fr->down_sample_sblimit = SBLIMIT >> (fr->down_sample);
        break;

    case 3:
#if 0
        fr->do_layer = do_layer3;
        if (fr->lsf)
            ssize = (fr->stereo == 1) ? 9 : 17;
        else
            ssize = (fr->stereo == 1) ? 17 : 32;
#endif

#if 0
        if (fr->error_protection)
            ssize += 2;
#endif
        if (fr->framesize > MAX_INPUT_FRAMESIZE) {
            fprintf(stderr, "Frame size too big.\n");
            fr->framesize = MAX_INPUT_FRAMESIZE;
            return (0);
        }


        if (fr->bitrate_index == 0)
            fr->framesize = 0;
        else {
            fr->framesize = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
            fr->framesize /= freqs[fr->sampling_frequency] << (fr->lsf);
            fr->framesize = fr->framesize + fr->padding - 4;
        }
        break;
    default:
        fprintf(stderr, "Sorry, layer %d not supported\n", fr->lay);
        return (0);
    }
    /*    print_header(fr); */

    return 1;
}


#if 1
void
print_header(struct frame *fr)
{
    static const char *modes[4] = { "Stereo", "Joint-Stereo", "Dual-Channel", "Single-Channel" };
    static const char *layers[4] = { "Unknown", "I", "II", "III" };

    fprintf(stderr, "MPEG %s, Layer: %s, Freq: %ld, mode: %s, modext: %d, BPF : %d\n",
            fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
            layers[fr->lay], freqs[fr->sampling_frequency],
            modes[fr->mode], fr->mode_ext, fr->framesize + 4);
    fprintf(stderr, "Channels: %d, copyright: %s, original: %s, CRC: %s, emphasis: %d.\n",
            fr->stereo, fr->copyright ? "Yes" : "No",
            fr->original ? "Yes" : "No", fr->error_protection ? "Yes" : "No", fr->emphasis);
    fprintf(stderr, "Bitrate: %d Kbits/s, Extension value: %d\n",
            tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index], fr->extension);
}

void
print_header_compact(struct frame *fr)
{
    static const char *modes[4] = { "stereo", "joint-stereo", "dual-channel", "mono" };
    static const char *layers[4] = { "Unknown", "I", "II", "III" };

    fprintf(stderr, "MPEG %s layer %s, %d kbit/s, %ld Hz %s\n",
            fr->mpeg25 ? "2.5" : (fr->lsf ? "2.0" : "1.0"),
            layers[fr->lay],
            tabsel_123[fr->lsf][fr->lay - 1][fr->bitrate_index],
            freqs[fr->sampling_frequency], modes[fr->mode]);
}

#endif

unsigned int
getbits(PMPSTR mp, int number_of_bits)
{
    unsigned long rval;

    if (number_of_bits <= 0 || !mp->wordpointer)
        return 0;

    {
        rval = mp->wordpointer[0];
        rval <<= 8;
        rval |= mp->wordpointer[1];
        rval <<= 8;
        rval |= mp->wordpointer[2];
        rval <<= mp->bitindex;
        rval &= 0xffffff;

        mp->bitindex += number_of_bits;

        rval >>= (24 - number_of_bits);

        mp->wordpointer += (mp->bitindex >> 3);
        mp->bitindex &= 7;
    }
    return rval;
}

unsigned int
getbits_fast(PMPSTR mp, int number_of_bits)
{
    unsigned long rval;

    {
        rval = mp->wordpointer[0];
        rval <<= 8;
        rval |= mp->wordpointer[1];
        rval <<= mp->bitindex;
        rval &= 0xffff;
        mp->bitindex += number_of_bits;

        rval >>= (16 - number_of_bits);

        mp->wordpointer += (mp->bitindex >> 3);
        mp->bitindex &= 7;
    }
    return rval;
}


int
set_pointer(PMPSTR mp, long backstep)
{
    unsigned char *bsbufold;

    if (mp->fsizeold < 0 && backstep > 0) {
        fprintf(stderr, "hip: Can't step back %ld bytes!\n", backstep);
        return MP3_ERR;
    }
    bsbufold = mp->bsspace[1 - mp->bsnum] + 512;
    mp->wordpointer -= backstep;
    if (backstep)
        memcpy(mp->wordpointer, bsbufold + mp->fsizeold - backstep, (size_t) backstep);
    mp->bitindex = 0;
    return MP3_OK;
}
