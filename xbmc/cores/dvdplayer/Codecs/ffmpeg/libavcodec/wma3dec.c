/*
 * WMA 9/3/PRO compatible decoder
 * Copyright (c) 2007 Baptiste Coudurier, Benjamin Larsson, Ulion
 * Copyright (c) 2008 - 2009 Sascha Sommer, Benjamin Larsson
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

#include "avcodec.h"
#include "bitstream.h"
#include "wma3.h"
#undef NDEBUG
#include <assert.h>

unsigned int bitstreamcounter;

/**
 *@brief helper function to print the most important members of the context
 *@param s context
 */
static void dump_context(WMA3DecodeContext *s)
{
#define PRINT(a,b) av_log(s->avctx,AV_LOG_ERROR," %s = %d\n", a, b);
#define PRINT_HEX(a,b) av_log(s->avctx,AV_LOG_ERROR," %s = %x\n", a, b);

    PRINT_HEX("ed channelmask",s->dwChannelMask);
    PRINT("ed sample bit depth",s->sample_bit_depth);
    PRINT_HEX("ed decode flags",s->decode_flags);
    PRINT("samples per frame",s->samples_per_frame);
    PRINT("log2 frame size",s->log2_frame_size);
    PRINT("max num subframes",s->max_num_subframes);
    PRINT("len prefix",s->len_prefix);
    PRINT("nb channels",s->nb_channels);
    PRINT("lossless",s->lossless);
}


/**
 *@brief Get the samples per frame for this stream.
 *@param sample_rate output sample_rate
 *@param decode_flags codec compression features
 *@return number of output samples per frame
 */
static int get_samples_per_frame(int sample_rate, unsigned int decode_flags) {

    int samples_per_frame;
    int tmp;

    if (sample_rate <= 16000)
        samples_per_frame = 512;
    else if (sample_rate <= 22050)
        samples_per_frame = 1024;
    else if (sample_rate <= 48000)
        samples_per_frame = 2048;
    else if (sample_rate <= 96000)
        samples_per_frame = 4096;
    else
        samples_per_frame = 8192;

    /* WMA voice code  if (decode_flags & 0x800) {
        tmp = ((decode_flags & 6) >> 1) | ((decode_flags & 0x600) >> 7);
        samples_per_frame = (tmp+1)*160;
    } else { */

    tmp = decode_flags & 0x6;
    if (tmp == 0x2)
        samples_per_frame <<= 1;
    else if (tmp == 0x4)
        samples_per_frame >>= 1;
    else if (tmp == 0x6)
        samples_per_frame >>= 2;

    return samples_per_frame;
}


/**
 *@brief Uninitialize the decoder and free all ressources.
 *@param avctx codec context
 *@return 0 on success, < 0 otherwise
 */
static av_cold int wma3_decode_end(AVCodecContext *avctx)
{
    WMA3DecodeContext *s = avctx->priv_data;
    int i;

    av_free(s->prev_frame);
    av_free(s->num_sfb);
    av_free(s->sfb_offsets);
    av_free(s->subwoofer_cutoffs);
    av_free(s->sf_offsets);

    if(s->default_decorrelation_matrix){
        for(i=1;i<=s->nb_channels;i++)
            av_free(s->default_decorrelation_matrix[i]);
        av_free(s->default_decorrelation_matrix);
    }

    free_vlc(&s->exp_vlc);
    free_vlc(&s->coef_vlc[0]);
    free_vlc(&s->coef_vlc[1]);
    free_vlc(&s->huff_0_vlc);
    free_vlc(&s->huff_1_vlc);
    free_vlc(&s->huff_2_vlc);
    free_vlc(&s->rlc_vlc);

    for(i=0 ; i<BLOCK_NB_SIZES ; i++)
        ff_mdct_end(&s->mdct_ctx[i]);

    return 0;
}


/**
 *@brief Initialize the decoder.
 *@param avctx codec context
 *@return 0 on success, -1 otherwise
 */
