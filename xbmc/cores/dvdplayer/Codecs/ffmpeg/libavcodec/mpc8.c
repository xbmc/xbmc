/*
 * Musepack SV8 decoder
 * Copyright (c) 2007 Konstantin Shishkov
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
 * @file mpc8.c Musepack SV8 decoder
 * MPEG Audio Layer 1/2 -like codec with frames of 1152 samples
 * divided into 32 subbands.
 */

#include "libavutil/random.h"
#include "avcodec.h"
#include "bitstream.h"
#include "dsputil.h"

#ifdef CONFIG_MPEGAUDIO_HP
#define USE_HIGHPRECISION
#endif
#include "mpegaudio.h"

#include "mpc.h"
#include "mpcdata.h"
#include "mpc8data.h"
#include "mpc8huff.h"

static VLC band_vlc, scfi_vlc[2], dscf_vlc[2], res_vlc[2];
static VLC q1_vlc, q2_vlc[2], q3_vlc[2], quant_vlc[4][2], q9up_vlc;

static const int q3_offsets[2] = { MPC8_Q3_OFFSET, MPC8_Q4_OFFSET };
static const int quant_offsets[6] = { MPC8_Q5_OFFSET, MPC8_Q6_OFFSET, MPC8_Q7_OFFSET, MPC8_Q8_OFFSET };

static inline int mpc8_dec_base(GetBitContext *gb, int k, int n)
{
    int code = get_bits(gb, mpc8_cnk_len[k-1][n-1] - 1);

    if (code >= mpc8_cnk_lost[k-1][n-1])
        code = ((code << 1) | get_bits1(gb)) - mpc8_cnk_lost[k-1][n-1];

    return code;
}

static inline int mpc8_dec_enum(GetBitContext *gb, int k, int n)
{
    int bits = 0;
    const uint32_t * C = mpc8_cnk[k-1];
    int code = mpc8_dec_base(gb, k, n);

    do {
        n--;
        if (code >= C[n]) {
            bits |= 1 << n;
            code -= C[n];
            C -= 32;
            k--;
        }
    } while(k > 0);

    return bits;
}

static inline int mpc8_get_mod_golomb(GetBitContext *gb, int m)
{
    if(mpc8_cnk_len[0][m] < 1) return 0;
    return mpc8_dec_base(gb, 1, m+1);
}

static int mpc8_get_mask(GetBitContext *gb, int size, int t)
{
    int mask = 0;

    if(t && t != size)
         mask = mpc8_dec_enum(gb, FFMIN(t, size - t), size);
    if((t << 1) > size) mask = ~mask;

    return mask;
}

