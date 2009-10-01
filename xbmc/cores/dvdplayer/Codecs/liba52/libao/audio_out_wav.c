/*
 * audio_out_wav.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
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
    int size;
} wav_instance_t;

static uint8_t wav_header[] = {
    'R', 'I', 'F', 'F', 0xfc, 0xff, 0xff, 0xff, 'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ', 16, 0, 0, 0,
    1, 0, 2, 0, -1, -1, -1, -1, -1, -1, -1, -1, 4, 0, 16, 0,
    'd', 'a', 't', 'a', 0xd8, 0xff, 0xff, 0xff
};

static int wav_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		      sample_t * level, sample_t * bias)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = 1;
    *bias = 384;

    return 0;
}

static void store (uint8_t * buf, int value)
{
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
}

static int wav_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;
    int16_t int16_samples[256*2];

#ifdef LIBA52_DOUBLE
    float samples[256 * 2];
    int i;

    for (i = 0; i < 256 * 2; i++)
	samples[i] = _samples[i];
#else
    float * samples = _samples;
#endif

    if (instance->set_params) {
	instance->set_params = 0;
	store (wav_header + 24, instance->sample_rate);
	store (wav_header + 28, instance->sample_rate * 4);
	fwrite (wav_header, sizeof (wav_header), 1, stdout);
    }

    float2s16_2 (samples, int16_samples);
    s16_LE (int16_samples, 2);
    fwrite (int16_samples, 256 * sizeof (int16_t) * 2, 1, stdout);

    instance->size += 256 * sizeof (int16_t) * 2;

    return 0;
}

static void wav_close (ao_instance_t * _instance)
{
    wav_instance_t * instance = (wav_instance_t *) _instance;

    if (fseek (stdout, 0, SEEK_SET) < 0)
	return;

    store (wav_header + 4, instance->size + 36);
    store (wav_header + 40, instance->size);
    fwrite (wav_header, sizeof (wav_header), 1, stdout);
}

static ao_instance_t * wav_open (int flags)
{
    wav_instance_t * instance;

    instance = malloc (sizeof (wav_instance_t));
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
