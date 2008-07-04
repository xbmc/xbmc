/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the actual decoder.
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
#include <string.h>
//#include <stdint.h>

#include "decomp.h"

#define _Swap32(v) do { \
                   v = (((v) & 0x000000FF) << 0x18) | \
                       (((v) & 0x0000FF00) << 0x08) | \
                       (((v) & 0x00FF0000) >> 0x08) | \
                       (((v) & 0xFF000000) >> 0x18); } while(0)

#define _Swap16(v) do { \
                   v = (((v) & 0x00FF) << 0x08) | \
                       (((v) & 0xFF00) >> 0x08); } while (0)


extern int host_bigendian;

struct alac_file
{
    unsigned char *input_buffer;
    int input_buffer_bitaccumulator; /* used so we can do arbitary
                                        bit reads */

    int samplesize;
    int numchannels;
    int bytespersample;


    /* buffers */
    int *predicterror_buffer_a;
    int *predicterror_buffer_b;

    int *outputsamples_buffer_a;
    int *outputsamples_buffer_b;


  /* stuff from setinfo */
  unsigned int setinfo_max_samples_per_frame; /* 0x1000 = 4096 */    /* max samples per frame? */
  unsigned char setinfo_7a; /* 0x00 */
  unsigned char setinfo_sample_size; /* 0x10 */
  unsigned char setinfo_rice_historymult; /* 0x28 */
  unsigned char setinfo_rice_initialhistory; /* 0x0a */
  unsigned char setinfo_rice_kmodifier; /* 0x0e */
  unsigned char setinfo_7f; /* 0x02 */
  unsigned short setinfo_80; /* 0x00ff */
  unsigned int setinfo_82; /* 0x000020e7 */
  unsigned int setinfo_86; /* 0x00069fe4 */
  unsigned int setinfo_8a_rate; /* 0x0000ac44 */
  /* end setinfo stuff */

};


static void allocate_buffers(alac_file *alac)
{
    alac->predicterror_buffer_a = malloc(alac->setinfo_max_samples_per_frame * 4);
    alac->predicterror_buffer_b = malloc(alac->setinfo_max_samples_per_frame * 4);

    alac->outputsamples_buffer_a = malloc(alac->setinfo_max_samples_per_frame * 4);
    alac->outputsamples_buffer_b = malloc(alac->setinfo_max_samples_per_frame * 4);
}

void alac_set_info(alac_file *alac, char *inputbuffer)
{
  char *ptr = inputbuffer;
  ptr += 4; /* size */
  ptr += 4; /* frma */
  ptr += 4; /* alac */
  ptr += 4; /* size */
  ptr += 4; /* alac */

  ptr += 4; /* 0 ? */

  alac->setinfo_max_samples_per_frame = *(unsigned int*)ptr; /* buffer size / 2 ? */
  if (!host_bigendian)
      _Swap32(alac->setinfo_max_samples_per_frame);
  ptr += 4;
  alac->setinfo_7a = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_sample_size = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_rice_historymult = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_rice_initialhistory = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_rice_kmodifier = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_7f = *(unsigned char*)ptr;
  ptr += 1;
  alac->setinfo_80 = *(unsigned short*)ptr;
  if (!host_bigendian)
      _Swap16(alac->setinfo_80);
  ptr += 2;
  alac->setinfo_82 = *(unsigned int*)ptr;
  if (!host_bigendian)
      _Swap32(alac->setinfo_82);
  ptr += 4;
  alac->setinfo_86 = *(unsigned int*)ptr;
  if (!host_bigendian)
      _Swap32(alac->setinfo_86);
  ptr += 4;
  alac->setinfo_8a_rate = *(unsigned int*)ptr;
  if (!host_bigendian)
      _Swap32(alac->setinfo_8a_rate);
  ptr += 4;

  allocate_buffers(alac);

}

/* stream reading */

/* supports reading 1 to 16 bits, in big endian format */
static unsigned int readbits_16(alac_file *alac, int bits)
{
    unsigned int result;
    int new_accumulator;

    result = (alac->input_buffer[0] << 16) |
             (alac->input_buffer[1] << 8) |
             (alac->input_buffer[2]);

    /* shift left by the number of bits we've already read,
     * so that the top 'n' bits of the 24 bits we read will
     * be the return bits */
    result = result << alac->input_buffer_bitaccumulator;

    result = result & 0x00ffffff;

    /* and then only want the top 'n' bits from that, where
     * n is 'bits' */
    result = result >> (24 - bits);

    new_accumulator = (alac->input_buffer_bitaccumulator + bits);

    /* increase the buffer pointer if we've read over n bytes. */
    alac->input_buffer += (new_accumulator >> 3);

    /* and the remainder goes back into the bit accumulator */
    alac->input_buffer_bitaccumulator = (new_accumulator & 7);

    return result;
}

