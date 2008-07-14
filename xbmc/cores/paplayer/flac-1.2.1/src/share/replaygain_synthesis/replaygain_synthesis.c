/* replaygain_synthesis - Routines for applying ReplayGain to a signal
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * This is an aggregation of pieces of code from John Edwards' WaveGain
 * program.  Mostly cosmetic changes were made; otherwise, the dithering
 * code is almost untouched and the gain processing was converted from
 * processing a whole file to processing chunks of samples.
 *
 * The original copyright notices for WaveGain's dither.c and wavegain.c
 * appear below:
 */
/*
 * (c) 2002 John Edwards
 * mostly lifted from work by Frank Klemm
 * random functions for dithering.
 */
/*
 * Copyright (C) 2002 John Edwards
 * Additional code by Magnus Holmgren and Gian-Carlo Pascutto
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h> /* for memset() */
#include <math.h>
#include "private/fast_float_math_hack.h"
#include "replaygain_synthesis.h"
#include "FLAC/assert.h"

#ifndef FLaC__INLINE
#define FLaC__INLINE
#endif

/* adjust for compilers that can't understand using LL suffix for int64_t literals */
#ifdef _MSC_VER
#define FLAC__I64L(x) x
#else
#define FLAC__I64L(x) x##LL
#endif


/*
 * the following is based on parts of dither.c
 */


/*
 *  This is a simple random number generator with good quality for audio purposes.
 *  It consists of two polycounters with opposite rotation direction and different
 *  periods. The periods are coprime, so the total period is the product of both.
 *
 *     -------------------------------------------------------------------------------------------------
 * +-> |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0|
 * |   -------------------------------------------------------------------------------------------------
 * |                                                                          |  |  |  |     |        |
 * |                                                                          +--+--+--+-XOR-+--------+
 * |                                                                                      |
 * +--------------------------------------------------------------------------------------+
 *
 *     -------------------------------------------------------------------------------------------------
 *     |31:30:29:28:27:26:25:24:23:22:21:20:19:18:17:16:15:14:13:12:11:10: 9: 8: 7: 6: 5: 4: 3: 2: 1: 0| <-+
 *     -------------------------------------------------------------------------------------------------   |
 *       |  |           |  |                                                                               |
 *       +--+----XOR----+--+                                                                               |
 *                |                                                                                        |
 *                +----------------------------------------------------------------------------------------+
 *
 *
 *  The first has an period of 3*5*17*257*65537, the second of 7*47*73*178481,
 *  which gives a period of 18.410.713.077.675.721.215. The result is the
 *  XORed values of both generators.
 */

static unsigned int random_int_(void)
{
	static const unsigned char parity_[256] = {
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
		1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
		1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
		1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
		0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
		1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
	};
	static unsigned int r1_ = 1;
	static unsigned int r2_ = 1;

	unsigned int t1, t2, t3, t4;

	/* Parity calculation is done via table lookup, this is also available
	 * on CPUs without parity, can be implemented in C and avoid unpredictable
	 * jumps and slow rotate through the carry flag operations.
	 */
	t3   = t1 = r1_;    t4   = t2 = r2_;
	t1  &= 0xF5;        t2 >>= 25;
	t1   = parity_[t1]; t2  &= 0x63;
	t1 <<= 31;          t2   = parity_[t2];

	return (r1_ = (t3 >> 1) | t1 ) ^ (r2_ = (t4 + t4) | t2 );
}

/* gives a equal distributed random number */
/* between -2^31*mult and +2^31*mult */
static double random_equi_(double mult)
{
	return mult * (int) random_int_();
}

/* gives a triangular distributed random number */
/* between -2^32*mult and +2^32*mult */
static double random_triangular_(double mult)
{
	return mult * ( (double) (int) random_int_() + (double) (int) random_int_() );
}


