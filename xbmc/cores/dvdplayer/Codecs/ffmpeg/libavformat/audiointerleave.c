/*
 * Audio Interleaving functions
 *
 * Copyright (c) 2009 Baptiste Coudurier <baptiste dot coudurier at gmail dot com>
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

#include "libavutil/fifo.h"
#include "avformat.h"
#include "audiointerleave.h"
#include "internal.h"

void ff_audio_interleave_close(AVFormatContext *s)
{
    int i;
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        AudioInterleaveContext *aic = st->priv_data;

        if (st->codec->codec_type == CODEC_TYPE_AUDIO)
            av_fifo_free(&aic->fifo);
    }
}

int ff_audio_interleave_init(AVFormatContext *s,
                             const int *samples_per_frame,
                             AVRational time_base)
{
    int i;

    if (!samples_per_frame)
        return -1;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        AudioInterleaveContext *aic = st->priv_data;

        if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
            aic->sample_size = (st->codec->channels *
                                av_get_bits_per_sample(st->codec->codec_id)) / 8;
            if (!aic->sample_size) {
                av_log(s, AV_LOG_ERROR, "could not compute sample size\n");
                return -1;
            }
            aic->samples_per_frame = samples_per_frame;
            aic->samples = aic->samples_per_frame;
            aic->time_base = time_base;

            aic->fifo_size = 100* *aic->samples;
            av_fifo_init(&aic->fifo, 100 * *aic->samples);
        }
    }

    return 0;
}

static int ff_interleave_new_audio_packet(AVFormatContext *s, AVPacket *pkt,
                                   int stream_index, int flush)
{
    AVStream *st = s->streams[stream_index];
    AudioInterleaveContext *aic = st->priv_data;

    int size = FFMIN(av_fifo_size(&aic->fifo), *aic->samples * aic->sample_size);
    if (!size || (!flush && size == av_fifo_size(&aic->fifo)))
        return 0;

    av_new_packet(pkt, size);
    av_fifo_read(&aic->fifo, pkt->data, size);

    pkt->dts = pkt->pts = aic->dts;
    pkt->duration = av_rescale_q(*aic->samples, st->time_base, aic->time_base);
    pkt->stream_index = stream_index;
    aic->dts += pkt->duration;

    aic->samples++;
    if (!*aic->samples)
        aic->samples = aic->samples_per_frame;

    return size;
}

int ff_audio_rechunk_interleave(AVFormatContext *s, AVPacket *out, AVPacket *pkt, int flush,
                        int (*get_packet)(AVFormatContext *, AVPacket *, AVPacket *, int),
                        int (*compare_ts)(AVFormatContext *, AVPacket *, AVPacket *))
{
    int i;

    if (pkt) {
        AVStream *st = s->streams[pkt->stream_index];
        AudioInterleaveContext *aic = st->priv_data;
        if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
            unsigned new_size = av_fifo_size(&aic->fifo) + pkt->size;
            if (new_size > aic->fifo_size) {
                if (av_fifo_realloc2(&aic->fifo, new_size) < 0)
                    return -1;
                aic->fifo_size = new_size;
            }
            av_fifo_generic_write(&aic->fifo, pkt->data, pkt->size, NULL);
        } else {
            // rewrite pts and dts to be decoded time line position
            pkt->pts = pkt->dts = aic->dts;
            aic->dts += pkt->duration;
            ff_interleave_add_packet(s, pkt, compare_ts);
        }
        pkt = NULL;
    }

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        if (st->codec->codec_type == CODEC_TYPE_AUDIO) {
            AVPacket new_pkt;
            while (ff_interleave_new_audio_packet(s, &new_pkt, i, flush))
                ff_interleave_add_packet(s, &new_pkt, compare_ts);
        }
    }

    return get_packet(s, out, pkt, flush);
}