/* supports reading 1 to 32 bits, in big endian format */
static unsigned int readbits(alac_file *alac, int bits)
{
    int result = 0;

    if (bits > 16)
    {
        bits -= 16;
        result = readbits_16(alac, 16) << bits;
    }

    result |= readbits_16(alac, bits);

    return result;
}

/* reads a single bit */
static int readbit(alac_file *alac)
{
    int result;
    int new_accumulator;

    result = alac->input_buffer[0];

    result = result << alac->input_buffer_bitaccumulator;

    result = result >> 7 & 1;

    new_accumulator = (alac->input_buffer_bitaccumulator + 1);

    alac->input_buffer += (new_accumulator / 8);

    alac->input_buffer_bitaccumulator = (new_accumulator % 8);

    return result;
}

static void unreadbits(alac_file *alac, int bits)
{
    int new_accumulator = (alac->input_buffer_bitaccumulator - bits);

    alac->input_buffer += (new_accumulator >> 3);

    alac->input_buffer_bitaccumulator = (new_accumulator & 7);
    if (alac->input_buffer_bitaccumulator < 0)
        alac->input_buffer_bitaccumulator *= -1;
}

/* hideously inefficient. could use a bitmask search,
 * alternatively bsr on x86,
 */
static int count_leading_zeros(int input)
{
    int i = 0;
    while (!(0x80000000 & input) && i < 32)
    {
        i++;
        input = input << 1;
    }
    return i;
}

void basterdised_rice_decompress(alac_file *alac,
                                 int *output_buffer,
                                 int output_size,
                                 int readsamplesize, /* arg_10 */
                                 int rice_initialhistory, /* arg424->b */
                                 int rice_kmodifier, /* arg424->d */
                                 int rice_historymult, /* arg424->c */
                                 int rice_kmodifier_mask /* arg424->e */
        )
{
    int output_count;
    unsigned int history = rice_initialhistory;
    int sign_modifier = 0;

    for (output_count = 0; output_count < output_size; output_count++)
    {
        int x = 0;
        int x_modified;
        int final_val;

        /* read x - number of 1s before 0 represent the rice */
        while (x <= 8 && readbit(alac))
        {
            x++;
        }


        if (x > 8) /* RICE THRESHOLD */
        { /* use alternative encoding */
            int value;

            value = readbits(alac, readsamplesize);

            /* mask value to readsamplesize size */
            if (readsamplesize != 32)
                value &= (0xffffffff >> (32 - readsamplesize));

            x = value;
        }
        else
        { /* standard rice encoding */
            int extrabits;
            int k; /* size of extra bits */

            /* read k, that is bits as is */
            k = 31 - rice_kmodifier - count_leading_zeros((history >> 9) + 3);

            if (k < 0) k += rice_kmodifier;
            else k = rice_kmodifier;

            if (k != 1)
            {
                extrabits = readbits(alac, k);

                /* multiply x by 2^k - 1, as part of their strange algorithm */
                x = (x << k) - x;

                if (extrabits > 1)
                {
                    x += extrabits - 1;
                }
                else unreadbits(alac, 1);
            }
        }

        x_modified = sign_modifier + x;
        final_val = (x_modified + 1) / 2;
        if (x_modified & 1) final_val *= -1;

        output_buffer[output_count] = final_val;

        sign_modifier = 0;

        /* now update the history */
        history += (x_modified * rice_historymult)
                 - ((history * rice_historymult) >> 9);

        if (x_modified > 0xffff)
            history = 0xffff;

        /* special case: there may be compressed blocks of 0 */
        if ((history < 128) && (output_count+1 < output_size))
        {
            int block_size;

            sign_modifier = 1;

            x = 0;
            while (x <= 8 && readbit(alac))
            {
                x++;
            }

            if (x > 8)
            {
                block_size = readbits(alac, 16);
                block_size &= 0xffff;
            }
            else
            {
                int k;
                int extrabits;

                k = count_leading_zeros(history) + ((history + 16) >> 6 /* / 64 */) - 24;

                extrabits = readbits(alac, k);

                block_size = (((1 << k) - 1) & rice_kmodifier_mask) * x
                           + extrabits - 1;

                if (extrabits < 2)
                {
                    x = 1 - extrabits;
                    block_size += x;
                    unreadbits(alac, 1);
                }
            }

            if (block_size > 0)
            {
                memset(&output_buffer[output_count+1], 0, block_size * 4);
                output_count += block_size;

            }

            if (block_size > 0xffff)
                sign_modifier = 0;

            history = 0;
        }
    }
}

