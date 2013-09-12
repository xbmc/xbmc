/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LIBAV_HACKS_H
#define __LIBAV_HACKS_H

#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavresample/avresample.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>

#if LIBAVUTIL_VERSION_MICRO >= 100
#error "You should not enable libav hacks when building against FFmpeg."
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52,8,0)
#error "Your libav version is too old. Please update to libav-10 or git master."
#endif

// libavutil

#define AVFRAME_IN_LAVU

#define AV_CODEC_ID_OTF AV_CODEC_ID_TTF
#define AV_CODEC_ID_SUBRIP  AV_CODEC_ID_FIRST_SUBTITLE

AVDictionary *av_frame_get_metadata       (const AVFrame *frame);

// libavformat

int avformat_alloc_output_context2(AVFormatContext **ctx, AVOutputFormat *oformat,
                const char *format_name, const char *filename);

#define AVFORMAT_HAS_STREAM_GET_R_FRAME_RATE

AVRational av_stream_get_r_frame_rate(const AVStream *s);

// libavresample

#define SwrContext AVAudioResampleContext

struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
                int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                int log_offset, void *log_ctx);

int swr_init(struct SwrContext *s);

void swr_free(struct SwrContext **s);

int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                const uint8_t **in , int in_count);

int64_t swr_get_delay(struct SwrContext *s, int64_t base);

int swr_set_channel_mapping(struct SwrContext *s, const int *channel_map);

int swr_set_matrix(struct SwrContext *s, const double *matrix, int stride);

int swr_set_compensation(struct SwrContext *s, int sample_delta, int compensation_distance);

// libavfilter

#define LIBAVFILTER_AVFRAME_BASED

typedef struct {
    const enum AVPixelFormat *pixel_fmts; ///< list of allowed pixel formats, terminated by AV_PIX_FMT_NONE
} AVBufferSinkParams;

AVBufferSinkParams *av_buffersink_params_alloc(void);

#define HAVE_AVFILTER_GRAPH_PARSE_PTR

int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters,
                             AVFilterInOut **inputs, AVFilterInOut **outputs,
                             void *log_ctx);

#endif
