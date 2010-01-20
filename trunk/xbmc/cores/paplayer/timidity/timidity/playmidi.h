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

    playmidi.h
*/

#ifndef ___PLAYMIDI_H_
#define ___PLAYMIDI_H_

typedef struct {
  int32 time;
  uint8 type, channel, a, b;
} MidiEvent;


typedef struct
{
	// Total number of samples
	int32 samples;

	// Chain of MIDI events
	MidiEvent *events;

	// During buffer filling the audio data should be flushed in the output_buffer, which is guaranteed
	// to have room for output_size bytes. output_offset represents current offset. This buffer is NOT allocated.
	void 		* output_buffer;
	unsigned int  output_size;
	unsigned int  output_offset;

	// During buffer filling the flushed audio stream may exceed the requested buffer size.
	// In this case we will store the extra data in the following buffer (and assotiate size),
	// and will return in in the next Fill cycle. This buffer IS allocated if not null.
	void 		* stored_buffer;
	unsigned int  stored_size;

	int	          end_of_song_reached; // 0 if end-of-song is not reached

} MidiSong;

// buffer_a.c
void outbuf_set_data( MidiSong * song );

#define REVERB_MAX_DELAY_OUT (4 * play_mode->rate)

#define MIDI_EVENT_NOTE(ep) (ISDRUMCHANNEL((ep)->channel) ? (ep)->a : \
			     (((int)(ep)->a + note_key_offset + \
			       channel[ep->channel].key_shift) & 0x7f))

#define MIDI_EVENT_TIME(ep) ((int32)((ep)->time * midi_time_ratio + 0.5))

#define SYSEX_TAG 0xFF

/* Midi events */
enum midi_event_t
{
	ME_NONE,
	
	/* MIDI events */
	ME_NOTEOFF,
	ME_NOTEON,
	ME_KEYPRESSURE,
	ME_PROGRAM,
	ME_CHANNEL_PRESSURE,
	ME_PITCHWHEEL,
	
	/* Controls */
	ME_TONE_BANK_MSB,
	ME_TONE_BANK_LSB,
	ME_MODULATION_WHEEL,
	ME_BREATH,
	ME_FOOT,
	ME_MAINVOLUME,
	ME_BALANCE,
	ME_PAN,
	ME_EXPRESSION,
	ME_SUSTAIN,
	ME_PORTAMENTO_TIME_MSB,
	ME_PORTAMENTO_TIME_LSB,
	ME_PORTAMENTO,
	ME_PORTAMENTO_CONTROL,
	ME_DATA_ENTRY_MSB,
	ME_DATA_ENTRY_LSB,
	ME_SOSTENUTO,
	ME_SOFT_PEDAL,
	ME_LEGATO_FOOTSWITCH,
	ME_HOLD2,
	ME_HARMONIC_CONTENT,
	ME_RELEASE_TIME,
	ME_ATTACK_TIME,
	ME_BRIGHTNESS,
	ME_REVERB_EFFECT,
	ME_TREMOLO_EFFECT,
	ME_CHORUS_EFFECT,
	ME_CELESTE_EFFECT,
	ME_PHASER_EFFECT,
	ME_RPN_INC,
	ME_RPN_DEC,
	ME_NRPN_LSB,
	ME_NRPN_MSB,
	ME_RPN_LSB,
	ME_RPN_MSB,
	ME_ALL_SOUNDS_OFF,
	ME_RESET_CONTROLLERS,
	ME_ALL_NOTES_OFF,
	ME_MONO,
	ME_POLY,
	
	/* TiMidity Extensionals */
#if 0
	ME_VOLUME_ONOFF,		/* Not supported */
#endif
	ME_SCALE_TUNING,		/* Scale tuning */
	ME_BULK_TUNING_DUMP,	/* Bulk tuning dump */
	ME_SINGLE_NOTE_TUNING,	/* Single-note tuning */
	ME_RANDOM_PAN,
	ME_SET_PATCH,			/* Install special instrument */
	ME_DRUMPART,
	ME_KEYSHIFT,
	ME_PATCH_OFFS,			/* Change special instrument sample position
							 * Channel, LSB, MSB
							 */
	
	/* Global channel events */
	ME_TEMPO,
	ME_CHORUS_TEXT,
	ME_LYRIC,
	ME_GSLCD,				/* GS L.C.D. Exclusive message event */
	ME_MARKER,
	ME_INSERT_TEXT,			/* for SC */
	ME_TEXT,
	ME_KARAOKE_LYRIC,		/* for KAR format */
	ME_MASTER_VOLUME,
	ME_RESET,				/* Reset and change system mode */
	ME_NOTE_STEP,
	