#define SIGN_EXTENDED32(val, bits) ((val << (32 - bits)) >> (32 - bits))

#define SIGN_ONLY(v) \
                     ((v < 0) ? (-1) : \
                                ((v > 0) ? (1) : \
                                           (0)))

static void predictor_decompress_fir_adapt(int *error_buffer,
                                           int *buffer_out,
                                           int output_size,
                                           int readsamplesize,
                                           short *predictor_coef_table,
                                           int predictor_coef_num,
                                           int predictor_quantitization)
{
    int i;

    /* first sample always copies */
    *buffer_out = *error_buffer;

    if (!predictor_coef_num)
    {
        if (output_size <= 1) return;
        memcpy(buffer_out+1, error_buffer+1, (output_size-1) * 4);
        return;
    }

    if (predictor_coef_num == 0x1f) /* 11111 - max value of predictor_coef_num */
    { /* second-best case scenario for fir decompression,
       * error describes a small difference from the previous sample only
       */
        if (output_size <= 1) return;
        for (i = 0; i < output_size - 1; i++)
        {
            int prev_value;
            int error_value;

            prev_value = buffer_out[i];
            error_value = error_buffer[i+1];
            buffer_out[i+1] = SIGN_EXTENDED32((prev_value + error_value), readsamplesize);
        }
        return;
    }

    /* read warm-up samples */
    if (predictor_coef_num > 0)
    {
        int i;
        for (i = 0; i < predictor_coef_num; i++)
        {
            int val;

            val = buffer_out[i] + error_buffer[i+1];

            val = SIGN_EXTENDED32(val, readsamplesize);

            buffer_out[i+1] = val;
        }
    }

#if 0
    /* 4 and 8 are very common cases (the only ones i've seen). these
     * should be unrolled and optimised
     */
    if (predictor_coef_num == 4)
    {
        /* FIXME: optimised general case */
        return;
    }

    if (predictor_coef_table == 8)
    {
        /* FIXME: optimised general case */
        return;
    }
#endif


    /* general case */
    if (predictor_coef_num > 0)
    {
        for (i = predictor_coef_num + 1;
             i < output_size;
             i++)
        {
            int j;
            int sum = 0;
            int outval;
            int error_val = error_buffer[i];

            for (j = 0; j < predictor_coef_num; j++)
            {
                sum += (buffer_out[predictor_coef_num-j] - buffer_out[0]) *
                       predictor_coef_table[j];
            }

            outval = (1 << (predictor_quantitization-1)) + sum;
            outval = outval >> predictor_quantitization;
            outval = outval + buffer_out[0] + error_val;
            outval = SIGN_EXTENDED32(outval, readsamplesize);

            buffer_out[predictor_coef_num+1] = outval;

            if (error_val > 0)
            {
                int predictor_num = predictor_coef_num - 1;

                while (predictor_num >= 0 && error_val > 0)
                {
                    int val = buffer_out[0] - buffer_out[predictor_coef_num - predictor_num];
                    int sign = SIGN_ONLY(val);

                    predictor_coef_table[predictor_num] -= sign;

                    val *= sign; /* absolute value */

                    error_val -= ((val >> predictor_quantitization) *
                                  (predictor_coef_num - predictor_num));

                    predictor_num--;
                }
            }
            else if (error_val < 0)
            {
                int predictor_num = predictor_coef_num - 1;

                while (predictor_num >= 0 && error_val < 0)
                {
                    int val = buffer_out[0] - buffer_out[predictor_coef_num - predictor_num];
                    int sign = - SIGN_ONLY(val);

                    predictor_coef_table[predictor_num] -= sign;

                    val *= sign; /* neg value */

                    error_val -= ((val >> predictor_quantitization) *
                                  (predictor_coef_num - predictor_num));

                    predictor_num--;
                }
            }

            buffer_out++;
        }
    }
}

