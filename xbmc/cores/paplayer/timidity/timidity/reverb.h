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
 * reverb.h
 *
 */
#ifndef ___REVERB_H_
#define ___REVERB_H_

#define DEFAULT_REVERB_SEND_LEVEL 40

extern int opt_reverb_control;

extern void set_dry_signal(int32 *, int32);
extern void set_dry_signal_xg(int32 *, int32, int32);
extern void mix_dry_signal(int32 *, int32);
extern void free_effect_buffers(void);

/*                    */
/*  Effect Utitities  */
/*                    */
/*! simple delay */
typedef struct {
	int32 *buf, size, index;
} delay;

/*! Pink Noise Generator */
typedef struct {
	float b0, b1, b2, b3, b4, b5, b6;
} pink_noise;

extern void init_pink_noise(pink_noise *);
extern float get_pink_noise(pink_noise *);
extern float get_pink_noise_light(pink_noise *);

#ifndef SINE_CYCLE_LENGTH
#define SINE_CYCLE_LENGTH 1024
#endif

/*! LFO */
typedef struct {
	int32 buf[SINE_CYCLE_LENGTH];
	int32 count, cycle;	/* in samples */
	int32 icycle;	/* proportional to (SINE_CYCLE_LENGTH / cycle) */
	int type;	/* current content of its buffer */
	double freq;	/* in Hz */
} lfo;

enum {
	LFO_NONE = 0,
	LFO_SINE,
	LFO_TRIANGULAR,
};

/*! modulated delay with allpass interpolation */
typedef struct {
	int32 *buf, size, rindex, windex, hist;
	int32 ndelay, depth;	/* in samples */
} mod_delay;

/*! modulated allpass filter with allpass interpolation */
typedef struct {
	int32 *buf, size, rindex, windex, hist;
	int32 ndelay, depth;	/* in samples */
	double feedback;
	int32 feedbacki;
} mod_allpass;

/*! Moog VCF (resonant IIR state variable filter) */
typedef struct {
	int16 freq, last_freq;	/* in Hz */
	double res_dB, last_res_dB; /* in dB */
	int32 f, q, p;	/* coefficients in fixed-point */
	int32 b0, b1, b2, b3, b4;
} filter_moog;

/*! Moog VCF (resonant IIR state variable filter with distortion) */
typedef struct {
	int16 freq, last_freq;	/* in Hz */
	double res_dB, last_res_dB; /* in dB */
	double dist, last_dist, f, q, p, d, b0, b1, b2, b3, b4;
} filter_moog_dist;

/*! LPF18 (resonant IIR lowpass filter with waveshaping) */
typedef struct {
	int16 freq, last_freq;	/* in Hz */
	double dist, res, last_dist, last_res; /* in linear */
	double ay1, ay2, aout, lastin, kres, value, kp, kp1h;
} filter_lpf18;

/*! 1st order lowpass filter */
typedef struct {
	double a;
	int32 ai, iai;	/* coefficients in fixed-point */
	int32 x1l, x1r;
} filter_lowpass1;

extern void init_filter_lowpass1(filter_lowpass1 *);

/*! lowpass / highpass filter */
typedef struct {
	double freq, q, last_freq, last_q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 a1, a2, b1, b02;
} filter_biquad;

#ifndef PART_EQ_XG
#define PART_EQ_XG
/*! shelving filter */
typedef struct {
	double freq, gain, q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 a1, a2, b0, b1, b2;
} filter_shelving;

struct part_eq_xg {
	int8 bass, treble, bass_freq, treble_freq;
	filter_shelving basss, trebles;
	int8 valid;
};
#endif /* PART_EQ_XG */

extern void calc_filter_shelving_high(filter_shelving *);
extern void calc_filter_shelving_low(filter_shelving *);

/*! peaking filter */
typedef struct {
	double freq, gain, q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 ba1, a2, b0, b2;
} filter_peaking;

extern void calc_filter_peaking(filter_peaking *);

/*! allpass filter */
typedef struct _allpass {
	int32 *buf, size, index;
	double feedback;
	int32 feedbacki;
} allpass;

