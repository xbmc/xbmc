/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    effect.c - To apply sound effects.
    Mainly written by Masanao Izumo <iz@onicos.co.jp>

    Interfaces:
    void init_effect(void);
    do_effect(int32* buf, int32 count);
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "mt19937ar.h"
#define RAND_MAX 0xffffffff

#include "timidity.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "reverb.h"

#define SIDE_CONTI_SEC	10
#define CHANGE_SEC		2.0

#define NS_AMP_MAX	((int32) 0x0fffffff)
#define NS_AMP_MIN	((int32)-0x0fffffff)

static void effect_left_right_delay(int32 *, int32);
static void init_mtrand(void);
static void init_ns_tap(void);
static void init_ns_tap16(void);
static void ns_shaping8(int32 *, int32);
static void ns_shaping16(int32 *, int32);
static void ns_shaping16_trad(int32 *, int32);
static void do_soft_clipping1(int32 *, int32);
static void do_soft_clipping2(int32 *, int32);
static void ns_shaping16_9(int32 *, int32);
static inline unsigned long frand(void);
static inline int32 my_mod(int32, int32);

static int32 ns_z0[4];
static int32 ns_z1[4];
static const int32 ns9_order = 9;
static int32 ns9_histposl, ns9_histposr;
static int32 ns9_ehl[18];
static int32 ns9_ehr[18];
static uint32 ns9_r1l, ns9_r2l, ns9_r1r, ns9_r2r;
static const float ns9_coef[9] = {
	2.412f, -3.370f, 3.937f, -4.174f, 3.353f,
	-2.205f, 1.281f, -0.569f, 0.0847f
};
static int32 ns9_c[9];
int noise_sharp_type = 4;

void init_effect(void)
{
	effect_left_right_delay(NULL, 0);
	init_mtrand();
	init_pink_noise(&global_pink_noise_light);
	init_ns_tap();
	init_reverb();
	init_ch_delay();
	init_ch_chorus();
	init_eq_gs();
}

/*
 * Left & Right Delay Effect
 */
static void effect_left_right_delay(int32 *buff, int32 count)
{
	static int32 prev[AUDIO_BUFFER_SIZE * 2];
	int32 save[AUDIO_BUFFER_SIZE * 2];
	int32 pi, i, j, k, v, backoff;
	int b;
	static int turn_counter = 0, tc;
	static int status;
	static double rate0, rate1, dr;
	int32 *p;
	
	if (buff == NULL) {
		memset(prev, 0, sizeof(prev));
		return;
	}
	if (play_mode->encoding & PE_MONO)
		return;
	if (effect_lr_mode == 0 || effect_lr_mode == 1 || effect_lr_mode == 2)
		b = effect_lr_mode;
	else
		return;
	count *= 2;
	backoff = 2 * (int) (play_mode->rate * effect_lr_delay_msec / 1000.0);
	if (backoff == 0)
		return;
	if (backoff > count)
		backoff = count;
	if (count < audio_buffer_size * 2) {
		memset(buff + count, 0, 4 * (audio_buffer_size * 2 - count));
		count = audio_buffer_size * 2;
	}
	memcpy(save, buff, 4 * count);
	pi = count - backoff;
	if (b == 2) {
		if (turn_counter == 0) {
			turn_counter = SIDE_CONTI_SEC * play_mode->rate;
			/* status: 0 -> 2 -> 3 -> 1 -> 4 -> 5 -> 0 -> ...
			 * status left   right
			 * 0      -      +		(right)
			 * 1      +      -		(left)
			 * 2     -> +    +		(right -> center)
			 * 3      +     -> -	(center -> left)
			 * 4     -> -    -		(left -> center)
			 * 5      -     -> +	(center -> right)
			 */
			status = 0;
			tc = 0;
		}
		p = prev;
		for (i = 0; i < count; i += 2, pi += 2) {
			if (i < backoff)
				p = prev;
			else if (p == prev) {
				pi = 0;
				p = save;
			}
			if (status < 2)
				buff[i + status] = p[pi + status];
			else if (status < 4) {
				j = (status & 1);
				v = (int32) (rate0 * buff[i + j] + rate1 * p[pi + j]);
				buff[i + j] = v;
				rate0 += dr, rate1 -= dr;
			} else {
				j = (status & 1);
				k = ! j;
				v = (int32) (rate0 * buff[i + j] + rate1 * p[pi + j]);
				buff[i + j] = v;
				buff[i + k] = p[pi + k];
				rate0 += dr, rate1 -= dr;
			}
			tc++;
			if (tc == turn_counter) {
				tc = 0;
				switch (status) {
				case 0:
					status = 2;
					turn_counter = (CHANGE_SEC / 2.0) * play_mode->rate;
					rate0 = 0.0;
					rate1 = 1.0;
					dr = 1.0 / turn_counter;
					break;
				case 2:
					status = 3;
					rate0 = 1.0;
					rate1 = 0.0;
					dr = -1.0 / turn_counter;
					break;
				case 3:
					status = 1;
					turn_counter = SIDE_CONTI_SEC * play_mode->rate;
					break;
				case 1:
					status = 4;
					turn_counter = (CHANGE_SEC / 2.0) * play_mode->rate;
					rate0 = 1.0;
					rate1 = 0.0;
					dr = -1.0 / turn_counter;
					break;
				case 4:
					status = 5;
					turn_counter = (CHANGE_SEC / 2.0) * play_mode->rate;
					rate0 = 0.0;
					rate1 = 1.0;
					dr = 1.0 / turn_counter;
					break;
				case 5:
					status = 0;
					turn_counter = SIDE_CONTI_SEC * play_mode->rate;
					break;
				}
			}
		}
	} else {
		for (i = 0; i < backoff; i += 2, pi += 2)
			buff[b + i] = prev[b + pi];
		for (pi = 0; i < count; i += 2, pi += 2)
			buff[b + i] = save[b + pi];
	}
	memcpy(prev + count - backoff, save + count - backoff, 4 * backoff);
}