static av_cold int mpc8_decode_init(AVCodecContext * avctx)
{
    int i;
    MPCContext *c = avctx->priv_data;
    GetBitContext gb;
    static int vlc_initialized = 0;

    if(avctx->extradata_size < 2){
        av_log(avctx, AV_LOG_ERROR, "Too small extradata size (%i)!\n", avctx->extradata_size);
        return -1;
    }
    memset(c->oldDSCF, 0, sizeof(c->oldDSCF));
    av_init_random(0xDEADBEEF, &c->rnd);
    dsputil_init(&c->dsp, avctx);

    ff_mpc_init();

    init_get_bits(&gb, avctx->extradata, 16);

    skip_bits(&gb, 3);//sample rate
    c->maxbands = get_bits(&gb, 5) + 1;
    skip_bits(&gb, 4);//channels
    c->MSS = get_bits1(&gb);
    c->frames = 1 << (get_bits(&gb, 3) * 2);

    if(vlc_initialized) return 0;
    av_log(avctx, AV_LOG_DEBUG, "Initing VLC\n");

    init_vlc(&band_vlc, MPC8_BANDS_BITS, MPC8_BANDS_SIZE,
             mpc8_bands_bits,  1, 1,
             mpc8_bands_codes, 1, 1, INIT_VLC_USE_STATIC);

    init_vlc(&q1_vlc, MPC8_Q1_BITS, MPC8_Q1_SIZE,
             mpc8_q1_bits,  1, 1,
             mpc8_q1_codes, 1, 1, INIT_VLC_USE_STATIC);
    init_vlc(&q9up_vlc, MPC8_Q9UP_BITS, MPC8_Q9UP_SIZE,
             mpc8_q9up_bits,  1, 1,
             mpc8_q9up_codes, 1, 1, INIT_VLC_USE_STATIC);

    init_vlc(&scfi_vlc[0], MPC8_SCFI0_BITS, MPC8_SCFI0_SIZE,
             mpc8_scfi0_bits,  1, 1,
             mpc8_scfi0_codes, 1, 1, INIT_VLC_USE_STATIC);
    init_vlc(&scfi_vlc[1], MPC8_SCFI1_BITS, MPC8_SCFI1_SIZE,
             mpc8_scfi1_bits,  1, 1,
             mpc8_scfi1_codes, 1, 1, INIT_VLC_USE_STATIC);

    init_vlc(&dscf_vlc[0], MPC8_DSCF0_BITS, MPC8_DSCF0_SIZE,
             mpc8_dscf0_bits,  1, 1,
             mpc8_dscf0_codes, 1, 1, INIT_VLC_USE_STATIC);
    init_vlc(&dscf_vlc[1], MPC8_DSCF1_BITS, MPC8_DSCF1_SIZE,
             mpc8_dscf1_bits,  1, 1,
             mpc8_dscf1_codes, 1, 1, INIT_VLC_USE_STATIC);

    init_vlc_sparse(&q3_vlc[0], MPC8_Q3_BITS, MPC8_Q3_SIZE,
             mpc8_q3_bits,  1, 1,
             mpc8_q3_codes, 1, 1,
             mpc8_q3_syms,  1, 1, INIT_VLC_USE_STATIC);
    init_vlc_sparse(&q3_vlc[1], MPC8_Q4_BITS, MPC8_Q4_SIZE,
             mpc8_q4_bits,  1, 1,
             mpc8_q4_codes, 1, 1,
             mpc8_q4_syms,  1, 1, INIT_VLC_USE_STATIC);

    for(i = 0; i < 2; i++){
        init_vlc(&res_vlc[i], MPC8_RES_BITS, MPC8_RES_SIZE,
                 &mpc8_res_bits[i],  1, 1,
                 &mpc8_res_codes[i], 1, 1, INIT_VLC_USE_STATIC);

        init_vlc(&q2_vlc[i], MPC8_Q2_BITS, MPC8_Q2_SIZE,
                 &mpc8_q2_bits[i],  1, 1,
                 &mpc8_q2_codes[i], 1, 1, INIT_VLC_USE_STATIC);

        init_vlc(&quant_vlc[0][i], MPC8_Q5_BITS, MPC8_Q5_SIZE,
                 &mpc8_q5_bits[i],  1, 1,
                 &mpc8_q5_codes[i], 1, 1, INIT_VLC_USE_STATIC);
        init_vlc(&quant_vlc[1][i], MPC8_Q6_BITS, MPC8_Q6_SIZE,
                 &mpc8_q6_bits[i],  1, 1,
                 &mpc8_q6_codes[i], 1, 1, INIT_VLC_USE_STATIC);
        init_vlc(&quant_vlc[2][i], MPC8_Q7_BITS, MPC8_Q7_SIZE,
                 &mpc8_q7_bits[i],  1, 1,
                 &mpc8_q7_codes[i], 1, 1, INIT_VLC_USE_STATIC);
        init_vlc(&quant_vlc[3][i], MPC8_Q8_BITS, MPC8_Q8_SIZE,
                 &mpc8_q8_bits[i],  1, 1,
                 &mpc8_q8_codes[i], 1, 1, INIT_VLC_USE_STATIC);
    }
    vlc_initialized = 1;
    avctx->sample_fmt = SAMPLE_FMT_S16;
    avctx->channel_layout = (avctx->channels==2) ? CH_LAYOUT_STEREO : CH_LAYOUT_MONO;
    return 0;
}

