/*
 * AVS video decoder.
 * Copyright (c) 2006  Aurelien Jacobs <aurel@gnuage.org>
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
#include "get_bits.h"


typedef struct {
    AVFrame picture;
} AvsContext;

typedef enum {
    AVS_VIDEO     = 0x01,
    AVS_AUDIO     = 0x02,
    AVS_PALETTE   = 0x03,
    AVS_GAME_DATA = 0x04,
} AvsBlockType;

typedef enum {
    AVS_I_FRAME     = 0x00,
    AVS_P_FRAME_3X3 = 0x01,
    AVS_P_FRAME_2X2 = 0x02,
    AVS_P_FRAME_2X3 = 0x03,
} AvsVideoSubType;


static int
avs_decode_frame(AVCodecContext * avctx,
                 void *data, int *data_size, AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    const uint8_t *buf_end = avpkt->data + avpkt->size;
    int buf_size = avpkt->size;
    AvsContext *const avs = avctx->priv_data;
    AVFrame *picture = data;
    AVFrame *const p = (AVFrame *) & avs->picture;
    const uint8_t *table, *vect;
    uint8_t *out;
    int i, j, x, y, stride, vect_w = 3, vect_h = 3;
    AvsVideoSubType sub_type;
    AvsBlockType type;
    GetBitContext change_map;

    if (avctx->reget_buffer(avctx, p)) {
        av_log(avctx, AV_LOG_ERROR, "reget_buffer() failed\n");
        return -1;
    }
    p->reference = 3;
    p->pict_type = AV_PICTURE_TYPE_P;
    p->key_frame = 0;

    out = avs->picture.data[0];
    stride = avs->picture.linesize[0];

    if (buf_end - buf < 4)
        return AVERROR_INVALIDDATA;
    sub_type = buf[0];
    type = buf[1];
    buf += 4;

    if (type == AVS_PALETTE) {
        int first, last;
        uint32_t *pal = (uint32_t *) avs->picture.data[1];

        first = AV_RL16(buf);
        last = first + AV_RL16(buf + 2);
        if (first >= 256 || last > 256 || buf_end - buf < 4 + 4 + 3 * (last - first))
            return AVERROR_INVALIDDATA;
        buf += 4;
        for (i=first; i<last; i++, buf+=3) {
            pal[i] = (buf[0] << 18) | (buf[1] << 10) | (buf[2] << 2);
            pal[i] |= 0xFF << 24 | (pal[i] >> 6) & 0x30303;
        }

        sub_type = buf[0];
        type = buf[1];
        buf += 4;
    }

    if (type != AVS_VIDEO)
        return -1;

    switch (sub_type) {
    case AVS_I_FRAME:
        p->pict_type = AV_PICTURE_TYPE_I;
        p->key_frame = 1;
    case AVS_P_FRAME_3X3:
        vect_w = 3;
        vect_h = 3;
        break;

    case AVS_P_FRAME_2X2:
        vect_w = 2;
        vect_h = 2;
        break;

    case AVS_P_FRAME_2X3:
        vect_w = 2;
        vect_h = 3;
        break;

    default:
      return -1;
    }

    if (buf_end - buf < 256 * vect_w * vect_h)
        return AVERROR_INVALIDDATA;
    table = buf + (256 * vect_w * vect_h);
    if (sub_type != AVS_I_FRAME) {
        int map_size = ((318 / vect_w + 7) / 8) * (198 / vect_h);
        if (buf_end - table < map_size)
            return AVERROR_INVALIDDATA;
        init_get_bits(&change_map, table, map_size * 8);
        table += map_size;
    }

    for (y=0; y<198; y+=vect_h) {
        for (x=0; x<318; x+=vect_w) {
            if (sub_type == AVS_I_FRAME || get_bits1(&change_map)) {
                if (buf_end - table < 1)
                    return AVERROR_INVALIDDATA;
                vect = &buf[*table++ * (vect_w * vect_h)];
                for (j=0; j<vect_w; j++) {
                    out[(y + 0) * stride + x + j] = vect[(0 * vect_w) + j];
                    out[(y + 1) * stride + x + j] = vect[(1 * vect_w) + j];
                    if (vect_h == 3)
                        out[(y + 2) * stride + x + j] =
                            vect[(2 * vect_w) + j];
                }
            }
        }
        if (sub_type != AVS_I_FRAME)
            align_get_bits(&change_map);
    }

    *picture = *(AVFrame *) & avs->picture;
    *data_size = sizeof(AVPicture);

    return buf_size;
}

static av_cold int avs_decode_init(AVCodecContext * avctx)
{
    AvsContext *const avs = avctx->priv_data;
    avctx->pix_fmt = PIX_FMT_PAL8;
    avcodec_get_frame_defaults(&avs->picture);
    return 0;
}

static av_cold int avs_decode_end(AVCodecContext *avctx)
{
    AvsContext *s = avctx->priv_data;
    if (s->picture.data[0])
        avctx->release_buffer(avctx, &s->picture);
    return 0;
}


AVCodec ff_avs_decoder = {
    .name           = "avs",
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = CODEC_ID_AVS,
    .priv_data_size = sizeof(AvsContext),
    .init           = avs_decode_init,
    .decode         = avs_decode_frame,
    .close          = avs_decode_end,
    .capabilities   = CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("AVS (Audio Video Standard) video"),
};
