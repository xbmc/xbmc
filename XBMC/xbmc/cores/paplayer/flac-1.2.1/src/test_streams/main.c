/* test_streams - Simple test pattern generator
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#if defined _MSC_VER || defined __MINGW32__
#include <time.h>
#else
#include <sys/time.h>
#endif
#include "FLAC/assert.h"
#include "FLAC/ordinals.h"

#ifndef M_PI
/* math.h in VC++ doesn't seem to have this (how Microsoft is that?) */
#define M_PI 3.14159265358979323846
#endif

#if !defined _MSC_VER && !defined __MINGW32__
#define GET_RANDOM_BYTE (((unsigned)random()) & 0xff)
#else
#define GET_RANDOM_BYTE (((unsigned)rand()) & 0xff)
#endif

static FLAC__bool is_big_endian_host;


static FLAC__bool write_little_endian(FILE *f, FLAC__int32 x, size_t bytes)
{
	while(bytes) {
		if(fputc(x, f) == EOF)
			return false;
		x >>= 8;
		bytes--;
	}
	return true;
}

static FLAC__bool write_little_endian_uint16(FILE *f, FLAC__uint16 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF
	;
}

static FLAC__bool write_little_endian_int16(FILE *f, FLAC__int16 x)
{
	return write_little_endian_uint16(f, (FLAC__uint16)x);
}

static FLAC__bool write_little_endian_uint24(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF
	;
}

static FLAC__bool write_little_endian_int24(FILE *f, FLAC__int32 x)
{
	return write_little_endian_uint24(f, (FLAC__uint32)x);
}

static FLAC__bool write_little_endian_uint32(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 24, f) != EOF
	;
}

#if 0
/* @@@ not used (yet) */
static FLAC__bool write_little_endian_int32(FILE *f, FLAC__int32 x)
{
	return write_little_endian_uint32(f, (FLAC__uint32)x);
}
#endif

static FLAC__bool write_big_endian(FILE *f, FLAC__int32 x, size_t bytes)
{
	if(bytes < 4)
		x <<= 8*(4-bytes);
	while(bytes) {
		if(fputc(x>>24, f) == EOF)
			return false;
		x <<= 8;
		bytes--;
	}
	return true;
}

static FLAC__bool write_big_endian_uint16(FILE *f, FLAC__uint16 x)
{
	return
		fputc(x >> 8, f) != EOF &&
		fputc(x, f) != EOF
	;
}

#if 0
/* @@@ not used (yet) */
static FLAC__bool write_big_endian_int16(FILE *f, FLAC__int16 x)
{
	return write_big_endian_uint16(f, (FLAC__uint16)x);
}
#endif

#if 0
/* @@@ not used (yet) */
static FLAC__bool write_big_endian_uint24(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x, f) != EOF
	;
}
#endif

#if 0
/* @@@ not used (yet) */
static FLAC__bool write_big_endian_int24(FILE *f, FLAC__int32 x)
{
	return write_big_endian_uint24(f, (FLAC__uint32)x);
}
#endif

static FLAC__bool write_big_endian_uint32(FILE *f, FLAC__uint32 x)
{
	return
		fputc(x >> 24, f) != EOF &&
		fputc(x >> 16, f) != EOF &&
		fputc(x >> 8, f) != EOF &&
		fputc(x, f) != EOF
	;
}

#if 0
/* @@@ not used (yet) */
static FLAC__bool write_big_endian_int32(FILE *f, FLAC__int32 x)
{
	return write_big_endian_uint32(f, (FLAC__uint32)x);
}
#endif

static FLAC__bool write_sane_extended(FILE *f, unsigned val)
	/* Write to 'f' a SANE extended representation of 'val'.  Return false if
	* the write succeeds; return true otherwise.
	*
	* SANE extended is an 80-bit IEEE-754 representation with sign bit, 15 bits
	* of exponent, and 64 bits of significand (mantissa).  Unlike most IEEE-754
	* representations, it does not imply a 1 above the MSB of the significand.
	*
	* Preconditions:
	*  val!=0U
	*/
{
	unsigned int shift, exponent;

	FLAC__ASSERT(val!=0U); /* handling 0 would require a special case */

	for(shift= 0U; (val>>(31-shift))==0U; ++shift)
		;
	val<<= shift;
	exponent= 63U-(shift+32U); /* add 32 for unused second word */

	if(!write_big_endian_uint16(f, (FLAC__uint16)(exponent+0x3FFF)))
		return false;
	if(!write_big_endian_uint32(f, val))
		return false;
	if(!write_big_endian_uint32(f, 0)) /* unused second word */
		return false;

	return true;
}