static av_cold int wma3_decode_init(AVCodecContext *avctx)
{
    WMA3DecodeContext *s = avctx->priv_data;
    uint8_t *edata_ptr = avctx->extradata;
    int* sfb_offsets;
    int i;

    s->avctx = avctx;
    dsputil_init(&s->dsp, avctx);

    /* FIXME is this really the right thing todo for 24 bit? */
    s->sample_bit_depth = 16; //avctx->bits_per_sample;
    if (avctx->extradata_size >= 18) {
        s->decode_flags     = AV_RL16(edata_ptr+14);
        s->dwChannelMask    = AV_RL32(edata_ptr+2);
//        s->sample_bit_depth = AV_RL16(edata_ptr);

        /** dump the extradata */
        for (i=0 ; i<avctx->extradata_size ; i++)
            av_log(avctx, AV_LOG_ERROR, "[%x] ",avctx->extradata[i]);
        av_log(avctx, AV_LOG_ERROR, "\n");

    } else {
        av_log(avctx, AV_LOG_ERROR, "Unknown extradata size %d.\n",
                      avctx->extradata_size);
        return -1;
    }

    /** generic init */
    s->packet_loss = 0; //FIXME unneeded
    s->log2_frame_size = av_log2(avctx->block_align*8)+1;

    /** frame info */
    s->skip_frame = 1; /** skip first frame */
    s->len_prefix = s->decode_flags & 0x40;

    if(!s->len_prefix){
         av_log(avctx, AV_LOG_ERROR, "file has no len prefix please report\n");
         return -1;
    }

    /** get frame len */
    s->samples_per_frame = get_samples_per_frame(avctx->sample_rate,
                                                 s->decode_flags);

    /** init previous block len */
    for(i=0;i<avctx->channels;i++)
        s->channel[i].prev_block_len = s->samples_per_frame;

    /** subframe info */
    s->max_num_subframes = 1 << ((s->decode_flags & 0x38) >> 3);
    s->num_possible_block_sizes = av_log2(s->max_num_subframes) + 1;
    s->allow_subframes = s->max_num_subframes > 1;
    s->min_samples_per_subframe = s->samples_per_frame / s->max_num_subframes;
    s->dynamic_range_compression = (s->decode_flags & 0x80) >> 7;

    if(s->max_num_subframes > MAX_SUBFRAMES){
        av_log(avctx, AV_LOG_ERROR, "invalid number of subframes %i\n",
                      s->max_num_subframes);
        return -1;
    }

    s->nb_channels = avctx->channels;

    /** extract lfe channel position */
    s->lfe_channel = -1;

    if(s->dwChannelMask & 8){
        unsigned int mask = 1;
        for(i=0;i<32;i++){
            if(s->dwChannelMask & mask)
                ++s->lfe_channel;
            if(mask & 8)
                break;
            mask <<= 1;
        }
    }

    if(s->nb_channels < 0 || s->nb_channels > MAX_CHANNELS){
        av_log(avctx, AV_LOG_ERROR, "invalid number of channels %i\n",
                      s->nb_channels);
        return -1;
    }

    init_vlc(&s->exp_vlc, EXPVLCBITS, FF_WMA3_SCALE_SIZE,
                 ff_wma3_scale_huffbits, 1, 1,
                 ff_wma3_scale_huffcodes, 4, 4, 0);

    init_vlc(&s->coef_vlc[0], VLCBITS, FF_WMA3_COEF0_SIZE,
                 ff_wma3_coef0_huffbits, 1, 1,
                 ff_wma3_coef0_huffcodes, 4, 4, 0);

    s->coef_max[0] = ((22+VLCBITS-1)/VLCBITS);

    init_vlc(&s->coef_vlc[1], VLCBITS, FF_WMA3_COEF1_SIZE,
                 ff_wma3_coef1_huffbits, 1, 1,
                 ff_wma3_coef1_huffcodes, 4, 4, 0);

    s->coef_max[1] = ((21+VLCBITS-1)/VLCBITS);

    init_vlc(&s->huff_0_vlc, VLCBITS, FF_WMA3_HUFF_0_SIZE,
                 ff_wma3_huff_0_bits, 1, 1,
                 ff_wma3_huff_0_codes, 4, 4, 0);

    init_vlc(&s->huff_1_vlc, VLCBITS, FF_WMA3_HUFF_1_SIZE,
                 ff_wma3_huff_1_bits, 1, 1,
                 ff_wma3_huff_1_codes, 4, 4, 0);

    init_vlc(&s->huff_2_vlc, VLCBITS, FF_WMA3_HUFF_2_SIZE,
                 ff_wma3_huff_2_bits, 1, 1,
                 ff_wma3_huff_2_codes, 4, 4, 0);

    init_vlc(&s->rlc_vlc, VLCBITS, FF_WMA3_HUFF_RLC_SIZE,
                 ff_wma3_huff_rlc_bits, 1, 1,
                 ff_wma3_huff_rlc_codes, 4, 4, 0);

    s->num_sfb = av_mallocz(sizeof(int)*s->num_possible_block_sizes);
    s->sfb_offsets = av_mallocz(MAX_BANDS * sizeof(int) *s->num_possible_block_sizes);
    s->subwoofer_cutoffs = av_mallocz(sizeof(int)*s->num_possible_block_sizes);
    s->sf_offsets = av_mallocz(s->num_possible_block_sizes *
                               s->num_possible_block_sizes * MAX_BANDS * sizeof(int));

    if(!s->num_sfb || !s->sfb_offsets || !s->subwoofer_cutoffs || !s->sf_offsets){
        av_log(avctx, AV_LOG_ERROR, "failed to allocate scale factor offset tables\n");
        wma3_decode_end(avctx);
        return -1;
    }

    /** calculate number of scale factor bands and their offsets
        for every possible block size */
    sfb_offsets = s->sfb_offsets;

    for(i=0;i<s->num_possible_block_sizes;i++){
        int subframe_len = s->samples_per_frame / (1 << i);
        int x;
        int band = 1;

        sfb_offsets[0] = 0;

        for(x=0;x < MAX_BANDS-1 && sfb_offsets[band-1] < subframe_len;x++){
            int offset = (subframe_len * 2 * ff_wma3_critical_freq[x]) / s->avctx->sample_rate + 2;
            offset -= offset % 4;
            if ( offset > sfb_offsets[band - 1] ){
                sfb_offsets[band] = offset;
                ++band;
            }
        }
        sfb_offsets[band - 1] = subframe_len;
        s->num_sfb[i] = band - 1;
        sfb_offsets += MAX_BANDS;
    }


    /** scale factors can be shared between blocks of different size
        as every block has a different scale factor band layout
        the matrix sf_offsets is needed to find the correct scale factor
     */

    for(i=0;i<s->num_possible_block_sizes;i++){
        int b;
        for(b=0;b< s->num_sfb[i];b++){
            int x;
            int offset = ((s->sfb_offsets[MAX_BANDS * i + b]
                          + s->sfb_offsets[MAX_BANDS * i + b + 1] - 1)<<i)/2;
            for(x=0;x<s->num_possible_block_sizes;x++){
                int v = 0;
                while(s->sfb_offsets[MAX_BANDS * x + v +1] << x < offset)
                    ++v;
                s->sf_offsets[  i * s->num_possible_block_sizes * MAX_BANDS
                              + x * MAX_BANDS + b] = v;
            }
        }
    }

    /** init MDCT, FIXME only init needed sizes */
    for(i = 0; i < BLOCK_NB_SIZES; i++)
        ff_mdct_init(&s->mdct_ctx[i], BLOCK_MIN_BITS+1+i, 1);

    /** init MDCT windows : simple sinus window */
    for(i=0 ; i<BLOCK_NB_SIZES ; i++) {
        int n;
        n = 1 << (BLOCK_MAX_BITS - i);
        ff_sine_window_init(ff_sine_windows[BLOCK_MAX_BITS - i - 7], n);
        s->windows[BLOCK_NB_SIZES-i-1] = ff_sine_windows[BLOCK_MAX_BITS - i - 7];
    }

    /** calculate subwoofer cutoff values */

    for(i=0;i< s->num_possible_block_sizes;i++){
        int block_size = s->samples_per_frame / (1 << i);
        int cutoff = ceil(block_size * 440.0 / (double)s->avctx->sample_rate + 0.5);
        if(cutoff < 4)
            cutoff = 4;
        if(cutoff > block_size)
            cutoff = block_size;
        s->subwoofer_cutoffs[i] = cutoff;
    }
    s->subwoofer_cutoff = s->subwoofer_cutoffs[0];


    //FIXME
    /** init stuff for the postprocxform */
    s->default_decorrelation_matrix = av_mallocz(sizeof(int) * (s->nb_channels + 1));
    s->default_decorrelation_matrix[0] = 0;
    for(i=1;i<=s->nb_channels;i++){
        const float* tab = ff_wma3_default_decorrelation_matrices;
        s->default_decorrelation_matrix[i] = av_mallocz(sizeof(float) * i);
        switch(i){
            case 1:
                s->default_decorrelation_matrix[i][0] = &tab[0];
                break;
            case 2:
                s->default_decorrelation_matrix[i][0] = &tab[1];
                s->default_decorrelation_matrix[i][1] = &tab[3];
                break;
            case 3:
                s->default_decorrelation_matrix[i][0] = &tab[5];
                s->default_decorrelation_matrix[i][1] = &tab[8];
                s->default_decorrelation_matrix[i][2] = &tab[11];
                break;
            case 4:
                s->default_decorrelation_matrix[i][0] = &tab[14];
                s->default_decorrelation_matrix[i][1] = &tab[18];
                s->default_decorrelation_matrix[i][2] = &tab[22];
                s->default_decorrelation_matrix[i][3] = &tab[26];
                break;
            case 5:
                s->default_decorrelation_matrix[i][0] = &tab[30];
                s->default_decorrelation_matrix[i][1] = &tab[35];
                s->default_decorrelation_matrix[i][2] = &tab[40];
                s->default_decorrelation_matrix[i][3] = &tab[45];
                s->default_decorrelation_matrix[i][4] = &tab[50];
                break;
            case 6:
                s->default_decorrelation_matrix[i][0] = &tab[55];
                s->default_decorrelation_matrix[i][1] = &tab[61];
                s->default_decorrelation_matrix[i][2] = &tab[67];
                s->default_decorrelation_matrix[i][3] = &tab[73];
                s->default_decorrelation_matrix[i][4] = &tab[79];
                s->default_decorrelation_matrix[i][5] = &tab[85];
                break;
        }
    }

    dump_context(s);
    return 0;
}



