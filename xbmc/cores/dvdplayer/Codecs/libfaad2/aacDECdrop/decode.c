/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
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
** $Id: decode.c,v 1.16 2004/04/03 19:08:37 menno Exp $
** $Id: decode.c,v 1.16 2004/04/03 19:08:37 menno Exp $
**/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define off_t __int64
#else
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <neaacdec.h>
#include <mp4ff.h>

#include "audio.h"
#include "decode.h"
#include "misc.h"
#include "wave_out.h"

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#define MAX_CHANNELS 8 /* make this higher to support files with
                          more channels */

/* FAAD file buffering routines */
/* declare buffering variables */
#define DEC_BUFF_VARS \
    int fileread, bytesconsumed, k; \
    int buffercount = 0, buffer_index = 0; \
    unsigned char *buffer; \
    unsigned int bytes_in_buffer = 0;

/* initialise buffering */
#define INIT_BUFF(file) \
    fseek(file, 0, SEEK_END); \
    fileread = ftell(file); \
    fseek(file, 0, SEEK_SET); \
    buffer = (unsigned char*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNELS); \
    memset(buffer, 0, FAAD_MIN_STREAMSIZE*MAX_CHANNELS); \
    bytes_in_buffer = fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, file);

/* skip bytes in buffer */
#define UPDATE_BUFF_SKIP(bytes) \
    fseek(infile, bytes, SEEK_SET); \
    buffer_index += bytes; \
    buffercount = 0; \
    bytes_in_buffer = fread(buffer, 1, FAAD_MIN_STREAMSIZE*MAX_CHANNELS, infile);

/* update buffer */
#define UPDATE_BUFF_READ \
    if (bytesconsumed > 0) { \
        for (k = 0; k < (FAAD_MIN_STREAMSIZE*MAX_CHANNELS - bytesconsumed); k++) \
            buffer[k] = buffer[k + bytesconsumed]; \
        bytes_in_buffer += fread(buffer + (FAAD_MIN_STREAMSIZE*MAX_CHANNELS) - bytesconsumed, 1, bytesconsumed, infile); \
        bytesconsumed = 0; \
    }

/* update buffer indices after NeAACDecDecode */
#define UPDATE_BUFF_IDX(frame) \
    bytesconsumed += frame.bytesconsumed; \
    buffer_index += frame.bytesconsumed; \
    bytes_in_buffer -= frame.bytesconsumed;

/* true if decoding has to stop because of EOF */
#define IS_FILE_END buffer_index >= fileread

/* end buffering */
#define END_BUFF if (buffer) free(buffer);



/* globals */
char *progName;
extern int stop_decoding;

int id3v2_tag(unsigned char *buffer)
{
    if (strncmp(buffer, "ID3", 3) == 0) {
        unsigned long tagsize;

        /* high bit is not used */
        tagsize = (buffer[6] << 21) | (buffer[7] << 14) |
            (buffer[8] <<  7) | (buffer[9] <<  0);

        tagsize += 10;

        return tagsize;
    } else {
        return 0;
    }
}

char *file_ext[] =
{
    NULL,
    ".wav",
    ".aif",
    ".au",
    ".au",
    NULL
};

/* MicroSoft channel definitions */
#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

long aacChannelConfig2wavexChannelMask(NeAACDecFrameInfo *hInfo)
{
    if (hInfo->channels == 6 && hInfo->num_lfe_channels)
    {
        return SPEAKER_FRONT_LEFT + SPEAKER_FRONT_RIGHT +
            SPEAKER_FRONT_CENTER + SPEAKER_LOW_FREQUENCY +
            SPEAKER_BACK_LEFT + SPEAKER_BACK_RIGHT;
    } else {
        return 0;
    }
}

