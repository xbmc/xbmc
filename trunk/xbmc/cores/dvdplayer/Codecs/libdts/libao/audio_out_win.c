/*
 * audio_out_win.c
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

#ifdef LIBAO_WIN

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <windows.h>
#include <mmsystem.h>
#include <inttypes.h>

#include "dts.h"
#include "audio_out.h"
#include "audio_out_internal.h"

static void win_close (ao_instance_t * _instance);

/* each buffer has a size of 256 samples. Using 40 of them will give us about
 * 1/5 sec of buffered sound for a 48000hz stream */
#define NUMBUF 40

typedef struct win_instance_s {
    ao_instance_t ao;
    HWAVEOUT h_waveout;
    WAVEHDR waveheader[NUMBUF];
    int16_t int16_samples[NUMBUF][256*2];
    int current_buffer;
    int sample_rate;
    int set_params;
    int flags;
} win_instance_t;

static int win_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		      level_t * level, sample_t * bias)
{
    win_instance_t * instance = (win_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = CONVERT_LEVEL;
    *bias = CONVERT_BIAS;

    return 0;
}

static int win_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    win_instance_t * instance = (win_instance_t *) _instance;
    int current_buffer;
    MMRESULT result;

#ifdef LIBDTS_DOUBLE
    convert_t samples[256 * 2];
    int i;

    for (i = 0; i < 256 * 2; i++)
	samples[i] = _samples[i];
#else
    convert_t * samples = _samples;
#endif

    flags &= DTS_CHANNEL_MASK | DTS_LFE;

    if (instance->set_params) {
	WAVEFORMATEX waveformat;
	MMRESULT result;
	int i;

	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nChannels = 2;
	waveformat.nSamplesPerSec = instance->sample_rate;
	waveformat.wBitsPerSample = 16;
	waveformat.nBlockAlign = 4;
	waveformat.nAvgBytesPerSec = 4 * instance->sample_rate;

	result = waveOutOpen (&instance->h_waveout, WAVE_MAPPER, &waveformat,
			      0 /*callback*/, 0 /*data*/, CALLBACK_NULL);
	if (result != MMSYSERR_NOERROR) {
	    fprintf (stderr, "Can not open waveOut device\n");
	    return 1;
	}

	for (i = 0; i < NUMBUF; i++) {
	    instance->waveheader[i].lpData = (LPSTR)instance->int16_samples[i];
	    instance->waveheader[i].dwBufferLength = 256 * 2 * sizeof(int16_t);
	    instance->waveheader[i].dwFlags = WHDR_DONE;
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

    current_buffer = instance->current_buffer;
    instance->current_buffer = (current_buffer + 1) % NUMBUF;

    while (!(instance->waveheader[current_buffer].dwFlags & WHDR_DONE))
	Sleep (1000 * 256 / instance->sample_rate);

    result = waveOutUnprepareHeader (instance->h_waveout,
				     &instance->waveheader[current_buffer],
				     sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
	fprintf (stderr, "waveOutUnprepareHeader failed\n");
	return 1;
    }

    instance->waveheader[current_buffer].dwFlags = 0;
    result = waveOutPrepareHeader (instance->h_waveout,
				   &instance->waveheader[current_buffer],
				   sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
	fprintf (stderr, "waveOutPrepareHeader failed\n");
	return 1;
    }

    convert2s16_2 (samples, instance->int16_samples[current_buffer]);

    result = waveOutWrite (instance->h_waveout,
			   &instance->waveheader[current_buffer],
			   sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
	fprintf (stderr, "waveOutWrite failed\n");
	return 1;
    }

    return 0;
}

static void win_close (ao_instance_t * _instance)
{
    win_instance_t * instance = (win_instance_t *) _instance;
    int i;

    waveOutReset (instance->h_waveout);

    for (i = 0; i < NUMBUF; i++)
	waveOutUnprepareHeader (instance->h_waveout,
				&instance->waveheader[i],
				sizeof(WAVEHDR));
    waveOutClose (instance->h_waveout);
}

static ao_instance_t * win_open (int flags)
{
    win_instance_t * instance;

    instance = malloc (sizeof (win_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = win_setup;
    instance->ao.play = win_play;
    instance->ao.close = win_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;
    instance->current_buffer = 0;

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_win_open (void)
{
    return win_open (DTS_STEREO);
}

ao_instance_t * ao_windolby_open (void)
{
    return win_open (DTS_DOLBY);
}

#endif
