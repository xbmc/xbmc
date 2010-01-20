/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * REVERB EFFECT FOR TIMIDITY++-1.X (Version 0.06e  1999/1/28)
 * 
 * Copyright (C) 1997,1998,1999  Masaki Kiryu <mkiryu@usa.net>
 *                           (http://w3mb.kcom.ne.jp/~mkiryu/)
 *
 * reverb.c  -- main reverb engine.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "timidity.h"
#include "controls.h"
#include "tables.h"
#include "common.h"
#include "output.h"
#include "reverb.h"
#include "mt19937ar.h"
#include <math.h>
#include <stdlib.h>

#define SYS_EFFECT_PRE_LPF

/* #define SYS_EFFECT_CLIP */
#ifdef SYS_EFFECT_CLIP
#define CLIP_AMP_MAX (1L << (32 - GUARD_BITS))
#define CLIP_AMP_MIN (-1L << (32 - GUARD_BITS))
#endif /* SYS_EFFECT_CLIP */

static double REV_INP_LEV = 1.0;
#define MASTER_CHORUS_LEVEL 1.7
#define MASTER_DELAY_LEVEL 3.25

/*              */
/*  Dry Signal  */
/*              */
static int32 direct_buffer[AUDIO_BUFFER_SIZE * 2];
static int32 direct_bufsize = sizeof(direct_buffer);

#if OPT_MODE != 0 && ( defined(_MSC_VER) || defined(__WATCOMC__)|| (defined(__BORLANDC__) && (__BORLANDC__ >= 1380)) )
void set_dry_signal(int32 *buf, int32 count)
{
	int32 *dbuf = direct_buffer;
	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		mov		ebx, [edi]
		add		esi, 4
		add		ebx, eax
		mov		[edi], ebx
		add		edi, 4
		dec		ecx
		jnz		L1
L2:
	}
}
#else
void set_dry_signal(register int32 *buf, int32 n)
{
#if USE_ALTIVEC
  if(is_altivec_available()) {
    v_set_dry_signal(direct_buffer, buf, n);
  } else {
#endif
    register int32 i;
	register int32 *dbuf = direct_buffer;

    for(i = n - 1; i >= 0; i--)
    {
        dbuf[i] += buf[i];
    }
#if USE_ALTIVEC
  }
#endif
}
#endif

/* XG has "dry level". */
#if OPT_MODE != 0	/* fixed-point implementation */
#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && (__BORLANDC__ >= 1380))
void set_dry_signal_xg(int32 *buf, int32 count, int32 level)
{
	int32 *dbuf = direct_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		mov		ebx, [level]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		imul	ebx
		shr		eax, 16
		shl		edx, 16
		or		eax, edx	/* u */
		mov		edx, [edi]	/* v */
		add		esi, 4		/* u */	
		add		edx, eax	/* v */
		mov		[edi], edx	/* u */
		add		edi, 4		/* v */
		dec		ecx			/* u */
		jnz		L1			/* v */
L2:
	}
}
#else
void set_dry_signal_xg(register int32 *sbuffer, int32 n, int32 level)
{
    register int32 i;
	int32 *buf = direct_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	for(i = n - 1; i >= 0; i--) {buf[i] += imuldiv16(sbuffer[i], level);}
}
#endif	/* _MSC_VER */
#else	/* floating-point implementation */
void set_dry_signal_xg(register int32 *sbuffer, int32 n, int32 level)
{
    register int32 i;
    register int32 count = n;
	if(!level) {return;}
    FLOAT_T send_level = (FLOAT_T)level / 127.0;

    for(i = 0; i < count; i++)
    {
		direct_buffer[i] += sbuffer[i] * send_level;
    }
}
#endif /* OPT_MODE != 0 */

#ifdef SYS_EFFECT_CLIP
void mix_dry_signal(int32 *buf, int32 n)
{
	int32 i, x;
	for (i = 0; i < n; i++) {
		x = direct_buffer[i];
		buf[i] = (x > CLIP_AMP_MAX) ? CLIP_AMP_MAX
				: (x < CLIP_AMP_MIN) ? CLIP_AMP_MIN : x;
	}
	memset(direct_buffer, 0, sizeof(int32) * n);
}
#else /* SYS_EFFECT_CLIP */
void mix_dry_signal(int32 *buf, int32 n)
{
 	memcpy(buf, direct_buffer, sizeof(int32) * n);
	memset(direct_buffer, 0, sizeof(int32) * n);
}
#endif /* SYS_EFFECT_CLIP */

/*                    */
/*  Effect Utilities  */
/*                    */
static inline int isprime(int val)
{
	int i;
	if (val == 2) {return 1;}
	if (val & 1) {
		for (i = 3; i < (int)sqrt((double)val) + 1; i += 2) {
			if ((val % i) == 0) {return 0;}
		}
		return 1; /* prime */
	} else {return 0;} /* even */
}

/*! delay */
static void free_delay(delay *delay)
{
	if(delay->buf != NULL) {
		free(delay->buf);
		delay->buf = NULL;
	}
}

static void set_delay(delay *delay, int32 size)
{
	if(size < 1) {size = 1;} 
	free_delay(delay);
	delay->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(delay->buf == NULL) {return;}
	delay->index = 0;
	delay->size = size;
	memset(delay->buf, 0, sizeof(int32) * delay->size);
}

static inline void do_delay(int32 *stream, int32 *buf, int32 size, int32 *index)
{
	int32 output;
	output = buf[*index];
	buf[*index] = *stream;
	if (++*index >= size) {*index = 0;}
	*stream = output;
}

/*! LFO (low frequency oscillator) */
static void init_lfo(lfo *lfo, double freq, int type, double phase)
{
	int32 i, cycle, diff;

	lfo->count = 0;
	lfo->freq = freq;
	if (lfo->freq < 0.05f) {lfo->freq = 0.05f;}
	cycle = (double)play_mode->rate / lfo->freq;
	if (cycle < 1) {cycle = 1;}
	lfo->cycle = cycle;
	lfo->icycle = TIM_FSCALE((SINE_CYCLE_LENGTH - 1) / (double)cycle, 24) - 0.5;
	diff = SINE_CYCLE_LENGTH * phase / 360.0f;

	if(lfo->type != type) {	/* generate LFO waveform */
		switch(type) {
		case LFO_SINE:
			for(i = 0; i < SINE_CYCLE_LENGTH; i++)
				lfo->buf[i] = TIM_FSCALE((lookup_sine(i + diff) + 1.0) / 2.0, 16);
			break;
		case LFO_TRIANGULAR:
			for(i = 0; i < SINE_CYCLE_LENGTH; i++)
				lfo->buf[i] = TIM_FSCALE((lookup_triangular(i + diff) + 1.0) / 2.0, 16);
			break;
		default:
			for(i = 0; i < SINE_CYCLE_LENGTH; i++) {lfo->buf[i] = TIM_FSCALE(0.5, 16);}
			break;
		}
	}
	lfo->type = type;
}

/* returned value is between 0 and (1 << 16) */
static inline int32 do_lfo(lfo *lfo)
{
	int32 val;
	val = lfo->buf[imuldiv24(lfo->count, lfo->icycle)];
	if(++lfo->count == lfo->cycle) {lfo->count = 0;}
	return val;
}

/*! modulated delay with allpass interpolation (for Chorus Effect,...) */
static void free_mod_delay(mod_delay *delay)
{
	if(delay->buf != NULL) {
		free(delay->buf);
		delay->buf = NULL;
	}
}

static void set_mod_delay(mod_delay *delay, int32 ndelay, int32 depth)
{
	int32 size = ndelay + depth + 1;
	free_mod_delay(delay);
	delay->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(delay->buf == NULL) {return;}
	delay->rindex = 0;
	delay->windex = 0;
	delay->hist = 0;
	delay->ndelay = ndelay;
	delay->depth = depth;
	delay->size = size;
	memset(delay->buf, 0, sizeof(int32) * delay->size);
}

static inline void do_mod_delay(int32 *stream, int32 *buf, int32 size, int32 *rindex, int32 *windex,
								int32 ndelay, int32 depth, int32 lfoval, int32 *hist)
{
	int32 t1, t2;
	if (++*windex == size) {*windex = 0;}
	t1 = buf[*rindex];
	t2 = imuldiv24(lfoval, depth);
	*rindex = *windex - ndelay - (t2 >> 8);
	if (*rindex < 0) {*rindex += size;}
	t2 = 0xFF - (t2 & 0xFF);
	*hist = t1 + imuldiv8(buf[*rindex] - *hist, t2);
	buf[*windex] = *stream;
	*stream = *hist;
}

/*! modulated allpass filter with allpass interpolation (for Plate Reverberator,...) */
static void free_mod_allpass(mod_allpass *delay)
{
	if(delay->buf != NULL) {
		free(delay->buf);
		delay->buf = NULL;
	}
}

static void set_mod_allpass(mod_allpass *delay, int32 ndelay, int32 depth, double feedback)
{
	int32 size = ndelay + depth + 1;
	free_mod_allpass(delay);
	delay->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(delay->buf == NULL) {return;}
	delay->rindex = 0;
	delay->windex = 0;
	delay->hist = 0;
	delay->ndelay = ndelay;
	delay->depth = depth;
	delay->size = size;
	delay->feedback = feedback;
	delay->feedbacki = TIM_FSCALE(feedback, 24);
	memset(delay->buf, 0, sizeof(int32) * delay->size);
}

static inline void do_mod_allpass(int32 *stream, int32 *buf, int32 size, int32 *rindex, int32 *windex,
								  int32 ndelay, int32 depth, int32 lfoval, int32 *hist, int32 feedback)
{
	int t1, t2, t3;
	if (++*windex == size) {*windex = 0;}
	t3 = *stream + imuldiv24(*hist, feedback);
	t1 = buf[*rindex];
	t2 = imuldiv24(lfoval, depth);
	*rindex = *windex - ndelay - (t2 >> 8);
	if (*rindex < 0) {*rindex += size;}
	t2 = 0xFF - (t2 & 0xFF);
	*hist = t1 + imuldiv8(buf[*rindex] - *hist, t2);
	buf[*windex] = t3;
	*stream = *hist - imuldiv24(t3, feedback);
}

/* allpass filter */
static void free_allpass(allpass *allpass)
{
	if(allpass->buf != NULL) {
		free(allpass->buf);
		allpass->buf = NULL;
	}
}

static void set_allpass(allpass *allpass, int32 size, double feedback)
{
	if(allpass->buf != NULL) {
		free(allpass->buf);
		allpass->buf = NULL;
	}
	allpass->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(allpass->buf == NULL) {return;}
	allpass->index = 0;
	allpass->size = size;
	allpass->feedback = feedback;
	allpass->feedbacki = TIM_FSCALE(feedback, 24);
	memset(allpass->buf, 0, sizeof(int32) * allpass->size);
}

static inline void do_allpass(int32 *stream, int32 *buf, int32 size, int32 *index, int32 feedback)
{
	int32 bufout, output;
	bufout = buf[*index];
	output = *stream - imuldiv24(bufout, feedback);
	buf[*index] = output;
	if (++*index >= size) {*index = 0;}
	*stream = bufout + imuldiv24(output, feedback);
}

static void init_filter_moog(filter_moog *svf)
{
	svf->b0 = svf->b1 = svf->b2 = svf->b3 = svf->b4 = 0;
}

/*! calculate Moog VCF coefficients */
void calc_filter_moog(filter_moog *svf)
{
	double res, fr, p, q, f;

	if (svf->freq > play_mode->rate / 2) {svf->freq = play_mode->rate / 2;}
	else if(svf->freq < 20) {svf->freq = 20;}

	if(svf->freq != svf->last_freq || svf->res_dB != svf->last_res_dB) {
		if(svf->last_freq == 0) {init_filter_moog(svf);}
		svf->last_freq = svf->freq, svf->last_res_dB = svf->res_dB;

		res = pow(10, (svf->res_dB - 96) / 20);
		fr = 2.0 * (double)svf->freq / (double)play_mode->rate;
		q = 1.0 - fr;
		p = fr + 0.8 * fr * q;
		f = p + p - 1.0;
		q = res * (1.0 + 0.5 * q * (1.0 - q + 5.6 * q * q));
		svf->f = TIM_FSCALE(f, 24);
		svf->p = TIM_FSCALE(p, 24);
		svf->q = TIM_FSCALE(q, 24);
	}
}

static inline void do_filter_moog(int32 *stream, int32 *high, int32 f, int32 p, int32 q,
								  int32 *b0, int32 *b1, int32 *b2, int32 *b3, int32 *b4)
{
	int32 t1, t2, t3, tb0 = *b0, tb1 = *b1, tb2 = *b2, tb3 = *b3, tb4 = *b4;
	t3 = *stream - imuldiv24(q, tb4);
	t1 = tb1;	tb1 = imuldiv24(t3 + tb0, p) - imuldiv24(tb1, f);
	t2 = tb2;	tb2 = imuldiv24(tb1 + t1, p) - imuldiv24(tb2, f);
	t1 = tb3;	tb3 = imuldiv24(tb2 + t2, p) - imuldiv24(tb3, f);
	*stream = tb4 = imuldiv24(tb3 + t1, p) - imuldiv24(tb4, f);
	tb0 = t3;
	*stream = tb4;
	*high = t3 - tb4;
	*b0 = tb0, *b1 = tb1, *b2 = tb2, *b3 = tb3, *b4 = tb4;
}

static void init_filter_moog_dist(filter_moog_dist *svf)
{
	svf->b0 = svf->b1 = svf->b2 = svf->b3 = svf->b4 = 0.0f;
}

/*! calculate Moog VCF coefficients */
void calc_filter_moog_dist(filter_moog_dist *svf)
{
	double res, fr, p, q, f;

	if (svf->freq > play_mode->rate / 2) {svf->freq = play_mode->rate / 2;}
	else if (svf->freq < 20) {svf->freq = 20;}

	if (svf->freq != svf->last_freq || svf->res_dB != svf->last_res_dB
		 || svf->dist != svf->last_dist) {
		if (svf->last_freq == 0) {init_filter_moog_dist(svf);}
		svf->last_freq = svf->freq, svf->last_res_dB = svf->res_dB,
			svf->last_dist = svf->dist;

		res = pow(10.0f, (svf->res_dB - 96.0f) / 20.0f);
		fr = 2.0f * (double)svf->freq / (double)play_mode->rate;
		q = 1.0f - fr;
		p = fr + 0.8f * fr * q;
		f = p + p - 1.0f;
		q = res * (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));
		svf->f = f;
		svf->p = p;
		svf->q = q;
		svf->d = 1.0f + svf->dist;
	}
}

static inline void do_filter_moog_dist(double *stream, double *high, double *band, double f, double p, double q, double d,
								   double *b0, double *b1, double *b2, double *b3, double *b4)
{
	double t1, t2, in, tb0 = *b0, tb1 = *b1, tb2 = *b2, tb3 = *b3, tb4 = *b4;
	in = *stream;
	in -= q * tb4;
	t1 = tb1;  tb1 = (in + tb0) * p - tb1 * f;
	t2 = tb2;  tb2 = (tb1 + t1) * p - tb2 * f;
	t1 = tb3;  tb3 = (tb2 + t2) * p - tb3 * f;
	tb4 = (tb3 + t1) * p - tb4 * f;
	tb4 *= d;
	tb4 = tb4 - tb4 * tb4 * tb4 * 0.166667f;
	tb0 = in;
	*stream = tb4;
	*high = in - tb4;
	*band = 3.0f * (tb3 - tb4);
	*b0 = tb0, *b1 = tb1, *b2 = tb2, *b3 = tb3, *b4 = tb4;
}

static inline void do_filter_moog_dist_band(double *stream, double f, double p, double q, double d,
								   double *b0, double *b1, double *b2, double *b3, double *b4)
{
	double t1, t2, in, tb0 = *b0, tb1 = *b1, tb2 = *b2, tb3 = *b3, tb4 = *b4;
	in = *stream;
	in -= q * tb4;
	t1 = tb1;  tb1 = (in + tb0) * p - tb1 * f;
	t2 = tb2;  tb2 = (tb1 + t1) * p - tb2 * f;
	t1 = tb3;  tb3 = (tb2 + t2) * p - tb3 * f;
	tb4 = (tb3 + t1) * p - tb4 * f;
	tb4 *= d;
	tb4 = tb4 - tb4 * tb4 * tb4 * 0.166667f;
	tb0 = in;
	*stream = 3.0f * (tb3 - tb4);
	*b0 = tb0, *b1 = tb1, *b2 = tb2, *b3 = tb3, *b4 = tb4;
}

static void init_filter_lpf18(filter_lpf18 *p)
{
	p->ay1 = p->ay2 = p->aout = p->lastin = 0;
}

/*! calculate LPF18 coefficients */
void calc_filter_lpf18(filter_lpf18 *p)
{
	double kfcn, kp, kp1, kp1h, kres, value;

	if(p->freq != p->last_freq || p->dist != p->last_dist
		|| p->res != p->last_res) {
		if(p->last_freq == 0) {init_filter_lpf18(p);}
		p->last_freq = p->freq, p->last_dist = p->dist, p->last_res = p->res;

		kfcn = 2.0 * (double)p->freq / (double)play_mode->rate;
		kp = ((-2.7528 * kfcn + 3.0429) * kfcn + 1.718) * kfcn - 0.9984;
		kp1 = kp + 1.0;
		kp1h = 0.5 * kp1;
		kres = p->res * (((-2.7079 * kp1 + 10.963) * kp1 - 14.934) * kp1 + 8.4974);
		value = 1.0 + (p->dist * (1.5 + 2.0 * kres * (1.0 - kfcn)));

		p->kp = kp, p->kp1h = kp1h, p->kres = kres, p->value = value;
	}
}

static inline void do_filter_lpf18(double *stream, double *ay1, double *ay2, double *aout, double *lastin,
								   double kres, double value, double kp, double kp1h)
{
	double ax1, ay11, ay31;
	ax1 = *lastin;
	ay11 = *ay1;
	ay31 = *ay2;
	*lastin = *stream - tanh(kres * *aout);
	*ay1 = kp1h * (*lastin + ax1) - kp * *ay1;
	*ay2 = kp1h * (*ay1 + ay11) - kp * *ay2;
	*aout = kp1h * (*ay2 + ay31) - kp * *aout;
	*stream = tanh(*aout * value);
}

#define WS_AMP_MAX	((int32) 0x0fffffff)
#define WS_AMP_MIN	((int32)-0x0fffffff)

static void do_dummy_clipping(int32 *stream, int32 d) {}

static void do_hard_clipping(int32 *stream, int32 d)
{
	int32 x;
	x = imuldiv24(*stream, d);
	x = (x > WS_AMP_MAX) ? WS_AMP_MAX
			: (x < WS_AMP_MIN) ? WS_AMP_MIN : x;
	*stream = x;
}

static void do_soft_clipping1(int32 *stream, int32 d)
{
	int32 x, ai = TIM_FSCALE(1.5f, 24), bi = TIM_FSCALE(0.5f, 24);
	x = imuldiv24(*stream, d);
	x = (x > WS_AMP_MAX) ? WS_AMP_MAX
			: (x < WS_AMP_MIN) ? WS_AMP_MIN : x;
	x = imuldiv24(x, ai) - imuldiv24(imuldiv28(imuldiv28(x, x), x), bi);
	*stream = x;
}

static void do_soft_clipping2(int32 *stream, int32 d)
{
	int32 x;
	x = imuldiv24(*stream, d);
	x = (x > WS_AMP_MAX) ? WS_AMP_MAX
			: (x < WS_AMP_MIN) ? WS_AMP_MIN : x;
	x = signlong(x) * ((abs(x) << 1) - imuldiv28(x, x));
	*stream = x;
}

/*! 1st order lowpass filter */
void init_filter_lowpass1(filter_lowpass1 *p)
{
	if (p->a > 1.0f) {p->a = 1.0f;}
	p->x1l = p->x1r = 0;
	p->ai = TIM_FSCALE(p->a, 24);
	p->iai = TIM_FSCALE(1.0 - p->a, 24);
}

static inline void do_filter_lowpass1(int32 *stream, int32 *x1, int32 a, int32 ia)
{
	*stream = *x1 = imuldiv24(*x1, ia) + imuldiv24(*stream, a);
}

void do_filter_lowpass1_stereo(int32 *buf, int32 count, filter_lowpass1 *p)
{
	int32 i, a = p->ai, ia = p->iai, x1l = p->x1l, x1r = p->x1r;

	for(i = 0; i < count; i++) {
		do_filter_lowpass1(&buf[i], &x1l, a, ia);
		++i;
		do_filter_lowpass1(&buf[i], &x1r, a, ia);
	}
	p->x1l = x1l, p->x1r = x1r;
}

void init_filter_biquad(filter_biquad *p)
{
	p->x1l = 0, p->x2l = 0, p->y1l = 0, p->y2l = 0, p->x1r = 0,
		p->x2r = 0, p->y1r = 0, p->y2r = 0;
}

/*! biquad lowpass filter */
void calc_filter_biquad_low(filter_biquad *p)
{
	double a0, a1, a2, b1, b02, omega, sn, cs, alpha;

	if(p->freq != p->last_freq || p->q != p->last_q) {
		if (p->last_freq == 0) {init_filter_biquad(p);}
		p->last_freq = p->freq, p->last_q = p->q;
		omega = 2.0 * M_PI * (double)p->freq / (double)play_mode->rate;
		sn = sin(omega);
		cs = cos(omega);
		if (p->q == 0 || p->freq < 0 || p->freq > play_mode->rate / 2) {
			p->b02 = TIM_FSCALE(1.0, 24);
			p->a1 = p->a2 = p->b1 = 0;
			return;
		} else {alpha = sn / (2.0 * p->q);}

		a0 = 1.0f / (1.0f + alpha);
		b02 = ((1.0f - cs) / 2.0f) * a0;
		b1 = (1.0f - cs) * a0;
		a1 = (-2.0f * cs) * a0;
		a2 = (1.0f - alpha) * a0;

		p->b1 = TIM_FSCALE(b1, 24);
		p->a2 = TIM_FSCALE(a2, 24);
		p->a1 = TIM_FSCALE(a1, 24);
		p->b02 = TIM_FSCALE(b02, 24);
	}
}

/*! biquad highpass filter */
void calc_filter_biquad_high(filter_biquad *p)
{
	double a0, a1, a2, b1, b02, omega, sn, cs, alpha;

	if(p->freq != p->last_freq || p->q != p->last_q) {
		if (p->last_freq == 0) {init_filter_biquad(p);}
		p->last_freq = p->freq, p->last_q = p->q;
		omega = 2.0 * M_PI * (double)p->freq / (double)play_mode->rate;
		sn = sin(omega);
		cs = cos(omega);
		if (p->q == 0 || p->freq < 0 || p->freq > play_mode->rate / 2) {
			p->b02 = TIM_FSCALE(1.0, 24);
			p->a1 = p->a2 = p->b1 = 0;
			return;
		} else {alpha = sn / (2.0 * p->q);}

		a0 = 1.0f / (1.0f + alpha);
		b02 = ((1.0f + cs) / 2.0f) * a0;
		b1 = (-(1.0f + cs)) * a0;
		a1 = (-2.0f * cs) * a0;
		a2 = (1.0f - alpha) * a0;

		p->b1 = TIM_FSCALE(b1, 24);
		p->a2 = TIM_FSCALE(a2, 24);
		p->a1 = TIM_FSCALE(a1, 24);
		p->b02 = TIM_FSCALE(b02, 24);
	}
}

