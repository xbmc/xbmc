/*
 * audio_out_aif.c
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

typedef struct aif_instance_s {
    ao_instance_t ao;
    int sample_rate;
    int set_params;
    int flags;
    int size;
} aif_instance_t;

static uint8_t aif_header[] = {
    'F', 'O', 'R', 'M', 0xff, 0xff, 0xff, 0xfe, 'A', 'I', 'F', 'F',
    'C', 'O', 'M', 'M', 0, 0, 0, 18,
    0, 2, 0x3f, 0xff, 0xff, 0xf4, 0, 16, 0x40, 0x0e, -1, -1, 0, 0, 0, 0, 0, 0,
    'S', 'S', 'N', 'D', 0xff, 0xff, 0xff, 0xd8, 0, 0, 0, 0, 0, 0, 0, 0
};

static int aif_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		      sample_t * level, sample_t * bias)
{
    aif_instance_t * instance = (aif_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = 1;
    *bias = 384;

    return 0;
}

static void store4 (uint8_t * buf, int value)
{
    buf[0] = value >> 24;
    buf[1] = value >> 16;
    buf[2] = value >> 8;
    buf[3] = value;
}

static void store2 (uint8_t * buf, int16_t value)
{
    buf[0] = value >> 8;
    buf[1] = value;
}

static int aif_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    aif_instance_t * instance = (aif_instance_t *) _instance;
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
	store2 (aif_header + 30, instance->sample_rate);
	fwrite (aif_header, sizeof (aif_header), 1, stdout);
    }

    float2s16_2 (samples, int16_samples);
    s16_BE (int16_samples, 2);
    fwrite (int16_samples, 256 * sizeof (int16_t) * 2, 1, stdout);

    instance->size += 256 * sizeof (int16_t) * 2;

    return 0;
}

static void aif_close (ao_instance_t * _instance)
{
    aif_instance_t * instance = (aif_instance_t *) _instance;

    if (fseek (stdout, 0, SEEK_SET) < 0)
	return;

    store4 (aif_header + 4, instance->size + 46);
    store4 (aif_header + 22, instance->size / 4);
    store4 (aif_header + 42, instance->size + 8);
    fwrite (aif_header, sizeof (aif_header), 1, stdout);
}

static ao_instance_t * aif_open (int flags)
{
    aif_instance_t * instance;

    instance = malloc (sizeof (aif_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = aif_setup;
    instance->ao.play = aif_play;
    instance->ao.close = aif_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;
    instance->size = 0;

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_aif_open (void)
{
    return aif_open (A52_STEREO);
}

ao_instance_t * ao_aifdolby_open (void)
{
    return aif_open (A52_DOLBY);
}
