/*
 * audio_out_wav.c
 * Copyright (C) 2004 Gildas Bazin <gbazin@videolan.org>
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of dtsdec, a free DTS Coherent Acoustics stream decoder.
 * See http://www.videolan.org/dtsdec.html for updates.
 *
 * dtsdec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dtsdec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include "dts.h"
#include "audio_out.h"
#include "audio_out_internal.h"

#define WAVE_FORMAT_PCM        0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_EXTENSIBLE 0xFFFE

#define SPEAKER_FRONT_LEFT             0x1
#define SPEAKER_FRONT_RIGHT            0x2
#define SPEAKER_FRONT_CENTER           0x4
#define SPEAKER_LOW_FREQUENCY          0x8
#define SPEAKER_BACK_LEFT              0x10
#define SPEAKER_BACK_RIGHT             0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER   0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER  0x80
#define SPEAKER_BACK_CENTER            0x100
#define SPEAKER_SIDE_LEFT              0x200
#define SPEAKER_SIDE_RIGHT             0x400
#define SPEAKER_TOP_CENTER             0x800
#define SPEAKER_TOP_FRONT_LEFT         0x1000
#define SPEAKER_TOP_FRONT_CENTER       0x2000
#define SPEAKER_TOP_FRONT_RIGHT        0x4000
#define SPEAKER_TOP_BACK_LEFT          0x8000
#define SPEAKER_TOP_BACK_CENTER        0x10000
#define SPEAKER_TOP_BACK_RIGHT         0x20000
#define SPEAKER_RESERVED               0x80000000

typedef struct wav_instance_s {
    ao_instance_t ao;
    int sample_rate;
    int set_params;
    int flags;
    uint32_t speaker_flags;
    int size;
} wav_instance_t;

static uint8_t wav_header[] = {
    'R', 'I', 'F', 'F', 0xfc, 0xff, 0xff, 0xff, 'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ', 16, 0, 0, 0,
    WAVE_FORMAT_PCM, WAVE_FORMAT_PCM >> 8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,
    'd', 'a', 't', 'a', 0xd8, 0xff, 0xff, 0xff
};

static uint8_t wavmulti_header[] = {
    'R', 'I', 'F', 'F', 0xf0, 0xff, 0xff, 0xff, 'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ', 40, 0, 0, 0,
    (uint8_t)(WAVE_FORMAT_EXTENSIBLE & 0xFF), WAVE_FORMAT_EXTENSIBLE >> 8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 22, 0,
    0, 0, 0, 0, 0, 0,
    WAVE_FORMAT_IEEE_FLOAT, WAVE_FORMAT_IEEE_FLOAT >> 8,
    0, 0, 0, 0, 0x10, 0x00, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71,
    'd', 'a', 't', 'a', 0xb4, 0xff, 0xff, 0xff
};

static int wav_setup (ao_instance_t * _instance, int sample_rate, int * flags,
                      level_t * level, sample_t * bias)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
        return 1;
    instance->sample_rate = sample_rate;

    if (instance->flags >= 0)
        *flags = instance->flags;
    *level = CONVERT_LEVEL;
    *bias = 0;

    if( instance->flags == DTS_STEREO );
        *bias = CONVERT_BIAS;

    return 0;
}

static void store4 (uint8_t * buf, int value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
}

static void store2 (uint8_t * buf, int16_t value)
{
    buf[0] = value;
    buf[1] = value >> 8;
}

static int wav_channels (int flags, uint32_t * speaker_flags)
{
    static const uint32_t speaker_tbl[] = {
        SPEAKER_FRONT_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
          | SPEAKER_BACK_CENTER,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT
          | SPEAKER_BACK_RIGHT,
        SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER
          | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT,
        /* TODO > 5 channels */
    };
    static const uint8_t nfchans_tbl[] =
    {
        1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
    };
    int chans;

    *speaker_flags = speaker_tbl[flags & DTS_CHANNEL_MASK];
    chans = nfchans_tbl[flags & DTS_CHANNEL_MASK];

    if (flags & DTS_LFE) {
        *speaker_flags |= SPEAKER_LOW_FREQUENCY;
        chans++;
    }

    return chans;
}

#include <stdio.h>