void deinterlace_16(int *buffer_a, int *buffer_b,
                    short *buffer_out,
                    int numchannels, int numsamples,
                    unsigned char interlacing_shift,
                    unsigned char interlacing_leftweight)
{
    int i;
    if (numsamples <= 0) return;

    /* weighted interlacing */
    if (interlacing_leftweight)
    {
        for (i = 0; i < numsamples; i++)
        {
            int difference, midright;
            short left;
            short right;

            midright = buffer_a[i];
            difference = buffer_b[i];


            right = midright - ((difference * interlacing_leftweight) >> interlacing_shift);
            left = (midright - ((difference * interlacing_leftweight) >> interlacing_shift))
                 + difference;

            /* output is always little endian */
            if (host_bigendian)
            {
                _Swap16(left);
                _Swap16(right);
            }

            buffer_out[i*numchannels] = left;
            buffer_out[i*numchannels + 1] = right;
        }

        return;
    }

    /* otherwise basic interlacing took place */
    for (i = 0; i < numsamples; i++)
    {
        short left, right;

        left = buffer_a[i];
        right = buffer_b[i];

        /* output is always little endian */
        if (host_bigendian)
        {
            _Swap16(left);
            _Swap16(right);
        }

        buffer_out[i*numchannels] = left;
        buffer_out[i*numchannels + 1] = right;
    }
}

