/*
 * audio_out_float.c
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
#include <inttypes.h>

#include "dts.h"
#include "audio_out.h"
#include "audio_out_internal.h"

static int float_setup (ao_instance_t * instance, int sample_rate, int * flags,
			level_t * level, sample_t * bias)
{
    *flags = DTS_STEREO;
    *level = CONVERT_LEVEL;
    *bias = 0;

    return 0;
}

static int float_play (ao_instance_t * instance, int flags,
		       sample_t * _samples)
{
#if defined(LIBDTS_FIXED)
    float samples[256 * 2];
    int i;
    
    for (i = 0; i < 256 * 2; i++)
      samples[i] = _samples[i] * (1.0 / (1 << 30));
#elif defined(LIBDTS_DOUBLE)
    float samples[256 * 2];
    int i;

    for (i = 0; i < 256 * 2; i++)
	samples[i] = _samples[i];
#else
    float * samples = _samples;
#endif

    fwrite (samples, sizeof (float), 256 * 2, stdout);

    return 0;
}

static void float_close (ao_instance_t * instance)
{
}

static ao_instance_t instance = {float_setup, float_play, float_close};

ao_instance_t * ao_float_open (void)
{
    return &instance;
}
