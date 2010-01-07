/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: audio.c,v 1.11 2004/02/06 10:23:27 menno Exp $
**/

#ifdef _WIN32
#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <faad.h>
#include "audio.h"


audio_file *open_audio_file(char *infile, int samplerate, int channels,
                            int outputFormat, int fileType, long channelMask)
{
    audio_file *aufile = malloc(sizeof(audio_file));

    aufile->outputFormat = outputFormat;

    aufile->samplerate = samplerate;
    aufile->channels = channels;
    aufile->total_samples = 0;
    aufile->fileType = fileType;
    aufile->channelMask = channelMask;

    switch (outputFormat)
    {
    case FAAD_FMT_16BIT:
        aufile->bits_per_sample = 16;
        break;
    case FAAD_FMT_24BIT:
        aufile->bits_per_sample = 24;
        break;
    case FAAD_FMT_32BIT:
    case FAAD_FMT_FLOAT:
        aufile->bits_per_sample = 32;
        break;
    default:
        if (aufile) free(aufile);
        return NULL;
    }

    if(infile[0] == '-')
    {
#ifdef _WIN32
        setmode(fileno(stdout), O_BINARY);
#endif
        aufile->sndfile = stdout;
    } else {
        aufile->sndfile = fopen(infile, "wb");
    }

    if (aufile->sndfile == NULL)
    {
        if (aufile) free(aufile);
        return NULL;
    }

    if (aufile->fileType == OUTPUT_WAV)
    {
        if (aufile->channelMask)
            write_wav_extensible_header(aufile, aufile->channelMask);
        else
            write_wav_header(aufile);
    }

    return aufile;
}

int write_audio_file(audio_file *aufile, void *sample_buffer, int samples, int offset)
{
    char *buf = (char *)sample_buffer;
    switch (aufile->outputFormat)
    {
    case FAAD_FMT_16BIT:
        return write_audio_16bit(aufile, buf + offset*2, samples);
    case FAAD_FMT_24BIT:
        return write_audio_24bit(aufile, buf + offset*4, samples);
    case FAAD_FMT_32BIT:
        return write_audio_32bit(aufile, buf + offset*4, samples);
    case FAAD_FMT_FLOAT:
        return write_audio_float(aufile, buf + offset*4, samples);
    default:
        return 0;
    }

    return 0;
}

void close_audio_file(audio_file *aufile)
{
    if (aufile->fileType == OUTPUT_WAV)
    {
        fseek(aufile->sndfile, 0, SEEK_SET);

        if (aufile->channelMask)
            write_wav_extensible_header(aufile, aufile->channelMask);
        else
            write_wav_header(aufile);
    }

    fclose(aufile->sndfile);

    if (aufile) free(aufile);
}

