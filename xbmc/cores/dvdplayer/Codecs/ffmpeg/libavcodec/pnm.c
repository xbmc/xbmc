/*
 * PNM image format
 * Copyright (c) 2002, 2003 Fabrice Bellard
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
#include "pnm.h"

static inline int pnm_space(int c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static void pnm_get(PNMContext *sc, char *str, int buf_size)
{
    char *s;
    int c;

    /* skip spaces and comments */
    for(;;) {
        c = *sc->bytestream++;
        if (c == '#')  {
            do  {
                c = *sc->bytestream++;
            } while (c != '\n' && sc->bytestream < sc->bytestream_end);
        } else if (!pnm_space(c)) {
            break;
        }
    }

    s = str;
    while (sc->bytestream < sc->bytestream_end && !pnm_space(c)) {
        if ((s - str)  < buf_size - 1)
            *s++ = c;
        c = *sc->bytestream++;
    }
    *s = '\0';
}

int ff_pnm_decode_header(AVCodecContext *avctx, PNMContext * const s){
    char buf1[32], tuple_type[32];
    int h, w, depth, maxval;

    pnm_get(s, buf1, sizeof(buf1));
    if (!strcmp(buf1, "P4")) {
        avctx->pix_fmt = PIX_FMT_MONOWHITE;
    } else if (!strcmp(buf1, "P5")) {
        if (avctx->codec_id == CODEC_ID_PGMYUV)
            avctx->pix_fmt = PIX_FMT_YUV420P;
        else
            avctx->pix_fmt = PIX_FMT_GRAY8;
    } else if (!strcmp(buf1, "P6")) {
        avctx->pix_fmt = PIX_FMT_RGB24;
    } else if (!strcmp(buf1, "P7")) {
        w = -1;
        h = -1;
        maxval = -1;
        depth = -1;
        tuple_type[0] = '\0';
        for(;;) {
            pnm_get(s, buf1, sizeof(buf1));
            if (!strcmp(buf1, "WIDTH")) {
                pnm_get(s, buf1, sizeof(buf1));
                w = strtol(buf1, NULL, 10);
            } else if (!strcmp(buf1, "HEIGHT")) {
                pnm_get(s, buf1, sizeof(buf1));
                h = strtol(buf1, NULL, 10);
            } else if (!strcmp(buf1, "DEPTH")) {
                pnm_get(s, buf1, sizeof(buf1));
                depth = strtol(buf1, NULL, 10);
            } else if (!strcmp(buf1, "MAXVAL")) {
                pnm_get(s, buf1, sizeof(buf1));
                maxval = strtol(buf1, NULL, 10);
            } else if (!strcmp(buf1, "TUPLETYPE")) {
                pnm_get(s, tuple_type, sizeof(tuple_type));
            } else if (!strcmp(buf1, "ENDHDR")) {
                break;
            } else {
                return -1;
            }
        }
        /* check that all tags are present */
        if (w <= 0 || h <= 0 || maxval <= 0 || depth <= 0 || tuple_type[0] == '\0' || avcodec_check_dimensions(avctx, w, h))
            return -1;

        avctx->width = w;
        avctx->height = h;
        if (depth == 1) {
            if (maxval == 1)
                avctx->pix_fmt = PIX_FMT_MONOWHITE;
            else
                avctx->pix_fmt = PIX_FMT_GRAY8;
        } else if (depth == 3) {
            if (maxval < 256) {
            avctx->pix_fmt = PIX_FMT_RGB24;
            } else {
                av_log(avctx, AV_LOG_ERROR, "16-bit components are only supported for grayscale\n");
                avctx->pix_fmt = PIX_FMT_NONE;
                return -1;
            }
        } else if (depth == 4) {
            avctx->pix_fmt = PIX_FMT_RGB32;
        } else {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
    pnm_get(s, buf1, sizeof(buf1));
    avctx->width = atoi(buf1);
    if (avctx->width <= 0)
        return -1;
    pnm_get(s, buf1, sizeof(buf1));
    avctx->height = atoi(buf1);
    if(avcodec_check_dimensions(avctx, avctx->width, avctx->height))
        return -1;
    if (avctx->pix_fmt != PIX_FMT_MONOWHITE) {
        pnm_get(s, buf1, sizeof(buf1));
        s->maxval = atoi(buf1);
        if (s->maxval >= 256) {
            if (avctx->pix_fmt == PIX_FMT_GRAY8) {
                avctx->pix_fmt = PIX_FMT_GRAY16BE;
                if (s->maxval != 65535)
                    avctx->pix_fmt = PIX_FMT_GRAY16;
            } if (avctx->pix_fmt == PIX_FMT_RGB24) {
                if (s->maxval > 255)
                    avctx->pix_fmt = PIX_FMT_RGB48BE;
            } else {
                av_log(avctx, AV_LOG_ERROR, "Unsupported pixel format\n");
                avctx->pix_fmt = PIX_FMT_NONE;
                return -1;
            }
        }
    }
    /* more check if YUV420 */
    if (avctx->pix_fmt == PIX_FMT_YUV420P) {
        if ((avctx->width & 1) != 0)
            return -1;
        h = (avctx->height * 2);
        if ((h % 3) != 0)
            return -1;
        h /= 3;
        avctx->height = h;
    }
    return 0;
}