	ME_TIMESIG,				/* Time signature */
	ME_KEYSIG,				/* Key signature */
	ME_TEMPER_KEYSIG,		/* Temperament key signature */
	ME_TEMPER_TYPE,			/* Temperament type */
	ME_MASTER_TEMPER_TYPE,	/* Master temperament type */
	ME_USER_TEMPER_ENTRY,	/* User-defined temperament entry */
	
	ME_SYSEX_LSB,			/* Universal system exclusive message (LSB) */
	ME_SYSEX_MSB,			/* Universal system exclusive message (MSB) */
	ME_SYSEX_GS_LSB,		/* GS system exclusive message (LSB) */
	ME_SYSEX_GS_MSB,		/* GS system exclusive message (MSB) */
	ME_SYSEX_XG_LSB,		/* XG system exclusive message (LSB) */
	ME_SYSEX_XG_MSB,		/* XG system exclusive message (MSB) */
	
	ME_WRD,					/* for MIMPI WRD tracer */
	ME_SHERRY,				/* for Sherry WRD tracer */
	ME_BARMARKER,
	ME_STEP,				/* for Metronome */
	
	ME_LAST = 254,			/* Last sequence of MIDI list.
							 * This event is reserved for realtime player.
							 */
	ME_EOT = 255			/* End of MIDI.  Finish to play */
};

#define GLOBAL_CHANNEL_EVENT_TYPE(type)	\
	((type) == ME_NONE || (type) >= ME_TEMPO)

enum rpn_data_address_t /* NRPN/RPN */
{
    NRPN_ADDR_0108,
    NRPN_ADDR_0109,
    NRPN_ADDR_010A,
    NRPN_ADDR_0120,
    NRPN_ADDR_0121,
	NRPN_ADDR_0130,
	NRPN_ADDR_0131,
	NRPN_ADDR_0134,
	NRPN_ADDR_0135,
    NRPN_ADDR_0163,
    NRPN_ADDR_0164,
    NRPN_ADDR_0166,
    NRPN_ADDR_1400,
    NRPN_ADDR_1500,
    NRPN_ADDR_1600,
    NRPN_ADDR_1700,
    NRPN_ADDR_1800,
    NRPN_ADDR_1900,
    NRPN_ADDR_1A00,
    NRPN_ADDR_1C00,
    NRPN_ADDR_1D00,
    NRPN_ADDR_1E00,
    NRPN_ADDR_1F00,
    NRPN_ADDR_3000,
    NRPN_ADDR_3100,
    NRPN_ADDR_3400,
    NRPN_ADDR_3500,
    RPN_ADDR_0000,
    RPN_ADDR_0001,
    RPN_ADDR_0002,
    RPN_ADDR_0003,
    RPN_ADDR_0004,
	RPN_ADDR_0005,
    RPN_ADDR_7F7F,
    RPN_ADDR_FFFF,
    RPN_MAX_DATA_ADDR
};

#define RX_PITCH_BEND (1<<0)
#define RX_CH_PRESSURE (1<<1)
#define RX_PROGRAM_CHANGE (1<<2)
#define RX_CONTROL_CHANGE (1<<3)
#define RX_POLY_PRESSURE (1<<4)
#define RX_NOTE_MESSAGE (1<<5)
#define RX_RPN (1<<6)
#define RX_NRPN (1<<7)
#define RX_MODULATION (1<<8)
#define RX_VOLUME (1<<9)
#define RX_PANPOT (1<<10)
#define RX_EXPRESSION (1<<11)
#define RX_HOLD1 (1<<12)
#define RX_PORTAMENTO (1<<13)
#define RX_SOSTENUTO (1<<14)
#define RX_SOFT (1<<15)
#define RX_NOTE_ON (1<<16)
#define RX_NOTE_OFF (1<<17)
#define RX_BANK_SELECT (1<<18)
#define RX_BANK_SELECT_LSB (1<<19)

enum {
	EG_ATTACK = 0,
	EG_DECAY = 2,
	EG_DECAY1 = 1,
	EG_DECAY2 = 2,
	EG_RELEASE = 3,
	EG_NULL = 5,
	EG_GUS_ATTACK = 0,
	EG_GUS_DECAY = 1,
	EG_GUS_SUSTAIN = 2,
	EG_GUS_RELEASE1 = 3,
	EG_GUS_RELEASE2 = 4,
	EG_GUS_RELEASE3 = 5,
	EG_SF_ATTACK = 0,
	EG_SF_HOLD = 1,
	EG_SF_DECAY = 2,
	EG_SF_RELEASE = 3,
};