static inline void do_filter_biquad(int32 *stream, int32 a1, int32 a2, int32 b1,
									int32 b02, int32 *x1, int32 *x2, int32 *y1, int32 *y2)
{
	int32 t1;
	t1 = imuldiv24(*stream + *x2, b02) + imuldiv24(*x1, b1) - imuldiv24(*y1, a1) - imuldiv24(*y2, a2);
	*x2 = *x1;
	*x1 = *stream;
	*y2 = *y1;
	*y1 = t1;
	*stream = t1;
}

static void do_biquad_filter_stereo(int32* buf, int32 count, filter_biquad *p)
{
	int32 i;
	int32 x1l = p->x1l, x2l = p->x2l, y1l = p->y1l, y2l = p->y2l,
		x1r = p->x1r, x2r = p->x2r, y1r = p->y1r, y2r = p->y2r;
	int32 a1 = p->a1, a2 = p->a2, b02 = p->b02, b1 = p->b1;

	for(i = 0; i < count; i++) {
		do_filter_biquad(&(buf[i]), a1, a2, b1, b02, &x1l, &x2l, &y1l, &y2l);
		++i;
		do_filter_biquad(&(buf[i]), a1, a2, b1, b02, &x1r, &x2r, &y1r, &y2r);
	}
	p->x1l = x1l, p->x2l = x2l, p->y1l = y1l, p->y2l = y2l,
		p->x1r = x1r, p->x2r = x2r, p->y1r = y1r, p->y2r = y2r;
}

void init_filter_shelving(filter_shelving *p)
{
	p->x1l = 0, p->x2l = 0, p->y1l = 0, p->y2l = 0, p->x1r = 0,
		p->x2r = 0, p->y1r = 0, p->y2r = 0;
}

/*! shelving filter */
void calc_filter_shelving_low(filter_shelving *p)
{
	double a0, a1, a2, b0, b1, b2, omega, sn, cs, A, beta;

	init_filter_shelving(p);

	A = pow(10, p->gain / 40);
	omega = 2.0 * M_PI * (double)p->freq / (double)play_mode->rate;
	sn = sin(omega);
	cs = cos(omega);
	if (p->freq < 0 || p->freq > play_mode->rate / 2) {
		p->b0 = TIM_FSCALE(1.0, 24);
		p->a1 = p->b1 = p->a2 = p->b2 = 0;
		return;
	}
	if (p->q == 0) {beta = sqrt(A + A);}
	else {beta = sqrt(A) / p->q;}

	a0 = 1.0 / ((A + 1) + (A - 1) * cs + beta * sn);
	a1 = 2.0 * ((A - 1) + (A + 1) * cs);
	a2 = -((A + 1) + (A - 1) * cs - beta * sn);
	b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
	b1 = 2.0 * A * ((A - 1) - (A + 1) * cs);
	b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);

	a1 *= a0;
	a2 *= a0;
	b1 *= a0;
	b2 *= a0;
	b0 *= a0;

	p->a1 = TIM_FSCALE(a1, 24);
	p->a2 = TIM_FSCALE(a2, 24);
	p->b0 = TIM_FSCALE(b0, 24);
	p->b1 = TIM_FSCALE(b1, 24);
	p->b2 = TIM_FSCALE(b2, 24);
}

void calc_filter_shelving_high(filter_shelving *p)
{
	double a0, a1, a2, b0, b1, b2, omega, sn, cs, A, beta;

	init_filter_shelving(p);

	A = pow(10, p->gain / 40);
	omega = 2.0 * M_PI * (double)p->freq / (double)play_mode->rate;
	sn = sin(omega);
	cs = cos(omega);
	if (p->freq < 0 || p->freq > play_mode->rate / 2) {
		p->b0 = TIM_FSCALE(1.0, 24);
		p->a1 = p->b1 = p->a2 = p->b2 = 0;
		return;
	}
	if (p->q == 0) {beta = sqrt(A + A);}
	else {beta = sqrt(A) / p->q;}

	a0 = 1.0 / ((A + 1) - (A - 1) * cs + beta * sn);
	a1 = (-2 * ((A - 1) - (A + 1) * cs));
	a2 = -((A + 1) - (A - 1) * cs - beta * sn);
	b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
	b1 = -2 * A * ((A - 1) + (A + 1) * cs);
	b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);

	a1 *= a0;
	a2 *= a0;
	b0 *= a0;
	b1 *= a0;
	b2 *= a0;

	p->a1 = TIM_FSCALE(a1, 24);
	p->a2 = TIM_FSCALE(a2, 24);
	p->b0 = TIM_FSCALE(b0, 24);
	p->b1 = TIM_FSCALE(b1, 24);
	p->b2 = TIM_FSCALE(b2, 24);
}

static void do_shelving_filter_stereo(int32* buf, int32 count, filter_shelving *p)
{
	int32 i;
	int32 x1l = p->x1l, x2l = p->x2l, y1l = p->y1l, y2l = p->y2l,
		x1r = p->x1r, x2r = p->x2r, y1r = p->y1r, y2r = p->y2r, yout;
	int32 a1 = p->a1, a2 = p->a2, b0 = p->b0, b1 = p->b1, b2 = p->b2;

	for(i = 0; i < count; i++) {
		yout = imuldiv24(buf[i], b0) + imuldiv24(x1l, b1) + imuldiv24(x2l, b2) + imuldiv24(y1l, a1) + imuldiv24(y2l, a2);
		x2l = x1l;
		x1l = buf[i];
		y2l = y1l;
		y1l = yout;
		buf[i] = yout;

		yout = imuldiv24(buf[++i], b0) + imuldiv24(x1r, b1) + imuldiv24(x2r, b2) + imuldiv24(y1r, a1) + imuldiv24(y2r, a2);
		x2r = x1r;
		x1r = buf[i];
		y2r = y1r;
		y1r = yout;
		buf[i] = yout;
	}
	p->x1l = x1l, p->x2l = x2l, p->y1l = y1l, p->y2l = y2l,
		p->x1r = x1r, p->x2r = x2r, p->y1r = y1r, p->y2r = y2r;
}

void init_filter_peaking(filter_peaking *p)
{
	p->x1l = 0, p->x2l = 0, p->y1l = 0, p->y2l = 0, p->x1r = 0,
		p->x2r = 0, p->y1r = 0, p->y2r = 0;
}

/*! peaking filter */
void calc_filter_peaking(filter_peaking *p)
{
	double a0, ba1, a2, b0, b2, omega, sn, cs, A, alpha;

	init_filter_peaking(p);

	A = pow(10, p->gain / 40);
	omega = 2.0 * M_PI * (double)p->freq / (double)play_mode->rate;
	sn = sin(omega);
	cs = cos(omega);
	if (p->q == 0 || p->freq < 0 || p->freq > play_mode->rate / 2) {
		p->b0 = TIM_FSCALE(1.0, 24);
		p->ba1 = p->a2 = p->b2 = 0;
		return;
	} else {alpha = sn / (2.0 * p->q);}

	a0 = 1.0 / (1.0 + alpha / A);
	ba1 = -2.0 * cs;
	a2 = 1.0 - alpha / A;
	b0 = 1.0 + alpha * A;
	b2 = 1.0 - alpha * A;

	ba1 *= a0;
	a2 *= a0;
	b0 *= a0;
	b2 *= a0;

	p->ba1 = TIM_FSCALE(ba1, 24);
	p->a2 = TIM_FSCALE(a2, 24);
	p->b0 = TIM_FSCALE(b0, 24);
	p->b2 = TIM_FSCALE(b2, 24);
}

static void do_peaking_filter_stereo(int32* buf, int32 count, filter_peaking *p)
{
	int32 i;
	int32 x1l = p->x1l, x2l = p->x2l, y1l = p->y1l, y2l = p->y2l,
		x1r = p->x1r, x2r = p->x2r, y1r = p->y1r, y2r = p->y2r, yout;
	int32 ba1 = p->ba1, a2 = p->a2, b0 = p->b0, b2 = p->b2;

	for(i = 0; i < count; i++) {
		yout = imuldiv24(buf[i], b0) + imuldiv24(x1l - y1l, ba1) + imuldiv24(x2l, b2) - imuldiv24(y2l, a2);
		x2l = x1l;
		x1l = buf[i];
		y2l = y1l;
		y1l = yout;
		buf[i] = yout;

		yout = imuldiv24(buf[++i], b0) + imuldiv24(x1r - y1r, ba1) + imuldiv24(x2r, b2) - imuldiv24(y2r, a2);
		x2r = x1r;
		x1r = buf[i];
		y2r = y1r;
		y1r = yout;
		buf[i] = yout;
	}
	p->x1l = x1l, p->x2l = x2l, p->y1l = y1l, p->y2l = y2l,
		p->x1r = x1r, p->x2r = x2r, p->y1r = y1r, p->y2r = y2r;
}

void init_pink_noise(pink_noise *p)
{
	p->b0 = p->b1 = p->b2 = p->b3 = p->b4 = p->b5 = p->b6 = 0;
}

float get_pink_noise(pink_noise *p)
{
	float b0 = p->b0, b1 = p->b1, b2 = p->b2, b3 = p->b3,
	   b4 = p->b4, b5 = p->b5, b6 = p->b6, pink, white;

	white = genrand_real1() * 2.0 - 1.0;
	b0 = 0.99886 * b0 + white * 0.0555179;
	b1 = 0.99332 * b1 + white * 0.0750759;
	b2 = 0.96900 * b2 + white * 0.1538520;
	b3 = 0.86650 * b3 + white * 0.3104856;
	b4 = 0.55000 * b4 + white * 0.5329522;
	b5 = -0.7616 * b5 - white * 0.0168980;
	pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
	b6 = white * 0.115926;
	pink *= 0.22;
	pink = (pink > 1.0) ? 1.0 : (pink < -1.0) ? -1.0 : pink;

	p->b0 = b0, p->b1 = b1, p->b2 = b2, p->b3 = b3,
		p->b4 = b4, p->b5 = b5, p->b6 = b6;

	return pink;
}

float get_pink_noise_light(pink_noise *p)
{
	float b0 = p->b0, b1 = p->b1, b2 = p->b2, pink, white;

	white = genrand_real1() * 2.0 - 1.0;
	b0 = 0.99765 * b0 + white * 0.0990460;
	b1 = 0.96300 * b1 + white * 0.2965164;
	b2 = 0.57000 * b2 + white * 1.0526913;
	pink = b0 + b1 + b2 + white * 0.1848;
	pink *= 0.22;
	pink = (pink > 1.0) ? 1.0 : (pink < -1.0) ? -1.0 : pink;

	p->b0 = b0, p->b1 = b1, p->b2 = b2;

	return pink;
}

/*                          */
/*  Standard Reverb Effect  */
/*                          */
#define REV_VAL0         5.3
#define REV_VAL1        10.5
#define REV_VAL2        44.12
#define REV_VAL3        21.0

static int32  reverb_effect_buffer[AUDIO_BUFFER_SIZE * 2];
static int32  reverb_effect_bufsize = sizeof(reverb_effect_buffer);

#if OPT_MODE != 0
#if defined(_MSC_VER) || defined(__WATCOMC__) || ( defined(__BORLANDC__) &&(__BORLANDC__ >= 1380) )
void set_ch_reverb(int32 *buf, int32 count, int32 level)
{
	int32 *dbuf = reverb_effect_buffer;
	if(!level) {return;}
	level = TIM_FSCALE(level / 127.0 * REV_INP_LEV, 24);

	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		mov		ebx, [level]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		imul	ebx
		shr		eax, 24
		shl		edx, 8
		or		eax, edx	/* u */
		mov		edx, [edi]	/* v */
		add		esi, 4		/* u */	
		add		edx, eax	/* v */
		mov		[edi], edx	/* u */
		add		edi, 4		/* v */
		dec		ecx			/* u */
		jnz		L1			/* v */
L2:
	}
}
#else
void set_ch_reverb(int32 *buf, int32 count, int32 level)
{
    int32 i, *dbuf = reverb_effect_buffer;
	if(!level) {return;}
    level = TIM_FSCALE(level / 127.0 * REV_INP_LEV, 24);

	for(i = count - 1; i >= 0; i--) {dbuf[i] += imuldiv24(buf[i], level);}
}
#endif	/* _MSC_VER */
#else
void set_ch_reverb(register int32 *sbuffer, int32 n, int32 level)
{
    register int32  i;
	if(!level) {return;}
    FLOAT_T send_level = (FLOAT_T)level / 127.0 * REV_INP_LEV;
	
	for(i = 0; i < n; i++)
    {
        reverb_effect_buffer[i] += sbuffer[i] * send_level;
    }
}
#endif /* OPT_MODE != 0 */

static double gs_revchar_to_roomsize(int character)
{
	double rs;
	switch(character) {
	case 0: rs = 1.0;	break;	/* Room 1 */
	case 1: rs = 0.94;	break;	/* Room 2 */
	case 2: rs = 0.97;	break;	/* Room 3 */
	case 3: rs = 0.90;	break;	/* Hall 1 */
	case 4: rs = 0.85;	break;	/* Hall 2 */
	default: rs = 1.0;	break;	/* Plate, Delay, Panning Delay */
	}
	return rs;
}

static double gs_revchar_to_level(int character)
{
	double level;
	switch(character) {
	case 0: level = 0.744025605;	break;	/* Room 1 */
	case 1: level = 1.224309745;	break;	/* Room 2 */
	case 2: level = 0.858592403;	break;	/* Room 3 */
	case 3: level = 1.0471802;	break;	/* Hall 1 */
	case 4: level = 1.0;	break;	/* Hall 2 */
	case 5: level = 0.865335496;	break;	/* Plate */
	default: level = 1.0;	break;	/* Delay, Panning Delay */
	}
	return level;
}

static double gs_revchar_to_rt(int character)
{
	double rt;
	switch(character) {
	case 0: rt = 0.516850262;	break;	/* Room 1 */
	case 1: rt = 1.004226004;	break;	/* Room 2 */
	case 2: rt = 0.691046825;	break;	/* Room 3 */
	case 3: rt = 0.893006004;	break;	/* Hall 1 */
	case 4: rt = 1.0;	break;	/* Hall 2 */
	case 5: rt = 0.538476488;	break;	/* Plate */
	default: rt = 1.0;	break;	/* Delay, Panning Delay */
	}
	return rt;
}

static void init_standard_reverb(InfoStandardReverb *info)
{
	double time;
	info->ta = info->tb = 0;
	info->HPFL = info->HPFR = info->LPFL = info->LPFR = info->EPFL = info->EPFR = 0;
	info->spt0 = info->spt1 = info->spt2 = info->spt3 = 0;
	time = reverb_time_table[reverb_status_gs.time] * gs_revchar_to_rt(reverb_status_gs.character) 
		/ reverb_time_table[64] * 0.8;
	info->rpt0 = REV_VAL0 * play_mode->rate / 1000.0f * time;
	info->rpt1 = REV_VAL1 * play_mode->rate / 1000.0f * time;
	info->rpt2 = REV_VAL2 * play_mode->rate / 1000.0f * time;
	info->rpt3 = REV_VAL3 * play_mode->rate / 1000.0f * time;
	while (!isprime(info->rpt0)) {info->rpt0++;}
	while (!isprime(info->rpt1)) {info->rpt1++;}
	while (!isprime(info->rpt2)) {info->rpt2++;}
	while (!isprime(info->rpt3)) {info->rpt3++;}
	set_delay(&(info->buf0_L), info->rpt0 + 1);
	set_delay(&(info->buf0_R), info->rpt0 + 1);
	set_delay(&(info->buf1_L), info->rpt1 + 1);
	set_delay(&(info->buf1_R), info->rpt1 + 1);
	set_delay(&(info->buf2_L), info->rpt2 + 1);
	set_delay(&(info->buf2_R), info->rpt2 + 1);
	set_delay(&(info->buf3_L), info->rpt3 + 1);
	set_delay(&(info->buf3_R), info->rpt3 + 1);
	info->fbklev = 0.12f;
	info->nmixlev = 0.7f;
	info->cmixlev = 0.9f;
	info->monolev = 0.7f;
	info->hpflev = 0.5f;
	info->lpflev = 0.45f;
	info->lpfinp = 0.55f;
	info->epflev = 0.4f;
	info->epfinp = 0.48f;
	info->width = 0.125f;
	info->wet = 2.0f * (double)reverb_status_gs.level / 127.0f * gs_revchar_to_level(reverb_status_gs.character);
	info->fbklevi = TIM_FSCALE(info->fbklev, 24);
	info->nmixlevi = TIM_FSCALE(info->nmixlev, 24);
	info->cmixlevi = TIM_FSCALE(info->cmixlev, 24);
	info->monolevi = TIM_FSCALE(info->monolev, 24);
	info->hpflevi = TIM_FSCALE(info->hpflev, 24);
	info->lpflevi = TIM_FSCALE(info->lpflev, 24);
	info->lpfinpi = TIM_FSCALE(info->lpfinp, 24);
	info->epflevi = TIM_FSCALE(info->epflev, 24);
	info->epfinpi = TIM_FSCALE(info->epfinp, 24);
	info->widthi = TIM_FSCALE(info->width, 24);
	info->weti = TIM_FSCALE(info->wet, 24);
}

static void free_standard_reverb(InfoStandardReverb *info)
{
	free_delay(&(info->buf0_L));
	free_delay(&(info->buf0_R));
	free_delay(&(info->buf1_L));
	free_delay(&(info->buf1_R));
	free_delay(&(info->buf2_L));
	free_delay(&(info->buf2_R));
	free_delay(&(info->buf3_L));
	free_delay(&(info->buf3_R));
}