int decodeAACfile(char *sndfile, int def_srate, aac_dec_opt *opt)
{
    int tagsize;
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    FILE *infile;

    audio_file *aufile;

    NeAACDecHandle hDecoder;
    NeAACDecFrameInfo frameInfo;
    NeAACDecConfigurationPtr config;

    int first_time = 1;


    /* declare variables for buffering */
    DEC_BUFF_VARS

    infile = fopen(opt->filename, "rb");
    if (infile == NULL)
    {
        /* unable to open file */
        error_handler("Error opening file: %s\n", opt->filename);
        return 1;
    }
    INIT_BUFF(infile)

    tagsize = id3v2_tag(buffer);
    if (tagsize)
    {
        UPDATE_BUFF_SKIP(tagsize)
    }

    hDecoder = NeAACDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = NeAACDecGetCurrentConfiguration(hDecoder);
    if (def_srate)
        config->defSampleRate = def_srate;
    config->defObjectType = opt->object_type;
    config->outputFormat = opt->output_format;

    NeAACDecSetConfiguration(hDecoder, config);

    if ((bytesconsumed = NeAACDecInit(hDecoder, buffer, bytes_in_buffer,
        &samplerate, &channels)) < 0)
    {
        /* If some error initializing occured, skip the file */
        error_handler("Error initializing decoder library.\n");
        END_BUFF
        NeAACDecClose(hDecoder);
        fclose(infile);
        return 1;
    }
    buffer_index += bytesconsumed;

    do
    {
        /* update buffer */
        UPDATE_BUFF_READ

        sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, buffer, bytes_in_buffer);

        /* update buffer indices */
        UPDATE_BUFF_IDX(frameInfo)

        if (frameInfo.error > 0)
        {
            error_handler("Error: %s\n",
            NeAACDecGetErrorMessage(frameInfo.error));
        }

        opt->progress_update((long)fileread, buffer_index);

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            if(opt->decode_mode == 0)
            {
                if (Set_WIN_Params (INVALID_FILEDESC, samplerate, SAMPLE_SIZE,
                                frameInfo.channels) < 0)
                {
                    error_handler("\nCan't access %s\n", "WAVE OUT");
                    END_BUFF
                    NeAACDecClose(hDecoder);
                    fclose(infile);
                    return (0);
                }
            }
            else
            {
                aufile = open_audio_file(sndfile, samplerate, frameInfo.channels,
                    opt->output_format, opt->file_type, aacChannelConfig2wavexChannelMask(&frameInfo));

                if (aufile == NULL)
                {
                    END_BUFF
                    NeAACDecClose(hDecoder);
                    fclose(infile);
                    return 0;
                }
            }
            first_time = 0;
        }

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            if(opt->decode_mode == 0)
                WIN_Play_Samples((short*)sample_buffer, frameInfo.channels*frameInfo.samples);
            else
                write_audio_file(aufile, sample_buffer, frameInfo.samples, 0);
        }

        if (buffer_index >= fileread)
            sample_buffer = NULL; /* to make sure it stops now */

        if(stop_decoding)
            break;

    } while (sample_buffer != NULL);

    NeAACDecClose(hDecoder);

    fclose(infile);

    if(opt->decode_mode == 0)
        WIN_Audio_close();
    else
    {
        if (!first_time)
            close_audio_file(aufile);
    }

    END_BUFF

    return frameInfo.error;
}

int GetAACTrack(mp4ff_t *infile)
{
    /* find AAC track */
    int i, rc;
    int numTracks = mp4ff_total_tracks(infile);

    for (i = 0; i < numTracks; i++)
    {
        unsigned char *buff = NULL;
        int buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;

        mp4ff_get_decoder_config(infile, i, &buff, &buff_size);

        if (buff)
        {
            rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);

            if (rc < 0)
                continue;
            return i;
        }
    }

    /* can't decode this */
    return -1;
}

unsigned long srates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000,
    12000, 11025, 8000
};

uint32_t read_callback(void *user_data, void *buffer, uint32_t length)
{
    return fread(buffer, 1, length, (FILE*)user_data);
}

uint32_t seek_callback(void *user_data, uint64_t position)
{
    return fseek((FILE*)user_data, position, SEEK_SET);
}