/*! comb filter */
typedef struct _comb {
	int32 *buf, filterstore, size, index;
	double feedback, damp1, damp2;
	int32 feedbacki, damp1i, damp2i;
} comb;

/*                                  */
/*  Insertion and Variation Effect  */
/*                                  */
struct effect_xg_t {
	int8 use_msb, type_msb, type_lsb, param_lsb[16], param_msb[10],
		ret, pan, send_reverb, send_chorus, connection, part,
		mw_depth, bend_depth, cat_depth, ac1_depth, ac2_depth, cbc1_depth,
		cbc2_depth;
	struct _EffectList *ef;
};

extern void do_insertion_effect_gs(int32*, int32);
extern void do_insertion_effect_xg(int32*, int32, struct effect_xg_t *);
extern void do_variation_effect1_xg(int32*, int32);
extern void init_ch_effect_xg(void);

enum {
	EFFECT_NONE,
	EFFECT_EQ2,
	EFFECT_EQ3,
	EFFECT_STEREO_EQ,
	EFFECT_OVERDRIVE1,
	EFFECT_DISTORTION1,
	EFFECT_OD1OD2,
	EFFECT_CHORUS,	
	EFFECT_FLANGER,
	EFFECT_SYMPHONIC,
	EFFECT_CHORUS_EQ3,
	EFFECT_STEREO_OVERDRIVE,
	EFFECT_STEREO_DISTORTION,
	EFFECT_STEREO_AMP_SIMULATOR,
	EFFECT_OD_EQ3,
	EFFECT_HEXA_CHORUS,
	EFFECT_DELAY_LCR,
	EFFECT_DELAY_LR,
	EFFECT_ECHO,
	EFFECT_CROSS_DELAY,
	EFFECT_DELAY_EQ2,
	EFFECT_LOFI,
	EFFECT_LOFI1,
	EFFECT_LOFI2,
	EFFECT_XG_AUTO_WAH,
	EFFECT_XG_AUTO_WAH_EQ2,
	EFFECT_XG_AUTO_WAH_OD,
	EFFECT_XG_AUTO_WAH_OD_EQ3,
};

#define MAGIC_INIT_EFFECT_INFO -1
#define MAGIC_FREE_EFFECT_INFO -2

struct insertion_effect_gs_t {
	int32 type;
	int8 type_lsb, type_msb, parameter[20], send_reverb,
		send_chorus, send_delay, control_source1, control_depth1,
		control_source2, control_depth2, send_eq_switch;
	struct _EffectList *ef;
} insertion_effect_gs;

enum {
	XG_CONN_INSERTION = 0,
	XG_CONN_SYSTEM = 1,
	XG_CONN_SYSTEM_CHORUS,
	XG_CONN_SYSTEM_REVERB,
};

#define XG_INSERTION_EFFECT_NUM 2
#define XG_VARIATION_EFFECT_NUM 1

struct effect_xg_t insertion_effect_xg[XG_INSERTION_EFFECT_NUM],
	variation_effect_xg[XG_VARIATION_EFFECT_NUM], reverb_status_xg, chorus_status_xg;

typedef struct _EffectList {
	int type;
	void *info;
	struct _EffectEngine *engine;
	struct _EffectList *next_ef;
} EffectList;

struct _EffectEngine {
	int type;
	char *name;
	void (*do_effect)(int32 *, int32, struct _EffectList *);
	void (*conv_gs)(struct insertion_effect_gs_t *, struct _EffectList *);
	void (*conv_xg)(struct effect_xg_t *, struct _EffectList *);
	int info_size;
};

extern struct _EffectEngine effect_engine[];

struct effect_parameter_gs_t {
	int8 type_msb, type_lsb;
	char *name;
	int8 param[20];
	int8 control1, control2;
};

extern struct effect_parameter_gs_t effect_parameter_gs[];

struct effect_parameter_xg_t {
	int8 type_msb, type_lsb;
	char *name;
	int8 param_msb[10], param_lsb[16];
	int8 control;
};

extern struct effect_parameter_xg_t effect_parameter_xg[];

