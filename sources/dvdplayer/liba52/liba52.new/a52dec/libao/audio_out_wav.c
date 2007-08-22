/*
 * audio_out_wav.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
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

#include "a52.h"
#include "audio_out.h"
#include "audio_out_internal.h"

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
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,
    'd', 'a', 't', 'a', 0xd8, 0xff, 0xff, 0xff
};

static uint8_t wav6_header[] = {
    'R', 'I', 'F', 'F', 0xf0, 0xff, 0xff, 0xff, 'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ', 40, 0, 0, 0,
    0xfe, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0,
    22, 0, 16, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0x10, 0x00, 0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71,
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
    static const uint16_t speaker_tbl[] = {
	3, 4, 3, 7, 0x103, 0x107, 0x33, 0x37, 4, 4, 3
    };
    static const uint8_t nfchans_tbl[] = {
	2, 1, 2, 3, 3, 4, 4, 5, 1, 1, 2
    };
    int chans;

    *speaker_flags = speaker_tbl[flags & A52_CHANNEL_MASK];
    chans = nfchans_tbl[flags & A52_CHANNEL_MASK];

    if (flags & A52_LFE) {
	*speaker_flags |= 8;	/* WAVE_SPEAKER_LOW_FREQUENCY */
	chans++;
    }

    return chans;	    
}

#include <stdio.h>

static int wav_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;
    int16_t int16_samples[256 * 6];
    int chans;
    uint32_t speaker_flags;

#ifdef LIBA52_DOUBLE
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
	if (speaker_flags == 3 || speaker_flags == 4) {
	    store2 (wav_header + 22, chans);
	    store4 (wav_header + 24, instance->sample_rate);
	    store4 (wav_header + 28, instance->sample_rate * 2 * chans);
	    store2 (wav_header + 32, 2 * chans);
	    fwrite (wav_header, sizeof (wav_header), 1, stdout);
	} else {
	    store2 (wav6_header + 22, chans);
	    store4 (wav6_header + 24, instance->sample_rate);
	    store4 (wav6_header + 28, instance->sample_rate * 2 * chans);
	    store2 (wav6_header + 32, 2 * chans);
	    store4 (wav6_header + 40, speaker_flags);
	    fwrite (wav6_header, sizeof (wav6_header), 1, stdout);
	}
    } else if (speaker_flags != instance->speaker_flags)
	return 1;

    convert2s16_wav (samples, int16_samples, flags);

    s16_LE (int16_samples, chans);
    fwrite (int16_samples, 256 * sizeof (int16_t) * chans, 1, stdout);

    instance->size += 256 * sizeof (int16_t) * chans;

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
	store4 (wav6_header + 4, instance->size + 60);
	store4 (wav6_header + 64, instance->size);
	fwrite (wav6_header, sizeof (wav6_header), 1, stdout);
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
    return wav_open (A52_STEREO);
}

ao_instance_t * ao_wavdolby_open (void)
{
    return wav_open (A52_DOLBY);
}

ao_instance_t * ao_wav6_open (void)
{
    return wav_open (-1);
}
