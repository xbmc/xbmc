/*
 * DVD subtitle decoding for ffmpeg
 * Copyright (c) 2005 Fabrice Bellard.
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
#include "colorspace.h"
#include "dsputil.h"

//#define DEBUG

typedef struct DVDSubContext
{
  uint8_t  colormap[4];
  uint8_t  alpha[4];
  uint32_t palette[16];
  int      has_palette;
} DVDSubContext;

static void yuv_a_to_rgba(const uint8_t *ycbcr, const uint8_t *alpha, uint32_t *rgba, int num_values)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
    uint8_t r, g, b;
    int i, y, cb, cr;
    int r_add, g_add, b_add;

    for (i = num_values; i > 0; i--) {
        y = *ycbcr++;
        cb = *ycbcr++;
        cr = *ycbcr++;
        YUV_TO_RGB1_CCIR(cb, cr);
        YUV_TO_RGB2_CCIR(r, g, b, y);
        *rgba++ = (*alpha++ << 24) | (r << 16) | (g << 8) | b;
    }
}

static void ayvu_to_argb(const uint8_t *ayvu, uint32_t *argb, int num_values)
{
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;
    uint8_t r, g, b;
    int i, y, cb, cr, a;
    int r_add, g_add, b_add;

    for (i = num_values; i > 0; i--) {
        a = *ayvu++;
        y = *ayvu++;
        cr = *ayvu++;
        cb = *ayvu++;
        YUV_TO_RGB1_CCIR(cb, cr);
        YUV_TO_RGB2_CCIR(r, g, b, y);
        *argb++ = (a << 24) | (r << 16) | (g << 8) | b;
    }
}

static int decode_run_2bit(GetBitContext *gb, int *color)
{
    unsigned int v, t;

    v = 0;
    for (t = 1; v < t && t <= 0x40; t <<= 2)
        v = (v << 4) | get_bits(gb, 4);
    *color = v & 3;
    if (v < 4) { /* Code for fill rest of line */
        return INT_MAX;
    }
    return v >> 2;
}

static int decode_run_8bit(GetBitContext *gb, int *color)
{
    int len;
    int has_run = get_bits1(gb);
    if (get_bits1(gb))
        *color = get_bits(gb, 8);
    else
        *color = get_bits(gb, 2);
    if (has_run) {
        if (get_bits1(gb)) {
            len = get_bits(gb, 7);
            if (len == 0)
                len = INT_MAX;
            else
                len += 9;
        } else
            len = get_bits(gb, 3) + 2;
    } else
        len = 1;
    return len;
}

static int decode_rle(uint8_t *bitmap, int linesize, int w, int h,
                      const uint8_t *buf, int start, int buf_size, int is_8bit)
{
    GetBitContext gb;
    int bit_len;
    int x, y, len, color;
    uint8_t *d;

    bit_len = (buf_size - start) * 8;
    init_get_bits(&gb, buf + start, bit_len);

    x = 0;
    y = 0;
    d = bitmap;
    for(;;) {
        if (get_bits_count(&gb) > bit_len)
            return -1;
        if (is_8bit)
            len = decode_run_8bit(&gb, &color);
        else
            len = decode_run_2bit(&gb, &color);
        len = FFMIN(len, w - x);
        memset(d + x, color, len);
        x += len;
        if (x >= w) {
            y++;
            if (y >= h)
                break;
            d += linesize;
            x = 0;
            /* byte align */
            align_get_bits(&gb);
        }
    }
    return 0;
}

static void fill_palette(DVDSubContext* ctx,
                         uint32_t *rgba_palette,
                         uint32_t subtitle_color)
{
    uint8_t color_used[16];
    int nb_opaque_colors, i, level, j, r, g, b;
    uint8_t *colormap = ctx->colormap;
    uint8_t *alpha = ctx->alpha;

    if(ctx->has_palette) {
        for(i = 0; i < 4; i++)
            rgba_palette[i] = (ctx->palette[ctx->colormap[i]] & 0x00ffffff)
                              | ((alpha[i] * 17) << 24);
        return;
    }

    for(i = 0; i < 4; i++)
        rgba_palette[i] = 0;

    memset(color_used, 0, 16);
    nb_opaque_colors = 0;
    for(i = 0; i < 4; i++) {
        if (alpha[i] != 0 && !color_used[colormap[i]]) {
            color_used[colormap[i]] = 1;
            nb_opaque_colors++;
        }
    }

    if (nb_opaque_colors == 0)
        return;

    j = nb_opaque_colors;
    memset(color_used, 0, 16);
    for(i = 0; i < 4; i++) {
        if (alpha[i] != 0) {
            if (!color_used[colormap[i]])  {
                level = (0xff * j) / nb_opaque_colors;
                r = (((subtitle_color >> 16) & 0xff) * level) >> 8;
                g = (((subtitle_color >> 8) & 0xff) * level) >> 8;
                b = (((subtitle_color >> 0) & 0xff) * level) >> 8;
                rgba_palette[i] = b | (g << 8) | (r << 16) | ((alpha[i] * 17) << 24);
                color_used[colormap[i]] = (i + 1);
                j--;
            } else {
                rgba_palette[i] = (rgba_palette[color_used[colormap[i]] - 1] & 0x00ffffff) |
                                    ((alpha[i] * 17) << 24);
            }
        }
    }
}