extern EffectList *push_effect(EffectList *, int);
extern void do_effect_list(int32 *, int32, EffectList *);
extern void free_effect_list(EffectList *);

/*! 2-Band EQ */
typedef struct {
    int16 low_freq, high_freq;		/* in Hz */
	int16 low_gain, high_gain;		/* in dB */
	filter_shelving hsf, lsf;
} InfoEQ2;

/*! 3-Band EQ */
typedef struct {
    int16 low_freq, high_freq, mid_freq;		/* in Hz */
	int16 low_gain, high_gain, mid_gain;		/* in dB */
	double mid_width;
	filter_shelving hsf, lsf;
	filter_peaking peak;
} InfoEQ3;

/*! Stereo EQ */
typedef struct {
    int16 low_freq, high_freq, m1_freq, m2_freq;		/* in Hz */
	int16 low_gain, high_gain, m1_gain, m2_gain;		/* in dB */
	double m1_q, m2_q, level;
	int32 leveli;
	filter_shelving hsf, lsf;
	filter_peaking m1, m2;
} InfoStereoEQ;

/*! Overdrive 1 / Distortion 1 */
typedef struct {
	double level;
	int32 leveli, di;	/* in fixed-point */
	int8 drive, pan, amp_sw, amp_type;
	filter_moog svf;
	filter_biquad lpf1;
	void (*amp_sim)(int32 *, int32);
} InfoOverdrive1;

/*! OD1 / OD2 */
typedef struct {
	double level, levell, levelr;
	int32 levelli, levelri, dli, dri;	/* in fixed-point */
	int8 drivel, driver, panl, panr, typel, typer, amp_swl, amp_swr, amp_typel, amp_typer;
	filter_moog svfl, svfr;
	filter_biquad lpf1;
	void (*amp_siml)(int32 *, int32), (*amp_simr)(int32 *, int32);
	void (*odl)(int32 *, int32), (*odr)(int32 *, int32);
} InfoOD1OD2;

/*! HEXA-CHORUS */
typedef struct {
	delay buf0;
	lfo lfo0;
	double dry, wet, level;
	int32 pdelay, depth;	/* in samples */
	int8 pdelay_dev, depth_dev, pan_dev;
	int32 dryi, weti;	/* in fixed-point */
	int32 pan0, pan1, pan2, pan3, pan4, pan5;
	int32 depth0, depth1, depth2, depth3, depth4, depth5,
		pdelay0, pdelay1, pdelay2, pdelay3, pdelay4, pdelay5;
	int32 spt0, spt1, spt2, spt3, spt4, spt5,
		hist0, hist1, hist2, hist3, hist4, hist5;
} InfoHexaChorus;

/*! Plate Reverb */
typedef struct {
	delay pd, od1l, od2l, od3l, od4l, od5l, od6l, od7l,
		od1r, od2r, od3r, od4r, od5r, od6r, od7r,
		td1, td2, td1d, td2d;
	lfo lfo1, lfo1d;
	allpass ap1, ap2, ap3, ap4, ap6, ap6d;
	mod_allpass ap5, ap5d;
	filter_lowpass1 lpf1, lpf2;
	int32 t1, t1d;
	double decay, ddif1, ddif2, idif1, idif2, dry, wet;
	int32 decayi, ddif1i, ddif2i, idif1i, idif2i, dryi, weti;
} InfoPlateReverb;

/*! Standard Reverb */
typedef struct {
	int32 spt0, spt1, spt2, spt3, rpt0, rpt1, rpt2, rpt3;
	int32 ta, tb, HPFL, HPFR, LPFL, LPFR, EPFL, EPFR;
	delay buf0_L, buf0_R, buf1_L, buf1_R, buf2_L, buf2_R, buf3_L, buf3_R;
	double fbklev, nmixlev, cmixlev, monolev, hpflev, lpflev, lpfinp, epflev, epfinp, width, wet;
	int32 fbklevi, nmixlevi, cmixlevi, monolevi, hpflevi, lpflevi, lpfinpi, epflevi, epfinpi, widthi, weti;
} InfoStandardReverb;

/*! Freeverb */
#define numcombs 8
#define numallpasses 4

