/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is simply the glue for everything. It asks the demuxer to
 * parse the quicktime container, asks the decoder to decode
 * the data and writes output as either RAW PCM or WAV.
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


#include <ctype.h>
#include <stdio.h>
//#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "demux.h"
#include "decomp.h"
#include "stream.h"
#include "wavwriter.h"

int host_bigendian = 0;

alac_file *alac = NULL;

static FILE *input_file = NULL;
static int input_opened = 0;
static stream_t *input_stream;

static FILE *output_file = NULL;
static int output_opened = 0;

static int write_wav_format = 1;
static int verbose = 0;

static int get_sample_info(demux_res_t *demux_res, uint32_t samplenum,
                           uint32_t *sample_duration,
                           uint32_t *sample_byte_size)
{
    unsigned int duration_index_accum = 0;
    unsigned int duration_cur_index = 0;

    if (samplenum >= demux_res->num_sample_byte_sizes)
    {
        fprintf(stderr, "sample %i does not exist\n", samplenum);
        return 0;
    }

    if (!demux_res->num_time_to_samples)
    {
        fprintf(stderr, "no time to samples\n");
        return 0;
    }
    while ((demux_res->time_to_sample[duration_cur_index].sample_count + duration_index_accum)
            <= samplenum)
    {
        duration_index_accum += demux_res->time_to_sample[duration_cur_index].sample_count;
        duration_cur_index++;
        if (duration_cur_index >= demux_res->num_time_to_samples)
        {
            fprintf(stderr, "sample %i does not have a duration\n", samplenum);
            return 0;
        }
    }

    *sample_duration = demux_res->time_to_sample[duration_cur_index].sample_duration;
    *sample_byte_size = demux_res->sample_byte_size[samplenum];

    return 1;
}

static void GetBuffer(demux_res_t *demux_res)
{
    unsigned long destBufferSize = 1024*16; /* 16kb buffer = 4096 frames = 1 alac sample */
    void *pDestBuffer = malloc(destBufferSize);
    int bytes_read = 0;

    unsigned int buffer_size = 1024*64;
    void *buffer;

    unsigned int i;

    buffer = malloc(buffer_size);

    for (i = 0; i < demux_res->num_sample_byte_sizes; i++)
    {
        uint32_t sample_duration;
        uint32_t sample_byte_size;

        int outputBytes;

        /* just get one sample for now */
        if (!get_sample_info(demux_res, i,
                             &sample_duration, &sample_byte_size))
        {
            fprintf(stderr, "sample failed\n");
            return;
        }

        if (buffer_size < sample_byte_size)
        {
            fprintf(stderr, "sorry buffer too small! (is %i want %i)\n",
                    buffer_size,
                    sample_byte_size);
            return;
        }

        stream_read(input_stream, sample_byte_size,
                    buffer);

        /* now fetch */
        outputBytes = destBufferSize;
        decode_frame(alac, buffer, pDestBuffer, &outputBytes);

        /* write */
        bytes_read += outputBytes;

        if (verbose)
            fprintf(stderr, "read %i bytes. total: %i\n", outputBytes, bytes_read);

        fwrite(pDestBuffer, outputBytes, 1, output_file);
    }
    if (verbose)
        fprintf(stderr, "done reading, read %i frames\n", i);
}

static void init_sound_converter(demux_res_t *demux_res)
{
    alac = create_alac(demux_res->sample_size, demux_res->num_channels);

    alac_set_info(alac, demux_res->codecdata);
}

static void usage()
{
    fprintf(stderr, "Usage: alac [options] [--] file\n"
                    "Decompresses the ALAC file specified\n"
                    "\n"
                    "Options:\n"
                    "  -f output.wav     outputs the decompressed data to the\n"
                    "                    specified file, in WAV format. Default\n"
                    "                    is stdout.\n"
                    "  -r                write output as raw PCM data. Default\n"
                    "                    is in WAV format.\n"
                    "  -v                verbose output.\n"
                    "\n"
                    "This software is Copyright (c) 2005 David Hammerton\n"
                    "All rights reserved\n"
                    "http://crazney.net/\n");
    exit(1);
}

