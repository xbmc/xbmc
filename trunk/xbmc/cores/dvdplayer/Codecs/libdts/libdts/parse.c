/*
 * parse.c
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 *
 * This file is part of dtsdec, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/dtsdec.html for updates.
 *
 * dtsdec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dtsdec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#endif

#include "dts.h"
#include "dts_internal.h"
#include "bitstream.h"

#include "tables.h"
#include "tables_huffman.h"
#include "tables_quantization.h"
#include "tables_adpcm.h"
#include "tables_fir.h"
#include "tables_vq.h"

/* #define DEBUG */

#if defined(HAVE_MEMALIGN) && !defined(__cplusplus)
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#else
/* assume malloc alignment is sufficient */
#define memalign(align,size) malloc (size)
#endif

static int decode_blockcode (int code, int levels, int *values);

static void qmf_32_subbands (dts_state_t * state, int chans,
                             double samples_in[32][8], sample_t *samples_out);

static void lfe_interpolation_fir (int nDecimationSelect, int nNumDeciSample,
                                   double *samples_in, sample_t *samples_out,
                                   sample_t bias);

static void pre_calc_cosmod( dts_state_t * state );

dts_state_t * dts_init (uint32_t mm_accel)
{
    dts_state_t * state;
    int i;

    state = (dts_state_t *) malloc (sizeof (dts_state_t));
    if (state == NULL)
        return NULL;

    memset (state, 0, sizeof(dts_state_t));

    state->samples = (sample_t *) memalign (16, 256 * 12 * sizeof (sample_t));
    if (state->samples == NULL) {
        free (state);
        return NULL;
    }

    for (i = 0; i < 256 * 12; i++)
        state->samples[i] = 0;

    /* Pre-calculate cosine modulation coefficients */
    pre_calc_cosmod( state );

    state->downmixed = 1;

    return state;
}

sample_t * dts_samples (dts_state_t * state)
{
    return state->samples;
}

int dts_blocks_num (dts_state_t * state)
{
    /* 8 samples per subsubframe and per subband */
    return state->sample_blocks / 8;
}

static int syncinfo (dts_state_t * state, int * flags,
                     int * sample_rate, int * bit_rate, int * frame_length)
{
    int frame_size;

    /* Sync code */
    bitstream_get (state, 32);
    /* Frame type */
    bitstream_get (state, 1);
    /* Samples deficit */
    bitstream_get (state, 5);
    /* CRC present */
    bitstream_get (state, 1);

    *frame_length = (bitstream_get (state, 7) + 1) * 32;
    if (*frame_length < 6 * 32) return 0;
    frame_size = bitstream_get (state, 14) + 1;
    if (frame_size < 96) return 0;
    if (!state->word_mode) frame_size = frame_size * 8 / 14 * 2;

    /* Audio channel arrangement */
    *flags = bitstream_get (state, 6);
    if (*flags > 63)
        return 0;

    *sample_rate = bitstream_get (state, 4);
    if (*sample_rate >= sizeof (dts_sample_rates) / sizeof (int))
        return 0;
    *sample_rate = dts_sample_rates[ *sample_rate ];
    if (!*sample_rate) return 0;

    *bit_rate = bitstream_get (state, 5);
    if (*bit_rate >= sizeof (dts_bit_rates) / sizeof (int))
        return 0;
    *bit_rate = dts_bit_rates[ *bit_rate ];
    if (!*bit_rate) return 0;

    /* LFE */
    bitstream_get (state, 10);
    if (bitstream_get (state, 2)) *flags |= DTS_LFE;

    return frame_size;
}

