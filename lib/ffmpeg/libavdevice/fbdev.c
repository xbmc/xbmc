/*
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2009 Giliard B. de Freitas <giliarde@gmail.com>
 * Copyright (C) 2002 Gunnar Monell <gmo@linux.nu>
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
 * @file
 * Linux framebuffer input device,
 * inspired by code from fbgrab.c by Gunnar Monell.
 * @see http://linux-fbdev.sourceforge.net/
 */

/* #define DEBUG */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>
#include <linux/fb.h>

#include "libavutil/log.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavutil/pixdesc.h"
#include "avdevice.h"
#include "libavformat/internal.h"

struct rgb_pixfmt_map_entry {
    int bits_per_pixel;
    int red_offset, green_offset, blue_offset, alpha_offset;
    enum PixelFormat pixfmt;
};

static const struct rgb_pixfmt_map_entry rgb_pixfmt_map[] = {
    // bpp, red_offset,  green_offset, blue_offset, alpha_offset, pixfmt
    {  32,       0,           8,          16,           24,   PIX_FMT_RGBA  },
    {  32,      16,           8,           0,           24,   PIX_FMT_BGRA  },
    {  32,       8,          16,          24,            0,   PIX_FMT_ARGB  },
    {  32,       3,           2,           8,            0,   PIX_FMT_ABGR  },
    {  24,       0,           8,          16,            0,   PIX_FMT_RGB24 },
    {  24,      16,           8,           0,            0,   PIX_FMT_BGR24 },
};

static enum PixelFormat get_pixfmt_from_fb_varinfo(struct fb_var_screeninfo *varinfo)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(rgb_pixfmt_map); i++) {
        const struct rgb_pixfmt_map_entry *entry = &rgb_pixfmt_map[i];
        if (entry->bits_per_pixel == varinfo->bits_per_pixel &&
            entry->red_offset     == varinfo->red.offset     &&
            entry->green_offset   == varinfo->green.offset   &&
            entry->blue_offset    == varinfo->blue.offset)
            return entry->pixfmt;
    }

    return PIX_FMT_NONE;
}

typedef struct {
    AVClass *class;          ///< class for private options
    int frame_size;          ///< size in bytes of a grabbed frame
    AVRational framerate_q;  ///< framerate
    char *framerate;         ///< framerate string set by a private option
    int64_t time_frame;      ///< time for the next frame to output (in 1/1000000 units)

    int fd;                  ///< framebuffer device file descriptor
    int width, height;       ///< assumed frame resolution
    int frame_linesize;      ///< linesize of the output frame, it is assumed to be constant
    int bytes_per_pixel;

    struct fb_var_screeninfo varinfo; ///< variable info;
    struct fb_fix_screeninfo fixinfo; ///< fixed    info;

    uint8_t *data;           ///< framebuffer data
} FBDevContext;