/*! Standard Reverberator; this implementation is specialized for system effect. */
#if OPT_MODE != 0 /* fixed-point implementation */
static void do_ch_standard_reverb(int32 *buf, int32 count, InfoStandardReverb *info)
{
	int32 i, fixp, s, t;
	int32 spt0 = info->spt0, spt1 = info->spt1, spt2 = info->spt2, spt3 = info->spt3,
		ta = info->ta, tb = info->tb, HPFL = info->HPFL, HPFR = info->HPFR,
		LPFL = info->LPFL, LPFR = info->LPFR, EPFL = info->EPFL, EPFR = info->EPFR;
	int32 *buf0_L = info->buf0_L.buf, *buf0_R = info->buf0_R.buf,
		*buf1_L = info->buf1_L.buf, *buf1_R = info->buf1_R.buf,
		*buf2_L = info->buf2_L.buf, *buf2_R = info->buf2_R.buf,
		*buf3_L = info->buf3_L.buf, *buf3_R = info->buf3_R.buf;
	int32 fbklevi = info->fbklevi, cmixlevi = info->cmixlevi,
		hpflevi = info->hpflevi, lpflevi = info->lpflevi, lpfinpi = info->lpfinpi,
		epflevi = info->epflevi, epfinpi = info->epfinpi, widthi = info->widthi,
		rpt0 = info->rpt0, rpt1 = info->rpt1, rpt2 = info->rpt2, rpt3 = info->rpt3, weti = info->weti;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_standard_reverb(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_standard_reverb(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
        /* L */
        fixp = reverb_effect_buffer[i];

        LPFL = imuldiv24(LPFL, lpflevi) + imuldiv24(buf2_L[spt2] + tb, lpfinpi) + imuldiv24(ta, widthi);
        ta = buf3_L[spt3];
        s  = buf3_L[spt3] = buf0_L[spt0];
        buf0_L[spt0] = -LPFL;

        t = imuldiv24(HPFL + fixp, hpflevi);
        HPFL = t - fixp;

        buf2_L[spt2] = imuldiv24(s - imuldiv24(fixp, fbklevi), cmixlevi);
        tb = buf1_L[spt1];
        buf1_L[spt1] = t;

        EPFL = imuldiv24(EPFL, epflevi) + imuldiv24(ta, epfinpi);
        buf[i] += imuldiv24(ta + EPFL, weti);

        /* R */
        fixp = reverb_effect_buffer[++i];

        LPFR = imuldiv24(LPFR, lpflevi) + imuldiv24(buf2_R[spt2] + tb, lpfinpi) + imuldiv24(ta, widthi);
        ta = buf3_R[spt3];
        s  = buf3_R[spt3] = buf0_R[spt0];
        buf0_R[spt0] = LPFR;

        t = imuldiv24(HPFR + fixp, hpflevi);
        HPFR = t - fixp;

        buf2_R[spt2] = imuldiv24(s - imuldiv24(fixp, fbklevi), cmixlevi);
        tb = buf1_R[spt1];
        buf1_R[spt1] = t;

        EPFR = imuldiv24(EPFR, epflevi) + imuldiv24(ta, epfinpi);
        buf[i] += imuldiv24(ta + EPFR, weti);

		if (++spt0 == rpt0) {spt0 = 0;}
		if (++spt1 == rpt1) {spt1 = 0;}
		if (++spt2 == rpt2) {spt2 = 0;}
		if (++spt3 == rpt3) {spt3 = 0;}
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
	info->spt0 = spt0, info->spt1 = spt1, info->spt2 = spt2, info->spt3 = spt3,
	info->ta = ta, info->tb = tb, info->HPFL = HPFL, info->HPFR = HPFR,
	info->LPFL = LPFL, info->LPFR = LPFR, info->EPFL = EPFL, info->EPFR = EPFR;
}
#else /* floating-point implementation */
static void do_ch_standard_reverb(int32 *buf, int32 count, InfoStandardReverb *info)
{
	int32 i, fixp, s, t;
	int32 spt0 = info->spt0, spt1 = info->spt1, spt2 = info->spt2, spt3 = info->spt3,
		ta = info->ta, tb = info->tb, HPFL = info->HPFL, HPFR = info->HPFR,
		LPFL = info->LPFL, LPFR = info->LPFR, EPFL = info->EPFL, EPFR = info->EPFR;
	int32 *buf0_L = info->buf0_L.buf, *buf0_R = info->buf0_R.buf,
		*buf1_L = info->buf1_L.buf, *buf1_R = info->buf1_R.buf,
		*buf2_L = info->buf2_L.buf, *buf2_R = info->buf2_R.buf,
		*buf3_L = info->buf3_L.buf, *buf3_R = info->buf3_R.buf;
	FLOAT_T fbklev = info->fbklev, cmixlev = info->cmixlev,
		hpflev = info->hpflev, lpflev = info->lpflev, lpfinp = info->lpfinp,
		epflev = info->epflev, epfinp = info->epfinp, width = info->width,
		rpt0 = info->rpt0, rpt1 = info->rpt1, rpt2 = info->rpt2, rpt3 = info->rpt3, wet = info->wet;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_standard_reverb(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_standard_reverb(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
        /* L */
        fixp = reverb_effect_buffer[i];

        LPFL = LPFL * lpflev + (buf2_L[spt2] + tb) * lpfinp + ta * width;
        ta = buf3_L[spt3];
        s  = buf3_L[spt3] = buf0_L[spt0];
        buf0_L[spt0] = -LPFL;

        t = (HPFL + fixp) * hpflev;
        HPFL = t - fixp;

        buf2_L[spt2] = (s - fixp * fbklev) * cmixlev;
        tb = buf1_L[spt1];
        buf1_L[spt1] = t;

        EPFL = EPFL * epflev + ta * epfinp;
        buf[i] += (ta + EPFL) * wet;

        /* R */
        fixp = reverb_effect_buffer[++i];

        LPFR = LPFR * lpflev + (buf2_R[spt2] + tb) * lpfinp + ta * width;
        ta = buf3_R[spt3];
        s  = buf3_R[spt3] = buf0_R[spt0];
        buf0_R[spt0] = LPFR;

        t = (HPFR + fixp) * hpflev;
        HPFR = t - fixp;

        buf2_R[spt2] = (s - fixp * fbklev) * cmixlev;
        tb = buf1_R[spt1];
        buf1_R[spt1] = t;

        EPFR = EPFR * epflev + ta * epfinp;
        buf[i] += (ta + EPFR) * wet;

		if (++spt0 == rpt0) {spt0 = 0;}
		if (++spt1 == rpt1) {spt1 = 0;}
		if (++spt2 == rpt2) {spt2 = 0;}
		if (++spt3 == rpt3) {spt3 = 0;}
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
	info->spt0 = spt0, info->spt1 = spt1, info->spt2 = spt2, info->spt3 = spt3,
	info->ta = ta, info->tb = tb, info->HPFL = HPFL, info->HPFR = HPFR,
	info->LPFL = LPFL, info->LPFR = LPFR, info->EPFL = EPFL, info->EPFR = EPFR;
}
#endif /* OPT_MODE != 0 */

/*! Standard Monoral Reverberator; this implementation is specialized for system effect. */
static void do_ch_standard_reverb_mono(int32 *buf, int32 count, InfoStandardReverb *info)
{
	int32 i, fixp, s, t;
	int32 spt0 = info->spt0, spt1 = info->spt1, spt2 = info->spt2, spt3 = info->spt3,
		ta = info->ta, tb = info->tb, HPFL = info->HPFL, HPFR = info->HPFR,
		LPFL = info->LPFL, LPFR = info->LPFR, EPFL = info->EPFL, EPFR = info->EPFR;
	int32 *buf0_L = info->buf0_L.buf, *buf0_R = info->buf0_R.buf,
		*buf1_L = info->buf1_L.buf, *buf1_R = info->buf1_R.buf,
		*buf2_L = info->buf2_L.buf, *buf2_R = info->buf2_R.buf,
		*buf3_L = info->buf3_L.buf, *buf3_R = info->buf3_R.buf;
	FLOAT_T fbklev = info->fbklev, nmixlev = info->nmixlev, monolev = info->monolev,
		hpflev = info->hpflev, lpflev = info->lpflev, lpfinp = info->lpfinp,
		epflev = info->epflev, epfinp = info->epfinp, width = info->width,
		rpt0 = info->rpt0, rpt1 = info->rpt1, rpt2 = info->rpt2, rpt3 = info->rpt3, wet = info->wet;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_standard_reverb(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_standard_reverb(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
        /* L */
        fixp = buf[i] * monolev;

        LPFL = LPFL * lpflev + (buf2_L[spt2] + tb) * lpfinp + ta * width;
        ta = buf3_L[spt3];
        s  = buf3_L[spt3] = buf0_L[spt0];
        buf0_L[spt0] = -LPFL;

        t = (HPFL + fixp) * hpflev;
        HPFL = t - fixp;

        buf2_L[spt2] = (s - fixp * fbklev) * nmixlev;
        tb = buf1_L[spt1];
        buf1_L[spt1] = t;

        /* R */
        LPFR = LPFR * lpflev + (buf2_R[spt2] + tb) * lpfinp + ta * width;
        ta = buf3_R[spt3];
        s  = buf3_R[spt3] = buf0_R[spt0];
        buf0_R[spt0] = LPFR;

        t = (HPFR + fixp) * hpflev;
        HPFR = t - fixp;

        buf2_R[spt2] = (s - fixp * fbklev) * nmixlev;
        tb = buf1_R[spt1];
        buf1_R[spt1] = t;

        EPFR = EPFR * epflev + ta * epfinp;
        buf[i] = (ta + EPFR) * wet + fixp;

		if (++spt0 == rpt0) {spt0 = 0;}
		if (++spt1 == rpt1) {spt1 = 0;}
		if (++spt2 == rpt2) {spt2 = 0;}
		if (++spt3 == rpt3) {spt3 = 0;}
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
	info->spt0 = spt0, info->spt1 = spt1, info->spt2 = spt2, info->spt3 = spt3,
	info->ta = ta, info->tb = tb, info->HPFL = HPFL, info->HPFR = HPFR,
	info->LPFL = LPFL, info->LPFR = LPFR, info->EPFL = EPFL, info->EPFR = EPFR;
}

/* dummy */
void reverb_rc_event(int rc, int32 val)
{
    switch(rc)
    {
      case RC_CHANGE_REV_EFFB:
        break;
      case RC_CHANGE_REV_TIME:
        break;
    }
}

/*             */
/*  Freeverb   */
/*             */
static void set_freeverb_allpass(allpass *allpass, int32 size)
{
	if(allpass->buf != NULL) {
		free(allpass->buf);
		allpass->buf = NULL;
	}
	allpass->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(allpass->buf == NULL) {return;}
	allpass->index = 0;
	allpass->size = size;
}

static void init_freeverb_allpass(allpass *allpass)
{
	memset(allpass->buf, 0, sizeof(int32) * allpass->size);
}

static void set_freeverb_comb(comb *comb, int32 size)
{
	if(comb->buf != NULL) {
		free(comb->buf);
		comb->buf = NULL;
	}
	comb->buf = (int32 *)safe_malloc(sizeof(int32) * size);
	if(comb->buf == NULL) {return;}
	comb->index = 0;
	comb->size = size;
	comb->filterstore = 0;
}

static void init_freeverb_comb(comb *comb)
{
	memset(comb->buf, 0, sizeof(int32) * comb->size);
}

#define scalewet 0.06f
#define scaledamp 0.4f
#define scaleroom 0.28f
#define offsetroom 0.7f
#define initialroom 0.5f
#define initialdamp 0.5f
#define initialwet 1 / scalewet
#define initialdry 0
#define initialwidth 0.5f
#define initialallpassfbk 0.65f
#define stereospread 23
static int combtunings[numcombs] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
static int allpasstunings[numallpasses] = {225, 341, 441, 556};
#define fixedgain 0.025f
#define combfbk 3.0f

static void realloc_freeverb_buf(InfoFreeverb *rev)
{
	int i;
	int32 tmpL, tmpR;
	double time, samplerate = play_mode->rate;

	time = reverb_time_table[reverb_status_gs.time] * gs_revchar_to_rt(reverb_status_gs.character) * combfbk
		/ (60 * combtunings[numcombs - 1] / (-20 * log10(rev->roomsize1) * 44100.0));

	for(i = 0; i < numcombs; i++)
	{
		tmpL = combtunings[i] * samplerate * time / 44100.0;
		tmpR = (combtunings[i] + stereospread) * samplerate * time / 44100.0;
		if(tmpL < 10) tmpL = 10;
		if(tmpR < 10) tmpR = 10;
		while(!isprime(tmpL)) tmpL++;
		while(!isprime(tmpR)) tmpR++;
		rev->combL[i].size = tmpL;
		rev->combR[i].size = tmpR;
		set_freeverb_comb(&rev->combL[i], rev->combL[i].size);
		set_freeverb_comb(&rev->combR[i], rev->combR[i].size);
	}

	for(i = 0; i < numallpasses; i++)
	{
		tmpL = allpasstunings[i] * samplerate * time / 44100.0;
		tmpR = (allpasstunings[i] + stereospread) * samplerate * time / 44100.0;
		if(tmpL < 10) tmpL = 10;
		if(tmpR < 10) tmpR = 10;
		while(!isprime(tmpL)) tmpL++;
		while(!isprime(tmpR)) tmpR++;
		rev->allpassL[i].size = tmpL;
		rev->allpassR[i].size = tmpR;
		set_freeverb_allpass(&rev->allpassL[i], rev->allpassL[i].size);
		set_freeverb_allpass(&rev->allpassR[i], rev->allpassR[i].size);
	}
}

static void update_freeverb(InfoFreeverb *rev)
{
	int i;
	double allpassfbk = 0.55, rtbase, rt;

	rev->wet = (double)reverb_status_gs.level / 127.0f * gs_revchar_to_level(reverb_status_gs.character) * fixedgain;
	rev->roomsize = gs_revchar_to_roomsize(reverb_status_gs.character) * scaleroom + offsetroom;
	rev->width = 0.5f;

	rev->wet1 = rev->width / 2.0f + 0.5f;
	rev->wet2 = (1.0f - rev->width) / 2.0f;
	rev->roomsize1 = rev->roomsize;
	rev->damp1 = rev->damp;

	realloc_freeverb_buf(rev);

	rtbase = 1.0 / (44100.0 * reverb_time_table[reverb_status_gs.time] * gs_revchar_to_rt(reverb_status_gs.character));

	for(i = 0; i < numcombs; i++)
	{
		rt = pow(10.0f, -combfbk * (double)combtunings[i] * rtbase);
		rev->combL[i].feedback = rt;
		rev->combR[i].feedback = rt;
		rev->combL[i].damp1 = rev->damp1;
		rev->combR[i].damp1 = rev->damp1;
		rev->combL[i].damp2 = 1 - rev->damp1;
		rev->combR[i].damp2 = 1 - rev->damp1;
		rev->combL[i].damp1i = TIM_FSCALE(rev->combL[i].damp1, 24);
		rev->combR[i].damp1i = TIM_FSCALE(rev->combR[i].damp1, 24);
		rev->combL[i].damp2i = TIM_FSCALE(rev->combL[i].damp2, 24);
		rev->combR[i].damp2i = TIM_FSCALE(rev->combR[i].damp2, 24);
		rev->combL[i].feedbacki = TIM_FSCALE(rev->combL[i].feedback, 24);
		rev->combR[i].feedbacki = TIM_FSCALE(rev->combR[i].feedback, 24);
	}

	for(i = 0; i < numallpasses; i++)
	{
		rev->allpassL[i].feedback = allpassfbk;
		rev->allpassR[i].feedback = allpassfbk;
		rev->allpassL[i].feedbacki = TIM_FSCALE(rev->allpassL[i].feedback, 24);
		rev->allpassR[i].feedbacki = TIM_FSCALE(rev->allpassR[i].feedback, 24);
	}

	rev->wet1i = TIM_FSCALE(rev->wet1, 24);
	rev->wet2i = TIM_FSCALE(rev->wet2, 24);

	set_delay(&(rev->pdelay), (int32)((double)reverb_status_gs.pre_delay_time * play_mode->rate / 1000.0f));
}

static void init_freeverb(InfoFreeverb *rev)
{
	int i;
	for(i = 0; i < numcombs; i++) {
		init_freeverb_comb(&rev->combL[i]);
		init_freeverb_comb(&rev->combR[i]);
	}
	for(i = 0; i < numallpasses; i++) {
		init_freeverb_allpass(&rev->allpassL[i]);
		init_freeverb_allpass(&rev->allpassR[i]);
	}
}

static void alloc_freeverb_buf(InfoFreeverb *rev)
{
	int i;
	if(rev->alloc_flag) {return;}
	for (i = 0; i < numcombs; i++) {
		set_freeverb_comb(&rev->combL[i], combtunings[i]);
		set_freeverb_comb(&rev->combR[i], combtunings[i] + stereospread);
	}
	for (i = 0; i < numallpasses; i++) {
		set_freeverb_allpass(&rev->allpassL[i], allpasstunings[i]);
		set_freeverb_allpass(&rev->allpassR[i], allpasstunings[i] + stereospread);
		rev->allpassL[i].feedback = initialallpassfbk;
		rev->allpassR[i].feedback = initialallpassfbk;
	}

	rev->wet = initialwet * scalewet;
	rev->damp = initialdamp * scaledamp;
	rev->width = initialwidth;
	rev->roomsize = initialroom * scaleroom + offsetroom;

	rev->alloc_flag = 1;
}

static void free_freeverb_buf(InfoFreeverb *rev)
{
	int i;
	
	for(i = 0; i < numcombs; i++)
	{
		if(rev->combL[i].buf != NULL) {
			free(rev->combL[i].buf);
			rev->combL[i].buf = NULL;
		}
		if(rev->combR[i].buf != NULL) {
			free(rev->combR[i].buf);
			rev->combR[i].buf = NULL;
		}
	}
	for(i = 0; i < numallpasses; i++)
	{
		if(rev->allpassL[i].buf != NULL) {
			free(rev->allpassL[i].buf);
			rev->allpassL[i].buf = NULL;
		}
		if(rev->allpassR[i].buf != NULL) {
			free(rev->allpassR[i].buf);
			rev->allpassR[i].buf = NULL;
		}
	}
	free_delay(&(rev->pdelay));
}

static inline void do_freeverb_allpass(int32 *stream, int32 *buf, int32 size, int32 *index, int32 feedback)
{
	int32 bufout, output;
	bufout = buf[*index];
	output = -*stream + bufout;
	buf[*index] = *stream + imuldiv24(bufout, feedback);
	if (++*index >= size) {*index = 0;}
	*stream = output;
}

static inline void do_freeverb_comb(int32 input, int32 *stream, int32 *buf, int32 size, int32 *index,
									int32 damp1, int32 damp2, int32 *fs, int32 feedback)
{
	int32 output;
	output = buf[*index];
	*fs = imuldiv24(output, damp2) + imuldiv24(*fs, damp1);
	buf[*index] = input + imuldiv24(*fs, feedback);
	if (++*index >= size) {*index = 0;}
	*stream += output;
}

static void do_ch_freeverb(int32 *buf, int32 count, InfoFreeverb *rev)
{
	int32 i, k = 0;
	int32 outl, outr, input;
	comb *combL = rev->combL, *combR = rev->combR;
	allpass *allpassL = rev->allpassL, *allpassR = rev->allpassR;
	delay *pdelay = &(rev->pdelay);

	if(count == MAGIC_INIT_EFFECT_INFO) {
		alloc_freeverb_buf(rev);
		update_freeverb(rev);
		init_freeverb(rev);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_freeverb_buf(rev);
		return;
	}

	for (k = 0; k < count; k++)
	{
		input = reverb_effect_buffer[k] + reverb_effect_buffer[k + 1];
		outl = outr = reverb_effect_buffer[k] = reverb_effect_buffer[k + 1] = 0;

		do_delay(&input, pdelay->buf, pdelay->size, &pdelay->index);

		for (i = 0; i < numcombs; i++) {
			do_freeverb_comb(input, &outl, combL[i].buf, combL[i].size, &combL[i].index,
				combL[i].damp1i, combL[i].damp2i, &combL[i].filterstore, combL[i].feedbacki);
			do_freeverb_comb(input, &outr, combR[i].buf, combR[i].size, &combR[i].index,
				combR[i].damp1i, combR[i].damp2i, &combR[i].filterstore, combR[i].feedbacki);
		}
		for (i = 0; i < numallpasses; i++) {
			do_freeverb_allpass(&outl, allpassL[i].buf, allpassL[i].size, &allpassL[i].index, allpassL[i].feedbacki);
			do_freeverb_allpass(&outr, allpassR[i].buf, allpassR[i].size, &allpassR[i].index, allpassR[i].feedbacki);
		}
		buf[k] += imuldiv24(outl, rev->wet1i) + imuldiv24(outr, rev->wet2i);
		buf[k + 1] += imuldiv24(outr, rev->wet1i) + imuldiv24(outl, rev->wet2i);
		++k;
	}
}

/*                                 */
/*  Reverb: Delay & Panning Delay  */
/*                                 */
/*! initialize Reverb: Delay Effect; this implementation is specialized for system effect. */
static void init_ch_reverb_delay(InfoDelay3 *info)
{
	int32 x;
	info->size[0] = (double)reverb_status_gs.time * 3.75f * play_mode->rate / 1000.0f;
	x = info->size[0] + 1;	/* allowance */
	set_delay(&(info->delayL), x);
	set_delay(&(info->delayR), x);
	info->index[0] = x - info->size[0];
	info->level[0] = (double)reverb_status_gs.level * 1.82f / 127.0f;
	info->feedback = sqrt((double)reverb_status_gs.delay_feedback / 127.0f) * 0.98f;
	info->leveli[0] = TIM_FSCALE(info->level[0], 24);
	info->feedbacki = TIM_FSCALE(info->feedback, 24);
}

static void free_ch_reverb_delay(InfoDelay3 *info)
{
	free_delay(&(info->delayL));
	free_delay(&(info->delayR));
}

/*! Reverb: Panning Delay Effect; this implementation is specialized for system effect. */
static void do_ch_reverb_panning_delay(int32 *buf, int32 count, InfoDelay3 *info)
{
	int32 i, l, r;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], level0i = info->leveli[0],
		feedbacki = info->feedbacki;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_ch_reverb_delay(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_ch_reverb_delay(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
		bufL[buf_index] = reverb_effect_buffer[i] + imuldiv24(bufR[index0], feedbacki);
		l = imuldiv24(bufL[index0], level0i);
		bufR[buf_index] = reverb_effect_buffer[i + 1] + imuldiv24(bufL[index0], feedbacki);
		r = imuldiv24(bufR[index0], level0i);

		buf[i] += r;
		buf[++i] += l;

		if (++index0 == buf_size) {index0 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
	info->index[0] = index0;
	delayL->index = delayR->index = buf_index;
}

/*! Reverb: Normal Delay Effect; this implementation is specialized for system effect. */
static void do_ch_reverb_normal_delay(int32 *buf, int32 count, InfoDelay3 *info)
{
	int32 i;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], level0i = info->leveli[0],
		feedbacki = info->feedbacki;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_ch_reverb_delay(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_ch_reverb_delay(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
		bufL[buf_index] = reverb_effect_buffer[i] + imuldiv24(bufL[index0], feedbacki);
		buf[i] += imuldiv24(bufL[index0], level0i);

		bufR[buf_index] = reverb_effect_buffer[++i] + imuldiv24(bufR[index0], feedbacki);
		buf[i] += imuldiv24(bufR[index0], level0i);

		if (++index0 == buf_size) {index0 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
	info->index[0] = index0;
	delayL->index = delayR->index = buf_index;
}

/*                      */
/*  Plate Reverberator  */
/*                      */
#define PLATE_SAMPLERATE 29761.0
#define PLATE_DECAY 0.50
#define PLATE_DECAY_DIFFUSION1 0.70
#define PLATE_DECAY_DIFFUSION2 0.50
#define PLATE_INPUT_DIFFUSION1 0.750
#define PLATE_INPUT_DIFFUSION2 0.625
#define PLATE_BANDWIDTH 0.9955
#define PLATE_DAMPING 0.0005
#define PLATE_WET 0.25

/*! calculate delay sample in current sample-rate */
static inline int32 get_plate_delay(double delay, double t)
{
	return (int32)(delay * play_mode->rate * t / PLATE_SAMPLERATE);
}

/*! Plate Reverberator; this implementation is specialized for system effect. */
static void do_ch_plate_reverb(int32 *buf, int32 count, InfoPlateReverb *info)
{
	int32 i;
	int32 x, xd, val, outl, outr, temp1, temp2, temp3;
	delay *pd = &(info->pd), *od1l = &(info->od1l), *od2l = &(info->od2l),
		*od3l = &(info->od3l), *od4l = &(info->od4l), *od5l = &(info->od5l),
		*od6l = &(info->od6l), *od1r = &(info->od1r), *od2r = &(info->od2r),
		*od3r = &(info->od3r), *od4r = &(info->od4r), *od5r = &(info->od5r),
		*od7r = &(info->od7r), *od7l = &(info->od7l), *od6r = &(info->od6r),
		*td1 = &(info->td1), *td2 = &(info->td2), *td1d = &(info->td1d), *td2d = &(info->td2d);
	allpass *ap1 = &(info->ap1), *ap2 = &(info->ap2), *ap3 = &(info->ap3),
		*ap4 = &(info->ap4), *ap6 = &(info->ap6), *ap6d = &(info->ap6d);
	mod_allpass *ap5 = &(info->ap5), *ap5d = &(info->ap5d);
	lfo *lfo1 = &(info->lfo1), *lfo1d = &(info->lfo1d);
	filter_lowpass1 *lpf1 = &(info->lpf1), *lpf2 = &(info->lpf2);
	int32 t1 = info->t1, t1d = info->t1d;
	int32 decayi = info->decayi, ddif1i = info->ddif1i, ddif2i = info->ddif2i,
		idif1i = info->idif1i, idif2i = info->idif2i;
	double t;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_lfo(lfo1, 1.30f, LFO_SINE, 0);
		init_lfo(lfo1d, 1.30f, LFO_SINE, 0);
		t = reverb_time_table[reverb_status_gs.time] / reverb_time_table[64] - 1.0;
		t = 1.0 + t / 2;
		set_delay(pd, reverb_status_gs.pre_delay_time * play_mode->rate / 1000);
		set_delay(td1, get_plate_delay(4453, t)),
		set_delay(td1d, get_plate_delay(4217, t));
		set_delay(td2, get_plate_delay(3720, t));
		set_delay(td2d, get_plate_delay(3163, t));
		set_delay(od1l, get_plate_delay(266, t));
		set_delay(od2l, get_plate_delay(2974, t));
		set_delay(od3l, get_plate_delay(1913, t));
		set_delay(od4l, get_plate_delay(1996, t));
		set_delay(od5l, get_plate_delay(1990, t));
		set_delay(od6l, get_plate_delay(187, t));
		set_delay(od7l, get_plate_delay(1066, t));
		set_delay(od1r, get_plate_delay(353, t));
		set_delay(od2r, get_plate_delay(3627, t));
		set_delay(od3r, get_plate_delay(1228, t));
		set_delay(od4r, get_plate_delay(2673, t));
		set_delay(od5r, get_plate_delay(2111, t));
		set_delay(od6r, get_plate_delay(335, t));
		set_delay(od7r, get_plate_delay(121, t));
		set_allpass(ap1, get_plate_delay(142, t), PLATE_INPUT_DIFFUSION1);
		set_allpass(ap2, get_plate_delay(107, t), PLATE_INPUT_DIFFUSION1);
		set_allpass(ap3, get_plate_delay(379, t), PLATE_INPUT_DIFFUSION2);
		set_allpass(ap4, get_plate_delay(277, t), PLATE_INPUT_DIFFUSION2);
		set_allpass(ap6, get_plate_delay(1800, t), PLATE_DECAY_DIFFUSION2);
		set_allpass(ap6d, get_plate_delay(2656, t), PLATE_DECAY_DIFFUSION2);
		set_mod_allpass(ap5, get_plate_delay(672, t), get_plate_delay(16, t), PLATE_DECAY_DIFFUSION1);
		set_mod_allpass(ap5d, get_plate_delay(908, t), get_plate_delay(16, t), PLATE_DECAY_DIFFUSION1);
		lpf1->a = PLATE_BANDWIDTH, lpf2->a = 1.0 - PLATE_DAMPING;
		init_filter_lowpass1(lpf1);
		init_filter_lowpass1(lpf2);
		info->t1 = info->t1d = 0;
		info->decay = PLATE_DECAY;
		info->decayi = TIM_FSCALE(info->decay, 24);
		info->ddif1 = PLATE_DECAY_DIFFUSION1;
		info->ddif1i = TIM_FSCALE(info->ddif1, 24);
		info->ddif2 = PLATE_DECAY_DIFFUSION2;
		info->ddif2i = TIM_FSCALE(info->ddif2, 24);
		info->idif1 = PLATE_INPUT_DIFFUSION1;
		info->idif1i = TIM_FSCALE(info->idif1, 24);
		info->idif2 = PLATE_INPUT_DIFFUSION2;
		info->idif2i = TIM_FSCALE(info->idif2, 24);
		info->wet = PLATE_WET * (double)reverb_status_gs.level / 127.0;
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(pd);	free_delay(td1); free_delay(td1d); free_delay(td2);
		free_delay(td2d); free_delay(od1l); free_delay(od2l); free_delay(od3l);
		free_delay(od4l); free_delay(od5l); free_delay(od6l); free_delay(od7l);
		free_delay(od1r); free_delay(od2r);	free_delay(od3r); free_delay(od4r);
		free_delay(od5r); free_delay(od6r);	free_delay(od7r); free_allpass(ap1);
		free_allpass(ap2); free_allpass(ap3); free_allpass(ap4); free_allpass(ap6);
		free_allpass(ap6d);	free_mod_allpass(ap5); free_mod_allpass(ap5d);
		return;
	}

	for (i = 0; i < count; i++)
	{
		outr = outl = 0;
		x = (reverb_effect_buffer[i] + reverb_effect_buffer[i + 1]) >> 1;
		reverb_effect_buffer[i] = reverb_effect_buffer[i + 1] = 0;

		do_delay(&x, pd->buf, pd->size, &pd->index);
		do_filter_lowpass1(&x, &lpf1->x1l, lpf1->ai, lpf1->iai);
		do_allpass(&x, ap1->buf, ap1->size, &ap1->index, idif1i);
		do_allpass(&x, ap2->buf, ap2->size, &ap2->index, idif1i);
		do_allpass(&x, ap3->buf, ap3->size, &ap3->index, idif2i);
		do_allpass(&x, ap4->buf, ap4->size, &ap4->index, idif2i);

		/* tank structure */
		xd = x;
		x += imuldiv24(t1d, decayi);
		val = do_lfo(lfo1);
		do_mod_allpass(&x, ap5->buf, ap5->size, &ap5->rindex, &ap5->windex,
			ap5->ndelay, ap5->depth, val, &ap5->hist, ddif1i);
		temp1 = temp2 = temp3 = x;	/* n_out_1 */
		do_delay(&temp1, od5l->buf, od5l->size, &od5l->index);
		outl -= temp1;	/* left output 5 */
		do_delay(&temp2, od1r->buf, od1r->size, &od1r->index);
		outr += temp2;	/* right output 1 */
		do_delay(&temp3, od2r->buf, od2r->size, &od2r->index);
		outr += temp3;	/* right output 2 */
		do_delay(&x, td1->buf, td1->size, &td1->index);
		do_filter_lowpass1(&x, &lpf2->x1l, lpf2->ai, lpf2->iai);
		temp1 = temp2 = x;	/* n_out_2 */
		do_delay(&temp1, od6l->buf, od6l->size, &od6l->index);
		outl -= temp1;	/* left output 6 */
		do_delay(&temp2, od3r->buf, od3r->size, &od3r->index);
		outr -= temp2;	/* right output 3 */
		x = imuldiv24(x, decayi);
		do_allpass(&x, ap6->buf, ap6->size, &ap6->index, ddif2i);
		temp1 = temp2 = x;	/* n_out_3 */
		do_delay(&temp1, od7l->buf, od7l->size, &od7l->index);
		outl -= temp1;	/* left output 7 */
		do_delay(&temp2, od4r->buf, od4r->size, &od4r->index);
		outr += temp2;	/* right output 4 */
		do_delay(&x, td2->buf, td2->size, &td2->index);
		t1 = x;

		xd += imuldiv24(t1, decayi);
		val = do_lfo(lfo1d);
		do_mod_allpass(&x, ap5d->buf, ap5d->size, &ap5d->rindex, &ap5d->windex,
			ap5d->ndelay, ap5d->depth, val, &ap5d->hist, ddif1i);
		temp1 = temp2 = temp3 = xd;	/* n_out_4 */
		do_delay(&temp1, od1l->buf, od1l->size, &od1l->index);
		outl += temp1;	/* left output 1 */
		do_delay(&temp2, od2l->buf, od2l->size, &od2l->index);
		outl += temp2;	/* left output 2 */
		do_delay(&temp3, od6r->buf, od6r->size, &od6r->index);
		outr -= temp3;	/* right output 6 */
		do_delay(&xd, td1d->buf, td1d->size, &td1d->index);
		do_filter_lowpass1(&xd, &lpf2->x1r, lpf2->ai, lpf2->iai);
		temp1 = temp2 = xd;	/* n_out_5 */
		do_delay(&temp1, od3l->buf, od3l->size, &od3l->index);
		outl -= temp1;	/* left output 3 */
		do_delay(&temp2, od6r->buf, od6r->size, &od6r->index);
		outr -= temp2;	/* right output 6 */
		xd = imuldiv24(xd, decayi);
		do_allpass(&xd, ap6d->buf, ap6d->size, &ap6d->index, ddif2i);
		temp1 = temp2 = xd;	/* n_out_6 */
		do_delay(&temp1, od4l->buf, od4l->size, &od4l->index);
		outl += temp1;	/* left output 4 */
		do_delay(&temp2, od7r->buf, od7r->size, &od7r->index);
		outr -= temp2;	/* right output 7 */
		do_delay(&xd, td2d->buf, td2d->size, &td2d->index);
		t1d = xd;

		buf[i] += outl;
		buf[i + 1] += outr;

		++i;
	}
	info->t1 = t1, info->t1d = t1d;
}

/*! initialize Reverb Effect */
void init_reverb(void)
{
	init_filter_lowpass1(&(reverb_status_gs.lpf));
	/* Only initialize freeverb if stereo output */
	/* Old non-freeverb must be initialized for mono reverb not to crash */
	if (! (play_mode->encoding & PE_MONO)
			&& (opt_reverb_control == 3 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x100)))) {
		switch(reverb_status_gs.character) {	/* select reverb algorithm */
		case 5:	/* Plate Reverb */
			do_ch_plate_reverb(NULL, MAGIC_INIT_EFFECT_INFO, &(reverb_status_gs.info_plate_reverb));
			REV_INP_LEV = reverb_status_gs.info_plate_reverb.wet;
			break;
		case 6:	/* Delay */
			do_ch_reverb_normal_delay(NULL, MAGIC_INIT_EFFECT_INFO, &(reverb_status_gs.info_reverb_delay));
			REV_INP_LEV = 1.0;
			break;
		case 7: /* Panning Delay */
			do_ch_reverb_panning_delay(NULL, MAGIC_INIT_EFFECT_INFO, &(reverb_status_gs.info_reverb_delay));
			REV_INP_LEV = 1.0;
			break;
		default: /* Freeverb */
			do_ch_freeverb(NULL, MAGIC_INIT_EFFECT_INFO, &(reverb_status_gs.info_freeverb));
			REV_INP_LEV = reverb_status_gs.info_freeverb.wet;
			break;
		}
	} else {	/* Old Reverb */
		do_ch_standard_reverb(NULL, MAGIC_INIT_EFFECT_INFO, &(reverb_status_gs.info_standard_reverb));
		REV_INP_LEV = 1.0;
	}
	memset(reverb_effect_buffer, 0, reverb_effect_bufsize);
	memset(direct_buffer, 0, direct_bufsize);
}

void do_ch_reverb(int32 *buf, int32 count)
{
#ifdef SYS_EFFECT_PRE_LPF
	if ((opt_reverb_control == 3 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x100))) && reverb_status_gs.pre_lpf)
		do_filter_lowpass1_stereo(reverb_effect_buffer, count, &(reverb_status_gs.lpf));
#endif /* SYS_EFFECT_PRE_LPF */
	if (opt_reverb_control == 3 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x100))) {
		switch(reverb_status_gs.character) {	/* select reverb algorithm */
		case 5:	/* Plate Reverb */
			do_ch_plate_reverb(buf, count, &(reverb_status_gs.info_plate_reverb));
			REV_INP_LEV = reverb_status_gs.info_plate_reverb.wet;
			break;
		case 6:	/* Delay */
			do_ch_reverb_normal_delay(buf, count, &(reverb_status_gs.info_reverb_delay));
			REV_INP_LEV = 1.0;
			break;
		case 7: /* Panning Delay */
			do_ch_reverb_panning_delay(buf, count, &(reverb_status_gs.info_reverb_delay));
			REV_INP_LEV = 1.0;
			break;
		default: /* Freeverb */
			do_ch_freeverb(buf, count, &(reverb_status_gs.info_freeverb));
			REV_INP_LEV = reverb_status_gs.info_freeverb.wet;
			break;
		}
	} else {	/* Old Reverb */
		do_ch_standard_reverb(buf, count, &(reverb_status_gs.info_standard_reverb));
	}
}

void do_mono_reverb(int32 *buf, int32 count)
{
	do_ch_standard_reverb_mono(buf, count, &(reverb_status_gs.info_standard_reverb));
}

/*                   */
/*   Delay Effect    */
/*                   */
static int32 delay_effect_buffer[AUDIO_BUFFER_SIZE * 2];
static void do_ch_3tap_delay(int32 *, int32, InfoDelay3 *);
static void do_ch_cross_delay(int32 *, int32, InfoDelay3 *);
static void do_ch_normal_delay(int32 *, int32, InfoDelay3 *);

void init_ch_delay(void)
{
	memset(delay_effect_buffer, 0, sizeof(delay_effect_buffer));
	init_filter_lowpass1(&(delay_status_gs.lpf));
	do_ch_3tap_delay(NULL, MAGIC_INIT_EFFECT_INFO, &(delay_status_gs.info_delay));
}

void do_ch_delay(int32 *buf, int32 count)
{
#ifdef SYS_EFFECT_PRE_LPF
	if ((opt_reverb_control == 3 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x100))) && delay_status_gs.pre_lpf)
		do_filter_lowpass1_stereo(delay_effect_buffer, count, &(delay_status_gs.lpf));
#endif /* SYS_EFFECT_PRE_LPF */
	switch (delay_status_gs.type) {
	case 1:
		do_ch_3tap_delay(buf, count, &(delay_status_gs.info_delay));
		break;
	case 2:
		do_ch_cross_delay(buf, count, &(delay_status_gs.info_delay));
		break;
	default:
		do_ch_normal_delay(buf, count, &(delay_status_gs.info_delay));
		break;
	}
}

#if OPT_MODE != 0
#if defined(_MSC_VER) || defined(__WATCOMC__) || (defined(__BORLANDC__) && (__BORLANDC__ >= 1380) )
void set_ch_delay(int32 *buf, int32 count, int32 level)
{
	int32 *dbuf = delay_effect_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		mov		ebx, [level]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		imul	ebx
		shr		eax, 16
		shl		edx, 16
		or		eax, edx	/* u */
		mov		edx, [edi]	/* v */
		add		esi, 4		/* u */	
		add		edx, eax	/* v */
		mov		[edi], edx	/* u */
		add		edi, 4		/* v */
		dec		ecx			/* u */
		jnz		L1			/* v */
L2:
	}
}
#else
void set_ch_delay(register int32 *sbuffer, int32 n, int32 level)
{
    register int32 i;
	int32 *buf = delay_effect_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	for(i = n - 1; i >= 0; i--) {buf[i] += imuldiv16(sbuffer[i], level);}
}
#endif	/* _MSC_VER */
#else
void set_ch_delay(register int32 *sbuffer, int32 n, int32 level)
{
    register int32 i;
	if(!level) {return;}
    FLOAT_T send_level = (FLOAT_T)level / 127.0f;

    for(i = 0; i < n; i++)
    {
        delay_effect_buffer[i] += sbuffer[i] * send_level;
    }
}
#endif /* OPT_MODE != 0 */