static void init_mtrand(void)
{
	unsigned long init[4] = { 0x123, 0x234, 0x345, 0x456 };
	unsigned long length = 4;
	init_by_array(init, length);
}

/* Noise Shaping filter from
 * Kunihiko IMAI <imai@leo.ec.t.kanazawa-u.ac.jp>
 * (Modified by Masanao Izumo <mo@goice.co.jp>)
 */
static void init_ns_tap(void)
{
	memset(ns_z0, 0, sizeof(ns_z0));
	memset(ns_z1, 0, sizeof(ns_z1));
	if (play_mode->encoding & PE_16BIT)
		init_ns_tap16();
}

static void init_ns_tap16(void)
{
	int i;

	for (i = 0; i < ns9_order; i++)
		ns9_c[i] = TIM_FSCALE(ns9_coef[i], 24);
	memset(ns9_ehl, 0, sizeof(ns9_ehl));
	memset(ns9_ehr, 0, sizeof(ns9_ehr));
	ns9_histposl = ns9_histposr = 8;
	ns9_r1l = ns9_r2l = ns9_r1r = ns9_r2r = 0;
}

void do_effect(int32 *buf, int32 count)
{
	int32 nsamples = (play_mode->encoding & PE_MONO)
			? count : count * 2;
	int reverb_level = (opt_reverb_control < 0)
			? -opt_reverb_control & 0x7f : DEFAULT_REVERB_SEND_LEVEL;

	/* reverb in mono */
	if (opt_reverb_control && play_mode->encoding & PE_MONO)
		do_mono_reverb(buf, count);
	/* for static reverb / chorus level */
	if (opt_reverb_control == 2 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x80))
			|| opt_chorus_control < 0) {
		set_dry_signal(buf, nsamples);
		/* chorus sounds horrible
		 * if applied globally on top of channel chorus
		 */
#if 0
		if (opt_chorus_control < 0)
			set_ch_chorus(buf, nsamples, -opt_chorus_control);
#endif
		if (opt_reverb_control == 2 || opt_reverb_control == 4
			|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x80)))
			set_ch_reverb(buf, nsamples, reverb_level);
		mix_dry_signal(buf, nsamples);
			/* chorus sounds horrible
			 * if applied globally on top of channel chorus
			 */