typedef struct {
	delay pdelay;
	double roomsize, roomsize1, damp, damp1, wet, wet1, wet2, width;
	comb combL[numcombs], combR[numcombs];
	allpass allpassL[numallpasses], allpassR[numallpasses];
	int32 wet1i, wet2i;
	int8 alloc_flag;
} InfoFreeverb;

/*! 3-Tap Stereo Delay Effect */
typedef struct {
	delay delayL, delayR;
	int32 size[3], index[3];
	double level[3], feedback, send_reverb;
	int32 leveli[3], feedbacki, send_reverbi;
} InfoDelay3;

/*! Stereo Chorus Effect */
typedef struct {
	delay delayL, delayR;
	lfo lfoL, lfoR;
	int32 wpt0, spt0, spt1, hist0, hist1;
	int32 rpt0, depth, pdelay;
	double level, feedback, send_reverb, send_delay;
	int32 leveli, feedbacki, send_reverbi, send_delayi;
} InfoStereoChorus;

/*! Chorus */
typedef struct {
	delay delayL, delayR;
	lfo lfoL, lfoR;
	int32 wpt0, spt0, spt1, hist0, hist1;
	int32 rpt0, depth, pdelay;
	double dry, wet, feedback, pdelay_ms, depth_ms, rate, phase_diff;
	int32 dryi, weti, feedbacki;
} InfoChorus;

/*! Stereo Overdrive / Distortion */
typedef struct {
	double level, dry, wet, drive, cutoff;
	int32 dryi, weti, di;
	filter_moog svfl, svfr;
	filter_biquad lpf1;
	void (*od)(int32 *, int32);
} InfoStereoOD;

/*! Delay L,C,R */
typedef struct {
	delay delayL, delayR;
	int32 index[3], size[3];	/* L,C,R */
	double rdelay, ldelay, cdelay, fdelay;	/* in ms */
	double dry, wet, feedback, clevel, high_damp;
	int32 dryi, weti, feedbacki, cleveli;
	filter_lowpass1 lpf;
} InfoDelayLCR;

/*! Delay L,R */
typedef struct {
	delay delayL, delayR;
	int32 index[2], size[2];	/* L,R */
	double rdelay, ldelay, fdelay1, fdelay2;	/* in ms */
	double dry, wet, feedback, high_damp;
	int32 dryi, weti, feedbacki;
	filter_lowpass1 lpf;
} InfoDelayLR;

/*! Echo */
typedef struct {
	delay delayL, delayR;
	int32 index[2], size[2];	/* L1,R1 */
	double rdelay1, ldelay1, rdelay2, ldelay2;	/* in ms */
	double dry, wet, lfeedback, rfeedback, high_damp, level;
	int32 dryi, weti, lfeedbacki, rfeedbacki, leveli;
	filter_lowpass1 lpf;
} InfoEcho;

/*! Cross Delay */
typedef struct {
	delay delayL, delayR;
	double lrdelay, rldelay;	/* in ms */
	double dry, wet, feedback, high_damp;
	int32 dryi, weti, feedbacki, input_select;
	filter_lowpass1 lpf;
} InfoCrossDelay;

/*! Lo-Fi 1 */
typedef struct {
	int8 lofi_type, pan, pre_filter, post_filter;
	double level, dry, wet;
	int32 bit_mask, dryi, weti;
	filter_biquad pre_fil, post_fil;
} InfoLoFi1;

/*! Lo-Fi 2 */
typedef struct {
	int8 wp_sel, disc_type, hum_type, ms, pan, rdetune, lofi_type, fil_type;
	double wp_level, rnz_lev, discnz_lev, hum_level, dry, wet, level;
	int32 bit_mask, wp_leveli, rnz_levi, discnz_levi, hum_keveki, dryi, weti;
	filter_biquad fil, wp_lpf, hum_lpf, disc_lpf;
} InfoLoFi2;

/*! LO-FI */
typedef struct {
	int8 output_gain, word_length, filter_type, bit_assign, emphasis;
	double dry, wet;
	int32 bit_mask, dryi, weti;
	filter_biquad lpf, srf;
} InfoLoFi;