#define READ_OFFSET(a) (big_offsets ? AV_RB32(a) : AV_RB16(a))

static int decode_dvd_subtitles(DVDSubContext *ctx, AVSubtitle *sub_header,
                                const uint8_t *buf, int buf_size)
{
    int cmd_pos, pos, cmd, x1, y1, x2, y2, offset1, offset2, next_cmd_pos;
    int big_offsets, offset_size, is_8bit = 0;
    const uint8_t *yuv_palette = 0;
    uint8_t *colormap = ctx->colormap;
    uint8_t *alpha = ctx->alpha;
    int date;
    int i;
    int is_menu = 0;

    if (buf_size < 10)
        return -1;
    sub_header->rects = NULL;
    sub_header->num_rects = 0;
    sub_header->start_display_time = 0;
    sub_header->end_display_time = 0;

    if (AV_RB16(buf) == 0) {   /* HD subpicture with 4-byte offsets */
        big_offsets = 1;
        offset_size = 4;
        cmd_pos = 6;
    } else {
        big_offsets = 0;
        offset_size = 2;
        cmd_pos = 2;
    }

    cmd_pos = READ_OFFSET(buf + cmd_pos);

    while ((cmd_pos + 2 + offset_size) < buf_size) {
        date = AV_RB16(buf + cmd_pos);
        next_cmd_pos = READ_OFFSET(buf + cmd_pos + 2);
#ifdef DEBUG
        av_log(NULL, AV_LOG_INFO, "cmd_pos=0x%04x next=0x%04x date=%d\n",
               cmd_pos, next_cmd_pos, date);
#endif
        pos = cmd_pos + 2 + offset_size;
        offset1 = -1;
        offset2 = -1;
        x1 = y1 = x2 = y2 = 0;
        while (pos < buf_size) {
            cmd = buf[pos++];
#ifdef DEBUG
            av_log(NULL, AV_LOG_INFO, "cmd=%02x\n", cmd);
#endif
            switch(cmd) {
            case 0x00:
                /* menu subpicture */
                is_menu = 1;
                break;
            case 0x01:
                /* set start date */
                sub_header->start_display_time = (date << 10) / 90;
                break;
            case 0x02:
                /* set end date */
                sub_header->end_display_time = (date << 10) / 90;
                break;
            case 0x03:
                /* set colormap */
                if ((buf_size - pos) < 2)
                    goto fail;
                colormap[3] = buf[pos] >> 4;
                colormap[2] = buf[pos] & 0x0f;
                colormap[1] = buf[pos + 1] >> 4;
                colormap[0] = buf[pos + 1] & 0x0f;
                pos += 2;
                break;
            case 0x04:
                /* set alpha */
                if ((buf_size - pos) < 2)
                    goto fail;
                alpha[3] = buf[pos] >> 4;
                alpha[2] = buf[pos] & 0x0f;
                alpha[1] = buf[pos + 1] >> 4;
                alpha[0] = buf[pos + 1] & 0x0f;
                pos += 2;
#ifdef DEBUG
            av_log(NULL, AV_LOG_INFO, "alpha=%x%x%x%x\n", alpha[0],alpha[1],alpha[2],alpha[3]);
#endif
                break;
            case 0x05:
            case 0x85:
                if ((buf_size - pos) < 6)
                    goto fail;
                x1 = (buf[pos] << 4) | (buf[pos + 1] >> 4);
                x2 = ((buf[pos + 1] & 0x0f) << 8) | buf[pos + 2];
                y1 = (buf[pos + 3] << 4) | (buf[pos + 4] >> 4);
                y2 = ((buf[pos + 4] & 0x0f) << 8) | buf[pos + 5];
                if (cmd & 0x80)
                    is_8bit = 1;
#ifdef DEBUG
                av_log(NULL, AV_LOG_INFO, "x1=%d x2=%d y1=%d y2=%d\n",
                       x1, x2, y1, y2);
#endif
                pos += 6;
                break;
            case 0x06:
                if ((buf_size - pos) < 4)
                    goto fail;
                offset1 = AV_RB16(buf + pos);
                offset2 = AV_RB16(buf + pos + 2);
#ifdef DEBUG
                av_log(NULL, AV_LOG_INFO, "offset1=0x%04x offset2=0x%04x\n", offset1, offset2);
#endif
                pos += 4;
                break;
            case 0x86:
                if ((buf_size - pos) < 8)
                    goto fail;
                offset1 = AV_RB32(buf + pos);
                offset2 = AV_RB32(buf + pos + 4);
#ifdef DEBUG
                av_log(NULL, AV_LOG_INFO, "offset1=0x%04x offset2=0x%04x\n", offset1, offset2);
#endif
                pos += 8;
                break;

            case 0x83:
                /* HD set palette */
                if ((buf_size - pos) < 768)
                    goto fail;
                yuv_palette = buf + pos;
                pos += 768;
                break;
            case 0x84:
                /* HD set contrast (alpha) */
                if ((buf_size - pos) < 256)
                    goto fail;
                for (i = 0; i < 256; i++)
                    alpha[i] = 0xFF - buf[pos+i];
                pos += 256;
                break;

            case 0xff:
                goto the_end;
            default:
#ifdef DEBUG
                av_log(NULL, AV_LOG_INFO, "unrecognised subpicture command 0x%x\n", cmd);
#endif
                goto the_end;
            }
        }
    the_end:
        if (offset1 >= 0) {
            int w, h;
            uint8_t *bitmap;

            /* decode the bitmap */
            w = x2 - x1 + 1;
            if (w < 0)
                w = 0;
            h = y2 - y1;
            if (h < 0)
                h = 0;
            if (w > 0 && h > 0) {
                if (sub_header->rects != NULL) {
                    for (i = 0; i < sub_header->num_rects; i++) {
                        av_free(sub_header->rects[i].bitmap);
                        av_free(sub_header->rects[i].rgba_palette);
                    }
                    av_freep(&sub_header->rects);
                    sub_header->num_rects = 0;
                }

                bitmap = av_malloc(w * h);
                sub_header->rects = av_mallocz(sizeof(AVSubtitleRect));
                sub_header->num_rects = 1;
                sub_header->rects[0].bitmap = bitmap;
                decode_rle(bitmap, w * 2, w, (h + 1) / 2,
                           buf, offset1, buf_size, is_8bit);
                decode_rle(bitmap + w, w * 2, w, h / 2,
                           buf, offset2, buf_size, is_8bit);
                if (is_8bit) {
                    if (yuv_palette == 0)
                        goto fail;
                    sub_header->rects[0].rgba_palette = av_malloc(256 * 4);
                    sub_header->rects[0].nb_colors = 256;
                    yuv_a_to_rgba(yuv_palette, alpha, sub_header->rects[0].rgba_palette, 256);
                } else {
                    sub_header->rects[0].rgba_palette = av_malloc(4 * 4);
                    sub_header->rects[0].nb_colors = 4;
                    fill_palette(ctx, sub_header->rects[0].rgba_palette, 0xffff00);
                }
                sub_header->rects[0].x = x1;
                sub_header->rects[0].y = y1;
                sub_header->rects[0].w = w;
                sub_header->rects[0].h = h;
                sub_header->rects[0].linesize = w;
            }
        }
        if (next_cmd_pos == cmd_pos)
            break;
        cmd_pos = next_cmd_pos;
    }
    if (sub_header->num_rects > 0)
        return is_menu;
 fail:
    if (sub_header->rects != NULL) {
        for (i = 0; i < sub_header->num_rects; i++) {
            av_free(sub_header->rects[i].bitmap);
            av_free(sub_header->rects[i].rgba_palette);
        }
        av_freep(&sub_header->rects);
        sub_header->num_rects = 0;
    }
    return -1;
}