static int write_wav_header(audio_file *aufile)
{
    unsigned char header[44];
    unsigned char* p = header;
    unsigned int bytes = (aufile->bits_per_sample + 7) / 8;
    float data_size = (float)bytes * aufile->total_samples;
    unsigned long word32;

    *p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';

    word32 = (data_size + (44 - 8) < (float)MAXWAVESIZE) ?
        (unsigned long)data_size + (44 - 8)  :  (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    *p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';

    *p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';

    *p++ = 0x10; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;

    if (aufile->outputFormat == FAAD_FMT_FLOAT)
    {
        *p++ = 0x03; *p++ = 0x00;
    } else {
        *p++ = 0x01; *p++ = 0x00;
    }

    *p++ = (unsigned char)(aufile->channels >> 0);
    *p++ = (unsigned char)(aufile->channels >> 8);

    word32 = (unsigned long)(aufile->samplerate + 0.5);
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = aufile->samplerate * bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);

    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    *p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';

    word32 = data_size < MAXWAVESIZE ?
        (unsigned long)data_size : (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    return fwrite(header, sizeof(header), 1, aufile->sndfile);
}

static int write_wav_extensible_header(audio_file *aufile, long channelMask)
{
    unsigned char header[68];
    unsigned char* p = header;
    unsigned int bytes = (aufile->bits_per_sample + 7) / 8;
    float data_size = (float)bytes * aufile->total_samples;
    unsigned long word32;

    *p++ = 'R'; *p++ = 'I'; *p++ = 'F'; *p++ = 'F';

    word32 = (data_size + (68 - 8) < (float)MAXWAVESIZE) ?
        (unsigned long)data_size + (68 - 8)  :  (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    *p++ = 'W'; *p++ = 'A'; *p++ = 'V'; *p++ = 'E';

    *p++ = 'f'; *p++ = 'm'; *p++ = 't'; *p++ = ' ';

    *p++ = /*0x10*/0x28; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;

    /* WAVE_FORMAT_EXTENSIBLE */
    *p++ = 0xFE; *p++ = 0xFF;

    *p++ = (unsigned char)(aufile->channels >> 0);
    *p++ = (unsigned char)(aufile->channels >> 8);

    word32 = (unsigned long)(aufile->samplerate + 0.5);
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = aufile->samplerate * bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    word32 = bytes * aufile->channels;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);

    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    /* cbSize */
    *p++ = (unsigned char)(22);
    *p++ = (unsigned char)(0);

    /* WAVEFORMATEXTENSIBLE */

    /* wValidBitsPerSample */
    *p++ = (unsigned char)(aufile->bits_per_sample >> 0);
    *p++ = (unsigned char)(aufile->bits_per_sample >> 8);

    /* dwChannelMask */
    word32 = channelMask;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    /* SubFormat */
    if (aufile->outputFormat == FAAD_FMT_FLOAT)
    {
        /* KSDATAFORMAT_SUBTYPE_IEEE_FLOAT: 00000003-0000-0010-8000-00aa00389b71 */
        *p++ = 0x03;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00; *p++ = 0x00; *p++ = 0x10; *p++ = 0x00; *p++ = 0x80; *p++ = 0x00;
        *p++ = 0x00; *p++ = 0xaa; *p++ = 0x00; *p++ = 0x38; *p++ = 0x9b; *p++ = 0x71;
    } else {
        /* KSDATAFORMAT_SUBTYPE_PCM: 00000001-0000-0010-8000-00aa00389b71 */
        *p++ = 0x01;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00;
        *p++ = 0x00; *p++ = 0x00; *p++ = 0x10; *p++ = 0x00; *p++ = 0x80; *p++ = 0x00;
        *p++ = 0x00; *p++ = 0xaa; *p++ = 0x00; *p++ = 0x38; *p++ = 0x9b; *p++ = 0x71;
    }

    /* end WAVEFORMATEXTENSIBLE */

    *p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';

    word32 = data_size < MAXWAVESIZE ?
        (unsigned long)data_size : (unsigned long)MAXWAVESIZE;
    *p++ = (unsigned char)(word32 >>  0);
    *p++ = (unsigned char)(word32 >>  8);
    *p++ = (unsigned char)(word32 >> 16);
    *p++ = (unsigned char)(word32 >> 24);

    return fwrite(header, sizeof(header), 1, aufile->sndfile);
}

static int write_audio_16bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    short *sample_buffer16 = (short*)sample_buffer;
    char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            short r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer16[i];
            r2 = sample_buffer16[i+1];
            r3 = sample_buffer16[i+2];
            r4 = sample_buffer16[i+3];
            r5 = sample_buffer16[i+4];
            r6 = sample_buffer16[i+5];
            sample_buffer16[i] = r2;
            sample_buffer16[i+1] = r3;
            sample_buffer16[i+2] = r1;
            sample_buffer16[i+3] = r6;
            sample_buffer16[i+4] = r4;
            sample_buffer16[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        data[i*2] = (char)(sample_buffer16[i] & 0xFF);
        data[i*2+1] = (char)((sample_buffer16[i] >> 8) & 0xFF);
    }

    ret = fwrite(data, samples, aufile->bits_per_sample/8, aufile->sndfile);

    if (data) free(data);

    return ret;
}