#ifndef PART_EQ_XG
#define PART_EQ_XG
/*! shelving filter */
typedef struct {
	double freq, gain, q;
	int32 x1l, x2l, y1l, y2l, x1r, x2r, y1r, y2r;
	int32 a1, a2, b0, b1, b2;
} filter_shelving;

/*! Part EQ (XG) */
struct part_eq_xg {
	int8 bass, treble, bass_freq, treble_freq;
	filter_shelving basss, trebles;
	int8 valid;
};
#endif /* PART_EQ_XG */

typedef struct {
  int16 val;
  int8 pitch;	/* in +-semitones [-24, 24] */
  int16 cutoff;	/* in +-cents [-9600, 9600] */
  float amp;	/* [-1.0, 1.0] */
  /* in GS, LFO1 means LFO for voice 1, LFO2 means LFO for voice2.
     LFO2 is not supported. */
  float lfo1_rate, lfo2_rate;	/* in +-Hz [-10.0, 10.0] */
  int16 lfo1_pitch_depth, lfo2_pitch_depth;	/* in cents [0, 600] */
  int16 lfo1_tvf_depth, lfo2_tvf_depth;	/* in cents [0, 2400] */
  float lfo1_tva_depth, lfo2_tva_depth;	/* [0, 1.0] */
  int8 variation_control_depth, insertion_control_depth;
} midi_controller;

struct DrumPartEffect
{
	int32 *buf;
	int8 note, reverb_send, chorus_send, delay_send;
};

struct DrumParts
{
    int8 drum_panning;
    int32 drum_envelope_rate[6]; /* drum instrument envelope */
    int8 pan_random;    /* flag for drum random pan */
	float drum_level;

	int8 chorus_level, reverb_level, delay_level, coarse, fine,
		play_note, drum_cutoff_freq, drum_resonance;
	int32 rx;
};

typedef struct {
  int8	bank_msb, bank_lsb, bank, program, volume,
	expression, sustain, panning, mono, portamento,
	key_shift, loop_timeout;

  /* chorus, reverb... Coming soon to a 300-MHz, eight-way superscalar
     processor near you */
  int8	chorus_level,	/* Chorus level */
	reverb_level;	/* Reverb level. */
  int	reverb_id;	/* Reverb ID used for reverb optimize implementation
			   >=0 reverb_level
			   -1: DEFAULT_REVERB_SEND_LEVEL
			   */
  int8 delay_level;	/* Delay Send Level */
  int8 eq_gs;	/* EQ ON/OFF (GS) */
  int8 insertion_effect;

  /* Special sample ID. (0 means Normal sample) */
  uint8 special_sample;

  int pitchbend;

  FLOAT_T
    pitchfactor; /* precomputed pitch bend factor to save some fdiv's */

  /* For portamento */
  uint8 portamento_time_msb, portamento_time_lsb;
  int porta_control_ratio, porta_dpb;
  int32 last_note_fine;

  /* For Drum part */
  struct DrumParts *drums[128];

  /* For NRPN Vibrato */
  int32 vibrato_depth, vibrato_delay;
  float vibrato_ratio;

  /* For RPN */
  uint8 rpnmap[RPN_MAX_DATA_ADDR]; /* pseudo RPN address map */
  uint8 rpnmap_lsb[RPN_MAX_DATA_ADDR];
  uint8 lastlrpn, lastmrpn;
  int8  nrpn; /* 0:RPN, 1:NRPN, -1:Undefined */
  int rpn_7f7f_flag;		/* Boolean flag used for RPN 7F/7F */

  /* For channel envelope */
  int32 envelope_rate[6]; /* for Envelope Generator in mix.c
			   * 0: value for attack rate
			   * 2: value for decay rate
			   * 3: value for release rate
			   */

  int mapID;			/* Program map ID */
  AlternateAssign *altassign;	/* Alternate assign patch table */
  int32 lasttime;     /* Last sample time of computed voice on this channel */

  /* flag for random pan */
  int pan_random;

  /* for Voice LPF / Resonance */
  int8 param_resonance, param_cutoff_freq;	/* -64 ~ 63 */
  float cutoff_freq_coef, resonance_dB;

  int8 velocity_sense_depth, velocity_sense_offset;
  
  int8 scale_tuning[12], prev_scale_tuning;
  int8 temper_type;

  int8 soft_pedal;
  int8 sostenuto;
  int8 damper_mode;

  int8 tone_map0_number;
  FLOAT_T pitch_offset_fine;	/* in Hz */
  int8 assign_mode;

  int8 legato;	/* legato footswitch */
  int8 legato_flag;	/* note-on flag for legato */

  midi_controller mod, bend, caf, paf, cc1, cc2;

  ChannelBitMask channel_layer;
  int port_select;

  struct part_eq_xg eq_xg;

  int8 dry_level;
  int8 note_limit_high, note_limit_low;	/* Note Limit (Keyboard Range) */
  int8 vel_limit_high, vel_limit_low;	/* Velocity Limit */
  int32 rx;	/* Rx. ~ (Rcv ~) */

  int drum_effect_num;
  int8 drum_effect_flag;
  struct DrumPartEffect *drum_effect;

  int8 sysex_gs_msb_addr, sysex_gs_msb_val,
		sysex_xg_msb_addr, sysex_xg_msb_val, sysex_msb_addr, sysex_msb_val;
} Channel;