static int is_transp(const uint8_t *buf, int pitch, int n,
                     const uint8_t *transp_color)
{
    int i;
    for(i = 0; i < n; i++) {
        if (!transp_color[*buf])
            return 0;
        buf += pitch;
    }
    return 1;
}

/* return 0 if empty rectangle, 1 if non empty */
static int find_smallest_bounding_rectangle(AVSubtitle *s)
{
    uint8_t transp_color[256];
    int y1, y2, x1, x2, y, w, h, i;
    uint8_t *bitmap;

    if (s->num_rects == 0 || s->rects == NULL || s->rects[0].w <= 0 || s->rects[0].h <= 0)
        return 0;

    memset(transp_color, 0, 256);
    for(i = 0; i < s->rects[0].nb_colors; i++) {
        if ((s->rects[0].rgba_palette[i] >> 24) == 0)
            transp_color[i] = 1;
    }
    y1 = 0;
    while (y1 < s->rects[0].h && is_transp(s->rects[0].bitmap + y1 * s->rects[0].linesize,
                                  1, s->rects[0].w, transp_color))
        y1++;
    if (y1 == s->rects[0].h) {
        av_freep(&s->rects[0].bitmap);
        s->rects[0].w = s->rects[0].h = 0;
        return 0;
    }

    y2 = s->rects[0].h - 1;
    while (y2 > 0 && is_transp(s->rects[0].bitmap + y2 * s->rects[0].linesize, 1,
                               s->rects[0].w, transp_color))
        y2--;
    x1 = 0;
    while (x1 < (s->rects[0].w - 1) && is_transp(s->rects[0].bitmap + x1, s->rects[0].linesize,
                                        s->rects[0].h, transp_color))
        x1++;
    x2 = s->rects[0].w - 1;
    while (x2 > 0 && is_transp(s->rects[0].bitmap + x2, s->rects[0].linesize, s->rects[0].h,
                                  transp_color))
        x2--;
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bitmap = av_malloc(w * h);
    if (!bitmap)
        return 1;
    for(y = 0; y < h; y++) {
        memcpy(bitmap + w * y, s->rects[0].bitmap + x1 + (y1 + y) * s->rects[0].linesize, w);
    }
    av_freep(&s->rects[0].bitmap);
    s->rects[0].bitmap = bitmap;
    s->rects[0].linesize = w;
    s->rects[0].w = w;
    s->rects[0].h = h;
    s->rects[0].x += x1;
    s->rects[0].y += y1;
    return 1;
}