static int write_audio_24bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    long *sample_buffer24 = (long*)sample_buffer;
    char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            long r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer24[i];
            r2 = sample_buffer24[i+1];
            r3 = sample_buffer24[i+2];
            r4 = sample_buffer24[i+3];
            r5 = sample_buffer24[i+4];
            r6 = sample_buffer24[i+5];
            sample_buffer24[i] = r2;
            sample_buffer24[i+1] = r3;
            sample_buffer24[i+2] = r1;
            sample_buffer24[i+3] = r6;
            sample_buffer24[i+4] = r4;
            sample_buffer24[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        data[i*3] = (char)(sample_buffer24[i] & 0xFF);
        data[i*3+1] = (char)((sample_buffer24[i] >> 8) & 0xFF);
        data[i*3+2] = (char)((sample_buffer24[i] >> 16) & 0xFF);
    }

    ret = fwrite(data, samples, aufile->bits_per_sample/8, aufile->sndfile);

    if (data) free(data);

    return ret;
}

static int write_audio_32bit(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    long *sample_buffer32 = (long*)sample_buffer;
    char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            long r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer32[i];
            r2 = sample_buffer32[i+1];
            r3 = sample_buffer32[i+2];
            r4 = sample_buffer32[i+3];
            r5 = sample_buffer32[i+4];
            r6 = sample_buffer32[i+5];
            sample_buffer32[i] = r2;
            sample_buffer32[i+1] = r3;
            sample_buffer32[i+2] = r1;
            sample_buffer32[i+3] = r6;
            sample_buffer32[i+4] = r4;
            sample_buffer32[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        data[i*4] = (char)(sample_buffer32[i] & 0xFF);
        data[i*4+1] = (char)((sample_buffer32[i] >> 8) & 0xFF);
        data[i*4+2] = (char)((sample_buffer32[i] >> 16) & 0xFF);
        data[i*4+3] = (char)((sample_buffer32[i] >> 24) & 0xFF);
    }

    ret = fwrite(data, samples, aufile->bits_per_sample/8, aufile->sndfile);

    if (data) free(data);

    return ret;
}

static int write_audio_float(audio_file *aufile, void *sample_buffer,
                             unsigned int samples)
{
    int ret;
    unsigned int i;
    float *sample_buffer_f = (float*)sample_buffer;
    unsigned char *data = malloc(samples*aufile->bits_per_sample*sizeof(char)/8);

    aufile->total_samples += samples;

    if (aufile->channels == 6 && aufile->channelMask)
    {
        for (i = 0; i < samples; i += aufile->channels)
        {
            float r1, r2, r3, r4, r5, r6;
            r1 = sample_buffer_f[i];
            r2 = sample_buffer_f[i+1];
            r3 = sample_buffer_f[i+2];
            r4 = sample_buffer_f[i+3];
            r5 = sample_buffer_f[i+4];
            r6 = sample_buffer_f[i+5];
            sample_buffer_f[i] = r2;
            sample_buffer_f[i+1] = r3;
            sample_buffer_f[i+2] = r1;
            sample_buffer_f[i+3] = r6;
            sample_buffer_f[i+4] = r4;
            sample_buffer_f[i+5] = r5;
        }
    }

    for (i = 0; i < samples; i++)
    {
        int exponent, mantissa, negative = 0 ;
        float in = sample_buffer_f[i];

        data[i*4] = 0; data[i*4+1] = 0; data[i*4+2] = 0; data[i*4+3] = 0;
        if (in == 0.0)
            continue;

        if (in < 0.0)
        {
            in *= -1.0;
            negative = 1;
        }
        in = (float)frexp(in, &exponent);
        exponent += 126;
        in *= (float)0x1000000;
        mantissa = (((int)in) & 0x7FFFFF);

        if (negative)
            data[i*4+3] |= 0x80;

        if (exponent & 0x01)
            data[i*4+2] |= 0x80;

        data[i*4] = mantissa & 0xFF;
        data[i*4+1] = (mantissa >> 8) & 0xFF;
        data[i*4+2] |= (mantissa >> 16) & 0x7F;
        data[i*4+3] |= (exponent >> 1) & 0x7F;
    }

    ret = fwrite(data, samples, aufile->bits_per_sample/8, aufile->sndfile);

    if (data) free(data);

    return ret;
}
