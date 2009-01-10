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

#ifndef WMA3_H
#define WMA3_H 1

#include "wma3data.h"
#include "dsputil.h"

#define MAX_CHANNELS 8               //< max number of handled channels
#define MAX_SUBFRAMES 32             //< max number of subframes per channel
#define MAX_BANDS     29             //< max number of scale factor bands
#define VLCBITS 9

#define EXPVLCBITS 8
#define EXPMAX ((19+EXPVLCBITS-1)/EXPVLCBITS)

/* size of blocks defines taken from wma.h*/
#define BLOCK_MIN_BITS 7
#define BLOCK_MAX_BITS 12
#define BLOCK_MAX_SIZE (1 << BLOCK_MAX_BITS)
#define BLOCK_NB_SIZES (BLOCK_MAX_BITS - BLOCK_MIN_BITS + 1)

/**
 *@brief decoder context for a single channel
 */
typedef struct {
    int prev_block_len;
    uint8_t transmit_coefs;
    uint8_t num_subframes;                //< number of subframes for the current channel
    uint16_t subframe_len[MAX_SUBFRAMES]; //< subframe len in samples
    uint16_t subframe_offset[MAX_SUBFRAMES];
    uint16_t channel_len;                 //< channel len in samples
    uint16_t decoded_samples;                   //< number of samples that have been processed already
    uint8_t  cur_subframe;
    uint8_t grouped; //< true if the channel is contained in a channel group

    float coeffs[4096]; //< MAX_COEF

    int   scale_factors[MAX_BANDS];     //< initial scale factor values
    int   resampled_scale_factors[MAX_BANDS]; //< resampled scale factors from the previous block
                                              //< can be used in case no new scale factors are transmitted

    int reuse_sf; //< reuse scale factors from a previous subframe
    int transmit_sf;
    int scale_factor_step;
    int quant_step_modifier;
    int    max_scale_factor;
    int   scale_factor_block_len; //< block len of the frame for which the scale factors were transmitted
    DECLARE_ALIGNED_16(float, out[8192]);

} wma_channel;

typedef struct {
    int nb_channels;
    int no_rotation; //< controls the type of the transform
    int transform; //< also controls the type of the transform
    char transform_band[MAX_BANDS]; //< controls if the transform is enabled for a certain band
    char rotation_offset[MAX_CHANNELS * MAX_CHANNELS];
    char positive[MAX_CHANNELS * MAX_CHANNELS]; //< fixme for what are these numbers used?
    float decorrelation_matrix[MAX_CHANNELS*MAX_CHANNELS];
    char use_channel[MAX_CHANNELS];

} wma_channel_group;

/**
 *@brief main decoder context
 */
typedef struct WMA3DecodeContext {
    int16_t*   samples;            //< current samplebuffer pointer
    int16_t*   samples_end;        //< maximum samplebuffer pointer
    int        skip_frame;         //< do not output the current frame
    DSPContext dsp;

    int        num_possible_block_sizes; //< number of possible block sizes
    int*       num_sfb;            //< number of scale factor bands for every block size
    int*       sfb_offsets;        //< scale factor band offsets
    int*       sf_offsets;         //< matrix to find the correct scale factor
    int*       subwoofer_cutoffs;  //< subwoofer cutoff values for every block
    int        subwoofer_cutoff;   //< current subwoofer cutoff value
    int        subframe_len;       //< current subframe length
    int        nb_chgroups;        //< number of channel groups for the current subframe
    wma_channel_group chgroup[MAX_CHANNELS]; //< channel group information

    int lfe_channel;
    int negative_quantstep;
    int quant_step;
    int channels_for_cur_subframe;
    int channel_indices_for_cur_subframe[MAX_CHANNELS];
    const float*** default_decorrelation_matrix;
    int esc_len;

    GetBitContext*    getbit;
    MDCTContext mdct_ctx[BLOCK_NB_SIZES];
    float *windows[BLOCK_NB_SIZES];

    int              cValidBarkBand;
    int*              rgiBarkIndex;

    VLC              exp_vlc;
    VLC              huff_0_vlc;
    VLC              huff_1_vlc;
    VLC              huff_2_vlc;
    VLC              coef_vlc[2];
    VLC              rlc_vlc;
    int              coef_max[2];

    int              parsed_all_subframes;
    AVCodecContext*  avctx;                    //< codec context for av_log
    GetBitContext    gb;                       //< getbitcontext for the packet
    int              buf_bit_size;             //< buffer size in bits

    /** packet info */
    uint8_t          packet_sequence_number;   //< current packet number
    uint8_t          bit5;                     //< padding bit? (CBR files)
    uint8_t          bit6;
    uint8_t          packet_loss;              //< set in case of bitstream error

    /** stream info */
    uint16_t         samples_per_frame;        //< number of outputed samples
    uint16_t         log2_frame_size;          //< frame size
    uint8_t          lossless;                 //< lossless mode
    uint8_t          no_tiling;                //< frames are split in subframes
    int8_t           nb_channels;              //< number of channels
    wma_channel      channel[MAX_CHANNELS];    //< per channel data

    /** extradata */
    unsigned int     decode_flags;             //< used compression features
    unsigned int     dwChannelMask;
    uint8_t          sample_bit_depth;         //< bits per sample

    /** general frame info */
    unsigned int     frame_num;                //< current frame number
    uint8_t          len_prefix;               //< frame is prefixed with its len
    uint8_t          allow_subframes;          //< frames may contain subframes
    uint8_t          max_num_subframes;        //< maximum number of subframes
    uint16_t         min_samples_per_subframe; //< minimum samples per subframe
    uint8_t          dynamic_range_compression;//< frame contains drc data
    uint8_t          drc_gain;                 //< gain for the drc tool

    /** buffered frame data */
    int              prev_frame_bit_size;      //< saved number of bits
    uint8_t*         prev_frame;               //< prev frame data
} WMA3DecodeContext;

#endif

