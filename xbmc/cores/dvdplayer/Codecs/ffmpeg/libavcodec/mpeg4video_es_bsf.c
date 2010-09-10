/*
 * Copyright (c) 2009 Google Inc.
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
#include "mpeg4video.h"

static int mpeg4video_es_filter(AVBitStreamFilterContext *bsfc,
				AVCodecContext *avctx, const char *args,
				uint8_t **poutbuf, int *poutbuf_size,
				const uint8_t *buf, int buf_size,
				int keyframe) {
    AVCodecParserContext *cpc;
    ParseContext1 *pc;
    MpegEncContext *s;
    int first_picture;
    int ret;
    uint8_t *frame_data;
    int frame_size;
    int outbuf_idx = 0;
    int count = 0;

    if (avctx->codec_id != CODEC_ID_MPEG4) {
        av_log(NULL, AV_LOG_ERROR, "Codec is not MPEG4\n");
        return -1;
    }

    if (!bsfc->parser) {
        bsfc->parser = av_parser_init(CODEC_ID_MPEG4);
    }
    cpc = bsfc->parser;
    pc = cpc->priv_data;
    s = pc->enc;

    *poutbuf = NULL;
    *poutbuf_size = 0;
    while (buf_size > 0) {
        first_picture = pc->first_picture;
        ret = cpc->parser->parser_parse(cpc, avctx, &frame_data, &frame_size, buf, buf_size);

        if (ret < 0)
            return ret;

        buf_size -= ret;
        buf += ret;

        // If the first picture is decoded, encode the header.
        if (first_picture && !pc->first_picture) {
            assert(!*poutbuf);
            *poutbuf = av_malloc(1024);
            *poutbuf_size = 1024;
            init_put_bits(&s->pb, *poutbuf, 1024);
            mpeg4_encode_visual_object_header(s);
            mpeg4_encode_vol_header(s, 0, 0);
            flush_put_bits(&s->pb);
            outbuf_idx = (put_bits_count(&s->pb)+7)>>3;
        }

        if (!frame_size)
            break;

        *poutbuf = av_fast_realloc(*poutbuf, poutbuf_size, outbuf_idx + frame_size);
        memcpy(*poutbuf + outbuf_idx, frame_data, frame_size);
        outbuf_idx += frame_size;
    }

    *poutbuf_size = outbuf_idx;
    return 0;
}

AVBitStreamFilter mpeg4video_es_bsf = {
    "mpeg4video_es",
    0,
    mpeg4video_es_filter,
};
