/*
 * audio_out_oss.c
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

#ifdef LIBAO_OSS

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

#if defined(__OpenBSD__)
#include <soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#ifndef AFMT_S16_NE
#include <machine/endian.h>
#if BYTE_ORDER == LITTLE_ENDIAN
#define AFMT_S16_NE AFMT_S16_LE
#else
#define AFMT_S16_NE AFMT_S16_BE
#endif
#endif
#else
#include <sys/soundcard.h>
#endif

#include "a52.h"
#include "audio_out.h"
#include "audio_out_internal.h"

typedef struct oss_instance_s {
    ao_instance_t ao;
    int fd;
    int sample_rate;
    int set_params;
    int flags;
} oss_instance_t;

static int oss_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		      sample_t * level, sample_t * bias)
{
    oss_instance_t * instance = (oss_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = 1;
    *bias = 384;

    return 0;
}

static int oss_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    oss_instance_t * instance = (oss_instance_t *) _instance;
    int16_t int16_samples[256*6];
    int chans = -1;

#ifdef LIBA52_DOUBLE
    float samples[256 * 6];
    int i;

    for (i = 0; i < 256 * 6; i++)
	samples[i] = _samples[i];
#else
    float * samples = _samples;
#endif

    chans = channels_multi (flags);
    flags &= A52_CHANNEL_MASK | A52_LFE;

    if (instance->set_params) {
	int tmp;

	tmp = chans;
	if ((ioctl (instance->fd, SNDCTL_DSP_CHANNELS, &tmp) < 0) ||
	    (tmp != chans)) {
	    fprintf (stderr, "Can not set number of channels\n");
	    return 1;
	}

	tmp = instance->sample_rate;
	if ((ioctl (instance->fd, SNDCTL_DSP_SPEED, &tmp) < 0) ||
	    (tmp != instance->sample_rate)) {
	    fprintf (stderr, "Can not set sample rate\n");
	    return 1;
	}

	instance->flags = flags;
	instance->set_params = 0;
    } else if ((flags == A52_DOLBY) && (instance->flags == A52_STEREO)) {
	fprintf (stderr, "Switching from stereo to dolby surround\n");
	instance->flags = A52_DOLBY;
    } else if ((flags == A52_STEREO) && (instance->flags == A52_DOLBY)) {
	fprintf (stderr, "Switching from dolby surround to stereo\n");
	instance->flags = A52_STEREO;
    } else if (flags != instance->flags)
	return 1;

    float2s16_multi (samples, int16_samples, flags);
    write (instance->fd, int16_samples, 256 * sizeof (int16_t) * chans);

    return 0;
}

static void oss_close (ao_instance_t * _instance)
{
    oss_instance_t * instance = (oss_instance_t *) _instance;

    close (instance->fd);
}

static ao_instance_t * oss_open (int flags)
{
    oss_instance_t * instance;
    int format;

    instance = malloc (sizeof (oss_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = oss_setup;
    instance->ao.play = oss_play;
    instance->ao.close = oss_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;

    instance->fd = open ("/dev/dsp", O_WRONLY);
    if (instance->fd < 0) {
	fprintf (stderr, "Can not open /dev/dsp\n");
	free (instance);
	return NULL;
    }

    format = AFMT_S16_NE;
    if ((ioctl (instance->fd, SNDCTL_DSP_SETFMT, &format) < 0) ||
	(format != AFMT_S16_NE)) {
	fprintf (stderr, "Can not set sample format\n");
	free (instance);
	return NULL;
    }

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_oss_open (void)
{
    return oss_open (A52_STEREO);
}

ao_instance_t * ao_ossdolby_open (void)
{
    return oss_open (A52_DOLBY);
}

ao_instance_t * ao_oss4_open (void)
{
    return oss_open (A52_2F2R);
}

ao_instance_t * ao_oss6_open (void)
{
    return oss_open (A52_3F2R | A52_LFE);
}

#endif
