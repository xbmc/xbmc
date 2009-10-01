/*
 * audio_out_solaris.c
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

#ifdef LIBAO_SOLARIS

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/audioio.h>
#include <inttypes.h>

#include "dts.h"
#include "audio_out.h"
#include "audio_out_internal.h"

typedef struct solaris_instance_s {
    ao_instance_t ao;
    int fd;
    int sample_rate;
    int set_params;
    int flags;
} solaris_instance_t;

static int solaris_setup (ao_instance_t * _instance, int sample_rate,
			  int * flags, level_t * level, sample_t * bias)
{
    solaris_instance_t * instance = (solaris_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = CONVERT_LEVEL;
    *bias = CONVERT_BIAS;

    return 0;
}

static int solaris_play (ao_instance_t * _instance, int flags,
			 sample_t * _samples)
{
    solaris_instance_t * instance = (solaris_instance_t *) _instance;
    int16_t int16_samples[256*2];

#ifdef LIBDTS_DOUBLE
    convert_t samples[256 * 2];
    int i;

    for (i = 0; i < 256 * 2; i++)
	samples[i] = _samples[i];
#else
    convert_t * samples = _samples;
#endif

    if (instance->set_params) {
	audio_info_t info;

        /* Setup our parameters */
        AUDIO_INITINFO (&info);
	info.play.sample_rate = instance->sample_rate;
	info.play.precision = 16;
	info.play.channels = 2;
	info.play.encoding = AUDIO_ENCODING_LINEAR;
	
	/* Write our configuration */
	/* An implicit GETINFO is also performed. */
	if (ioctl (instance->fd, AUDIO_SETINFO, &info) < 0) {
	    perror ("Writing audio config block");
	    return 1;
	}

	if ((info.play.sample_rate != instance->sample_rate) ||
	    (info.play.precision != 16) || (info.play.channels != 2)) {
	    fprintf (stderr, "Wanted %dHz %d bits %d channels\n",
		     instance->sample_rate, 16, 2);
	    fprintf (stderr, "Got    %dHz %d bits %d channels\n",
		     info.play.sample_rate, info.play.precision,
		     info.play.channels);
	    return 1;
	}

	instance->flags = flags;
	instance->set_params = 0;
    } else if ((flags == DTS_DOLBY) && (instance->flags == DTS_STEREO)) {
	fprintf (stderr, "Switching from stereo to dolby surround\n");
	instance->flags = DTS_DOLBY;
    } else if ((flags == DTS_STEREO) && (instance->flags == DTS_DOLBY)) {
	fprintf (stderr, "Switching from dolby surround to stereo\n");
	instance->flags = DTS_STEREO;
    } else if (flags != instance->flags)
	return 1;

    convert2s16_2 (samples, int16_samples);
    write (instance->fd, int16_samples, 256 * sizeof (int16_t) * 2);

    return 0;
}

static void solaris_close (ao_instance_t * _instance)
{
    solaris_instance_t * instance = (solaris_instance_t *) _instance;

    close (instance->fd);
}

static ao_instance_t * solaris_open (int flags)
{
    solaris_instance_t * instance;

    instance = malloc (sizeof (solaris_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = solaris_setup;
    instance->ao.play = solaris_play;
    instance->ao.close = solaris_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;

    instance->fd = open ("/dev/audio", O_WRONLY);
    if (instance->fd < 0) {
	fprintf (stderr, "Can not open /dev/audio\n");
	free (instance);
	return NULL;
    }

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_solaris_open (void)
{
    return solaris_open (DTS_STEREO);
}

ao_instance_t * ao_solarisdolby_open (void)
{
    return solaris_open (DTS_DOLBY);
}

#endif