#ifdef DEBUG
#undef fprintf
#undef perror
#undef exit
static void ppm_save(const char *filename, uint8_t *bitmap, int w, int h,
                     uint32_t *rgba_palette)
{
    int x, y, v;
    FILE *f;

    f = fopen(filename, "w");
    if (!f) {
        perror(filename);
        exit(1);
    }
    fprintf(f, "P6\n"
            "%d %d\n"
            "%d\n",
            w, h, 255);
    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            v = rgba_palette[bitmap[y * w + x]];
            putc((v >> 16) & 0xff, f);
            putc((v >> 8) & 0xff, f);
            putc((v >> 0) & 0xff, f);
        }
    }
    fclose(f);
}
#endif

static int dvdsub_decode(AVCodecContext *avctx,
                         void *data, int *data_size,
                         const uint8_t *buf, int buf_size)
{
    DVDSubContext *ctx = (DVDSubContext*) avctx->priv_data;
    AVSubtitle *sub = (void *)data;
    int is_menu;

    is_menu = decode_dvd_subtitles(ctx, sub, buf, buf_size);

    if (is_menu < 0) {
    no_subtitle:
        *data_size = 0;

        return buf_size;
    }
    if (!is_menu && find_smallest_bounding_rectangle(sub) == 0)
        goto no_subtitle;

#if defined(DEBUG)
    av_log(NULL, AV_LOG_INFO, "start=%d ms end =%d ms\n",
           sub->start_display_time,
           sub->end_display_time);
    ppm_save("/tmp/a.ppm", sub->rects[0].bitmap,
             sub->rects[0].w, sub->rects[0].h, sub->rects[0].rgba_palette);
#endif

    *data_size = 1;
    return buf_size;
}

static int dvdsub_init(AVCodecContext *avctx)
{
    DVDSubContext *ctx = (DVDSubContext*) avctx->priv_data;
    char *data = avctx->extradata;

    if (!avctx->extradata || !avctx->extradata_size)
        return 1;

    data[avctx->extradata_size] = '\0';

    for(;;) {
        int pos = strcspn(data, "\n\r");
        if (pos==0 && *data==0)
            break;

        if (strncmp("palette:", data, 8) == 0) {
            int i;
            char *p = data+8;
            ctx->has_palette = 1;
            for(i=0;i<16;i++) {
                ctx->palette[i] = strtoul(p, &p, 16);
                while(*p == ',' || isspace(*p))
                    p++;
            }
#if defined(DEBUG)
            av_log(avctx, AV_LOG_INFO, "palette:");
            for(i=0;i<16;i++)
                av_log(avctx, AV_LOG_WARNING, " 0x%06x", ctx->palette[i]);
            av_log(avctx, AV_LOG_INFO, "\n");
#endif
        }

        data += pos;
        data += strspn(data, "\n\r");
    }

    if(!ctx->has_palette && avctx->extradata_size == 64)
        ayvu_to_argb((uint8_t*)avctx->extradata, ctx->palette, 16);

    return 1;
}

AVCodec dvdsub_decoder = {
    "dvdsub",
    CODEC_TYPE_SUBTITLE,
    CODEC_ID_DVD_SUBTITLE,
    sizeof(DVDSubContext),
    dvdsub_init,
    NULL,
    NULL,
    dvdsub_decode,
    .long_name = NULL_IF_CONFIG_SMALL("DVD subtitles"),
};