/* a mono one-sample 16bps stream */
static FLAC__bool generate_01(void)
{
	FILE *f;
	FLAC__int16 x = -32768;

	if(0 == (f = fopen("test01.raw", "wb")))
		return false;

	if(!write_little_endian_int16(f, x))
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo one-sample 16bps stream */
static FLAC__bool generate_02(void)
{
	FILE *f;
	FLAC__int16 xl = -32768, xr = 32767;

	if(0 == (f = fopen("test02.raw", "wb")))
		return false;

	if(!write_little_endian_int16(f, xl))
		goto foo;
	if(!write_little_endian_int16(f, xr))
		goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono five-sample 16bps stream */
static FLAC__bool generate_03(void)
{
	FILE *f;
	FLAC__int16 x[] = { -25, 0, 25, 50, 100 };
	unsigned i;

	if(0 == (f = fopen("test03.raw", "wb")))
		return false;

	for(i = 0; i < 5; i++)
		if(!write_little_endian_int16(f, x[i]))
			goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo five-sample 16bps stream */
static FLAC__bool generate_04(void)
{
	FILE *f;
	FLAC__int16 x[] = { -25, 500, 0, 400, 25, 300, 50, 200, 100, 100 };
	unsigned i;

	if(0 == (f = fopen("test04.raw", "wb")))
		return false;

	for(i = 0; i < 10; i++)
		if(!write_little_endian_int16(f, x[i]))
			goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 8bps stream */
static FLAC__bool generate_fsd8(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	FLAC__ASSERT(pattern != 0);

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			signed char x = pattern[p] > 0? 127 : -128;
			if(fwrite(&x, sizeof(x), 1, f) < 1)
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 16bps stream */
static FLAC__bool generate_fsd16(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	FLAC__ASSERT(pattern != 0);

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			FLAC__int16 x = pattern[p] > 0? 32767 : -32768;
			if(!write_little_endian_int16(f, x))
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo wasted-bits-per-sample 16bps stream */
static FLAC__bool generate_wbps16(const char *fn, unsigned samples)
{
	FILE *f;
	unsigned sample;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(sample = 0; sample < samples; sample++) {
		FLAC__int16 l = (sample % 2000) << 2;
		FLAC__int16 r = (sample % 1000) << 3;
		if(!write_little_endian_int16(f, l))
			goto foo;
		if(!write_little_endian_int16(f, r))
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono full-scale deflection 24bps stream */
static FLAC__bool generate_fsd24(const char *fn, const int pattern[], unsigned reps)
{
	FILE *f;
	unsigned rep, p;

	FLAC__ASSERT(pattern != 0);

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(rep = 0; rep < reps; rep++) {
		for(p = 0; pattern[p]; p++) {
			FLAC__int32 x = pattern[p] > 0? 8388607 : -8388608;
			if(!write_little_endian_int24(f, x))
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono sine-wave 8bps stream */
static FLAC__bool generate_sine8_1(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const FLAC__int8 full_scale = 127;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int8 v = (FLAC__int8)(val + 0.5);
		if(fwrite(&v, sizeof(v), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo sine-wave 8bps stream */
static FLAC__bool generate_sine8_2(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2, double fmult)
{
	const FLAC__int8 full_scale = 127;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int8 v = (FLAC__int8)(val + 0.5);
		if(fwrite(&v, sizeof(v), 1, f) < 1)
			goto foo;
		val = -(a1*sin(theta1*fmult) + a2*sin(theta2*fmult))*(double)full_scale;
		v = (FLAC__int8)(val + 0.5);
		if(fwrite(&v, sizeof(v), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono sine-wave 16bps stream */
static FLAC__bool generate_sine16_1(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const FLAC__int16 full_scale = 32767;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int16 v = (FLAC__int16)(val + 0.5);
		if(!write_little_endian_int16(f, v))
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo sine-wave 16bps stream */
static FLAC__bool generate_sine16_2(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2, double fmult)
{
	const FLAC__int16 full_scale = 32767;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int16 v = (FLAC__int16)(val + 0.5);
		if(!write_little_endian_int16(f, v))
			goto foo;
		val = -(a1*sin(theta1*fmult) + a2*sin(theta2*fmult))*(double)full_scale;
		v = (FLAC__int16)(val + 0.5);
		if(!write_little_endian_int16(f, v))
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a mono sine-wave 24bps stream */
static FLAC__bool generate_sine24_1(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2)
{
	const FLAC__int32 full_scale = 0x7fffff;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int32 v = (FLAC__int32)(val + 0.5);
		if(!write_little_endian_int24(f, v))
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

/* a stereo sine-wave 24bps stream */
static FLAC__bool generate_sine24_2(const char *fn, const double sample_rate, const unsigned samples, const double f1, const double a1, const double f2, const double a2, double fmult)
{
	const FLAC__int32 full_scale = 0x7fffff;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	FILE *f;
	double theta1, theta2;
	unsigned i;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
		FLAC__int32 v = (FLAC__int32)(val + 0.5);
		if(!write_little_endian_int24(f, v))
			goto foo;
		val = -(a1*sin(theta1*fmult) + a2*sin(theta2*fmult))*(double)full_scale;
		v = (FLAC__int32)(val + 0.5);
		if(!write_little_endian_int24(f, v))
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static FLAC__bool generate_noise(const char *fn, unsigned bytes)
{
	FILE *f;
	unsigned b;

	if(0 == (f = fopen(fn, "wb")))
		return false;

	for(b = 0; b < bytes; b++) {
#if !defined _MSC_VER && !defined __MINGW32__
		FLAC__byte x = (FLAC__byte)(((unsigned)random()) & 0xff);
#else
		FLAC__byte x = (FLAC__byte)(((unsigned)rand()) & 0xff);
#endif
		if(fwrite(&x, sizeof(x), 1, f) < 1)
			goto foo;
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static FLAC__bool generate_raw(const char *filename, unsigned channels, unsigned bytes_per_sample, unsigned samples)
{
	const FLAC__int32 full_scale = (1 << (bytes_per_sample*8-1)) - 1;
	const double f1 = 441.0, a1 = 0.61, f2 = 661.5, a2 = 0.37;
	const double delta1 = 2.0 * M_PI / ( 44100.0 / f1);
	const double delta2 = 2.0 * M_PI / ( 44100.0 / f2);
	double theta1, theta2;
	FILE *f;
	unsigned i, j;

	if(0 == (f = fopen(filename, "wb")))
		return false;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		for(j = 0; j < channels; j++) {
			double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
			FLAC__int32 v = (FLAC__int32)(val + 0.5) + ((GET_RANDOM_BYTE>>4)-8);
			if(!write_little_endian(f, v, bytes_per_sample))
				goto foo;
		}
	}

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static FLAC__bool generate_aiff(const char *filename, unsigned sample_rate, unsigned channels, unsigned bps, unsigned samples)
{
	const unsigned bytes_per_sample = (bps+7)/8;
	const unsigned true_size = channels * bytes_per_sample * samples;
	const unsigned padded_size = (true_size + 1) & (~1u);
	const unsigned shift = (bps%8)? 8 - (bps%8) : 0;
	const FLAC__int32 full_scale = (1 << (bps-1)) - 1;
	const double f1 = 441.0, a1 = 0.61, f2 = 661.5, a2 = 0.37;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	double theta1, theta2;
	FILE *f;
	unsigned i, j;

	if(0 == (f = fopen(filename, "wb")))
		return false;
	if(fwrite("FORM", 1, 4, f) < 4)
		goto foo;
	if(!write_big_endian_uint32(f, padded_size + 46))
		goto foo;
	if(fwrite("AIFFCOMM\000\000\000\022", 1, 12, f) < 12)
		goto foo;
	if(!write_big_endian_uint16(f, (FLAC__uint16)channels))
		goto foo;
	if(!write_big_endian_uint32(f, samples))
		goto foo;
	if(!write_big_endian_uint16(f, (FLAC__uint16)bps))
		goto foo;
	if(!write_sane_extended(f, sample_rate))
		goto foo;
	if(fwrite("SSND", 1, 4, f) < 4)
		goto foo;
	if(!write_big_endian_uint32(f, true_size + 8))
		goto foo;
	if(fwrite("\000\000\000\000\000\000\000\000", 1, 8, f) < 8)
		goto foo;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		for(j = 0; j < channels; j++) {
			double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
			FLAC__int32 v = ((FLAC__int32)(val + 0.5) + ((GET_RANDOM_BYTE>>4)-8)) << shift;
			if(!write_big_endian(f, v, bytes_per_sample))
				goto foo;
		}
	}
	for(i = true_size; i < padded_size; i++)
		if(fputc(0, f) == EOF)
			goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static FLAC__bool generate_wav(const char *filename, unsigned sample_rate, unsigned channels, unsigned bps, unsigned samples, FLAC__bool strict)
{
	const FLAC__bool waveformatextensible = strict && (channels > 2 || (bps%8));
	/*                                                                 ^^^^^^^
	 * (bps%8) allows 24 bps which is technically supposed to be WAVEFORMATEXTENSIBLE but we
	 * write 24bps as WAVEFORMATEX since it's unambiguous and matches how flac writes it
	 */
	const unsigned bytes_per_sample = (bps+7)/8;
	const unsigned true_size = channels * bytes_per_sample * samples;
	const unsigned padded_size = (true_size + 1) & (~1u);
	const unsigned shift = (bps%8)? 8 - (bps%8) : 0;
	const FLAC__int32 full_scale = (1 << (bps-1)) - 1;
	const double f1 = 441.0, a1 = 0.61, f2 = 661.5, a2 = 0.37;
	const double delta1 = 2.0 * M_PI / ( sample_rate / f1);
	const double delta2 = 2.0 * M_PI / ( sample_rate / f2);
	double theta1, theta2;
	FILE *f;
	unsigned i, j;

	if(0 == (f = fopen(filename, "wb")))
		return false;
	if(fwrite("RIFF", 1, 4, f) < 4)
		goto foo;
	if(!write_little_endian_uint32(f, padded_size + (waveformatextensible?60:36)))
		goto foo;
	if(fwrite("WAVEfmt ", 1, 8, f) < 8)
		goto foo;
	if(!write_little_endian_uint32(f, waveformatextensible?40:16))
		goto foo;
	if(!write_little_endian_uint16(f, (FLAC__uint16)(waveformatextensible?65534:1)))
		goto foo;
	if(!write_little_endian_uint16(f, (FLAC__uint16)channels))
		goto foo;
	if(!write_little_endian_uint32(f, sample_rate))
		goto foo;
	if(!write_little_endian_uint32(f, sample_rate * channels * bytes_per_sample))
		goto foo;
	if(!write_little_endian_uint16(f, (FLAC__uint16)(channels * bytes_per_sample))) /* block align */
		goto foo;
	if(!write_little_endian_uint16(f, (FLAC__uint16)(bps+shift)))
		goto foo;
	if(waveformatextensible) {
		if(!write_little_endian_uint16(f, (FLAC__uint16)22)) /* cbSize */
			goto foo;
		if(!write_little_endian_uint16(f, (FLAC__uint16)bps)) /* validBitsPerSample */
			goto foo;
		if(!write_little_endian_uint32(f, 0)) /* channelMask */
			goto foo;
		/* GUID = {0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}} */
		if(fwrite("\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71", 1, 16, f) != 16)
			goto foo;
	}
	if(fwrite("data", 1, 4, f) < 4)
		goto foo;
	if(!write_little_endian_uint32(f, true_size))
		goto foo;

	for(i = 0, theta1 = theta2 = 0.0; i < samples; i++, theta1 += delta1, theta2 += delta2) {
		for(j = 0; j < channels; j++) {
			double val = (a1*sin(theta1) + a2*sin(theta2))*(double)full_scale;
			FLAC__int32 v = ((FLAC__int32)(val + 0.5) + ((GET_RANDOM_BYTE>>4)-8)) << shift;
			if(!write_little_endian(f, v, bytes_per_sample))
				goto foo;
		}
	}
	for(i = true_size; i < padded_size; i++)
		if(fputc(0, f) == EOF)
			goto foo;

	fclose(f);
	return true;
foo:
	fclose(f);
	return false;
}

static FLAC__bool generate_wackywavs(void)
{
	FILE *f;
	FLAC__byte wav[] = {
		'R', 'I', 'F', 'F',  76,   0,   0,   0,
		'W', 'A', 'V', 'E', 'f', 'a', 'c', 't',
		  4,   0,   0,  0 , 'b', 'l', 'a', 'h',
		'p', 'a', 'd', ' ',   4,   0,   0,   0,
		'B', 'L', 'A', 'H', 'f', 'm', 't', ' ',
		 16,   0,   0,   0,   1,   0,   1,   0,
		0x44,0xAC,  0,   0,   0,   0,   0,   0,
		  2,   0,  16,   0, 'd', 'a', 't', 'a',
		 16,   0,   0,   0,   0,   0,   1,   0,
		  4,   0,   9,   0,  16,   0,  25,   0,
		 36,   0,  49,   0, 'p', 'a', 'd', ' ',
		  4,   0,   0,   0, 'b', 'l', 'a', 'h'
	};

	if(0 == (f = fopen("wacky1.wav", "wb")))
		return false;
	if(fwrite(wav, 1, 84, f) < 84)
		goto foo;
	fclose(f);

	wav[4] += 12;
	if(0 == (f = fopen("wacky2.wav", "wb")))
		return false;
	if(fwrite(wav, 1, 96, f) < 96)
		goto foo;
	fclose(f);

	return true;
foo:
	fclose(f);
	return false;
}

int main(int argc, char *argv[])
{
	FLAC__uint32 test = 1;
	unsigned channels;

	int pattern01[] = { 1, -1, 0 };
	int pattern02[] = { 1, 1, -1, 0 };
	int pattern03[] = { 1, -1, -1, 0 };
	int pattern04[] = { 1, -1, 1, -1, 0 };
	int pattern05[] = { 1, -1, -1, 1, 0 };
	int pattern06[] = { 1, -1, 1, 1, -1, 0 };
	int pattern07[] = { 1, -1, -1, 1, -1, 0 };

	(void)argc;
	(void)argv;
	is_big_endian_host = (*((FLAC__byte*)(&test)))? false : true;

#if !defined _MSC_VER && !defined __MINGW32__
	{
		struct timeval tv;

		if(gettimeofday(&tv, 0) < 0) {
			fprintf(stderr, "WARNING: couldn't seed RNG with time\n");
			tv.tv_usec = 4321;
		}
		srandom(tv.tv_usec);
	}
#else
	srand((unsigned)time(0));
#endif

	if(!generate_01()) return 1;
	if(!generate_02()) return 1;
	if(!generate_03()) return 1;
	if(!generate_04()) return 1;

	if(!generate_fsd8("fsd8-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd8("fsd8-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd8("fsd8-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd8("fsd8-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd8("fsd8-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd8("fsd8-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd8("fsd8-07.raw", pattern07, 100)) return 1;

	if(!generate_fsd16("fsd16-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd16("fsd16-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd16("fsd16-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd16("fsd16-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd16("fsd16-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd16("fsd16-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd16("fsd16-07.raw", pattern07, 100)) return 1;

	if(!generate_fsd24("fsd24-01.raw", pattern01, 100)) return 1;
	if(!generate_fsd24("fsd24-02.raw", pattern02, 100)) return 1;
	if(!generate_fsd24("fsd24-03.raw", pattern03, 100)) return 1;
	if(!generate_fsd24("fsd24-04.raw", pattern04, 100)) return 1;
	if(!generate_fsd24("fsd24-05.raw", pattern05, 100)) return 1;
	if(!generate_fsd24("fsd24-06.raw", pattern06, 100)) return 1;
	if(!generate_fsd24("fsd24-07.raw", pattern07, 100)) return 1;

	if(!generate_wbps16("wbps16-01.raw", 1000)) return 1;

	if(!generate_sine8_1("sine8-00.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine8_1("sine8-01.raw", 96000.0, 200000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine8_1("sine8-02.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine8_1("sine8-03.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine8_1("sine8-04.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	if(!generate_sine8_2("sine8-10.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49, 1.0)) return 1;
	if(!generate_sine8_2("sine8-11.raw", 48000.0, 200000, 441.0, 0.61, 661.5, 0.37, 1.0)) return 1;
	if(!generate_sine8_2("sine8-12.raw", 96000.0, 200000, 441.0, 0.50, 882.0, 0.49, 1.0)) return 1;
	if(!generate_sine8_2("sine8-13.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.0)) return 1;
	if(!generate_sine8_2("sine8-14.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 1.0)) return 1;
	if(!generate_sine8_2("sine8-15.raw", 44100.0, 200000, 441.0, 0.50, 441.0, 0.49, 0.5)) return 1;
	if(!generate_sine8_2("sine8-16.raw", 44100.0, 200000, 441.0, 0.61, 661.5, 0.37, 2.0)) return 1;
	if(!generate_sine8_2("sine8-17.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49, 0.7)) return 1;
	if(!generate_sine8_2("sine8-18.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.3)) return 1;
	if(!generate_sine8_2("sine8-19.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 0.1)) return 1;

	if(!generate_sine16_1("sine16-00.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-01.raw", 96000.0, 200000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine16_1("sine16-02.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-03.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine16_1("sine16-04.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	if(!generate_sine16_2("sine16-10.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-11.raw", 48000.0, 200000, 441.0, 0.61, 661.5, 0.37, 1.0)) return 1;
	if(!generate_sine16_2("sine16-12.raw", 96000.0, 200000, 441.0, 0.50, 882.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-13.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.0)) return 1;
	if(!generate_sine16_2("sine16-14.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 1.0)) return 1;
	if(!generate_sine16_2("sine16-15.raw", 44100.0, 200000, 441.0, 0.50, 441.0, 0.49, 0.5)) return 1;
	if(!generate_sine16_2("sine16-16.raw", 44100.0, 200000, 441.0, 0.61, 661.5, 0.37, 2.0)) return 1;
	if(!generate_sine16_2("sine16-17.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49, 0.7)) return 1;
	if(!generate_sine16_2("sine16-18.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.3)) return 1;
	if(!generate_sine16_2("sine16-19.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 0.1)) return 1;

	if(!generate_sine24_1("sine24-00.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-01.raw", 96000.0, 200000, 441.0, 0.61, 661.5, 0.37)) return 1;
	if(!generate_sine24_1("sine24-02.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-03.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49)) return 1;
	if(!generate_sine24_1("sine24-04.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29)) return 1;

	if(!generate_sine24_2("sine24-10.raw", 48000.0, 200000, 441.0, 0.50, 441.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-11.raw", 48000.0, 200000, 441.0, 0.61, 661.5, 0.37, 1.0)) return 1;
	if(!generate_sine24_2("sine24-12.raw", 96000.0, 200000, 441.0, 0.50, 882.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-13.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.0)) return 1;
	if(!generate_sine24_2("sine24-14.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 1.0)) return 1;
	if(!generate_sine24_2("sine24-15.raw", 44100.0, 200000, 441.0, 0.50, 441.0, 0.49, 0.5)) return 1;
	if(!generate_sine24_2("sine24-16.raw", 44100.0, 200000, 441.0, 0.61, 661.5, 0.37, 2.0)) return 1;
	if(!generate_sine24_2("sine24-17.raw", 44100.0, 200000, 441.0, 0.50, 882.0, 0.49, 0.7)) return 1;
	if(!generate_sine24_2("sine24-18.raw", 44100.0, 200000, 441.0, 0.50, 4410.0, 0.49, 1.3)) return 1;
	if(!generate_sine24_2("sine24-19.raw", 44100.0, 200000, 8820.0, 0.70, 4410.0, 0.29, 0.1)) return 1;

	/* WATCHOUT: the size of noise.raw is hardcoded into test/test_flac.sh */
	if(!generate_noise("noise.raw", 65536 * 8 * 3)) return 1;
	if(!generate_noise("noise8m32.raw", 32)) return 1;
	if(!generate_wackywavs()) return 1;
	for(channels = 1; channels <= 8; channels++) {
		unsigned bits_per_sample;
		for(bits_per_sample = 4; bits_per_sample <= 24; bits_per_sample++) {
			static const unsigned nsamples[] = { 1, 111, 4777 } ;
			unsigned samples;
			for(samples = 0; samples < sizeof(nsamples)/sizeof(nsamples[0]); samples++) {
				char fn[64];

				sprintf(fn, "rt-%u-%u-%u.aiff", channels, bits_per_sample, nsamples[samples]);
				if(!generate_aiff(fn, 44100, channels, bits_per_sample, nsamples[samples]))
					return 1;

				sprintf(fn, "rt-%u-%u-%u.wav", channels, bits_per_sample, nsamples[samples]);
				if(!generate_wav(fn, 44100, channels, bits_per_sample, nsamples[samples], /*strict=*/true))
					return 1;

				if(bits_per_sample % 8 == 0) {
					sprintf(fn, "rt-%u-%u-%u.raw", channels, bits_per_sample, nsamples[samples]);
					if(!generate_raw(fn, channels, bits_per_sample/8, nsamples[samples]))
						return 1;
				}
			}
		}
	}

	return 0;
}