int decodeMP4file(char *sndfile, aac_dec_opt *opt)
{
    int track;
    unsigned long samplerate;
    unsigned char channels;
    void *sample_buffer;

    mp4ff_t *infile;
    FILE *mp4File;
    int sampleId, numSamples;

    audio_file *aufile;

    NeAACDecHandle hDecoder;
    NeAACDecFrameInfo frameInfo;

    unsigned char *buffer;
    int buffer_size;

    int first_time = 1;

    /* initialise the callback structure */
    mp4ff_callback_t *mp4cb = malloc(sizeof(mp4ff_callback_t));

    mp4File = fopen(opt->filename, "rb");
    mp4cb->read = read_callback;
    mp4cb->seek = seek_callback;
    mp4cb->user_data = mp4File;

    infile = mp4ff_open_read(mp4cb);
    if (!infile)
    {
        /* unable to open file */
        error_handler("Error opening file: %s\n", opt->filename);
        return 1;
    }

    if ((track = GetAACTrack(infile)) < 0)
    {
        error_handler("Unable to find correct AAC sound track in the MP4 file.\n");
        mp4ff_close(infile);
        free(mp4cb);
        fclose(mp4File);
        return 1;
    }

    buffer = NULL;
    buffer_size = 0;
    mp4ff_get_decoder_config(infile, track, &buffer, &buffer_size);

    hDecoder = NeAACDecOpen();

    if(NeAACDecInit2(hDecoder, buffer, buffer_size, &samplerate, &channels) < 0)
    {
        /* If some error initializing occured, skip the file */
        error_handler("Error initializing decoder library.\n");
        NeAACDecClose(hDecoder);
        mp4ff_close(infile);
        free(mp4cb);
        fclose(mp4File);
        return 1;
    }
    if (buffer)
        free(buffer);

    numSamples = mp4ff_num_samples(infile, track);

    for (sampleId = 0; sampleId < numSamples; sampleId++)
    {
        int rc;

        /* get access unit from MP4 file */
        buffer = NULL;
        buffer_size = 0;

        rc = mp4ff_read_sample(infile, track, sampleId, &buffer, &buffer_size);
        if (rc == 0)
        {
            error_handler("Reading from MP4 file failed.\n");
            NeAACDecClose(hDecoder);
            mp4ff_close(infile);
            free(mp4cb);
            fclose(mp4File);
            return 1;
        }

        sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, buffer, buffer_size);

        if (buffer)
            free(buffer);

        opt->progress_update((long)numSamples, sampleId);

        /* open the sound file now that the number of channels are known */
        if (first_time && !frameInfo.error)
        {
            if(opt->decode_mode == 0)
            {
                if (Set_WIN_Params (INVALID_FILEDESC, samplerate, SAMPLE_SIZE,
                                frameInfo.channels) < 0)
                {
                    error_handler("\nCan't access %s\n", "WAVE OUT");
                    NeAACDecClose(hDecoder);
                    mp4ff_close(infile);
                    free(mp4cb);
                    fclose(mp4File);
                    return (0);
                }
            }
            else
            {
                aufile = open_audio_file(sndfile, samplerate, frameInfo.channels,
                     opt->output_format, opt->file_type, aacChannelConfig2wavexChannelMask(&frameInfo));

                if (aufile == NULL)
                {
                    NeAACDecClose(hDecoder);
                    mp4ff_close(infile);
                    free(mp4cb);
                    fclose(mp4File);
                    return 0;
                }
            }
            first_time = 0;
        }

        if ((frameInfo.error == 0) && (frameInfo.samples > 0))
        {
            if(opt->decode_mode == 0)
                WIN_Play_Samples((short*)sample_buffer, frameInfo.channels*frameInfo.samples);
            else
                write_audio_file(aufile, sample_buffer, frameInfo.samples, 0);
        }

        if (frameInfo.error > 0)
        {
            error_handler("Error: %s\n",
            NeAACDecGetErrorMessage(frameInfo.error));
            break;
        }
        if(stop_decoding)
            break;
    }


    NeAACDecClose(hDecoder);


    mp4ff_close(infile);
    free(mp4cb);
    fclose(mp4File);

    if(opt->decode_mode == 0)
        WIN_Audio_close();
    else
    {
        if (!first_time)
            close_audio_file(aufile);
    }

    return frameInfo.error;
}

int str_no_case_comp(char const *str1, char const *str2, unsigned long len)
{
    signed int c1 = 0, c2 = 0;

    while (len--)
    {
        c1 = tolower(*str1++);
        c2 = tolower(*str2++);

        if (c1 == 0 || c1 != c2)
            break;
    }

    return c1 - c2;
}

int aac_decode(aac_dec_opt *opt)
{
    int result;
    int def_srate = 0;
    int outfile_set = 0;
    int mp4file = 0;
    char *fnp;
    char audioFileName[MAX_PATH];
    unsigned char header[8];
    FILE *hMP4File;


    /* point to the specified file name */
    strcpy(audioFileName, opt->filename);
    fnp = (char *)strrchr(audioFileName,'.');
    if (fnp)
        fnp[0] = '\0';
    strcat(audioFileName, file_ext[opt->file_type]);

    mp4file = 0;
    hMP4File = fopen(opt->filename, "rb");
    if (!hMP4File)
    {
        return 1;
    }
    fread(header, 1, 8, hMP4File);
    fclose(hMP4File);
    if (header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p')
        mp4file = 1;

    if (mp4file)
    {
        result = decodeMP4file(audioFileName, opt);
    }
    else
    {
        result = decodeAACfile(audioFileName, def_srate, opt);
    }

    return 0;
}
