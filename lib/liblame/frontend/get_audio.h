/*
 *	Get Audio routines include file
 *
 *	Copyright (c) 1999 Albert L Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef LAME_GET_AUDIO_H
#define LAME_GET_AUDIO_H
#include "lame.h"
#include <stdio.h>

typedef enum sound_file_format_e {
    sf_unknown,
    sf_raw,
    sf_wave,
    sf_aiff,
    sf_mp1,                  /* MPEG Layer 1, aka mpg */
    sf_mp2,                  /* MPEG Layer 2 */
    sf_mp3,                  /* MPEG Layer 3 */
    sf_mp123,                /* MPEG Layer 1,2 or 3; whatever .mp3, .mp2, .mp1 or .mpg contains */
    sf_ogg
} sound_file_format;


int     is_mpeg_file_format( int input_format );

FILE   *init_outfile(char *outPath, int decode);
void    init_infile(lame_global_flags *, char *inPath, int *enc_delay, int *enc_padding);
void    close_infile(void);
int     get_audio(lame_global_flags * const gfp, int buffer[2][1152]);
int     get_audio16(lame_global_flags * const gfp, short buffer[2][1152]);
int     WriteWaveHeader(FILE * const fp, const int pcmbytes,
                        const int freq, const int channels, const int bits);



/* the simple lame decoder */
/* After calling lame_init(), lame_init_params() and
 * init_infile(), call this routine to read the input MP3 file 
 * and output .wav data to the specified file pointer
 * lame_decoder will ignore the first 528 samples, since these samples
 * represent the mpglib decoding delay (and are all 0).  
 *skip = number of additional
 * samples to skip, to (for example) compensate for the encoder delay,
 * only used when decoding mp3 
*/
int     lame_decoder(lame_global_flags * gfp, FILE * outf, int skip, char *inPath, char *outPath,
                     int *enc_delay, int *enc_padding);



void    SwapBytesInWords(short *loc, int words);



#ifdef LIBSNDFILE

#include "sndfile.h"


#else
/*****************************************************************
 * LAME/ISO built in audio file I/O routines 
 *******************************************************************/
#include "portableio.h"


typedef struct blockAlign_struct {
    unsigned long offset;
    unsigned long blockSize;
} blockAlign;

typedef struct IFF_AIFF_struct {
    short   numChannels;
    unsigned long numSampleFrames;
    short   sampleSize;
    double  sampleRate;
    unsigned long sampleType;
    blockAlign blkAlgn;
} IFF_AIFF;

extern int aiff_read_headers(FILE *, IFF_AIFF *);
extern int aiff_seek_to_sound_data(FILE *);
extern int aiff_write_headers(FILE *, IFF_AIFF *);
extern int parse_wavheader(void);
extern int parse_aiff(const char fn[]);
extern void aiff_check(const char *, IFF_AIFF *, int *);



#endif /* ifdef LIBSNDFILE */
#endif /* ifndef LAME_GET_AUDIO_H */