/*! initialize Delay Effect; this implementation is specialized for system effect. */
static void init_ch_3tap_delay(InfoDelay3 *info)
{
	int32 i, x;

	info->size[0] = delay_status_gs.sample_c;
	info->size[1] = delay_status_gs.sample_l;
	info->size[2] = delay_status_gs.sample_r;
	x = info->size[0];	/* find maximum value */
	for (i = 1; i < 3; i++) {
		if (info->size[i] > x) {x = info->size[i];}
	}
	x += 1;	/* allowance */
	set_delay(&(info->delayL), x);
	set_delay(&(info->delayR), x);
	for (i = 0; i < 3; i++) {	/* set start-point */
		info->index[i] = x - info->size[i];
	}
	info->level[0] = delay_status_gs.level_ratio_c * MASTER_DELAY_LEVEL;
	info->level[1] = delay_status_gs.level_ratio_l * MASTER_DELAY_LEVEL;
	info->level[2] = delay_status_gs.level_ratio_r * MASTER_DELAY_LEVEL;
	info->feedback = delay_status_gs.feedback_ratio;
	info->send_reverb = delay_status_gs.send_reverb_ratio * REV_INP_LEV;
	for (i = 0; i < 3; i++) {
		info->leveli[i] = TIM_FSCALE(info->level[i], 24);
	}
	info->feedbacki = TIM_FSCALE(info->feedback, 24);
	info->send_reverbi = TIM_FSCALE(info->send_reverb, 24);
}

static void free_ch_3tap_delay(InfoDelay3 *info)
{
	free_delay(&(info->delayL));
	free_delay(&(info->delayR));
}

