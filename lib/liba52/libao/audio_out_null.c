/*
 * audio_out_null.c
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

#include <inttypes.h>

#include "a52.h"
#include "audio_out.h"

typedef struct null_instance_s {
    ao_instance_t ao;
    int channels;
} null_instance_t;

static int null_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		       sample_t * level, sample_t * bias)
{
    null_instance_t * instance = (null_instance_t *) _instance;

    *flags = instance->channels;
    *level = 1;
    *bias = 0;

    return 0;
}

static int null_play (ao_instance_t * instance, int flags, sample_t * samples)
{
    return 0;
}

static void null_close (ao_instance_t * instance)
{
}

static null_instance_t instance = {{null_setup, null_play, null_close}, 0};

ao_instance_t * ao_null_open (void)
{
    instance.channels = A52_STEREO;

    return (ao_instance_t *) &instance;
}

ao_instance_t * ao_null4_open (void)
{
    instance.channels = A52_2F2R;

    return (ao_instance_t *) &instance;
}

ao_instance_t * ao_null6_open (void)
{
    instance.channels = A52_3F2R | A52_LFE;

    return (ao_instance_t *) &instance;
}