/* Causes the instrument's default panning to be used. */
#define NO_PANNING -1

typedef struct {
	int16 freq, last_freq, orig_freq;
	double reso_dB, last_reso_dB, orig_reso_dB, reso_lin; 
	int8 type;	/* filter type. 0: Off, 1: 12dB/oct, 2: 24dB/oct */ 
	int32 f, q, p;	/* coefficients in fixed-point */
	int32 b0, b1, b2, b3, b4;
	float gain;
	int8 start_flag;
} FilterCoefficients;

#define ENABLE_PAN_DELAY
#ifdef ENABLE_PAN_DELAY
#define PAN_DELAY_BUF_MAX 48	/* 0.5ms in 96kHz */
#endif	/* ENABLE_PAN_DELAY */

typedef struct {
  uint8
    status, channel, note, velocity;
  int vid, temper_instant;
  Sample *sample;
#if SAMPLE_LENGTH_BITS == 32 && TIMIDITY_HAVE_INT64
  int64 sample_offset;	/* sample_offset must be signed */
#else
  splen_t sample_offset;
#endif
  int32
    orig_frequency, frequency, sample_increment,
    envelope_volume, envelope_target, envelope_increment,
    tremolo_sweep, tremolo_sweep_position,
    tremolo_phase, tremolo_phase_increment,
    vibrato_sweep, vibrato_sweep_position;

  final_volume_t left_mix, right_mix;
#ifdef SMOOTH_MIXING
  int32 old_left_mix, old_right_mix,
     left_mix_offset, right_mix_offset,
     left_mix_inc, right_mix_inc;
#endif

  FLOAT_T
    left_amp, right_amp, tremolo_volume;
  int32
    vibrato_sample_increment[VIBRATO_SAMPLE_INCREMENTS], vibrato_delay;
  int
	vibrato_phase, orig_vibrato_control_ratio, vibrato_control_ratio,
    vibrato_depth, vibrato_control_counter,
    envelope_stage, control_counter, panning, panned;
  int16 tremolo_depth;

  /* for portamento */
  int porta_control_ratio, porta_control_counter, porta_dpb;
  int32 porta_pb;

  int delay; /* Note ON delay samples */
  int32 timeout;
  struct cache_hash *cache;

  uint8 chorus_link;	/* Chorus link */
  int8 proximate_flag;

  FilterCoefficients fc;

  FLOAT_T envelope_scale, last_envelope_volume;
  int32 inv_envelope_scale;

  int modenv_stage;
  int32
    modenv_volume, modenv_target, modenv_increment;
  FLOAT_T last_modenv_volume;
  int32 tremolo_delay, modenv_delay;

  int32 delay_counter;

#ifdef ENABLE_PAN_DELAY
  int32 *pan_delay_buf, pan_delay_rpt, pan_delay_wpt, pan_delay_spt;
#endif	/* ENABLE_PAN_DELAY */
} Voice;

/* Voice status options: */
#define VOICE_FREE	(1<<0)
#define VOICE_ON	(1<<1)
#define VOICE_SUSTAINED	(1<<2)
#define VOICE_OFF	(1<<3)
#define VOICE_DIE	(1<<4)

/* Voice panned options: */
#define PANNED_MYSTERY 0
#define PANNED_LEFT 1
#define PANNED_RIGHT 2
#define PANNED_CENTER 3
/* Anything but PANNED_MYSTERY only uses the left volume */

#define ISDRUMCHANNEL(c)  IS_SET_CHANNELMASK(drumchannels, c)