#if 0
		if (opt_chorus_control < 0 && !opt_surround_chorus)
			do_ch_chorus(buf, nsamples);
#endif
		if (opt_reverb_control == 2 || opt_reverb_control == 4
				|| (opt_reverb_control < 0 && ! (opt_reverb_control & 0x80)))
			do_ch_reverb(buf, nsamples);
	}
	/* L/R Delay */
	effect_left_right_delay(buf, count);
	/* Noise shaping filter must apply at last */
	if (play_mode->encoding & PE_24BIT) {}
	else if (! (play_mode->encoding & (PE_16BIT | PE_ULAW | PE_ALAW)))
		ns_shaping8(buf, count);
	else if (play_mode->encoding & PE_16BIT)
		ns_shaping16(buf, count);
}

static void ns_shaping8(int32 *lp, int32 c)
{
	int32 l, i, ll;
	int32 ns_tap_0, ns_tap_1, ns_tap_2, ns_tap_3;
	
	switch (noise_sharp_type) {
	case 1:
		ns_tap_0 = 1;
		ns_tap_1 = 0;
		ns_tap_2 = 0;
		ns_tap_3 = 0;
		break;
	case 2:
		ns_tap_0 = -2;
		ns_tap_1 = 1;
		ns_tap_2 = 0;
		ns_tap_3 = 0;
		break;
	case 3:
		ns_tap_0 = 3;
		ns_tap_1 = -3;
		ns_tap_2 = 1;
		ns_tap_3 = 0;
		break;
	case 4:
		ns_tap_0 = -4;
		ns_tap_1 = 6;
		ns_tap_2 = -4;
		ns_tap_3 = 1;
		break;
	default:
		return;
	}
	if (! (play_mode->encoding & PE_MONO))
		c *= 2;
	for (i = 0; i < c; i++) {
		/* applied noise-shaping filter */
		if (lp[i] > NS_AMP_MAX)
			lp[i] = NS_AMP_MAX;
		else if (lp[i] < NS_AMP_MIN)
			lp[i] = NS_AMP_MIN;
		ll = lp[i] + ns_tap_0 * ns_z0[0] + ns_tap_1 * ns_z0[1]
				+ ns_tap_2 * ns_z0[2] + ns_tap_3 * ns_z0[3];
		l = ll >> (32 - 8 - GUARD_BITS);
		lp[i] = l << (32 - 8 - GUARD_BITS);
		ns_z0[3] = ns_z0[2], ns_z0[2] = ns_z0[1], ns_z0[1] = ns_z0[0];
		ns_z0[0] = ll - l * (1U << (32 - 8 - GUARD_BITS));
		if (play_mode->encoding & PE_MONO)
			continue;
		i++;
		if (lp[i] > NS_AMP_MAX)
			lp[i] = NS_AMP_MAX;
		else if (lp[i] < NS_AMP_MIN)
			lp[i] = NS_AMP_MIN;
		ll = lp[i] + ns_tap_0 * ns_z1[0] + ns_tap_1 * ns_z1[1]
				+ ns_tap_2 * ns_z1[2] + ns_tap_3 * ns_z1[3];
		l = ll >> (32 - 8 - GUARD_BITS);
		lp[i] = l << (32 - 8 - GUARD_BITS);
		ns_z1[3] = ns_z1[2], ns_z1[2] = ns_z1[1], ns_z1[1] = ns_z1[0];
		ns_z1[0] = ll - l * (1U << (32 - 8 - GUARD_BITS));
	}
}

static void ns_shaping16(int32 *lp, int32 c)
{
	if (! (play_mode->encoding & PE_MONO))
		c *= 2;
	switch (noise_sharp_type) {
	case 1:
		ns_shaping16_trad(lp, c);
		break;
	case 2:
		do_soft_clipping1(lp, c);
		ns_shaping16_9(lp, c);
		break;
	case 3:
		do_soft_clipping2(lp, c);
		ns_shaping16_9(lp, c);
		break;
	case 4:
		ns_shaping16_9(lp, c);
		break;
	default:
		break;
	}
}