/*! 3-Tap Stereo Delay Effect; this implementation is specialized for system effect. */
static void do_ch_3tap_delay(int32 *buf, int32 count, InfoDelay3 *info)
{
	int32 i, x;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], index1 = info->index[1], index2 = info->index[2];
	int32 level0i = info->leveli[0], level1i = info->leveli[1], level2i = info->leveli[2],
		feedbacki = info->feedbacki, send_reverbi = info->send_reverbi;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_ch_3tap_delay(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_ch_3tap_delay(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
		bufL[buf_index] = delay_effect_buffer[i] + imuldiv24(bufL[index0], feedbacki);
		x = imuldiv24(bufL[index0], level0i) + imuldiv24(bufL[index1] + bufR[index1], level1i);
		buf[i] += x;
		reverb_effect_buffer[i] += imuldiv24(x, send_reverbi);

		bufR[buf_index] = delay_effect_buffer[++i] + imuldiv24(bufR[index0], feedbacki);
		x = imuldiv24(bufR[index0], level0i) + imuldiv24(bufL[index2] + bufR[index2], level2i);
		buf[i] += x;
		reverb_effect_buffer[i] += imuldiv24(x, send_reverbi);

		if (++index0 == buf_size) {index0 = 0;}
		if (++index1 == buf_size) {index1 = 0;}
		if (++index2 == buf_size) {index2 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	memset(delay_effect_buffer, 0, sizeof(int32) * count);
	info->index[0] = index0, info->index[1] = index1, info->index[2] = index2;
	delayL->index = delayR->index = buf_index;
}

/*! Cross Delay Effect; this implementation is specialized for system effect. */
static void do_ch_cross_delay(int32 *buf, int32 count, InfoDelay3 *info)
{
	int32 i, l, r;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], level0i = info->leveli[0],
		feedbacki = info->feedbacki, send_reverbi = info->send_reverbi;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_ch_3tap_delay(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_ch_3tap_delay(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
		bufL[buf_index] = delay_effect_buffer[i] + imuldiv24(bufR[index0], feedbacki);
		l = imuldiv24(bufL[index0], level0i);
		bufR[buf_index] = delay_effect_buffer[i + 1] + imuldiv24(bufL[index0], feedbacki);
		r = imuldiv24(bufR[index0], level0i);

		buf[i] += r;
		reverb_effect_buffer[i] += imuldiv24(r, send_reverbi);
		buf[++i] += l;
		reverb_effect_buffer[i] += imuldiv24(l, send_reverbi);

		if (++index0 == buf_size) {index0 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	memset(delay_effect_buffer, 0, sizeof(int32) * count);
	info->index[0] = index0;
	delayL->index = delayR->index = buf_index;
}

/*! Normal Delay Effect; this implementation is specialized for system effect. */
static void do_ch_normal_delay(int32 *buf, int32 count, InfoDelay3 *info)
{
	int32 i, x;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], level0i = info->leveli[0],
		feedbacki = info->feedbacki, send_reverbi = info->send_reverbi;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_ch_3tap_delay(info);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_ch_3tap_delay(info);
		return;
	}

	for (i = 0; i < count; i++)
	{
		bufL[buf_index] = delay_effect_buffer[i] + imuldiv24(bufL[index0], feedbacki);
		x = imuldiv24(bufL[index0], level0i);
		buf[i] += x;
		reverb_effect_buffer[i] += imuldiv24(x, send_reverbi);

		bufR[buf_index] = delay_effect_buffer[++i] + imuldiv24(bufR[index0], feedbacki);
		x = imuldiv24(bufR[index0], level0i);
		buf[i] += x;
		reverb_effect_buffer[i] += imuldiv24(x, send_reverbi);

		if (++index0 == buf_size) {index0 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	memset(delay_effect_buffer, 0, sizeof(int32) * count);
	info->index[0] = index0;
	delayL->index = delayR->index = buf_index;
}

/*                             */
/*        Chorus Effect        */
/*                             */
static int32 chorus_effect_buffer[AUDIO_BUFFER_SIZE * 2];

/*! Stereo Chorus; this implementation is specialized for system effect. */
static void do_ch_stereo_chorus(int32 *buf, int32 count, InfoStereoChorus *info)
{
	int32 i, output, f0, f1, v0, v1;
	int32 *bufL = info->delayL.buf, *bufR = info->delayR.buf,
		*lfobufL = info->lfoL.buf, *lfobufR = info->lfoR.buf,
		icycle = info->lfoL.icycle, cycle = info->lfoL.cycle,
		leveli = info->leveli, feedbacki = info->feedbacki,
		send_reverbi = info->send_reverbi, send_delayi = info->send_delayi,
		depth = info->depth, pdelay = info->pdelay, rpt0 = info->rpt0;
	int32 wpt0 = info->wpt0, spt0 = info->spt0, spt1 = info->spt1,
		hist0 = info->hist0, hist1 = info->hist1, lfocnt = info->lfoL.count;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_lfo(&(info->lfoL), (double)chorus_status_gs.rate * 0.122f, LFO_TRIANGULAR, 0);
		init_lfo(&(info->lfoR), (double)chorus_status_gs.rate * 0.122f, LFO_TRIANGULAR, 90);
		info->pdelay = chorus_delay_time_table[chorus_status_gs.delay] * (double)play_mode->rate / 1000.0f;
		info->depth = (double)(chorus_status_gs.depth + 1) / 3.2f * (double)play_mode->rate / 1000.0f;
		info->pdelay -= info->depth / 2;	/* NOMINAL_DELAY to delay */
		if (info->pdelay < 1) {info->pdelay = 1;}
		info->rpt0 = info->pdelay + info->depth + 2;	/* allowance */
		set_delay(&(info->delayL), info->rpt0);
		set_delay(&(info->delayR), info->rpt0);
		info->feedback = (double)chorus_status_gs.feedback * 0.763f / 100.0f;
		info->level = (double)chorus_status_gs.level / 127.0f * MASTER_CHORUS_LEVEL;
		info->send_reverb = (double)chorus_status_gs.send_reverb * 0.787f / 100.0f * REV_INP_LEV;
		info->send_delay = (double)chorus_status_gs.send_delay * 0.787f / 100.0f;
		info->feedbacki = TIM_FSCALE(info->feedback, 24);
		info->leveli = TIM_FSCALE(info->level, 24);
		info->send_reverbi = TIM_FSCALE(info->send_reverb, 24);
		info->send_delayi = TIM_FSCALE(info->send_delay, 24);
		info->wpt0 = info->spt0 = info->spt1 = info->hist0 = info->hist1 = 0;
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	/* LFO */
	f0 = imuldiv24(lfobufL[imuldiv24(lfocnt, icycle)], depth);
	spt0 = wpt0 - pdelay - (f0 >> 8);	/* integral part of delay */
	f0 = 0xFF - (f0 & 0xFF);	/* (1 - frac) * 256 */
	if(spt0 < 0) {spt0 += rpt0;}
	f1 = imuldiv24(lfobufR[imuldiv24(lfocnt, icycle)], depth);
	spt1 = wpt0 - pdelay - (f1 >> 8);	/* integral part of delay */
	f1 = 0xFF - (f1 & 0xFF);	/* (1 - frac) * 256 */
	if(spt1 < 0) {spt1 += rpt0;}
	
	for(i = 0; i < count; i++) {
		v0 = bufL[spt0];
		v1 = bufR[spt1];

		/* LFO */
		if(++wpt0 == rpt0) {wpt0 = 0;}
		f0 = imuldiv24(lfobufL[imuldiv24(lfocnt, icycle)], depth);
		spt0 = wpt0 - pdelay - (f0 >> 8);	/* integral part of delay */
		f0 = 0xFF - (f0 & 0xFF);	/* (1 - frac) * 256 */
		if(spt0 < 0) {spt0 += rpt0;}
		f1 = imuldiv24(lfobufR[imuldiv24(lfocnt, icycle)], depth);
		spt1 = wpt0 - pdelay - (f1 >> 8);	/* integral part of delay */
		f1 = 0xFF - (f1 & 0xFF);	/* (1 - frac) * 256 */
		if(spt1 < 0) {spt1 += rpt0;}
		if(++lfocnt == cycle) {lfocnt = 0;}

		/* left */
		/* delay with all-pass interpolation */
		output = hist0 = v0 + imuldiv8(bufL[spt0] - hist0, f0);
		bufL[wpt0] = chorus_effect_buffer[i] + imuldiv24(output, feedbacki);
		output = imuldiv24(output, leveli);
		buf[i] += output;
		/* send to other system effects (it's peculiar to GS) */
		reverb_effect_buffer[i] += imuldiv24(output, send_reverbi);
		delay_effect_buffer[i] += imuldiv24(output, send_delayi);

		/* right */
		/* delay with all-pass interpolation */
		output = hist1 = v1 + imuldiv8(bufR[spt1] - hist1, f1);
		bufR[wpt0] = chorus_effect_buffer[++i] + imuldiv24(output, feedbacki);
		output = imuldiv24(output, leveli);
		buf[i] += output;
		/* send to other system effects (it's peculiar to GS) */
		reverb_effect_buffer[i] += imuldiv24(output, send_reverbi);
		delay_effect_buffer[i] += imuldiv24(output, send_delayi);
	}
	memset(chorus_effect_buffer, 0, sizeof(int32) * count);
	info->wpt0 = wpt0, info->spt0 = spt0, info->spt1 = spt1,
		info->hist0 = hist0, info->hist1 = hist1;
	info->lfoL.count = info->lfoR.count = lfocnt;
}

void init_ch_chorus(void)
{
	/* clear delay-line of LPF */
	init_filter_lowpass1(&(chorus_status_gs.lpf));
	do_ch_stereo_chorus(NULL, MAGIC_INIT_EFFECT_INFO, &(chorus_status_gs.info_stereo_chorus));
	memset(chorus_effect_buffer, 0, sizeof(chorus_effect_buffer));
}

#if OPT_MODE != 0	/* fixed-point implementation */
#if defined(_MSC_VER) || defined(__WATCOMC__) || ( defined(__BORLANDC__) && (__BORLANDC__ >= 1380) )
void set_ch_chorus(int32 *buf, int32 count, int32 level)
{
	int32 *dbuf = chorus_effect_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		mov		ebx, [level]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		imul	ebx
		shr		eax, 16
		shl		edx, 16
		or		eax, edx	/* u */
		mov		edx, [edi]	/* v */
		add		esi, 4		/* u */	
		add		edx, eax	/* v */
		mov		[edi], edx	/* u */
		add		edi, 4		/* v */
		dec		ecx			/* u */
		jnz		L1			/* v */
L2:
	}
}
#else
void set_ch_chorus(register int32 *sbuffer,int32 n, int32 level)
{
    register int32 i;
	int32 *buf = chorus_effect_buffer;
	if(!level) {return;}
	level = level * 65536 / 127;

	for(i = n - 1; i >= 0; i--) {buf[i] += imuldiv16(sbuffer[i], level);}
}
#endif	/* _MSC_VER */
#else	/* floating-point implementation */
void set_ch_chorus(register int32 *sbuffer,int32 n, int32 level)
{
    register int32 i;
    register int32 count = n;
	if(!level) {return;}
    FLOAT_T send_level = (FLOAT_T)level / 127.0;

    for(i = 0; i < count; i++)
    {
		chorus_effect_buffer[i] += sbuffer[i] * send_level;
    }
}
#endif /* OPT_MODE != 0 */

void do_ch_chorus(int32 *buf, int32 count)
{
#ifdef SYS_EFFECT_PRE_LPF
	if ((opt_reverb_control == 3 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x100))) && chorus_status_gs.pre_lpf)
		do_filter_lowpass1_stereo(chorus_effect_buffer, count, &(chorus_status_gs.lpf));
#endif /* SYS_EFFECT_PRE_LPF */

	do_ch_stereo_chorus(buf, count, &(chorus_status_gs.info_stereo_chorus));
}

/*                             */
/*       EQ (Equalizer)        */
/*                             */
static int32 eq_buffer[AUDIO_BUFFER_SIZE * 2];

void init_eq_gs()
{
	memset(eq_buffer, 0, sizeof(eq_buffer));
	calc_filter_shelving_low(&(eq_status_gs.lsf));
	calc_filter_shelving_high(&(eq_status_gs.hsf));
}

void do_ch_eq_gs(int32* buf, int32 count)
{
	register int32 i;

	do_shelving_filter_stereo(eq_buffer, count, &(eq_status_gs.lsf));
	do_shelving_filter_stereo(eq_buffer, count, &(eq_status_gs.hsf));

	for(i = 0; i < count; i++) {
		buf[i] += eq_buffer[i];
		eq_buffer[i] = 0;
	}
}

void do_ch_eq_xg(int32* buf, int32 count, struct part_eq_xg *p)
{
	if(p->bass - 0x40 != 0) {
		do_shelving_filter_stereo(buf, count, &(p->basss));
	}
	if(p->treble - 0x40 != 0) {
		do_shelving_filter_stereo(buf, count, &(p->trebles));
	}
}

void do_multi_eq_xg(int32* buf, int32 count)
{
	if(multi_eq_xg.valid1) {
		if(multi_eq_xg.shape1) {	/* peaking */
			do_peaking_filter_stereo(buf, count, &(multi_eq_xg.eq1p));
		} else {	/* shelving */
			do_shelving_filter_stereo(buf, count, &(multi_eq_xg.eq1s));
		}
	}
	if(multi_eq_xg.valid2) {
		do_peaking_filter_stereo(buf, count, &(multi_eq_xg.eq2p));
	}
	if(multi_eq_xg.valid3) {
		do_peaking_filter_stereo(buf, count, &(multi_eq_xg.eq3p));
	}
	if(multi_eq_xg.valid4) {
		do_peaking_filter_stereo(buf, count, &(multi_eq_xg.eq4p));
	}
	if(multi_eq_xg.valid5) {
		if(multi_eq_xg.shape5) {	/* peaking */
			do_peaking_filter_stereo(buf, count, &(multi_eq_xg.eq5p));
		} else {	/* shelving */
			do_shelving_filter_stereo(buf, count, &(multi_eq_xg.eq5s));
		}
	}
}

#if OPT_MODE != 0
#if defined(_MSC_VER) || defined(__WATCOMC__) || ( defined(__BORLANDC__) && (__BORLANDC__ >= 1380) )
void set_ch_eq_gs(int32 *buf, int32 count)
{
	int32 *dbuf = eq_buffer;
	_asm {
		mov		ecx, [count]
		mov		esi, [buf]
		test	ecx, ecx
		jz		short L2
		mov		edi, [dbuf]
L1:		mov		eax, [esi]
		mov		ebx, [edi]
		add		esi, 4
		add		ebx, eax
		mov		[edi], ebx
		add		edi, 4
		dec		ecx
		jnz		L1
L2:
	}
}
#else
void set_ch_eq_gs(register int32 *buf, int32 n)
{
    register int32 i;

    for(i = n-1; i >= 0; i--)
    {
        eq_buffer[i] += buf[i];
    }
}
#endif	/* _MSC_VER */
#else
void set_ch_eq_gs(register int32 *sbuffer, int32 n)
{
    register int32  i;
    
	for(i = 0; i < n; i++)
    {
        eq_buffer[i] += sbuffer[i];
    }
}
#endif /* OPT_MODE != 0 */


/*                                  */
/*  Insertion and Variation Effect  */
/*                                  */
void do_insertion_effect_gs(int32 *buf, int32 count)
{
	do_effect_list(buf, count, insertion_effect_gs.ef);
}

void do_insertion_effect_xg(int32 *buf, int32 count, struct effect_xg_t *st)
{
	do_effect_list(buf, count, st->ef);
}

void do_variation_effect1_xg(int32 *buf, int32 count)
{
	int32 i, x;
	int32 send_reverbi = TIM_FSCALE((double)variation_effect_xg[0].send_reverb * (0.787f / 100.0f * REV_INP_LEV), 24),
		send_chorusi = TIM_FSCALE((double)variation_effect_xg[0].send_chorus * (0.787f / 100.0f), 24);
	if (variation_effect_xg[0].connection == XG_CONN_SYSTEM) {
		do_effect_list(delay_effect_buffer, count, variation_effect_xg[0].ef);
		for (i = 0; i < count; i++) {
			x = delay_effect_buffer[i];
			buf[i] += x;
			reverb_effect_buffer[i] += imuldiv24(x, send_reverbi);
			chorus_effect_buffer[i] += imuldiv24(x, send_chorusi);
		}
	}
	memset(delay_effect_buffer, 0, sizeof(int32) * count);
}

void do_ch_chorus_xg(int32 *buf, int32 count)
{
	int32 i;
	int32 send_reverbi = TIM_FSCALE((double)chorus_status_xg.send_reverb * (0.787f / 100.0f * REV_INP_LEV), 24);

	do_effect_list(chorus_effect_buffer, count, chorus_status_xg.ef);
	for (i = 0; i < count; i++) {
		buf[i] += chorus_effect_buffer[i];
		reverb_effect_buffer[i] += imuldiv24(chorus_effect_buffer[i], send_reverbi);
	}
	memset(chorus_effect_buffer, 0, sizeof(int32) * count);
}

void do_ch_reverb_xg(int32 *buf, int32 count)
{
	int32 i;

	do_effect_list(reverb_effect_buffer, count, reverb_status_xg.ef);
	for (i = 0; i < count; i++) {
		buf[i] += reverb_effect_buffer[i];
	}
	memset(reverb_effect_buffer, 0, sizeof(int32) * count);
}

void init_ch_effect_xg(void)
{
	memset(reverb_effect_buffer, 0, sizeof(reverb_effect_buffer));
	memset(chorus_effect_buffer, 0, sizeof(chorus_effect_buffer));
	memset(delay_effect_buffer, 0, sizeof(delay_effect_buffer));
}

void alloc_effect(EffectList *ef)
{
	int i;

	ef->engine = NULL;
	for(i = 0; effect_engine[i].type != -1; i++) {
		if (effect_engine[i].type == ef->type) {
			ef->engine = &(effect_engine[i]);
			break;
		}
	}
	if (ef->engine == NULL) {return;}

	if (ef->info != NULL) {
		free(ef->info);
		ef->info = NULL;
	}
	ef->info = safe_malloc(ef->engine->info_size);
	memset(ef->info, 0, ef->engine->info_size);

/*	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Effect Engine: %s", ef->engine->name); */
}

/*! allocate new effect item and add it into the tail of effect list.
    EffectList *efc: pointer to the top of effect list.
    int8 type: type of new effect item.
    void *info: pointer to infomation of new effect item. */
EffectList *push_effect(EffectList *efc, int type)
{
	EffectList *eft, *efn;
	if (type == EFFECT_NONE) {return NULL;}
	efn = (EffectList *)safe_malloc(sizeof(EffectList));
	memset(efn, 0, sizeof(EffectList));
	efn->type = type;
	efn->next_ef = NULL;
	efn->info = NULL;
	alloc_effect(efn);

	if(efc == NULL) {
		efc = efn;
	} else {
		eft = efc;
		while(eft->next_ef != NULL) {
			eft = eft->next_ef;
		}
		eft->next_ef = efn;
	}
	return efc;
}

/*! process all items of effect list. */
void do_effect_list(int32 *buf, int32 count, EffectList *ef)
{
	EffectList *efc = ef;
	if(ef == NULL) {return;}
	while(efc != NULL && efc->engine->do_effect != NULL)
	{
		(*efc->engine->do_effect)(buf, count, efc);
		efc = efc->next_ef;
	}
}

/*! free all items of effect list. */
void free_effect_list(EffectList *ef)
{
	EffectList *efc, *efn;
	efc = ef;
	if (efc == NULL) {return;}
	do {
		efn = efc->next_ef;
		if(efc->info != NULL) {
			(*efc->engine->do_effect)(NULL, MAGIC_FREE_EFFECT_INFO, efc);
			free(efc->info);
			efc->info = NULL;
		}
		efc->engine = NULL;
		free(efc);
		efc = NULL;
	} while ((efc = efn) != NULL);
}

/*! 2-Band EQ */
void do_eq2(int32 *buf, int32 count, EffectList *ef)
{
	InfoEQ2 *eq = (InfoEQ2 *)ef->info;
	if(count == MAGIC_INIT_EFFECT_INFO) {
		eq->lsf.q = 0;
		eq->lsf.freq = eq->low_freq;
		eq->lsf.gain = eq->low_gain;
		calc_filter_shelving_low(&(eq->lsf));
		eq->hsf.q = 0;
		eq->hsf.freq = eq->high_freq;
		eq->hsf.gain = eq->high_gain;
		calc_filter_shelving_high(&(eq->hsf));
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	if(eq->low_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->lsf));
	}
	if(eq->high_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->hsf));
	}
}

/*! panning (pan = [0, 127]) */
static inline int32 do_left_panning(int32 sample, int32 pan)
{
	return imuldiv8(sample, 256 - pan - pan);
}

static inline int32 do_right_panning(int32 sample, int32 pan)
{
	return imuldiv8(sample, pan + pan);
}

#define OD_BITS 28
#define OD_MAX_NEG (1.0 / (double)(1L << OD_BITS))
#define OD_DRIVE_GS 4.0
#define OD_LEVEL_GS 0.5
#define STEREO_OD_BITS 27
#define STEREO_OD_MAX_NEG (1.0 / (double)(1L << STEREO_OD_BITS))
#define OVERDRIVE_DIST 4.0
#define OVERDRIVE_RES 0.1
#define OVERDRIVE_LEVEL 1.0
#define OVERDRIVE_OFFSET 0
#define DISTORTION_DIST 40.0
#define DISTORTION_RES 0.2
#define DISTORTION_LEVEL 0.2
#define DISTORTION_OFFSET 0

static inline double calc_gs_drive(int val)
{
	return (OD_DRIVE_GS * (double)val / 127.0 + 1.0f);
}

/*! GS 0x0110: Overdrive 1 */
void do_overdrive1(int32 *buf, int32 count, EffectList *ef)
{
	InfoOverdrive1 *info = (InfoOverdrive1 *)ef->info;
	filter_moog *svf = &(info->svf);
	filter_biquad *lpf1 = &(info->lpf1);
	void (*do_amp_sim)(int32 *, int32) = info->amp_sim;
	int32 i, input, high, leveli = info->leveli, di = info->di,
		pan = info->pan, asdi = TIM_FSCALE(1.0f, 24);

	if(count == MAGIC_INIT_EFFECT_INFO) {
		/* decompositor */
		svf->freq = 500;
		svf->res_dB = 0;
		calc_filter_moog(svf);
		init_filter_moog(svf);
		/* amp simulator */
		info->amp_sim = do_dummy_clipping;
		if (info->amp_sw == 1) {
			if (info->amp_type <= 3) {info->amp_sim = do_soft_clipping2;}
		}
		/* waveshaper */
		info->di = TIM_FSCALE(calc_gs_drive(info->drive), 24);
		info->leveli = TIM_FSCALE(info->level * OD_LEVEL_GS, 24);
		/* anti-aliasing */
		lpf1->freq = 8000.0f;
		lpf1->q = 1.0f;
		calc_filter_biquad_low(lpf1);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	for(i = 0; i < count; i++) {
		input = (buf[i] + buf[i + 1]) >> 1;
		/* amp simulation */
		do_amp_sim(&input, asdi);
		/* decomposition */
		do_filter_moog(&input, &high, svf->f, svf->p, svf->q,
			&svf->b0, &svf->b1, &svf->b2, &svf->b3, &svf->b4);
		/* waveshaping */
		do_soft_clipping1(&high, di);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1l, &lpf1->x2l, &lpf1->y1l, &lpf1->y2l);
		/* mixing */
		input = imuldiv24(high + input, leveli);
		buf[i] = do_left_panning(input, pan);
		buf[i + 1] = do_right_panning(input, pan);
		++i;
	}
}

/*! GS 0x0111: Distortion 1 */
void do_distortion1(int32 *buf, int32 count, EffectList *ef)
{
	InfoOverdrive1 *info = (InfoOverdrive1 *)ef->info;
	filter_moog *svf = &(info->svf);
	filter_biquad *lpf1 = &(info->lpf1);
	void (*do_amp_sim)(int32 *, int32) = info->amp_sim;
	int32 i, input, high, leveli = info->leveli, di = info->di,
		pan = info->pan, asdi = TIM_FSCALE(1.0f, 24);

	if(count == MAGIC_INIT_EFFECT_INFO) {
		/* decompositor */
		svf->freq = 500;
		svf->res_dB = 0;
		calc_filter_moog(svf);
		init_filter_moog(svf);
		/* amp simulator */
		info->amp_sim = do_dummy_clipping;
		if (info->amp_sw == 1) {
			if (info->amp_type <= 3) {info->amp_sim = do_soft_clipping2;}
		}
		/* waveshaper */
		info->di = TIM_FSCALE(calc_gs_drive(info->drive), 24);
		info->leveli = TIM_FSCALE(info->level * OD_LEVEL_GS, 24);
		/* anti-aliasing */
		lpf1->freq = 8000.0f;
		lpf1->q = 1.0f;
		calc_filter_biquad_low(lpf1);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	for(i = 0; i < count; i++) {
		input = (buf[i] + buf[i + 1]) >> 1;
		/* amp simulation */
		do_amp_sim(&input, asdi);
		/* decomposition */
		do_filter_moog(&input, &high, svf->f, svf->p, svf->q,
			&svf->b0, &svf->b1, &svf->b2, &svf->b3, &svf->b4);
		/* waveshaping */
		do_hard_clipping(&high, di);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1l, &lpf1->x2l, &lpf1->y1l, &lpf1->y2l);
		/* mixing */
		input = imuldiv24(high + input, leveli);
		buf[i] = do_left_panning(input, pan);
		buf[i + 1] = do_right_panning(input, pan);
		++i;
	}
}

/*! GS 0x1103: OD1 / OD2 */
void do_dual_od(int32 *buf, int32 count, EffectList *ef)
{
	InfoOD1OD2 *info = (InfoOD1OD2 *)ef->info;
	filter_moog *svfl = &(info->svfl), *svfr = &(info->svfr);
	filter_biquad *lpf1 = &(info->lpf1);
	void (*do_amp_siml)(int32 *, int32) = info->amp_siml,
		(*do_amp_simr)(int32 *, int32) = info->amp_simr,
		(*do_odl)(int32 *, int32) = info->odl,
		(*do_odr)(int32 *, int32) = info->odr;
	int32 i, inputl, inputr, high, levelli = info->levelli, levelri = info->levelri,
		dli = info->dli, dri = info->dri, panl = info->panl, panr = info->panr, asdi = TIM_FSCALE(1.0f, 24);

	if(count == MAGIC_INIT_EFFECT_INFO) {
		/* left */
		/* decompositor */
		svfl->freq = 500;
		svfl->res_dB = 0;
		calc_filter_moog(svfl);
		init_filter_moog(svfl);
		/* amp simulator */
		info->amp_siml = do_dummy_clipping;
		if (info->amp_swl == 1) {
			if (info->amp_typel <= 3) {info->amp_siml = do_soft_clipping2;}
		}
		/* waveshaper */
		if(info->typel == 0) {info->odl = do_soft_clipping1;}
		else {info->odl = do_hard_clipping;}
		info->dli = TIM_FSCALE(calc_gs_drive(info->drivel), 24);
		info->levelli = TIM_FSCALE(info->levell * OD_LEVEL_GS, 24);
		/* right */
		/* decompositor */
		svfr->freq = 500;
		svfr->res_dB = 0;
		calc_filter_moog(svfr);
		init_filter_moog(svfr);
		/* amp simulator */
		info->amp_simr = do_dummy_clipping;
		if (info->amp_swr == 1) {
			if (info->amp_typer <= 3) {info->amp_simr = do_soft_clipping2;}
		}
		/* waveshaper */
		if(info->typer == 0) {info->odr = do_soft_clipping1;}
		else {info->odr = do_hard_clipping;}
		info->dri = TIM_FSCALE(calc_gs_drive(info->driver), 24);
		info->levelri = TIM_FSCALE(info->levelr * OD_LEVEL_GS, 24);
		/* anti-aliasing */
		lpf1->freq = 8000.0f;
		lpf1->q = 1.0f;
		calc_filter_biquad_low(lpf1);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	for(i = 0; i < count; i++) {
		/* left */
		inputl = buf[i];
		/* amp simulation */
		do_amp_siml(&inputl, asdi);
		/* decomposition */
		do_filter_moog(&inputl, &high, svfl->f, svfl->p, svfl->q,
			&svfl->b0, &svfl->b1, &svfl->b2, &svfl->b3, &svfl->b4);
		/* waveshaping */
		do_odl(&high, dli);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1l, &lpf1->x2l, &lpf1->y1l, &lpf1->y2l);
		inputl = imuldiv24(high + inputl, levelli);

		/* right */
		inputr = buf[++i];
		/* amp simulation */
		do_amp_siml(&inputr, asdi);
		/* decomposition */
		do_filter_moog(&inputr, &high, svfr->f, svfr->p, svfr->q,
			&svfr->b0, &svfr->b1, &svfr->b2, &svfr->b3, &svfr->b4);
		/* waveshaping */
		do_odr(&high, dri);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1r, &lpf1->x2r, &lpf1->y1r, &lpf1->y2r);
		inputr = imuldiv24(high + inputr, levelri);

		/* panning */
		buf[i - 1] = do_left_panning(inputl, panl) + do_left_panning(inputr, panr);
		buf[i] = do_right_panning(inputl, panl) + do_right_panning(inputr, panr);
	}
}

#define HEXA_CHORUS_WET_LEVEL 0.2
#define HEXA_CHORUS_DEPTH_DEV (1.0 / (20.0 + 1.0))
#define HEXA_CHORUS_DELAY_DEV (1.0 / (20.0 * 3.0))

