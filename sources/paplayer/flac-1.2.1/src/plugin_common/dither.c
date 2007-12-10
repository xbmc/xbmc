/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * dithering routine derived from (other GPLed source):
 * mad - MPEG audio decoder
 * Copyright (C) 2000-2001 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "dither.h"
#include "FLAC/assert.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))

#ifndef FLaC__INLINE
#define FLaC__INLINE
#endif


/* 32-bit pseudo-random number generator
 *
 * @@@ According to Miroslav, this one is poor quality, the one from the
 * @@@ original replaygain code is much better
 */
static FLaC__INLINE FLAC__uint32 prng(FLAC__uint32 state)
{
	return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

/* dither routine derived from MAD winamp plugin */

typedef struct {
	FLAC__int32 error[3];
	FLAC__int32 random;
} dither_state;

static FLaC__INLINE FLAC__int32 linear_dither(unsigned source_bps, unsigned target_bps, FLAC__int32 sample, dither_state *dither, const FLAC__int32 MIN, const FLAC__int32 MAX)
{
	unsigned scalebits;
	FLAC__int32 output, mask, random;

	FLAC__ASSERT(source_bps < 32);
	FLAC__ASSERT(target_bps <= 24);
	FLAC__ASSERT(target_bps <= source_bps);

	/* noise shape */
	sample += dither->error[0] - dither->error[1] + dither->error[2];

	dither->error[2] = dither->error[1];
	dither->error[1] = dither->error[0] / 2;

	/* bias */
	output = sample + (1L << (source_bps - target_bps - 1));

	scalebits = source_bps - target_bps;
	mask = (1L << scalebits) - 1;

	/* dither */
	random = (FLAC__int32)prng(dither->random);
	output += (random & mask) - (dither->random & mask);

	dither->random = random;

	/* clip */
	if(output > MAX) {
		output = MAX;

		if(sample > MAX)
			sample = MAX;
	}
	else if(output < MIN) {
		output = MIN;

		if(sample < MIN)
			sample = MIN;
	}

	/* quantize */
	output &= ~mask;

	/* error feedback */
	dither->error[0] = sample - output;

	/* scale */
	return output >> scalebits;
}

size_t FLAC__plugin_common__pack_pcm_signed_big_endian(FLAC__byte *data, const FLAC__int32 * const input[], unsigned wide_samples, unsigned channels, unsigned source_bps, unsigned target_bps)
{
	static dither_state dither[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
	FLAC__byte * const start = data;
	FLAC__int32 sample;
	const FLAC__int32 *input_;
	unsigned samples, channel;
	const unsigned bytes_per_sample = target_bps / 8;
	const unsigned incr = bytes_per_sample * channels;

	FLAC__ASSERT(channels > 0 && channels <= FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS);
	FLAC__ASSERT(source_bps < 32);
	FLAC__ASSERT(target_bps <= 24);
	FLAC__ASSERT(target_bps <= source_bps);
	FLAC__ASSERT((source_bps & 7) == 0);
	FLAC__ASSERT((target_bps & 7) == 0);

	if(source_bps != target_bps) {
		const FLAC__int32 MIN = -(1L << (source_bps - 1));
		const FLAC__int32 MAX = ~MIN; /*(1L << (source_bps-1)) - 1 */

		for(channel = 0; channel < channels; channel++) {
			
			samples = wide_samples;
			data = start + bytes_per_sample * channel;
			input_ = input[channel];

			while(samples--) {
				sample = linear_dither(source_bps, target_bps, *input_++, &dither[channel], MIN, MAX);

				switch(target_bps) {
					case 8:
						data[0] = sample ^ 0x80;
						break;
					case 16:
						data[0] = (FLAC__byte)(sample >> 8);
						data[1] = (FLAC__byte)sample;
						break;
					case 24:
						data[0] = (FLAC__byte)(sample >> 16);
						data[1] = (FLAC__byte)(sample >> 8);
						data[2] = (FLAC__byte)sample;
						break;
				}

				data += incr;
			}
		}
	}
	else {
		for(channel = 0; channel < channels; channel++) {
			samples = wide_samples;
			data = start + bytes_per_sample * channel;
			input_ = input[channel];

			while(samples--) {
				sample = *input_++;

				switch(target_bps) {
					case 8:
						data[0] = sample ^ 0x80;
						break;
					case 16:
						data[0] = (FLAC__byte)(sample >> 8);
						data[1] = (FLAC__byte)sample;
						break;
					case 24:
						data[0] = (FLAC__byte)(sample >> 16);
						data[1] = (FLAC__byte)(sample >> 8);
						data[2] = (FLAC__byte)sample;
						break;
				}

				data += incr;
			}
		}
	}

	return wide_samples * channels * (target_bps/8);
}

size_t FLAC__plugin_common__pack_pcm_signed_little_endian(FLAC__byte *data, const FLAC__int32 * const input[], unsigned wide_samples, unsigned channels, unsigned source_bps, unsigned target_bps)
{
	static dither_state dither[FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
	FLAC__byte * const start = data;
	FLAC__int32 sample;
	const FLAC__int32 *input_;
	unsigned samples, channel;
	const unsigned bytes_per_sample = target_bps / 8;
	const unsigned incr = bytes_per_sample * channels;

	FLAC__ASSERT(channels > 0 && channels <= FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS);
	FLAC__ASSERT(source_bps < 32);
	FLAC__ASSERT(target_bps <= 24);
	FLAC__ASSERT(target_bps <= source_bps);
	FLAC__ASSERT((source_bps & 7) == 0);
	FLAC__ASSERT((target_bps & 7) == 0);

	if(source_bps != target_bps) {
		const FLAC__int32 MIN = -(1L << (source_bps - 1));
		const FLAC__int32 MAX = ~MIN; /*(1L << (source_bps-1)) - 1 */

		for(channel = 0; channel < channels; channel++) {
			
			samples = wide_samples;
			data = start + bytes_per_sample * channel;
			input_ = input[channel];

			while(samples--) {
				sample = linear_dither(source_bps, target_bps, *input_++, &dither[channel], MIN, MAX);

				switch(target_bps) {
					case 8:
						data[0] = sample ^ 0x80;
						break;
					case 24:
						data[2] = (FLAC__byte)(sample >> 16);
						/* fall through */
					case 16:
						data[1] = (FLAC__byte)(sample >> 8);
						data[0] = (FLAC__byte)sample;
				}

				data += incr;
			}
		}
	}
	else {
		for(channel = 0; channel < channels; channel++) {
			samples = wide_samples;
			data = start + bytes_per_sample * channel;
			input_ = input[channel];

			while(samples--) {
				sample = *input_++;

				switch(target_bps) {
					case 8:
						data[0] = sample ^ 0x80;
						break;
					case 24:
						data[2] = (FLAC__byte)(sample >> 16);
						/* fall through */
					case 16:
						data[1] = (FLAC__byte)(sample >> 8);
						data[0] = (FLAC__byte)sample;
				}

				data += incr;
			}
		}
	}

	return wide_samples * channels * (target_bps/8);
}