extern Channel channel[];
extern Voice *voice;

/* --module */
extern int opt_default_module;

enum {
	MODULE_TIMIDITY_DEFAULT = 0x0,
	/* GS modules */
	MODULE_SC55 = 0x1,
	MODULE_SC88 = 0x2,
	MODULE_SC88PRO = 0x3,
	MODULE_SC8850 = 0x4,
	/* XG modules */
	MODULE_MU50 = 0x10,
	MODULE_MU80 = 0x11,
	MODULE_MU90 = 0x12,
	MODULE_MU100 = 0x13,
	/* GM modules */
	MODULE_SBLIVE = 0x20,
	MODULE_SBAUDIGY = 0x21,
	/* Special modules */
	MODULE_TIMIDITY_SPECIAL1 = 0x70,
	MODULE_TIMIDITY_DEBUG = 0x7f,
};

static inline int get_module() {return opt_default_module;}

static inline int is_gs_module()
{
	int module = get_module();
    return (module >= MODULE_SC55 && module <= MODULE_MU100);
}

static inline int is_xg_module()
{
	int module = get_module();
    return (module >= MODULE_MU50 && module <= MODULE_MU100);
}

extern int32 control_ratio, amp_with_poly, amplification;

extern ChannelBitMask default_drumchannel_mask;
extern ChannelBitMask drumchannel_mask;
extern ChannelBitMask default_drumchannels;
extern ChannelBitMask drumchannels;

extern int adjust_panning_immediately;
extern int max_voices;
extern int voices, upper_voices;
extern int note_key_offset;
extern FLOAT_T midi_time_ratio;
extern int opt_modulation_wheel;
extern int opt_portamento;
extern int opt_nrpn_vibrato;
extern int opt_reverb_control;
extern int opt_chorus_control;
extern int opt_surround_chorus;
extern int opt_channel_pressure;
extern int opt_lpf_def;
extern int opt_overlap_voice_allow;
extern int opt_temper_control;
extern int opt_tva_attack;
extern int opt_tva_decay;
extern int opt_tva_release;
extern int opt_delay_control;
extern int opt_eq_control;
extern int opt_insertion_effect;
extern int opt_drum_effect;
extern int opt_env_attack;
extern int opt_modulation_envelope;
extern int noise_sharp_type;
extern int32 current_play_tempo;
extern int opt_realtime_playing;
extern int reduce_voice_threshold; /* msec */
extern int check_eot_flag;
extern int special_tonebank;
extern int default_tonebank;
extern int playmidi_seek_flag;
extern int effect_lr_mode;
extern int effect_lr_delay_msec;
extern int auto_reduce_polyphony;
extern int play_pause_flag;
extern int reduce_quality_flag;
extern int no_4point_interpolation;
extern ChannelBitMask channel_mute;
extern int temper_type_mute;
extern int8 current_keysig;
extern int8 current_temper_keysig;
extern int temper_adj;
extern int8 opt_init_keysig;
extern int8 opt_force_keysig;
extern int key_adjust;
extern FLOAT_T tempo_adjust;
extern int opt_pure_intonation;
extern int current_freq_table;
extern int32 opt_drum_power;
extern int opt_amp_compensation;
extern int opt_realtime_priority;	/* interface/alsaseq_c.c */
extern int opt_sequencer_ports;		/* interface/alsaseq_c.c */
extern int opt_user_volume_curve;
extern int opt_pan_delay;

extern int play_midi_file(char *fn);
extern void dumb_pass_playing_list(int number_of_files, char *list_of_files[]);
extern void default_ctl_lyric(int lyricid);
extern int check_apply_control(void);
extern void recompute_freq(int v);
extern int midi_drumpart_change(int ch, int isdrum);
extern void ctl_note_event(int noteID);
extern void ctl_mode_event(int type, int trace, long arg1, long arg2);
extern char *channel_instrum_name(int ch);
extern int get_reverb_level(int ch);
extern int get_chorus_level(int ch);
extern void playmidi_output_changed(int play_state);
extern Instrument *play_midi_load_instrument(int dr, int bk, int prog);
extern void midi_program_change(int ch, int prog);
extern void free_voice(int v);
extern void play_midi_setup_drums(int ch,int note);

/* For stream player */
extern void playmidi_stream_init(void);
extern void playmidi_tmr_reset(void);
extern int play_event(MidiEvent *ev);

extern void recompute_voice_filter(int);
extern int32 get_note_freq(Sample *, int);

extern void free_drum_effect(int);

#endif /* ___PLAYMIDI_H_ */
