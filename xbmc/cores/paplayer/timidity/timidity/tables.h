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

    tables.h
*/

#ifndef ___TABLES_H_
#define ___TABLES_H_

#ifdef LOOKUP_SINE
extern FLOAT_T lookup_sine(int x);
#else
#include <math.h>
#define lookup_sine(x) (sin((2*M_PI/1024.0) * (x)))
#endif
extern FLOAT_T lookup_triangular(int x);
extern FLOAT_T lookup_log(int x);

#define SINE_CYCLE_LENGTH 1024
extern int32 freq_table[];
extern int32 freq_table_zapped[];
extern int32 freq_table_tuning[][128];
extern int32 freq_table_pytha[][128];
extern int32 freq_table_meantone[][128];
extern int32 freq_table_pureint[][128];
extern int32 freq_table_user[][48][128];
extern FLOAT_T *vol_table;
extern FLOAT_T def_vol_table[];
extern FLOAT_T gs_vol_table[];
extern FLOAT_T *xg_vol_table; /* == gs_vol_table */
extern FLOAT_T *pan_table;
extern FLOAT_T bend_fine[];
extern FLOAT_T bend_coarse[];
extern FLOAT_T midi_time_table[], midi_time_table2[];
#ifdef LOOKUP_HACK
extern uint8 *_l2u; /* 13-bit PCM to 8-bit u-law */
extern uint8 _l2u_[]; /* used in LOOKUP_HACK */
extern int16 _u2l[];
extern int32 *mixup;
#ifdef LOOKUP_INTERPOLATION
extern int8 *iplookup;
#endif
#endif
extern uint8 reverb_macro_presets[];
extern uint8 chorus_macro_presets[];
extern uint8 delay_macro_presets[];
extern float delay_time_center_table[];
extern float pre_delay_time_table[];
extern float chorus_delay_time_table[];
extern float rate1_table[];
extern FLOAT_T attack_vol_table[];
extern FLOAT_T perceived_vol_table[];
extern FLOAT_T gm2_vol_table[];
extern FLOAT_T user_vol_table[];
extern float sc_eg_attack_table[];
extern float sc_eg_decay_table[];
extern float sc_eg_release_table[];
extern FLOAT_T sc_vel_table[];
extern FLOAT_T sc_vol_table[];
extern FLOAT_T sc_pan_table[], gm2_pan_table[];
extern FLOAT_T sc_drum_level_table[];
extern FLOAT_T sb_vol_table[];
extern FLOAT_T modenv_vol_table[];
extern float cb_to_amp_table[];
extern float reverb_time_table[];
extern float pan_delay_table[];
extern float chamberlin_filter_db_to_q_table[];
extern uint8 multi_eq_block_table_xg[];
extern float eq_freq_table_xg[];
extern float lfo_freq_table_xg[];
extern float mod_delay_offset_table_xg[];
extern float reverb_time_table_xg[];
extern float delay_time_table_xg[];
extern int16 cutoff_freq_table_gs[];
extern int16 lpf_table_gs[];
extern int16 eq_freq_table_gs[];
extern float lofi_sampling_freq_table_xg[];

extern void init_freq_table(void);
extern void init_freq_table_tuning(void);
extern void init_freq_table_pytha(void);
extern void init_freq_table_meantone(void);
extern void init_freq_table_pureint(void);
extern void init_freq_table_user(void);
extern void init_bend_fine(void);
extern void init_bend_coarse(void);
extern void init_tables(void);
extern void init_gm2_pan_table(void);
extern void init_attack_vol_table(void);
extern void init_sb_vol_table(void);
extern void init_modenv_vol_table(void);
extern void init_def_vol_table(void);
extern void init_gs_vol_table(void);
extern void init_perceived_vol_table(void);
extern void init_gm2_vol_table(void);
extern void init_user_vol_table(FLOAT_T power);

#endif /* ___TABLES_H_ */
