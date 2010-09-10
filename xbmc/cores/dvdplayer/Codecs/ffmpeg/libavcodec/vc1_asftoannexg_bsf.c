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
#include "vc1.h"

// An arbitrary limit in bytes greater than the current bytes used.
#define MAX_SEQ_HEADER_SIZE 50

typedef struct ASFTOANNEXGBSFContext {
    int frames;
    uint8_t *seq_header;
    int seq_header_size;
    uint8_t *ep_header;
    int ep_header_size;
} ASFTOANNEXGBSFContext;

static int generate_sequence_header(VC1Context *v, ASFTOANNEXGBSFContext *ctx)
{
    PutBitContext pb;
    ctx->seq_header = av_mallocz(MAX_SEQ_HEADER_SIZE);
    init_put_bits(&pb, ctx->seq_header, MAX_SEQ_HEADER_SIZE);

    put_bits(&pb, 32, VC1_CODE_SEQHDR);    // start code
    put_bits(&pb, 2, PROFILE_ADVANCED);    // profile
    put_bits(&pb, 3, v->level);            // level
    put_bits(&pb, 2, v->chromaformat);     // chromaformat
    put_bits(&pb, 3, v->frmrtq_postproc);  // frmrtq postproc
    put_bits(&pb, 5, v->bitrtq_postproc);  // bitrtq postproc
    put_bits(&pb, 1, v->postprocflag);     // post proc flag
    put_bits(&pb, 12, (v->s.avctx->coded_width >> 1) - 1);   // codec width
    put_bits(&pb, 12, (v->s.avctx->coded_height >> 1) - 1);  // codec height
    put_bits(&pb, 1, v->broadcast);        // broadcast
    put_bits(&pb, 1, v->interlace);        // interlace
    put_bits(&pb, 1, v->tfcntrflag);       // tfcntrflag
    put_bits(&pb, 1, v->finterpflag);      // finterpflag
    put_bits(&pb, 1, 1);                   // reserved
    put_bits(&pb, 1, v->psf);              // progressive segmented frame mode
    put_bits(&pb, 1, 1);                   // has display info
    put_bits(&pb, 14, v->s.avctx->width - 1);                // width
    put_bits(&pb, 14, v->s.avctx->height - 1);               // height
    put_bits(&pb, 1, 0);                   // no aspect ratio
    put_bits(&pb, 1, 1);                   // frame rate
    put_bits(&pb, 8, 3);                   // fake it to be 30fps, see vc1.c and vc1data.c
    put_bits(&pb, 4, 1);                   // fake dr to be 1000, see vc1.c and vc1data.c
    put_bits(&pb, 1, 0);                   // no color info
    put_bits(&pb, 1, 0);                   // no hrd param

    align_put_bits(&pb);
    ctx->seq_header_size = (put_bits_count(&pb) + 7) >> 3;
    return 0;
}

static int decode_sequence_header(AVCodecContext *avctx, VC1Context *v, uint8_t *header, int header_size) {
    GetBitContext gb;
    uint8_t *buf = av_mallocz(header_size + FF_INPUT_BUFFER_PADDING_SIZE);
    int buf_size = vc1_unescape_buffer(header + 4, header_size - 4, buf);
    int ret;
    init_get_bits(&gb, buf, buf_size * 8);
    ret = vc1_decode_sequence_header(avctx, v, &gb);
    av_free(buf);
    return ret;
}

static int parse_extradata(AVCodecContext *avctx, ASFTOANNEXGBSFContext *ctx, uint8_t *extradata, int extradata_size) {
    const uint8_t *start = extradata;
    const uint8_t *end = extradata + extradata_size;
    const uint8_t *next;
    VC1Context vc1ctx;
    int size;
    int seq_ret;

    if(extradata_size < 16) {
        av_log(avctx, AV_LOG_ERROR, "Extradata size too small: %i\n", extradata_size);
        return -1;
    }

    start = find_next_marker(start, end);
    next = start;
    for(; next < end; start = next){
        next = find_next_marker(start + 4, end);
        size = next - start;
        if(size <= 0) continue;
        switch(AV_RB32(start)){
        case VC1_CODE_SEQHDR:
            memset(&vc1ctx, 0, sizeof(VC1Context));
            vc1ctx.profile = PROFILE_ADVANCED;
            vc1ctx.s.avctx = avctx;

            seq_ret = decode_sequence_header(avctx, &vc1ctx, start, size) < 0 ||
                      generate_sequence_header(&vc1ctx, ctx) < 0;
            if (seq_ret) {
                av_log(avctx, AV_LOG_ERROR, "Cannot regenerate sequence header\n");
                return -1;
            }
            break;
        case VC1_CODE_ENTRYPOINT:
            ctx->ep_header = av_malloc(size);
            ctx->ep_header_size = size;
            memcpy(ctx->ep_header, start, size);
            break;
        default:
            break;
        }
    }

    if(!ctx->seq_header || !ctx->ep_header) {
        av_log(avctx, AV_LOG_ERROR, "Incomplete extradata\n");
        return -1;
    }
    return 0;
}

static int asftoannexg_filter(AVBitStreamFilterContext *bsfc, AVCodecContext *avctx, const char *args,
                              uint8_t **poutbuf, int *poutbuf_size,
                              const uint8_t *buf, int buf_size, int keyframe){
    ASFTOANNEXGBSFContext* ctx = (ASFTOANNEXGBSFContext*)bsfc->priv_data;

    if (avctx->codec_id != CODEC_ID_VC1) {
        av_log(avctx, AV_LOG_ERROR, "Only VC1 Advanced profile is accepted!\n");
        return -1;
    }

    if (!ctx->frames && parse_extradata(avctx, ctx, avctx->extradata, avctx->extradata_size) < 0) {
        av_log(avctx, AV_LOG_ERROR, "Cannot parse extra data!\n");
        return -1;
    }

    uint8_t* bs;
    if (keyframe) {
        // If this is the keyframe, need to put sequence header and entry point header.
        *poutbuf_size = ctx->seq_header_size + ctx->ep_header_size + 4 + buf_size;
        *poutbuf = av_malloc(*poutbuf_size);
        bs = *poutbuf;

        memcpy(bs, ctx->seq_header, ctx->seq_header_size);
        bs += ctx->seq_header_size;
        memcpy(bs, ctx->ep_header, ctx->ep_header_size);
        bs += ctx->ep_header_size;
    } else {
        *poutbuf_size = 4 + buf_size;
        *poutbuf = av_malloc(*poutbuf_size);
        bs = *poutbuf;
    }

    // Put the frame start code and frame data.
    bytestream_put_be32(&bs, VC1_CODE_FRAME);
    memcpy(bs, buf, buf_size);
    ++ctx->frames;
    return 0;
}

static void asftoannexg_close(AVBitStreamFilterContext *bsfc) {
    ASFTOANNEXGBSFContext *ctx = bsfc->priv_data;
    av_freep(&ctx->seq_header);
    av_freep(&ctx->ep_header);
}

AVBitStreamFilter vc1_asftoannexg_bsf = {
    "vc1_asftoannexg",
    sizeof(ASFTOANNEXGBSFContext),
    asftoannexg_filter,
    asftoannexg_close,
};