/*! GS 0x0140: HEXA-CHORUS */
void do_hexa_chorus(int32 *buf, int32 count, EffectList *ef)
{
	InfoHexaChorus *info = (InfoHexaChorus *)ef->info;
	lfo *lfo = &(info->lfo0);
	delay *buf0 = &(info->buf0);
	int32 *ebuf = buf0->buf, size = buf0->size, index = buf0->index;
	int32 spt0 = info->spt0, spt1 = info->spt1, spt2 = info->spt2,
		spt3 = info->spt3, spt4 = info->spt4, spt5 = info->spt5,
		hist0 = info->hist0, hist1 = info->hist1, hist2 = info->hist2,
		hist3 = info->hist3, hist4 = info->hist4, hist5 = info->hist5;
	int32 dryi = info->dryi, weti = info->weti;
	int32 pan0 = info->pan0, pan1 = info->pan1, pan2 = info->pan2,
		pan3 = info->pan3, pan4 = info->pan4, pan5 = info->pan5;
	int32 depth0 = info->depth0, depth1 = info->depth1, depth2 = info->depth2,
		depth3 = info->depth3, depth4 = info->depth4, depth5 = info->depth5,
		pdelay0 = info->pdelay0, pdelay1 = info->pdelay1, pdelay2 = info->pdelay2,
		pdelay3 = info->pdelay3, pdelay4 = info->pdelay4, pdelay5 = info->pdelay5;
	int32 i, lfo_val,
		v0, v1, v2, v3, v4, v5, f0, f1, f2, f3, f4, f5;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		set_delay(buf0, (int32)(9600.0f * play_mode->rate / 44100.0f));
		init_lfo(lfo, lfo->freq, LFO_TRIANGULAR, 0);
		info->dryi = TIM_FSCALE(info->level * info->dry, 24);
		info->weti = TIM_FSCALE(info->level * info->wet * HEXA_CHORUS_WET_LEVEL, 24);
		v0 = info->depth * ((double)info->depth_dev * HEXA_CHORUS_DEPTH_DEV);
		info->depth0 = info->depth - v0;
		info->depth1 = info->depth;
		info->depth2 = info->depth + v0;
		info->depth3 = info->depth + v0;
		info->depth4 = info->depth;
		info->depth5 = info->depth - v0;
		v0 = info->pdelay * ((double)info->pdelay_dev * HEXA_CHORUS_DELAY_DEV);
		info->pdelay0 = info->pdelay + v0;
		info->pdelay1 = info->pdelay + v0 * 2;
		info->pdelay2 = info->pdelay + v0 * 3;
		info->pdelay3 = info->pdelay + v0 * 3;
		info->pdelay4 = info->pdelay + v0 * 2;
		info->pdelay5 = info->pdelay + v0;
		/* in this part, validation check may be necessary. */
		info->pan0 = 64 - info->pan_dev * 3;
		info->pan1 = 64 - info->pan_dev * 2;
		info->pan2 = 64 - info->pan_dev;
		info->pan3 = 64 + info->pan_dev;
		info->pan4 = 64 + info->pan_dev * 2;
		info->pan5 = 64 + info->pan_dev * 3;
		info->hist0 = info->hist1 = info->hist2
			= info->hist3 = info->hist4 = info->hist5 = 0;
		info->spt0 = info->spt1 = info->spt2
			= info->spt3 = info->spt4 = info->spt5 = 0;
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(buf0);
		return;
	}

	/* LFO */
	lfo_val = lfo->buf[imuldiv24(lfo->count, lfo->icycle)];
	f0 = imuldiv24(lfo_val, depth0);
	spt0 = index - pdelay0 - (f0 >> 8);	/* integral part of delay */
	if(spt0 < 0) {spt0 += size;}
	f1 = imuldiv24(lfo_val, depth1);
	spt1 = index - pdelay1 - (f1 >> 8);	/* integral part of delay */
	if(spt1 < 0) {spt1 += size;}
	f2 = imuldiv24(lfo_val, depth2);
	spt2 = index - pdelay2 - (f2 >> 8);	/* integral part of delay */
	if(spt2 < 0) {spt2 += size;}
	f3 = imuldiv24(lfo_val, depth3);
	spt3 = index - pdelay3 - (f3 >> 8);	/* integral part of delay */
	if(spt3 < 0) {spt3 += size;}
	f4 = imuldiv24(lfo_val, depth4);
	spt4 = index - pdelay4 - (f4 >> 8);	/* integral part of delay */
	if(spt4 < 0) {spt4 += size;}
	f5 = imuldiv24(lfo_val, depth5);
	spt5 = index - pdelay5 - (f5 >> 8);	/* integral part of delay */
	if(spt5 < 0) {spt5 += size;}

	for(i = 0; i < count; i++) {
		v0 = ebuf[spt0], v1 = ebuf[spt1], v2 = ebuf[spt2],
		v3 = ebuf[spt3], v4 = ebuf[spt4], v5 = ebuf[spt5];

		/* LFO */
		if(++index == size) {index = 0;}
		lfo_val = do_lfo(lfo);
		f0 = imuldiv24(lfo_val, depth0);
		spt0 = index - pdelay0 - (f0 >> 8);	/* integral part of delay */
		f0 = 0xFF - (f0 & 0xFF);	/* (1 - frac) * 256 */
		if(spt0 < 0) {spt0 += size;}
		f1 = imuldiv24(lfo_val, depth1);
		spt1 = index - pdelay1 - (f1 >> 8);	/* integral part of delay */
		f1 = 0xFF - (f1 & 0xFF);	/* (1 - frac) * 256 */
		if(spt1 < 0) {spt1 += size;}
		f2 = imuldiv24(lfo_val, depth2);
		spt2 = index - pdelay2 - (f2 >> 8);	/* integral part of delay */
		f2 = 0xFF - (f2 & 0xFF);	/* (1 - frac) * 256 */
		if(spt2 < 0) {spt2 += size;}
		f3 = imuldiv24(lfo_val, depth3);
		spt3 = index - pdelay3 - (f3 >> 8);	/* integral part of delay */
		f3 = 0xFF - (f3 & 0xFF);	/* (1 - frac) * 256 */
		if(spt3 < 0) {spt3 += size;}
		f4 = imuldiv24(lfo_val, depth4);
		spt4 = index - pdelay4 - (f4 >> 8);	/* integral part of delay */
		f4 = 0xFF - (f4 & 0xFF);	/* (1 - frac) * 256 */
		if(spt4 < 0) {spt4 += size;}
		f5 = imuldiv24(lfo_val, depth5);
		spt5 = index - pdelay5 - (f5 >> 8);	/* integral part of delay */
		f5 = 0xFF - (f5 & 0xFF);	/* (1 - frac) * 256 */
		if(spt5 < 0) {spt5 += size;}

		/* chorus effect */
		/* all-pass interpolation */
		hist0 = v0 + imuldiv8(ebuf[spt0] - hist0, f0);
		hist1 = v1 + imuldiv8(ebuf[spt1] - hist1, f1);
		hist2 = v2 + imuldiv8(ebuf[spt2] - hist2, f2);
		hist3 = v3 + imuldiv8(ebuf[spt3] - hist3, f3);
		hist4 = v4 + imuldiv8(ebuf[spt4] - hist4, f4);
		hist5 = v5 + imuldiv8(ebuf[spt5] - hist5, f5);
		ebuf[index] = imuldiv24(buf[i] + buf[i + 1], weti);

		/* mixing */
		buf[i] = do_left_panning(hist0, pan0) + do_left_panning(hist1, pan1)
			+ do_left_panning(hist2, pan2) + do_left_panning(hist3, pan3)
			+ do_left_panning(hist4, pan4) + do_left_panning(hist5, pan5)
			+ imuldiv24(buf[i], dryi);
		buf[i + 1] = do_right_panning(hist0, pan0) + do_right_panning(hist1, pan1)
			+ do_right_panning(hist2, pan2) + do_right_panning(hist3, pan3)
			+ do_right_panning(hist4, pan4) + do_right_panning(hist5, pan5)
			+ imuldiv24(buf[i + 1], dryi);

		++i;
	}
	buf0->size = size, buf0->index = index;
	info->spt0 = spt0, info->spt1 = spt1, info->spt2 = spt2,
	info->spt3 = spt3, info->spt4 = spt4, info->spt5 = spt5,
	info->hist0 = hist0, info->hist1 = hist1, info->hist2 = hist2,
	info->hist3 = hist3, info->hist4 = hist4, info->hist5 = hist5;
}

static void free_effect_xg(struct effect_xg_t *st)
{
	free_effect_list(st->ef);
	st->ef = NULL;
}

void free_effect_buffers(void)
{
	int i;
	/* free GM/GS/GM2 effects */
	do_ch_standard_reverb(NULL, MAGIC_FREE_EFFECT_INFO, &(reverb_status_gs.info_standard_reverb));
	do_ch_freeverb(NULL, MAGIC_FREE_EFFECT_INFO, &(reverb_status_gs.info_freeverb));
	do_ch_plate_reverb(NULL, MAGIC_FREE_EFFECT_INFO, &(reverb_status_gs.info_plate_reverb));
	do_ch_reverb_normal_delay(NULL, MAGIC_FREE_EFFECT_INFO, &(reverb_status_gs.info_reverb_delay));
	do_ch_stereo_chorus(NULL, MAGIC_FREE_EFFECT_INFO, &(chorus_status_gs.info_stereo_chorus));
	do_ch_3tap_delay(NULL, MAGIC_FREE_EFFECT_INFO, &(delay_status_gs.info_delay));
	free_effect_list(insertion_effect_gs.ef);
	insertion_effect_gs.ef = NULL;
	/* free XG effects */
	free_effect_xg(&reverb_status_xg);
	free_effect_xg(&chorus_status_xg);
	for (i = 0; i < XG_VARIATION_EFFECT_NUM; i++) {
		free_effect_xg(&variation_effect_xg[i]);
	}
	for (i = 0; i < XG_INSERTION_EFFECT_NUM; i++) {
		free_effect_xg(&insertion_effect_xg[i]);
	}
}

static inline int clip_int(int val, int min, int max)
{
	return ((val > max) ? max : (val < min) ? min : val);
}

static void conv_gs_eq2(struct insertion_effect_gs_t *ieffect, EffectList *ef)
{
	InfoEQ2 *eq = (InfoEQ2 *)ef->info; 

	eq->high_freq = 4000;
	eq->high_gain = clip_int(ieffect->parameter[16] - 0x40, -12, 12);
	eq->low_freq = 400;
	eq->low_gain = clip_int(ieffect->parameter[17] - 0x40, -12, 12);
}

static void conv_gs_overdrive1(struct insertion_effect_gs_t *ieffect, EffectList *ef)
{
	InfoOverdrive1 *od = (InfoOverdrive1 *)ef->info;
	
	od->drive = ieffect->parameter[0];
	od->amp_type = ieffect->parameter[1];
	od->amp_sw = ieffect->parameter[2];
	od->pan = ieffect->parameter[18];
	od->level = (double)ieffect->parameter[19] / 127.0;
}

static void conv_gs_dual_od(struct insertion_effect_gs_t *ieffect, EffectList *ef)
{
	InfoOD1OD2 *od = (InfoOD1OD2 *)ef->info;

	od->typel = ieffect->parameter[0];
	od->drivel = ieffect->parameter[1];
	od->amp_typel = ieffect->parameter[2];
	od->amp_swl = ieffect->parameter[3];
	od->typer = ieffect->parameter[5];
	od->driver = ieffect->parameter[6];
	od->amp_typer = ieffect->parameter[7];
	od->amp_swr = ieffect->parameter[8];
	od->panl = ieffect->parameter[15];
	od->levell = (double)ieffect->parameter[16] / 127.0;
	od->panr = ieffect->parameter[17];
	od->levelr = (double)ieffect->parameter[18] / 127.0;
	od->level = (double)ieffect->parameter[19] / 127.0;
}

static double calc_dry_gs(int val)
{
	return ((double)(127 - val) / 127.0f);
}

static double calc_wet_gs(int val)
{
	return ((double)val / 127.0f);
}

static void conv_gs_hexa_chorus(struct insertion_effect_gs_t *ieffect, EffectList *ef)
{
	InfoHexaChorus *info = (InfoHexaChorus *)ef->info;
	
	info->level = (double)ieffect->parameter[19] / 127.0f;
	info->pdelay = pre_delay_time_table[ieffect->parameter[0]] * (double)play_mode->rate / 1000.0f;
	info->depth = (double)(ieffect->parameter[2] + 1) / 3.2f  * (double)play_mode->rate / 1000.0f;
	info->pdelay -= info->depth / 2;
	if(info->pdelay <= 1) {info->pdelay = 1;}
	info->lfo0.freq = rate1_table[ieffect->parameter[1]];
	info->pdelay_dev = ieffect->parameter[3];
	info->depth_dev = ieffect->parameter[4] - 64;
	info->pan_dev = ieffect->parameter[5];
	info->dry = calc_dry_gs(ieffect->parameter[15]);
	info->wet = calc_wet_gs(ieffect->parameter[15]);
}

static double calc_dry_xg(int val, struct effect_xg_t *st)
{
	if (st->connection) {return 0.0f;}
	else {return ((double)(127 - val) / 127.0f);}
}

static double calc_wet_xg(int val, struct effect_xg_t *st)
{
	switch(st->connection) {
	case XG_CONN_SYSTEM:
		return ((double)st->ret / 127.0f);
	case XG_CONN_SYSTEM_CHORUS:
		return ((double)st->ret / 127.0f);
	case XG_CONN_SYSTEM_REVERB:
		return ((double)st->ret / 127.0f);
	default:
		return ((double)val / 127.0f); 
	}
}

/*! 3-Band EQ */
static void do_eq3(int32 *buf, int32 count, EffectList *ef)
{
	InfoEQ3 *eq = (InfoEQ3 *)ef->info;
	if (count == MAGIC_INIT_EFFECT_INFO) {
		eq->lsf.q = 0;
		eq->lsf.freq = eq->low_freq;
		eq->lsf.gain = eq->low_gain;
		calc_filter_shelving_low(&(eq->lsf));
		eq->hsf.q = 0;
		eq->hsf.freq = eq->high_freq;
		eq->hsf.gain = eq->high_gain;
		calc_filter_shelving_high(&(eq->hsf));
		eq->peak.q = 1.0f / eq->mid_width;
		eq->peak.freq = eq->mid_freq;
		eq->peak.gain = eq->mid_gain;
		calc_filter_peaking(&(eq->peak));
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	if (eq->low_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->lsf));
	}
	if (eq->high_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->hsf));
	}
	if (eq->mid_gain != 0) {
		do_peaking_filter_stereo(buf, count, &(eq->peak));
	}
}

/*! Stereo EQ */
static void do_stereo_eq(int32 *buf, int32 count, EffectList *ef)
{
	InfoStereoEQ *eq = (InfoStereoEQ *)ef->info;
	int32 i, leveli = eq->leveli;
	if (count == MAGIC_INIT_EFFECT_INFO) {
		eq->lsf.q = 0;
		eq->lsf.freq = eq->low_freq;
		eq->lsf.gain = eq->low_gain;
		calc_filter_shelving_low(&(eq->lsf));
		eq->hsf.q = 0;
		eq->hsf.freq = eq->high_freq;
		eq->hsf.gain = eq->high_gain;
		calc_filter_shelving_high(&(eq->hsf));
		eq->m1.q = eq->m1_q;
		eq->m1.freq = eq->m1_freq;
		eq->m1.gain = eq->m1_gain;
		calc_filter_peaking(&(eq->m1));
		eq->m2.q = eq->m2_q;
		eq->m2.freq = eq->m2_freq;
		eq->m2.gain = eq->m2_gain;
		calc_filter_peaking(&(eq->m2));
		eq->leveli = TIM_FSCALE(eq->level, 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	if (eq->level != 1.0f) {
		for (i = 0; i < count; i++) {
			buf[i] = imuldiv24(buf[i], leveli);
		}
	}
	if (eq->low_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->lsf));
	}
	if (eq->high_gain != 0) {
		do_shelving_filter_stereo(buf, count, &(eq->hsf));
	}
	if (eq->m1_gain != 0) {
		do_peaking_filter_stereo(buf, count, &(eq->m1));
	}
	if (eq->m2_gain != 0) {
		do_peaking_filter_stereo(buf, count, &(eq->m2));
	}
}

static void conv_xg_eq2(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ2 *info = (InfoEQ2 *)ef->info;

	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[0], 4, 40)];
	info->low_gain = clip_int(st->param_lsb[1] - 64, -12, 12);
	info->high_freq = eq_freq_table_xg[clip_int(st->param_lsb[2], 28, 58)];
	info->high_gain = clip_int(st->param_lsb[3] - 64, -12, 12);
}

static void conv_xg_eq3(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ3 *info = (InfoEQ3 *)ef->info;

	info->low_gain = clip_int(st->param_lsb[0] - 64, -12, 12);
	info->mid_freq = eq_freq_table_xg[clip_int(st->param_lsb[1], 14, 54)];
	info->mid_gain = clip_int(st->param_lsb[2] - 64, -12, 12);
	info->mid_width = (double)clip_int(st->param_lsb[3], 10, 120) / 10.0f;
	info->high_gain = clip_int(st->param_lsb[4] - 64, -12, 12);
	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[5], 4, 40)];
	info->high_freq = eq_freq_table_xg[clip_int(st->param_lsb[6], 28, 58)];
}

static float eq_q_table_gs[] =
{
	0.5, 1.0, 2.0, 4.0, 9.0,
};

static void conv_gs_stereo_eq(struct insertion_effect_gs_t *st, EffectList *ef)
{
	InfoStereoEQ *info = (InfoStereoEQ *)ef->info;

	info->low_freq = (st->parameter[0] == 0) ? 200 : 400;
	info->low_gain = clip_int(st->parameter[1] - 64, -12, 12);
	info->high_freq = (st->parameter[2] == 0) ? 4000 : 8000;
	info->high_gain = clip_int(st->parameter[3] - 64, -12, 12);
	info->m1_freq = eq_freq_table_gs[st->parameter[4]];
	info->m1_q = eq_q_table_gs[clip_int(st->parameter[5], 0, 4)];
	info->m1_gain = clip_int(st->parameter[6] - 64, -12, 12);
	info->m2_freq = eq_freq_table_gs[st->parameter[7]];
	info->m2_q = eq_q_table_gs[clip_int(st->parameter[8], 0, 4)];
	info->m2_gain = clip_int(st->parameter[9] - 64, -12, 12);
	info->level = (double)st->parameter[19] / 127.0f;
}

static void conv_xg_chorus_eq3(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ3 *info = (InfoEQ3 *)ef->info;

	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[5], 4, 40)];
	info->low_gain = clip_int(st->param_lsb[6] - 64, -12, 12);
	info->high_freq = eq_freq_table_xg[clip_int(st->param_lsb[7], 28, 58)];
	info->high_gain = clip_int(st->param_lsb[8] - 64, -12, 12);
	info->mid_freq = eq_freq_table_xg[clip_int(st->param_lsb[10], 14, 54)];
	info->mid_gain = clip_int(st->param_lsb[11] - 64, -12, 12);
	info->mid_width = (double)clip_int(st->param_lsb[12], 10, 120) / 10.0f;
}

static void conv_xg_chorus(struct effect_xg_t *st, EffectList *ef)
{
	InfoChorus *info = (InfoChorus *)ef->info;

	info->rate = lfo_freq_table_xg[st->param_lsb[0]];
	info->depth_ms = (double)(st->param_lsb[1] + 1) / 3.2f / 2.0f;
	info->feedback = (double)(st->param_lsb[2] - 64) * (0.763f * 2.0f / 100.0f);
	info->pdelay_ms = mod_delay_offset_table_xg[st->param_lsb[3]];
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
	info->phase_diff = 90.0f;
}

static void conv_xg_flanger(struct effect_xg_t *st, EffectList *ef)
{
	InfoChorus *info = (InfoChorus *)ef->info;

	info->rate = lfo_freq_table_xg[st->param_lsb[0]];
	info->depth_ms = (double)(st->param_lsb[1] + 1) / 3.2f / 2.0f;
	info->feedback = (double)(st->param_lsb[2] - 64) * (0.763f * 2.0f / 100.0f);
	info->pdelay_ms = mod_delay_offset_table_xg[st->param_lsb[2]];
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
	info->phase_diff = (double)(clip_int(st->param_lsb[13], 4, 124) - 64) * 3.0f;
}

static void conv_xg_symphonic(struct effect_xg_t *st, EffectList *ef)
{
	InfoChorus *info = (InfoChorus *)ef->info;

	info->rate = lfo_freq_table_xg[st->param_lsb[0]];
	info->depth_ms = (double)(st->param_lsb[1] + 1) / 3.2f / 2.0f;
	info->feedback = 0.0f;
	info->pdelay_ms = mod_delay_offset_table_xg[st->param_lsb[3]];
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
	info->phase_diff = 90.0f;
}

static void do_chorus(int32 *buf, int32 count, EffectList *ef)
{
	InfoChorus *info = (InfoChorus *)ef->info;
	int32 i, output, f0, f1, v0, v1;
	int32 *bufL = info->delayL.buf, *bufR = info->delayR.buf,
		*lfobufL = info->lfoL.buf, *lfobufR = info->lfoR.buf,
		icycle = info->lfoL.icycle, cycle = info->lfoL.cycle,
		dryi = info->dryi, weti = info->weti, feedbacki = info->feedbacki,
		depth = info->depth, pdelay = info->pdelay, rpt0 = info->rpt0;
	int32 wpt0 = info->wpt0, spt0 = info->spt0, spt1 = info->spt1,
		hist0 = info->hist0, hist1 = info->hist1, lfocnt = info->lfoL.count;

	if (count == MAGIC_INIT_EFFECT_INFO) {
		init_lfo(&(info->lfoL), info->rate, LFO_TRIANGULAR, 0);
		init_lfo(&(info->lfoR), info->rate, LFO_TRIANGULAR, info->phase_diff);
		info->pdelay = info->pdelay_ms * (double)play_mode->rate / 1000.0f;
		info->depth = info->depth_ms * (double)play_mode->rate / 1000.0f;
		info->pdelay -= info->depth / 2;	/* NOMINAL_DELAY to delay */
		if (info->pdelay < 1) {info->pdelay = 1;}
		info->rpt0 = info->pdelay + info->depth + 2;	/* allowance */
		set_delay(&(info->delayL), info->rpt0);
		set_delay(&(info->delayR), info->rpt0);
		info->feedbacki = TIM_FSCALE(info->feedback, 24);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		info->wpt0 = info->spt0 = info->spt1 = info->hist0 = info->hist1 = 0;
		return;
	} else if (count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	/* LFO */
	f0 = imuldiv24(lfobufL[imuldiv24(lfocnt, icycle)], depth);
	spt0 = wpt0 - pdelay - (f0 >> 8);	/* integral part of delay */
	f0 = 0xFF - (f0 & 0xFF);	/* (1 - frac) * 256 */
	if (spt0 < 0) {spt0 += rpt0;}
	f1 = imuldiv24(lfobufR[imuldiv24(lfocnt, icycle)], depth);
	spt1 = wpt0 - pdelay - (f1 >> 8);	/* integral part of delay */
	f1 = 0xFF - (f1 & 0xFF);	/* (1 - frac) * 256 */
	if (spt1 < 0) {spt1 += rpt0;}
	
	for (i = 0; i < count; i++) {
		v0 = bufL[spt0];
		v1 = bufR[spt1];

		/* LFO */
		if(++wpt0 == rpt0) {wpt0 = 0;}
		f0 = imuldiv24(lfobufL[imuldiv24(lfocnt, icycle)], depth);
		spt0 = wpt0 - pdelay - (f0 >> 8);	/* integral part of delay */
		f0 = 0xFF - (f0 & 0xFF);	/* (1 - frac) * 256 */
		if(spt0 < 0) {spt0 += rpt0;}
		f1 = imuldiv24(lfobufR[imuldiv24(lfocnt, icycle)], depth);
		spt1 = wpt0 - pdelay - (f1 >> 8);	/* integral part of delay */
		f1 = 0xFF - (f1 & 0xFF);	/* (1 - frac) * 256 */
		if(spt1 < 0) {spt1 += rpt0;}
		if(++lfocnt == cycle) {lfocnt = 0;}

		/* left */
		/* delay with all-pass interpolation */
		output = hist0 = v0 + imuldiv8(bufL[spt0] - hist0, f0);
		bufL[wpt0] = buf[i] + imuldiv24(output, feedbacki);
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(output, weti);

		/* right */
		/* delay with all-pass interpolation */
		output = hist1 = v1 + imuldiv8(bufR[spt1] - hist1, f1);
		bufR[wpt0] = buf[++i] + imuldiv24(output, feedbacki);
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(output, weti);
	}
	info->wpt0 = wpt0, info->spt0 = spt0, info->spt1 = spt1,
		info->hist0 = hist0, info->hist1 = hist1;
	info->lfoL.count = info->lfoR.count = lfocnt;
}

static void conv_xg_od_eq3(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ3 *info = (InfoEQ3 *)ef->info;

	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[1], 4, 40)];
	info->low_gain = clip_int(st->param_lsb[2] - 64, -12, 12);
	info->mid_freq = eq_freq_table_xg[clip_int(st->param_lsb[6], 14, 54)];
	info->mid_gain = clip_int(st->param_lsb[7] - 64, -12, 12);
	info->mid_width = (double)clip_int(st->param_lsb[8], 10, 120) / 10.0f;
	info->high_freq = 0;
	info->high_gain = 0;
}

