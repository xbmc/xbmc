/*
 *
 * Copyright (C) 2003 the ffmpeg project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Creative YUV (CYUV) Video Decoder
 *   by Mike Melanson (melanson@pcisys.net)
 * based on "Creative YUV (CYUV) stream format for AVI":
 *   http://www.csse.monash.edu.au/~timf/videocodec/cyuv.txt
 *
 */

/**
 * @file cyuv.c 
 * Creative YUV (CYUV) Video Decoder.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "avcodec.h"
#include "dsputil.h"
#include "mpegvideo.h"


typedef struct CyuvDecodeContext {
    AVCodecContext *avctx;
    int width, height;
    AVFrame frame;
} CyuvDecodeContext;

static int cyuv_decode_init(AVCodecContext *avctx)
{
    CyuvDecodeContext *s = avctx->priv_data;

    s->avctx = avctx;
    s->width = avctx->width;
    s->height = avctx->height;
    avctx->pix_fmt = PIX_FMT_YUV411P;
    avctx->has_b_frames = 0;

    return 0;
}

static int cyuv_decode_frame(AVCodecContext *avctx, 
                             void *data, int *data_size,
                             uint8_t *buf, int buf_size)
{
    CyuvDecodeContext *s=avctx->priv_data;

    unsigned char *y_plane;
    unsigned char *u_plane;
    unsigned char *v_plane;
    int y_ptr;
    int u_ptr;
    int v_ptr;

    /* prediction error tables (make it clear that they are signed values) */
    signed char *y_table = buf +  0;
    signed char *u_table = buf + 16;
    signed char *v_table = buf + 32;

    unsigned char y_pred, u_pred, v_pred;
    int stream_ptr;
    unsigned char cur_byte;
    int pixel_groups;

    *data_size = 0;

    /* sanity check the buffer size: A buffer has 3x16-bytes tables
     * followed by (height) lines each with 3 bytes to represent groups
     * of 4 pixels. Thus, the total size of the buffer ought to be:
     *    (3 * 16) + height * (width * 3 / 4) */
    if (buf_size != 48 + s->height * (s->width * 3 / 4)) {
      av_log(avctx, AV_LOG_ERROR, "ffmpeg: cyuv: got a buffer with %d bytes when %d were expected\n",
        buf_size,
        48 + s->height * (s->width * 3 / 4));
      return -1;
    }

    /* pixel data starts 48 bytes in, after 3x16-byte tables */
    stream_ptr = 48;

    if(s->frame.data[0])
        avctx->release_buffer(avctx, &s->frame);

    s->frame.buffer_hints = FF_BUFFER_HINTS_VALID;
    s->frame.reference = 0;
    if(avctx->get_buffer(avctx, &s->frame) < 0) {
        av_log(avctx, AV_LOG_ERROR, "get_buffer() failed\n");
        return -1;
    }

    y_plane = s->frame.data[0];
    u_plane = s->frame.data[1];
    v_plane = s->frame.data[2];

    /* iterate through each line in the height */
    for (y_ptr = 0, u_ptr = 0, v_ptr = 0;
         y_ptr < (s->height * s->frame.linesize[0]); 
         y_ptr += s->frame.linesize[0] - s->width,
         u_ptr += s->frame.linesize[1] - s->width / 4,
         v_ptr += s->frame.linesize[2] - s->width / 4) {

        /* reset predictors */
        cur_byte = buf[stream_ptr++];
        u_plane[u_ptr++] = u_pred = cur_byte & 0xF0;
        y_plane[y_ptr++] = y_pred = (cur_byte & 0x0F) << 4;

        cur_byte = buf[stream_ptr++];
        v_plane[v_ptr++] = v_pred = cur_byte & 0xF0;
        y_pred += y_table[cur_byte & 0x0F];
        y_plane[y_ptr++] = y_pred;

        cur_byte = buf[stream_ptr++];
        y_pred += y_table[cur_byte & 0x0F];
        y_plane[y_ptr++] = y_pred;
        y_pred += y_table[(cur_byte & 0xF0) >> 4];
        y_plane[y_ptr++] = y_pred;

        /* iterate through the remaining pixel groups (4 pixels/group) */
        pixel_groups = s->width / 4 - 1;
        while (pixel_groups--) {

            cur_byte = buf[stream_ptr++];
            u_pred += u_table[(cur_byte & 0xF0) >> 4];
            u_plane[u_ptr++] = u_pred;
            y_pred += y_table[cur_byte & 0x0F];
            y_plane[y_ptr++] = y_pred;

            cur_byte = buf[stream_ptr++];
            v_pred += v_table[(cur_byte & 0xF0) >> 4];
            v_plane[v_ptr++] = v_pred;
            y_pred += y_table[cur_byte & 0x0F];
            y_plane[y_ptr++] = y_pred;

            cur_byte = buf[stream_ptr++];
            y_pred += y_table[cur_byte & 0x0F];
            y_plane[y_ptr++] = y_pred;
            y_pred += y_table[(cur_byte & 0xF0) >> 4];
            y_plane[y_ptr++] = y_pred;

        }
    }

    *data_size=sizeof(AVFrame);
    *(AVFrame*)data= s->frame;

    return buf_size;
}

static int cyuv_decode_end(AVCodecContext *avctx)
{
/*    CyuvDecodeContext *s = avctx->priv_data;*/

    return 0;
}

AVCodec cyuv_decoder = {
    "cyuv",
    CODEC_TYPE_VIDEO,
    CODEC_ID_CYUV,
    sizeof(CyuvDecodeContext),
    cyuv_decode_init,
    NULL,
    cyuv_decode_end,
    cyuv_decode_frame,
    CODEC_CAP_DR1,
    NULL
};