av_cold static int fbdev_read_header(AVFormatContext *avctx,
                                     AVFormatParameters *ap)
{
    FBDevContext *fbdev = avctx->priv_data;
    AVStream *st = NULL;
    enum PixelFormat pix_fmt;
    int ret, flags = O_RDONLY;

    ret = av_parse_video_rate(&fbdev->framerate_q, fbdev->framerate);
    if (ret < 0) {
        av_log(avctx, AV_LOG_ERROR, "Could not parse framerate '%s'.\n", fbdev->framerate);
        return ret;
    }

    if (!(st = avformat_new_stream(avctx, NULL)))
        return AVERROR(ENOMEM);
    avpriv_set_pts_info(st, 64, 1, 1000000); /* 64 bits pts in microseconds */

    /* NONBLOCK is ignored by the fbdev driver, only set for consistency */
    if (avctx->flags & AVFMT_FLAG_NONBLOCK)
        flags |= O_NONBLOCK;

    if ((fbdev->fd = open(avctx->filename, flags)) == -1) {
        ret = AVERROR(errno);
        av_log(avctx, AV_LOG_ERROR,
               "Could not open framebuffer device '%s': %s\n",
               avctx->filename, strerror(ret));
        return ret;
    }

    if (ioctl(fbdev->fd, FBIOGET_VSCREENINFO, &fbdev->varinfo) < 0) {
        ret = AVERROR(errno);
        av_log(avctx, AV_LOG_ERROR,
               "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
        goto fail;
    }

    if (ioctl(fbdev->fd, FBIOGET_FSCREENINFO, &fbdev->fixinfo) < 0) {
        ret = AVERROR(errno);
        av_log(avctx, AV_LOG_ERROR,
               "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
        goto fail;
    }

    pix_fmt = get_pixfmt_from_fb_varinfo(&fbdev->varinfo);
    if (pix_fmt == PIX_FMT_NONE) {
        ret = AVERROR(EINVAL);
        av_log(avctx, AV_LOG_ERROR,
               "Framebuffer pixel format not supported.\n");
        goto fail;
    }

    fbdev->width           = fbdev->varinfo.xres;
    fbdev->height          = fbdev->varinfo.yres;
    fbdev->bytes_per_pixel = (fbdev->varinfo.bits_per_pixel + 7) >> 3;
    fbdev->frame_linesize  = fbdev->width * fbdev->bytes_per_pixel;
    fbdev->frame_size      = fbdev->frame_linesize * fbdev->height;
    fbdev->time_frame      = AV_NOPTS_VALUE;
    fbdev->data = mmap(NULL, fbdev->fixinfo.smem_len, PROT_READ, MAP_SHARED, fbdev->fd, 0);
    if (fbdev->data == MAP_FAILED) {
        ret = AVERROR(errno);
        av_log(avctx, AV_LOG_ERROR, "Error in mmap(): %s\n", strerror(errno));
        goto fail;
    }

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id   = CODEC_ID_RAWVIDEO;
    st->codec->width      = fbdev->width;
    st->codec->height     = fbdev->height;
    st->codec->pix_fmt    = pix_fmt;
    st->codec->time_base  = (AVRational){fbdev->framerate_q.den, fbdev->framerate_q.num};
    st->codec->bit_rate   =
        fbdev->width * fbdev->height * fbdev->bytes_per_pixel * av_q2d(fbdev->framerate_q) * 8;

    av_log(avctx, AV_LOG_INFO,
           "w:%d h:%d bpp:%d pixfmt:%s fps:%d/%d bit_rate:%d\n",
           fbdev->width, fbdev->height, fbdev->varinfo.bits_per_pixel,
           av_pix_fmt_descriptors[pix_fmt].name,
           fbdev->framerate_q.num, fbdev->framerate_q.den,
           st->codec->bit_rate);
    return 0;

fail:
    close(fbdev->fd);
    return ret;
}

static int fbdev_read_packet(AVFormatContext *avctx, AVPacket *pkt)
{
    FBDevContext *fbdev = avctx->priv_data;
    int64_t curtime, delay;
    struct timespec ts;
    int i, ret;
    uint8_t *pin, *pout;

    if (fbdev->time_frame == AV_NOPTS_VALUE)
        fbdev->time_frame = av_gettime();

    /* wait based on the frame rate */
    while (1) {
        curtime = av_gettime();
        delay = fbdev->time_frame - curtime;
        av_dlog(avctx,
                "time_frame:%"PRId64" curtime:%"PRId64" delay:%"PRId64"\n",
                fbdev->time_frame, curtime, delay);
        if (delay <= 0) {
            fbdev->time_frame += INT64_C(1000000) / av_q2d(fbdev->framerate_q);
            break;
        }
        if (avctx->flags & AVFMT_FLAG_NONBLOCK)
            return AVERROR(EAGAIN);
        ts.tv_sec  =  delay / 1000000;
        ts.tv_nsec = (delay % 1000000) * 1000;
        while (nanosleep(&ts, &ts) < 0 && errno == EINTR);
    }

    if ((ret = av_new_packet(pkt, fbdev->frame_size)) < 0)
        return ret;

    /* refresh fbdev->varinfo, visible data position may change at each call */
    if (ioctl(fbdev->fd, FBIOGET_VSCREENINFO, &fbdev->varinfo) < 0)
        av_log(avctx, AV_LOG_WARNING,
               "Error refreshing variable info: %s\n", strerror(errno));

    pkt->pts = curtime;

    /* compute visible data offset */
    pin = fbdev->data + fbdev->bytes_per_pixel * fbdev->varinfo.xoffset +
                        fbdev->varinfo.yoffset * fbdev->fixinfo.line_length;
    pout = pkt->data;

    for (i = 0; i < fbdev->height; i++) {
        memcpy(pout, pin, fbdev->frame_linesize);
        pin  += fbdev->fixinfo.line_length;
        pout += fbdev->frame_linesize;
    }

    return fbdev->frame_size;
}

av_cold static int fbdev_read_close(AVFormatContext *avctx)
{
    FBDevContext *fbdev = avctx->priv_data;

    munmap(fbdev->data, fbdev->frame_size);
    close(fbdev->fd);

    return 0;
}

#define OFFSET(x) offsetof(FBDevContext, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    { "framerate","", OFFSET(framerate), AV_OPT_TYPE_STRING, {.str = "25"}, 0, 0, DEC },
    { NULL },
};

static const AVClass fbdev_class = {
    .class_name = "fbdev indev",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_fbdev_demuxer = {
    .name           = "fbdev",
    .long_name      = NULL_IF_CONFIG_SMALL("Linux framebuffer"),
    .priv_data_size = sizeof(FBDevContext),
    .read_header    = fbdev_read_header,
    .read_packet    = fbdev_read_packet,
    .read_close     = fbdev_read_close,
    .flags          = AVFMT_NOFILE,
    .priv_class     = &fbdev_class,
};