static void ns_shaping16_trad(int32 *lp, int32 c)
{
	int32 l, i, ll;
	int32 ns_tap_0, ns_tap_1, ns_tap_2, ns_tap_3;
	
	ns_tap_0 = -4;
	ns_tap_1 = 6;
	ns_tap_2 = -4;
	ns_tap_3 = 1;
	for (i = 0; i < c; i++) {
		/* applied noise-shaping filter */
		if (lp[i] > NS_AMP_MAX)
			lp[i] = NS_AMP_MAX;
		else if (lp[i] < NS_AMP_MIN)
			lp[i] = NS_AMP_MIN;
		ll = lp[i] + ns_tap_0 * ns_z0[0] + ns_tap_1 * ns_z0[1]
				+ ns_tap_2 * ns_z0[2] + ns_tap_3 * ns_z0[3];
		l = ll >> (32 - 16 - GUARD_BITS);
		lp[i] = l << (32 - 16 - GUARD_BITS);
		ns_z0[3] = ns_z0[2], ns_z0[2] = ns_z0[1], ns_z0[1] = ns_z0[0];
		ns_z0[0] = ll - l * (1U << (32 - 16 - GUARD_BITS));
		if (play_mode->encoding & PE_MONO)
			continue;
		i++;
		if (lp[i] > NS_AMP_MAX)
			lp[i] = NS_AMP_MAX;
		else if (lp[i] < NS_AMP_MIN)
			lp[i] = NS_AMP_MIN;
		ll = lp[i] + ns_tap_0 * ns_z1[0] + ns_tap_1 * ns_z1[1]
				+ ns_tap_2 * ns_z1[2] + ns_tap_3 * ns_z1[3];
		l = ll >> (32 - 16 - GUARD_BITS);
		lp[i] = l << (32 - 16 - GUARD_BITS);
		ns_z1[3] = ns_z1[2], ns_z1[2] = ns_z1[1], ns_z1[1] = ns_z1[0];
		ns_z1[0] = ll - l * (1U << (32 - 16 - GUARD_BITS));
	}
}

#define WS_AMP_MAX	((int32) 0x0fffffff)
#define WS_AMP_MIN	((int32)-0x0fffffff)

static void do_soft_clipping1(int32 *buf, int32 count)
{
	int32 i, x;
	int32 ai = TIM_FSCALE(1.5f, 24), bi = TIM_FSCALE(0.5f, 24);
	
	for (i = 0; i < count; i++) {
		x = buf[i];
		x = (x > WS_AMP_MAX) ? WS_AMP_MAX
				: ((x < WS_AMP_MIN) ? WS_AMP_MIN : x);
		x = imuldiv24(x, ai) - imuldiv24(imuldiv28(imuldiv28(x, x), x), bi);
		buf[i] = x;
	}
}

static void do_soft_clipping2(int32 *buf, int32 count)
{
	int32 i, x;
	
	for (i = 0; i < count; i++) {
		x = buf[i];
		x = (x > WS_AMP_MAX) ? WS_AMP_MAX
				: ((x < WS_AMP_MIN) ? WS_AMP_MIN : x);
		x = signlong(x) * ((abs(x) << 1) - imuldiv28(x, x));
		buf[i] = x;
	}
}

