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

#include "libav_hacks.h"

#include <libavutil/mathematics.h>
#include <libavutil/opt.h>

struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
                int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                int log_offset, void *log_ctx)
{
    AVAudioResampleContext *ret = avresample_alloc_context();
    av_opt_set_int(ret, "out_channel_layout", out_ch_layout  , 0);
    av_opt_set_int(ret, "out_sample_fmt"    , out_sample_fmt , 0);
    av_opt_set_int(ret, "out_sample_rate"   , out_sample_rate, 0);
    av_opt_set_int(ret, "in_channel_layout" , in_ch_layout   , 0);
    av_opt_set_int(ret, "in_sample_fmt"     , in_sample_fmt  , 0);
    av_opt_set_int(ret, "in_sample_rate"    , in_sample_rate , 0);
    return ret;
}


int swr_init(struct SwrContext *s)
{
    return avresample_open(s);
}

void swr_free(struct SwrContext **s)
{
    avresample_close(*s);
    *s = NULL;
}

int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                const uint8_t **in , int in_count)
{
    return avresample_convert(s, out, 0, out_count, (uint8_t**)in, 0,in_count);
}

int64_t swr_get_delay(struct SwrContext *s, int64_t base)
{
    int64_t in_sr, out_sr;
    av_opt_get_int(s, "in_sample_rate", 0, &in_sr);
    av_opt_get_int(s, "out_sample_rate", 0, &out_sr);
    return av_rescale_rnd(avresample_available(s), base, out_sr, AV_ROUND_UP) + av_rescale_rnd(avresample_get_delay(s), base, in_sr, AV_ROUND_UP);
}

int swr_set_channel_mapping(struct SwrContext *s, const int *channel_map)
{
    return avresample_set_channel_mapping(s, channel_map);
}

int swr_set_matrix(struct SwrContext *s, const double *matrix, int stride)
{
    return avresample_set_matrix(s, matrix, stride);
}

int swr_set_compensation(struct SwrContext *s, int sample_delta, int compensation_distance)
{
    return avresample_set_compensation(s, sample_delta, compensation_distance);
}