static int mpc8_decode_frame(AVCodecContext * avctx,
                            void *data, int *data_size,
                            const uint8_t * buf, int buf_size)
{
    MPCContext *c = avctx->priv_data;
    GetBitContext gb2, *gb = &gb2;
    int i, j, k, ch, cnt, res, t;
    Band *bands = c->bands;
    int off;
    int maxband, keyframe;
    int last[2];

    keyframe = c->cur_frame == 0;

    if(keyframe){
        memset(c->Q, 0, sizeof(c->Q));
        c->last_bits_used = 0;
    }
    init_get_bits(gb, buf, buf_size * 8);
    skip_bits(gb, c->last_bits_used & 7);

    if(keyframe)
        maxband = mpc8_get_mod_golomb(gb, c->maxbands + 1);
    else{
        maxband = c->last_max_band + get_vlc2(gb, band_vlc.table, MPC8_BANDS_BITS, 2);
        if(maxband > 32) maxband -= 33;
    }
    c->last_max_band = maxband;

    /* read subband indexes */
    if(maxband){
        last[0] = last[1] = 0;
        for(i = maxband - 1; i >= 0; i--){
            for(ch = 0; ch < 2; ch++){
                last[ch] = get_vlc2(gb, res_vlc[last[ch] > 2].table, MPC8_RES_BITS, 2) + last[ch];
                if(last[ch] > 15) last[ch] -= 17;
                bands[i].res[ch] = last[ch];
            }
        }
        if(c->MSS){
            int mask;

            cnt = 0;
            for(i = 0; i < maxband; i++)
                if(bands[i].res[0] || bands[i].res[1])
                    cnt++;
            t = mpc8_get_mod_golomb(gb, cnt);
            mask = mpc8_get_mask(gb, cnt, t);
            for(i = maxband - 1; i >= 0; i--)
                if(bands[i].res[0] || bands[i].res[1]){
                    bands[i].msf = mask & 1;
                    mask >>= 1;
                }
        }
    }
    for(i = maxband; i < c->maxbands; i++)
        bands[i].res[0] = bands[i].res[1] = 0;

    if(keyframe){
        for(i = 0; i < 32; i++)
            c->oldDSCF[0][i] = c->oldDSCF[1][i] = 1;
    }

    for(i = 0; i < maxband; i++){
        if(bands[i].res[0] || bands[i].res[1]){
            cnt = !!bands[i].res[0] + !!bands[i].res[1] - 1;
            if(cnt >= 0){
                t = get_vlc2(gb, scfi_vlc[cnt].table, scfi_vlc[cnt].bits, 1);
                if(bands[i].res[0]) bands[i].scfi[0] = t >> (2 * cnt);
                if(bands[i].res[1]) bands[i].scfi[1] = t & 3;
            }
        }
    }

    for(i = 0; i < maxband; i++){
        for(ch = 0; ch < 2; ch++){
            if(!bands[i].res[ch]) continue;

            if(c->oldDSCF[ch][i]){
                bands[i].scf_idx[ch][0] = get_bits(gb, 7) - 6;
                c->oldDSCF[ch][i] = 0;
            }else{
                t = get_vlc2(gb, dscf_vlc[1].table, MPC8_DSCF1_BITS, 2);
                if(t == 64)
                    t += get_bits(gb, 6);
                bands[i].scf_idx[ch][0] = ((bands[i].scf_idx[ch][2] + t - 25) & 0x7F) - 6;
            }
            for(j = 0; j < 2; j++){
                if((bands[i].scfi[ch] << j) & 2)
                    bands[i].scf_idx[ch][j + 1] = bands[i].scf_idx[ch][j];
                else{
                    t = get_vlc2(gb, dscf_vlc[0].table, MPC8_DSCF0_BITS, 2);
                    if(t == 31)
                        t = 64 + get_bits(gb, 6);
                    bands[i].scf_idx[ch][j + 1] = ((bands[i].scf_idx[ch][j] + t - 25) & 0x7F) - 6;
                }
            }
        }
    }

    for(i = 0, off = 0; i < maxband; i++, off += SAMPLES_PER_BAND){
        for(ch = 0; ch < 2; ch++){
            res = bands[i].res[ch];
            switch(res){
            case -1:
                for(j = 0; j < SAMPLES_PER_BAND; j++)
                    c->Q[ch][off + j] = (av_random(&c->rnd) & 0x3FC) - 510;
                break;
            case 0:
                break;
            case 1:
                for(j = 0; j < SAMPLES_PER_BAND; j += SAMPLES_PER_BAND / 2){
                    cnt = get_vlc2(gb, q1_vlc.table, MPC8_Q1_BITS, 2);
                    t = mpc8_get_mask(gb, 18, cnt);
                    for(k = 0; k < SAMPLES_PER_BAND / 2; k++, t <<= 1)
                        c->Q[ch][off + j + k] = (t & 0x20000) ? (get_bits1(gb) << 1) - 1 : 0;
                }
                break;
            case 2:
                cnt = 6;//2*mpc8_thres[res]
                for(j = 0; j < SAMPLES_PER_BAND; j += 3){
                    t = get_vlc2(gb, q2_vlc[cnt > 3].table, MPC8_Q2_BITS, 2);
                    c->Q[ch][off + j + 0] = mpc8_idx50[t];
                    c->Q[ch][off + j + 1] = mpc8_idx51[t];
                    c->Q[ch][off + j + 2] = mpc8_idx52[t];
                    cnt = (cnt >> 1) + mpc8_huffq2[t];
                }
                break;
            case 3:
            case 4:
                for(j = 0; j < SAMPLES_PER_BAND; j += 2){
                    t = get_vlc2(gb, q3_vlc[res - 3].table, MPC8_Q3_BITS, 2) + q3_offsets[res - 3];
                    c->Q[ch][off + j + 1] = t >> 4;
                    c->Q[ch][off + j + 0] = (t & 8) ? (t & 0xF) - 16 : (t & 0xF);
                }
                break;
            case 5:
            case 6:
            case 7:
            case 8:
                cnt = 2 * mpc8_thres[res];
                for(j = 0; j < SAMPLES_PER_BAND; j++){
                    t = get_vlc2(gb, quant_vlc[res - 5][cnt > mpc8_thres[res]].table, quant_vlc[res - 5][cnt > mpc8_thres[res]].bits, 2) + quant_offsets[res - 5];
                    c->Q[ch][off + j] = t;
                    cnt = (cnt >> 1) + FFABS(c->Q[ch][off + j]);
                }
                break;
            default:
                for(j = 0; j < SAMPLES_PER_BAND; j++){
                    c->Q[ch][off + j] = get_vlc2(gb, q9up_vlc.table, MPC8_Q9UP_BITS, 2);
                    if(res != 9){
                        c->Q[ch][off + j] <<= res - 9;
                        c->Q[ch][off + j] |= get_bits(gb, res - 9);
                    }
                    c->Q[ch][off + j] -= (1 << (res - 2)) - 1;
                }
            }
        }
    }

    ff_mpc_dequantize_and_synth(c, maxband, data);

    c->cur_frame++;

    c->last_bits_used = get_bits_count(gb);
    if(c->cur_frame >= c->frames)
        c->cur_frame = 0;
    *data_size =  MPC_FRAME_SIZE * 4;

    return c->cur_frame ? c->last_bits_used >> 3 : buf_size;
}

AVCodec mpc8_decoder = {
    "mpc8",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MUSEPACK8,
    sizeof(MPCContext),
    mpc8_decode_init,
    NULL,
    NULL,
    mpc8_decode_frame,
    .long_name = NULL_IF_CONFIG_SMALL("Musepack SV8"),
};