/**
 *@brief Decode how the data in the frame is split into subframes
 *       every WMA frame contains the encoded data for a fixed number of
 *       samples per channel the data for every channel might be split
 *       into several subframes this function will reconstruct the list of
 *       subframes for every channel
 *
 *       If the subframes are not evenly split the algorithm estimates the
 *       channels with the lowest number of total samples.
 *       Afterwards for each of these channels a bit is read from the
 *       bitstream that indicates if the channel contains a frame with the
 *       next subframe size that is going to be read from the bitstream or not.
 *       If a channel contains such a subframe the subframe size gets added to
 *       the channel's subframe list.
 *       The algorithm repeats these steps until the frame is properly divided
 *       between the individual channels.
 *
 *
 *@param s context
 *@param gb current get bit context
 *@return 0 on success < 0 in case of an error
 */
static int wma_decode_tilehdr(WMA3DecodeContext *s, GetBitContext* gb){
    int c;
    int missing_samples = s->nb_channels * s->samples_per_frame;

    /** reset tiling information */
    for(c=0;c<s->nb_channels;c++){
        s->channel[c].num_subframes = 0;
        s->channel[c].channel_len = 0;
    }

    /** handle the easy case whith one constant sized subframe per channel */
    if(s->max_num_subframes == 1){
        for(c=0;c<s->nb_channels;c++){
            s->channel[c].num_subframes = 1;
            s->channel[c].subframe_len[0] = s->samples_per_frame;
            s->channel[c].channel_len = 0;
        }
    }else{ /** subframe len and number of subframes is not constant */
        int subframe_len_bits = 0;     /** bits needed for the subframe len */
        int subframe_len_zero_bit = 0; /** how many of the len bits indicate
                                           if the subframe is zero */

        /** calculate subframe len bits */
        if(s->lossless)
            subframe_len_bits = av_log2(s->max_num_subframes - 1) + 1;
        else if(s->max_num_subframes == 16){
            subframe_len_zero_bit = 1;
            subframe_len_bits = 3;
        }else
            subframe_len_bits = av_log2(av_log2(s->max_num_subframes)) + 1;

        /** loop until the frame data is split between the subframes */
        while(missing_samples > 0){
            int64_t tileinfo = -1;
            int min_channel_len = s->samples_per_frame;
            int num_subframes_per_channel = 0;
            int num_channels = 0;
            int subframe_len = s->samples_per_frame / s->max_num_subframes;

            /** find channel with the smallest overall len */
            for(c=0;c<s->nb_channels;c++){
                if(min_channel_len > s->channel[c].channel_len)
                    min_channel_len = s->channel[c].channel_len;
            }

            /** check if this is the start of a new frame */
            if(missing_samples == s->nb_channels * s->samples_per_frame){
                s->no_tiling = get_bits1(gb);
            }

            if(s->no_tiling){
                num_subframes_per_channel = 1;
                num_channels = s->nb_channels;
            }else{
                /** count how many channels have the minimum len */
                for(c=0;c<s->nb_channels;c++){
                    if(min_channel_len == s->channel[c].channel_len){
                        ++num_channels;
                    }
                }
                if(num_channels <= 1)
                    num_subframes_per_channel = 1;
            }

            /** maximum number of subframes, evenly split */
            if(subframe_len == missing_samples / num_channels){
                num_subframes_per_channel = 1;
            }

            /** if there might be multiple subframes per channel */
            if(num_subframes_per_channel != 1){
                int total_num_bits = num_channels;
                tileinfo = 0;
                /** for every channel with the minimum len 1 bit is
                    transmitted that informs us if the channel
                    contains a subframe with the next subframe_len */
                while(total_num_bits){
                int num_bits = total_num_bits;
                if(num_bits > 32)
                    num_bits = 32;
                tileinfo |= get_bits_long(gb,num_bits);
                total_num_bits -= num_bits;
                num_bits = total_num_bits;
                tileinfo <<= (num_bits > 32)? 32 : num_bits;
                }
            }

            /** if the frames are not evenly split get the next subframe len
                from the bitstream */
            if(subframe_len != missing_samples / num_channels){
                int log2_subframe_len = 0;
                /* 1 bit indicates if the subframe len is zero */
                if(subframe_len_zero_bit){
                    if(get_bits1(gb)){
                        log2_subframe_len = get_bits(gb,subframe_len_bits-1);
                        ++log2_subframe_len;
                    }
                }else
                    log2_subframe_len = get_bits(gb,subframe_len_bits);

                if(s->lossless){
                    subframe_len =
                        s->samples_per_frame / s->max_num_subframes;
                    subframe_len *= log2_subframe_len + 1;
                }else
                    subframe_len =
                        s->samples_per_frame / (1 << log2_subframe_len);
            }

            /** sanity check the len */
            if(subframe_len < s->min_samples_per_subframe
                || subframe_len > s->samples_per_frame){
                av_log(s->avctx, AV_LOG_ERROR,
                        "broken frame: subframe_len %i\n", subframe_len);
                return -1;
            }
            for(c=0; c<s->nb_channels;c++){
                wma_channel* chan = &s->channel[c];
                if(chan->num_subframes > 32){
                    av_log(s->avctx, AV_LOG_ERROR,
                            "broken frame: num subframes %i\n",
                            chan->num_subframes);
                    return -1;
                }

                /** add subframes to the individual channels */
                if(min_channel_len == chan->channel_len){
                    --num_channels;
                    if(tileinfo & (1<<num_channels)){
                        if(chan->num_subframes > 31){
                            av_log(s->avctx, AV_LOG_ERROR,
                                    "broken frame: num subframes > 31\n");
                            return -1;
                        }
                        chan->subframe_len[chan->num_subframes] = subframe_len;
                        chan->channel_len += subframe_len;
                        missing_samples -= subframe_len;
                        ++chan->num_subframes;
                        if(missing_samples < 0
                            || chan->channel_len > s->samples_per_frame){
                            av_log(s->avctx, AV_LOG_ERROR,"broken frame: "
                                    "channel len > samples_per_frame\n");
                            return -1;
                        }
                    }
                }
            }
        }
    }

    for(c=0;c<s->nb_channels;c++){
        int i;
        int offset = 0;
        for(i=0;i<s->channel[c].num_subframes;i++){
            av_log(s->avctx, AV_LOG_DEBUG,"frame[%i] channel[%i] subframe[%i]"
                   " len %i\n",s->frame_num,c,i,s->channel[c].subframe_len[i]);
            s->channel[c].subframe_offset[i] = offset;
            offset += s->channel[c].subframe_len[i];
        }
    }

    return 0;
}