static int wav_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;
    float ordered_samples[256 * 6];
    int chans, size;
    uint32_t speaker_flags;

    static const int chan_map[10][6] =
    { { 0, 0, 0, 0, 0, 0 },       /* DTS_MONO */
      { 0, 1, 0, 0, 0, 0 },       /* DTS_CHANNEL */
      { 0, 1, 0, 0, 0, 0 },       /* DTS_STEREO */
      { 0, 1, 0, 0, 0, 0 },       /* DTS_STEREO_SUMDIFF */
      { 0, 1, 0, 0, 0, 0 },       /* DTS_STEREO_TOTAL */
      { 2, 0, 1, 0, 0, 0 },       /* DTS_3F */
      { 0, 1, 2, 0, 0, 0 },       /* DTS_2F1R */
      { 2, 0, 1, 3, 0, 0 },       /* DTS_3F1R */
      { 0, 1, 2, 3, 0, 0 },       /* DTS_2F2R */
      { 2, 0, 1, 3, 4, 0 },       /* DTS_3F2R */
    };
    static const int chan_map_lfe[10][6] =
    { { 0, 1, 0, 0, 0, 0 },       /* DTS_MONO */
      { 0, 1, 2, 0, 0, 0 },       /* DTS_CHANNEL */
      { 0, 1, 2, 0, 0, 0 },       /* DTS_STEREO */
      { 0, 1, 2, 0, 0, 0 },       /* DTS_STEREO_SUMDIFF */
      { 0, 1, 2, 0, 0, 0 },       /* DTS_STEREO_TOTAL */
      { 2, 0, 1, 3, 0, 0 },       /* DTS_3F */
      { 0, 1, 3, 2, 0, 0 },       /* DTS_2F1R */
      { 2, 0, 1, 4, 3, 0 },       /* DTS_3F1R */
      { 0, 1, 3, 4, 2, 0 },       /* DTS_2F2R */
      { 2, 0, 1, 4, 5, 3 },       /* DTS_3F2R */
    };

#ifdef LIBDTS_DOUBLE
    convert_t samples[256 * 6];
    int i;

    for (i = 0; i < 256 * 6; i++)
        samples[i] = _samples[i];
#else
    convert_t * samples = _samples;
#endif

    chans = wav_channels (flags, &speaker_flags);
    if (instance->set_params) {
        instance->set_params = 0;
        instance->speaker_flags = speaker_flags;
        if (chans == 2) {
            store2 (wav_header + 22, chans);
            store4 (wav_header + 24, instance->sample_rate);
            store4 (wav_header + 28, instance->sample_rate * 2 * chans);
            store2 (wav_header + 32, 2 * chans);
            fwrite (wav_header, sizeof (wav_header), 1, stdout);
        } else {
            store2 (wavmulti_header + 22, chans);
            store4 (wavmulti_header + 24, instance->sample_rate);
            store4 (wavmulti_header + 28,
                    instance->sample_rate * sizeof(float) * chans);
            store2 (wavmulti_header + 32, sizeof(float) * chans);
            store2 (wavmulti_header + 38, sizeof(float) * 8);
            store4 (wavmulti_header + 40, speaker_flags);
            fwrite (wavmulti_header, sizeof (wavmulti_header), 1, stdout);
        }
    } else if (speaker_flags != instance->speaker_flags)
        return 1;

    if (chans == 2) {
        convert2s16_2 (samples, (int16_t *)ordered_samples);
        s16_LE ((int16_t *)ordered_samples, chans);
        size = 256 * sizeof (int16_t) * chans;
    } else {
        int i, j;

        if (flags & DTS_LFE)
        {
            flags &= ~DTS_LFE;
            for (j = 0; j < chans; j++)
                for (i = 0; i < 256; i++)
                  ordered_samples[i * chans + chan_map_lfe[flags][j]] =
                        _samples[j * 256 + i];
        }
        else
            for (j = 0; j < chans; j++)
                for (i = 0; i < 256; i++)
                    ordered_samples[i * chans + chan_map[flags][j]] =
                        _samples[j * chans + i];

        s32_LE (ordered_samples, chans);
        size = 256 * sizeof (float) * chans;
    }

    fwrite (ordered_samples, size, 1, stdout);
    instance->size += size;

    return 0;
}

static void wav_close (ao_instance_t * _instance)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;

    if (fseek (stdout, 0, SEEK_SET) < 0)
        return;

    if (instance->speaker_flags == 3 || instance->speaker_flags == 4) {
        store4 (wav_header + 4, instance->size + 36);
        store4 (wav_header + 40, instance->size);
        fwrite (wav_header, sizeof (wav_header), 1, stdout);
    } else {
        store4 (wavmulti_header + 4, instance->size + 60);
        store4 (wavmulti_header + 64, instance->size);
        fwrite (wavmulti_header, sizeof (wavmulti_header), 1, stdout);
    }
}

static ao_instance_t * wav_open (int flags)
{
    wav_instance_t * instance;

    instance = (wav_instance_t *) malloc (sizeof (wav_instance_t));
    if (instance == NULL)
        return NULL;

    instance->ao.setup = wav_setup;
    instance->ao.play = wav_play;
    instance->ao.close = wav_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;
    instance->size = 0;

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_wav_open (void)
{
    return wav_open (DTS_STEREO);
}

ao_instance_t * ao_wavdolby_open (void)
{
    return wav_open (DTS_DOLBY);
}

ao_instance_t * ao_wav6_open (void)
{
    return wav_open (DTS_3F2R|DTS_LFE);
}

ao_instance_t * ao_wavall_open (void)
{
    return wav_open (-1);
}