static void conv_xg_overdrive(struct effect_xg_t *st, EffectList *ef)
{
	InfoStereoOD *info = (InfoStereoOD *)ef->info;

	info->od = do_soft_clipping1;
	info->drive = (double)st->param_lsb[0] / 127.0f;
	info->cutoff = eq_freq_table_xg[clip_int(st->param_lsb[3], 34, 60)];
	info->level = (double)st->param_lsb[4] / 127.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void conv_xg_distortion(struct effect_xg_t *st, EffectList *ef)
{
	InfoStereoOD *info = (InfoStereoOD *)ef->info;

	info->od = do_hard_clipping;
	info->drive = (double)st->param_lsb[0] / 127.0f;
	info->cutoff = eq_freq_table_xg[clip_int(st->param_lsb[3], 34, 60)];
	info->level = (double)st->param_lsb[4] / 127.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void conv_xg_amp_simulator(struct effect_xg_t *st, EffectList *ef)
{
	InfoStereoOD *info = (InfoStereoOD *)ef->info;

	info->od = do_soft_clipping2;
	info->drive = (double)st->param_lsb[0] / 127.0f;
	info->cutoff = eq_freq_table_xg[clip_int(st->param_lsb[2], 34, 60)];
	info->level = (double)st->param_lsb[3] / 127.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void do_stereo_od(int32 *buf, int32 count, EffectList *ef)
{
	InfoStereoOD *info = (InfoStereoOD *)ef->info;
	filter_moog *svfl = &(info->svfl), *svfr = &(info->svfr);
	filter_biquad *lpf1 = &(info->lpf1);
	void (*do_od)(int32 *, int32) = info->od;
	int32 i, inputl, inputr, high, weti = info->weti, dryi = info->dryi, di = info->di;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		/* decompositor */
		svfl->freq = 500;
		svfl->res_dB = 0;
		calc_filter_moog(svfl);
		init_filter_moog(svfl);
		svfr->freq = 500;
		svfr->res_dB = 0;
		calc_filter_moog(svfr);
		init_filter_moog(svfr);
		/* anti-aliasing */
		lpf1->freq = info->cutoff;
		lpf1->q = 1.0f;
		calc_filter_biquad_low(lpf1);
		info->weti = TIM_FSCALE(info->wet * info->level, 24);
		info->dryi = TIM_FSCALE(info->dry * info->level, 24);
		info->di = TIM_FSCALE(calc_gs_drive(info->drive), 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	for(i = 0; i < count; i++) {
		/* left */
		inputl = buf[i];
		/* decomposition */
		do_filter_moog(&inputl, &high, svfl->f, svfl->p, svfl->q,
			&svfl->b0, &svfl->b1, &svfl->b2, &svfl->b3, &svfl->b4);
		/* waveshaping */
		do_od(&high, di);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1l, &lpf1->x2l, &lpf1->y1l, &lpf1->y2l);
		buf[i] = imuldiv24(high + inputl, weti) + imuldiv24(buf[i], dryi);

		/* right */
		inputr = buf[++i];
		/* decomposition */
		do_filter_moog(&inputr, &high, svfr->f, svfr->p, svfr->q,
			&svfr->b0, &svfr->b1, &svfr->b2, &svfr->b3, &svfr->b4);
		/* waveshaping */
		do_od(&high, di);
		/* anti-aliasing */
		do_filter_biquad(&high, lpf1->a1, lpf1->a2, lpf1->b1, lpf1->b02, &lpf1->x1r, &lpf1->x2r, &lpf1->y1r, &lpf1->y2r);
		buf[i] = imuldiv24(high + inputr, weti) + imuldiv24(buf[i], dryi);
	}
}

static void do_delay_lcr(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x;
	InfoDelayLCR *info = (InfoDelayLCR *)ef->info;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	filter_lowpass1 *lpf = &(info->lpf);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 buf_index = delayL->index, buf_size = delayL->size;
	int32 index0 = info->index[0], index1 = info->index[1], index2 = info->index[2],
		x1l = lpf->x1l, x1r = lpf->x1r;
	int32 cleveli = info->cleveli, feedbacki = info->feedbacki, 
		dryi = info->dryi, weti = info->weti, ai = lpf->ai, iai = lpf->iai;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		info->size[0] = info->ldelay * play_mode->rate / 1000.0f;
		info->size[1] = info->cdelay * play_mode->rate / 1000.0f;
		info->size[2] = info->rdelay * play_mode->rate / 1000.0f;
		x = info->fdelay * play_mode->rate / 1000.0f;
		for (i = 0; i < 3; i++) {
			if (info->size[i] > x) {info->size[i] = x;}
		}
		x += 1;	/* allowance */
		set_delay(&(info->delayL), x);
		set_delay(&(info->delayR), x);
		for (i = 0; i < 3; i++) {	/* set start-point */
			info->index[i] = x - info->size[i];
		}
		info->feedbacki = TIM_FSCALE(info->feedback, 24);
		info->cleveli = TIM_FSCALE(info->clevel, 24);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		lpf->a = (1.0f - info->high_damp) * 44100.0f / play_mode->rate;
		init_filter_lowpass1(lpf);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = imuldiv24(bufL[buf_index], feedbacki);
		do_filter_lowpass1(&x, &x1l, ai, iai);
		bufL[buf_index] = buf[i] + x;
		x = bufL[index0] + imuldiv24(bufL[index1], cleveli);
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(x, weti);

		x = imuldiv24(bufR[buf_index], feedbacki);
		do_filter_lowpass1(&x, &x1r, ai, iai);
		bufR[buf_index] = buf[++i] + x;
		x = bufR[index2] + imuldiv24(bufR[index1], cleveli);
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(x, weti);

		if (++index0 == buf_size) {index0 = 0;}
		if (++index1 == buf_size) {index1 = 0;}
		if (++index2 == buf_size) {index2 = 0;}
		if (++buf_index == buf_size) {buf_index = 0;}
	}
	info->index[0] = index0, info->index[1] = index1, info->index[2] = index2;
	lpf->x1l = x1l, lpf->x1r = x1r;
	delayL->index = delayR->index = buf_index;
}

static void conv_xg_delay_eq2(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ2 *info = (InfoEQ2 *)ef->info;

	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[12], 4, 40)];
	info->low_gain = clip_int(st->param_lsb[13] - 64, -12, 12);
	info->high_freq = eq_freq_table_xg[clip_int(st->param_lsb[14], 28, 58)];
	info->high_gain = clip_int(st->param_lsb[15] - 64, -12, 12);
}

static void conv_xg_delay_lcr(struct effect_xg_t *st, EffectList *ef)
{
	InfoDelayLCR *info = (InfoDelayLCR *)ef->info;

	info->ldelay = (double)clip_int(st->param_msb[0] * 128 + st->param_lsb[0], 1, 14860) / 10.0f;
	info->rdelay = (double)clip_int(st->param_msb[1] * 128 + st->param_lsb[1], 1, 14860) / 10.0f;
	info->cdelay = (double)clip_int(st->param_msb[2] * 128 + st->param_lsb[2], 1, 14860) / 10.0f;
	info->fdelay = (double)clip_int(st->param_msb[3] * 128 + st->param_lsb[3], 1, 14860) / 10.0f;
	info->feedback = (double)(st->param_lsb[4] - 64) * (0.763f * 2.0f / 100.0f);
	info->clevel = (double)st->param_lsb[5] / 127.0f;
	info->high_damp = (double)clip_int(st->param_lsb[6], 1, 10) / 10.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void conv_xg_delay_lr(struct effect_xg_t *st, EffectList *ef)
{
	InfoDelayLR *info = (InfoDelayLR *)ef->info;

	info->ldelay = (double)clip_int(st->param_msb[0] * 128 + st->param_lsb[0], 1, 14860) / 10.0f;
	info->rdelay = (double)clip_int(st->param_msb[1] * 128 + st->param_lsb[1], 1, 14860) / 10.0f;
	info->fdelay1 = (double)clip_int(st->param_msb[2] * 128 + st->param_lsb[2], 1, 14860) / 10.0f;
	info->fdelay2 = (double)clip_int(st->param_msb[3] * 128 + st->param_lsb[3], 1, 14860) / 10.0f;
	info->feedback = (double)(st->param_lsb[4] - 64) * (0.763f * 2.0f / 100.0f);
	info->high_damp = (double)clip_int(st->param_lsb[5], 1, 10) / 10.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void do_delay_lr(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x;
	InfoDelayLR *info = (InfoDelayLR *)ef->info;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	filter_lowpass1 *lpf = &(info->lpf);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 indexl = delayL->index, sizel = delayL->size,
		indexr = delayR->index, sizer = delayR->size;
	int32 index0 = info->index[0], index1 = info->index[1],
		x1l = lpf->x1l, x1r = lpf->x1r;
	int32 feedbacki = info->feedbacki, 
		dryi = info->dryi, weti = info->weti, ai = lpf->ai, iai = lpf->iai;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		info->size[0] = info->ldelay * play_mode->rate / 1000.0f;
		x = info->fdelay1 * play_mode->rate / 1000.0f;
		if (info->size[0] > x) {info->size[0] = x;}
		x++;
		set_delay(&(info->delayL), x);
		info->index[0] = x - info->size[0];
		info->size[1] = info->rdelay * play_mode->rate / 1000.0f;
		x = info->fdelay2 * play_mode->rate / 1000.0f;
		if (info->size[1] > x) {info->size[1] = x;}
		x++;
		set_delay(&(info->delayR), x);
		info->index[1] = x - info->size[1];
		info->feedbacki = TIM_FSCALE(info->feedback, 24);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		lpf->a = (1.0f - info->high_damp) * 44100.0f / play_mode->rate;
		init_filter_lowpass1(lpf);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = imuldiv24(bufL[indexl], feedbacki);
		do_filter_lowpass1(&x, &x1l, ai, iai);
		bufL[indexl] = buf[i] + x;
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(bufL[index0], weti);

		x = imuldiv24(bufR[indexr], feedbacki);
		do_filter_lowpass1(&x, &x1r, ai, iai);
		bufR[indexr] = buf[++i] + x;
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(bufR[index1], weti);

		if (++index0 == sizel) {index0 = 0;}
		if (++index1 == sizer) {index1 = 0;}
		if (++indexl == sizel) {indexl = 0;}
		if (++indexr == sizer) {indexr = 0;}
	}
	info->index[0] = index0, info->index[1] = index1;
	lpf->x1l = x1l, lpf->x1r = x1r;
	delayL->index = indexl, delayR->index = indexr;
}

static void conv_xg_echo(struct effect_xg_t *st, EffectList *ef)
{
	InfoEcho *info = (InfoEcho *)ef->info;

	info->ldelay1 = (double)clip_int(st->param_msb[0] * 128 + st->param_lsb[0], 1, 7430) / 10.0f;
	info->lfeedback = (double)(st->param_lsb[1] - 64) * (0.763f * 2.0f / 100.0f);
	info->rdelay1 = (double)clip_int(st->param_msb[2] * 128 + st->param_lsb[2], 1, 7430) / 10.0f;
	info->rfeedback = (double)(st->param_lsb[3] - 64) * (0.763f * 2.0f / 100.0f);
	info->high_damp = (double)clip_int(st->param_lsb[4], 1, 10) / 10.0f;
	info->ldelay2 = (double)clip_int(st->param_msb[5] * 128 + st->param_lsb[5], 1, 7430) / 10.0f;
	info->rdelay2 = (double)clip_int(st->param_msb[6] * 128 + st->param_lsb[6], 1, 7430) / 10.0f;
	info->level = (double)st->param_lsb[7] / 127.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void do_echo(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x, y;
	InfoEcho *info = (InfoEcho *)ef->info;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	filter_lowpass1 *lpf = &(info->lpf);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 indexl = delayL->index, sizel = delayL->size,
		indexr = delayR->index, sizer = delayR->size;
	int32 index0 = info->index[0], index1 = info->index[1],
		x1l = lpf->x1l, x1r = lpf->x1r;
	int32 lfeedbacki = info->lfeedbacki, rfeedbacki = info->rfeedbacki, leveli = info->leveli,
		dryi = info->dryi, weti = info->weti, ai = lpf->ai, iai = lpf->iai;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		info->size[0] = info->ldelay2 * play_mode->rate / 1000.0f;
		x = info->ldelay1 * play_mode->rate / 1000.0f;
		if (info->size[0] > x) {info->size[0] = x;}
		x++;
		set_delay(&(info->delayL), x);
		info->index[0] = x - info->size[0];
		info->size[1] = info->rdelay2 * play_mode->rate / 1000.0f;
		x = info->rdelay1 * play_mode->rate / 1000.0f;
		if (info->size[1] > x) {info->size[1] = x;}
		x++;
		set_delay(&(info->delayR), x);
		info->index[1] = x - info->size[1];
		info->lfeedbacki = TIM_FSCALE(info->lfeedback, 24);
		info->rfeedbacki = TIM_FSCALE(info->rfeedback, 24);
		info->leveli = TIM_FSCALE(info->level, 24);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		lpf->a = (1.0f - info->high_damp) * 44100.0f / play_mode->rate;
		init_filter_lowpass1(lpf);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	for (i = 0; i < count; i++)
	{
		y = bufL[indexl] + imuldiv24(bufL[index0], leveli);
		x = imuldiv24(bufL[indexl], lfeedbacki);
		do_filter_lowpass1(&x, &x1l, ai, iai);
		bufL[indexl] = buf[i] + x;
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(y, weti);

		y = bufR[indexr] + imuldiv24(bufR[index1], leveli);
		x = imuldiv24(bufR[indexr], rfeedbacki);
		do_filter_lowpass1(&x, &x1r, ai, iai);
		bufR[indexr] = buf[++i] + x;
		buf[i] = imuldiv24(buf[i], dryi) + imuldiv24(y, weti);

		if (++index0 == sizel) {index0 = 0;}
		if (++index1 == sizer) {index1 = 0;}
		if (++indexl == sizel) {indexl = 0;}
		if (++indexr == sizer) {indexr = 0;}
	}
	info->index[0] = index0, info->index[1] = index1;
	lpf->x1l = x1l, lpf->x1r = x1r;
	delayL->index = indexl, delayR->index = indexr;
}

static void conv_xg_cross_delay(struct effect_xg_t *st, EffectList *ef)
{
	InfoCrossDelay *info = (InfoCrossDelay *)ef->info;

	info->lrdelay = (double)clip_int(st->param_msb[0] * 128 + st->param_lsb[0], 1, 7430) / 10.0f;
	info->rldelay = (double)clip_int(st->param_msb[1] * 128 + st->param_lsb[1], 1, 7430) / 10.0f;
	info->feedback = (double)(st->param_lsb[2] - 64) * (0.763f * 2.0f / 100.0f);
	info->input_select = st->param_lsb[3];
	info->high_damp = (double)clip_int(st->param_lsb[4], 1, 10) / 10.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void do_cross_delay(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, lfb, rfb, lout, rout;
	InfoCrossDelay *info = (InfoCrossDelay *)ef->info;
	delay *delayL = &(info->delayL), *delayR = &(info->delayR);
	filter_lowpass1 *lpf = &(info->lpf);
	int32 *bufL = delayL->buf, *bufR = delayR->buf;
	int32 indexl = delayL->index, sizel = delayL->size,
		indexr = delayR->index, sizer = delayR->size,
		x1l = lpf->x1l, x1r = lpf->x1r;
	int32 feedbacki = info->feedbacki, 
		dryi = info->dryi, weti = info->weti, ai = lpf->ai, iai = lpf->iai;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		set_delay(&(info->delayL), (int32)(info->lrdelay * play_mode->rate / 1000.0f));
		set_delay(&(info->delayR), (int32)(info->rldelay * play_mode->rate / 1000.0f));
		info->feedbacki = TIM_FSCALE(info->feedback, 24);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		lpf->a = (1.0f - info->high_damp) * 44100.0f / play_mode->rate;
		init_filter_lowpass1(lpf);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		free_delay(&(info->delayL));
		free_delay(&(info->delayR));
		return;
	}

	for (i = 0; i < count; i++)
	{
		lfb = imuldiv24(bufL[indexl], feedbacki);
		do_filter_lowpass1(&lfb, &x1l, ai, iai);
		lout = imuldiv24(buf[i], dryi) + imuldiv24(bufL[indexl], weti);
		rfb = imuldiv24(bufR[indexr], feedbacki);
		do_filter_lowpass1(&rfb, &x1r, ai, iai);
		rout = imuldiv24(buf[i + 1], dryi) + imuldiv24(bufR[indexr], weti);
		bufL[indexl] = buf[i] + rfb;
		buf[i] = lout;
		bufR[indexr] = buf[++i] + lfb;
		buf[i] = rout;

		if (++indexl == sizel) {indexl = 0;}
		if (++indexr == sizer) {indexr = 0;}
	}
	lpf->x1l = x1l, lpf->x1r = x1r;
	delayL->index = indexl, delayR->index = indexr;
}

static void conv_gs_lofi1(struct insertion_effect_gs_t *st, EffectList *ef)
{
	InfoLoFi1 *info = (InfoLoFi1 *)ef->info;

	info->pre_filter = st->parameter[0];
	info->lofi_type = st->parameter[1];
	info->post_filter = st->parameter[2];
	info->dry = calc_dry_gs(st->parameter[15]);
	info->wet = calc_wet_gs(st->parameter[15]);
	info->pan = st->parameter[18];
	info->level = (double)st->parameter[19] / 127.0f;
}

static void do_lofi1(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x, y;
	InfoLoFi1 *info = (InfoLoFi1 *)ef->info;
	int32 bit_mask = info->bit_mask, dryi = info->dryi,	weti = info->weti;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		info->bit_mask = ~((1L << (info->lofi_type + 22 - GUARD_BITS)) - 1L);
		info->dryi = TIM_FSCALE(info->dry * info->level, 24);
		info->weti = TIM_FSCALE(info->wet * info->level, 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = buf[i];
		y = x & bit_mask;
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);

		x = buf[++i];
		y = x & bit_mask;
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);
	}
}

static void conv_gs_lofi2(struct insertion_effect_gs_t *st, EffectList *ef)
{
	InfoLoFi2 *info = (InfoLoFi2 *)ef->info;

	info->lofi_type = clip_int(st->parameter[0], 1, 6);
	info->fil_type = clip_int(st->parameter[1], 0, 2);
	info->fil.freq = cutoff_freq_table_gs[st->parameter[2]];
	info->rdetune = st->parameter[3];
	info->rnz_lev = (double)st->parameter[4] / 127.0f;
	info->wp_sel = clip_int(st->parameter[5], 0, 1);
	info->wp_lpf.freq = lpf_table_gs[st->parameter[6]];
	info->wp_level = (double)st->parameter[7] / 127.0f;
	info->disc_type = clip_int(st->parameter[8], 0, 3);
	info->disc_lpf.freq = lpf_table_gs[st->parameter[9]];
	info->discnz_lev = (double)st->parameter[10] / 127.0f;
	info->hum_type = clip_int(st->parameter[11], 0, 1);
	info->hum_lpf.freq = lpf_table_gs[st->parameter[12]];
	info->hum_level = (double)st->parameter[13] / 127.0f;
	info->ms = clip_int(st->parameter[14], 0, 1);
	info->dry = calc_dry_gs(st->parameter[15]);
	info->wet = calc_wet_gs(st->parameter[15]);
	info->pan = st->parameter[18];
	info->level = (double)st->parameter[19] / 127.0f;
}

static void do_lofi2(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x, y;
	InfoLoFi2 *info = (InfoLoFi2 *)ef->info;
	filter_biquad *fil = &(info->fil);
	int32 bit_mask = info->bit_mask, dryi = info->dryi,	weti = info->weti;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		fil->q = 1.0f;
		if (info->fil_type == 1) {calc_filter_biquad_low(fil);}
		else if (info->fil_type == 2) {calc_filter_biquad_high(fil);}
		else {
			fil->freq = -1;	/* bypass */
			calc_filter_biquad_low(fil);
		}
		info->bit_mask = ~((1L << (info->lofi_type + 22 - GUARD_BITS)) - 1L);
		info->dryi = TIM_FSCALE(info->dry * info->level, 24);
		info->weti = TIM_FSCALE(info->wet * info->level, 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}
	for (i = 0; i < count; i++)
	{
		x = buf[i];
		y = x & bit_mask;
		do_filter_biquad(&y, fil->a1, fil->a2, fil->b1, fil->b02, &fil->x1l, &fil->x2l, &fil->y1l, &fil->y2l);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);

		x = buf[++i];
		y = x & bit_mask;
		do_filter_biquad(&y, fil->a1, fil->a2, fil->b1, fil->b02, &fil->x1r, &fil->x2r, &fil->y1r, &fil->y2r);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);
	}
}

static void conv_xg_lofi(struct effect_xg_t *st, EffectList *ef)
{
	InfoLoFi *info = (InfoLoFi *)ef->info;

	info->srf.freq = lofi_sampling_freq_table_xg[st->param_lsb[0]] / 2.0f;
	info->word_length = st->param_lsb[1];
	info->output_gain = clip_int(st->param_lsb[2], 0, 18);
	info->lpf.freq = eq_freq_table_xg[clip_int(st->param_lsb[3], 10, 80)];
	info->filter_type = st->param_lsb[4];
	info->lpf.q = (double)clip_int(st->param_lsb[5], 10, 120) / 10.0f;
	info->bit_assign = clip_int(st->param_lsb[6], 0, 6);
	info->emphasis = st->param_lsb[7];
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
}

static void do_lofi(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x, y;
	InfoLoFi *info = (InfoLoFi *)ef->info;
	filter_biquad *lpf = &(info->lpf), *srf = &(info->srf);
	int32 bit_mask = info->bit_mask, dryi = info->dryi,	weti = info->weti;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		srf->q = 1.0f;
		calc_filter_biquad_low(srf);
		calc_filter_biquad_low(lpf);
		info->bit_mask = ~((1L << (info->bit_assign + 22 - GUARD_BITS)) - 1L);
		info->dryi = TIM_FSCALE(info->dry * pow(10.0f, (double)info->output_gain / 20.0f), 24);
		info->weti = TIM_FSCALE(info->wet * pow(10.0f, (double)info->output_gain / 20.0f), 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = buf[i];
		y = x & bit_mask;
		do_filter_biquad(&y, srf->a1, srf->a2, srf->b1, srf->b02, &srf->x1l, &srf->x2l, &srf->y1l, &srf->y2l);
		do_filter_biquad(&y, lpf->a1, lpf->a2, lpf->b1, lpf->b02, &lpf->x1l, &lpf->x2l, &lpf->y1l, &lpf->y2l);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);

		x = buf[++i];
		y = x & bit_mask;
		do_filter_biquad(&y, srf->a1, srf->a2, srf->b1, srf->b02, &srf->x1r, &srf->x2r, &srf->y1r, &srf->y2r);
		do_filter_biquad(&y, lpf->a1, lpf->a2, lpf->b1, lpf->b02, &lpf->x1r, &lpf->x2r, &lpf->y1r, &lpf->y2r);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);
	}
}

static void conv_xg_auto_wah_od(struct effect_xg_t *st, EffectList *ef)
{
	InfoXGAutoWahOD *info = (InfoXGAutoWahOD *)ef->info;

	info->lpf.freq = eq_freq_table_xg[clip_int(st->param_lsb[13], 34, 80)];
	info->level = (double)st->param_lsb[14] / 127.0f;
}

static void conv_xg_auto_wah_od_eq3(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ3 *info = (InfoEQ3 *)ef->info;

	info->low_freq = eq_freq_table_xg[24];
	info->low_gain = clip_int(st->param_lsb[11] - 64, -12, 12);
	info->mid_freq = eq_freq_table_xg[41];
	info->mid_gain = clip_int(st->param_lsb[12] - 64, -12, 12);
	info->mid_width = 1.0f;
	info->high_freq = 0;
	info->high_gain = 0;
}