static int wma_decode_channel_transform(WMA3DecodeContext* s, GetBitContext* gb){
    int i;
    for(i=0;i< s->nb_channels;i++){
        memset(s->chgroup[i].decorrelation_matrix,0,4*s->nb_channels * s->nb_channels);
    }

    if(s->nb_channels == 1 ){
        s->nb_chgroups = 0;
        s->chgroup[0].nb_channels = 1;
        s->chgroup[0].no_rotation = 1;
        s->chgroup[0].transform = 2;
        s->channel[0].resampled_scale_factors[0] = 0;
        memset(s->chgroup[0].transform_band,0,MAX_BANDS);
        memset(s->chgroup[0].decorrelation_matrix,0,4*s->nb_channels * s->nb_channels);

        s->chgroup[0].decorrelation_matrix[0] = 1.0;

    }else{
        int remaining_channels = s->channels_for_cur_subframe;

        if(get_bits(gb,1)){
            av_log(s->avctx,AV_LOG_ERROR,"unsupported channel transform bit\n");
            return 0;
        }

        for(s->nb_chgroups = 0; remaining_channels && s->nb_chgroups < s->channels_for_cur_subframe;s->nb_chgroups++){
            wma_channel_group* chgroup = &s->chgroup[s->nb_chgroups];
            chgroup->nb_channels = 0;
            chgroup->no_rotation = 0;
            chgroup->transform = 0;

            /* decode channel mask */
            memset(chgroup->use_channel,0,sizeof(chgroup->use_channel));

            if(remaining_channels > 2){
                for(i=0;i<s->channels_for_cur_subframe;i++){
                    int channel_idx = s->channel_indices_for_cur_subframe[i];
                    if(!s->channel[channel_idx].grouped && get_bits(gb,1)){
                        ++chgroup->nb_channels;
                        s->channel[channel_idx].grouped = 1;
                        chgroup->use_channel[channel_idx] = 1;
                    }
                }
            }else{
                chgroup->nb_channels = remaining_channels;
                for(i=0;i<s->nb_channels ;i++){
                    chgroup->use_channel[i] = s->channel[i].grouped != 1;
                    s->channel[i].grouped = 1;
                }
            }

            /** done decode channel mask */

            /* decide x form type FIXME port this to float, all rotations should lie on the unit circle*/
            if(chgroup->nb_channels == 1){
                chgroup->no_rotation = 1;
                chgroup->transform = 2;
                chgroup->decorrelation_matrix[0] = 1.0;

            }else if(chgroup->nb_channels == 2){
                if(get_bits(gb,1)){
                    if(!get_bits(gb,1)){
                        chgroup->no_rotation = 1;
                        chgroup->transform = 2;
                        chgroup->decorrelation_matrix[0] = 1.0;
                        chgroup->decorrelation_matrix[1] = 0;
                        chgroup->decorrelation_matrix[2] = 0;
                        chgroup->decorrelation_matrix[3] = 1.0;
                    }
                }else{
                    chgroup->no_rotation = 1;
                    chgroup->transform = 1;
                    chgroup->decorrelation_matrix[0] = 0.70703125;  //FIXME cos(pi/4)
                    chgroup->decorrelation_matrix[1] = -0.70703125;
                    chgroup->decorrelation_matrix[2] = 0.70703125;
                    chgroup->decorrelation_matrix[3] = 0.70703125;
                }
            }else{
                if(get_bits(gb,1)){
                    if(get_bits(gb,1)){
                        chgroup->no_rotation = 0;
                        chgroup->transform = 0;
                    }else{
                        int x;
                        chgroup->no_rotation = 1;
                        chgroup->transform = 3;
                        for(x = 0; x < chgroup->nb_channels ; x++){
                            int y;
                            for(y=0;y< chgroup->nb_channels ;y++){
                                chgroup->decorrelation_matrix[y + x * chgroup->nb_channels] = s->default_decorrelation_matrix[chgroup->nb_channels][x][y];
                        }
                        }
                    }
                }else{
                    int i;
                    chgroup->no_rotation = 1;
                    chgroup->transform = 2;
                    for(i=0;i<chgroup->nb_channels;i++){
                        chgroup->decorrelation_matrix[i+i*chgroup->nb_channels] = 1.0;
                    }
                }
            }

            /** done decide x form type */

            if(!chgroup->no_rotation){ /** decode channel transform */
                int n_offset = chgroup->nb_channels  * (chgroup->nb_channels - 1) / 2;
                int i;
                for(i=0;i<n_offset;i++){
                    chgroup->rotation_offset[i] = get_bits(gb,6);
                }
                for(i=0;i<chgroup->nb_channels;i++)
                    chgroup->positive[i] = get_bits(gb,1);
            }

            /* decode transform on / off */
            if(chgroup->nb_channels <= 1 ||  ((chgroup->no_rotation != 1 || chgroup->transform == 2) && chgroup->no_rotation)){
                // done
                int i;
                for(i=0;i<s->cValidBarkBand;i++)
                    chgroup->transform_band[i] = 1;
            }else{
                if(get_bits(gb,1) == 0){
                    int i;
                    // transform works on individual scale factor bands
                    for(i=0;i< s->cValidBarkBand;i++){
                        chgroup->transform_band[i] = get_bits(gb,1);
                    }
                }else{
                    int i;
                    for(i=0;i<s->cValidBarkBand;i++)
                        chgroup->transform_band[i] = 1;
                }
            }
            /** done decode transform on / off */
            remaining_channels -= chgroup->nb_channels;
        }
    }
    return 1;
}


static unsigned int wma_get_large_val(WMA3DecodeContext* s)
{
    int n_bits = 8;
    if(get_bits(s->getbit,1)){
        n_bits += 8;
        if(get_bits(s->getbit,1)){
            n_bits += 8;
            if(get_bits(s->getbit,1)){
                n_bits += 7;
            }
        }
    }
    return get_bits_long(s->getbit,n_bits);
}


static inline void wma_get_vec4(WMA3DecodeContext *s,int* vals,int* masks){
        unsigned int idx;
        int i = 0;
        // read 4 values
        idx = get_vlc2(s->getbit, s->huff_0_vlc.table, VLCBITS, ((FF_WMA3_HUFF_0_MAXBITS+VLCBITS-1)/VLCBITS));


        if ( idx == 126 )
        {
          while(i < 4){
              idx = get_vlc2(s->getbit, s->huff_1_vlc.table, VLCBITS, ((FF_WMA3_HUFF_1_MAXBITS+VLCBITS-1)/VLCBITS));
              if ( idx == 136 ){
                   vals[i] = get_vlc2(s->getbit, s->huff_2_vlc.table, VLCBITS, ((FF_WMA3_HUFF_2_MAXBITS+VLCBITS-1)/VLCBITS));
                   if(vals[i] == 100)
                       vals[i] += wma_get_large_val(s);
                   vals[i+1] = get_vlc2(s->getbit, s->huff_2_vlc.table, VLCBITS, ((FF_WMA3_HUFF_2_MAXBITS+VLCBITS-1)/VLCBITS));
                   if(vals[i+1] == 100)
                       vals[i+1] += wma_get_large_val(s);
              }else{
                  vals[i] = (ff_wma3_sym2vec2[idx] >> 4) & 0xF;
                  vals[i+1] = ff_wma3_sym2vec2[idx] & 0xF;
              }
              i += 2;
          }
        }
        else
        {
          vals[0] = (unsigned char)(ff_wma3_sym2vec4[idx] >> 8) >> 4;
          vals[1] = (ff_wma3_sym2vec4[idx] >> 8) & 0xF;
          vals[2] = (ff_wma3_sym2vec4[idx] >> 4) & 0xF;
          vals[3] = ff_wma3_sym2vec4[idx] & 0xF;
        }

        if(vals[0])
            masks[0] = get_bits(s->getbit,1);
        if(vals[1])
            masks[1] = get_bits(s->getbit,1);
        if(vals[2])
            masks[2] = get_bits(s->getbit,1);
        if(vals[3])
            masks[3] = get_bits(s->getbit,1);
}