int dts_syncinfo (dts_state_t * state, uint8_t * buf, int * flags,
                  int * sample_rate, int * bit_rate, int * frame_length)
{
    /*
     * Look for sync code
     */

    /* 14 bits and little endian bitstream */
    if (buf[0] == 0xff && buf[1] == 0x1f &&
        buf[2] == 0x00 && buf[3] == 0xe8 &&
        (buf[4] & 0xf0) == 0xf0 && buf[5] == 0x07)
    {
        int frame_size;
        dts_bitstream_init (state, buf, 0, 0);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 14 bits and big endian bitstream */
    if (buf[0] == 0x1f && buf[1] == 0xff &&
        buf[2] == 0xe8 && buf[3] == 0x00 &&
        buf[4] == 0x07 && (buf[5] & 0xf0) == 0xf0)
    {
        int frame_size;
        dts_bitstream_init (state, buf, 0, 1);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 16 bits and little endian bitstream */
    if (buf[0] == 0xfe && buf[1] == 0x7f &&
        buf[2] == 0x01 && buf[3] == 0x80)
    {
        int frame_size;
        dts_bitstream_init (state, buf, 1, 0);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    /* 16 bits and big endian bitstream */
    if (buf[0] == 0x7f && buf[1] == 0xfe &&
        buf[2] == 0x80 && buf[3] == 0x01)
    {
        int frame_size;
        dts_bitstream_init (state, buf, 1, 1);
        frame_size = syncinfo (state, flags, sample_rate,
                               bit_rate, frame_length);
        return frame_size;
    }

    return 0;
}

int dts_frame (dts_state_t * state, uint8_t * buf, int * flags,
               level_t * level, sample_t bias)
{
    int i, j;
    static float adj_table[] = { 1.0, 1.1250, 1.2500, 1.4375 };

    dts_bitstream_init (state, buf, state->word_mode, state->bigendian_mode);

    /* Sync code */
    bitstream_get (state, 32);

    /* Frame header */
    state->frame_type = bitstream_get (state, 1);
    state->samples_deficit = bitstream_get (state, 5) + 1;
    state->crc_present = bitstream_get (state, 1);
    state->sample_blocks = bitstream_get (state, 7) + 1;
    state->frame_size = bitstream_get (state, 14) + 1;
    state->amode = bitstream_get (state, 6);
    state->sample_rate = bitstream_get (state, 4);
    state->bit_rate = bitstream_get (state, 5);

    state->downmix = bitstream_get (state, 1);
    state->dynrange = bitstream_get (state, 1);
    state->timestamp = bitstream_get (state, 1);
    state->aux_data = bitstream_get (state, 1);
    state->hdcd = bitstream_get (state, 1);
    state->ext_descr = bitstream_get (state, 3);
    state->ext_coding = bitstream_get (state, 1);
    state->aspf = bitstream_get (state, 1);
    state->lfe = bitstream_get (state, 2);
    state->predictor_history = bitstream_get (state, 1);

    /* TODO: check CRC */
    if (state->crc_present) state->header_crc = bitstream_get (state, 16);

    state->multirate_inter = bitstream_get (state, 1);
    state->version = bitstream_get (state, 4);
    state->copy_history = bitstream_get (state, 2);
    state->source_pcm_res = bitstream_get (state, 3);
    state->front_sum = bitstream_get (state, 1);
    state->surround_sum = bitstream_get (state, 1);
    state->dialog_norm = bitstream_get (state, 4);

    /* FIME: channels mixing levels */
    state->clev = state->slev = 1;
    state->output = dts_downmix_init (state->amode, *flags, level,
                                      state->clev, state->slev);
    if (state->output < 0)
        return 1;

    if (state->lfe && (*flags & DTS_LFE))
        state->output |= DTS_LFE;

    *flags = state->output;

    state->dynrng = state->level = MUL_C (*level, 2);
    state->bias = bias;
    state->dynrnge = 1;
    state->dynrngcall = NULL;

#ifdef DEBUG
    fprintf (stderr, "frame type: %i\n", state->frame_type);
    fprintf (stderr, "samples deficit: %i\n", state->samples_deficit);
    fprintf (stderr, "crc present: %i\n", state->crc_present);
    fprintf (stderr, "sample blocks: %i (%i samples)\n",
             state->sample_blocks, state->sample_blocks * 32);
    fprintf (stderr, "frame size: %i bytes\n", state->frame_size);
    fprintf (stderr, "amode: %i (%i channels)\n",
             state->amode, dts_channels[state->amode]);
    fprintf (stderr, "sample rate: %i (%i Hz)\n",
             state->sample_rate, dts_sample_rates[state->sample_rate]);
    fprintf (stderr, "bit rate: %i (%i bits/s)\n",
             state->bit_rate, dts_bit_rates[state->bit_rate]);
    fprintf (stderr, "downmix: %i\n", state->downmix);
    fprintf (stderr, "dynrange: %i\n", state->dynrange);
    fprintf (stderr, "timestamp: %i\n", state->timestamp);
    fprintf (stderr, "aux_data: %i\n", state->aux_data);
    fprintf (stderr, "hdcd: %i\n", state->hdcd);
    fprintf (stderr, "ext descr: %i\n", state->ext_descr);
    fprintf (stderr, "ext coding: %i\n", state->ext_coding);
    fprintf (stderr, "aspf: %i\n", state->aspf);
    fprintf (stderr, "lfe: %i\n", state->lfe);
    fprintf (stderr, "predictor history: %i\n", state->predictor_history);
    fprintf (stderr, "header crc: %i\n", state->header_crc);
    fprintf (stderr, "multirate inter: %i\n", state->multirate_inter);
    fprintf (stderr, "version number: %i\n", state->version);
    fprintf (stderr, "copy history: %i\n", state->copy_history);
    fprintf (stderr, "source pcm resolution: %i (%i bits/sample)\n",
             state->source_pcm_res,
             dts_bits_per_sample[state->source_pcm_res]);
    fprintf (stderr, "front sum: %i\n", state->front_sum);
    fprintf (stderr, "surround sum: %i\n", state->surround_sum);
    fprintf (stderr, "dialog norm: %i\n", state->dialog_norm);
    fprintf (stderr, "\n");
#endif

    /* Primary audio coding header */
    state->subframes = bitstream_get (state, 4) + 1;

    if (state->subframes > DTS_SUBFRAMES_MAX)
        state->subframes = DTS_SUBFRAMES_MAX;

    state->prim_channels = bitstream_get (state, 3) + 1;

    if (state->prim_channels > DTS_PRIM_CHANNELS_MAX)
        state->prim_channels = DTS_PRIM_CHANNELS_MAX;

#ifdef DEBUG
    fprintf (stderr, "subframes: %i\n", state->subframes);
    fprintf (stderr, "prim channels: %i\n", state->prim_channels);
#endif

    for (i = 0; i < state->prim_channels; i++)
    {
        state->subband_activity[i] = bitstream_get (state, 5) + 2;
#ifdef DEBUG
        fprintf (stderr, "subband activity: %i\n", state->subband_activity[i]);
#endif
        if (state->subband_activity[i] > DTS_SUBBANDS)
            state->subband_activity[i] = DTS_SUBBANDS;
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        state->vq_start_subband[i] = bitstream_get (state, 5) + 1;
#ifdef DEBUG
        fprintf (stderr, "vq start subband: %i\n", state->vq_start_subband[i]);
#endif
        if (state->vq_start_subband[i] > DTS_SUBBANDS)
            state->vq_start_subband[i] = DTS_SUBBANDS;
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        state->joint_intensity[i] = bitstream_get (state, 3);
#ifdef DEBUG
        fprintf (stderr, "joint intensity: %i\n", state->joint_intensity[i]);
        if (state->joint_intensity[i]) {fprintf (stderr, "JOINTINTENSITY\n");}
#endif
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        state->transient_huffman[i] = bitstream_get (state, 2);
#ifdef DEBUG
        fprintf (stderr, "transient mode codebook: %i\n",
                 state->transient_huffman[i]);
#endif
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        state->scalefactor_huffman[i] = bitstream_get (state, 3);
#ifdef DEBUG
        fprintf (stderr, "scale factor codebook: %i\n",
                 state->scalefactor_huffman[i]);
#endif
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        state->bitalloc_huffman[i] = bitstream_get (state, 3);
        /* There might be a way not to trash the whole frame, but for
         * now we must bail out or we will buffer overflow later. */
        if (state->bitalloc_huffman[i] == 7)
            return 1;
#ifdef DEBUG
        fprintf (stderr, "bit allocation quantizer: %i\n",
                 state->bitalloc_huffman[i]);
#endif
    }

    /* Get codebooks quantization indexes */
    for (i = 0; i < state->prim_channels; i++)
    {
        state->quant_index_huffman[i][0] = 0; /* Not transmitted */
        state->quant_index_huffman[i][1] = bitstream_get (state, 1);
    }
    for (j = 2; j < 6; j++)
        for (i = 0; i < state->prim_channels; i++)
            state->quant_index_huffman[i][j] = bitstream_get (state, 2);
    for (j = 6; j < 11; j++)
        for (i = 0; i < state->prim_channels; i++)
            state->quant_index_huffman[i][j] = bitstream_get (state, 3);
    for (j = 11; j < 27; j++)
        for (i = 0; i < state->prim_channels; i++)
            state->quant_index_huffman[i][j] = 0; /* Not transmitted */

#ifdef DEBUG
    for (i = 0; i < state->prim_channels; i++)
    {
        fprintf( stderr, "quant index huff:" );
        for (j = 0; j < 11; j++)
            fprintf (stderr, " %i", state->quant_index_huffman[i][j]);
        fprintf (stderr, "\n");
    }
#endif

    /* Get scale factor adjustment */
    for (j = 0; j < 11; j++)
    {
        for (i = 0; i < state->prim_channels; i++)
            state->scalefactor_adj[i][j] = 1;
    }
    for (i = 0; i < state->prim_channels; i++)
    {
        if (state->quant_index_huffman[i][1] == 0)
        {
            /* Transmitted only if quant_index_huffman=0 (Huffman code used) */
            state->scalefactor_adj[i][1] = adj_table[bitstream_get (state, 2)];
        }
    }
    for (j = 2; j < 6; j++)
        for (i = 0; i < state->prim_channels; i++)
            if (state->quant_index_huffman[i][j] < 3)
            {
                /* Transmitted only if quant_index_huffman < 3 */
                state->scalefactor_adj[i][j] =
                    adj_table[bitstream_get (state, 2)];
            }
    for (j = 6; j < 11; j++)
        for (i = 0; i < state->prim_channels; i++)
            if (state->quant_index_huffman[i][j] < 7)
            {
                /* Transmitted only if quant_index_huffman < 7 */
                state->scalefactor_adj[i][j] =
                    adj_table[bitstream_get (state, 2)];
            }

#ifdef DEBUG
    for (i = 0; i < state->prim_channels; i++)
    {
        fprintf (stderr, "scalefac adj:");
        for (j = 0; j < 11; j++)
            fprintf (stderr, " %1.3f", state->scalefactor_adj[i][j]);
        fprintf (stderr, "\n");
    }
#endif

    if (state->crc_present)
    {
        /* Audio header CRC check */
        bitstream_get (state, 16);
    }

    state->current_subframe = 0;
    state->current_subsubframe = 0;

    return 0;
}

int dts_subframe_header (dts_state_t * state)
{
    /* Primary audio coding side information */
    int j, k;

    /* Subsubframe count */
    state->subsubframes = bitstream_get (state, 2) + 1;
#ifdef DEBUG
    fprintf (stderr, "subsubframes: %i\n", state->subsubframes);
#endif

    /* Partial subsubframe sample count */
    state->partial_samples = bitstream_get (state, 3);
#ifdef DEBUG
    fprintf (stderr, "partial samples: %i\n", state->partial_samples);
#endif

    /* Get prediction mode for each subband */
    for (j = 0; j < state->prim_channels; j++)
    {
        for (k = 0; k < state->subband_activity[j]; k++)
            state->prediction_mode[j][k] = bitstream_get (state, 1);
#ifdef DEBUG
        fprintf (stderr, "prediction mode:");
        for (k = 0; k < state->subband_activity[j]; k++)
            fprintf (stderr, " %i", state->prediction_mode[j][k]);
        fprintf (stderr, "\n");
#endif
    }

    /* Get prediction codebook */
    for (j = 0; j < state->prim_channels; j++)
    {
        for (k = 0; k < state->subband_activity[j]; k++)
        {
            if (state->prediction_mode[j][k] > 0)
            {
                /* (Prediction coefficient VQ address) */
                state->prediction_vq[j][k] = bitstream_get (state, 12);
#ifdef DEBUG
                fprintf (stderr, "prediction coefs: %f, %f, %f, %f\n",
                         (double)adpcm_vb[state->prediction_vq[j][k]][0]/8192,
                         (double)adpcm_vb[state->prediction_vq[j][k]][1]/8192,
                         (double)adpcm_vb[state->prediction_vq[j][k]][2]/8192,
                         (double)adpcm_vb[state->prediction_vq[j][k]][3]/8192);
#endif
            }
        }
    }

    /* Bit allocation index */
    for (j = 0; j < state->prim_channels; j++)
    {
        for (k = 0; k < state->vq_start_subband[j]; k++)
        {
            if (state->bitalloc_huffman[j] == 6)
                state->bitalloc[j][k] = bitstream_get (state, 5);
            else if (state->bitalloc_huffman[j] == 5)
                state->bitalloc[j][k] = bitstream_get (state, 4);
            else
            {
                state->bitalloc[j][k] = InverseQ (state,
                    bitalloc_12[state->bitalloc_huffman[j]]);
            }

            if (state->bitalloc[j][k] > 26)
            {
                fprintf (stderr, "bitalloc index [%i][%i] too big (%i)\n",
                         j, k, state->bitalloc[j][k]);
                return -1;
            }
        }

#ifdef DEBUG
        fprintf (stderr, "bitalloc index: ");
        for (k = 0; k < state->vq_start_subband[j]; k++)
            fprintf (stderr, "%2.2i ", state->bitalloc[j][k]);
        fprintf (stderr, "\n");
#endif
    }

    /* Transition mode */
    for (j = 0; j < state->prim_channels; j++)
    {
        for (k = 0; k < state->subband_activity[j]; k++)
        {
            state->transition_mode[j][k] = 0;
            if (state->subsubframes > 1 &&
                k < state->vq_start_subband[j] &&
                state->bitalloc[j][k] > 0)
            {
                state->transition_mode[j][k] = InverseQ (state,
                    tmode[state->transient_huffman[j]]);
            }
        }
#ifdef DEBUG
        fprintf (stderr, "Transition mode:");
        for (k = 0; k < state->subband_activity[j]; k++)
            fprintf (stderr, " %i", state->transition_mode[j][k]);
        fprintf (stderr, "\n");
#endif
    }

    /* Scale factors */
    for (j = 0; j < state->prim_channels; j++)
    {
        int *scale_table;
        int scale_sum;

        for (k = 0; k < state->subband_activity[j]; k++)
        {
            state->scale_factor[j][k][0] = 0;
            state->scale_factor[j][k][1] = 0;
        }

        if (state->scalefactor_huffman[j] == 6)
            scale_table = scale_factor_quant7;
        else
            scale_table = scale_factor_quant6;

        /* When huffman coded, only the difference is encoded */
        scale_sum = 0;

        for (k = 0; k < state->subband_activity[j]; k++)
        {
            if (k >= state->vq_start_subband[j] || state->bitalloc[j][k] > 0)
            {
                if (state->scalefactor_huffman[j] < 5)
                {
                    /* huffman encoded */
                    scale_sum += InverseQ (state,
                        scales_129[state->scalefactor_huffman[j]]);
                }
                else if (state->scalefactor_huffman[j] == 5)
                {
                    scale_sum = bitstream_get (state, 6);
                }
                else if (state->scalefactor_huffman[j] == 6)
                {
                    scale_sum = bitstream_get (state, 7);
                }

                state->scale_factor[j][k][0] = scale_table[scale_sum];
            }

            if (k < state->vq_start_subband[j] && state->transition_mode[j][k])
            {
                /* Get second scale factor */
                if (state->scalefactor_huffman[j] < 5)
                {
                    /* huffman encoded */
                    scale_sum += InverseQ (state,
                        scales_129[state->scalefactor_huffman[j]]);
                }
                else if (state->scalefactor_huffman[j] == 5)
                {
                    scale_sum = bitstream_get (state, 6);
                }
                else if (state->scalefactor_huffman[j] == 6)
                {
                    scale_sum = bitstream_get (state, 7);
                }

                state->scale_factor[j][k][1] = scale_table[scale_sum];
            }
        }

#ifdef DEBUG
        fprintf (stderr, "Scale factor:");
        for (k = 0; k < state->subband_activity[j]; k++)
        {
            if (k >= state->vq_start_subband[j] || state->bitalloc[j][k] > 0)
                fprintf (stderr, " %i", state->scale_factor[j][k][0]);
            if (k < state->vq_start_subband[j] && state->transition_mode[j][k])
                fprintf (stderr, " %i(t)", state->scale_factor[j][k][1]);
        }
        fprintf (stderr, "\n");
#endif
    }

    /* Joint subband scale factor codebook select */
    for (j = 0; j < state->prim_channels; j++)
    {
        /* Transmitted only if joint subband coding enabled */
        if (state->joint_intensity[j] > 0)
            state->joint_huff[j] = bitstream_get (state, 3);
    }

    /* Scale factors for joint subband coding */
    for (j = 0; j < state->prim_channels; j++)
    {
        int source_channel;

        /* Transmitted only if joint subband coding enabled */
        if (state->joint_intensity[j] > 0)
        {
            int scale = 0;
            source_channel = state->joint_intensity[j] - 1;

            /* When huffman coded, only the difference is encoded
             * (is this valid as well for joint scales ???) */

            for (k = state->subband_activity[j];
                 k < state->subband_activity[source_channel]; k++)
            {
                if (state->joint_huff[j] < 5)
                {
                    /* huffman encoded */
                    scale = InverseQ (state,
                        scales_129[state->joint_huff[j]]);
                }
                else if (state->joint_huff[j] == 5)
                {
                    scale = bitstream_get (state, 6);
                }
                else if (state->joint_huff[j] == 6)
                {
                    scale = bitstream_get (state, 7);
                }

                scale += 64; /* bias */
                state->joint_scale_factor[j][k] = scale;/*joint_scale_table[scale];*/
            }

            if (!state->debug_flag & 0x02)
            {
                fprintf (stderr, "Joint stereo coding not supported\n");
                state->debug_flag |= 0x02;
            }

#ifdef DEBUG
            fprintf (stderr, "Joint scale factor index:\n");
            for (k = state->subband_activity[j];
                 k < state->subband_activity[source_channel]; k++)
                fprintf (stderr, " %i", state->joint_scale_factor[j][k]);
            fprintf (stderr, "\n");
#endif
        }
    }

    /* Stereo downmix coefficients */
    if (state->prim_channels > 2 && state->downmix)
    {
        for (j = 0; j < state->prim_channels; j++)
        {
            state->downmix_coef[j][0] = bitstream_get (state, 7);
            state->downmix_coef[j][1] = bitstream_get (state, 7);
        }
    }

    /* Dynamic range coefficient */
    if (state->dynrange) state->dynrange_coef = bitstream_get (state, 8);

    /* Side information CRC check word */
    if (state->crc_present)
    {
        bitstream_get (state, 16);
    }

    /*
     * Primary audio data arrays
     */

    /* VQ encoded high frequency subbands */
    for (j = 0; j < state->prim_channels; j++)
    {
        for (k = state->vq_start_subband[j];
             k < state->subband_activity[j]; k++)
        {
            /* 1 vector -> 32 samples */
            state->high_freq_vq[j][k] = bitstream_get (state, 10);

#ifdef DEBUG
            fprintf( stderr, "VQ index: %i\n", state->high_freq_vq[j][k] );
#endif
        }
    }

    /* Low frequency effect data */
    if (state->lfe)
    {
        /* LFE samples */
        int lfe_samples = 2 * state->lfe * state->subsubframes;
        double lfe_scale;

        for (j = lfe_samples; j < lfe_samples * 2; j++)
        {
            /* Signed 8 bits int */
            state->lfe_data[j] =
                (signed int)(signed char)bitstream_get (state, 8);
        }

        /* Scale factor index */
        state->lfe_scale_factor =
            scale_factor_quant7[bitstream_get (state, 8)];

        /* Quantization step size * scale factor */
        lfe_scale = 0.035 * state->lfe_scale_factor;

        for (j = lfe_samples; j < lfe_samples * 2; j++)
            state->lfe_data[j] *= lfe_scale;

#ifdef DEBUG
        fprintf (stderr, "LFE samples:\n");
        for (j = lfe_samples; j < lfe_samples * 2; j++)
            fprintf (stderr, " %f", state->lfe_data[j]);
        fprintf (stderr, "\n");
#endif

    }

    return 0;
}

int dts_subsubframe (dts_state_t * state)
{
    int k, l;
    int subsubframe = state->current_subsubframe;

    double *quant_step_table;

    /* FIXME */
    double subband_samples[DTS_PRIM_CHANNELS_MAX][DTS_SUBBANDS][8];

    /*
     * Audio data
     */

    /* Select quantization step size table */
    if (state->bit_rate == 0x1f) 
        quant_step_table = lossless_quant_d;
    else
        quant_step_table = lossy_quant_d;

    for (k = 0; k < state->prim_channels; k++)
    {
        for (l = 0; l < state->vq_start_subband[k] ; l++)
        {
            int m;

            /* Select the mid-tread linear quantizer */
            int abits = state->bitalloc[k][l];

            double quant_step_size = quant_step_table[abits];
            double rscale;

            /*
             * Determine quantization index code book and its type 
             */

            /* Select quantization index code book */
            int sel = state->quant_index_huffman[k][abits]; 

            /* Determine its type */
            int q_type = 1; /* (Assume Huffman type by default) */
            if (abits >= 11 || !bitalloc_select[abits][sel])
            {
                /* Not Huffman type */
                if (abits <= 7) q_type = 3; /* Block code */
                else q_type = 2; /* No further encoding */
            }

            if (abits == 0) q_type = 0; /* No bits allocated */

            /*
             * Extract bits from the bit stream 
             */
            switch (q_type)
            {
            case 0: /* No bits allocated */
                for (m=0; m<8; m++)
                    subband_samples[k][l][m] = 0;
                break;

            case 1: /* Huffman code */
                for (m=0; m<8; m++)
                    subband_samples[k][l][m] =
                        InverseQ (state, bitalloc_select[abits][sel]);
                break;

            case 2: /* No further encoding */
                for (m=0; m<8; m++)
                {
                    /* Extract (signed) quantization index */
                    int q_index = bitstream_get (state, abits - 3);
                    if( q_index & (1 << (abits - 4)) )
                    {
                        q_index = (1 << (abits - 3)) - q_index;
                        q_index = -q_index;
                    }
                    subband_samples[k][l][m] = q_index;
                }
                break;

            case 3: /* Block code */
                {
                    int block_code1, block_code2, size, levels;
                    int block[8];

                    switch (abits)
                    {
                    case 1:
                        size = 7;
                        levels = 3;
                        break;
                    case 2:
                        size = 10;
                        levels = 5;
                        break;
                    case 3:
                        size = 12;
                        levels = 7;
                        break;
                    case 4:
                        size = 13;
                        levels = 9;
                        break;
                    case 5:
                        size = 15;
                        levels = 13;
                        break;
                    case 6:
                        size = 17;
                        levels = 17;
                        break;
                    case 7:
                    default:
                        size = 19;
                        levels = 25;
                        break;
                    }

                    block_code1 = bitstream_get (state, size);
                    /* Should test return value */
                    decode_blockcode (block_code1, levels, block);
                    block_code2 = bitstream_get (state, size);
                    decode_blockcode (block_code2, levels, &block[4]);
                    for (m=0; m<8; m++)
                        subband_samples[k][l][m] = block[m];

                }
                break;

            default: /* Undefined */
                fprintf (stderr, "Unknown quantization index codebook");
                return -1;
            }

            /*
             * Account for quantization step and scale factor
             */

            /* Deal with transients */
            if (state->transition_mode[k][l] &&
                subsubframe >= state->transition_mode[k][l])
                rscale = quant_step_size * state->scale_factor[k][l][1];
            else
                rscale = quant_step_size * state->scale_factor[k][l][0];

            /* Adjustment */
            rscale *= state->scalefactor_adj[k][sel];
            for (m=0; m<8; m++) subband_samples[k][l][m] *= rscale;

            /*
             * Inverse ADPCM if in prediction mode
             */
            if (state->prediction_mode[k][l])
            {
                int n;
                for (m=0; m<8; m++)
                {
                    for (n=1; n<=4; n++)
                        if (m-n >= 0)
                            subband_samples[k][l][m] +=
                              (adpcm_vb[state->prediction_vq[k][l]][n-1] *
                                subband_samples[k][l][m-n]/8192);
                        else if (state->predictor_history)
                            subband_samples[k][l][m] +=
                              (adpcm_vb[state->prediction_vq[k][l]][n-1] *
                               state->subband_samples_hist[k][l][m-n+4]/8192);
                }
            }
        }

        /*
         * Decode VQ encoded high frequencies
         */
        for (l = state->vq_start_subband[k];
             l < state->subband_activity[k]; l++)
        {
            /* 1 vector -> 32 samples but we only need the 8 samples
             * for this subsubframe. */
            int m;

            if (!state->debug_flag & 0x01)
            {
                fprintf (stderr, "Stream with high frequencies VQ coding\n");
                state->debug_flag |= 0x01;
            }

            for (m=0; m<8; m++)
            {
                subband_samples[k][l][m] = 
                    high_freq_vq[state->high_freq_vq[k][l]][subsubframe*8+m]
                        * (double)state->scale_factor[k][l][0] / 16.0;
            }
        }
    }

    /* Check for DSYNC after subsubframe */
    if (state->aspf || subsubframe == state->subsubframes - 1)
    {
        if (0xFFFF == bitstream_get (state, 16)) /* 0xFFFF */
        {
#ifdef DEBUG
            fprintf( stderr, "Got subframe DSYNC\n" );
#endif
        }
        else
        {
            fprintf( stderr, "Didn't get subframe DSYNC\n" );
        }
    }

    /* Backup predictor history for adpcm */
    for (k = 0; k < state->prim_channels; k++)
    {
        for (l = 0; l < state->vq_start_subband[k] ; l++)
        {
            int m;
            for (m = 0; m < 4; m++)
                state->subband_samples_hist[k][l][m] =
                    subband_samples[k][l][4+m];
        }
    }

    /* 32 subbands QMF */
    for (k = 0; k < state->prim_channels; k++)
    {
        qmf_32_subbands (state, k, subband_samples[k], &state->samples[256*k]);
    }

    /* Down/Up mixing */
    if (state->prim_channels < dts_channels[state->output & DTS_CHANNEL_MASK])
    {
        dts_upmix (state->samples, state->amode, state->output);
    } else
    if (state->prim_channels > dts_channels[state->output & DTS_CHANNEL_MASK])
    {
        dts_downmix (state->samples, state->amode, state->output, state->bias,
                     state->clev, state->slev);
    } else if (state->bias)
    {
        for ( k = 0; k < 256*state->prim_channels; k++ )
            state->samples[k] += state->bias;
    }

    /* Generate LFE samples for this subsubframe FIXME!!! */
    if (state->output & DTS_LFE)
    {
        int lfe_samples = 2 * state->lfe * state->subsubframes;
        int i_channels = dts_channels[state->output & DTS_CHANNEL_MASK];

        lfe_interpolation_fir (state->lfe, 2 * state->lfe,
                               state->lfe_data + lfe_samples +
                               2 * state->lfe * subsubframe,
                               &state->samples[256*i_channels], state->bias);
        /* Outputs 20bits pcm samples */
    }

    return 0;
}

int dts_subframe_footer (dts_state_t * state)
{
    int aux_data_count = 0, i;
    int lfe_samples;

    /*
     * Unpack optional information
     */

    /* Time code stamp */
    if (state->timestamp) bitstream_get (state, 32);

    /* Auxiliary data byte count */
    if (state->aux_data) aux_data_count = bitstream_get (state, 6);

    /* Auxiliary data bytes */
    for(i = 0; i < aux_data_count; i++)
        bitstream_get (state, 8);

    /* Optional CRC check bytes */
    if (state->crc_present && (state->downmix || state->dynrange))
        bitstream_get (state, 16);

    /* Backup LFE samples history */
    lfe_samples = 2 * state->lfe * state->subsubframes;
    for (i = 0; i < lfe_samples; i++)
    {
        state->lfe_data[i] = state->lfe_data[i+lfe_samples];
    }

#ifdef DEBUG
    fprintf( stderr, "\n" );
#endif

    return 0;
}

int dts_block (dts_state_t * state)
{
    /* Sanity check */
    if (state->current_subframe >= state->subframes)
    {
        fprintf (stderr, "check failed: %i>%i",
                 state->current_subframe, state->subframes);
        return -1;
    }

    if (!state->current_subsubframe)
    {
#ifdef DEBUG
        fprintf (stderr, "DSYNC dts_subframe_header\n");
#endif
        /* Read subframe header */
        if (dts_subframe_header (state)) return -1;
    }

    /* Read subsubframe */
#ifdef DEBUG
    fprintf (stderr, "DSYNC dts_subsubframe\n");
#endif
    if (dts_subsubframe (state)) return -1;

    /* Update state */
    state->current_subsubframe++;
    if (state->current_subsubframe >= state->subsubframes)
    {
        state->current_subsubframe = 0;
        state->current_subframe++;
    }
    if (state->current_subframe >= state->subframes)
    {
#ifdef DEBUG
        fprintf(stderr, "DSYNC dts_subframe_footer\n");
#endif
        /* Read subframe footer */
        if (dts_subframe_footer (state)) return -1;
    }

    return 0;
}

/* Very compact version of the block code decoder that does not use table
 * look-up but is slightly slower */
int decode_blockcode( int code, int levels, int *values )
{ 
    int i;
    int offset = (levels - 1) >> 1;

    for (i = 0; i < 4; i++)
    {
        values[i] = (code % levels) - offset;
        code /= levels;
    }

    if (code == 0)
        return 1;
    else
    {
        fprintf (stderr, "ERROR: block code look-up failed\n");
        return 0;
    }
}

static void pre_calc_cosmod( dts_state_t * state )
{
    int i, j, k;

    for (j=0,k=0;k<16;k++)
        for (i=0;i<16;i++)
            state->cos_mod[j++] = cos((2*i+1)*(2*k+1)*M_PI/64);

    for (k=0;k<16;k++)
        for (i=0;i<16;i++)
            state->cos_mod[j++] = cos((i)*(2*k+1)*M_PI/32);

    for (k=0;k<16;k++)
        state->cos_mod[j++] = 0.25/(2*cos((2*k+1)*M_PI/128));

    for (k=0;k<16;k++)
        state->cos_mod[j++] = -0.25/(2.0*sin((2*k+1)*M_PI/128));
}

static void qmf_32_subbands (dts_state_t * state, int chans,
                             double samples_in[32][8], sample_t *samples_out)
{
    static const double scale = 1.4142135623730951 /* sqrt(2) */ * 32768.0;
    const double *prCoeff;
    int i, j, k;
    double raXin[32];

    double *subband_fir_hist = state->subband_fir_hist[chans];
    double *subband_fir_hist2 = state->subband_fir_noidea[chans];

    int nChIndex = 0, NumSubband = 32, nStart = 0, nEnd = 8, nSubIndex;

    /* Select filter */
    if (!state->multirate_inter) /* Non-perfect reconstruction */
        prCoeff = fir_32bands_nonperfect;
    else /* Perfect reconstruction */
        prCoeff = fir_32bands_perfect;

    /* Reconstructed channel sample index */
    for (nSubIndex=nStart; nSubIndex<nEnd; nSubIndex++)
    {
        double A[16], B[16], SUM[16], DIFF[16];

        /* Load in one sample from each subband */
        for (i=0; i<state->subband_activity[chans]; i++)
            raXin[i] = samples_in[i][nSubIndex];

        /* Clear inactive subbands */
        for (i=state->subband_activity[chans]; i<NumSubband; i++)
            raXin[i] = 0.0;

        /* Multiply by cosine modulation coefficients and
         * create temporary arrays SUM and DIFF */
        for (j=0,k=0;k<16;k++)
        {
            A[k] = 0.0;
            for (i=0;i<16;i++)
                A[k]+=(raXin[2*i]+raXin[2*i+1])*state->cos_mod[j++];
        }

        for (k=0;k<16;k++)
        {
            B[k] = 0.0;
            for (i=0;i<16;i++)
            {
                if(i>0) B[k]+=(raXin[2*i]+raXin[2*i-1])*state->cos_mod[j++];
                else B[k]+=(raXin[2*i])*state->cos_mod[j++];
            }
            SUM[k]=A[k]+B[k];
            DIFF[k]=A[k]-B[k];
        }

        /* Store history */
        for (k=0;k<16;k++)
            subband_fir_hist[k]=state->cos_mod[j++]*SUM[k];
        for (k=0;k<16;k++)
            subband_fir_hist[32-k-1]=state->cos_mod[j++]*DIFF[k];
        /* Multiply by filter coefficients */
        for (k=31,i=0;i<32;i++,k--)
            for (j=0;j<512;j+=64)
                subband_fir_hist2[i] += prCoeff[i+j]*
                   (subband_fir_hist[i+j] - subband_fir_hist[j+k]);
        for (k=31,i=0;i<32;i++,k--)
            for (j=0;j<512;j+=64)
                subband_fir_hist2[32+i] += prCoeff[32+i+j]*
                    (-subband_fir_hist[i+j] - subband_fir_hist[j+k]);

        /* Create 32 PCM output samples */
        for (i=0;i<32;i++)
            samples_out[nChIndex++] = subband_fir_hist2[i] / scale;

        /* Update working arrays */
        for (i=511;i>=32;i--)
            subband_fir_hist[i] = subband_fir_hist[i-32];
        for (i=0;i<NumSubband;i++)
            subband_fir_hist2[i] = subband_fir_hist2[i+32];
        for (i=0;i<NumSubband;i++)
            subband_fir_hist2[i+32] = 0.0;
    }
}

static void lfe_interpolation_fir (int nDecimationSelect, int nNumDeciSample,
                                   double *samples_in, sample_t *samples_out,
                                   sample_t bias)
{
    /* samples_in: An array holding decimated samples.
     *   Samples in current subframe starts from samples_in[0],
     *   while samples_in[-1], samples_in[-2], ..., stores samples
     *   from last subframe as history.
     *
     * samples_out: An array holding interpolated samples
     */

    static const double scale = 8388608.0;
    int nDeciFactor, k, J;
    double *prCoeff;

    int NumFIRCoef = 512; /* Number of FIR coefficients */
    int nInterpIndex = 0; /* Index to the interpolated samples */
    int nDeciIndex;

    /* Select decimation filter */
    if (nDecimationSelect==1)
    {
        /* 128 decimation */
        nDeciFactor = 128;
        /* Pointer to the FIR coefficients array */
        prCoeff = lfe_fir_128;
    } else {
        /* 64 decimation */
        nDeciFactor = 64;
        prCoeff = lfe_fir_64;
    }

    /* Interpolation */
    for (nDeciIndex=0; nDeciIndex<nNumDeciSample; nDeciIndex++)
    {
        /* One decimated sample generates nDeciFactor interpolated ones */
        for (k=0; k<nDeciFactor; k++)
        {
            /* Clear accumulation */
            double rTmp = 0.0;

            /* Accumulate */
            for (J=0; J<NumFIRCoef/nDeciFactor; J++)
                rTmp += samples_in[nDeciIndex-J]*prCoeff[k+J*nDeciFactor];

            /* Save interpolated samples */
            samples_out[nInterpIndex++] = rTmp / scale + bias;
        }
    }
}

void dts_dynrng (dts_state_t * state,
                 level_t (* call) (level_t, void *), void * data)
{
    state->dynrange = 0;
    if (call) {
        state->dynrange = 1;
        state->dynrngcall = call;
        state->dynrngdata = data;
    }
}

void dts_free (dts_state_t * state)
{
    free (state->samples);
    free (state);
}