static void conv_xg_auto_wah_eq2(struct effect_xg_t *st, EffectList *ef)
{
	InfoEQ2 *info = (InfoEQ2 *)ef->info;

	info->low_freq = eq_freq_table_xg[clip_int(st->param_lsb[5], 4, 40)];
	info->low_gain = clip_int(st->param_lsb[6] - 64, -12, 12);
	info->high_freq = eq_freq_table_xg[clip_int(st->param_lsb[7], 28, 58)];
	info->high_gain = clip_int(st->param_lsb[8] - 64, -12, 12);
}

static void conv_xg_auto_wah(struct effect_xg_t *st, EffectList *ef)
{
	InfoXGAutoWah *info = (InfoXGAutoWah *)ef->info;

	info->lfo_freq = lfo_freq_table_xg[st->param_lsb[0]];
	info->lfo_depth = st->param_lsb[1];
	info->offset_freq = (double)(st->param_lsb[2]) * 3900.0f / 127.0f + 100.0f;
	info->resonance = (double)clip_int(st->param_lsb[3], 10, 120) / 10.0f;
	info->dry = calc_dry_xg(st->param_lsb[9], st);
	info->wet = calc_wet_xg(st->param_lsb[9], st);
	info->drive = st->param_lsb[10];
}

static inline double calc_xg_auto_wah_freq(int32 lfo_val, double offset_freq, int8 depth)
{
	double freq;
	int32 fine;
	fine = ((lfo_val - (1L << 15)) * depth) >> 7;	/* max: +-2^8 fine */
	if (fine >= 0) {
		freq = offset_freq * bend_fine[fine & 0xff]
			* bend_coarse[fine >> 8 & 0x7f];
	} else {
		freq = offset_freq / (bend_fine[(-fine) & 0xff]
			* bend_coarse[(-fine) >> 8 & 0x7f]);
	}
	return freq;
}

#define XG_AUTO_WAH_BITS (32 - GUARD_BITS)
#define XG_AUTO_WAH_MAX_NEG (1.0 / (double)(1L << XG_AUTO_WAH_BITS))

static void do_xg_auto_wah(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x, y, val;
	InfoXGAutoWah *info = (InfoXGAutoWah *)ef->info;
	filter_moog_dist *fil0 = &(info->fil0), *fil1 = &(info->fil1);
	lfo *lfo = &(info->lfo);
	int32 dryi = info->dryi, weti = info->weti, fil_cycle = info->fil_cycle;
	int8 lfo_depth = info->lfo_depth;
	double yf, offset_freq = info->offset_freq;
	int32 fil_count = info->fil_count;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		init_lfo(lfo, info->lfo_freq, LFO_TRIANGULAR, 0);
		fil0->res_dB = fil1->res_dB = (info->resonance - 1.0) * 12.0f / 11.0f;
		fil0->dist = fil1->dist = 4.0f * sqrt((double)info->drive / 127.0);
		val = do_lfo(lfo);
		fil0->freq = fil1->freq = calc_xg_auto_wah_freq(val, info->offset_freq, info->lfo_depth);
		calc_filter_moog_dist(fil0);
		init_filter_moog_dist(fil0);
		calc_filter_moog_dist(fil1);
		init_filter_moog_dist(fil1);
		info->fil_count = 0;
		info->fil_cycle = (int32)(44.0f * play_mode->rate / 44100.0f);
		info->dryi = TIM_FSCALE(info->dry, 24);
		info->weti = TIM_FSCALE(info->wet, 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = y = buf[i];
		yf = (double)y * XG_AUTO_WAH_MAX_NEG;
		do_filter_moog_dist_band(&yf, fil0->f, fil0->p, fil0->q, fil0->d,
								   &fil0->b0, &fil0->b1, &fil0->b2, &fil0->b3, &fil0->b4);
		y = TIM_FSCALE(yf, XG_AUTO_WAH_BITS);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);

		x = y = buf[++i];
		yf = (double)y * XG_AUTO_WAH_MAX_NEG;
		do_filter_moog_dist_band(&yf, fil0->f, fil0->p, fil0->q, fil0->d,
								   &fil1->b0, &fil1->b1, &fil1->b2, &fil1->b3, &fil1->b4);
		y = TIM_FSCALE(yf, XG_AUTO_WAH_BITS);
		buf[i] = imuldiv24(x, dryi) + imuldiv24(y, weti);

		val = do_lfo(lfo);

		if (++fil_count == fil_cycle) {
			fil_count = 0;
			fil0->freq = calc_xg_auto_wah_freq(val, offset_freq, lfo_depth);
			calc_filter_moog_dist(fil0);
		}
	}
	info->fil_count = fil_count;
}

static void do_xg_auto_wah_od(int32 *buf, int32 count, EffectList *ef)
{
	int32 i, x;
	InfoXGAutoWahOD *info = (InfoXGAutoWahOD *)ef->info;
	filter_biquad *lpf = &(info->lpf);
	int32 leveli = info->leveli;

	if(count == MAGIC_INIT_EFFECT_INFO) {
		lpf->q = 1.0f;
		calc_filter_biquad_low(lpf);
		info->leveli = TIM_FSCALE(info->level, 24);
		return;
	} else if(count == MAGIC_FREE_EFFECT_INFO) {
		return;
	}

	for (i = 0; i < count; i++)
	{
		x = buf[i];
		do_filter_biquad(&x, lpf->a1, lpf->a2, lpf->b1, lpf->b02, &lpf->x1l, &lpf->x2l, &lpf->y1l, &lpf->y2l);
		buf[i] = imuldiv24(x, leveli);

		x = buf[++i];
		do_filter_biquad(&x, lpf->a1, lpf->a2, lpf->b1, lpf->b02, &lpf->x1r, &lpf->x2r, &lpf->y1r, &lpf->y2r);
		buf[i] = imuldiv24(x, leveli);
	}
}

enum {	/* width * length * height */
	ER_SMALL_ROOM,	/* 5m * 5m * 3m */
	ER_MEDIUM_ROOM,	/* 10m * 10m * 5m */
	ER_LARGE_ROOM,	/* 12m * 12m * 8m */
	ER_MEDIUM_HALL,	/* 15m * 20m * 10m */
	ER_LARGE_HALL,	/* 20m * 40m * 18m */
	ER_EOF,
};

struct early_reflection_param_t {
	int8 character;
	int16 time_left[20];	/* in ms */
	float weight_left[20];
	int16 time_right[20];	/* in ms */
	float weight_right[20];
};

static struct early_reflection_param_t early_reflection_param[] = {
	ER_SMALL_ROOM, 461, 487, 505, 530, 802, 818, 927, 941, 1047, 1058, 1068, 1070, 1076, 1096, 1192, 1203, 1236, 1261, 1321, 1344,
	0.1594, 0.1328, 0.1197, 0.1025, 0.0285, 0.0267, 0.0182, 0.0173, 0.0098, 0.0121, 0.0092, 0.0116, 0.0090, 0.0085, 0.0084, 0.0081, 0.0059, 0.0055, 0.0048, 0.0045,
	381, 524, 621, 636, 718, 803, 890, 906, 975, 1016, 1026, 1040, 1137, 1166, 1212, 1220, 1272, 1304, 1315, 1320,
	0.1896, 0.0706, 0.0467, 0.0388, 0.0298, 0.0211, 0.0137, 0.0181, 0.0144, 0.0101, 0.0088, 0.0118, 0.0072, 0.0081, 0.0073, 0.0071, 0.0062, 0.0042, 0.0057, 0.0024,
	ER_MEDIUM_ROOM, 461, 802, 927, 1047, 1236, 1321, 1345, 1468, 1497, 1568, 1609, 1642, 1713, 1744, 1768, 1828, 1835, 1864, 1893, 1923,
	0.3273, 0.0586, 0.0374, 0.0201, 0.0120, 0.0098, 0.0152, 0.0090, 0.0109, 0.0095, 0.0068, 0.0034, 0.0073, 0.0041, 0.0027, 0.0024, 0.0059, 0.0034, 0.0054, 0.0035,
	381, 636, 906, 1026, 1040, 1315, 1320, 1415, 1445, 1556, 1588, 1628, 1637, 1663, 1693, 1768, 1788, 1824, 1882, 1920,
	0.1896, 0.0706, 0.0467, 0.0388, 0.0298, 0.0211, 0.0137, 0.0181, 0.0144, 0.0101, 0.0088, 0.0118, 0.0072, 0.0081, 0.0073, 0.0071, 0.0062, 0.0042, 0.0057, 0.0024,
	ER_LARGE_ROOM, 577, 875, 1665, 1713, 1784, 1790, 1835, 1913, 1954, 2023, 2062, 2318, 2349, 2371, 2405, 2409, 2469, 2492, 2534, 2552,
	0.2727, 0.0752, 0.0070, 0.0110, 0.0083, 0.0056, 0.0089, 0.0079, 0.0051, 0.0066, 0.0043, 0.0019, 0.0035, 0.0023, 0.0038, 0.0017, 0.0015, 0.0029, 0.0027, 0.0032,
	381, 636, 1485, 1570, 1693, 1768, 1875, 1895, 1963, 2066, 2128, 2220, 2277, 2309, 2361, 2378, 2432, 2454, 2497, 2639,
	0.5843, 0.1197, 0.0117, 0.0098, 0.0032, 0.0028, 0.0042, 0.0022, 0.0020, 0.0038, 0.0035, 0.0043, 0.0040, 0.0022, 0.0028, 0.0035, 0.0033, 0.0018, 0.0010, 0.0008,
	ER_MEDIUM_HALL, 1713, 1835, 2534, 2618, 2780, 2849, 2857, 3000, 3071, 3159, 3226, 3338, 3349, 3407, 3413, 3465, 3594, 3669, 3714, 3728,
	0.0146, 0.0118, 0.0036, 0.0033, 0.0033, 0.0030, 0.0031, 0.0013, 0.0012, 0.0023, 0.0021, 0.0018, 0.0016, 0.0015, 0.0016, 0.0016, 0.0015, 0.0013, 0.0006, 0.0012,
	381, 636, 1693, 1768, 2066, 2128, 2362, 2416, 2454, 2644, 2693, 2881, 2890, 2926, 2957, 3036, 3185, 3328, 3385, 3456,
	0.7744, 0.1586, 0.0042, 0.0037, 0.0051, 0.0046, 0.0036, 0.0034, 0.0024, 0.0018, 0.0017, 0.0024, 0.0015, 0.0023, 0.0008, 0.0012, 0.0013, 0.0005, 0.0012, 0.0005,
	ER_LARGE_HALL, 1713, 1835, 2534, 2618, 3159, 3226, 3669, 3728, 4301, 4351, 4936, 5054, 5097, 5278, 5492, 5604, 5631, 5714, 5751, 5800,
	0.0150, 0.0121, 0.0037, 0.0034, 0.0023, 0.0022, 0.0013, 0.0012, 0.0005, 0.0005, 0.0006, 0.0002, 0.0002, 0.0004, 0.0004, 0.0004, 0.0004, 0.0002, 0.0002, 0.0003,
	381, 636, 1693, 1768, 2066, 2128, 2644, 2693, 3828, 3862, 4169, 4200, 4792, 5068, 5204, 5231, 5378, 5460, 5485, 5561,
	0.7936, 0.1625, 0.0043, 0.0038, 0.0052, 0.0047, 0.0018, 0.0017, 0.0008, 0.0008, 0.0007, 0.0007, 0.0003, 0.0001, 0.0003, 0.0002, 0.0002, 0.0002, 0.0001, 0.0003,
	ER_EOF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

struct _EffectEngine effect_engine[] = {
	EFFECT_NONE, "None", NULL, NULL, NULL, 0,
	EFFECT_STEREO_EQ, "Stereo-EQ", do_stereo_eq, conv_gs_stereo_eq, NULL, sizeof(InfoStereoEQ),
	EFFECT_EQ2, "2-Band EQ", do_eq2, conv_gs_eq2, conv_xg_eq2, sizeof(InfoEQ2),
	EFFECT_EQ3, "3-Band EQ", do_eq3, NULL, conv_xg_eq3, sizeof(InfoEQ3),
	EFFECT_OVERDRIVE1, "Overdrive", do_overdrive1, conv_gs_overdrive1, NULL, sizeof(InfoOverdrive1),
	EFFECT_DISTORTION1, "Distortion", do_distortion1, conv_gs_overdrive1, NULL, sizeof(InfoOverdrive1),
	EFFECT_OD1OD2, "OD1/OD2", do_dual_od, conv_gs_dual_od, NULL, sizeof(InfoOD1OD2),
	EFFECT_HEXA_CHORUS, "Hexa-Chorus", do_hexa_chorus, conv_gs_hexa_chorus, NULL, sizeof(InfoHexaChorus),
	EFFECT_CHORUS, "Chorus", do_chorus, NULL, conv_xg_chorus, sizeof(InfoChorus),
	EFFECT_FLANGER, "Flanger", do_chorus, NULL, conv_xg_flanger, sizeof(InfoChorus),
	EFFECT_SYMPHONIC, "Symphonic", do_chorus, NULL, conv_xg_symphonic, sizeof(InfoChorus),
	EFFECT_CHORUS_EQ3, "3-Band EQ (XG Chorus built-in)", do_eq3, NULL, conv_xg_chorus_eq3, sizeof(InfoEQ3),
	EFFECT_STEREO_OVERDRIVE, "Stereo Overdrive", do_stereo_od, NULL, conv_xg_overdrive, sizeof(InfoStereoOD),
	EFFECT_STEREO_DISTORTION, "Stereo Distortion", do_stereo_od, NULL, conv_xg_distortion, sizeof(InfoStereoOD),
	EFFECT_STEREO_AMP_SIMULATOR, "Amp Simulator", do_stereo_od, NULL, conv_xg_amp_simulator, sizeof(InfoStereoOD),
	EFFECT_OD_EQ3, "2-Band EQ (XG OD built-in)", do_eq3, NULL, conv_xg_od_eq3, sizeof(InfoEQ3),
	EFFECT_DELAY_LCR, "Delay L,C,R", do_delay_lcr, NULL, conv_xg_delay_lcr, sizeof(InfoDelayLCR),
	EFFECT_DELAY_LR, "Delay L,R", do_delay_lr, NULL, conv_xg_delay_lr, sizeof(InfoDelayLR),
	EFFECT_ECHO, "Echo", do_echo, NULL, conv_xg_echo, sizeof(InfoEcho),
	EFFECT_CROSS_DELAY, "Cross Delay", do_cross_delay, NULL, conv_xg_cross_delay, sizeof(InfoCrossDelay),
	EFFECT_DELAY_EQ2, "2-Band EQ (XG Delay built-in)", do_eq2, NULL, conv_xg_delay_eq2, sizeof(InfoEQ2),
	EFFECT_LOFI, "Lo-Fi", do_lofi, NULL, conv_xg_lofi, sizeof(InfoLoFi),
	EFFECT_LOFI1, "Lo-Fi 1", do_lofi1, conv_gs_lofi1, NULL, sizeof(InfoLoFi1),
	EFFECT_LOFI2, "Lo-Fi 2", do_lofi2, conv_gs_lofi2, NULL, sizeof(InfoLoFi2),
	EFFECT_XG_AUTO_WAH, "Auto Wah", do_xg_auto_wah, NULL, conv_xg_auto_wah, sizeof(InfoXGAutoWah),
	EFFECT_XG_AUTO_WAH_EQ2, "2-Band EQ (Auto Wah built-in)", do_eq2, NULL, conv_xg_auto_wah_eq2, sizeof(InfoEQ2),
	EFFECT_XG_AUTO_WAH_OD, "OD (Auto Wah built-in)", do_xg_auto_wah_od, NULL, conv_xg_auto_wah_od, sizeof(InfoXGAutoWahOD),
	EFFECT_XG_AUTO_WAH_OD_EQ3, "2-Band EQ (Auto Wah OD built-in)", do_eq3, NULL, conv_xg_auto_wah_od_eq3, sizeof(InfoEQ3),
	-1, "EOF", NULL, NULL, NULL, 0, 
};

struct effect_parameter_xg_t effect_parameter_xg[] = {
	0, 0, "NO EFFECT", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x05, 0, "DELAY L,C,R", 0x1A, 0x0D, 0x27, 0x27, 0, 0, 0, 0, 0, 0,
	0x05, 0x03, 0x08, 0x08, 74, 100, 10, 0, 0, 32, 0, 0, 28, 64, 46, 64, 9,
	0x06, 0, "DELAY L,R", 0x13, 0x1D, 0x1D, 0x1D, 0, 0, 0, 0, 0, 0,
	0x44, 0x26, 0x28, 0x26, 87, 10, 0, 0, 0, 32, 0, 0, 28, 64, 46, 64, 9,
	0x07, 0, "ECHO", 0x0D, 0, 0x0D, 0, 0, 0x0D, 0x0D, 0, 0, 0,
	0x24, 80, 0x74, 80, 10, 0x24, 0x74, 0, 0, 40, 0, 0, 28, 64, 46, 64, 9,
	0x08, 0, "CROSS DELAY", 0x0D, 0x0D, 0, 0, 0, 0, 0, 0, 0, 0,
	0x24, 0x56, 111, 1, 10, 0, 0, 0, 0, 32, 0, 0, 28, 64, 46, 64, 9,
	0x41, 0, "CHORUS 1", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 54, 77, 106, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x01, "CHORUS 2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8, 63, 64, 30, 0, 28, 62, 42, 58, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x02, "CHORUS 3", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 44, 64, 110, 0, 28, 64, 46, 66, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x03, "GM CHORUS 1", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	9, 10, 64, 109, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x04, "GM CHORUS 2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	28, 34, 67, 105, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x05, "GM CHORUS 3", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	9, 34, 69, 105, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x06, "GM CHORUS 4", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	26, 29, 75, 102, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x07, "FB CHORUS", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	6, 43, 107, 111, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 0, 0, 9,
	0x41, 0x08, "CHORUS 4", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	9, 32, 69, 104, 0, 28, 64, 46, 64, 64, 46, 64, 10, 0, 1, 0, 9,
	0x42, 0, "CELESTE 1", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	12, 32, 64, 0, 0, 28, 64, 46, 64, 127, 40, 68, 10, 0, 0, 0, 9,
	0x42, 0x01, "CELESTE 2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	28, 18, 90, 2, 0, 28, 62, 42, 60, 84, 40, 68, 10, 0, 0, 0, 9,
	0x42, 0x02, "CELESTE 3", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 63, 44, 2, 0, 28, 64, 46, 68, 127, 40, 68, 10, 0, 0, 0, 9,
	0x42, 0x08, "CELESTE 4", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8, 29, 64, 0, 0, 28, 64, 51, 66, 127, 40, 68, 10, 0, 1, 0, 9,
	0x43, 0, "FLANGER 1", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	14, 14, 104, 2, 0, 28, 64, 46, 64, 96, 40, 64, 10, 4, 0, 0, 9, 
	0x43, 0x01, "FLANGER 2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	32, 17, 26, 2, 0, 28, 64, 46, 60, 96, 40, 64, 10, 4, 0, 0, 9, 
	0x43, 0x07, "GM FLANGER", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 21, 120, 1, 0, 28, 64, 46, 64, 96, 40, 64, 10, 4, 0, 0, 9, 
	0x43, 0x08, "FLANGER 3", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 109, 109, 2, 0, 28, 64, 46, 64, 127, 40, 64, 10, 4, 0, 0, 9, 
	0x44, 0, "SYMPHONIC", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	12, 25, 16, 0, 0, 28, 64, 46, 64, 127, 46, 64, 10, 0, 0, 0, 9,
	0x49, 0, "DISTORTION", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	40, 20, 72, 53, 48, 0, 43, 74, 10, 127, 120, 0, 0, 0, 0, 0, 0, 
	0x49, 0x08, "STEREO DISTORTION", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	18, 27, 71, 48, 84, 0, 32, 66, 10, 127, 105, 0, 0, 0, 0, 0, 0, 
	0x4A, 0, "OVERDRIVE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	29, 24, 68, 45, 55, 0, 41, 72, 10, 127, 104, 0, 0, 0, 0, 0, 0, 
	0x4A, 0x08, "STEREO OVERDRIVE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	10, 24, 69, 46, 105, 0, 41, 66, 10, 127, 104, 0, 0, 0, 0, 0, 0, 
	0x4B, 0, "AMP SIMULATOR", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	39, 1, 48, 55, 0, 0, 0, 0, 0, 127, 112, 0, 0, 0, 0, 0, 0, 
	0x4B, 0x08, "STEREO AMP SIMULATOR", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	16, 2, 46, 119, 0, 0, 0, 0, 0, 127, 106, 0, 0, 0, 0, 0, 0, 
	0x4C, 0, "3-BAND EQ", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	70, 34, 60, 10, 70, 28, 46, 0, 0, 127, 0, 0, 0, 0, 0, 0, -1, 
	0x4D, 0, "2-BAND EQ", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	28, 70, 46, 70, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, -1, 
	0x4E, 0, "AUTO WAH", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	70, 56, 39, 25, 0, 28, 66, 46, 64, 127, 0, 0, 0, 0, 0, 0, 2, 
	0x4E, 0x01, "AUTO WAH+DISTORTION", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	40, 73, 26, 29, 0, 28, 66, 46, 64, 127, 30, 72, 74, 53, 48, 0, 2, 
	0x4E, 0x02, "AUTO WAH+OVERDRIVE", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	48, 64, 32, 23, 0, 28, 66, 46, 64, 127, 29, 68, 72, 45, 55, 0, 2, 
	0x5E, 0, "LO-FI", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 60, 6, 54, 5, 10, 1, 1, 0, 127, 0, 0, 0, 0, 1, 0, 9, 
	-1, -1, "EOF", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

struct effect_parameter_gs_t effect_parameter_gs[] = {
	0, 0, "None", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0x01, 0x00, "Stereo-EQ", 1, 0x45, 1, 0x34, 0x48, 0, 0x48, 0x38, 0, 0x48,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 19, -1,
	0x01, 0x10, "Overdrive", 48, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0x40, 0x40, 0x40, 96, 0, 18,
	0x01, 0x11, "Distrotion", 76, 3, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0x40, 0x38, 0x40, 84, 0, 18, 
	0x11, 0x03, "OD1/OD2", 0, 48, 1, 1, 0, 1, 76, 3, 1, 0,
	0, 0, 0, 0, 0, 0x40, 96, 0x40, 84, 127, 1, 6, 
	0x01, 0x40, "Hexa Chorus", 0x18, 0x08, 127, 5, 66, 16, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0x40, 0x40, 64, 112, 1, 15, 
	0x01, 0x72, "Lo-Fi 1", 2, 6, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 127, 0x40, 0x40, 64, 127, 15, 18, 
	0x01, 0x73, "Lo-Fi 2", 2, 1, 0x20, 0, 64, 1, 127, 0, 0, 127,
	0, 0, 127, 0, 1, 127, 0x40, 0x40, 64, 127, 3, 15, 
	-1, -1, "EOF", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