static int decode_coeffs(WMA3DecodeContext *s,GetBitContext* gb,int c)
{
    int vlctable;
    VLC* vlc;
    int vlcmax;
    wma_channel* ci = &s->channel[c];
    int rl_mode = 0;
    int cur_coeff = 0;
    int last_write = 0;
    const uint8_t* run;
    const uint8_t* level;

    av_log(s->avctx,AV_LOG_DEBUG,"decode  coefs for channel %i\n",c);

    s->getbit = gb;
    s->esc_len = av_log2(s->subframe_len -1) +1;
    vlctable = get_bits(s->getbit, 1);
    vlc = &s->coef_vlc[!vlctable];
    vlcmax = s->coef_max[!vlctable];

    if(!vlctable){
        run =  ff_wma3_run_1;
        level =  ff_wma3_level_1;
    }else{
        run =  ff_wma3_run_0;
        level =  ff_wma3_level_0;
    }

    while(cur_coeff < s->subframe_len){
        if(rl_mode){
            unsigned int idx;
            int mask;
            int val;
            idx = get_vlc2(s->getbit, vlc->table, VLCBITS, vlcmax);

            if( idx == 1)
                return 0;
            else if( !idx ){
                val = wma_get_large_val(s);
                /** escape decode */
                if(get_bits(s->getbit,1)){
                    if(get_bits(s->getbit,1)){
                        if(get_bits(s->getbit,1)){
                            av_log(s->avctx,AV_LOG_ERROR,"broken escape sequence\n");
                            return 0;
                        }else
                            cur_coeff += get_bits(s->getbit,s->esc_len) + 4;
                    }else
                        cur_coeff += get_bits(s->getbit,2) + 1;
                }
            }else{
                cur_coeff += run[idx - 2];
                val = level[idx - 2];
            }
            mask = get_bits(s->getbit,1) - 1;
            ci->coeffs[cur_coeff] = (val^mask) - mask;
            ++cur_coeff;
        }else{
            int i = 0;
            int vals[4];
            int masks[4];

            if( cur_coeff >= s->subframe_len )
                return 0;

            wma_get_vec4(s,vals,masks);

            while(i < 4){
                ++cur_coeff;
                ++last_write;

                if(vals[i]){
                    ci->coeffs[cur_coeff-1] = (((int64_t)vals[i])^(masks[i] -1)) - (masks[i] -1);
                    last_write = 0;
                }
                if( cur_coeff >= s->subframe_len ) // handled entire subframe -> quit
                    return 0;
                if ( last_write > s->subframe_len / 256 ) //switch to RL mode
                    rl_mode = 1;
                ++i;
            }
        }
    }

    return 0;
}



static int wma_decode_scale_factors(WMA3DecodeContext* s,GetBitContext* gb)
{
    int i;
    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];

        /** resample scale factors for the new block size */
        if(s->channel[c].reuse_sf){
            int b;
            for(b=0;b< s->cValidBarkBand;b++){
                int idx0 = av_log2(s->samples_per_frame/s->subframe_len);
                int idx1 = av_log2(s->samples_per_frame/s->channel[c].scale_factor_block_len);
                int bidx = s->sf_offsets[s->num_possible_block_sizes * MAX_BANDS  * idx0
                                                 + MAX_BANDS * idx1 + b];
                s->channel[c].resampled_scale_factors[b] = s->channel[c].scale_factors[bidx];
            }
            s->channel[c].max_scale_factor = s->channel[c].resampled_scale_factors[0];
            for(b=1;b<s->cValidBarkBand;b++){
                if(s->channel[c].resampled_scale_factors[b] > s->channel[c].max_scale_factor)
                    s->channel[c].max_scale_factor = s->channel[c].resampled_scale_factors[b];
            }
        }

        if(s->channel[c].cur_subframe > 0){
            s->channel[c].transmit_sf = get_bits(gb,1);
        }else
            s->channel[c].transmit_sf = 1;

        if(s->channel[c].transmit_sf){
            int b;

            if(!s->channel[c].reuse_sf){
                int i;
                s->channel[c].scale_factor_step = get_bits(gb,2) + 1;
                for(i=0;i<s->cValidBarkBand;i++){
                    int val = get_vlc2(gb, s->exp_vlc.table, EXPVLCBITS, EXPMAX); //dpcm coded
                    if(!i)
                        s->channel[c].scale_factors[i] = 45 / s->channel[c].scale_factor_step + val - 60;
                    else
                        s->channel[c].scale_factors[i]  = s->channel[c].scale_factors[i-1] + val - 60;
                }
            }else{     // rl coded
                int i;
                memcpy(s->channel[c].scale_factors,s->channel[c].resampled_scale_factors,
                       4 * s->cValidBarkBand);

                for(i=0;i<s->cValidBarkBand;i++){
                    int idx;
                    short skip;
                    short level_mask;
                    short val;

                    idx = get_vlc2(gb, s->rlc_vlc.table, VLCBITS, ((FF_WMA3_HUFF_RLC_MAXBITS+VLCBITS-1)/VLCBITS));

                    if( !idx ){
                        uint32_t mask = get_bits(gb,14);
                        level_mask = mask >> 6;
                        val = (mask & 1) - 1;
                        skip = (0x3f & mask)>>1;
                    }else if(idx == 1){
                        break;
                    }else{
                        skip = ff_wma3_run_mask[idx-2];
                        level_mask = ff_wma3_level_mask[idx-2];
                        val = get_bits(gb,1)-1;
                    }

                    i += skip;
                    if(i >= s->cValidBarkBand){
                        av_log(s->avctx,AV_LOG_ERROR,"invalid scale factor coding\n");
                        return 0;
                    }else
                        s->channel[c].scale_factors[i] += (val ^ level_mask) - val;
                }
            }

            s->channel[c].reuse_sf = 1;
            s->channel[c].max_scale_factor = s->channel[c].scale_factors[0];
            for(b=1;b<s->cValidBarkBand;b++){
                if(s->channel[c].max_scale_factor < s->channel[c].scale_factors[b])
                    s->channel[c].max_scale_factor = s->channel[c].scale_factors[b];
            }
            s->channel[c].scale_factor_block_len = s->subframe_len;
        }
    }
    return 1;
}

static void wma_calc_decorrelation_matrix(WMA3DecodeContext *s, wma_channel_group* chgroup){
    int i;
    int offset = 0;
    memset(chgroup->decorrelation_matrix, 0, chgroup->nb_channels * 4 * chgroup->nb_channels);
    for(i=0;i<chgroup->nb_channels;i++)
        chgroup->decorrelation_matrix[chgroup->nb_channels * i + i] = chgroup->positive[i]?1.0:-1.0;

    for(i=0;i<chgroup->nb_channels;i++){
        if ( i > 0 )
        {
            int x;
            for(x=0;x<i;x++){
                int y;
                float tmp1[MAX_CHANNELS];
                float tmp2[MAX_CHANNELS];
                memcpy(tmp1, &chgroup->decorrelation_matrix[x * chgroup->nb_channels], 4 * (i + 1));
                memcpy(tmp2, &chgroup->decorrelation_matrix[i * chgroup->nb_channels], 4 * (i + 1));
                for(y=0;y < i + 1 ; y++){
                    float v1 = tmp1[y];
                    float v2 = tmp2[y];
                    int n = chgroup->rotation_offset[offset + x];
                    float cosv = sin(n*M_PI / 64.0);                //FIXME use one table for this
                    float sinv = -cos(n*M_PI / 64.0);

                    chgroup->decorrelation_matrix[y + x * chgroup->nb_channels] = (v1 * cosv) + (v2 * sinv);
                    chgroup->decorrelation_matrix[y + i * chgroup->nb_channels] = (v1 * -sinv) + (v2 * cosv);
                }
            }
        }
        offset += i;
    }

}