/*! XG: Auto Wah */
typedef struct {
	int8 lfo_depth, drive;
	double resonance, lfo_freq, offset_freq, dry, wet;
	int32 dryi, weti, fil_count, fil_cycle;
	lfo lfo;
	filter_moog_dist fil0, fil1;
} InfoXGAutoWah;

typedef struct {
	double level;
	int32 leveli;
	filter_biquad lpf;
} InfoXGAutoWahOD;

/*                             */
/*        System Effect        */
/*                             */
/* Reverb Effect */
extern void do_ch_reverb(int32 *, int32);
extern void do_mono_reverb(int32 *, int32);
extern void set_ch_reverb(int32 *, int32, int32);
extern void init_reverb(void);
extern void reverb_rc_event(int, int32);
extern void do_ch_reverb_xg(int32 *, int32);

/* Chorus Effect */
extern void do_ch_chorus(int32 *, int32);
extern void set_ch_chorus(int32 *, int32, int32);
extern void init_ch_chorus(void);
extern void do_ch_chorus_xg(int32 *, int32);

/* Delay (Celeste) Effect */
extern void do_ch_delay(int32 *, int32);
extern void set_ch_delay(int32 *, int32, int32);
extern void init_ch_delay(void);

/* EQ */
extern void init_eq_gs(void);
extern void set_ch_eq_gs(int32 *, int32);
extern void do_ch_eq_gs(int32 *, int32);
extern void do_ch_eq_xg(int32 *, int32, struct part_eq_xg *); 
extern void do_multi_eq_xg(int32 *, int32);

/* GS parameters of reverb effect */
struct reverb_status_gs_t
{
	/* GS parameters */
	int8 character, pre_lpf, level, time, delay_feedback, pre_delay_time;

	InfoStandardReverb info_standard_reverb;
	InfoPlateReverb info_plate_reverb;
	InfoFreeverb info_freeverb;
	InfoDelay3 info_reverb_delay;
	filter_lowpass1 lpf;
} reverb_status_gs;

struct chorus_text_gs_t
{
    int status;
    uint8 voice_reserve[18], macro[3], pre_lpf[3], level[3], feed_back[3],
		delay[3], rate[3], depth[3], send_level[3];
};

/* GS parameters of chorus effect */
struct chorus_status_gs_t
{
	/* GS parameters */
	int8 macro, pre_lpf, level, feedback, delay, rate, depth, send_reverb, send_delay;

	struct chorus_text_gs_t text;

	InfoStereoChorus info_stereo_chorus;
	filter_lowpass1 lpf;
} chorus_status_gs;

/* GS parameters of delay effect */
struct delay_status_gs_t
{
	/* GS parameters */
	int8 type, level, level_center, level_left, level_right,
		feedback, pre_lpf, send_reverb, time_c, time_l, time_r;
    double time_center;			/* in ms */
    double time_ratio_left, time_ratio_right;		/* in pct */

	/* for pre-calculation */
	int32 sample_c, sample_l, sample_r;
	double level_ratio_c, level_ratio_l, level_ratio_r,
		feedback_ratio, send_reverb_ratio;

	filter_lowpass1 lpf;
	InfoDelay3 info_delay;
} delay_status_gs;

/* GS parameters of channel EQ */
struct eq_status_gs_t
{
	/* GS parameters */
    int8 low_freq, high_freq, low_gain, high_gain;

	filter_shelving hsf, lsf;
} eq_status_gs;

/* XG parameters of Multi EQ */
struct multi_eq_xg_t
{
	/* XG parameters */
	int8 type, gain1, gain2, gain3, gain4, gain5,
		freq1, freq2, freq3, freq4, freq5,
		q1, q2, q3, q4, q5, shape1, shape5;

	int8 valid, valid1, valid2, valid3, valid4, valid5;
	filter_shelving eq1s, eq5s;
	filter_peaking eq1p, eq2p, eq3p, eq4p, eq5p;
} multi_eq_xg;

pink_noise global_pink_noise_light;

#endif /* ___REVERB_H_ */
