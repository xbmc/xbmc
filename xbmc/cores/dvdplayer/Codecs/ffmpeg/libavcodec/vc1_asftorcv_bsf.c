/*
 * copyright (c) 2010 Google Inc.
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
#include "bytestream.h"

#define RCV_STREAM_HEADER_SIZE 36
#define RCV_PICTURE_HEADER_SIZE 8

typedef struct ASFTORCVBSFContext {
    int frames;
} ASFTORCVBSFContext;

static int asftorcv_filter(AVBitStreamFilterContext *bsfc, AVCodecContext *avctx, const char *args,
                          uint8_t **poutbuf, int *poutbuf_size,
                          const uint8_t *buf, int buf_size, int keyframe){
    ASFTORCVBSFContext* ctx = (ASFTORCVBSFContext*)bsfc->priv_data;

    if (avctx->codec_id != CODEC_ID_WMV3) {
        av_log(avctx, AV_LOG_ERROR, "Only WMV3 is accepted!\n");
        return -1;
    }

    uint8_t* bs = NULL;
    if (!ctx->frames) {
        // Write the header if this is the first frame.
        *poutbuf = av_malloc(RCV_STREAM_HEADER_SIZE + RCV_PICTURE_HEADER_SIZE + buf_size);
        *poutbuf_size = RCV_STREAM_HEADER_SIZE + RCV_PICTURE_HEADER_SIZE + buf_size;
        bs = *poutbuf;

        // The following structure of stream header comes from libavformat/vc1testenc.c.
        bytestream_put_le24(&bs, 0);  // Frame count. 0 for streaming.
        bytestream_put_byte(&bs, 0xC5);
        bytestream_put_le32(&bs, 4);  // 4 bytes of extra data.
        bytestream_put_byte(&bs, avctx->extradata[0]);
        bytestream_put_byte(&bs, avctx->extradata[1]);
        bytestream_put_byte(&bs, avctx->extradata[2]);
        bytestream_put_byte(&bs, avctx->extradata[3]);
        bytestream_put_le32(&bs, avctx->height);
        bytestream_put_le32(&bs, avctx->width);
        bytestream_put_le32(&bs, 0xC);
        bytestream_put_le24(&bs, 0);  // hrd_buffer
        bytestream_put_byte(&bs, 0x80);  // level|cbr|res1
        bytestream_put_le32(&bs, 0);  // hrd_rate

        // The following LE32 describes the frame rate. Since we don't care so fill
        // it with 0xFFFFFFFF which means variable framerate.
        // See: libavformat/vc1testenc.c
        bytestream_put_le32(&bs, 0xFFFFFFFF);
    } else {
        *poutbuf = av_malloc(RCV_PICTURE_HEADER_SIZE + buf_size);
        *poutbuf_size = RCV_PICTURE_HEADER_SIZE + buf_size;
        bs = *poutbuf;
    }

    // Write the picture header.
    bytestream_put_le32(&bs, buf_size | (keyframe ? 0x80000000 : 0));

    //  The following LE32 describes the pts. Since we don't care so fill it with 0.
    bytestream_put_le32(&bs, 0);
    memcpy(bs, buf, buf_size);

    ++ctx->frames;
    return 0;
}

AVBitStreamFilter vc1_asftorcv_bsf = {
    "vc1_asftorcv",
    sizeof(ASFTORCVBSFContext),
    asftorcv_filter,
};