static void wma_inverse_channel_transform(WMA3DecodeContext *s)
{
    int i;

    for(i=0;i<s->nb_chgroups;i++){

        if(s->chgroup[i].nb_channels == 1)
            continue;

        if(s->chgroup[i].no_rotation == 1 && s->chgroup[i].transform == 2)
            continue;

        if((s->nb_channels == 2) &&
            (s->chgroup[i].no_rotation == 1) &&
            (s->chgroup[i].transform == 1)){
            int b;
            for(b = 0; b < s->cValidBarkBand;b++){
                int y;
                if(s->chgroup[i].transform_band[b] == 1){ //M/S stereo
                    for(y=s->rgiBarkIndex[b];y<FFMIN(s->rgiBarkIndex[b+1], s->subframe_len);y++){
                        float v1 = s->channel[0].coeffs[y];
                        float v2 = s->channel[1].coeffs[y];
                        s->channel[0].coeffs[y] = v1 - v2;
                        s->channel[1].coeffs[y] = v1 + v2;
                    }
                }else{
                    for(y=s->rgiBarkIndex[b];y<FFMIN(s->rgiBarkIndex[b+1], s->subframe_len);y++){
                        s->channel[0].coeffs[y] *= 362;
                        s->channel[0].coeffs[y] /= 256;
                        s->channel[1].coeffs[y] *= 362;
                        s->channel[1].coeffs[y] /= 256;
                    }
                }
            }
        }else{
            int x;
            int b;
            int cnt = 0;
            float* ch_data[MAX_CHANNELS];
            float  sums[MAX_CHANNELS * MAX_CHANNELS];
            if(!s->chgroup[i].no_rotation)
                wma_calc_decorrelation_matrix(s,&s->chgroup[i]);

            for(x=0;x<s->channels_for_cur_subframe;x++){
                int chan = s->channel_indices_for_cur_subframe[x];
                if(s->chgroup[i].use_channel[chan] == 1){    // assign ptrs
                    ch_data[cnt] = s->channel[chan].coeffs;
                    ++cnt;
                }
            }

            for(b = 0; b < s->cValidBarkBand;b++){
                int y;
                if(s->chgroup[i].transform_band[b] == 1){
                    //multiply values with decorrelation_matrix
                    for(y=s->rgiBarkIndex[b];y<FFMIN(s->rgiBarkIndex[b+1], s->subframe_len);y++){
                        float* matrix = s->chgroup[i].decorrelation_matrix;
                        int m;

                        for(m = 0;m<s->chgroup[i].nb_channels;m++)
                            sums[m] = 0;

                        for(m = 0;m<s->chgroup[i].nb_channels * s->chgroup[i].nb_channels;m++)
                            sums[m/s->chgroup[i].nb_channels] += (matrix[m] * ch_data[m%s->chgroup[i].nb_channels][0]);

                        for(m = 0;m<s->chgroup[i].nb_channels;m++){
                            ch_data[m][0] = sums[m];
                            ++ch_data[m];
                        }
                    }
                }else{      /** skip band */
                    for(y=0;y<s->chgroup[i].nb_channels;y++)
                        ch_data[y] += s->rgiBarkIndex[b+1] -  s->rgiBarkIndex[b];
                }
            }
        }
    }
}


static void wma_window(WMA3DecodeContext *s){
    int i;
    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];
        int j = s->channel[c].cur_subframe;
        int x;
        //FIXME use dsp.vector_fmul_window
        float* start;
        float* end;
        float* window;
        int prev_block_len = s->channel[c].prev_block_len;
        int block_len = s->channel[c].subframe_len[j];
        int len;
        int winlen;
        start = &s->channel[c].out[s->samples_per_frame/2 + s->channel[c].subframe_offset[j] - prev_block_len /2 ];
        end = &s->channel[c].out[s->samples_per_frame/2 + s->channel[c].subframe_offset[j] + block_len /2 - 1];

        if(block_len <= prev_block_len){
            start += (prev_block_len - block_len)/2;
            len = block_len/2;
            winlen = block_len;
        }else{
            end -= (block_len - prev_block_len)/2;
            len = prev_block_len /2;
            winlen = prev_block_len;
        }

//    float* rs = &s->channel[c].out[s->samples_per_frame/2 + s->channel[c].subframe_offset[i]];
//    printf("Dstart %i %i end %i win %i prev %i\n",s->frame_num+1, start - rs,end -rs,winlen,prev_block_len);

        //FIXME untagle the windowing so dsp functions can be used
        window = s->windows[av_log2(winlen)-BLOCK_MIN_BITS];
        for(x=0;x<len;x++){
            float ts = *start;
            float te = *end;
            float sin_v = window[x];
            float cos_v = window[winlen - x -1];
            *start = cos_v * ts - sin_v * te;
            *end = cos_v * te   + sin_v * ts;
            ++start;
            --end;
        }
        s->channel[c].prev_block_len = block_len;
    }
}



