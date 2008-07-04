/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * Very basic WAV file writer (just writes the header)
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ( \
    ( (int32_t)(char)(ch0) << 24 ) | \
    ( (int32_t)(char)(ch1) << 16 ) | \
    ( (int32_t)(char)(ch2) << 8 ) | \
    ( (int32_t)(char)(ch3) ) )
#endif

#define _Swap32(v) do { \
                   v = (((v) & 0x000000FF) << 0x18) | \
                       (((v) & 0x0000FF00) << 0x08) | \
                       (((v) & 0x00FF0000) >> 0x08) | \
                       (((v) & 0xFF000000) >> 0x18); } while(0)

#define _Swap16(v) do { \
                   v = (((v) & 0x00FF) << 0x08) | \
                       (((v) & 0xFF00) >> 0x08); } while (0)

extern int host_bigendian;

static void write_uint32(FILE *f, uint32_t v, int bigendian)
{
    if (bigendian ^ host_bigendian) _Swap32(v);
    fwrite(&v, 4, 1, f);
}

static void write_uint16(FILE *f, uint16_t v, int bigendian)
{
    if (bigendian ^ host_bigendian) _Swap16(v);
    fwrite(&v, 2, 1, f);
}

void wavwriter_writeheaders(FILE *f, int datasize,
                            int numchannels, int samplerate,
                            int bitspersample)
{
    /* write RIFF header */
    write_uint32(f, MAKEFOURCC('R','I','F','F'), 1);
    write_uint32(f, 36 + datasize, 0);
    write_uint32(f, MAKEFOURCC('W','A','V','E'), 1);

    /* write fmt header */
    write_uint32(f, MAKEFOURCC('f','m','t',' '), 1);
    write_uint32(f, 16, 0);
    write_uint16(f, 1, 0); /* PCM data */
    write_uint16(f, numchannels, 0);
    write_uint32(f, samplerate, 0);
    write_uint32(f, samplerate * numchannels * (bitspersample / 8), 0); /* byterate */
    write_uint16(f, numchannels * (bitspersample / 8), 0);
    write_uint16(f, bitspersample, 0);

    /* write data header */
    write_uint32(f, MAKEFOURCC('d','a','t','a'), 1);
    write_uint32(f, datasize, 0);
}