static void setup_environment(int argc, char **argv)
{
    int i = argc;

    char *input_file_n = NULL;
    char *output_file_n = NULL;

    int escaped = 0;

    if (argc < 2) usage();

    for (i = argc-1; i > 1; i--)
    {
        if (strcmp(argv[argc-i], "-f") == 0) /* output file */
        {
            if (!--i) usage();
            output_file_n = argv[argc-i];
        }
        else if (strcmp(argv[argc-i], "-r") == 0) /* raw PCM output */
        {
            write_wav_format = 0;
        }
        else if (strcmp(argv[argc-i], "-v") == 0) /* verbose */
        {
            verbose = 1;
        }
        else if (strcmp(argv[argc-i], "--") == 0) /* filename can begin with - */
        {
            /* must be 2nd last arg */
            if (i != 2) usage();
            escaped = 1;
        }
        else
            usage();
    }

    if (i != 1) usage();

    input_file_n = argv[argc-1];

    /* if the final parameter begins with '-', and isn't just "-",
     * it's probably an unsupported argument, print usage and exit.
     * (Unless it's been escaped with --)
     */
    if (!escaped && input_file_n[0] == '-' && input_file_n[1] != 0) usage();

    if (!input_file_n) usage();

    if (output_file_n)
    {
        output_file = fopen(output_file_n, "wb");
        if (!output_file)
        {
            fprintf(stderr, "failed to open output file '%s': ", output_file_n);
            perror(NULL);
            exit(1);
        }
        output_opened = 1;
    }
    else
    { /* defaults to stdout */
        output_file = stdout;
    }

    if (strcmp(input_file_n, "-") == 0)
    {
        input_file = stdin;
    }
    else
    {
        input_file = fopen(input_file_n, "rb");
        if (!input_file)
        {
            fprintf(stderr, "failed to open input file '%s': ", input_file_n);
            perror(NULL);
            exit(1);
        }
        input_opened = 1;
    }
}

/* this could quite easily be done at compile time,
 * however I don't want to have to bother with all the
 * various possible #define's for endianness, worrying about
 * different compilers etc. and I'm too lazy to use autoconf.
 */
void set_endian()
{
    uint32_t integer = 0x000000aa;
    unsigned char *p = (unsigned char*)&integer;

    if (p[0] == 0xaa) host_bigendian = 0;
    else host_bigendian = 1;
}

int main(int argc, char **argv)
{
    demux_res_t demux_res;
    unsigned int output_size, i;

    set_endian();

    setup_environment(argc, argv);

    input_stream = stream_create_file(input_file, 1);
    if (!input_stream)
    {
        fprintf(stderr, "failed to create input stream from file\n");
        return 0;
    }

    /* if qtmovie_read returns successfully, the stream is up to
     * the movie data, which can be used directly by the decoder */
    if (!qtmovie_read(input_stream, &demux_res))
    {
        fprintf(stderr, "failed to load the QuickTime movie headers\n");
        return 0;
    }

    /* initialise the sound converter */
    init_sound_converter(&demux_res);

    /* write wav output headers */
    if (write_wav_format)
    {
        /* calculate output size */
        output_size = 0;
        for (i = 0; i < demux_res.num_sample_byte_sizes; i++)
        {
            unsigned int thissample_duration;
            unsigned int thissample_bytesize;

            get_sample_info(&demux_res, i, &thissample_duration,
                            &thissample_bytesize);

            output_size += thissample_duration * (demux_res.sample_size / 8)
                           * demux_res.num_channels;
        }
        wavwriter_writeheaders(output_file,
                               output_size,
                               demux_res.num_channels,
                               demux_res.sample_rate,
                               demux_res.sample_size);
    }

    /* will convert the entire buffer */
    GetBuffer(&demux_res);

    stream_destroy(input_stream);

    if (output_opened)
        fclose(output_file);

    if (input_opened)
        fclose(input_file);

    return 0;
}