static void ns_shaping16_9(int32 *lp, int32 c)
{
	int32 i, l, sample, output;
	
	for (i = 0; i < c; i++) {
		/* left channel */
		ns9_r2l = ns9_r1l;
		ns9_r1l = frand();
		lp[i] = (lp[i] > NS_AMP_MAX) ? NS_AMP_MAX
				: (lp[i] < NS_AMP_MIN) ? NS_AMP_MIN : lp[i];
#if OPT_MODE != 0
		sample = lp[i] - imuldiv24(ns9_c[8], ns9_ehl[ns9_histposl + 8])
				- imuldiv24(ns9_c[7], ns9_ehl[ns9_histposl + 7])
				- imuldiv24(ns9_c[6], ns9_ehl[ns9_histposl + 6])
				- imuldiv24(ns9_c[5], ns9_ehl[ns9_histposl + 5])
				- imuldiv24(ns9_c[4], ns9_ehl[ns9_histposl + 4])
				- imuldiv24(ns9_c[3], ns9_ehl[ns9_histposl + 3])
				- imuldiv24(ns9_c[2], ns9_ehl[ns9_histposl + 2])
				- imuldiv24(ns9_c[1], ns9_ehl[ns9_histposl + 1])
				- imuldiv24(ns9_c[0], ns9_ehl[ns9_histposl]);
#else
		sample = lp[i] - ns9_coef[8] * ns9_ehl[ns9_histposl + 8]
				- ns9_coef[7] * ns9_ehl[ns9_histposl + 7]
				- ns9_coef[6] * ns9_ehl[ns9_histposl + 6]
				- ns9_coef[5] * ns9_ehl[ns9_histposl + 5]
				- ns9_coef[4] * ns9_ehl[ns9_histposl + 4]
				- ns9_coef[3] * ns9_ehl[ns9_histposl + 3]
				- ns9_coef[2] * ns9_ehl[ns9_histposl + 2]
				- ns9_coef[1] * ns9_ehl[ns9_histposl + 1]
				- ns9_coef[0] * ns9_ehl[ns9_histposl];
#endif
		l = sample >> (32 - 16 - GUARD_BITS);
		output = l * (1U << (32 - 16 - GUARD_BITS))
				+ ((ns9_r1l - ns9_r2l) >> 30);
		ns9_histposl = my_mod((ns9_histposl + 8), ns9_order);
		ns9_ehl[ns9_histposl + 9] = ns9_ehl[ns9_histposl] = output - sample;
		lp[i] = output;
		/* right channel */
		i++;
		ns9_r2r = ns9_r1r;
		ns9_r1r = frand();
		lp[i] = (lp[i] > NS_AMP_MAX) ? NS_AMP_MAX
				: (lp[i] < NS_AMP_MIN) ? NS_AMP_MIN : lp[i];
#if OPT_MODE != 0
		sample = lp[i] - imuldiv24(ns9_c[8], ns9_ehr[ns9_histposr + 8])
				- imuldiv24(ns9_c[7], ns9_ehr[ns9_histposr + 7])
				- imuldiv24(ns9_c[6], ns9_ehr[ns9_histposr + 6])
				- imuldiv24(ns9_c[5], ns9_ehr[ns9_histposr + 5])
				- imuldiv24(ns9_c[4], ns9_ehr[ns9_histposr + 4])
				- imuldiv24(ns9_c[3], ns9_ehr[ns9_histposr + 3])
				- imuldiv24(ns9_c[2], ns9_ehr[ns9_histposr + 2])
				- imuldiv24(ns9_c[1], ns9_ehr[ns9_histposr + 1])
				- imuldiv24(ns9_c[0], ns9_ehr[ns9_histposr]);
#else
		sample = lp[i] - ns9_coef[8] * ns9_ehr[ns9_histposr + 8]
				- ns9_coef[7] * ns9_ehr[ns9_histposr + 7]
				- ns9_coef[6] * ns9_ehr[ns9_histposr + 6]
				- ns9_coef[5] * ns9_ehr[ns9_histposr + 5]
				- ns9_coef[4] * ns9_ehr[ns9_histposr + 4]
				- ns9_coef[3] * ns9_ehr[ns9_histposr + 3]
				- ns9_coef[2] * ns9_ehr[ns9_histposr + 2]
				- ns9_coef[1] * ns9_ehr[ns9_histposr + 1]
				- ns9_coef[0] * ns9_ehr[ns9_histposr];
#endif
		l = sample >> (32 - 16 - GUARD_BITS);
		output = l * (1U << (32 - 16 - GUARD_BITS))
				+ ((ns9_r1r - ns9_r2r) >> 30);
		ns9_histposr = my_mod((ns9_histposr + 8), ns9_order);
		ns9_ehr[ns9_histposr + 9] = ns9_ehr[ns9_histposr] = output - sample;
		lp[i] = output;
	}
}

static inline unsigned long frand(void)
{
	return genrand_int32();
}

static inline int32 my_mod(int32 x, int32 n)
{
	if (x > n)
		x -= n;
	return x;
}