static int wma_decode_subframe(WMA3DecodeContext *s,GetBitContext* gb){
    int offset = s->samples_per_frame;
    int subframe_len = s->samples_per_frame;
    int i;
    int total_samples = s->samples_per_frame * s->nb_channels;
    int transmit_coeffs = 0;

    bitstreamcounter = get_bits_count(gb);


    /** reset channel context and find the next block offset and size
        == the next block of the channel with the smallest number of decoded samples
    */
    for(i=0;i<s->nb_channels;i++){
        s->channel[i].grouped = 0;
        memset(s->channel[i].coeffs,0,sizeof(s->channel[0].coeffs));
        if(offset > s->channel[i].decoded_samples){
            offset = s->channel[i].decoded_samples;
            subframe_len = s->channel[i].subframe_len[s->channel[i].cur_subframe];
        }
    }

    av_log(s->avctx, AV_LOG_DEBUG,"processing subframe with offset %i len %i\n",offset,subframe_len);

    /** get a list of all channels that contain the estimated block */
    s->channels_for_cur_subframe = 0;
    for(i=0;i<s->nb_channels;i++){
        /** substract already processed samples */
        total_samples -= s->channel[i].decoded_samples;

        /** and count if there are multiple subframes that match our profile */
        if(offset == s->channel[i].decoded_samples && subframe_len == s->channel[i].subframe_len[s->channel[i].cur_subframe]){
             total_samples -= s->channel[i].subframe_len[s->channel[i].cur_subframe];
             s->channel[i].decoded_samples += s->channel[i].subframe_len[s->channel[i].cur_subframe];
             s->channel_indices_for_cur_subframe[s->channels_for_cur_subframe] = i;
             ++s->channels_for_cur_subframe;
        }
    }

    /** check if the frame will be complete after processing the estimated block */
    if(total_samples == 0)
        s->parsed_all_subframes = 1;


    av_log(s->avctx, AV_LOG_DEBUG,"subframe is part of %i channels\n",s->channels_for_cur_subframe);

    /** configure the decoder for the current subframe */
    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];

        if(s->channel[c].num_subframes <= 1){
          s->cValidBarkBand = s->num_sfb[0];
          s->rgiBarkIndex = s->sfb_offsets;
          s->subwoofer_cutoff = s->subwoofer_cutoffs[0];
        }else{
          int frame_offset = av_log2(s->samples_per_frame/s->channel[c].subframe_len[s->channel[c].cur_subframe]);
          s->cValidBarkBand = s->num_sfb[frame_offset];
          s->rgiBarkIndex = &s->sfb_offsets[MAX_BANDS * frame_offset];
          s->subwoofer_cutoff = s->subwoofer_cutoffs[frame_offset];
        }

        /** init some things if this is the first subframe */
        if(!s->channel[c].cur_subframe){
              s->channel[c].scale_factor_step = 1;
              s->channel[c].max_scale_factor = 0;
              memset(s->channel[c].scale_factors, 0, sizeof(s->channel[c].scale_factors));
              memset(s->channel[c].resampled_scale_factors, 0, sizeof(s->channel[c].resampled_scale_factors));
        }

    }

    s->subframe_len = subframe_len;

    /** skip extended header if any */
    if(get_bits(gb,1)){
        int num_fill_bits;
        if(!(num_fill_bits = get_bits(gb,2))){
            num_fill_bits = get_bits(gb,4);
            num_fill_bits = get_bits(gb,num_fill_bits) + 1;
        }

        if(num_fill_bits >= 0){
            skip_bits(gb,num_fill_bits);
        }
    }

    /** no idea for what the following bit is used */
    if(get_bits(gb,1)){
        av_log(s->avctx,AV_LOG_ERROR,"reserved bit set\n");
        return 0;
    }


    if(!wma_decode_channel_transform(s,gb))
        return 0;


    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];
        if((s->channel[c].transmit_coefs = get_bits(gb,1)))
            transmit_coeffs = 1;
    }

    s->quant_step = 90 * s->sample_bit_depth / 16;

    if(transmit_coeffs){
        int quant;
        int sign = 1;
        int large_quant = 0;
        if((get_bits(gb,1))){ /** FIXME might influence how often getvec4 may be called */
            av_log(s->avctx,AV_LOG_ERROR,"unsupported quant step coding\n");
            return 0;
        }
        /** decode quantization step */
        quant = get_bits(gb,6);
        if(quant & 0x20){
            quant |= 0xFFFFFFC0u;
            sign = -1;
        }
        s->quant_step += quant;
        if(quant <= -32 || quant > 30)
            large_quant = 1;
        while(large_quant){
            quant = get_bits(gb,5);
            if(quant != 31){
                s->quant_step += quant * sign;
                break;
            }
            s->quant_step += 31 * sign;
            if(s->quant_step < 0){
                s->negative_quantstep = 1;
                av_log(s->avctx,AV_LOG_ERROR,"negative quant step\n");
                return 0;
            }
        }

        /** decode quantization step modifiers for every channel */

        if(s->channels_for_cur_subframe == 1)
            s->channel[s->channel_indices_for_cur_subframe[0]].quant_step_modifier = 0;
        else{
            int modifier_len = get_bits(gb,3);
            for(i=0;i<s->channels_for_cur_subframe;i++){
                int c = s->channel_indices_for_cur_subframe[i];
                s->channel[c].quant_step_modifier = 0;
                if(get_bits(gb,1)){
                    if(modifier_len)
                        s->channel[c].quant_step_modifier = get_bits(gb,modifier_len) + 1;
                    else
                        s->channel[c].quant_step_modifier = 1;
                }else
                    s->channel[c].quant_step_modifier = 0;

            }
        }

        /** decode scale factors */
        if(!wma_decode_scale_factors(s,gb))
            return 0;
    }

    av_log(s->avctx,AV_LOG_DEBUG,"BITSTREAM: subframe header length was %i\n",get_bits_count(gb) - bitstreamcounter);

    /** parse coefficients */
    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];
        if(s->channel[c].transmit_coefs)
                decode_coeffs(s,gb,c);
    }

    av_log(s->avctx,AV_LOG_DEBUG,"BITSTREAM: subframe length was %i\n",get_bits_count(gb) - bitstreamcounter);

    if(transmit_coeffs){
        wma_inverse_channel_transform(s);
        for(i=0;i<s->channels_for_cur_subframe;i++){
            int c = s->channel_indices_for_cur_subframe[i];
            int b;
            float* dst;
            if(c == s->lfe_channel)
                memset(&s->channel[c].coeffs[s->subwoofer_cutoff],0,4 * (subframe_len - s->subwoofer_cutoff));

            /** inverse quantization */
            for(b=0;b<s->cValidBarkBand;b++){
                int start = s->rgiBarkIndex[b];
                int end = s->rgiBarkIndex[b+1];
                int min;
                float quant;
                if(end > s->subframe_len)
                    end = s->subframe_len;

                if(s->channel[c].transmit_sf)
                     min = s->channel[c].scale_factor_step * (s->channel[c].max_scale_factor - s->channel[c].scale_factors[b]);
                else
                     min = s->channel[c].scale_factor_step * (s->channel[c].max_scale_factor - s->channel[c].resampled_scale_factors[b]);
                quant = pow(10.0,(s->quant_step + s->channel[c].quant_step_modifier - min) / 20.0);
                while(start < end){
                    s->channel[c].coeffs[start] *= quant;
                    ++start;
                }
            }

            dst = &s->channel[c].out[s->samples_per_frame/2  + s->channel[c].subframe_offset[s->channel[c].cur_subframe]];
            ff_imdct_half(&s->mdct_ctx[av_log2(subframe_len)-BLOCK_MIN_BITS], dst, s->channel[c].coeffs); //DCTIV  with reverse
            for(b=0;b<subframe_len;b++)
                dst[b] /= subframe_len / 2;     //FIXME try to remove this scaling
        }
    }

    wma_window(s);

    /** handled one subframe */
    for(i=0;i<s->channels_for_cur_subframe;i++){
        int c = s->channel_indices_for_cur_subframe[i];
        if(s->channel[c].cur_subframe >= s->channel[c].num_subframes){
            av_log(s->avctx,AV_LOG_ERROR,"broken subframe\n");
            return 0;
        }
        ++s->channel[c].cur_subframe;
    }

    return 1;
}

/**
 *@brief Decode one WMA frame.
 *@param s context
 *@param gb current get bit context
 *@return 0 if the trailer bit indicates that this is the last frame,
 *        1 if there are more frames
 */