static const float  F44_0 [16 + 32] = {
	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0,
	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0,

	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0,
	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0,

	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0,
	(float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0, (float)0
};


static const float  F44_1 [16 + 32] = {  /* SNR(w) = 4.843163 dB, SNR = -3.192134 dB */
	(float) 0.85018292704024355931, (float) 0.29089597350995344721, (float)-0.05021866022121039450, (float)-0.23545456294599161833,
	(float)-0.58362726442227032096, (float)-0.67038978965193036429, (float)-0.38566861572833459221, (float)-0.15218663390367969967,
	(float)-0.02577543084864530676, (float) 0.14119295297688728127, (float) 0.22398848581628781612, (float) 0.15401727203382084116,
	(float) 0.05216161232906000929, (float)-0.00282237820999675451, (float)-0.03042794608323867363, (float)-0.03109780942998826024,

	(float) 0.85018292704024355931, (float) 0.29089597350995344721, (float)-0.05021866022121039450, (float)-0.23545456294599161833,
	(float)-0.58362726442227032096, (float)-0.67038978965193036429, (float)-0.38566861572833459221, (float)-0.15218663390367969967,
	(float)-0.02577543084864530676, (float) 0.14119295297688728127, (float) 0.22398848581628781612, (float) 0.15401727203382084116,
	(float) 0.05216161232906000929, (float)-0.00282237820999675451, (float)-0.03042794608323867363, (float)-0.03109780942998826024,

	(float) 0.85018292704024355931, (float) 0.29089597350995344721, (float)-0.05021866022121039450, (float)-0.23545456294599161833,
	(float)-0.58362726442227032096, (float)-0.67038978965193036429, (float)-0.38566861572833459221, (float)-0.15218663390367969967,
	(float)-0.02577543084864530676, (float) 0.14119295297688728127, (float) 0.22398848581628781612, (float) 0.15401727203382084116,
	(float) 0.05216161232906000929, (float)-0.00282237820999675451, (float)-0.03042794608323867363, (float)-0.03109780942998826024,
};


static const float  F44_2 [16 + 32] = {  /* SNR(w) = 10.060213 dB, SNR = -12.766730 dB */
	(float) 1.78827593892108555290, (float) 0.95508210637394326553, (float)-0.18447626783899924429, (float)-0.44198126506275016437,
	(float)-0.88404052492547413497, (float)-1.42218907262407452967, (float)-1.02037566838362314995, (float)-0.34861755756425577264,
	(float)-0.11490230170431934434, (float) 0.12498899339968611803, (float) 0.38065885268563131927, (float) 0.31883491321310506562,
	(float) 0.10486838686563442765, (float)-0.03105361685110374845, (float)-0.06450524884075370758, (float)-0.02939198261121969816,

	(float) 1.78827593892108555290, (float) 0.95508210637394326553, (float)-0.18447626783899924429, (float)-0.44198126506275016437,
	(float)-0.88404052492547413497, (float)-1.42218907262407452967, (float)-1.02037566838362314995, (float)-0.34861755756425577264,
	(float)-0.11490230170431934434, (float) 0.12498899339968611803, (float) 0.38065885268563131927, (float) 0.31883491321310506562,
	(float) 0.10486838686563442765, (float)-0.03105361685110374845, (float)-0.06450524884075370758, (float)-0.02939198261121969816,

	(float) 1.78827593892108555290, (float) 0.95508210637394326553, (float)-0.18447626783899924429, (float)-0.44198126506275016437,
	(float)-0.88404052492547413497, (float)-1.42218907262407452967, (float)-1.02037566838362314995, (float)-0.34861755756425577264,
	(float)-0.11490230170431934434, (float) 0.12498899339968611803, (float) 0.38065885268563131927, (float) 0.31883491321310506562,
	(float) 0.10486838686563442765, (float)-0.03105361685110374845, (float)-0.06450524884075370758, (float)-0.02939198261121969816,
};


static const float  F44_3 [16 + 32] = {  /* SNR(w) = 15.382598 dB, SNR = -29.402334 dB */
	(float) 2.89072132015058161445, (float) 2.68932810943698754106, (float) 0.21083359339410251227, (float)-0.98385073324997617515,
	(float)-1.11047823227097316719, (float)-2.18954076314139673147, (float)-2.36498032881953056225, (float)-0.95484132880101140785,
	(float)-0.23924057925542965158, (float)-0.13865235703915925642, (float) 0.43587843191057992846, (float) 0.65903257226026665927,
	(float) 0.24361815372443152787, (float)-0.00235974960154720097, (float) 0.01844166574603346289, (float) 0.01722945988740875099,

	(float) 2.89072132015058161445, (float) 2.68932810943698754106, (float) 0.21083359339410251227, (float)-0.98385073324997617515,
	(float)-1.11047823227097316719, (float)-2.18954076314139673147, (float)-2.36498032881953056225, (float)-0.95484132880101140785,
	(float)-0.23924057925542965158, (float)-0.13865235703915925642, (float) 0.43587843191057992846, (float) 0.65903257226026665927,
	(float) 0.24361815372443152787, (float)-0.00235974960154720097, (float) 0.01844166574603346289, (float) 0.01722945988740875099,

	(float) 2.89072132015058161445, (float) 2.68932810943698754106, (float) 0.21083359339410251227, (float)-0.98385073324997617515,
	(float)-1.11047823227097316719, (float)-2.18954076314139673147, (float)-2.36498032881953056225, (float)-0.95484132880101140785,
	(float)-0.23924057925542965158, (float)-0.13865235703915925642, (float) 0.43587843191057992846, (float) 0.65903257226026665927,
	(float) 0.24361815372443152787, (float)-0.00235974960154720097, (float) 0.01844166574603346289, (float) 0.01722945988740875099
};


static double scalar16_(const float* x, const float* y)
{
	return
		x[ 0]*y[ 0] + x[ 1]*y[ 1] + x[ 2]*y[ 2] + x[ 3]*y[ 3] +
		x[ 4]*y[ 4] + x[ 5]*y[ 5] + x[ 6]*y[ 6] + x[ 7]*y[ 7] +
		x[ 8]*y[ 8] + x[ 9]*y[ 9] + x[10]*y[10] + x[11]*y[11] +
		x[12]*y[12] + x[13]*y[13] + x[14]*y[14] + x[15]*y[15];
}


void FLAC__replaygain_synthesis__init_dither_context(DitherContext *d, int bits, int shapingtype)
{
	static unsigned char default_dither [] = { 92, 92, 88, 84, 81, 78, 74, 67,  0,  0 };
	static const float*               F [] = { F44_0, F44_1, F44_2, F44_3 };

	int index;

	if (shapingtype < 0) shapingtype = 0;
	if (shapingtype > 3) shapingtype = 3;
	d->ShapingType = (NoiseShaping)shapingtype;
	index = bits - 11 - shapingtype;
	if (index < 0) index = 0;
	if (index > 9) index = 9;

	memset ( d->ErrorHistory , 0, sizeof (d->ErrorHistory ) );
	memset ( d->DitherHistory, 0, sizeof (d->DitherHistory) );

	d->FilterCoeff = F [shapingtype];
	d->Mask   = ((FLAC__uint64)-1) << (32 - bits);
	d->Add    = 0.5     * ((1L << (32 - bits)) - 1);
	d->Dither = 0.01f*default_dither[index] / (((FLAC__int64)1) << bits);
	d->LastHistoryIndex = 0;
}

/*
 * the following is based on parts of wavegain.c
 */

static FLaC__INLINE FLAC__int64 dither_output_(DitherContext *d, FLAC__bool do_dithering, int shapingtype, int i, double Sum, int k)
{
	union {
		double d;
		FLAC__int64 i;
	} doubletmp;
	double Sum2;
	FLAC__int64 val;

#define ROUND64(x)   ( doubletmp.d = (x) + d->Add + (FLAC__int64)FLAC__I64L(0x001FFFFD80000000), doubletmp.i - (FLAC__int64)FLAC__I64L(0x433FFFFD80000000) )

	if(do_dithering) {
		if(shapingtype == 0) {
			double  tmp = random_equi_(d->Dither);
			Sum2 = tmp - d->LastRandomNumber [k];
			d->LastRandomNumber [k] = (int)tmp;
			Sum2 = Sum += Sum2;
			val = ROUND64(Sum2) & d->Mask;
		}
		else {
			Sum2 = random_triangular_(d->Dither) - scalar16_(d->DitherHistory[k], d->FilterCoeff + i);
			Sum += d->DitherHistory [k] [(-1-i)&15] = (float)Sum2;
			Sum2 = Sum + scalar16_(d->ErrorHistory [k], d->FilterCoeff + i);
			val = ROUND64(Sum2) & d->Mask;
			d->ErrorHistory [k] [(-1-i)&15] = (float)(Sum - val);
		}
		return val;
	}
	else
		return ROUND64(Sum);

#undef ROUND64
}

#if 0
	float        peak = 0.f,
	             new_peak,
	             factor_clip
	double       scale,
	             dB;

	...

	peak is in the range -32768.0 .. 32767.0

	/* calculate factors for ReplayGain and ClippingPrevention */
	*track_gain = GetTitleGain() + settings->man_gain;
	scale = (float) pow(10., *track_gain * 0.05);
	if(settings->clip_prev) {
		factor_clip  = (float) (32767./( peak + 1));
		if(scale < factor_clip)
			factor_clip = 1.f;
		else
			factor_clip /= scale;
		scale *= factor_clip;
	}
	new_peak = (float) peak * scale;

	dB = 20. * log10(scale);
	*track_gain = (float) dB;

 	const double scale = pow(10., (double)gain * 0.05);
#endif


size_t FLAC__replaygain_synthesis__apply_gain(FLAC__byte *data_out, FLAC__bool little_endian_data_out, FLAC__bool unsigned_data_out, const FLAC__int32 * const input[], unsigned wide_samples, unsigned channels, const unsigned source_bps, const unsigned target_bps, const double scale, const FLAC__bool hard_limit, FLAC__bool do_dithering, DitherContext *dither_context)
{
	static const FLAC__int32 conv_factors_[33] = {
		-1, /* 0 bits-per-sample (not supported) */
		-1, /* 1 bits-per-sample (not supported) */
		-1, /* 2 bits-per-sample (not supported) */
		-1, /* 3 bits-per-sample (not supported) */
		268435456, /* 4 bits-per-sample */
		134217728, /* 5 bits-per-sample */
		67108864, /* 6 bits-per-sample */
		33554432, /* 7 bits-per-sample */
		16777216, /* 8 bits-per-sample */
		8388608, /* 9 bits-per-sample */
		4194304, /* 10 bits-per-sample */
		2097152, /* 11 bits-per-sample */
		1048576, /* 12 bits-per-sample */
		524288, /* 13 bits-per-sample */
		262144, /* 14 bits-per-sample */
		131072, /* 15 bits-per-sample */
		65536, /* 16 bits-per-sample */
		32768, /* 17 bits-per-sample */
		16384, /* 18 bits-per-sample */
		8192, /* 19 bits-per-sample */
		4096, /* 20 bits-per-sample */
		2048, /* 21 bits-per-sample */
		1024, /* 22 bits-per-sample */
		512, /* 23 bits-per-sample */
		256, /* 24 bits-per-sample */
		128, /* 25 bits-per-sample */
		64, /* 26 bits-per-sample */
		32, /* 27 bits-per-sample */
		16, /* 28 bits-per-sample */
		8, /* 29 bits-per-sample */
		4, /* 30 bits-per-sample */
		2, /* 31 bits-per-sample */
		1 /* 32 bits-per-sample */
	};
	static const FLAC__int64 hard_clip_factors_[33] = {
		0, /* 0 bits-per-sample (not supported) */
		0, /* 1 bits-per-sample (not supported) */
		0, /* 2 bits-per-sample (not supported) */
		0, /* 3 bits-per-sample (not supported) */
		-8, /* 4 bits-per-sample */
		-16, /* 5 bits-per-sample */
		-32, /* 6 bits-per-sample */
		-64, /* 7 bits-per-sample */
		-128, /* 8 bits-per-sample */
		-256, /* 9 bits-per-sample */
		-512, /* 10 bits-per-sample */
		-1024, /* 11 bits-per-sample */
		-2048, /* 12 bits-per-sample */
		-4096, /* 13 bits-per-sample */
		-8192, /* 14 bits-per-sample */
		-16384, /* 15 bits-per-sample */
		-32768, /* 16 bits-per-sample */
		-65536, /* 17 bits-per-sample */
		-131072, /* 18 bits-per-sample */
		-262144, /* 19 bits-per-sample */
		-524288, /* 20 bits-per-sample */
		-1048576, /* 21 bits-per-sample */
		-2097152, /* 22 bits-per-sample */
		-4194304, /* 23 bits-per-sample */
		-8388608, /* 24 bits-per-sample */
		-16777216, /* 25 bits-per-sample */
		-33554432, /* 26 bits-per-sample */
		-67108864, /* 27 bits-per-sample */
		-134217728, /* 28 bits-per-sample */
		-268435456, /* 29 bits-per-sample */
		-536870912, /* 30 bits-per-sample */
		-1073741824, /* 31 bits-per-sample */
		(FLAC__int64)(-1073741824) * 2 /* 32 bits-per-sample */
	};
	const FLAC__int32 conv_factor = conv_factors_[target_bps];
	const FLAC__int64 hard_clip_factor = hard_clip_factors_[target_bps];
	/*
	 * The integer input coming in has a varying range based on the
	 * source_bps.  We want to normalize it to [-1.0, 1.0) so instead
	 * of doing two multiplies on each sample, we just multiple
	 * 'scale' by 1/(2^(source_bps-1))
	 */
	const double multi_scale = scale / (double)(1u << (source_bps-1));

	FLAC__byte * const start = data_out;
	unsigned i, channel;
	const FLAC__int32 *input_;
	double sample;
	const unsigned bytes_per_sample = target_bps / 8;
	const unsigned last_history_index = dither_context->LastHistoryIndex;
	NoiseShaping noise_shaping = dither_context->ShapingType;
	FLAC__int64 val64;
	FLAC__int32 val32;
	FLAC__int32 uval32;
	const FLAC__uint32 twiggle = 1u << (target_bps - 1);

	FLAC__ASSERT(channels > 0 && channels <= FLAC_SHARE__MAX_SUPPORTED_CHANNELS);
	FLAC__ASSERT(source_bps >= 4);
	FLAC__ASSERT(target_bps >= 4);
	FLAC__ASSERT(source_bps <= 32);
	FLAC__ASSERT(target_bps < 32);
	FLAC__ASSERT((target_bps & 7) == 0);

	for(channel = 0; channel < channels; channel++) {
		const unsigned incr = bytes_per_sample * channels;
		data_out = start + bytes_per_sample * channel;
		input_ = input[channel];
		for(i = 0; i < wide_samples; i++, data_out += incr) {
			sample = (double)input_[i] * multi_scale;

			if(hard_limit) {
				/* hard 6dB limiting */
				if(sample < -0.5)
					sample = tanh((sample + 0.5) / (1-0.5)) * (1-0.5) - 0.5;
				else if(sample > 0.5)
					sample = tanh((sample - 0.5) / (1-0.5)) * (1-0.5) + 0.5;
			}
			sample *= 2147483647.f;

			val64 = dither_output_(dither_context, do_dithering, noise_shaping, (i + last_history_index) % 32, sample, channel) / conv_factor;

			val32 = (FLAC__int32)val64;
			if(val64 >= -hard_clip_factor)
				val32 = (FLAC__int32)(-(hard_clip_factor+1));
			else if(val64 < hard_clip_factor)
				val32 = (FLAC__int32)hard_clip_factor;

			uval32 = (FLAC__uint32)val32;
			if (unsigned_data_out)
				uval32 ^= twiggle;

			if (little_endian_data_out) {
				switch(target_bps) {
					case 24:
						data_out[2] = (FLAC__byte)(uval32 >> 16);
						/* fall through */
					case 16:
						data_out[1] = (FLAC__byte)(uval32 >> 8);
						/* fall through */
					case 8:
						data_out[0] = (FLAC__byte)uval32;
						break;
				}
			}
			else {
				switch(target_bps) {
					case 24:
						data_out[0] = (FLAC__byte)(uval32 >> 16);
						data_out[1] = (FLAC__byte)(uval32 >> 8);
						data_out[2] = (FLAC__byte)uval32;
						break;
					case 16:
						data_out[0] = (FLAC__byte)(uval32 >> 8);
						data_out[1] = (FLAC__byte)uval32;
						break;
					case 8:
						data_out[0] = (FLAC__byte)uval32;
						break;
				}
			}
		}
	}
	dither_context->LastHistoryIndex = (last_history_index + wide_samples) % 32;

	return wide_samples * channels * (target_bps/8);
}
