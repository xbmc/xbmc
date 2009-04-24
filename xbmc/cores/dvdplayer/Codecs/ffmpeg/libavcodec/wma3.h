/*
 * WMA 9/3/PRO compatible decoder
 * Copyright (c) 2007 Baptiste Coudurier, Benjamin Larsson, Ulion
 * Copyright (c) 2008 - 2009 Sascha Sommer
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file  libavcodec/wma3.h
 * @brief wmapro specific structs and defines
 */

#ifndef AVCODEC_WMA3_H
#define AVCODEC_WMA3_H

#include "wma3data.h"
#include "dsputil.h"

/* current decoder limitations */
#define MAX_CHANNELS    8                                    //< max number of handled channels
#define MAX_SUBFRAMES  32                                    //< max number of subframes per channel
#define MAX_BANDS      29                                    //< max number of scale factor bands
#define MAX_FRAMESIZE  16384                                 //< maximum compressed frame size

/* size of block defines taken from wma.h */
#define BLOCK_MIN_BITS  7                                    //< log2 of min block size
#define BLOCK_MAX_BITS 12                                    //< log2 of max block size
#define BLOCK_MAX_SIZE (1 << BLOCK_MAX_BITS)                 //< maximum block size
#define BLOCK_NB_SIZES (BLOCK_MAX_BITS - BLOCK_MIN_BITS + 1) //< possible block sizes

/**
 * @brief decoder context for a single channel
 */
typedef struct {
    int      prev_block_len;                          //< length of the previous block
    uint8_t  transmit_coefs;                          //< transmit coefficients
    uint8_t  num_subframes;                           //< number of subframes
    uint16_t subframe_len[MAX_SUBFRAMES];             //< subframe length in samples
    uint16_t subframe_offset[MAX_SUBFRAMES];          //< subframe position
    uint8_t  cur_subframe;                            //< subframe index
    uint16_t channel_len;                             //< channel length in samples
    uint16_t decoded_samples;                         //< already processed samples
    uint8_t  grouped;                                 //< channel is part of a group
    int      quant_step_modifier;                     //< deviation from the main quantization step
    int      transmit_sf;                             //< transmit scale factors
    int      reuse_sf;                                //< share scale factors between subframes
    int      scale_factor_step;                       //< scaling step
    int      max_scale_factor;                        //< maximum scale factor
    int      scale_factors[MAX_BANDS];                //< scale factor values
    int      resampled_scale_factors[MAX_BANDS];      //< scale factors from a previous block
    int      scale_factor_block_len;                  //< scale factor reference block length
    float*   coeffs;                                  //< pointer to the decode buffer
    DECLARE_ALIGNED_16(float, out[2*BLOCK_MAX_SIZE]); //< output buffer
} WMA3ChannelCtx;

/**
 * @brief channel group for channel transformations
 */
typedef struct {
    int   num_channels;
    int   no_rotation;                                     //< controls the type of the transform
    int   transform;                                       //< also controls the type of the transform
    char  transform_band[MAX_BANDS];                       //< controls if the transform is enabled for a certain band
    char  rotation_offset[MAX_CHANNELS * MAX_CHANNELS];
    char  positive[MAX_CHANNELS * MAX_CHANNELS];           //< FIXME: What are these numbers used for?
    float decorrelation_matrix[MAX_CHANNELS*MAX_CHANNELS];
    char  use_channel[MAX_CHANNELS];
} WMA3ChannelGroup;

/**
 * @brief main decoder context
 */
typedef struct WMA3DecodeContext {
    /** generic decoder variables */
    AVCodecContext*  avctx;                         //< codec context for av_log
    DSPContext       dsp;                           //< accelerated dsp functions
    uint8_t          frame_data[MAX_FRAMESIZE +
                      FF_INPUT_BUFFER_PADDING_SIZE];//< compressed frame data
    MDCTContext      mdct_ctx[BLOCK_NB_SIZES];      //< MDCT context per block size
    DECLARE_ALIGNED_16(float, tmp[BLOCK_MAX_SIZE]); //< imdct output buffer
    float*           windows[BLOCK_NB_SIZES];       //< window per block size
    VLC              sf_vlc;                        //< scale factor dpcm vlc
    VLC              sf_rl_vlc;                     //< scale factor run length vlc
    VLC              vec4_vlc;                      //< 4 coefficients per symbol
    VLC              vec2_vlc;                      //< 2 coefficients per symbol
    VLC              vec1_vlc;                      //< 1 coefficient per symbol
    VLC              coef_vlc[2];                   //< coefficient run length vlc codes
    int              coef_max[2];                   //< max length of vlc codes

    /** frame size dependent frame information (set during initialization) */
    uint8_t          lossless;                      //< lossless mode
    unsigned int     decode_flags;                  //< used compression features
    uint8_t          len_prefix;                    //< frame is prefixed with its length
    uint8_t          dynamic_range_compression;     //< frame contains DRC data
    uint8_t          sample_bit_depth;              //< bits per sample
    uint16_t         samples_per_frame;             //< number of samples to output
    uint16_t         log2_frame_size;               //< frame size
    int8_t           num_channels;                  //< number of channels
    int8_t           lfe_channel;                   //< lfe channel index
    const float***   def_decorrelation_mat;         //< default decorrelation matrix
    uint8_t          allow_subframes;               //< frames may contain subframes
    uint8_t          max_num_subframes;             //< maximum number of subframes
    int8_t           num_possible_block_sizes;      //< nb of supported block sizes
    uint16_t         min_samples_per_subframe;      //< minimum samples per subframe
    int*             num_sfb;                       //< scale factor bands per block size
    int*             sfb_offsets;                   //< scale factor band offsets
    int*             sf_offsets;                    //< scale factor resample matrix
    int*             subwoofer_cutoffs;             //< subwoofer cutoff values

    /** packet decode state */
    uint8_t          packet_sequence_number;        //< current packet number
    int              prev_packet_bit_size;          //< saved number of bits
    uint8_t          bit5;                          //< padding bit? (CBR files)
    uint8_t          bit6;                          //< unknown
    uint8_t          packet_loss;                   //< set in case of bitstream error
    uint8_t          negative_quantstep;            //< packet loss due to negative quant step

    /** frame decode state */
    unsigned int     frame_num;                     //< current frame number
    GetBitContext    getbit;                        //< bitstream reader context
    int              buf_bit_size;                  //< buffer size in bits
    int16_t*         samples;                       //< current samplebuffer pointer
    int16_t*         samples_end;                   //< maximum samplebuffer pointer
    uint8_t          drc_gain;                      //< gain for the DRC tool
    int              skip_frame;                    //< skip output step
    int              parsed_all_subframes;          //< all subframes decoded?

    /** subframe/block decode state */
    int              subframe_len;                  //< current subframe length
    int              channels_for_cur_subframe;     //< number of channels that contain the subframe
    int              channel_indexes_for_cur_subframe[MAX_CHANNELS];
    int              cur_subwoofer_cutoff;          //< subwoofer cutoff value
    int              num_bands;                     //< number of scale factor bands
    int*             cur_sfb_offsets;               //< sfb offsets for the current block
    int              quant_step;                    //< quantization step
    int              esc_len;                       //< length of escaped coefficients

    uint8_t          num_chgroups;                  //< number of channel groups
    WMA3ChannelGroup chgroup[MAX_CHANNELS];         //< channel group information

    WMA3ChannelCtx   channel[MAX_CHANNELS];         //< per channel data
} WMA3DecodeContext;

#endif /* AVCODEC_WMA3_H */

