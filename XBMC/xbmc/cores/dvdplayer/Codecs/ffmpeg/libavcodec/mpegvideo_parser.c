/*
 * MPEG1 / MPEG2 video parser
 * Copyright (c) 2000,2001 Fabrice Bellard.
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
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

#include "parser.h"
#include "mpegvideo.h"

static void mpegvideo_extract_headers(AVCodecParserContext *s,
                                      AVCodecContext *avctx,
                                      const uint8_t *buf, int buf_size)
{
    ParseContext1 *pc = s->priv_data;
    const uint8_t *buf_end;
    const uint8_t *buf_start= buf;
    uint32_t start_code;
    int frame_rate_index, ext_type, bytes_left;
    int frame_rate_ext_n, frame_rate_ext_d;
    int picture_structure, top_field_first, repeat_first_field, progressive_frame;
    int horiz_size_ext, vert_size_ext, bit_rate_ext;
//FIXME replace the crap with get_bits()
    s->repeat_pict = 0;
    buf_end = buf + buf_size;
    while (buf < buf_end) {
        start_code= -1;
        buf= ff_find_start_code(buf, buf_end, &start_code);
        bytes_left = buf_end - buf;
        switch(start_code) {
        case PICTURE_START_CODE:
            ff_fetch_timestamp(s, buf-buf_start-4, 1);

            if (bytes_left >= 2) {
                s->pict_type = (buf[1] >> 3) & 7;
            }
            break;
        case SEQ_START_CODE:
            if (bytes_left >= 7) {
                pc->width  = (buf[0] << 4) | (buf[1] >> 4);
                pc->height = ((buf[1] & 0x0f) << 8) | buf[2];
                avcodec_set_dimensions(avctx, pc->width, pc->height);
                frame_rate_index = buf[3] & 0xf;
                pc->frame_rate.den = avctx->time_base.den = ff_frame_rate_tab[frame_rate_index].num;
                pc->frame_rate.num = avctx->time_base.num = ff_frame_rate_tab[frame_rate_index].den;
                avctx->bit_rate = ((buf[4]<<10) | (buf[5]<<2) | (buf[6]>>6))*400;
                avctx->codec_id = CODEC_ID_MPEG1VIDEO;
                avctx->sub_id = 1;
            }
            break;
        case EXT_START_CODE:
            if (bytes_left >= 1) {
                ext_type = (buf[0] >> 4);
                switch(ext_type) {
                case 0x1: /* sequence extension */
                    if (bytes_left >= 6) {
                        horiz_size_ext = ((buf[1] & 1) << 1) | (buf[2] >> 7);
                        vert_size_ext = (buf[2] >> 5) & 3;
                        bit_rate_ext = ((buf[2] & 0x1F)<<7) | (buf[3]>>1);
                        frame_rate_ext_n = (buf[5] >> 5) & 3;
                        frame_rate_ext_d = (buf[5] & 0x1f);
                        pc->progressive_sequence = buf[1] & (1 << 3);
                        avctx->has_b_frames= !(buf[5] >> 7);

                        pc->width  |=(horiz_size_ext << 12);
                        pc->height |=( vert_size_ext << 12);
                        avctx->bit_rate += (bit_rate_ext << 18) * 400;
                        avcodec_set_dimensions(avctx, pc->width, pc->height);
                        avctx->time_base.den = pc->frame_rate.den * (frame_rate_ext_n + 1);
                        avctx->time_base.num = pc->frame_rate.num * (frame_rate_ext_d + 1);
                        avctx->codec_id = CODEC_ID_MPEG2VIDEO;
                        avctx->sub_id = 2; /* forces MPEG2 */
                    }
                    break;
                case 0x8: /* picture coding extension */
                    if (bytes_left >= 5) {
                        picture_structure = buf[2]&3;
                        top_field_first = buf[3] & (1 << 7);
                        repeat_first_field = buf[3] & (1 << 1);
                        progressive_frame = buf[4] & (1 << 7);

                        /* check if we must repeat the frame */
                        if (repeat_first_field) {
                            if (pc->progressive_sequence) {
                                if (top_field_first)
                                    s->repeat_pict = 4;
                                else
                                    s->repeat_pict = 2;
                            } else if (progressive_frame) {
                                s->repeat_pict = 1;
                            }
                        }
                    }
                    break;
                }
            }
            break;
        case -1:
            goto the_end;
        default:
            /* we stop parsing when we encounter a slice. It ensures
               that this function takes a negligible amount of time */
            if (start_code >= SLICE_MIN_START_CODE &&
                start_code <= SLICE_MAX_START_CODE)
                goto the_end;
            break;
        }
    }
 the_end: ;
}

static int mpegvideo_parse(AVCodecParserContext *s,
                           AVCodecContext *avctx,
                           const uint8_t **poutbuf, int *poutbuf_size,
                           const uint8_t *buf, int buf_size)
{
    ParseContext1 *pc1 = s->priv_data;
    ParseContext *pc= &pc1->pc;
    int next;

    if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
        next= buf_size;
    }else{
        next= ff_mpeg1_find_frame_end(pc, buf, buf_size);

        if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
            *poutbuf = NULL;
            *poutbuf_size = 0;
            return buf_size;
        }

    }
    /* we have a full frame : we just parse the first few MPEG headers
       to have the full timing information. The time take by this
       function should be negligible for uncorrupted streams */
    mpegvideo_extract_headers(s, avctx, buf, buf_size);
#if 0
    printf("pict_type=%d frame_rate=%0.3f repeat_pict=%d\n",
           s->pict_type, (double)avctx->time_base.den / avctx->time_base.num, s->repeat_pict);
#endif

    *poutbuf = buf;
    *poutbuf_size = buf_size;
    return next;
}

static int mpegvideo_split(AVCodecContext *avctx,
                           const uint8_t *buf, int buf_size)
{
    int i;
    uint32_t state= -1;

    for(i=0; i<buf_size; i++){
        state= (state<<8) | buf[i];
        if(state != 0x1B3 && state != 0x1B5 && state < 0x200 && state >= 0x100)
            return i-3;
    }
    return 0;
}

AVCodecParser mpegvideo_parser = {
    { CODEC_ID_MPEG1VIDEO,
      CODEC_ID_MPEG2VIDEO, 
      CODEC_ID_MPEGVIDEO_VDPAU },
    sizeof(ParseContext1),
    NULL,
    mpegvideo_parse,
    ff_parse1_close,
};