void decode_frame(alac_file *alac,
                  unsigned char *inbuffer,
                  void *outbuffer, int *outputsize)
{
    int channels;
    int outputsamples = alac->setinfo_max_samples_per_frame;

    /* setup the stream */
    alac->input_buffer = inbuffer;
    alac->input_buffer_bitaccumulator = 0;

    channels = readbits(alac, 3);

    *outputsize = outputsamples * alac->bytespersample;

    switch(channels)
    {
    case 0: /* 1 channel */
    {
        int hassize;
        int isnotcompressed;
        int readsamplesize;

        int wasted_bytes;
        int ricemodifier;


        /* 2^result = something to do with output waiting.
         * perhaps matters if we read > 1 frame in a pass?
         */
        readbits(alac, 4);

        readbits(alac, 12); /* unknown, skip 12 bits */

        hassize = readbits(alac, 1); /* the output sample size is stored soon */

        wasted_bytes = readbits(alac, 2); /* unknown ? */

        isnotcompressed = readbits(alac, 1); /* whether the frame is compressed */

        if (hassize)
        {
            /* now read the number of samples,
             * as a 32bit integer */
            outputsamples = readbits(alac, 32);
            *outputsize = outputsamples * alac->bytespersample;
        }

        readsamplesize = alac->setinfo_sample_size - (wasted_bytes * 8);

        if (!isnotcompressed)
        { /* so it is compressed */
            short predictor_coef_table[32];
            int predictor_coef_num;
            int prediction_type;
            int prediction_quantitization;
            int i;

            /* skip 16 bits, not sure what they are. seem to be used in
             * two channel case */
            readbits(alac, 8);
            readbits(alac, 8);

            prediction_type = readbits(alac, 4);
            prediction_quantitization = readbits(alac, 4);

            ricemodifier = readbits(alac, 3);
            predictor_coef_num = readbits(alac, 5);

            /* read the predictor table */
            for (i = 0; i < predictor_coef_num; i++)
            {
                predictor_coef_table[i] = (short)readbits(alac, 16);
            }

            if (wasted_bytes)
            {
                /* these bytes seem to have something to do with
                 * > 2 channel files.
                 */
                fprintf(stderr, "FIXME: unimplemented, unhandling of wasted_bytes\n");
            }

            basterdised_rice_decompress(alac,
                                        alac->predicterror_buffer_a,
                                        outputsamples,
                                        readsamplesize,
                                        alac->setinfo_rice_initialhistory,
                                        alac->setinfo_rice_kmodifier,
                                        ricemodifier * alac->setinfo_rice_historymult / 4,
                                        (1 << alac->setinfo_rice_kmodifier) - 1);

            if (prediction_type == 0)
            { /* adaptive fir */
                predictor_decompress_fir_adapt(alac->predicterror_buffer_a,
                                               alac->outputsamples_buffer_a,
                                               outputsamples,
                                               readsamplesize,
                                               predictor_coef_table,
                                               predictor_coef_num,
                                               prediction_quantitization);
            }
            else
            {
                fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type);
                /* i think the only other prediction type (or perhaps this is just a
                 * boolean?) runs adaptive fir twice.. like:
                 * predictor_decompress_fir_adapt(predictor_error, tempout, ...)
                 * predictor_decompress_fir_adapt(predictor_error, outputsamples ...)
                 * little strange..
                 */
            }

        }
        else
        { /* not compressed, easy case */
            if (readsamplesize <= 16)
            {
                int i;
                for (i = 0; i < outputsamples; i++)
                {
                    int audiobits = readbits(alac, readsamplesize);

                    audiobits = SIGN_EXTENDED32(audiobits, readsamplesize);

                    alac->outputsamples_buffer_a[i] = audiobits;
                }
            }
            else
            {
                int i;
                for (i = 0; i < outputsamples; i++)
                {
                    int audiobits;

                    audiobits = readbits(alac, 16);
                    /* special case of sign extension..
                     * as we'll be ORing the low 16bits into this */
                    audiobits = audiobits << 16;
                    audiobits = audiobits >> (32 - readsamplesize);

                    audiobits |= readbits(alac, readsamplesize - 16);

                    alac->outputsamples_buffer_a[i] = audiobits;
                }
            }
            /* wasted_bytes = 0; // unused */
        }

        switch(alac->setinfo_sample_size)
        {
        case 16:
        {
            int i;
            for (i = 0; i < outputsamples; i++)
            {
                short sample = alac->outputsamples_buffer_a[i];
                if (host_bigendian)
                    _Swap16(sample);
                ((short*)outbuffer)[i * alac->numchannels] = sample;
            }
            break;
        }
        case 20:
        case 24:
        case 32:
            fprintf(stderr, "FIXME: unimplemented sample size %i\n", alac->setinfo_sample_size);
            break;
        default:
            break;
        }
        break;
    }
    case 1: /* 2 channels */
    {
        int hassize;
        int isnotcompressed;
        int readsamplesize;

        int wasted_bytes;

        unsigned char interlacing_shift;
        unsigned char interlacing_leftweight;

        /* 2^result = something to do with output waiting.
         * perhaps matters if we read > 1 frame in a pass?
         */
        readbits(alac, 4);

        readbits(alac, 12); /* unknown, skip 12 bits */

        hassize = readbits(alac, 1); /* the output sample size is stored soon */

        wasted_bytes = readbits(alac, 2); /* unknown ? */

        isnotcompressed = readbits(alac, 1); /* whether the frame is compressed */

        if (hassize)
        {
            /* now read the number of samples,
             * as a 32bit integer */
            outputsamples = readbits(alac, 32);
            *outputsize = outputsamples * alac->bytespersample;
        }

        readsamplesize = alac->setinfo_sample_size - (wasted_bytes * 8) + 1;

        if (!isnotcompressed)
        { /* compressed */
            short predictor_coef_table_a[32];
            int predictor_coef_num_a;
            int prediction_type_a;
            int prediction_quantitization_a;
            int ricemodifier_a;

            short predictor_coef_table_b[32];
            int predictor_coef_num_b;
            int prediction_type_b;
            int prediction_quantitization_b;
            int ricemodifier_b;

            int i;

            interlacing_shift = readbits(alac, 8);
            interlacing_leftweight = readbits(alac, 8);

            /******** channel 1 ***********/
            prediction_type_a = readbits(alac, 4);
            prediction_quantitization_a = readbits(alac, 4);

            ricemodifier_a = readbits(alac, 3);
            predictor_coef_num_a = readbits(alac, 5);

            /* read the predictor table */
            for (i = 0; i < predictor_coef_num_a; i++)
            {
                predictor_coef_table_a[i] = (short)readbits(alac, 16);
            }

            /******** channel 2 *********/
            prediction_type_b = readbits(alac, 4);
            prediction_quantitization_b = readbits(alac, 4);

            ricemodifier_b = readbits(alac, 3);
            predictor_coef_num_b = readbits(alac, 5);

            /* read the predictor table */
            for (i = 0; i < predictor_coef_num_b; i++)
            {
                predictor_coef_table_b[i] = (short)readbits(alac, 16);
            }

            /*********************/
            if (wasted_bytes)
            { /* see mono case */
                fprintf(stderr, "FIXME: unimplemented, unhandling of wasted_bytes\n");
            }

            /* channel 1 */
            basterdised_rice_decompress(alac,
                                        alac->predicterror_buffer_a,
                                        outputsamples,
                                        readsamplesize,
                                        alac->setinfo_rice_initialhistory,
                                        alac->setinfo_rice_kmodifier,
                                        ricemodifier_a * alac->setinfo_rice_historymult / 4,
                                        (1 << alac->setinfo_rice_kmodifier) - 1);

            if (prediction_type_a == 0)
            { /* adaptive fir */
                predictor_decompress_fir_adapt(alac->predicterror_buffer_a,
                                               alac->outputsamples_buffer_a,
                                               outputsamples,
                                               readsamplesize,
                                               predictor_coef_table_a,
                                               predictor_coef_num_a,
                                               prediction_quantitization_a);
            }
            else
            { /* see mono case */
                fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type_a);
            }

            /* channel 2 */
            basterdised_rice_decompress(alac,
                                        alac->predicterror_buffer_b,
                                        outputsamples,
                                        readsamplesize,
                                        alac->setinfo_rice_initialhistory,
                                        alac->setinfo_rice_kmodifier,
                                        ricemodifier_b * alac->setinfo_rice_historymult / 4,
                                        (1 << alac->setinfo_rice_kmodifier) - 1);

            if (prediction_type_b == 0)
            { /* adaptive fir */
                predictor_decompress_fir_adapt(alac->predicterror_buffer_b,
                                               alac->outputsamples_buffer_b,
                                               outputsamples,
                                               readsamplesize,
                                               predictor_coef_table_b,
                                               predictor_coef_num_b,
                                               prediction_quantitization_b);
            }
            else
            {
                fprintf(stderr, "FIXME: unhandled predicition type: %i\n", prediction_type_b);
            }
        }
        else
        { /* not compressed, easy case */
            if (alac->setinfo_sample_size <= 16)
            {
                int i;
                for (i = 0; i < outputsamples; i++)
                {
                    int audiobits_a, audiobits_b;

                    audiobits_a = readbits(alac, alac->setinfo_sample_size);
                    audiobits_b = readbits(alac, alac->setinfo_sample_size);

                    audiobits_a = SIGN_EXTENDED32(audiobits_a, alac->setinfo_sample_size);
                    audiobits_b = SIGN_EXTENDED32(audiobits_b, alac->setinfo_sample_size);

                    alac->outputsamples_buffer_a[i] = audiobits_a;
                    alac->outputsamples_buffer_b[i] = audiobits_b;
                }
            }
            else
            {
                int i;
                for (i = 0; i < outputsamples; i++)
                {
                    int audiobits_a, audiobits_b;

                    audiobits_a = readbits(alac, 16);
                    audiobits_a = audiobits_a << 16;
                    audiobits_a = audiobits_a >> (32 - alac->setinfo_sample_size);
                    audiobits_a |= readbits(alac, alac->setinfo_sample_size - 16);

                    audiobits_b = readbits(alac, 16);
                    audiobits_b = audiobits_b << 16;
                    audiobits_b = audiobits_b >> (32 - alac->setinfo_sample_size);
                    audiobits_b |= readbits(alac, alac->setinfo_sample_size - 16);

                    alac->outputsamples_buffer_a[i] = audiobits_a;
                    alac->outputsamples_buffer_b[i] = audiobits_b;
                }
            }
            /* wasted_bytes = 0; */
            interlacing_shift = 0;
            interlacing_leftweight = 0;
        }

        switch(alac->setinfo_sample_size)
        {
        case 16:
        {
            deinterlace_16(alac->outputsamples_buffer_a,
                           alac->outputsamples_buffer_b,
                           (short*)outbuffer,
                           alac->numchannels,
                           outputsamples,
                           interlacing_shift,
                           interlacing_leftweight);
            break;
        }
        case 20:
        case 24:
        case 32:
            fprintf(stderr, "FIXME: unimplemented sample size %i\n", alac->setinfo_sample_size);
            break;
        default:
            break;
        }

        break;
    }
    }
}

alac_file *create_alac(int samplesize, int numchannels)
{
    alac_file *newfile = malloc(sizeof(alac_file));

    newfile->samplesize = samplesize;
    newfile->numchannels = numchannels;
    newfile->bytespersample = (samplesize / 8) * numchannels;

    return newfile;
}