static int wma_decode_frame(WMA3DecodeContext *s,GetBitContext* gb){
    unsigned int gb_start_count = get_bits_count(gb);
    int more_frames = 0;
    int len = 0;
    int i;

    /** check for potential output buffer overflow */
    if(s->samples + s->nb_channels * s->samples_per_frame > s->samples_end){
        av_log(s->avctx,AV_LOG_ERROR,"not enough space for the output samples\n");
        s->packet_loss = 1;
        return 0;
    }

    s->negative_quantstep = 0;
    bitstreamcounter = get_bits_count(gb);

    /** get frame length */
    if(s->len_prefix)
        len = get_bits(gb,s->log2_frame_size);

    av_log(s->avctx,AV_LOG_DEBUG,"decoding frame with len %x\n",len);

    /** decode tile information */
    if(wma_decode_tilehdr(s,gb)){
        s->packet_loss = 1;
        return 0;
    }

    /** read postproc transform */
    if(s->nb_channels > 1 && get_bits1(gb)){
        av_log(s->avctx,AV_LOG_ERROR,"Unsupported postproc transform found\n");
        s->packet_loss = 1;
        return 0;
    }

    /** read drc info */
    if(s->dynamic_range_compression){
        s->drc_gain = get_bits(gb,8);
        av_log(s->avctx,AV_LOG_DEBUG,"drc_gain %i\n",s->drc_gain);
    }

    /** no idea what these are for, might be the number of samples
        that need to be skipped at the beginning or end of a stream */
    if(get_bits(gb,1)){
        int skip;

        /** usually true for the first frame */
        if(get_bits1(gb)){
            skip = get_bits(gb,av_log2(s->samples_per_frame * 2));
            av_log(s->avctx,AV_LOG_DEBUG,"start skip: %i\n",skip);
        }

        /** sometimes true for the last frame */
        if(get_bits(gb,1)){
            skip = get_bits(gb,av_log2(s->samples_per_frame * 2));
            av_log(s->avctx,AV_LOG_DEBUG,"end skip: %i\n",skip);
        }

    }

    av_log(s->avctx,AV_LOG_DEBUG,"BITSTREAM: frame header length was %i\n",get_bits_count(gb) - bitstreamcounter);

    /** reset subframe states */
    s->parsed_all_subframes = 0;
    for(i=0;i<s->nb_channels;i++){
        s->channel[i].decoded_samples = 0;
        s->channel[i].cur_subframe = 0;
        s->channel[i].reuse_sf = 0;
    }

    /** parse all subframes */
    while(!s->parsed_all_subframes){
        if(!wma_decode_subframe(s,gb)){
            s->packet_loss = 1;
            return 0;
        }
    }

    /** convert samples to short and write them to the output buffer */
    for(i = 0; i < s->nb_channels; i++) {
        int16_t* ptr;
        int incr = s->nb_channels;
        /* FIXME what about other channel layouts? */
        const char layout[] = {0,1,4,5,2,3};
        int chpos;
        float* iptr = s->channel[i].out;
        int x;

        if(s->nb_channels == 6){
              chpos = layout[i];
        }else
              chpos = i;

        ptr = s->samples + chpos;

        for(x=0;x<s->samples_per_frame;x++) {
            *ptr = av_clip_int16(lrintf(*iptr++));
            ptr += incr;
        }

        /** reuse second half of the imdct output for the next frame */
        memmove(&s->channel[i].out[0], &s->channel[i].out[s->samples_per_frame],
                s->samples_per_frame * sizeof(float));
    }

    if(s->skip_frame)
        s->skip_frame = 0;
    else
        s->samples += s->nb_channels * s->samples_per_frame;


    //FIXME remove
    av_log(s->avctx,AV_LOG_DEBUG,"frame[%i] skipping %i bits\n",s->frame_num,len - (get_bits_count(gb) - gb_start_count) - 1);
    if(len - (get_bits_count(gb) - gb_start_count) - 1 != 1)
        assert(0);

    /** skip the rest of the frame data */
    skip_bits_long(gb,len - (get_bits_count(gb) - gb_start_count) - 1);

    /** decode trailer bit */
    more_frames = get_bits1(gb);

    ++s->frame_num;
    return more_frames;
}


/**
 *@brief Calculate remaining input buffer length.
 *@param s codec context
 *@return remaining size in bits
 */
static int remaining_bits(WMA3DecodeContext *s){
    return s->buf_bit_size - get_bits_count(&s->gb);
}

/**
 *@brief Fill the bit reservoir with a partial frame.
 *@param s codec context
 *@param len length of the partial frame
 */
static void save_bits(WMA3DecodeContext *s,int len){
    int buflen = (s->prev_frame_bit_size + len + 8) / 8;
    int bit_offset = s->prev_frame_bit_size % 8;
    int pos = (s->prev_frame_bit_size - bit_offset) / 8;
    s->prev_frame_bit_size += len;

    if(len <= 0)
         return;

    /** increase length if needed */
    s->prev_frame = av_realloc(s->prev_frame,buflen +
                               FF_INPUT_BUFFER_PADDING_SIZE);

    /** byte align prev_frame buffer */
    if(bit_offset){
        int missing = 8 - bit_offset;
        if(len < missing)
            missing = len;
        s->prev_frame[pos++] |=
            get_bits(&s->gb, missing) << (8 - bit_offset - missing);
        len -= missing;
    }

    /** copy full bytes */
    while(len > 7){
        s->prev_frame[pos++] = get_bits(&s->gb,8);
        len -= 8;
    }

    /** copy remaining bits */
    if(len > 0)
        s->prev_frame[pos++] = get_bits(&s->gb,len) << (8 - len);
}

/**
 *@brief Decode a single WMA packet.
 *@param avctx codec context
 *@param data the output buffer
 *@param data_size number of bytes that were written to the output buffer
 *@param buf input buffer
 *@param buf_size input buffer length
 *@return number of bytes that were read from the input buffer
 */
static int wma3_decode_packet(AVCodecContext *avctx,
                             void *data, int *data_size,
                             const uint8_t *buf, int buf_size)
{
    WMA3DecodeContext *s = avctx->priv_data;
    int more_frames=1;
    int num_bits_prev_frame;

    s->samples = data;
    s->samples_end = (int16_t*)((int8_t*)data + *data_size);
    s->buf_bit_size = buf_size << 3;


    *data_size = 0;

    /** sanity check for the buffer length */
    if(buf_size < avctx->block_align)
        return 0;

    /** Parse packet header */
    init_get_bits(&s->gb, buf, s->buf_bit_size);
    s->packet_sequence_number = get_bits(&s->gb, 4);
    s->bit5                   = get_bits1(&s->gb);
    s->bit6                   = get_bits1(&s->gb);

    /** get number of bits that need to be added to the previous frame */
    num_bits_prev_frame = get_bits(&s->gb, s->log2_frame_size);
    av_log(avctx, AV_LOG_DEBUG, "packet[%d]: nbpf %x\n", avctx->frame_number,
                  num_bits_prev_frame);

    /** check for packet loss */
    if (s->packet_sequence_number != (avctx->frame_number&0xF)) {
        s->packet_loss = 1;
        av_log(avctx, AV_LOG_ERROR, "!!Packet loss detected! seq %x vs %x\n",
                      s->packet_sequence_number,avctx->frame_number&0xF);
    }

    if (num_bits_prev_frame > 0) {
        /** append the prev frame data to the remaining data from the
            previous packet to create a full frame */
        save_bits(s,num_bits_prev_frame);
        av_log(avctx, AV_LOG_DEBUG, "accumulated %x bits of frame data\n",
                      s->prev_frame_bit_size);

        /** decode the cross packet frame if it is valid */
        if(!s->packet_loss){
            GetBitContext gb_prev;
            init_get_bits(&gb_prev, s->prev_frame, s->prev_frame_bit_size);
            wma_decode_frame(s,&gb_prev);
        }
    }else if(s->prev_frame_bit_size){
        av_log(avctx, AV_LOG_ERROR, "ignoring %x previously saved bits\n",
                      s->prev_frame_bit_size);
    }

    /** reset prev frame buffer */
    s->prev_frame_bit_size = 0;
    s->packet_loss = 0;
    /** decode the rest of the packet */
    while(more_frames && remaining_bits(s) > s->log2_frame_size){
        int frame_size = show_bits(&s->gb, s->log2_frame_size);

        /** there is enough data for a full frame */
        if(remaining_bits(s) >= frame_size){

            /** decode the frame */
            more_frames = wma_decode_frame(s,&s->gb);

            if(!more_frames){
                av_log(avctx, AV_LOG_ERROR, "no more frames\n");
            }
        }else
            more_frames = 0;
    }

    if(s->packet_loss == 1 && !s->negative_quantstep)
        assert(0);

    /** save the rest of the data so that it can be decoded
       with the next packet */
    save_bits(s,remaining_bits(s));

    *data_size = (int8_t *)s->samples - (int8_t *)data;

    return avctx->block_align;
}


/**
 *@brief WMA9 decoder
 */
AVCodec wmapro_decoder =
{
    "wmapro",
    CODEC_TYPE_AUDIO,
    CODEC_ID_WMAPRO,
    sizeof(WMA3DecodeContext),
    wma3_decode_init,
    NULL,
    wma3_decode_end,
    wma3_decode_packet,
    .long_name = NULL_IF_CONFIG_SMALL("Windows Media Audio 9 Professional"),
};
