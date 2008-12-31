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

    playmidi.c -- random stuff in need of rearrangement
*/

// oldnemesis: If you uncomment this, you'll get a q:\\debug.wav file for the MIDI you're playing.
// Very helpful to find whether the problem is in timidity code or somewhere else.
//#define DEBUG_WAV_OUTPUT

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef __W32__
//#include "interface.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#ifdef __W32__
#include <windows.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "mix.h"
#include "controls.h"
#include "miditrace.h"
#include "recache.h"
//#include "arc.h"
#include "reverb.h"
#include "wrd.h"
#include "aq.h"
#include "freq.h"
#include "quantity.h"

#define ABORT_AT_FATAL 1 /*#################*/
#define MYCHECK(s) do { if(s == 0) { printf("## L %d\n", __LINE__); abort(); } } while(0)

extern VOLATILE int intr;

/* #define SUPPRESS_CHANNEL_LAYER */

#ifdef SOLARIS
/* shut gcc warning up */
int usleep(unsigned int useconds);
#endif

#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */

#include "tables.h"

#define PLAY_INTERLEAVE_SEC		1.0
#define PORTAMENTO_TIME_TUNING		(1.0 / 5000.0)
#define PORTAMENTO_CONTROL_RATIO	256	/* controls per sec */
#define DEFAULT_CHORUS_DELAY1		0.02
#define DEFAULT_CHORUS_DELAY2		0.003
#define CHORUS_OPPOSITE_THRESHOLD	32
#define CHORUS_VELOCITY_TUNING1		0.7
#define CHORUS_VELOCITY_TUNING2		0.6
#define EOT_PRESEARCH_LEN		32
#define SPEED_CHANGE_RATE		1.0594630943592953  /* 2^(1/12) */

/* Undefine if you don't want to use auto voice reduce implementation */
#define REDUCE_VOICE_TIME_TUNING	(play_mode->rate/5) /* 0.2 sec */
#ifdef REDUCE_VOICE_TIME_TUNING
static int max_good_nv = 1;
static int min_bad_nv = 256;
static int32 ok_nv_total = 32;
static int32 ok_nv_counts = 1;
static int32 ok_nv_sample = 0;
static int ok_nv = 32;
static int old_rate = -1;
#endif

static int midi_streaming = 0;
int volatile stream_max_compute = 500; /* compute time limit (in msec) when streaming */
static int prescanning_flag;
static int32 midi_restart_time = 0;
Channel channel[MAX_CHANNELS];
int max_voices = DEFAULT_VOICES;
Voice *voice = NULL;
int8 current_keysig = 0;
int8 current_temper_keysig = 0;
int temper_adj = 0;
int8 opt_init_keysig = 8;
int8 opt_force_keysig = 8;
int32 current_play_tempo = 500000;
int opt_realtime_playing = 0;
int reduce_voice_threshold = -1;
static MBlockList playmidi_pool;
int check_eot_flag;
int special_tonebank = -1;
int default_tonebank = 0;
int playmidi_seek_flag = 0;
int play_pause_flag = 0;
static int file_from_stdin;
int key_adjust = 0;
FLOAT_T tempo_adjust = 1.0;
int opt_pure_intonation = 0;
int current_freq_table = 0;
int current_temper_freq_table = 0;

static void set_reverb_level(int ch, int level);
static int make_rvid_flag = 0; /* For reverb optimization */

/* Ring voice id for each notes.  This ID enables duplicated note. */
static uint8 vidq_head[128 * MAX_CHANNELS], vidq_tail[128 * MAX_CHANNELS];

#ifdef MODULATION_WHEEL_ALLOW
int opt_modulation_wheel = 1;
#else
int opt_modulation_wheel = 0;
#endif /* MODULATION_WHEEL_ALLOW */

#ifdef PORTAMENTO_ALLOW
int opt_portamento = 1;
#else
int opt_portamento = 0;
#endif /* PORTAMENTO_ALLOW */

#ifdef NRPN_VIBRATO_ALLOW
int opt_nrpn_vibrato = 1;
#else
int opt_nrpn_vibrato = 0;
#endif /* NRPN_VIBRATO_ALLOW */

#ifdef REVERB_CONTROL_ALLOW
int opt_reverb_control = 1;
#else
#ifdef FREEVERB_CONTROL_ALLOW
int opt_reverb_control = 3;
#else
int opt_reverb_control = 0;
#endif /* FREEVERB_CONTROL_ALLOW */
#endif /* REVERB_CONTROL_ALLOW */

#ifdef CHORUS_CONTROL_ALLOW
int opt_chorus_control = 1;
#else
int opt_chorus_control = 0;
#endif /* CHORUS_CONTROL_ALLOW */

#ifdef SURROUND_CHORUS_ALLOW
int opt_surround_chorus = 1;
#else
int opt_surround_chorus = 0;
#endif /* SURROUND_CHORUS_ALLOW */

#ifdef GM_CHANNEL_PRESSURE_ALLOW
int opt_channel_pressure = 1;
#else
int opt_channel_pressure = 0;
#endif /* GM_CHANNEL_PRESSURE_ALLOW */

#ifdef VOICE_CHAMBERLIN_LPF_ALLOW
int opt_lpf_def = 1;
#else
#ifdef VOICE_MOOG_LPF_ALLOW
int opt_lpf_def = 2;
#else
int opt_lpf_def = 0;
#endif /* VOICE_MOOG_LPF_ALLOW */
#endif /* VOICE_CHAMBERLIN_LPF_ALLOW */

#ifdef OVERLAP_VOICE_ALLOW
int opt_overlap_voice_allow = 1;
#else
int opt_overlap_voice_allow = 0;
#endif /* OVERLAP_VOICE_ALLOW */

#ifdef TEMPER_CONTROL_ALLOW
int opt_temper_control = 1;
#else
int opt_temper_control = 0;
#endif /* TEMPER_CONTROL_ALLOW */

int opt_tva_attack = 0;	/* attack envelope control */
int opt_tva_decay = 0;	/* decay envelope control */
int opt_tva_release = 0;	/* release envelope control */
int opt_delay_control = 0;	/* CC#94 delay(celeste) effect control */
int opt_eq_control = 0;		/* channel equalizer control */
int opt_insertion_effect = 0;	/* insertion effect control */
int opt_drum_effect = 0;	/* drumpart effect control */
int32 opt_drum_power = 100;		/* coef. of drum amplitude */
int opt_amp_compensation = 0;
int opt_modulation_envelope = 0;
int opt_pan_delay = 0;	/* phase difference between left ear and right ear. */
int opt_user_volume_curve = 0;
int opt_default_module = MODULE_TIMIDITY_DEFAULT;

int voices=DEFAULT_VOICES, upper_voices;

int32
    control_ratio=0,
    amplification=DEFAULT_AMPLIFICATION;

static FLOAT_T
    master_volume;
static int32 master_volume_ratio = 0xFFFF;
ChannelBitMask default_drumchannel_mask;
ChannelBitMask default_drumchannels;
ChannelBitMask drumchannel_mask;
ChannelBitMask drumchannels;
int adjust_panning_immediately = 1;
int auto_reduce_polyphony = 1;
double envelope_modify_rate = 1.0;
int reduce_quality_flag = 0;
int no_4point_interpolation = 0;
char* pcm_alternate_file = NULL; /* NULL or "none": Nothing (default)
				  * "auto": Auto select
				  * filename: Use it
				  */

static int32 lost_notes, cut_notes;
static int32 common_buffer[AUDIO_BUFFER_SIZE*2], /* stereo samples */
             *buffer_pointer;
static int16 wav_buffer[AUDIO_BUFFER_SIZE*2];
static int32 buffered_count;
static char *reverb_buffer = NULL; /* MAX_CHANNELS*AUDIO_BUFFER_SIZE*8 */

#ifdef USE_DSP_EFFECT
static int32 insertion_effect_buffer[AUDIO_BUFFER_SIZE * 2];
#endif /* USE_DSP_EFFECT */

#define VIBRATO_DEPTH_MAX 384	/* 600 cent */

static MidiEvent *event_list;
static MidiEvent *current_event;
static int32 sample_count;	/* Length of event_list */
int32 current_sample;		/* Number of calclated samples */

int note_key_offset = 0;		/* For key up/down */
FLOAT_T midi_time_ratio = 1.0;	/* For speed up/down */
ChannelBitMask channel_mute;	/* For channel mute */
int temper_type_mute;			/* For temperament type mute */

/* for auto amplitude compensation */
static int mainvolume_max; /* maximum value of mainvolume */
static double compensation_ratio = 1.0; /* compensation ratio */

static int find_samples(MidiEvent *, int *);
static int select_play_sample(Sample *, int, int *, int *, MidiEvent *);
static double get_play_note_ratio(int, int);
static int find_voice(MidiEvent *);
static void update_portamento_controls(int ch);
static void update_rpn_map(int ch, int addr, int update_now);
static void ctl_prog_event(int ch);
static void ctl_timestamp(void);
static void ctl_updatetime(int32 samples);
static void ctl_pause_event(int pause, int32 samples);
static void update_legato_controls(int ch);
static void update_channel_freq(int ch);
static void set_single_note_tuning(int, int, int, int);
static void set_user_temper_entry(int, int, int);
void recompute_bank_parameter(int, int);

static void init_voice_filter(int);
/* XG Part EQ */
void init_part_eq_xg(struct part_eq_xg *);
void recompute_part_eq_xg(struct part_eq_xg *);
/* MIDI controllers (MW, Bend, CAf, PAf,...) */
static void init_midi_controller(midi_controller *);
static float get_midi_controller_amp(midi_controller *);
static float get_midi_controller_filter_cutoff(midi_controller *);
static float get_midi_controller_filter_depth(midi_controller *);
static int32 get_midi_controller_pitch(midi_controller *);
static int16 get_midi_controller_pitch_depth(midi_controller *);
static int16 get_midi_controller_amp_depth(midi_controller *);
/* Rx. ~ (Rcv ~) */
static void init_rx(int);
static void set_rx(int, int32, int);
static int32 get_rx(int, int32);
static void init_rx_drum(struct DrumParts *);
static void set_rx_drum(struct DrumParts *, int32, int);
static int32 get_rx_drum(struct DrumParts *, int32);

#define IS_SYSEX_EVENT_TYPE(event) ((event)->type == ME_NONE || (event)->type >= ME_RANDOM_PAN || (event)->b == SYSEX_TAG)

static char *event_name(int type)
{
#define EVENT_NAME(X) case X: return #X
	switch (type) {
	EVENT_NAME(ME_NONE);
	EVENT_NAME(ME_NOTEOFF);
	EVENT_NAME(ME_NOTEON);
	EVENT_NAME(ME_KEYPRESSURE);
	EVENT_NAME(ME_PROGRAM);
	EVENT_NAME(ME_CHANNEL_PRESSURE);
	EVENT_NAME(ME_PITCHWHEEL);
	EVENT_NAME(ME_TONE_BANK_MSB);
	EVENT_NAME(ME_TONE_BANK_LSB);
	EVENT_NAME(ME_MODULATION_WHEEL);
	EVENT_NAME(ME_BREATH);
	EVENT_NAME(ME_FOOT);
	EVENT_NAME(ME_MAINVOLUME);
	EVENT_NAME(ME_BALANCE);
	EVENT_NAME(ME_PAN);
	EVENT_NAME(ME_EXPRESSION);
	EVENT_NAME(ME_SUSTAIN);
	EVENT_NAME(ME_PORTAMENTO_TIME_MSB);
	EVENT_NAME(ME_PORTAMENTO_TIME_LSB);
	EVENT_NAME(ME_PORTAMENTO);
	EVENT_NAME(ME_PORTAMENTO_CONTROL);
	EVENT_NAME(ME_DATA_ENTRY_MSB);
	EVENT_NAME(ME_DATA_ENTRY_LSB);
	EVENT_NAME(ME_SOSTENUTO);
	EVENT_NAME(ME_SOFT_PEDAL);
	EVENT_NAME(ME_LEGATO_FOOTSWITCH);
	EVENT_NAME(ME_HOLD2);
	EVENT_NAME(ME_HARMONIC_CONTENT);
	EVENT_NAME(ME_RELEASE_TIME);
	EVENT_NAME(ME_ATTACK_TIME);
	EVENT_NAME(ME_BRIGHTNESS);
	EVENT_NAME(ME_REVERB_EFFECT);
	EVENT_NAME(ME_TREMOLO_EFFECT);
	EVENT_NAME(ME_CHORUS_EFFECT);
	EVENT_NAME(ME_CELESTE_EFFECT);
	EVENT_NAME(ME_PHASER_EFFECT);
	EVENT_NAME(ME_RPN_INC);
	EVENT_NAME(ME_RPN_DEC);
	EVENT_NAME(ME_NRPN_LSB);
	EVENT_NAME(ME_NRPN_MSB);
	EVENT_NAME(ME_RPN_LSB);
	EVENT_NAME(ME_RPN_MSB);
	EVENT_NAME(ME_ALL_SOUNDS_OFF);
	EVENT_NAME(ME_RESET_CONTROLLERS);
	EVENT_NAME(ME_ALL_NOTES_OFF);
	EVENT_NAME(ME_MONO);
	EVENT_NAME(ME_POLY);
#if 0
	EVENT_NAME(ME_VOLUME_ONOFF);		/* Not supported */
#endif
	EVENT_NAME(ME_SCALE_TUNING);
	EVENT_NAME(ME_BULK_TUNING_DUMP);
	EVENT_NAME(ME_SINGLE_NOTE_TUNING);
	EVENT_NAME(ME_RANDOM_PAN);
	EVENT_NAME(ME_SET_PATCH);
	EVENT_NAME(ME_DRUMPART);
	EVENT_NAME(ME_KEYSHIFT);
	EVENT_NAME(ME_PATCH_OFFS);
	EVENT_NAME(ME_TEMPO);
	EVENT_NAME(ME_CHORUS_TEXT);
	EVENT_NAME(ME_LYRIC);
	EVENT_NAME(ME_GSLCD);
	EVENT_NAME(ME_MARKER);
	EVENT_NAME(ME_INSERT_TEXT);
	EVENT_NAME(ME_TEXT);
	EVENT_NAME(ME_KARAOKE_LYRIC);
	EVENT_NAME(ME_MASTER_VOLUME);
	EVENT_NAME(ME_RESET);
	EVENT_NAME(ME_NOTE_STEP);
	EVENT_NAME(ME_TIMESIG);
	EVENT_NAME(ME_KEYSIG);
	EVENT_NAME(ME_TEMPER_KEYSIG);
	EVENT_NAME(ME_TEMPER_TYPE);
	EVENT_NAME(ME_MASTER_TEMPER_TYPE);
	EVENT_NAME(ME_USER_TEMPER_ENTRY);
	EVENT_NAME(ME_SYSEX_LSB);
	EVENT_NAME(ME_SYSEX_MSB);
	EVENT_NAME(ME_SYSEX_GS_LSB);
	EVENT_NAME(ME_SYSEX_GS_MSB);
	EVENT_NAME(ME_SYSEX_XG_LSB);
	EVENT_NAME(ME_SYSEX_XG_MSB);
	EVENT_NAME(ME_WRD);
	EVENT_NAME(ME_SHERRY);
	EVENT_NAME(ME_BARMARKER);
	EVENT_NAME(ME_STEP);
	EVENT_NAME(ME_LAST);
	EVENT_NAME(ME_EOT);
	}
	return "Unknown";
#undef EVENT_NAME
}


// For debugging inside XBMC
void adddebuglog( const char * fmt, ... )
{
	char logbuf[4096];
	int characters;
	FILE * fp;
	va_list va_alist;
		
	va_start (va_alist, fmt);
	characters = vsnprintf( logbuf, sizeof(logbuf), fmt, va_alist);
	va_end (va_alist);

	characters += 2;
	strcat( logbuf, "\r\n" );

	if ( (fp = fopen( "q:\\debug.log", "r+")) != 0 )
	{
		fseek( fp, 0, SEEK_END );
		fwrite( logbuf, 1, characters, fp);
		fclose( fp );
	}
}


/*! convert Hz to internal vibrato control ratio. */
static FLOAT_T cnv_Hz_to_vib_ratio(FLOAT_T freq)
{
	return ((FLOAT_T)(play_mode->rate) / (freq * 2.0f * VIBRATO_SAMPLE_INCREMENTS));
}

static void adjust_amplification(void)
{
    /* compensate master volume */
    master_volume = (double)(amplification) / 100.0 *
	((double)master_volume_ratio * (compensation_ratio/0xFFFF));
}

static int new_vidq(int ch, int note)
{
    int i;

    if(opt_overlap_voice_allow)
    {
	i = ch * 128 + note;
	return vidq_head[i]++;
    }
    return 0;
}

static int last_vidq(int ch, int note)
{
    int i;

    if(opt_overlap_voice_allow)
    {
	i = ch * 128 + note;
	if(vidq_head[i] == vidq_tail[i])
	{
	    ctl->cmsg(CMSG_WARNING, VERB_DEBUG_SILLY,
		      "channel=%d, note=%d: Voice is already OFF", ch, note);
	    return -1;
	}
	return vidq_tail[i]++;
    }
    return 0;
}

static void reset_voices(void)
{
    int i;
    for(i = 0; i < max_voices; i++)
    {
	voice[i].status = VOICE_FREE;
	voice[i].temper_instant = 0;
	voice[i].chorus_link = i;
    }
    upper_voices = 0;
    memset(vidq_head, 0, sizeof(vidq_head));
    memset(vidq_tail, 0, sizeof(vidq_tail));
}

static void kill_note(int i)
{
    voice[i].status = VOICE_DIE;
    if(!prescanning_flag)
	ctl_note_event(i);
}

static void kill_all_voices(void)
{
    int i, uv = upper_voices;

    for(i = 0; i < uv; i++)
	if(voice[i].status & ~(VOICE_FREE | VOICE_DIE))
	    kill_note(i);
    memset(vidq_head, 0, sizeof(vidq_head));
    memset(vidq_tail, 0, sizeof(vidq_tail));
}

static void reset_drum_controllers(struct DrumParts *d[], int note)
{
    int i,j;

    if(note == -1)
    {
	for(i = 0; i < 128; i++)
	    if(d[i] != NULL)
	    {
		d[i]->drum_panning = NO_PANNING;
		for(j=0;j<6;j++) {d[i]->drum_envelope_rate[j] = -1;}
		d[i]->pan_random = 0;
		d[i]->drum_level = 1.0f;
		d[i]->coarse = 0;
		d[i]->fine = 0;
		d[i]->delay_level = -1;
		d[i]->chorus_level = -1;
		d[i]->reverb_level = -1;
		d[i]->play_note = -1;
		d[i]->drum_cutoff_freq = 0;
		d[i]->drum_resonance = 0;
		init_rx_drum(d[i]);
	    }
    }
    else
    {
	d[note]->drum_panning = NO_PANNING;
	for(j = 0; j < 6; j++) {d[note]->drum_envelope_rate[j] = -1;}
	d[note]->pan_random = 0;
	d[note]->drum_level = 1.0f;
	d[note]->coarse = 0;
	d[note]->fine = 0;
	d[note]->delay_level = -1;
	d[note]->chorus_level = -1;
	d[note]->reverb_level = -1;
	d[note]->play_note = -1;
	d[note]->drum_cutoff_freq = 0;
	d[note]->drum_resonance = 0;
	init_rx_drum(d[note]);
    }
}

static void reset_module_dependent_controllers(int c)
{
	int module = get_module();
	switch(module) {	/* TONE MAP-0 NUMBER */
	case MODULE_SC55:
		channel[c].tone_map0_number = 1;
		break;
	case MODULE_SC88:
		channel[c].tone_map0_number = 2;
		break;
	case MODULE_SC88PRO:
		channel[c].tone_map0_number = 3;
		break;
	case MODULE_SC8850:
		channel[c].tone_map0_number = 4;
		break;
	default:
		channel[c].tone_map0_number = 0;
		break;
	}
	switch(module) {	/* MIDI Controllers */
	case MODULE_SC55:
		channel[c].mod.lfo1_pitch_depth = 10;
		break;
	case MODULE_SC88:
		channel[c].mod.lfo1_pitch_depth = 10;
		break;
	case MODULE_SC88PRO:
		channel[c].mod.lfo1_pitch_depth = 10;
		break;
	default:
		channel[c].mod.lfo1_pitch_depth = 50;
		break;
	}
}

static void reset_nrpn_controllers(int c)
{
  int i;

  /* NRPN */
  reset_drum_controllers(channel[c].drums, -1);
  channel[c].vibrato_ratio = 1.0;
  channel[c].vibrato_depth = 0;
  channel[c].vibrato_delay = 0;
  channel[c].param_cutoff_freq = 0;
  channel[c].param_resonance = 0;
  channel[c].cutoff_freq_coef = 1.0;
  channel[c].resonance_dB = 0;

  /* System Exclusive */
  channel[c].dry_level = 127;
  channel[c].eq_gs = 1;
  channel[c].insertion_effect = 0;
  channel[c].velocity_sense_depth = 0x40;
  channel[c].velocity_sense_offset = 0x40;
  channel[c].pitch_offset_fine = 0;
  if(play_system_mode == GS_SYSTEM_MODE) {channel[c].assign_mode = 1;}
  else {
	  if(ISDRUMCHANNEL(c)) {channel[c].assign_mode = 1;}
	  else {channel[c].assign_mode = 2;}
  }
  for (i = 0; i < 12; i++)
	  channel[c].scale_tuning[i] = 0;
  channel[c].prev_scale_tuning = 0;
  channel[c].temper_type = 0;

  init_channel_layer(c);
  init_part_eq_xg(&(channel[c].eq_xg));

  /* channel pressure & polyphonic key pressure control */
  init_midi_controller(&(channel[c].mod));
  init_midi_controller(&(channel[c].bend)); 
  init_midi_controller(&(channel[c].caf)); 
  init_midi_controller(&(channel[c].paf)); 
  init_midi_controller(&(channel[c].cc1)); 
  init_midi_controller(&(channel[c].cc2)); 
  channel[c].bend.pitch = 2;

  init_rx(c);
  channel[c].note_limit_high = 127;
  channel[c].note_limit_low = 0;
  channel[c].vel_limit_high = 127;
  channel[c].vel_limit_low = 0;

  free_drum_effect(c);

  channel[c].legato = 0;
  channel[c].damper_mode = 0;
  channel[c].loop_timeout = 0;

  channel[c].sysex_gs_msb_addr = channel[c].sysex_gs_msb_val =
	channel[c].sysex_xg_msb_addr = channel[c].sysex_xg_msb_val =
	channel[c].sysex_msb_addr = channel[c].sysex_msb_val = 0;
}

/* Process the Reset All Controllers event */
static void reset_controllers(int c)
{
  int j;
    /* Some standard says, although the SCC docs say 0. */
    
  if(play_system_mode == XG_SYSTEM_MODE)
      channel[c].volume = 100;
  else
      channel[c].volume = 90;
  if (prescanning_flag) {
    if (channel[c].volume > mainvolume_max) {	/* pick maximum value of mainvolume */
      mainvolume_max = channel[c].volume;
      ctl->cmsg(CMSG_INFO,VERB_DEBUG,"ME_MAINVOLUME/max (CH:%d VAL:%#x)",c,mainvolume_max);
    }
  }

  channel[c].expression = 127; /* SCC-1 does this. */
  channel[c].sustain = 0;
  channel[c].sostenuto = 0;
  channel[c].pitchbend = 0x2000;
  channel[c].pitchfactor = 0; /* to be computed */
  channel[c].mod.val = 0;
  channel[c].bend.val = 0;
  channel[c].caf.val = 0;
  channel[c].paf.val = 0;
  channel[c].cc1.val = 0;
  channel[c].cc2.val = 0;
  channel[c].portamento_time_lsb = 0;
  channel[c].portamento_time_msb = 0;
  channel[c].porta_control_ratio = 0;
  channel[c].portamento = 0;
  channel[c].last_note_fine = -1;
  for(j = 0; j < 6; j++) {channel[c].envelope_rate[j] = -1;}
  update_portamento_controls(c);
  set_reverb_level(c, -1);
  if(opt_chorus_control == 1)
      channel[c].chorus_level = 0;
  else
      channel[c].chorus_level = -opt_chorus_control;
  channel[c].mono = 0;
  channel[c].delay_level = 0;
}

static void redraw_controllers(int c)
{
    ctl_mode_event(CTLE_VOLUME, 1, c, channel[c].volume);
    ctl_mode_event(CTLE_EXPRESSION, 1, c, channel[c].expression);
    ctl_mode_event(CTLE_SUSTAIN, 1, c, channel[c].sustain);
    ctl_mode_event(CTLE_MOD_WHEEL, 1, c, channel[c].mod.val);
    ctl_mode_event(CTLE_PITCH_BEND, 1, c, channel[c].pitchbend);
    ctl_prog_event(c);
    ctl_mode_event(CTLE_TEMPER_TYPE, 1, c, channel[c].temper_type);
    ctl_mode_event(CTLE_MUTE, 1,
    		c, (IS_SET_CHANNELMASK(channel_mute, c)) ? 1 : 0);
    ctl_mode_event(CTLE_CHORUS_EFFECT, 1, c, get_chorus_level(c));
    ctl_mode_event(CTLE_REVERB_EFFECT, 1, c, get_reverb_level(c));
}

static void reset_midi(int playing)
{
	int i, cnt;
	
	for (i = 0; i < MAX_CHANNELS; i++) {
		reset_controllers(i);
		reset_nrpn_controllers(i);
     	reset_module_dependent_controllers(i);
		/* The rest of these are unaffected
		 * by the Reset All Controllers event
		 */
		channel[i].program = default_program[i];
		channel[i].panning = NO_PANNING;
		channel[i].pan_random = 0;
		/* tone bank or drum set */
		if (ISDRUMCHANNEL(i)) {
			channel[i].bank = 0;
			channel[i].altassign = drumset[0]->alt;
		} else {
			if (special_tonebank >= 0)
				channel[i].bank = special_tonebank;
			else
				channel[i].bank = default_tonebank;
		}
		channel[i].bank_lsb = channel[i].bank_msb = 0;
		if (play_system_mode == XG_SYSTEM_MODE && i % 16 == 9)
			channel[i].bank_msb = 127;	/* Use MSB=127 for XG */
		update_rpn_map(i, RPN_ADDR_FFFF, 0);
		channel[i].special_sample = 0;
		channel[i].key_shift = 0;
		channel[i].mapID = get_default_mapID(i);
		channel[i].lasttime = 0;
	}
	if (playing) {
		kill_all_voices();
		if (temper_type_mute) {
			if (temper_type_mute & 1)
				FILL_CHANNELMASK(channel_mute);
			else
				CLEAR_CHANNELMASK(channel_mute);
		}
		for (i = 0; i < MAX_CHANNELS; i++)
			redraw_controllers(i);
		if (midi_streaming && free_instruments_afterwards) {
			free_instruments(0);
			/* free unused memory */
			cnt = free_global_mblock();
			if (cnt > 0)
				ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
						"%d memory blocks are free", cnt);
		}
	} else
		reset_voices();
	master_volume_ratio = 0xffff;
	adjust_amplification();
	init_freq_table_tuning();
	if (current_file_info) {
		COPY_CHANNELMASK(drumchannels, current_file_info->drumchannels);
		COPY_CHANNELMASK(drumchannel_mask,
				current_file_info->drumchannel_mask);
	} else {
		COPY_CHANNELMASK(drumchannels, default_drumchannels);
		COPY_CHANNELMASK(drumchannel_mask, default_drumchannel_mask);
	}
	ctl_mode_event(CTLE_MASTER_VOLUME, 0, amplification, 0);
	ctl_mode_event(CTLE_KEY_OFFSET, 0, note_key_offset, 0);
	ctl_mode_event(CTLE_TIME_RATIO, 0, 100 / midi_time_ratio + 0.5, 0);
}

void recompute_freq(int v)
{
	int i;
	int ch = voice[v].channel;
	int note = voice[v].note;
	int32 tuning = 0;
	int8 st = channel[ch].scale_tuning[note % 12];
	int8 tt = channel[ch].temper_type;
	uint8 tp = channel[ch].rpnmap[RPN_ADDR_0003];
	int32 f;
	int pb = channel[ch].pitchbend;
	int32 tmp;
	FLOAT_T pf, root_freq;
	int32 a;
	Voice *vp = &(voice[v]);

	if (! voice[v].sample->sample_rate)
		return;
	if (! opt_modulation_wheel)
		channel[ch].mod.val = 0;
	if (! opt_portamento)
		voice[v].porta_control_ratio = 0;
	voice[v].vibrato_control_ratio = voice[v].orig_vibrato_control_ratio;
	if (voice[v].vibrato_control_ratio || channel[ch].mod.val > 0) {
		/* This instrument has vibrato. Invalidate any precomputed
		 * sample_increments.
		 */

		/* MIDI controllers LFO pitch depth */
		if (opt_channel_pressure || opt_modulation_wheel) {
			vp->vibrato_depth = vp->sample->vibrato_depth + channel[ch].vibrato_depth;
			vp->vibrato_depth += get_midi_controller_pitch_depth(&(channel[ch].mod))
				+ get_midi_controller_pitch_depth(&(channel[ch].bend))
				+ get_midi_controller_pitch_depth(&(channel[ch].caf))
				+ get_midi_controller_pitch_depth(&(channel[ch].paf))
				+ get_midi_controller_pitch_depth(&(channel[ch].cc1))
				+ get_midi_controller_pitch_depth(&(channel[ch].cc2));
			if (vp->vibrato_depth > VIBRATO_DEPTH_MAX) {vp->vibrato_depth = VIBRATO_DEPTH_MAX;}
			else if (vp->vibrato_depth < 1) {vp->vibrato_depth = 1;}
			if (vp->sample->vibrato_depth < 0) {	/* in opposite phase */
				vp->vibrato_depth = -vp->vibrato_depth;
			}
		}
		
		/* fill parameters for modulation wheel */
		if (channel[ch].mod.val > 0) {
			if(vp->vibrato_control_ratio == 0) {
				vp->vibrato_control_ratio = 
					vp->orig_vibrato_control_ratio = (int)(cnv_Hz_to_vib_ratio(5.0) * channel[ch].vibrato_ratio);
			}
			vp->vibrato_delay = 0;
		}

		for (i = 0; i < VIBRATO_SAMPLE_INCREMENTS; i++)
			vp->vibrato_sample_increment[i] = 0;
		vp->cache = NULL;
	}
	/* fine: [0..128] => [-256..256]
	 * 1 coarse = 256 fine (= 1 note)
	 * 1 fine = 2^5 tuning
	 */
	tuning = (channel[ch].rpnmap[RPN_ADDR_0001] - 0x40
			+ (channel[ch].rpnmap[RPN_ADDR_0002] - 0x40) * 64) << 7;
	/* for NRPN Coarse Pitch of Drum (GS) & Fine Pitch of Drum (XG) */
	if (ISDRUMCHANNEL(ch) && channel[ch].drums[note] != NULL
			&& (channel[ch].drums[note]->fine
			|| channel[ch].drums[note]->coarse)) {
		tuning += (channel[ch].drums[note]->fine
				+ channel[ch].drums[note]->coarse * 64) << 7;
	}
	/* MIDI controllers pitch control */
	if (opt_channel_pressure) {
		tuning += get_midi_controller_pitch(&(channel[ch].mod))
			+ get_midi_controller_pitch(&(channel[ch].bend))
			+ get_midi_controller_pitch(&(channel[ch].caf))
			+ get_midi_controller_pitch(&(channel[ch].paf))
			+ get_midi_controller_pitch(&(channel[ch].cc1))
			+ get_midi_controller_pitch(&(channel[ch].cc2));
	}
	if (opt_modulation_envelope) {
		if (voice[v].sample->tremolo_to_pitch) {
			tuning += lookup_triangular(voice[v].tremolo_phase >> RATE_SHIFT)
					* (voice[v].sample->tremolo_to_pitch << 13) / 100.0 + 0.5;
			channel[ch].pitchfactor = 0;
		}
		if (voice[v].sample->modenv_to_pitch) {
			tuning += voice[v].last_modenv_volume
					* (voice[v].sample->modenv_to_pitch << 13) / 100.0 + 0.5;
			channel[ch].pitchfactor = 0;
		}
	}
	/* GS/XG - Scale Tuning */
	if (! ISDRUMCHANNEL(ch)) {
		tuning += (st << 13) / 100.0 + 0.5;
		if (st != channel[ch].prev_scale_tuning) {
			channel[ch].pitchfactor = 0;
			channel[ch].prev_scale_tuning = st;
		}
	}
	if (! opt_pure_intonation
			&& opt_temper_control && voice[v].temper_instant) {
		switch (tt) {
		case 0:
			f = freq_table_tuning[tp][note];
			break;
		case 1:
			if (current_temper_keysig < 8)
				f = freq_table_pytha[current_temper_freq_table][note];
			else
				f = freq_table_pytha[current_temper_freq_table + 12][note];
			break;
		case 2:
			if (current_temper_keysig < 8)
				f = freq_table_meantone[current_temper_freq_table
						+ ((temper_adj) ? 36 : 0)][note];
			else
				f = freq_table_meantone[current_temper_freq_table
						+ ((temper_adj) ? 24 : 12)][note];
			break;
		case 3:
			if (current_temper_keysig < 8)
				f = freq_table_pureint[current_temper_freq_table
						+ ((temper_adj) ? 36 : 0)][note];
			else
				f = freq_table_pureint[current_temper_freq_table
						+ ((temper_adj) ? 24 : 12)][note];
			break;
		default:	/* user-defined temperament */
			if ((tt -= 0x40) >= 0 && tt < 4) {
				if (current_temper_keysig < 8)
					f = freq_table_user[tt][current_temper_freq_table
							+ ((temper_adj) ? 36 : 0)][note];
				else
					f = freq_table_user[tt][current_temper_freq_table
							+ ((temper_adj) ? 24 : 12)][note];
			} else
				f = freq_table[note];
			break;
		}
		voice[v].orig_frequency = f;
	}
	if (! voice[v].porta_control_ratio) {
		if (tuning == 0 && pb == 0x2000)
			voice[v].frequency = voice[v].orig_frequency;
		else {
			pb -= 0x2000;
			if (! channel[ch].pitchfactor) {
				/* Damn.  Somebody bent the pitch. */
				tmp = pb * channel[ch].rpnmap[RPN_ADDR_0000] + tuning;
				if (tmp >= 0)
					channel[ch].pitchfactor = bend_fine[tmp >> 5 & 0xff]
							* bend_coarse[tmp >> 13 & 0x7f];
				else
					channel[ch].pitchfactor = 1.0 /
							(bend_fine[-tmp >> 5 & 0xff]
							* bend_coarse[-tmp >> 13 & 0x7f]);
			}
			voice[v].frequency =
					voice[v].orig_frequency * channel[ch].pitchfactor;
			if (voice[v].frequency != voice[v].orig_frequency)
				voice[v].cache = NULL;
		}
	} else {	/* Portamento */
		pb -= 0x2000;
		tmp = pb * channel[ch].rpnmap[RPN_ADDR_0000]
				+ (voice[v].porta_pb << 5) + tuning;
		if (tmp >= 0)
			pf = bend_fine[tmp >> 5 & 0xff]
					* bend_coarse[tmp >> 13 & 0x7f];
		else
			pf = 1.0 / (bend_fine[-tmp >> 5 & 0xff]
					* bend_coarse[-tmp >> 13 & 0x7f]);
		voice[v].frequency = voice[v].orig_frequency * pf;
		voice[v].cache = NULL;
	}
	root_freq = voice[v].sample->root_freq;
	a = TIM_FSCALE(((double) voice[v].sample->sample_rate
			* ((double)voice[v].frequency + channel[ch].pitch_offset_fine))
			/ (root_freq * play_mode->rate), FRACTION_BITS) + 0.5;
	/* need to preserve the loop direction */
	voice[v].sample_increment = (voice[v].sample_increment >= 0) ? a : -a;
#ifdef ABORT_AT_FATAL
	if (voice[v].sample_increment == 0) {
		fprintf(stderr, "Invalid sample increment a=%e %ld %ld %ld %ld%s\n",
				(double)a, (long) voice[v].sample->sample_rate,
				(long) voice[v].frequency, (long) voice[v].sample->root_freq,
				(long) play_mode->rate, (voice[v].cache) ? " (Cached)" : "");
		abort();
	}
#endif	/* ABORT_AT_FATAL */
}

static int32 calc_velocity(int32 ch,int32 vel)
{
	int32 velocity;
	velocity = channel[ch].velocity_sense_depth * vel / 64 + (channel[ch].velocity_sense_offset - 64) * 2;
	if(velocity > 127) {velocity = 127;}
	return velocity;
}

static void recompute_voice_tremolo(int v)
{
	Voice *vp = &(voice[v]);
	int ch = vp->channel;
	int32 depth = vp->sample->tremolo_depth;
	depth += get_midi_controller_amp_depth(&(channel[ch].mod))
		+ get_midi_controller_amp_depth(&(channel[ch].bend))
		+ get_midi_controller_amp_depth(&(channel[ch].caf))
		+ get_midi_controller_amp_depth(&(channel[ch].paf))
		+ get_midi_controller_amp_depth(&(channel[ch].cc1))
		+ get_midi_controller_amp_depth(&(channel[ch].cc2));
	if(depth > 256) {depth = 256;}
	vp->tremolo_depth = depth;
}

static void recompute_amp(int v)
{
	FLOAT_T tempamp;
	int ch = voice[v].channel;

	/* master_volume and sample->volume are percentages, used to scale
	 *  amplitude directly, NOT perceived volume
	 *
	 * all other MIDI volumes are linear in perceived volume, 0-127
	 * use a lookup table for the non-linear scalings
	 */
	if (opt_user_volume_curve) {
	tempamp = master_volume *
		   voice[v].sample->volume *
		   user_vol_table[calc_velocity(ch, voice[v].velocity)] *
		   user_vol_table[channel[ch].volume] *
		   user_vol_table[channel[ch].expression]; /* 21 bits */
	} else if (play_system_mode == GM2_SYSTEM_MODE) {
	tempamp = master_volume *
		  voice[v].sample->volume *
		  gm2_vol_table[calc_velocity(ch, voice[v].velocity)] *	/* velocity: not in GM2 standard */
		  gm2_vol_table[channel[ch].volume] *
		  gm2_vol_table[channel[ch].expression]; /* 21 bits */
	} else if(play_system_mode == GS_SYSTEM_MODE) {	/* use measured curve */ 
	tempamp = master_volume *
		   voice[v].sample->volume *
		   sc_vel_table[calc_velocity(ch, voice[v].velocity)] *
		   sc_vol_table[channel[ch].volume] *
		   sc_vol_table[channel[ch].expression]; /* 21 bits */
	} else if (IS_CURRENT_MOD_FILE) {	/* use linear curve */
	tempamp = master_volume *
		  voice[v].sample->volume *
		  calc_velocity(ch, voice[v].velocity) *
		  channel[ch].volume *
		  channel[ch].expression; /* 21 bits */
	} else {	/* use generic exponential curve */
	tempamp = master_volume *
		  voice[v].sample->volume *
		  perceived_vol_table[calc_velocity(ch, voice[v].velocity)] *
		  perceived_vol_table[channel[ch].volume] *
		  perceived_vol_table[channel[ch].expression]; /* 21 bits */
	}

	/* every digital effect increases amplitude,
	 * so that it must be reduced in advance.
	 */
	if (! (play_mode->encoding & PE_MONO)
	    && (opt_reverb_control || opt_chorus_control || opt_delay_control
		|| (opt_eq_control && (eq_status_gs.low_gain != 0x40
				       || eq_status_gs.high_gain != 0x40))
		|| opt_insertion_effect))
		tempamp *= 1.35f * 0.55f;
	else
		tempamp *= 1.35f;

	/* NRPN - drum instrument tva level */
	if(ISDRUMCHANNEL(ch)) {
		if(channel[ch].drums[voice[v].note] != NULL) {
			tempamp *= channel[ch].drums[voice[v].note]->drum_level;
		}
		tempamp *= (double)opt_drum_power * 0.01f;	/* global drum power */
	}

	/* MIDI controllers amplitude control */
	if(opt_channel_pressure) {
		tempamp *= get_midi_controller_amp(&(channel[ch].mod))
			* get_midi_controller_amp(&(channel[ch].bend))
			* get_midi_controller_amp(&(channel[ch].caf))
			* get_midi_controller_amp(&(channel[ch].paf))
			* get_midi_controller_amp(&(channel[ch].cc1))
			* get_midi_controller_amp(&(channel[ch].cc2));
		recompute_voice_tremolo(v);
	}

	if (voice[v].fc.type != 0) {
		tempamp *= voice[v].fc.gain;	/* filter gain */
	}

	/* applying panning to amplitude */
	if(!(play_mode->encoding & PE_MONO))
    	{
		if(voice[v].panning == 64)
		{
			voice[v].panned = PANNED_CENTER;
			voice[v].left_amp = voice[v].right_amp = TIM_FSCALENEG(tempamp * pan_table[64], 27);
		}
		else if (voice[v].panning < 2)
		{
			voice[v].panned = PANNED_LEFT;
			voice[v].left_amp = TIM_FSCALENEG(tempamp, 20);
			voice[v].right_amp = 0;
		}
		else if(voice[v].panning == 127)
		{
#ifdef SMOOTH_MIXING
			if(voice[v].panned == PANNED_MYSTERY) {
				voice[v].old_left_mix = voice[v].old_right_mix;
				voice[v].old_right_mix = 0;
			}
#endif
			voice[v].panned = PANNED_RIGHT;
			voice[v].left_amp =  TIM_FSCALENEG(tempamp, 20);
			voice[v].right_amp = 0;
		}
		else
		{
#ifdef SMOOTH_MIXING
			if(voice[v].panned == PANNED_RIGHT) {
				voice[v].old_right_mix = voice[v].old_left_mix;
				voice[v].old_left_mix = 0;
			}
#endif
			voice[v].panned = PANNED_MYSTERY;
			voice[v].left_amp = TIM_FSCALENEG(tempamp * pan_table[128 - voice[v].panning], 27);
			voice[v].right_amp = TIM_FSCALENEG(tempamp * pan_table[voice[v].panning], 27);
		}
    	}
    	else
    	{
		voice[v].panned = PANNED_CENTER;
		voice[v].left_amp = TIM_FSCALENEG(tempamp, 21);
    	}
}

#define RESONANCE_COEFF 0.2393

void recompute_channel_filter(int ch, int note)
{
	double coef = 1.0f, reso = 0;

	if(channel[ch].special_sample > 0) {return;}

	/* Soft Pedal */
	if(channel[ch].soft_pedal != 0) {
		if(note > 49) {	/* tre corde */
			coef *= 1.0 - 0.20 * ((double)channel[ch].soft_pedal) / 127.0f;
		} else {	/* una corda (due corde) */
			coef *= 1.0 - 0.25 * ((double)channel[ch].soft_pedal) / 127.0f;
		}
	}

	if(!ISDRUMCHANNEL(ch)) {
		/* NRPN Filter Cutoff */
		coef *= pow(1.26, (double)(channel[ch].param_cutoff_freq) / 8.0f);
		/* NRPN Resonance */
		reso = (double)channel[ch].param_resonance * RESONANCE_COEFF;
	}

	channel[ch].cutoff_freq_coef = coef;
	channel[ch].resonance_dB = reso;
}

void init_voice_filter(int i)
{
  memset(&(voice[i].fc), 0, sizeof(FilterCoefficients));
  if(opt_lpf_def && voice[i].sample->cutoff_freq) {
	  voice[i].fc.orig_freq = voice[i].sample->cutoff_freq;
	  voice[i].fc.orig_reso_dB = (double)voice[i].sample->resonance / 10.0f - 3.01f;
	  if (voice[i].fc.orig_reso_dB < 0.0f) {voice[i].fc.orig_reso_dB = 0.0f;}
	  if (opt_lpf_def == 2) {
		  voice[i].fc.gain = 1.0;
		  voice[i].fc.type = 2;
	  } else if(opt_lpf_def == 1) {
		  voice[i].fc.gain = pow(10.0f, -voice[i].fc.orig_reso_dB / 2.0f / 20.0f);
		  voice[i].fc.type = 1;
	  }
	  voice[i].fc.start_flag = 0;
  } else {
	  voice[i].fc.type = 0;
  }
}

#define CHAMBERLIN_RESONANCE_MAX 24.0

void recompute_voice_filter(int v)
{
	int ch = voice[v].channel, note = voice[v].note;
	double coef = 1.0, reso = 0, cent = 0, depth_cent = 0, freq;
	FilterCoefficients *fc = &(voice[v].fc);
	Sample *sp = (Sample *) &voice[v].sample;

	if(fc->type == 0) {return;}
	coef = channel[ch].cutoff_freq_coef;

	if(ISDRUMCHANNEL(ch) && channel[ch].drums[note] != NULL) {
		/* NRPN Drum Instrument Filter Cutoff */
		coef *= pow(1.26, (double)(channel[ch].drums[note]->drum_cutoff_freq) / 8.0f);
		/* NRPN Drum Instrument Filter Resonance */
		reso += (double)channel[ch].drums[note]->drum_resonance * RESONANCE_COEFF;
	}

	/* MIDI controllers filter cutoff control and LFO filter depth */
	if(opt_channel_pressure) {
		cent += get_midi_controller_filter_cutoff(&(channel[ch].mod))
			+ get_midi_controller_filter_cutoff(&(channel[ch].bend))
			+ get_midi_controller_filter_cutoff(&(channel[ch].caf))
			+ get_midi_controller_filter_cutoff(&(channel[ch].paf))
			+ get_midi_controller_filter_cutoff(&(channel[ch].cc1))
			+ get_midi_controller_filter_cutoff(&(channel[ch].cc2));
		depth_cent += get_midi_controller_filter_depth(&(channel[ch].mod))
			+ get_midi_controller_filter_depth(&(channel[ch].bend))
			+ get_midi_controller_filter_depth(&(channel[ch].caf))
			+ get_midi_controller_filter_depth(&(channel[ch].paf))
			+ get_midi_controller_filter_depth(&(channel[ch].cc1))
			+ get_midi_controller_filter_depth(&(channel[ch].cc2));
	}

	if(sp->vel_to_fc) {	/* velocity to filter cutoff frequency */
		if(voice[v].velocity > sp->vel_to_fc_threshold)
			cent += sp->vel_to_fc * (double)(127 - voice[v].velocity) / 127.0f;
		else
			coef += sp->vel_to_fc * (double)(127 - sp->vel_to_fc_threshold) / 127.0f;
	}
	if(sp->vel_to_resonance) {	/* velocity to filter resonance */
		reso += (double)voice[v].velocity * sp->vel_to_resonance / 127.0f / 10.0f;
	}
	if(sp->key_to_fc) {	/* filter cutoff key-follow */
		cent += sp->key_to_fc * (double)(voice[v].note - sp->key_to_fc_bpo);
	}

	if(opt_modulation_envelope) {
		if(voice[v].sample->tremolo_to_fc + (int16)depth_cent) {
			cent += ((double)voice[v].sample->tremolo_to_fc + depth_cent) * lookup_triangular(voice[v].tremolo_phase >> RATE_SHIFT);
		}
		if(voice[v].sample->modenv_to_fc) {
			cent += (double)voice[v].sample->modenv_to_fc * voice[v].last_modenv_volume;
		}
	}

	if(cent != 0) {coef *= pow(2.0, cent / 1200.0f);}

	freq = (double)fc->orig_freq * coef;

	if (freq > play_mode->rate / 2) {freq = play_mode->rate / 2;}
	else if(freq < 5) {freq = 5;}
	else if(freq > 20000) {freq = 20000;}
	fc->freq = (int32)freq;

	fc->reso_dB = fc->orig_reso_dB + channel[ch].resonance_dB + reso;
	if(fc->reso_dB < 0.0f) {fc->reso_dB = 0.0f;}
	else if(fc->reso_dB > 96.0f) {fc->reso_dB = 96.0f;}

	if(fc->type == 1) {	/* Chamberlin filter */
		if(fc->freq > play_mode->rate / 6) {
			if (fc->start_flag == 0) {fc->type = 0;}	/* turn off. */ 
			else {fc->freq = play_mode->rate / 6;}
		}
		if(fc->reso_dB > CHAMBERLIN_RESONANCE_MAX) {fc->reso_dB = CHAMBERLIN_RESONANCE_MAX;}
	} else if(fc->type == 2) {	/* Moog VCF */
		if(fc->reso_dB > fc->orig_reso_dB / 2) {
			fc->gain = pow(10.0f, (fc->reso_dB - fc->orig_reso_dB / 2) / 20.0f);
		}
	}
	fc->start_flag = 1;	/* filter is started. */
}

float calc_drum_tva_level(int ch, int note, int level)
{
	int def_level, nbank, nprog;
	ToneBank *bank;

	if(channel[ch].special_sample > 0) {return 1.0;}

	nbank = channel[ch].bank;
	nprog = note;
	instrument_map(channel[ch].mapID, &nbank, &nprog);

	if(ISDRUMCHANNEL(ch)) {
		bank = drumset[nbank];
		if(bank == NULL) {bank = drumset[0];}
	} else {
		return 1.0;
	}

	def_level = bank->tone[nprog].tva_level;

	if(def_level == -1 || def_level == 0) {def_level = 127;}
	else if(def_level > 127) {def_level = 127;}

	return (sc_drum_level_table[level] / sc_drum_level_table[def_level]);
}

static int32 calc_random_delay(int ch, int note)
{
	int nbank, nprog;
	ToneBank *bank;

	if(channel[ch].special_sample > 0) {return 0;}

	nbank = channel[ch].bank;

	if(ISDRUMCHANNEL(ch)) {
		nprog = note;
		instrument_map(channel[ch].mapID, &nbank, &nprog);
		bank = drumset[nbank];
		if (bank == NULL) {bank = drumset[0];}
	} else {
		nprog = channel[ch].program;
		if(nprog == SPECIAL_PROGRAM) {return 0;}
		instrument_map(channel[ch].mapID, &nbank, &nprog);
		bank = tonebank[nbank];
		if(bank == NULL) {bank = tonebank[0];}
	}

	if (bank->tone[nprog].rnddelay == 0) {return 0;}
	else {return (int32)((double)bank->tone[nprog].rnddelay * play_mode->rate / 1000.0
		* (get_pink_noise_light(&global_pink_noise_light) + 1.0f) * 0.5);}
}

void recompute_bank_parameter(int ch, int note)
{
	int nbank, nprog;
	ToneBank *bank;
	struct DrumParts *drum;

	if(channel[ch].special_sample > 0) {return;}

	nbank = channel[ch].bank;

	if(ISDRUMCHANNEL(ch)) {
		nprog = note;
		instrument_map(channel[ch].mapID, &nbank, &nprog);
		bank = drumset[nbank];
		if (bank == NULL) {bank = drumset[0];}
		if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
		drum = channel[ch].drums[note];
		if (drum->reverb_level == -1 && bank->tone[nprog].reverb_send != -1) {
			drum->reverb_level = bank->tone[nprog].reverb_send;
		}
		if (drum->chorus_level == -1 && bank->tone[nprog].chorus_send != -1) {
			drum->chorus_level = bank->tone[nprog].chorus_send;
		}
		if (drum->delay_level == -1 && bank->tone[nprog].delay_send != -1) {
			drum->delay_level = bank->tone[nprog].delay_send;
		}
	} else {
		nprog = channel[ch].program;
		if (nprog == SPECIAL_PROGRAM) {return;}
		instrument_map(channel[ch].mapID, &nbank, &nprog);
		bank = tonebank[nbank];
		if (bank == NULL) {bank = tonebank[0];}
		channel[ch].legato = bank->tone[nprog].legato;
		channel[ch].damper_mode = bank->tone[nprog].damper_mode;
		channel[ch].loop_timeout = bank->tone[nprog].loop_timeout;
	}
}

Instrument *play_midi_load_instrument(int dr, int bk, int prog)
{
	ToneBank **bank = (dr) ? drumset : tonebank;
	Instrument *ip;
	int load_success = 0;

	if (bank[bk] == NULL)
		alloc_instrument_bank(dr, bk);

	if (bank[bk]->tone[prog].name) {
		/* Instrument is found. */
		if ((ip = bank[bk]->tone[prog].instrument) == MAGIC_LOAD_INSTRUMENT
#ifndef SUPPRESS_CHANNEL_LAYER
			|| ip == NULL	/* see also readmidi.c: groom_list(). */
#endif
		) {ip = bank[bk]->tone[prog].instrument = load_instrument(dr, bk, prog);}
		if (ip == NULL || IS_MAGIC_INSTRUMENT(ip)) {
			bank[bk]->tone[prog].instrument = MAGIC_ERROR_INSTRUMENT;
		} else {
			load_success = 1;
		}
	} else {
		/* Instrument is not found.
		   Try to load the instrument from bank 0 */
		if ((ip = bank[0]->tone[prog].instrument) == NULL
			|| ip == MAGIC_LOAD_INSTRUMENT)
			ip = bank[0]->tone[prog].instrument = load_instrument(dr, 0, prog);
		if (ip == NULL || IS_MAGIC_INSTRUMENT(ip)) {
			bank[0]->tone[prog].instrument = MAGIC_ERROR_INSTRUMENT;
		} else {
			copy_tone_bank_element(&bank[bk]->tone[prog], &bank[0]->tone[prog]);
			bank[bk]->tone[prog].instrument = ip;
			load_success = 1;
		}
	}

	if (load_success)
		aq_add(NULL, 0);	/* Update software buffer */

	if (ip == MAGIC_ERROR_INSTRUMENT)
		return NULL;

	return ip;
}

#if 0
/* reduce_voice_CPU() may not have any speed advantage over reduce_voice().
 * So this function is not used, now.
 */

/* The goal of this routine is to free as much CPU as possible without
   loosing too much sound quality.  We would like to know how long a note
   has been playing, but since we usually can't calculate this, we guess at
   the value instead.  A bad guess is better than nothing.  Notes which
   have been playing a short amount of time are killed first.  This causes
   decays and notes to be cut earlier, saving more CPU time.  It also causes
   notes which are closer to ending not to be cut as often, so it cuts
   a different note instead and saves more CPU in the long run.  ON voices
   are treated a little differently, since sound quality is more important
   than saving CPU at this point.  Duration guesses for loop regions are very
   crude, but are still better than nothing, they DO help.  Non-looping ON
   notes are cut before looping ON notes.  Since a looping ON note is more
   likely to have been playing for a long time, we want to keep it because it
   sounds better to keep long notes.
*/
static int reduce_voice_CPU(void)
{
    int32 lv, v, vr;
    int i, j, lowest=-0x7FFFFFFF;
    int32 duration;

    i = upper_voices;
    lv = 0x7FFFFFFF;
    
    /* Look for the decaying note with the longest remaining decay time */
    /* Protect drum decays.  They do not take as much CPU (?) and truncating
       them early sounds bad, especially on snares and cymbals */
    for(j = 0; j < i; j++)
    {
	if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
	/* skip notes that don't need resampling (most drums) */
	if (voice[j].sample->note_to_use)
	    continue;
	if(voice[j].status & ~(VOICE_ON | VOICE_DIE | VOICE_SUSTAINED))
	{
	    /* Choose note with longest decay time remaining */
	    /* This frees more CPU than choosing lowest volume */
	    if (!voice[j].envelope_increment) duration = 0;
	    else duration =
	    	(voice[j].envelope_target - voice[j].envelope_volume) /
	    	voice[j].envelope_increment;
	    v = -duration;
	    if(v < lv)
	    {
		lv = v;
		lowest = j;
	    }
	}
    }
    if(lowest != -0x7FFFFFFF)
    {
	/* This can still cause a click, but if we had a free voice to
	   spare for ramping down this note, we wouldn't need to kill it
	   in the first place... Still, this needs to be fixed. Perhaps
	   we could use a reserve of voices to play dying notes only. */

	cut_notes++;
	return lowest;
    }

    /* try to remove VOICE_DIE before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -1;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
      if(voice[j].status & ~(VOICE_ON | VOICE_SUSTAINED))
      {
	/* continue protecting non-resample decays */
	if (voice[j].status & ~(VOICE_DIE) && voice[j].sample->note_to_use)
		continue;

	/* choose note which has been on the shortest amount of time */
	/* this is a VERY crude estimate... */
	if (voice[j].sample->modes & MODES_LOOPING)
	    duration = voice[j].sample_offset - voice[j].sample->loop_start;
	else
	    duration = voice[j].sample_offset;
	if (voice[j].sample_increment > 0)
	    duration /= voice[j].sample_increment;
	v = duration;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -1)
    {
	cut_notes++;
	return lowest;
    }

    /* try to remove VOICE_SUSTAINED before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
      if(voice[j].status & VOICE_SUSTAINED)
      {
	/* choose note which has been on the shortest amount of time */
	/* this is a VERY crude estimate... */
	if (voice[j].sample->modes & MODES_LOOPING)
	    duration = voice[j].sample_offset - voice[j].sample->loop_start;
	else
	    duration = voice[j].sample_offset;
	if (voice[j].sample_increment > 0)
	    duration /= voice[j].sample_increment;
	v = duration;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -0x7FFFFFFF)
    {
	cut_notes++;
	return lowest;
    }

    /* try to remove chorus before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
      if(voice[j].chorus_link < j)
      {
	/* score notes based on both volume AND duration */
	/* this scoring function needs some more tweaking... */
	if (voice[j].sample->modes & MODES_LOOPING)
	    duration = voice[j].sample_offset - voice[j].sample->loop_start;
	else
	    duration = voice[j].sample_offset;
	if (voice[j].sample_increment > 0)
	    duration /= voice[j].sample_increment;
	v = voice[j].left_mix * duration;
	vr = voice[j].right_mix * duration;
	if(voice[j].panned == PANNED_MYSTERY && vr > v)
	    v = vr;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -0x7FFFFFFF)
    {
	cut_notes++;

	/* hack - double volume of chorus partner, fix pan */
	j = voice[lowest].chorus_link;
	voice[j].velocity <<= 1;
    	voice[j].panning = channel[voice[lowest].channel].panning;
    	recompute_amp(j);
    	apply_envelope_to_amp(j);

	return lowest;
    }

    lost_notes++;

    /* try to remove non-looping voices first */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
      if(!(voice[j].sample->modes & MODES_LOOPING))
      {
	/* score notes based on both volume AND duration */
	/* this scoring function needs some more tweaking... */
	duration = voice[j].sample_offset;
	if (voice[j].sample_increment > 0)
	    duration /= voice[j].sample_increment;
	v = voice[j].left_mix * duration;
	vr = voice[j].right_mix * duration;
	if(voice[j].panned == PANNED_MYSTERY && vr > v)
	    v = vr;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -0x7FFFFFFF)
    {
	return lowest;
    }

    lv = 0x7FFFFFFF;
    lowest = 0;
    for(j = 0; j < i; j++)
    {
	if(voice[j].status & VOICE_FREE || voice[j].cache != NULL)
	    continue;
	if (!(voice[j].sample->modes & MODES_LOOPING)) continue;

	/* score notes based on both volume AND duration */
	/* this scoring function needs some more tweaking... */
	duration = voice[j].sample_offset - voice[j].sample->loop_start;
	if (voice[j].sample_increment > 0)
	    duration /= voice[j].sample_increment;
	v = voice[j].left_mix * duration;
	vr = voice[j].right_mix * duration;
	if(voice[j].panned == PANNED_MYSTERY && vr > v)
	    v = vr;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
    }

    return lowest;
}
#endif

/* this reduces voices while maintaining sound quality */
static int reduce_voice(void)
{
    int32 lv, v;
    int i, j, lowest=-0x7FFFFFFF;

    i = upper_voices;
    lv = 0x7FFFFFFF;
    
    /* Look for the decaying note with the smallest volume */
    /* Protect drum decays.  Truncating them early sounds bad, especially on
       snares and cymbals */
    for(j = 0; j < i; j++)
    {
	if(voice[j].status & VOICE_FREE ||
	   (voice[j].sample->note_to_use && ISDRUMCHANNEL(voice[j].channel)))
	    continue;
	
	if(voice[j].status & ~(VOICE_ON | VOICE_DIE | VOICE_SUSTAINED))
	{
	    /* find lowest volume */
	    v = voice[j].left_mix;
	    if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    	v = voice[j].right_mix;
	    if(v < lv)
	    {
		lv = v;
		lowest = j;
	    }
	}
    }
    if(lowest != -0x7FFFFFFF)
    {
	/* This can still cause a click, but if we had a free voice to
	   spare for ramping down this note, we wouldn't need to kill it
	   in the first place... Still, this needs to be fixed. Perhaps
	   we could use a reserve of voices to play dying notes only. */

	cut_notes++;
	free_voice(lowest);
	if(!prescanning_flag)
	    ctl_note_event(lowest);
	return lowest;
    }

    /* try to remove VOICE_DIE before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -1;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE)
	    continue;
      if(voice[j].status & ~(VOICE_ON | VOICE_SUSTAINED))
      {
	/* continue protecting drum decays */
	if (voice[j].status & ~(VOICE_DIE) &&
	    (voice[j].sample->note_to_use && ISDRUMCHANNEL(voice[j].channel)))
		continue;
	/* find lowest volume */
	v = voice[j].left_mix;
	if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    v = voice[j].right_mix;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -1)
    {
	cut_notes++;
	free_voice(lowest);
	if(!prescanning_flag)
	    ctl_note_event(lowest);
	return lowest;
    }

    /* try to remove VOICE_SUSTAINED before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE)
	    continue;
      if(voice[j].status & VOICE_SUSTAINED)
      {
	/* find lowest volume */
	v = voice[j].left_mix;
	if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    v = voice[j].right_mix;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -0x7FFFFFFF)
    {
	cut_notes++;
	free_voice(lowest);
	if(!prescanning_flag)
	    ctl_note_event(lowest);
	return lowest;
    }

    /* try to remove chorus before VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
      if(voice[j].status & VOICE_FREE)
	    continue;
      if(voice[j].chorus_link < j)
      {
	/* find lowest volume */
	v = voice[j].left_mix;
	if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    v = voice[j].right_mix;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
      }
    }
    if(lowest != -0x7FFFFFFF)
    {
	cut_notes++;

	/* hack - double volume of chorus partner, fix pan */
	j = voice[lowest].chorus_link;
	voice[j].velocity <<= 1;
    	voice[j].panning = channel[voice[lowest].channel].panning;
    	recompute_amp(j);
    	apply_envelope_to_amp(j);

	free_voice(lowest);
	if(!prescanning_flag)
	    ctl_note_event(lowest);
	return lowest;
    }

    lost_notes++;

    /* remove non-drum VOICE_ON */
    lv = 0x7FFFFFFF;
    lowest = -0x7FFFFFFF;
    for(j = 0; j < i; j++)
    {
        if(voice[j].status & VOICE_FREE ||
	   (voice[j].sample->note_to_use && ISDRUMCHANNEL(voice[j].channel)))
	   	continue;

	/* find lowest volume */
	v = voice[j].left_mix;
	if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    v = voice[j].right_mix;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
    }
    if(lowest != -0x7FFFFFFF)
    {
	free_voice(lowest);
	if(!prescanning_flag)
	    ctl_note_event(lowest);
	return lowest;
    }

    /* remove all other types of notes */
    lv = 0x7FFFFFFF;
    lowest = 0;
    for(j = 0; j < i; j++)
    {
	if(voice[j].status & VOICE_FREE)
	    continue;
	/* find lowest volume */
	v = voice[j].left_mix;
	if(voice[j].panned == PANNED_MYSTERY && voice[j].right_mix > v)
	    v = voice[j].right_mix;
	if(v < lv)
	{
	    lv = v;
	    lowest = j;
	}
    }

    free_voice(lowest);
    if(!prescanning_flag)
	ctl_note_event(lowest);
    return lowest;
}

void free_voice(int v1)
{
    int v2;

#ifdef ENABLE_PAN_DELAY
	if (voice[v1].pan_delay_buf != NULL) {
		free(voice[v1].pan_delay_buf);
		voice[v1].pan_delay_buf = NULL;
	}
#endif /* ENABLE_PAN_DELAY */

    v2 = voice[v1].chorus_link;
    if(v1 != v2)
    {
	/* Unlink chorus link */
	voice[v1].chorus_link = v1;
	voice[v2].chorus_link = v2;
    }
    voice[v1].status = VOICE_FREE;
    voice[v1].temper_instant = 0;
}

static int find_free_voice(void)
{
    int i, nv = voices, lowest;
    int32 lv, v;

    for(i = 0; i < nv; i++)
	if(voice[i].status == VOICE_FREE)
	{
	    if(upper_voices <= i)
		upper_voices = i + 1;
	    return i;
	}

    upper_voices = voices;

    /* Look for the decaying note with the lowest volume */
    lv = 0x7FFFFFFF;
    lowest = -1;
    for(i = 0; i < nv; i++)
    {
	if(voice[i].status & ~(VOICE_ON | VOICE_DIE) &&
	   !(voice[i].sample && voice[i].sample->note_to_use && ISDRUMCHANNEL(voice[i].channel)))
	{
	    v = voice[i].left_mix;
	    if((voice[i].panned==PANNED_MYSTERY) && (voice[i].right_mix>v))
		v = voice[i].right_mix;
	    if(v<lv)
	    {
		lv = v;
		lowest = i;
	    }
	}
    }
    if(lowest != -1 && !prescanning_flag)
    {
	free_voice(lowest);
	ctl_note_event(lowest);
    }
    return lowest;
}

static int find_samples(MidiEvent *e, int *vlist)
{
	int i, j, ch, bank, prog, note, nv;
	SpecialPatch *s;
	Instrument *ip;
	
	ch = e->channel;
	if (channel[ch].special_sample > 0) {
		if ((s = special_patch[channel[ch].special_sample]) == NULL) {
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
					"Strange: Special patch %d is not installed",
					channel[ch].special_sample);
			return 0;
		}
		note = e->a + channel[ch].key_shift + note_key_offset;
		note = (note < 0) ? 0 : ((note > 127) ? 127 : note);
		return select_play_sample(s->sample, s->samples, &note, vlist, e);
	}
	bank = channel[ch].bank;
	if (ISDRUMCHANNEL(ch)) {
		note = e->a & 0x7f;
		instrument_map(channel[ch].mapID, &bank, &note);
		if (! (ip = play_midi_load_instrument(1, bank, note)))
			return 0;	/* No instrument? Then we can't play. */
		/* if (ip->type == INST_GUS && ip->samples != 1)
			ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
					"Strange: percussion instrument with %d samples!",
					ip->samples); */
		/* "keynum" of SF2, and patch option "note=" */
		if (ip->sample->note_to_use)
			note = ip->sample->note_to_use;
	} else {
		if ((prog = channel[ch].program) == SPECIAL_PROGRAM)
			ip = default_instrument;
		else {
			instrument_map(channel[ch].mapID, &bank, &prog);
			if (! (ip = play_midi_load_instrument(0, bank, prog)))
				return 0;	/* No instrument? Then we can't play. */
		}
		note = ((ip->sample->note_to_use) ? ip->sample->note_to_use : e->a)
				+ channel[ch].key_shift + note_key_offset;
		note = (note < 0) ? 0 : ((note > 127) ? 127 : note);
	}
	nv = select_play_sample(ip->sample, ip->samples, &note, vlist, e);
	/* Replace the sample if the sample is cached. */
	if (! prescanning_flag) {
		if (ip->sample->note_to_use)
			note = MIDI_EVENT_NOTE(e);
		for (i = 0; i < nv; i++) {
			j = vlist[i];
			if (! opt_realtime_playing && allocate_cache_size > 0
					&& ! channel[ch].portamento) {
				voice[j].cache = resamp_cache_fetch(voice[j].sample, note);
				if (voice[j].cache)	/* cache hit */
					voice[j].sample = voice[j].cache->resampled;
			} else
				voice[j].cache = NULL;
		}
	}
	return nv;
}

static int select_play_sample(Sample *splist,
		int nsp, int *note, int *vlist, MidiEvent *e)
{
	int ch = e->channel, kn = e->a & 0x7f, vel = e->b;
	int32 f, fs, ft, fst, fc, fr, cdiff, diff, sample_link;
	int8 tt = channel[ch].temper_type;
	uint8 tp = channel[ch].rpnmap[RPN_ADDR_0003];
	Sample *sp, *spc, *spr;
	int16 sf, sn;
	double ratio;
	int i, j, k, nv, nvc;
	
	if (ISDRUMCHANNEL(ch))
		f = fs = freq_table[*note];
	else {
		if (opt_pure_intonation) {
			if (current_keysig < 8)
				f = freq_table_pureint[current_freq_table][*note];
			else
				f = freq_table_pureint[current_freq_table + 12][*note];
		} else if (opt_temper_control)
			switch (tt) {
			case 0:
				f = freq_table_tuning[tp][*note];
				break;
			case 1:
				if (current_temper_keysig < 8)
					f = freq_table_pytha[
							current_temper_freq_table][*note];
				else
					f = freq_table_pytha[
							current_temper_freq_table + 12][*note];
				break;
			case 2:
				if (current_temper_keysig < 8)
					f = freq_table_meantone[current_temper_freq_table
							+ ((temper_adj) ? 36 : 0)][*note];
				else
					f = freq_table_meantone[current_temper_freq_table
							+ ((temper_adj) ? 24 : 12)][*note];
				break;
			case 3:
				if (current_temper_keysig < 8)
					f = freq_table_pureint[current_temper_freq_table
							+ ((temper_adj) ? 36 : 0)][*note];
				else
					f = freq_table_pureint[current_temper_freq_table
							+ ((temper_adj) ? 24 : 12)][*note];
				break;
			default:	/* user-defined temperament */
				if ((tt -= 0x40) >= 0 && tt < 4) {
					if (current_temper_keysig < 8)
						f = freq_table_user[tt][current_temper_freq_table
								+ ((temper_adj) ? 36 : 0)][*note];
					else
						f = freq_table_user[tt][current_temper_freq_table
								+ ((temper_adj) ? 24 : 12)][*note];
				} else
					f = freq_table[*note];
				break;
			}
		else
			f = freq_table[*note];
		if (! opt_pure_intonation && opt_temper_control
				&& tt == 0 && f != freq_table[*note]) {
			*note = log(f / 440000.0) / log(2) * 12 + 69.5;
			*note = (*note < 0) ? 0 : ((*note > 127) ? 127 : *note);
			fs = freq_table[*note];
		} else
			fs = freq_table[*note];
	}
	nv = 0;
	for (i = 0, sp = splist; i < nsp; i++, sp++) {
		/* GUS/SF2 - Scale Tuning */
		if ((sf = sp->scale_factor) != 1024) {
			sn = sp->scale_freq;
			ratio = pow(2.0, (*note - sn) * (sf - 1024) / 12288.0);
			ft = f * ratio + 0.5, fst = fs * ratio + 0.5;
		} else
			ft = f, fst = fs;
		if (ISDRUMCHANNEL(ch) && channel[ch].drums[kn] != NULL)
			if ((ratio = get_play_note_ratio(ch, kn)) != 1.0)
				ft = ft * ratio + 0.5, fst = fst * ratio + 0.5;
		if (sp->low_freq <= fst && sp->high_freq >= fst
				&& sp->low_vel <= vel && sp->high_vel >= vel
				&& ! (sp->inst_type == INST_SF2
				&& sp->sample_type == SF_SAMPLETYPE_RIGHT)) {
			j = vlist[nv] = find_voice(e);
			voice[j].orig_frequency = ft;
			MYCHECK(voice[j].orig_frequency);
			voice[j].sample = sp;
			voice[j].status = VOICE_ON;
			nv++;
		}
	}
	if (nv == 0) {	/* we must select at least one sample. */
		fr = fc = 0;
		spc = spr = NULL;
		cdiff = 0x7fffffff;
		for (i = 0, sp = splist; i < nsp; i++, sp++) {
			/* GUS/SF2 - Scale Tuning */
			if ((sf = sp->scale_factor) != 1024) {
				sn = sp->scale_freq;
				ratio = pow(2.0, (*note - sn) * (sf - 1024) / 12288.0);
				ft = f * ratio + 0.5, fst = fs * ratio + 0.5;
			} else
				ft = f, fst = fs;
			if (ISDRUMCHANNEL(ch) && channel[ch].drums[kn] != NULL)
				if ((ratio = get_play_note_ratio(ch, kn)) != 1.0)
					ft = ft * ratio + 0.5, fst = fst * ratio + 0.5;
			diff = abs(sp->root_freq - fst);
			if (diff < cdiff) {
				if (sp->inst_type == INST_SF2
						&& sp->sample_type == SF_SAMPLETYPE_RIGHT) {
					fr = ft;	/* reserve */
					spr = sp;	/* reserve */
				} else {
					fc = ft;
					spc = sp;
					cdiff = diff;
				}
			}
		}
		/* If spc is not NULL, a makeshift sample is found. */
		/* Otherwise, it's a lonely right sample, but better than nothing. */
		j = vlist[nv] = find_voice(e);
		voice[j].orig_frequency = (spc) ? fc : fr;
		MYCHECK(voice[j].orig_frequency);
		voice[j].sample = (spc) ? spc : spr;
		voice[j].status = VOICE_ON;
		nv++;
	}
	nvc = nv;
	for (i = 0; i < nvc; i++) {
		spc = voice[vlist[i]].sample;
		/* If it's left sample, there must be right sample. */
		if (spc->inst_type == INST_SF2
				&& spc->sample_type == SF_SAMPLETYPE_LEFT) {
			sample_link = spc->sf_sample_link;
			for (j = 0, sp = splist; j < nsp; j++, sp++)
				if (sp->inst_type == INST_SF2
						&& sp->sample_type == SF_SAMPLETYPE_RIGHT
						&& sp->sf_sample_index == sample_link) {
					/* right sample is found. */
					/* GUS/SF2 - Scale Tuning */
					if ((sf = sp->scale_factor) != 1024) {
						sn = sp->scale_freq;
						ratio = pow(2.0, (*note - sn) * (sf - 1024) / 12288.0);
						ft = f * ratio + 0.5;
					} else
						ft = f;
					if (ISDRUMCHANNEL(ch) && channel[ch].drums[kn] != NULL)
						if ((ratio = get_play_note_ratio(ch, kn)) != 1.0)
							ft = ft * ratio + 0.5;
					k = vlist[nv] = find_voice(e);
					voice[k].orig_frequency = ft;
					MYCHECK(voice[k].orig_frequency);
					voice[k].sample = sp;
					voice[k].status = VOICE_ON;
					nv++;
					break;
				}
		}
	}
	return nv;
}

static double get_play_note_ratio(int ch, int note)
{
	int play_note = channel[ch].drums[note]->play_note;
	int bank = channel[ch].bank;
	ToneBank *dbank;
	int def_play_note;
	
	if (play_note == -1)
		return 1.0;
	instrument_map(channel[ch].mapID, &bank, &note);
	dbank = (drumset[bank]) ? drumset[bank] : drumset[0];
	if ((def_play_note = dbank->tone[note].play_note) == -1)
		return 1.0;
	if (play_note >= def_play_note)
		return bend_coarse[(play_note - def_play_note) & 0x7f];
	else
		return 1 / bend_coarse[(def_play_note - play_note) & 0x7f];
}

/* Only one instance of a note can be playing on a single channel. */
static int find_voice(MidiEvent *e)
{
	int ch = e->channel;
	int note = MIDI_EVENT_NOTE(e);
	int status_check, mono_check;
	AlternateAssign *altassign;
	int i, lowest = -1;
	
	status_check = (opt_overlap_voice_allow)
			? (VOICE_OFF | VOICE_SUSTAINED) : 0xff;
	mono_check = channel[ch].mono;
	altassign = find_altassign(channel[ch].altassign, note);
	for (i = 0; i < upper_voices; i++)
		if (voice[i].status == VOICE_FREE) {
			lowest = i;	/* lower volume */
			break;
		}
	for (i = 0; i < upper_voices; i++)
		if (voice[i].status != VOICE_FREE && voice[i].channel == ch) {
			if ((voice[i].note == note && (voice[i].status & status_check))
			    || mono_check
			    || (altassign && find_altassign(altassign, voice[i].note)))
				kill_note(i);
			else if (voice[i].note == note &&
				 (channel[ch].assign_mode == 0
				  || (channel[ch].assign_mode == 1
				      && voice[i].proximate_flag == 0)))
				kill_note(i);
		}
	for (i = 0; i < upper_voices; i++)
		if (voice[i].channel == ch && voice[i].note == note)
			voice[i].proximate_flag = 0;
	if (lowest != -1)	/* Found a free voice. */
		return lowest;
	if (upper_voices < voices)
		return upper_voices++;
	return reduce_voice();
}

int32 get_note_freq(Sample *sp, int note)
{
	int32 f;
	int16 sf, sn;
	double ratio;
	
	f = freq_table[note];
	/* GUS/SF2 - Scale Tuning */
	if ((sf = sp->scale_factor) != 1024) {
		sn = sp->scale_freq;
		ratio = pow(2.0, (note - sn) * (sf - 1024) / 12288.0);
		f = f * ratio + 0.5;
	}
	return f;
}

static int get_panning(int ch, int note,int v)
{
    int pan;

	if(channel[ch].panning != NO_PANNING) {pan = (int)channel[ch].panning - 64;}
	else {pan = 0;}
	if(ISDRUMCHANNEL(ch) &&
	 channel[ch].drums[note] != NULL &&
	 channel[ch].drums[note]->drum_panning != NO_PANNING) {
		pan += channel[ch].drums[note]->drum_panning;
	} else {
		pan += voice[v].sample->panning;
	}

	if (pan > 127) pan = 127;
	else if (pan < 0) pan = 0;

	return pan;
}

/*! initialize vibrato parameters for a voice. */
static void init_voice_vibrato(int v)
{
	Voice *vp = &(voice[v]);
	int ch = vp->channel, j, nrpn_vib_flag;
	double ratio;

	/* if NRPN vibrato is set, it's believed that there must be vibrato. */
	nrpn_vib_flag = opt_nrpn_vibrato
		&& (channel[ch].vibrato_ratio != 1.0 || channel[ch].vibrato_depth != 0);
	
	/* vibrato sweep */
	vp->vibrato_sweep = vp->sample->vibrato_sweep_increment;
	vp->vibrato_sweep_position = 0;

	/* vibrato rate */
	if (nrpn_vib_flag) {
		if(vp->sample->vibrato_control_ratio == 0) {
			ratio = cnv_Hz_to_vib_ratio(5.0) * channel[ch].vibrato_ratio;
		} else {
			ratio = (double)vp->sample->vibrato_control_ratio * channel[ch].vibrato_ratio;
		}
		if (ratio < 0) {ratio = 0;}
		vp->vibrato_control_ratio = (int)ratio;
	} else {
		vp->vibrato_control_ratio = vp->sample->vibrato_control_ratio;
	}
	
	/* vibrato depth */
	if (nrpn_vib_flag) {
		vp->vibrato_depth = vp->sample->vibrato_depth + channel[ch].vibrato_depth;
		if (vp->vibrato_depth > VIBRATO_DEPTH_MAX) {vp->vibrato_depth = VIBRATO_DEPTH_MAX;}
		else if (vp->vibrato_depth < 1) {vp->vibrato_depth = 1;}
		if (vp->sample->vibrato_depth < 0) {	/* in opposite phase */
			vp->vibrato_depth = -vp->vibrato_depth;
		}
	} else {
		vp->vibrato_depth = vp->sample->vibrato_depth;
	}
	
	/* vibrato delay */
	vp->vibrato_delay = vp->sample->vibrato_delay + channel[ch].vibrato_delay;
	
	/* internal parameters */
	vp->orig_vibrato_control_ratio = vp->vibrato_control_ratio;
	vp->vibrato_control_counter = vp->vibrato_phase = 0;
	for (j = 0; j < VIBRATO_SAMPLE_INCREMENTS; j++) {
		vp->vibrato_sample_increment[j] = 0;
	}
}

/*! initialize panning-delay for a voice. */
static void init_voice_pan_delay(int v)
{
#ifdef ENABLE_PAN_DELAY
	Voice *vp = &(voice[v]);
	int ch = vp->channel;
	double pan_delay_diff; 

	if (vp->pan_delay_buf != NULL) {
		free(vp->pan_delay_buf);
		vp->pan_delay_buf = NULL;
	}
	vp->pan_delay_rpt = 0;
	if (opt_pan_delay && channel[ch].insertion_effect == 0 && !opt_surround_chorus) {
		if (vp->panning == 64) {vp->delay += pan_delay_table[64] * play_mode->rate / 1000;}
		else {
			if(pan_delay_table[vp->panning] > pan_delay_table[127 - vp->panning]) {
				pan_delay_diff = pan_delay_table[vp->panning] - pan_delay_table[127 - vp->panning];
				vp->delay += (pan_delay_table[vp->panning] - pan_delay_diff) * play_mode->rate / 1000;
			} else {
				pan_delay_diff = pan_delay_table[127 - vp->panning] - pan_delay_table[vp->panning];
				vp->delay += (pan_delay_table[127 - vp->panning] - pan_delay_diff) * play_mode->rate / 1000;
			}
			vp->pan_delay_rpt = pan_delay_diff * play_mode->rate / 1000;
		}
		if(vp->pan_delay_rpt < 1) {vp->pan_delay_rpt = 0;}
		vp->pan_delay_wpt = 0;
		vp->pan_delay_spt = vp->pan_delay_wpt - vp->pan_delay_rpt;
		if (vp->pan_delay_spt < 0) {vp->pan_delay_spt += PAN_DELAY_BUF_MAX;}
		vp->pan_delay_buf = (int32 *)safe_malloc(sizeof(int32) * PAN_DELAY_BUF_MAX);
		memset(vp->pan_delay_buf, 0, sizeof(int32) * PAN_DELAY_BUF_MAX);
	}
#endif	/* ENABLE_PAN_DELAY */
}

/*! initialize portamento or legato for a voice. */
static void init_voice_portamento(int v)
{
	Voice *vp = &(voice[v]);
	int ch = vp->channel;

  vp->porta_control_counter = 0;
  if(channel[ch].legato && channel[ch].legato_flag) {
	  update_legato_controls(ch);
  } else if(channel[ch].portamento && !channel[ch].porta_control_ratio) {
      update_portamento_controls(ch);
  }
  vp->porta_control_ratio = 0;
  if(channel[ch].porta_control_ratio)
  {
      if(channel[ch].last_note_fine == -1) {
	  /* first on */
	  channel[ch].last_note_fine = vp->note * 256;
	  channel[ch].porta_control_ratio = 0;
      } else {
	  vp->porta_control_ratio = channel[ch].porta_control_ratio;
	  vp->porta_dpb = channel[ch].porta_dpb;
	  vp->porta_pb = channel[ch].last_note_fine -
	      vp->note * 256;
	  if(vp->porta_pb == 0) {vp->porta_control_ratio = 0;}
      }
  }
}

/*! initialize tremolo for a voice. */
static void init_voice_tremolo(int v)
{
	Voice *vp = &(voice[v]);

  vp->tremolo_delay = vp->sample->tremolo_delay;
  vp->tremolo_phase = 0;
  vp->tremolo_phase_increment = vp->sample->tremolo_phase_increment;
  vp->tremolo_sweep = vp->sample->tremolo_sweep_increment;
  vp->tremolo_sweep_position = 0;
  vp->tremolo_depth = vp->sample->tremolo_depth;
}

static void start_note(MidiEvent *e, int i, int vid, int cnt)
{
  int j, ch, note;

  ch = e->channel;

  note = MIDI_EVENT_NOTE(e);
  voice[i].status = VOICE_ON;
  voice[i].channel = ch;
  voice[i].note = note;
  voice[i].velocity = e->b;
  voice[i].chorus_link = i;	/* No link */
  voice[i].proximate_flag = 1;

  j = channel[ch].special_sample;
  if(j == 0 || special_patch[j] == NULL)
      voice[i].sample_offset = 0;
  else
  {
      voice[i].sample_offset =
	  special_patch[j]->sample_offset << FRACTION_BITS;
      if(voice[i].sample->modes & MODES_LOOPING)
      {
	  if(voice[i].sample_offset > voice[i].sample->loop_end)
	      voice[i].sample_offset = voice[i].sample->loop_start;
      }
      else if(voice[i].sample_offset > voice[i].sample->data_length)
      {
	  free_voice(i);
	  return;
      }
  }
  voice[i].sample_increment = 0; /* make sure it isn't negative */
  voice[i].vid = vid;
  voice[i].delay = voice[i].sample->envelope_delay;
  voice[i].modenv_delay = voice[i].sample->modenv_delay;
  voice[i].delay_counter = 0;

  init_voice_tremolo(i);	/* tremolo */
  init_voice_filter(i);		/* resonant lowpass filter */
  init_voice_vibrato(i);	/* vibrato */
  voice[i].panning = get_panning(ch, note, i);	/* pan */
  init_voice_pan_delay(i);	/* panning-delay */
  init_voice_portamento(i);	/* portamento or legato */

  if(cnt == 0)
      channel[ch].last_note_fine = voice[i].note * 256;

  /* initialize modulation envelope */
  if (voice[i].sample->modes & MODES_ENVELOPE)
    {
	  voice[i].modenv_stage = EG_GUS_ATTACK;
      voice[i].modenv_volume = 0;
      recompute_modulation_envelope(i);
	  apply_modulation_envelope(i);
    }
  else
    {
	  voice[i].modenv_increment=0;
	  apply_modulation_envelope(i);
    }
  recompute_freq(i);
  recompute_voice_filter(i);

  recompute_amp(i);
  /* initialize volume envelope */
  if (voice[i].sample->modes & MODES_ENVELOPE)
    {
      /* Ramp up from 0 */
	  voice[i].envelope_stage = EG_GUS_ATTACK;
      voice[i].envelope_volume = 0;
      voice[i].control_counter = 0;
      recompute_envelope(i);
	  apply_envelope_to_amp(i);
    }
  else
    {
      voice[i].envelope_increment = 0;
      apply_envelope_to_amp(i);
    }

  voice[i].timeout = -1;
  if(!prescanning_flag)
      ctl_note_event(i);
}

static void finish_note(int i)
{
    if (voice[i].sample->modes & MODES_ENVELOPE)
    {
		/* We need to get the envelope out of Sustain stage. */
		/* Note that voice[i].envelope_stage < EG_GUS_RELEASE1 */
		voice[i].status = VOICE_OFF;
		voice[i].envelope_stage = EG_GUS_RELEASE1;
		recompute_envelope(i);
		voice[i].modenv_stage = EG_GUS_RELEASE1;
		recompute_modulation_envelope(i);
		apply_modulation_envelope(i);
		apply_envelope_to_amp(i);
		ctl_note_event(i);
	}
    else
    {
		if(current_file_info->pcm_mode != PCM_MODE_NON)
		{
			free_voice(i);
			ctl_note_event(i);
		}
		else
		{
			/* Set status to OFF so resample_voice() will let this voice out
			of its loop, if any. In any case, this voice dies when it
				hits the end of its data (ofs>=data_length). */
			if(voice[i].status != VOICE_OFF)
			{
			voice[i].status = VOICE_OFF;
			ctl_note_event(i);
			}
		}
    }
}

static void set_envelope_time(int ch, int val, int stage)
{
	val = val & 0x7F;
	switch(stage) {
	case EG_ATTACK:	/* Attack */
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"Attack Time (CH:%d VALUE:%d)", ch, val);
		break;
	case EG_DECAY: /* Decay */
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"Decay Time (CH:%d VALUE:%d)", ch, val);
		break;
	case EG_RELEASE:	/* Release */
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"Release Time (CH:%d VALUE:%d)", ch, val);
		break;
	default:
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"? Time (CH:%d VALUE:%d)", ch, val);
	}
	channel[ch].envelope_rate[stage] = val;
}

/*! pseudo Delay Effect without DSP */
#ifndef USE_DSP_EFFECT
static void new_delay_voice(int v1, int level)
{
    int v2, ch = voice[v1].channel;
	FLOAT_T delay, vol;
	FLOAT_T threshold = 1.0;

	/* NRPN Delay Send Level of Drum */
	if(ISDRUMCHANNEL(ch) &&	channel[ch].drums[voice[v1].note] != NULL) {
		level *= (FLOAT_T)channel[ch].drums[voice[v1].note]->delay_level / 127.0;
	}

	vol = voice[v1].velocity * level / 127.0 * delay_status_gs.level_ratio_c;

	if (vol > threshold) {
		delay = 0;
		if((v2 = find_free_voice()) == -1) {return;}
		voice[v2].cache = NULL;
		delay += delay_status_gs.time_center;
		voice[v2] = voice[v1];	/* copy all parameters */
		voice[v2].velocity = (uint8)vol;
		voice[v2].delay += (int32)(play_mode->rate * delay / 1000);
		init_voice_pan_delay(v2);
		recompute_amp(v2);
		apply_envelope_to_amp(v2);
		recompute_freq(v2);
	}
}


static void new_chorus_voice(int v1, int level)
{
    int v2, ch;
    uint8 vol;

    if((v2 = find_free_voice()) == -1)
	return;
    ch = voice[v1].channel;
    vol = voice[v1].velocity;
    voice[v2] = voice[v1];	/* copy all parameters */

	/* NRPN Chorus Send Level of Drum */
	if(ISDRUMCHANNEL(ch) && channel[ch].drums[voice[v1].note] != NULL) {
		level *= (FLOAT_T)channel[ch].drums[voice[v1].note]->chorus_level / 127.0;
	}

    /* Choose lower voice index for base voice (v1) */
    if(v1 > v2)
    {
    	v1 ^= v2;
    	v2 ^= v1;
    	v1 ^= v2;
    }

    /* v1: Base churos voice
     * v2: Sub chorus voice (detuned)
     */

    voice[v1].velocity = (uint8)(vol * CHORUS_VELOCITY_TUNING1);
    voice[v2].velocity = (uint8)(vol * CHORUS_VELOCITY_TUNING2);

    /* Make doubled link v1 and v2 */
    voice[v1].chorus_link = v2;
    voice[v2].chorus_link = v1;

    level >>= 2;		     /* scale level to a "better" value */
    if(channel[ch].pitchbend + level < 0x2000)
        voice[v2].orig_frequency *= bend_fine[level];
    else
	voice[v2].orig_frequency /= bend_fine[level];

    MYCHECK(voice[v2].orig_frequency);

    voice[v2].cache = NULL;

    /* set panning & delay */
    if(!(play_mode->encoding & PE_MONO))
    {
	double delay;

	if(voice[v2].panned == PANNED_CENTER)
	{
	    voice[v2].panning = 64 + int_rand(40) - 20; /* 64 +- rand(20) */
	    delay = 0;
	}
	else
	{
	    int panning = voice[v2].panning;

	    if(panning < CHORUS_OPPOSITE_THRESHOLD)
	    {
		voice[v2].panning = 127;
		delay = DEFAULT_CHORUS_DELAY1;
	    }
	    else if(panning > 127 - CHORUS_OPPOSITE_THRESHOLD)
	    {
		voice[v2].panning = 0;
		delay = DEFAULT_CHORUS_DELAY1;
	    }
	    else
	    {
		voice[v2].panning = (panning < 64 ? 0 : 127);
		delay = DEFAULT_CHORUS_DELAY2;
	    }
	}
	voice[v2].delay += (int)(play_mode->rate * delay);
    }

	init_voice_pan_delay(v1);
	init_voice_pan_delay(v2);

    recompute_amp(v1);
    apply_envelope_to_amp(v1);
    recompute_amp(v2);
    apply_envelope_to_amp(v2);

    /* voice[v2].orig_frequency is changed.
     * Update the depened parameters.
     */
    recompute_freq(v2);
}
#endif /* !USE_DSP_EFFECT */


/* Yet another chorus implementation
 *	by Eric A. Welsh <ewelsh@gpc.wustl.edu>.
 */
static void new_chorus_voice_alternate(int v1, int level)
{
    int v2, ch, panlevel;
    uint8 vol, pan;
    double delay;
    double freq, frac;
    int note_adjusted;

    if((v2 = find_free_voice()) == -1)
	return;
    ch = voice[v1].channel;
    voice[v2] = voice[v1];

    /* NRPN Chorus Send Level of Drum */
    if(ISDRUMCHANNEL(ch) && channel[ch].drums[voice[v1].note] != NULL) {
	level *= (FLOAT_T)channel[ch].drums[voice[v1].note]->chorus_level / 127.0;
    }

    /* for our purposes, hard left will be equal to 1 instead of 0 */
    pan = voice[v1].panning;
    if (!pan) pan = 1;

    /* Choose lower voice index for base voice (v1) */
    if(v1 > v2)
    {
    	v1 ^= v2;
    	v2 ^= v1;
    	v1 ^= v2;
    }

    /* lower the volumes so that the two notes add to roughly the orig. vol */
    vol = voice[v1].velocity;
    voice[v1].velocity  = (uint8)(vol * CHORUS_VELOCITY_TUNING1);
    voice[v2].velocity  = (uint8)(vol * CHORUS_VELOCITY_TUNING1);

    /* Make doubled link v1 and v2 */
    voice[v1].chorus_link = v2;
    voice[v2].chorus_link = v1;

    /* detune notes for chorus effect */
    level >>= 2;		/* scale to a "better" value */
    if (level)
    {
        if(channel[ch].pitchbend + level < 0x2000)
            voice[v2].orig_frequency *= bend_fine[level];
        else
	    voice[v2].orig_frequency /= bend_fine[level];
        voice[v2].cache = NULL;
    }

    MYCHECK(voice[v2].orig_frequency);

    delay = 0.003;

    /* Try to keep the delayed voice from cancelling out the other voice */
    /* Pitch detection is used to find the real pitches for drums and MODs */
    note_adjusted = voice[v1].note + voice[v1].sample->transpose_detected;
    if (note_adjusted > 127) note_adjusted = 127;
    else if (note_adjusted < 0) note_adjusted = 0;
    freq = pitch_freq_table[note_adjusted];
    delay *= freq;
    frac = delay - floor(delay);

    /* force the delay away from 0.5 period */
    if (frac < 0.5 && frac > 0.40)
    {
    	delay = (floor(delay) + 0.40) / freq;
    	if (!(play_mode->encoding & PE_MONO))
    	    delay += (0.5 - frac) * (1.0 - labs(64 - pan) / 63.0) / freq;
    }
    else if (frac >= 0.5 && frac < 0.60)
    {
    	delay = (floor(delay) + 0.60) / freq;
    	if (!(play_mode->encoding & PE_MONO))
    	    delay += (0.5 - frac) * (1.0 - labs(64 - pan) / 63.0) / freq;
    }
    else
	delay = 0.003;

    /* set panning & delay for pseudo-surround effect */
    if(play_mode->encoding & PE_MONO)    /* delay sounds good */
        voice[v2].delay += (int)(play_mode->rate * delay);
    else
    {
        panlevel = 63;
        if (pan - panlevel < 1) panlevel = pan - 1;
        if (pan + panlevel > 127) panlevel = 127 - pan;
        voice[v1].panning -= panlevel;
        voice[v2].panning += panlevel;

        /* choose which voice is delayed based on panning */
        if (voice[v1].panned == PANNED_CENTER) {
            /* randomly choose which voice is delayed */
            if (int_rand(2))
                voice[v1].delay += (int)(play_mode->rate * delay);
            else
                voice[v2].delay += (int)(play_mode->rate * delay);
        }
        else if (pan - 64 < 0) {
            voice[v2].delay += (int)(play_mode->rate * delay);
        }
        else {
            voice[v1].delay += (int)(play_mode->rate * delay);
        }
    }

    /* check for similar drums playing simultaneously with center pans */
    if (!(play_mode->encoding & PE_MONO) &&
    	ISDRUMCHANNEL(ch) && voice[v1].panned == PANNED_CENTER)
    {
    	int i, j;
    
    	/* force Rimshot (37), Snare1 (38), Snare2 (40), and XG #34 to have
    	 * the same delay, otherwise there will be bad voice cancellation.
    	 */
    	if (voice[v1].note == 37 ||
    	    voice[v1].note == 38 ||
    	    voice[v1].note == 40 ||
    	    (voice[v1].note == 34 && play_system_mode == XG_SYSTEM_MODE))
    	{
    	    for (i = 0; i < upper_voices; i++)
    	    {
    	    	if (voice[i].status & (VOICE_DIE | VOICE_FREE))
    	    	    continue;

    	    	if (!ISDRUMCHANNEL(voice[i].channel))
    	    	    continue;

	    	if (i == v1 || i == v2)
	    	    continue;

	    	if (voice[i].note == 37 ||
	    	    voice[i].note == 38 ||
	    	    voice[i].note == 40 ||
	    	    (voice[i].note == 34 &&
	    	     play_system_mode == XG_SYSTEM_MODE))
	    	{
	    	    j = voice[i].chorus_link;

	    	    if (voice[i].panned == PANNED_LEFT &&
	    	        voice[j].panned == PANNED_RIGHT)
	    	    {
	    	    	voice[v1].delay = voice[i].delay;
	    	    	voice[v2].delay = voice[j].delay;

	    	    	break;
	    	    }
	    	}
    	    }
    	}

    	/* force Kick1 (35), Kick2 (36), and XG Kick #33 to have the same
    	 * delay, otherwise there will be bad voice cancellation.
    	 */
    	if (voice[v1].note == 35 ||
    	    voice[v1].note == 36 ||
    	    (voice[v1].note == 33 && play_system_mode == XG_SYSTEM_MODE))
    	{
    	    for (i = 0; i < upper_voices; i++)
    	    {
    	    	if (voice[i].status & (VOICE_DIE | VOICE_FREE))
    	    	    continue;

    	    	if (!ISDRUMCHANNEL(voice[i].channel))
    	    	    continue;

	    	if (i == v1 || i == v2)
	    	    continue;

	    	if (voice[i].note == 35 ||
	    	    voice[i].note == 36 ||
	    	    (voice[i].note == 33 &&
	    	     play_system_mode == XG_SYSTEM_MODE))
	    	{
	    	    j = voice[i].chorus_link;

	    	    if (voice[i].panned == PANNED_LEFT &&
	    	        voice[j].panned == PANNED_RIGHT)
	    	    {
	    	    	voice[v1].delay = voice[i].delay;
	    	    	voice[v2].delay = voice[j].delay;

	    	    	break;
	    	    }
	    	}
    	    }
    	}
    }

    init_voice_pan_delay(v1);
    init_voice_pan_delay(v2);

    recompute_amp(v1);
    apply_envelope_to_amp(v1);
    recompute_amp(v2);
    apply_envelope_to_amp(v2);
    if (level) recompute_freq(v2);
}

/*! note_on() (prescanning) */
static void note_on_prescan(MidiEvent *ev)
{
	int i, ch = ev->channel, note = MIDI_EVENT_NOTE(ev);
	int32 random_delay = 0;

	if(ISDRUMCHANNEL(ch) &&
	   channel[ch].drums[note] != NULL &&
	   !get_rx_drum(channel[ch].drums[note], RX_NOTE_ON)) {	/* Rx. Note On */
		return;
	}
	if(channel[ch].note_limit_low > note ||
		channel[ch].note_limit_high < note ||
		channel[ch].vel_limit_low > ev->b ||
		channel[ch].vel_limit_high < ev->b) {
		return;
	}

    if((channel[ch].portamento_time_msb |
		channel[ch].portamento_time_lsb) == 0 ||
	    channel[ch].portamento == 0)
	{
		int nv;
		int vlist[32];
		Voice *vp;

		nv = find_samples(ev, vlist);

		for(i = 0; i < nv; i++)
		{
		    vp = voice + vlist[i];
		    start_note(ev, vlist[i], 0, nv - i - 1);
			vp->delay += random_delay;
			vp->modenv_delay += random_delay;
		    resamp_cache_refer_on(vp, ev->time);
		    vp->status = VOICE_FREE;
		    vp->temper_instant = 0;
		}
	}
}

static void note_on(MidiEvent *e)
{
    int i, nv, v, ch, note;
    int vlist[32];
    int vid;
	int32 random_delay = 0;

	ch = e->channel;
	note = MIDI_EVENT_NOTE(e);
	
	if(ISDRUMCHANNEL(ch) &&
	   channel[ch].drums[note] != NULL &&
	   !get_rx_drum(channel[ch].drums[note], RX_NOTE_ON)) {	/* Rx. Note On */
		return;
	}
	if(channel[ch].note_limit_low > note ||
		channel[ch].note_limit_high < note ||
		channel[ch].vel_limit_low > e->b ||
		channel[ch].vel_limit_high < e->b) {
		return;
	}
    if((nv = find_samples(e, vlist)) == 0)
	return;

    vid = new_vidq(e->channel, note);

	recompute_bank_parameter(ch, note);
	recompute_channel_filter(ch, note);
	random_delay = calc_random_delay(ch, note);

    for(i = 0; i < nv; i++)
    {
	v = vlist[i];
	if(ISDRUMCHANNEL(ch) &&
	   channel[ch].drums[note] != NULL &&
	   channel[ch].drums[note]->pan_random)
	    channel[ch].drums[note]->drum_panning = int_rand(128);
	else if(channel[ch].pan_random)
	{
	    channel[ch].panning = int_rand(128);
	    ctl_mode_event(CTLE_PANNING, 1, ch, channel[ch].panning);
	}
	start_note(e, v, vid, nv - i - 1);
	voice[v].delay += random_delay;
	voice[v].modenv_delay += random_delay;
#ifdef SMOOTH_MIXING
	voice[v].old_left_mix = voice[v].old_right_mix =
	voice[v].left_mix_inc = voice[v].left_mix_offset =
	voice[v].right_mix_inc = voice[v].right_mix_offset = 0;
#endif
#ifdef USE_DSP_EFFECT
	if(opt_surround_chorus)
	    new_chorus_voice_alternate(v, 0);
#else
	if((channel[ch].chorus_level || opt_surround_chorus))
	{
	    if(opt_surround_chorus)
		new_chorus_voice_alternate(v, channel[ch].chorus_level);
	    else
		new_chorus_voice(v, channel[ch].chorus_level);
	}
	if(channel[ch].delay_level)
	{
	    new_delay_voice(v, channel[ch].delay_level);
	}
#endif
    }

    channel[ch].legato_flag = 1;
}

/*! sostenuto is now implemented as an instant sustain */
static void update_sostenuto_controls(int ch)
{
  int uv = upper_voices, i;

  if(ISDRUMCHANNEL(ch) || channel[ch].sostenuto == 0) {return;}

  for(i = 0; i < uv; i++)
  {
	if ((voice[i].status & (VOICE_ON | VOICE_OFF))
			&& voice[i].channel == ch)
	 {
		  voice[i].status = VOICE_SUSTAINED;
		  ctl_note_event(i);
		  voice[i].envelope_stage = EG_GUS_RELEASE1;
		  recompute_envelope(i);
	 }
  }
}

/*! redamper effect for piano instruments */
static void update_redamper_controls(int ch)
{
  int uv = upper_voices, i;

  if(ISDRUMCHANNEL(ch) || channel[ch].damper_mode == 0) {return;}

  for(i = 0; i < uv; i++)
  {
	if ((voice[i].status & (VOICE_ON | VOICE_OFF))
			&& voice[i].channel == ch)
	  {
		  voice[i].status = VOICE_SUSTAINED;
		  ctl_note_event(i);
		  voice[i].envelope_stage = EG_GUS_RELEASE1;
		  recompute_envelope(i);
	  }
  }
}

static void note_off(MidiEvent *e)
{
  int uv = upper_voices, i;
  int ch, note, vid, sustain;

  ch = e->channel;
  note = MIDI_EVENT_NOTE(e);

  if(ISDRUMCHANNEL(ch))
  {
      int nbank, nprog;

      nbank = channel[ch].bank;
      nprog = note;
      instrument_map(channel[ch].mapID, &nbank, &nprog);
      
      if (channel[ch].drums[nprog] != NULL &&
          get_rx_drum(channel[ch].drums[nprog], RX_NOTE_OFF))
      {
          ToneBank *bank;
          bank = drumset[nbank];
          if(bank == NULL) bank = drumset[0];
          
          /* uh oh, this drum doesn't have an instrument loaded yet */
          if (bank->tone[nprog].instrument == NULL)
              return;

          /* this drum is not loaded for some reason (error occured?) */
          if (IS_MAGIC_INSTRUMENT(bank->tone[nprog].instrument))
              return;

          /* only disallow Note Off if the drum sample is not looped */
          if (!(bank->tone[nprog].instrument->sample->modes & MODES_LOOPING))
              return;	/* Note Off is not allowed. */
      }
  }

  if ((vid = last_vidq(ch, note)) == -1)
      return;
  sustain = channel[ch].sustain;
  for (i = 0; i < uv; i++)
  {
      if(voice[i].status == VOICE_ON &&
	 voice[i].channel == ch &&
	 voice[i].note == note &&
	 voice[i].vid == vid)
      {
	  if(sustain)
	  {
	      voice[i].status = VOICE_SUSTAINED;
	      ctl_note_event(i);
	  }
	  else
	      finish_note(i);
      }
  }

  channel[ch].legato_flag = 0;
}

/* Process the All Notes Off event */
static void all_notes_off(int c)
{
  int i, uv = upper_voices;
  ctl->cmsg(CMSG_INFO, VERB_DEBUG, "All notes off on channel %d", c);
  for(i = 0; i < uv; i++)
    if (voice[i].status==VOICE_ON &&
	voice[i].channel==c)
      {
	if (channel[c].sustain)
	  {
	    voice[i].status=VOICE_SUSTAINED;
	    ctl_note_event(i);
	  }
	else
	  finish_note(i);
      }
  for(i = 0; i < 128; i++)
      vidq_head[c * 128 + i] = vidq_tail[c * 128 + i] = 0;
}

/* Process the All Sounds Off event */
static void all_sounds_off(int c)
{
  int i, uv = upper_voices;
  for(i = 0; i < uv; i++)
    if (voice[i].channel==c &&
	(voice[i].status & ~(VOICE_FREE | VOICE_DIE)))
      {
	kill_note(i);
      }
  for(i = 0; i < 128; i++)
      vidq_head[c * 128 + i] = vidq_tail[c * 128 + i] = 0;
}

/*! adjust polyphonic key pressure (PAf, PAT) */
static void adjust_pressure(MidiEvent *e)
{
    int i, uv = upper_voices;
    int note, ch;

    if(opt_channel_pressure)
    {
	ch = e->channel;
    note = MIDI_EVENT_NOTE(e);
	channel[ch].paf.val = e->b;
	if(channel[ch].paf.pitch != 0) {channel[ch].pitchfactor = 0;}

    for(i = 0; i < uv; i++)
    if(voice[i].status == VOICE_ON &&
       voice[i].channel == ch &&
       voice[i].note == note)
    {
		recompute_amp(i);
		apply_envelope_to_amp(i);
		recompute_freq(i);
		recompute_voice_filter(i);
    }
	}
}

/*! adjust channel pressure (channel aftertouch, CAf, CAT) */
static void adjust_channel_pressure(MidiEvent *e)
{
    if(opt_channel_pressure)
    {
	int i, uv = upper_voices;
	int ch;

	ch = e->channel;
	channel[ch].caf.val = e->a;
	if(channel[ch].caf.pitch != 0) {channel[ch].pitchfactor = 0;}
	  
	for(i = 0; i < uv; i++)
	{
	    if(voice[i].status == VOICE_ON && voice[i].channel == ch)
	    {
		recompute_amp(i);
		apply_envelope_to_amp(i);
		recompute_freq(i);
		recompute_voice_filter(i);
		}
	}
    }
}

static void adjust_panning(int c)
{
    int i, uv = upper_voices, pan = channel[c].panning;
    for(i = 0; i < uv; i++)
    {
	if ((voice[i].channel==c) &&
	    (voice[i].status & (VOICE_ON | VOICE_SUSTAINED)))
	{
            /* adjust pan to include drum/sample pan offsets */
            pan = get_panning(c, voice[i].note, i);

	    /* Hack to handle -EFchorus=2 in a "reasonable" way */
#ifdef USE_DSP_EFFECT
	    if(opt_surround_chorus && voice[i].chorus_link != i)
#else
	    if((channel[c].chorus_level || opt_surround_chorus) &&
	       voice[i].chorus_link != i)
#endif
	    {
		int v1, v2;

		if(i >= voice[i].chorus_link)
		    /* `i' is not base chorus voice.
		     *  This sub voice is already updated.
		     */
		    continue;

		v1 = i;				/* base voice */
		v2 = voice[i].chorus_link;	/* sub voice (detuned) */

		if(opt_surround_chorus) /* Surround chorus mode by Eric. */
		{
		    int panlevel;

		    if (!pan) pan = 1;	/* make hard left be 1 instead of 0 */
		    panlevel = 63;
		    if (pan - panlevel < 1) panlevel = pan - 1;
		    if (pan + panlevel > 127) panlevel = 127 - pan;
		    voice[v1].panning = pan - panlevel;
		    voice[v2].panning = pan + panlevel;
		}
		else
		{
		    voice[v1].panning = pan;
		    if(pan > 60 && pan < 68) /* PANNED_CENTER */
			voice[v2].panning =
			    64 + int_rand(40) - 20; /* 64 +- rand(20) */
		    else if(pan < CHORUS_OPPOSITE_THRESHOLD)
			voice[v2].panning = 127;
		    else if(pan > 127 - CHORUS_OPPOSITE_THRESHOLD)
			voice[v2].panning = 0;
		    else
			voice[v2].panning = (pan < 64 ? 0 : 127);
		}
		recompute_amp(v2);
		apply_envelope_to_amp(v2);
		/* v1 == i, so v1 will be updated next */
	    }
	    else
		voice[i].panning = pan;

		recompute_amp(i);
	    apply_envelope_to_amp(i);
	}
    }
}

void play_midi_setup_drums(int ch, int note)
{
    channel[ch].drums[note] = (struct DrumParts *)
	new_segment(&playmidi_pool, sizeof(struct DrumParts));
    reset_drum_controllers(channel[ch].drums, note);
}

static void adjust_drum_panning(int ch, int note)
{
    int i, uv = upper_voices;

    for(i = 0; i < uv; i++) {
		if(voice[i].channel == ch &&
		   voice[i].note == note &&
		   (voice[i].status & (VOICE_ON | VOICE_SUSTAINED)))
		{
			voice[i].panning = get_panning(ch, note, i);
			recompute_amp(i);
			apply_envelope_to_amp(i);
		}
	}
}

static void drop_sustain(int c)
{
  int i, uv = upper_voices;
  for(i = 0; i < uv; i++)
    if (voice[i].status == VOICE_SUSTAINED && voice[i].channel == c)
      finish_note(i);
}

static void adjust_pitch(int c)
{
  int i, uv = upper_voices;
  for(i = 0; i < uv; i++)
    if (voice[i].status != VOICE_FREE && voice[i].channel == c)
	recompute_freq(i);
}

static void adjust_volume(int c)
{
  int i, uv = upper_voices;
  for(i = 0; i < uv; i++)
    if (voice[i].channel == c &&
	(voice[i].status & (VOICE_ON | VOICE_SUSTAINED)))
      {
	recompute_amp(i);
	apply_envelope_to_amp(i);
      }
}

static void set_reverb_level(int ch, int level)
{
	if (level == -1) {
		channel[ch].reverb_level = channel[ch].reverb_id =
				(opt_reverb_control < 0)
				? -opt_reverb_control & 0x7f : DEFAULT_REVERB_SEND_LEVEL;
		make_rvid_flag = 1;
		return;
	}
	channel[ch].reverb_level = level;
	make_rvid_flag = 0;	/* to update reverb_id */
}

int get_reverb_level(int ch)
{
	if (channel[ch].reverb_level == -1)
		return (opt_reverb_control < 0)
			? -opt_reverb_control & 0x7f : DEFAULT_REVERB_SEND_LEVEL;
	return channel[ch].reverb_level;
}

int get_chorus_level(int ch)
{
#ifdef DISALLOW_DRUM_BENDS
    if(ISDRUMCHANNEL(ch))
	return 0; /* Not supported drum channel chorus */
#endif
    if(opt_chorus_control == 1)
	return channel[ch].chorus_level;
    return -opt_chorus_control;
}

#ifndef USE_DSP_EFFECT
static void make_rvid(void)
{
    int i, j, lv, maxrv;

    for(maxrv = MAX_CHANNELS - 1; maxrv >= 0; maxrv--)
    {
	if(channel[maxrv].reverb_level == -1)
	    channel[maxrv].reverb_id = -1;
	else if(channel[maxrv].reverb_level >= 0)
	    break;
    }

    /* collect same reverb level. */
    for(i = 0; i <= maxrv; i++)
    {
	if((lv = channel[i].reverb_level) == -1)
	{
	    channel[i].reverb_id = -1;
	    continue;
	}
	channel[i].reverb_id = i;
	for(j = 0; j < i; j++)
	{
	    if(channel[j].reverb_level == lv)
	    {
		channel[i].reverb_id = j;
		break;
	    }
	}
    }
}
#endif /* !USE_DSP_EFFECT */

void free_drum_effect(int ch)
{
	int i;
	if (channel[ch].drum_effect != NULL) {
		for (i = 0; i < channel[ch].drum_effect_num; i++) {
			if (channel[ch].drum_effect[i].buf != NULL) {
				free(channel[ch].drum_effect[i].buf);
				channel[ch].drum_effect[i].buf = NULL;
			}
		}
		free(channel[ch].drum_effect);
		channel[ch].drum_effect = NULL;
	}
	channel[ch].drum_effect_num = 0;
	channel[ch].drum_effect_flag = 0;
}

static void make_drum_effect(int ch)
{
	int i, note, num = 0;
	int8 note_table[128];
	struct DrumParts *drum;
	struct DrumPartEffect *de;

	if (channel[ch].drums == NULL) {return;}

	if (channel[ch].drum_effect_flag == 0) {
		free_drum_effect(ch);
		memset(note_table, 0, sizeof(int8) * 128);

		for(i = 0; i < 128; i++) {
			if ((drum = channel[ch].drums[i]) != NULL)
			{
				if (drum->reverb_level != -1
				|| drum->chorus_level != -1 || drum->delay_level != -1) {
					note_table[num++] = i;
				}
			}
		}

		channel[ch].drum_effect = (struct DrumPartEffect *)safe_malloc(sizeof(struct DrumPartEffect) * num);

		for(i = 0; i < num; i++) {
			de = &(channel[ch].drum_effect[i]);
			de->note = note = note_table[i];
			drum = channel[ch].drums[note];
			de->reverb_send = (int32)drum->reverb_level * (int32)get_reverb_level(ch) / 127;
			de->chorus_send = (int32)drum->chorus_level * (int32)channel[ch].chorus_level / 127;
			de->delay_send = (int32)drum->delay_level * (int32)channel[ch].delay_level / 127;
			de->buf = (int32 *)safe_malloc(AUDIO_BUFFER_SIZE * 8);
			memset(de->buf, 0, AUDIO_BUFFER_SIZE * 8);
		}

		channel[ch].drum_effect_num = num;
		channel[ch].drum_effect_flag = 1;
	}
}

static void adjust_master_volume(void)
{
  int i, uv = upper_voices;
  adjust_amplification();
  for(i = 0; i < uv; i++)
      if(voice[i].status & (VOICE_ON | VOICE_SUSTAINED))
      {
	  recompute_amp(i);
	  apply_envelope_to_amp(i);
      }
}

int midi_drumpart_change(int ch, int isdrum)
{
    if(IS_SET_CHANNELMASK(drumchannel_mask, ch))
	return 0;
    if(isdrum)
    {
	SET_CHANNELMASK(drumchannels, ch);
	SET_CHANNELMASK(current_file_info->drumchannels, ch);
    }
    else
    {
	UNSET_CHANNELMASK(drumchannels, ch);
	UNSET_CHANNELMASK(current_file_info->drumchannels, ch);
    }

    return 1;
}

void midi_program_change(int ch, int prog)
{
	int dr = ISDRUMCHANNEL(ch);
	int newbank, b, p, map;
	
	switch (play_system_mode) {
	case GS_SYSTEM_MODE:	/* GS */
		if ((map = channel[ch].bank_lsb) == 0) {
			map = channel[ch].tone_map0_number;
		}
		switch (map) {
		case 0:		/* No change */
			break;
		case 1:
			channel[ch].mapID = (dr) ? SC_55_DRUM_MAP : SC_55_TONE_MAP;
			break;
		case 2:
			channel[ch].mapID = (dr) ? SC_88_DRUM_MAP : SC_88_TONE_MAP;
			break;
		case 3:
			channel[ch].mapID = (dr) ? SC_88PRO_DRUM_MAP : SC_88PRO_TONE_MAP;
			break;
		case 4:
			channel[ch].mapID = (dr) ? SC_8850_DRUM_MAP : SC_8850_TONE_MAP;
			break;
		default:
			break;
		}
		newbank = channel[ch].bank_msb;
		break;
	case XG_SYSTEM_MODE:	/* XG */
		switch (channel[ch].bank_msb) {
		case 0:		/* Normal */
#if 0
			if (ch == 9 && channel[ch].bank_lsb == 127
					&& channel[ch].mapID == XG_DRUM_MAP)
				/* FIXME: Why this part is drum?  Is this correct? */
				break;
#endif
/* Eric's explanation for the FIXME (March 2004):
 *
 * I don't have the original email from my archived inbox, but I found a
 * reply I made in my archived sent-mail from 1999.  A September 5th message
 * to Masanao Izumo is discussing a problem with a "reapxg.mid", a file which
 * I still have, and how it issues an MSB=0 with a program change on ch 9, 
 * thus turning it into a melodic channel.  The strange thing is, this doesn't
 * happen on XG hardware, nor on the XG softsynth.  It continues to play as a
 * normal drum.  The author of the midi file obviously intended it to be
 * drumset 16 too.  The original fix was to detect LSB == -1, then break so
 * as to not set it to a melodic channel.  I'm guessing that this somehow got
 * mutated into checking for 127 instead, and the current FIXME is related to
 * the original hack from Sept 1999.  The Sept 5th email discusses patches
 * being applied to version 2.5.1 to get XG drums to work properly, and a
 * Sept 7th email to someone else discusses the fixes being part of the
 * latest 2.6.0-beta3.  A September 23rd email to Masanao Izumo specifically
 * mentions the LSB == -1 hack (and reapxg.mid not playing "correctly"
 * anymore), as well as new changes in 2.6.0 that broke a lot of other XG
 * files (XG drum support was extremely buggy in 1999 and we were still trying
 * to figure out how to initialize things to reproduce hardware behavior).  An
 * October 5th email says that 2.5.1 was correct, 2.6.0 had very broken XG
 * drum changes, and 2.6.1 still has problems.  Further discussions ensued
 * over what was "correct": to follow the XG spec, or to reproduce
 * "features" / bugs in the hardware.  I can't find the rest of the
 * discussions, but I think it ended with us agreeing to just follow the spec
 * and not try to reproduce the hardware strangeness.  I don't know how the
 * current FIXME wound up the way it is now.  I'm still going to guess it is
 * related to the old reapxg.mid hack.
 *
 * Now that reset_midi() initializes channel[ch].bank_lsb to 0 instead of -1,
 * checking for LSB == -1 won't do anything anymore, so changing the above
 * FIXME to the original == -1 won't do any good.  It is best to just #if 0
 * it out and leave it here as a reminder that there is at least one XG
 * hardware / softsynth "bug" that is not reproduced by timidity at the
 * moment.
 *
 * If the current FIXME actually reproduces some other XG hadware bug that
 * I don't know about, then it may have a valid purpose.  I just don't know
 * what that purpose is at the moment.  Perhaps someone else does?  I still
 * have src going back to 2.10.4, and the FIXME comment was already there by
 * then.  I don't see any entries in the Changelog that could explain it
 * either.  If someone has src from 2.5.1 through 2.10.3 and wants to
 * investigate this further, go for it :)
 */
			midi_drumpart_change(ch, 0);
			channel[ch].mapID = XG_NORMAL_MAP;
			dr = ISDRUMCHANNEL(ch);
			break;
		case 64:	/* SFX voice */
			midi_drumpart_change(ch, 0);
			channel[ch].mapID = XG_SFX64_MAP;
			dr = ISDRUMCHANNEL(ch);
			break;
		case 126:	/* SFX kit */
			midi_drumpart_change(ch, 1);
			channel[ch].mapID = XG_SFX126_MAP;
			dr = ISDRUMCHANNEL(ch);
			break;
		case 127:	/* Drumset */
			midi_drumpart_change(ch, 1);
			channel[ch].mapID = XG_DRUM_MAP;
			dr = ISDRUMCHANNEL(ch);
			break;
		default:
			break;
		}
		newbank = channel[ch].bank_lsb;
		break;
	case GM2_SYSTEM_MODE:	/* GM2 */
		if ((channel[ch].bank_msb & 0xFE) == 0x78)	/* 0x78/0x79 */
		{
			midi_drumpart_change(ch, channel[ch].bank_msb == 0x78);
			dr = ISDRUMCHANNEL(ch);
		}
		channel[ch].mapID = (dr) ? GM2_DRUM_MAP : GM2_TONE_MAP;
		newbank = channel[ch].bank_lsb;
		break;
	default:
		newbank = channel[ch].bank_msb;
		break;
	}
	if (dr) {
		channel[ch].bank = prog;	/* newbank is ignored */
		channel[ch].program = prog;
		if (drumset[prog] == NULL || drumset[prog]->alt == NULL)
			channel[ch].altassign = drumset[0]->alt;
		else
			channel[ch].altassign = drumset[prog]->alt;
		ctl_mode_event(CTLE_DRUMPART, 1, ch, 1);
	} else {
		channel[ch].bank = (special_tonebank >= 0)
				? special_tonebank : newbank;
		channel[ch].program = (default_program[ch] == SPECIAL_PROGRAM)
				? SPECIAL_PROGRAM : prog;
		channel[ch].altassign = NULL;
		ctl_mode_event(CTLE_DRUMPART, 1, ch, 0);
		if (opt_realtime_playing && (play_mode->flag & PF_PCM_STREAM)) {
			b = channel[ch].bank, p = prog;
			instrument_map(channel[ch].mapID, &b, &p);
			play_midi_load_instrument(0, b, p);
		}
	}
}

static int16 conv_lfo_pitch_depth(float val)
{
	return (int16)(0.0318f * val * val + 0.6858f * val + 0.5f);
}

static int16 conv_lfo_filter_depth(float val)
{
	return (int16)((0.0318f * val * val + 0.6858f * val) * 4.0f + 0.5f);
}

/*! process system exclusive sent from parse_sysex_event_multi(). */
static void process_sysex_event(int ev, int ch, int val, int b)
{
	int temp, msb, note;

	if (ch >= MAX_CHANNELS)
		return;
	if (ev == ME_SYSEX_MSB) {
		channel[ch].sysex_msb_addr = b;
		channel[ch].sysex_msb_val = val;
	} else if(ev == ME_SYSEX_GS_MSB) {
		channel[ch].sysex_gs_msb_addr = b;
		channel[ch].sysex_gs_msb_val = val;
	} else if(ev == ME_SYSEX_XG_MSB) {
		channel[ch].sysex_xg_msb_addr = b;
		channel[ch].sysex_xg_msb_val = val;
	} else if(ev == ME_SYSEX_LSB) {	/* Universal system exclusive message */
		msb = channel[ch].sysex_msb_addr;
		note = channel[ch].sysex_msb_val;
		channel[ch].sysex_msb_addr = channel[ch].sysex_msb_val = 0;
		switch(b)
		{
		case 0x00:	/* CAf Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].caf.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf Pitch Control (CH:%d %d semitones)", ch, channel[ch].caf.pitch);
			break;
		case 0x01:	/* CAf Filter Cutoff Control */
			channel[ch].caf.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].caf.cutoff);
			break;
		case 0x02:	/* CAf Amplitude Control */
			channel[ch].caf.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf Amplitude Control (CH:%d %.2f)", ch, channel[ch].caf.amp);
			break;
		case 0x03:	/* CAf LFO1 Rate Control */
			channel[ch].caf.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].caf.lfo1_rate);
			break;
		case 0x04:	/* CAf LFO1 Pitch Depth */
			channel[ch].caf.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].caf.lfo1_pitch_depth); 
			break;
		case 0x05:	/* CAf LFO1 Filter Depth */
			channel[ch].caf.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].caf.lfo1_tvf_depth); 
			break;
		case 0x06:	/* CAf LFO1 Amplitude Depth */
			channel[ch].caf.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].caf.lfo1_tva_depth); 
			break;
		case 0x07:	/* CAf LFO2 Rate Control */
			channel[ch].caf.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].caf.lfo2_rate);
			break;
		case 0x08:	/* CAf LFO2 Pitch Depth */
			channel[ch].caf.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].caf.lfo2_pitch_depth); 
			break;
		case 0x09:	/* CAf LFO2 Filter Depth */
			channel[ch].caf.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].caf.lfo2_tvf_depth); 
			break;
		case 0x0A:	/* CAf LFO2 Amplitude Depth */
			channel[ch].caf.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAf LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].caf.lfo2_tva_depth); 
			break;
		case 0x0B:	/* PAf Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].paf.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf Pitch Control (CH:%d %d semitones)", ch, channel[ch].paf.pitch);
			break;
		case 0x0C:	/* PAf Filter Cutoff Control */
			channel[ch].paf.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].paf.cutoff);
			break;
		case 0x0D:	/* PAf Amplitude Control */
			channel[ch].paf.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf Amplitude Control (CH:%d %.2f)", ch, channel[ch].paf.amp);
			break;
		case 0x0E:	/* PAf LFO1 Rate Control */
			channel[ch].paf.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].paf.lfo1_rate);
			break;
		case 0x0F:	/* PAf LFO1 Pitch Depth */
			channel[ch].paf.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].paf.lfo1_pitch_depth); 
			break;
		case 0x10:	/* PAf LFO1 Filter Depth */
			channel[ch].paf.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].paf.lfo1_tvf_depth); 
			break;
		case 0x11:	/* PAf LFO1 Amplitude Depth */
			channel[ch].paf.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].paf.lfo1_tva_depth); 
			break;
		case 0x12:	/* PAf LFO2 Rate Control */
			channel[ch].paf.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].paf.lfo2_rate);
			break;
		case 0x13:	/* PAf LFO2 Pitch Depth */
			channel[ch].paf.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].paf.lfo2_pitch_depth); 
			break;
		case 0x14:	/* PAf LFO2 Filter Depth */
			channel[ch].paf.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].paf.lfo2_tvf_depth); 
			break;
		case 0x15:	/* PAf LFO2 Amplitude Depth */
			channel[ch].paf.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "PAf LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].paf.lfo2_tva_depth); 
			break;
		case 0x16:	/* MOD Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].mod.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD Pitch Control (CH:%d %d semitones)", ch, channel[ch].mod.pitch);
			break;
		case 0x17:	/* MOD Filter Cutoff Control */
			channel[ch].mod.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].mod.cutoff);
			break;
		case 0x18:	/* MOD Amplitude Control */
			channel[ch].mod.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD Amplitude Control (CH:%d %.2f)", ch, channel[ch].mod.amp);
			break;
		case 0x19:	/* MOD LFO1 Rate Control */
			channel[ch].mod.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].mod.lfo1_rate);
			break;
		case 0x1A:	/* MOD LFO1 Pitch Depth */
			channel[ch].mod.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].mod.lfo1_pitch_depth); 
			break;
		case 0x1B:	/* MOD LFO1 Filter Depth */
			channel[ch].mod.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].mod.lfo1_tvf_depth); 
			break;
		case 0x1C:	/* MOD LFO1 Amplitude Depth */
			channel[ch].mod.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].mod.lfo1_tva_depth); 
			break;
		case 0x1D:	/* MOD LFO2 Rate Control */
			channel[ch].mod.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].mod.lfo2_rate);
			break;
		case 0x1E:	/* MOD LFO2 Pitch Depth */
			channel[ch].mod.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].mod.lfo2_pitch_depth); 
			break;
		case 0x1F:	/* MOD LFO2 Filter Depth */
			channel[ch].mod.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].mod.lfo2_tvf_depth); 
			break;
		case 0x20:	/* MOD LFO2 Amplitude Depth */
			channel[ch].mod.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MOD LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].mod.lfo2_tva_depth); 
			break;
		case 0x21:	/* BEND Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].bend.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND Pitch Control (CH:%d %d semitones)", ch, channel[ch].bend.pitch);
			break;
		case 0x22:	/* BEND Filter Cutoff Control */
			channel[ch].bend.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].bend.cutoff);
			break;
		case 0x23:	/* BEND Amplitude Control */
			channel[ch].bend.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND Amplitude Control (CH:%d %.2f)", ch, channel[ch].bend.amp);
			break;
		case 0x24:	/* BEND LFO1 Rate Control */
			channel[ch].bend.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].bend.lfo1_rate);
			break;
		case 0x25:	/* BEND LFO1 Pitch Depth */
			channel[ch].bend.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].bend.lfo1_pitch_depth); 
			break;
		case 0x26:	/* BEND LFO1 Filter Depth */
			channel[ch].bend.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].bend.lfo1_tvf_depth); 
			break;
		case 0x27:	/* BEND LFO1 Amplitude Depth */
			channel[ch].bend.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].bend.lfo1_tva_depth); 
			break;
		case 0x28:	/* BEND LFO2 Rate Control */
			channel[ch].bend.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].bend.lfo2_rate);
			break;
		case 0x29:	/* BEND LFO2 Pitch Depth */
			channel[ch].bend.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].bend.lfo2_pitch_depth); 
			break;
		case 0x2A:	/* BEND LFO2 Filter Depth */
			channel[ch].bend.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].bend.lfo2_tvf_depth); 
			break;
		case 0x2B:	/* BEND LFO2 Amplitude Depth */
			channel[ch].bend.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].bend.lfo2_tva_depth); 
			break;
		case 0x2C:	/* CC1 Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].cc1.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 Pitch Control (CH:%d %d semitones)", ch, channel[ch].cc1.pitch);
			break;
		case 0x2D:	/* CC1 Filter Cutoff Control */
			channel[ch].cc1.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].cc1.cutoff);
			break;
		case 0x2E:	/* CC1 Amplitude Control */
			channel[ch].cc1.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 Amplitude Control (CH:%d %.2f)", ch, channel[ch].cc1.amp);
			break;
		case 0x2F:	/* CC1 LFO1 Rate Control */
			channel[ch].cc1.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].cc1.lfo1_rate);
			break;
		case 0x30:	/* CC1 LFO1 Pitch Depth */
			channel[ch].cc1.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].cc1.lfo1_pitch_depth); 
			break;
		case 0x31:	/* CC1 LFO1 Filter Depth */
			channel[ch].cc1.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].cc1.lfo1_tvf_depth); 
			break;
		case 0x32:	/* CC1 LFO1 Amplitude Depth */
			channel[ch].cc1.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].cc1.lfo1_tva_depth); 
			break;
		case 0x33:	/* CC1 LFO2 Rate Control */
			channel[ch].cc1.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].cc1.lfo2_rate);
			break;
		case 0x34:	/* CC1 LFO2 Pitch Depth */
			channel[ch].cc1.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].cc1.lfo2_pitch_depth); 
			break;
		case 0x35:	/* CC1 LFO2 Filter Depth */
			channel[ch].cc1.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].cc1.lfo2_tvf_depth); 
			break;
		case 0x36:	/* CC1 LFO2 Amplitude Depth */
			channel[ch].cc1.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC1 LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].cc1.lfo2_tva_depth); 
			break;
		case 0x37:	/* CC2 Pitch Control */
			if(val > 0x58) {val = 0x58;}
			else if(val < 0x28) {val = 0x28;}
			channel[ch].cc2.pitch = val - 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 Pitch Control (CH:%d %d semitones)", ch, channel[ch].cc2.pitch);
			break;
		case 0x38:	/* CC2 Filter Cutoff Control */
			channel[ch].cc2.cutoff = (val - 64) * 150;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 Filter Cutoff Control (CH:%d %d cents)", ch, channel[ch].cc2.cutoff);
			break;
		case 0x39:	/* CC2 Amplitude Control */
			channel[ch].cc2.amp = (float)val / 64.0f - 1.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 Amplitude Control (CH:%d %.2f)", ch, channel[ch].cc2.amp);
			break;
		case 0x3A:	/* CC2 LFO1 Rate Control */
			channel[ch].cc2.lfo1_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO1 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].cc2.lfo1_rate);
			break;
		case 0x3B:	/* CC2 LFO1 Pitch Depth */
			channel[ch].cc2.lfo1_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO1 Pitch Depth (CH:%d %d cents)", ch, channel[ch].cc2.lfo1_pitch_depth); 
			break;
		case 0x3C:	/* CC2 LFO1 Filter Depth */
			channel[ch].cc2.lfo1_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO1 Filter Depth (CH:%d %d cents)", ch, channel[ch].cc2.lfo1_tvf_depth); 
			break;
		case 0x3D:	/* CC2 LFO1 Amplitude Depth */
			channel[ch].cc2.lfo1_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO1 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].cc2.lfo1_tva_depth); 
			break;
		case 0x3E:	/* CC2 LFO2 Rate Control */
			channel[ch].cc2.lfo2_rate = (float)(val - 64) / 6.4f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO2 Rate Control (CH:%d %.1f Hz)", ch, channel[ch].cc2.lfo2_rate);
			break;
		case 0x3F:	/* CC2 LFO2 Pitch Depth */
			channel[ch].cc2.lfo2_pitch_depth = conv_lfo_pitch_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO2 Pitch Depth (CH:%d %d cents)", ch, channel[ch].cc2.lfo2_pitch_depth); 
			break;
		case 0x40:	/* CC2 LFO2 Filter Depth */
			channel[ch].cc2.lfo2_tvf_depth = conv_lfo_filter_depth(val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO2 Filter Depth (CH:%d %d cents)", ch, channel[ch].cc2.lfo2_tvf_depth); 
			break;
		case 0x41:	/* CC2 LFO2 Amplitude Depth */
			channel[ch].cc2.lfo2_tva_depth = (float)val / 127.0f;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CC2 LFO2 Amplitude Depth (CH:%d %.2f)", ch, channel[ch].cc2.lfo2_tva_depth); 
			break;
		case 0x42:	/* Note Limit Low */
			channel[ch].note_limit_low = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Note Limit Low (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x43:	/* Note Limit High */
			channel[ch].note_limit_high = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Note Limit High (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x44:	/* Velocity Limit Low */
			channel[ch].vel_limit_low = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Velocity Limit Low (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x45:	/* Velocity Limit High */
			channel[ch].vel_limit_high = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Velocity Limit High (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x46:	/* Rx. Note Off */
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			set_rx_drum(channel[ch].drums[note], RX_NOTE_OFF, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Rx. Note Off (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
			break;
		case 0x47:	/* Rx. Note On */
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			set_rx_drum(channel[ch].drums[note], RX_NOTE_ON, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Rx. Note On (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
			break;
		case 0x48:	/* Rx. Pitch Bend */
			set_rx(ch, RX_PITCH_BEND, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Pitch Bend (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x49:	/* Rx. Channel Pressure */
			set_rx(ch, RX_CH_PRESSURE, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Channel Pressure (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4A:	/* Rx. Program Change */
			set_rx(ch, RX_PROGRAM_CHANGE, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Program Change (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4B:	/* Rx. Control Change */
			set_rx(ch, RX_CONTROL_CHANGE, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Control Change (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4C:	/* Rx. Poly Pressure */
			set_rx(ch, RX_POLY_PRESSURE, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Poly Pressure (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4D:	/* Rx. Note Message */
			set_rx(ch, RX_NOTE_MESSAGE, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Note Message (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4E:	/* Rx. RPN */
			set_rx(ch, RX_RPN, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. RPN (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x4F:	/* Rx. NRPN */
			set_rx(ch, RX_NRPN, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. NRPN (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x50:	/* Rx. Modulation */
			set_rx(ch, RX_MODULATION, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Modulation (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x51:	/* Rx. Volume */
			set_rx(ch, RX_VOLUME, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Volume (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x52:	/* Rx. Panpot */
			set_rx(ch, RX_PANPOT, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Panpot (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x53:	/* Rx. Expression */
			set_rx(ch, RX_EXPRESSION, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Expression (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x54:	/* Rx. Hold1 */
			set_rx(ch, RX_HOLD1, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Hold1 (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x55:	/* Rx. Portamento */
			set_rx(ch, RX_PORTAMENTO, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Portamento (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x56:	/* Rx. Sostenuto */
			set_rx(ch, RX_SOSTENUTO, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Sostenuto (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x57:	/* Rx. Soft */
			set_rx(ch, RX_SOFT, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Soft (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x58:	/* Rx. Bank Select */
			set_rx(ch, RX_BANK_SELECT, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Bank Select (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x59:	/* Rx. Bank Select LSB */
			set_rx(ch, RX_BANK_SELECT_LSB, val);
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Rx. Bank Select LSB (CH:%d VAL:%d)", ch, val); 
			break;
		case 0x60:	/* Reverb Type (GM2) */
			if (val > 8) {val = 8;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Type (%d)", val);
			set_reverb_macro_gm2(val);
			recompute_reverb_status_gs();
			init_reverb();
			break;
		case 0x61:	/* Chorus Type (GM2) */
			if (val > 5) {val = 5;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Type (%d)", val);
			set_chorus_macro_gs(val);
			recompute_chorus_status_gs();
			init_ch_chorus();
			break;
		default:
			break;
		}
		return;
	} else if(ev == ME_SYSEX_GS_LSB) {	/* GS system exclusive message */
		msb = channel[ch].sysex_gs_msb_addr;
		note = channel[ch].sysex_gs_msb_val;
		channel[ch].sysex_gs_msb_addr = channel[ch].sysex_gs_msb_val = 0;
		switch(b)
		{
		case 0x00:	/* EQ ON/OFF */
			if(!opt_eq_control) {break;}
			if(channel[ch].eq_gs != val) {
				if(val) {
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ ON (CH:%d)",ch);
				} else {
					ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ OFF (CH:%d)",ch);
				}
			}
			channel[ch].eq_gs = val;
			break;
		case 0x01:	/* EQ LOW FREQ */
			if(!opt_eq_control) {break;}
			eq_status_gs.low_freq = val;
			recompute_eq_status_gs();
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ LOW FREQ (%d)",val);
			break;
		case 0x02:	/* EQ LOW GAIN */
			if(!opt_eq_control) {break;}
			eq_status_gs.low_gain = val;
			recompute_eq_status_gs();
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ LOW GAIN (%d dB)",val - 0x40);
			break;
		case 0x03:	/* EQ HIGH FREQ */
			if(!opt_eq_control) {break;}
			eq_status_gs.high_freq = val;
			recompute_eq_status_gs();
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ HIGH FREQ (%d)",val);
			break;
		case 0x04:	/* EQ HIGH GAIN */
			if(!opt_eq_control) {break;}
			eq_status_gs.high_gain = val;
			recompute_eq_status_gs();
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ HIGH GAIN (%d dB)",val - 0x40);
			break;
		case 0x05:	/* Reverb Macro */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Macro (%d)",val);
			set_reverb_macro_gs(val);
			recompute_reverb_status_gs();
			init_reverb();
			break;
		case 0x06:	/* Reverb Character */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Character (%d)",val);
			if (reverb_status_gs.character != val) {
				reverb_status_gs.character = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
			break;
		case 0x07:	/* Reverb Pre-LPF */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Pre-LPF (%d)",val);
			if(reverb_status_gs.pre_lpf != val) {
				reverb_status_gs.pre_lpf = val;
				recompute_reverb_status_gs();
			}
			break;
		case 0x08:	/* Reverb Level */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Level (%d)",val);
			if(reverb_status_gs.level != val) {
				reverb_status_gs.level = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
			break;
		case 0x09:	/* Reverb Time */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Time (%d)",val);
			if(reverb_status_gs.time != val) {
				reverb_status_gs.time = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
			break;
		case 0x0A:	/* Reverb Delay Feedback */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Delay Feedback (%d)",val);
			if(reverb_status_gs.delay_feedback != val) {
				reverb_status_gs.delay_feedback = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
			break;
		case 0x0C:	/* Reverb Predelay Time */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Predelay Time (%d)",val);
			if(reverb_status_gs.pre_delay_time != val) {
				reverb_status_gs.pre_delay_time = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
			break;
		case 0x0D:	/* Chorus Macro */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Macro (%d)",val);
			set_chorus_macro_gs(val);
			recompute_chorus_status_gs();
			init_ch_chorus();
			break;
		case 0x0E:	/* Chorus Pre-LPF */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Pre-LPF (%d)",val);
			if (chorus_status_gs.pre_lpf != val) {
				chorus_status_gs.pre_lpf = val;
				recompute_chorus_status_gs();
			}
			break;
		case 0x0F:	/* Chorus Level */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Level (%d)",val);
			if (chorus_status_gs.level != val) {
				chorus_status_gs.level = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x10:	/* Chorus Feedback */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Feedback (%d)",val);
			if (chorus_status_gs.feedback != val) {
				chorus_status_gs.feedback = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x11:	/* Chorus Delay */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Delay (%d)",val);
			if (chorus_status_gs.delay != val) {
				chorus_status_gs.delay = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x12:	/* Chorus Rate */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Rate (%d)",val);
			if (chorus_status_gs.rate != val) {
				chorus_status_gs.rate = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x13:	/* Chorus Depth */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Depth (%d)",val);
			if (chorus_status_gs.depth != val) {
				chorus_status_gs.depth = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x14:	/* Chorus Send Level to Reverb */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Send Level to Reverb (%d)",val);
			if (chorus_status_gs.send_reverb != val) {
				chorus_status_gs.send_reverb = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x15:	/* Chorus Send Level to Delay */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Send Level to Delay (%d)",val);
			if (chorus_status_gs.send_delay != val) {
				chorus_status_gs.send_delay = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
			break;
		case 0x16:	/* Delay Macro */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Macro (%d)",val);
			set_delay_macro_gs(val);
			recompute_delay_status_gs();
			init_ch_delay();
			break;
		case 0x17:	/* Delay Pre-LPF */
			if (val > 7) {val = 7;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Pre-LPF (%d)",val);
			val &= 0x7;
			if (delay_status_gs.pre_lpf != val) {
				delay_status_gs.pre_lpf = val;
				recompute_delay_status_gs();
			}
			break;
		case 0x18:	/* Delay Time Center */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Time Center (%d)",val);
			if (delay_status_gs.time_c != val) {
				delay_status_gs.time_c = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x19:	/* Delay Time Ratio Left */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Time Ratio Left (%d)",val);
			if (val == 0) {val = 1;}
			if (delay_status_gs.time_l != val) {
				delay_status_gs.time_l = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1A:	/* Delay Time Ratio Right */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Time Ratio Right (%d)",val);
			if (val == 0) {val = 1;}
			if (delay_status_gs.time_r != val) {
				delay_status_gs.time_r = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1B:	/* Delay Level Center */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Level Center (%d)",val);
			if (delay_status_gs.level_center != val) {
				delay_status_gs.level_center = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1C:	/* Delay Level Left */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Level Left (%d)",val);
			if (delay_status_gs.level_left != val) {
				delay_status_gs.level_left = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1D:	/* Delay Level Right */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Level Right (%d)",val);
			if (delay_status_gs.level_right != val) {
				delay_status_gs.level_right = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1E:	/* Delay Level */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Level (%d)",val);
			if (delay_status_gs.level != val) {
				delay_status_gs.level = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x1F:	/* Delay Feedback */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Feedback (%d)",val);
			if (delay_status_gs.feedback != val) {
				delay_status_gs.feedback = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x20:	/* Delay Send Level to Reverb */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Send Level to Reverb (%d)",val);
			if (delay_status_gs.send_reverb = val) {
				delay_status_gs.send_reverb = val;
				recompute_delay_status_gs();
				init_ch_delay();
			}
			break;
		case 0x21:	/* Velocity Sense Depth */
			channel[ch].velocity_sense_depth = val;
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Velocity Sense Depth (CH:%d VAL:%d)",ch,val);
			break;
		case 0x22:	/* Velocity Sense Offset */
			channel[ch].velocity_sense_offset = val;
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Velocity Sense Offset (CH:%d VAL:%d)",ch,val);
			break;
		case 0x23:	/* Insertion Effect ON/OFF */
			if(!opt_insertion_effect) {break;}
			if(channel[ch].insertion_effect != val) {
				if(val) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EFX ON (CH:%d)",ch);}
				else {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EFX OFF (CH:%d)",ch);}
			}
			channel[ch].insertion_effect = val;
			break;
		case 0x24:	/* Assign Mode */
			channel[ch].assign_mode = val;
			if(val == 0) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Assign Mode: Single (CH:%d)",ch);
			} else if(val == 1) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Assign Mode: Limited-Multi (CH:%d)",ch);
			} else if(val == 2) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Assign Mode: Full-Multi (CH:%d)",ch);
			}
			break;
		case 0x25:	/* TONE MAP-0 NUMBER */
			channel[ch].tone_map0_number = val;
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Tone Map-0 Number (CH:%d VAL:%d)",ch,val);
			break;
		case 0x26:	/* Pitch Offset Fine */
			channel[ch].pitch_offset_fine = (FLOAT_T)((((int32)val << 4) | (int32)val) - 0x80) / 10.0;
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Pitch Offset Fine (CH:%d %3fHz)",ch,channel[ch].pitch_offset_fine);
			break;
		case 0x27:	/* Insertion Effect Parameter */
			if(!opt_insertion_effect) {break;}
			temp = insertion_effect_gs.type;
			insertion_effect_gs.type_msb = val;
			insertion_effect_gs.type = ((int32)insertion_effect_gs.type_msb << 8) | (int32)insertion_effect_gs.type_lsb;
			if(temp == insertion_effect_gs.type) {
				recompute_insertion_effect_gs();
			} else {
				realloc_insertion_effect_gs();
			}
			break;
		case 0x28:	/* Insertion Effect Parameter */
			if(!opt_insertion_effect) {break;}
			temp = insertion_effect_gs.type;
			insertion_effect_gs.type_lsb = val;
			insertion_effect_gs.type = ((int32)insertion_effect_gs.type_msb << 8) | (int32)insertion_effect_gs.type_lsb;
			if(temp == insertion_effect_gs.type) {
				recompute_insertion_effect_gs();
			} else {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "EFX TYPE (%02X %02X)", insertion_effect_gs.type_msb, insertion_effect_gs.type_lsb);
				realloc_insertion_effect_gs();
			}
			break;
		case 0x29:
			insertion_effect_gs.parameter[0] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2A:
			insertion_effect_gs.parameter[1] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2B:
			insertion_effect_gs.parameter[2] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2C:
			insertion_effect_gs.parameter[3] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2D:
			insertion_effect_gs.parameter[4] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2E:
			insertion_effect_gs.parameter[5] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x2F:
			insertion_effect_gs.parameter[6] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x30:
			insertion_effect_gs.parameter[7] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x31:
			insertion_effect_gs.parameter[8] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x32:
			insertion_effect_gs.parameter[9] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x33:
			insertion_effect_gs.parameter[10] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x34:
			insertion_effect_gs.parameter[11] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x35:
			insertion_effect_gs.parameter[12] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x36:
			insertion_effect_gs.parameter[13] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x37:
			insertion_effect_gs.parameter[14] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x38:
			insertion_effect_gs.parameter[15] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x39:
			insertion_effect_gs.parameter[16] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3A:
			insertion_effect_gs.parameter[17] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3B:
			insertion_effect_gs.parameter[18] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3C:
			insertion_effect_gs.parameter[19] = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3D:
			insertion_effect_gs.send_reverb = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3E:
			insertion_effect_gs.send_chorus = val;
			recompute_insertion_effect_gs();
			break;
		case 0x3F:
			insertion_effect_gs.send_delay = val;
			recompute_insertion_effect_gs();
			break;
		case 0x40:
			insertion_effect_gs.control_source1 = val;
			recompute_insertion_effect_gs();
			break;
		case 0x41:
			insertion_effect_gs.control_depth1 = val;
			recompute_insertion_effect_gs();
			break;
		case 0x42:
			insertion_effect_gs.control_source2 = val;
			recompute_insertion_effect_gs();
			break;
		case 0x43:
			insertion_effect_gs.control_depth2 = val;
			recompute_insertion_effect_gs();
			break;
		case 0x44:
			insertion_effect_gs.send_eq_switch = val;
			recompute_insertion_effect_gs();
			break;
		case 0x45:	/* Rx. Channel */
			reset_controllers(ch);
			redraw_controllers(ch);
			all_notes_off(ch);
			if (val == 0x80)
				remove_channel_layer(ch);
			else
				add_channel_layer(ch, val);
			break;
		case 0x46:	/* Channel Msg Rx Port */
			reset_controllers(ch);
			redraw_controllers(ch);
			all_notes_off(ch);
			channel[ch].port_select = val;
			break;
		case 0x47:	/* Play Note Number */
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			channel[ch].drums[note]->play_note = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Play Note (CH:%d NOTE:%d VAL:%d)",
				ch, note, channel[ch].drums[note]->play_note);
			channel[ch].pitchfactor = 0;
			break;
		default:
			break;
		}
		return;
	} else if(ev == ME_SYSEX_XG_LSB) {	/* XG system exclusive message */
		msb = channel[ch].sysex_xg_msb_addr;
		note = channel[ch].sysex_xg_msb_val;
		if (note == 3 && msb == 0) {	/* Effect 2 */
		note = 0;	/* force insertion effect num 0 ?? */
		if (note >= XG_INSERTION_EFFECT_NUM || note < 0) {return;}
		switch(b)
		{
		case 0x00:	/* Insertion Effect Type MSB */
			if (insertion_effect_xg[note].type_msb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Type MSB (%d %02X)", note, val);
				insertion_effect_xg[note].type_msb = val;
				realloc_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x01:	/* Insertion Effect Type LSB */
			if (insertion_effect_xg[note].type_lsb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Type LSB (%d %02X)", note, val);
				insertion_effect_xg[note].type_lsb = val;
				realloc_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x02:	/* Insertion Effect Parameter 1 - 10 */
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
			if (insertion_effect_xg[note].use_msb) {break;}
			temp = b - 0x02;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Parameter %d (%d %d)", temp + 1, note, val);
			if (insertion_effect_xg[note].param_lsb[temp] != val) {
				insertion_effect_xg[note].param_lsb[temp] = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x0C:	/* Insertion Effect Part */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Part (%d %d)", note, val);
			if (insertion_effect_xg[note].part != val) {
				insertion_effect_xg[note].part = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x0D:	/* MW Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MW Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].mw_depth != val) {
				insertion_effect_xg[note].mw_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x0E:	/* BEND Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].bend_depth != val) {
				insertion_effect_xg[note].bend_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x0F:	/* CAT Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAT Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].cat_depth != val) {
				insertion_effect_xg[note].cat_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x10:	/* AC1 Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC1 Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].ac1_depth != val) {
				insertion_effect_xg[note].ac1_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x11:	/* AC2 Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC2 Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].ac2_depth != val) {
				insertion_effect_xg[note].ac2_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x12:	/* CBC1 Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CBC1 Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].cbc1_depth != val) {
				insertion_effect_xg[note].cbc1_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x13:	/* CBC2 Insertion Control Depth */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CBC2 Insertion Control Depth (%d %d)", note, val);
			if (insertion_effect_xg[note].cbc2_depth != val) {
				insertion_effect_xg[note].cbc2_depth = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x20:	/* Insertion Effect Parameter 11 - 16 */
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
			temp = b - 0x20 + 10;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Parameter %d (%d %d)", temp + 1, note, val);
			if (insertion_effect_xg[note].param_lsb[temp] != val) {
				insertion_effect_xg[note].param_lsb[temp] = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x30:	/* Insertion Effect Parameter 1 - 10 MSB */
		case 0x32:
		case 0x34:
		case 0x36:
		case 0x38:
		case 0x3A:
		case 0x3C:
		case 0x3E:
		case 0x40:
		case 0x42:
			if (!insertion_effect_xg[note].use_msb) {break;}
			temp = (b - 0x30) / 2;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Parameter %d MSB (%d %d)", temp + 1, note, val);
			if (insertion_effect_xg[note].param_msb[temp] != val) {
				insertion_effect_xg[note].param_msb[temp] = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		case 0x31:	/* Insertion Effect Parameter 1 - 10 LSB */
		case 0x33:
		case 0x35:
		case 0x37:
		case 0x39:
		case 0x3B:
		case 0x3D:
		case 0x3F:
		case 0x41:
		case 0x43:
			if (!insertion_effect_xg[note].use_msb) {break;}
			temp = (b - 0x31) / 2;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Insertion Effect Parameter %d LSB (%d %d)", temp + 1, note, val);
			if (insertion_effect_xg[note].param_lsb[temp] != val) {
				insertion_effect_xg[note].param_lsb[temp] = val;
				recompute_effect_xg(&insertion_effect_xg[note]);
			}
			break;
		default:
			break;
		}
		} else if (note == 2 && msb == 1) {	/* Effect 1 */
		note = 0;	/* force variation effect num 0 ?? */
		switch(b)
		{
		case 0x00:	/* Reverb Type MSB */
			if (reverb_status_xg.type_msb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Type MSB (%02X)", val);
				reverb_status_xg.type_msb = val;
				realloc_effect_xg(&reverb_status_xg);
			}
			break;
		case 0x01:	/* Reverb Type LSB */
			if (reverb_status_xg.type_lsb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Type LSB (%02X)", val);
				reverb_status_xg.type_lsb = val;
				realloc_effect_xg(&reverb_status_xg);
			}
			break;
		case 0x02:	/* Reverb Parameter 1 - 10 */
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Parameter %d (%d)", b - 0x02 + 1, val);
			if (reverb_status_xg.param_lsb[b - 0x02] != val) {
				reverb_status_xg.param_lsb[b - 0x02] = val;
				recompute_effect_xg(&reverb_status_xg);
			}
			break;
		case 0x0C:	/* Reverb Return */
#if 0	/* XG specific reverb is not currently implemented */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Return (%d)", val);
			if (reverb_status_xg.ret != val) {
				reverb_status_xg.ret = val;
				recompute_effect_xg(&reverb_status_xg);
			}
#else	/* use GS reverb instead */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Reverb Return (%d)", val);
			if (reverb_status_gs.level != val) {
				reverb_status_gs.level = val;
				recompute_reverb_status_gs();
				init_reverb();
			}
#endif
			break;
		case 0x0D:	/* Reverb Pan */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Pan (%d)", val);
			if (reverb_status_xg.pan != val) {
				reverb_status_xg.pan = val;
				recompute_effect_xg(&reverb_status_xg);
			}
			break;
		case 0x10:	/* Reverb Parameter 11 - 16 */
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
			temp = b - 0x10 + 10;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Reverb Parameter %d (%d)", temp + 1, val);
			if (reverb_status_xg.param_lsb[temp] != val) {
				reverb_status_xg.param_lsb[temp] = val;
				recompute_effect_xg(&reverb_status_xg);
			}
			break;
		case 0x20:	/* Chorus Type MSB */
			if (chorus_status_xg.type_msb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Type MSB (%02X)", val);
				chorus_status_xg.type_msb = val;
				realloc_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x21:	/* Chorus Type LSB */
			if (chorus_status_xg.type_lsb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Type LSB (%02X)", val);
				chorus_status_xg.type_lsb = val;
				realloc_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x22:	/* Chorus Parameter 1 - 10 */
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Parameter %d (%d)", b - 0x22 + 1, val);
			if (chorus_status_xg.param_lsb[b - 0x22] != val) {
				chorus_status_xg.param_lsb[b - 0x22] = val;
				recompute_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x2C:	/* Chorus Return */
#if 0	/* XG specific chorus is not currently implemented */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Return (%d)", val);
			if (chorus_status_xg.ret != val) {
				chorus_status_xg.ret = val;
				recompute_effect_xg(&chorus_status_xg);
			}
#else	/* use GS chorus instead */
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Return (%d)", val);
			if (chorus_status_gs.level != val) {
				chorus_status_gs.level = val;
				recompute_chorus_status_gs();
				init_ch_chorus();
			}
#endif
			break;
		case 0x2D:	/* Chorus Pan */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Pan (%d)", val);
			if (chorus_status_xg.pan != val) {
				chorus_status_xg.pan = val;
				recompute_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x2E:	/* Send Chorus To Reverb */
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Send Chorus To Reverb (%d)", val);
			if (chorus_status_xg.send_reverb != val) {
				chorus_status_xg.send_reverb = val;
				recompute_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x30:	/* Chorus Parameter 11 - 16 */
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
			temp = b - 0x30 + 10;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Chorus Parameter %d (%d)", temp + 1, val);
			if (chorus_status_xg.param_lsb[temp] != val) {
				chorus_status_xg.param_lsb[temp] = val;
				recompute_effect_xg(&chorus_status_xg);
			}
			break;
		case 0x40:	/* Variation Type MSB */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			if (variation_effect_xg[note].type_msb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Type MSB (%02X)", val);
				variation_effect_xg[note].type_msb = val;
				realloc_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x41:	/* Variation Type LSB */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			if (variation_effect_xg[note].type_lsb != val) {
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Type LSB (%02X)", val);
				variation_effect_xg[note].type_lsb = val;
				realloc_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x42:	/* Variation Parameter 1 - 10 MSB */
		case 0x44:
		case 0x46:
		case 0x48:
		case 0x4A:
		case 0x4C:
		case 0x4E:
		case 0x50:
		case 0x52:
		case 0x54:
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			temp = (b - 0x42) / 2;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Parameter %d MSB (%d)", temp, val);
			if (variation_effect_xg[note].param_msb[temp] != val) {
				variation_effect_xg[note].param_msb[temp] = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x43:	/* Variation Parameter 1 - 10 LSB */
		case 0x45:
		case 0x47:
		case 0x49:
		case 0x4B:
		case 0x4D:
		case 0x4F:
		case 0x51:
		case 0x53:
		case 0x55:
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			temp = (b - 0x43) / 2;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Parameter %d LSB (%d)", temp, val);
			if (variation_effect_xg[note].param_lsb[temp] != val) {
				variation_effect_xg[note].param_lsb[temp] = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x56:	/* Variation Return */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Return (%d)", val);
			if (variation_effect_xg[note].ret != val) {
				variation_effect_xg[note].ret = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x57:	/* Variation Pan */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Pan (%d)", val);
			if (variation_effect_xg[note].pan != val) {
				variation_effect_xg[note].pan = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x58:	/* Send Variation To Reverb */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Send Variation To Reverb (%d)", val);
			if (variation_effect_xg[note].send_reverb != val) {
				variation_effect_xg[note].send_reverb = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x59:	/* Send Variation To Chorus */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Send Variation To Chorus (%d)", val);
			if (variation_effect_xg[note].send_chorus != val) {
				variation_effect_xg[note].send_chorus = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5A:	/* Variation Connection */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Connection (%d)", val);
			if (variation_effect_xg[note].connection != val) {
				variation_effect_xg[note].connection = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5B:	/* Variation Part */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Part (%d)", val);
			if (variation_effect_xg[note].part != val) {
				variation_effect_xg[note].part = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5C:	/* MW Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "MW Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].mw_depth != val) {
				variation_effect_xg[note].mw_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5D:	/* BEND Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "BEND Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].bend_depth != val) {
				variation_effect_xg[note].bend_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5E:	/* CAT Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CAT Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].cat_depth != val) {
				variation_effect_xg[note].cat_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x5F:	/* AC1 Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC1 Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].ac1_depth != val) {
				variation_effect_xg[note].ac1_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x60:	/* AC2 Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "AC2 Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].ac2_depth != val) {
				variation_effect_xg[note].ac2_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x61:	/* CBC1 Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CBC1 Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].cbc1_depth != val) {
				variation_effect_xg[note].cbc1_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x62:	/* CBC2 Variation Control Depth */
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "CBC2 Variation Control Depth (%d)", val);
			if (variation_effect_xg[note].cbc2_depth != val) {
				variation_effect_xg[note].cbc2_depth = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		case 0x70:	/* Variation Parameter 11 - 16 */
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
			temp = b - 0x70 + 10;
			if (note >= XG_VARIATION_EFFECT_NUM || note < 0) {break;}
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Variation Parameter %d (%d)", temp + 1, val);
			if (variation_effect_xg[note].param_lsb[temp] != val) {
				variation_effect_xg[note].param_lsb[temp] = val;
				recompute_effect_xg(&variation_effect_xg[note]);
			}
			break;
		default:
			break;
		}
		} else if (note == 2 && msb == 40) {	/* Multi EQ */
		switch(b)
		{
		case 0x00:	/* EQ type */
			if(opt_eq_control) {
				if(val == 0) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ type (0: Flat)");}
				else if(val == 1) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ type (1: Jazz)");}
				else if(val == 2) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ type (2: Pops)");}
				else if(val == 3) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ type (3: Rock)");}
				else if(val == 4) {ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ type (4: Concert)");}
				multi_eq_xg.type = val;
				set_multi_eq_type_xg(val);
				recompute_multi_eq_xg();
			}
			break;
		case 0x01:	/* EQ gain1 */
			if(opt_eq_control) {
				if(val > 0x4C) {val = 0x4C;}
				else if(val < 0x34) {val = 0x34;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ gain1 (%d dB)", val - 0x40);
				multi_eq_xg.gain1 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x02:	/* EQ frequency1 */
			if(opt_eq_control) {
				if(val > 60) {val = 60;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ frequency1 (%d Hz)", (int32)eq_freq_table_xg[val]);
				multi_eq_xg.freq1 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x03:	/* EQ Q1 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ Q1 (%f)", (double)val / 10.0);
				multi_eq_xg.q1 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x04:	/* EQ shape1 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ shape1 (%d)", val);
				multi_eq_xg.shape1 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x05:	/* EQ gain2 */
			if(opt_eq_control) {
				if(val > 0x4C) {val = 0x4C;}
				else if(val < 0x34) {val = 0x34;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ gain2 (%d dB)", val - 0x40);
				multi_eq_xg.gain2 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x06:	/* EQ frequency2 */
			if(opt_eq_control) {
				if(val > 60) {val = 60;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ frequency2 (%d Hz)", (int32)eq_freq_table_xg[val]);
				multi_eq_xg.freq2 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x07:	/* EQ Q2 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ Q2 (%f)", (double)val / 10.0);
				multi_eq_xg.q2 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x09:	/* EQ gain3 */
			if(opt_eq_control) {
				if(val > 0x4C) {val = 0x4C;}
				else if(val < 0x34) {val = 0x34;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ gain3 (%d dB)", val - 0x40);
				multi_eq_xg.gain3 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x0A:	/* EQ frequency3 */
			if(opt_eq_control) {
				if(val > 60) {val = 60;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ frequency3 (%d Hz)", (int32)eq_freq_table_xg[val]);
				multi_eq_xg.freq3 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x0B:	/* EQ Q3 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ Q3 (%f)", (double)val / 10.0);
				multi_eq_xg.q3 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x0D:	/* EQ gain4 */
			if(opt_eq_control) {
				if(val > 0x4C) {val = 0x4C;}
				else if(val < 0x34) {val = 0x34;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ gain4 (%d dB)", val - 0x40);
				multi_eq_xg.gain4 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x0E:	/* EQ frequency4 */
			if(opt_eq_control) {
				if(val > 60) {val = 60;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ frequency4 (%d Hz)", (int32)eq_freq_table_xg[val]);
				multi_eq_xg.freq4 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x0F:	/* EQ Q4 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ Q4 (%f)", (double)val / 10.0);
				multi_eq_xg.q4 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x11:	/* EQ gain5 */
			if(opt_eq_control) {
				if(val > 0x4C) {val = 0x4C;}
				else if(val < 0x34) {val = 0x34;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ gain5 (%d dB)", val - 0x40);
				multi_eq_xg.gain5 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x12:	/* EQ frequency5 */
			if(opt_eq_control) {
				if(val > 60) {val = 60;}
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ frequency5 (%d Hz)", (int32)eq_freq_table_xg[val]);
				multi_eq_xg.freq5 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x13:	/* EQ Q5 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ Q5 (%f)", (double)val / 10.0);
				multi_eq_xg.q5 = val;
				recompute_multi_eq_xg();
			}
			break;
		case 0x14:	/* EQ shape5 */
			if(opt_eq_control) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ shape5 (%d)", val);
				multi_eq_xg.shape5 = val;
				recompute_multi_eq_xg();
			}
			break;
		}
		} else if (note == 8 && msb == 0) {	/* Multi Part */
		switch(b)
		{
		case 0x99:	/* Rcv CHANNEL, remapped from 0x04 */
			reset_controllers(ch);
			redraw_controllers(ch);
			all_notes_off(ch);
			if (val == 0x7f)
				remove_channel_layer(ch);
			else {
				if((ch < REDUCE_CHANNELS) != (val < REDUCE_CHANNELS)) {
					channel[ch].port_select = ch < REDUCE_CHANNELS ? 1 : 0;
				}
				if((ch % REDUCE_CHANNELS) != (val % REDUCE_CHANNELS)) {
					add_channel_layer(ch, val);
				}
			}
			break;
		case 0x06:	/* Same Note Number Key On Assign */
			if(val == 0) {
				channel[ch].assign_mode = 0;
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Same Note Number Key On Assign: Single (CH:%d)",ch);
			} else if(val == 1) {
				channel[ch].assign_mode = 2;
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Same Note Number Key On Assign: Multi (CH:%d)",ch);
			} else if(val == 2) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Same Note Number Key On Assign: Inst is not supported. (CH:%d)",ch);
			}
			break;
		case 0x11:	/* Dry Level */
			channel[ch].dry_level = val;
			ctl->cmsg(CMSG_INFO, VERB_NOISY, "Dry Level (CH:%d VAL:%d)", ch, val);
			break;
		}
		} else if ((note & 0xF0) == 0x30) {	/* Drum Setup */
		note = msb;
		switch(b)
		{
		case 0x0E:	/* EG Decay1 */
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument EG Decay1 (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
			channel[ch].drums[note]->drum_envelope_rate[EG_DECAY1] = val;
			break;
		case 0x0F:	/* EG Decay2 */
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument EG Decay2 (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
			channel[ch].drums[note]->drum_envelope_rate[EG_DECAY2] = val;
			break;
		default:
			break;
		}
		}
		return;
	}
}

static void play_midi_prescan(MidiEvent *ev)
{
    int i, j, k, ch, orig_ch, port_ch, offset, layered;
    
    if(opt_amp_compensation) {mainvolume_max = 0;}
    else {mainvolume_max = 0x7f;}
    compensation_ratio = 1.0;

    prescanning_flag = 1;
    change_system_mode(DEFAULT_SYSTEM_MODE);
    reset_midi(0);
    resamp_cache_reset();

	while (ev->type != ME_EOT) {
#ifndef SUPPRESS_CHANNEL_LAYER
		orig_ch = ev->channel;
		layered = ! IS_SYSEX_EVENT_TYPE(ev);
		for (j = 0; j < MAX_CHANNELS; j += 16) {
			port_ch = (orig_ch + j) % MAX_CHANNELS;
			offset = port_ch & ~0xf;
			for (k = offset; k < offset + 16; k++) {
				if (! layered && (j || k != offset))
					continue;
				if (layered) {
					if (! IS_SET_CHANNELMASK(
							channel[k].channel_layer, port_ch)
							|| channel[k].port_select != (orig_ch >> 4))
						continue;
					ev->channel = k;
				}
#endif
	ch = ev->channel;

	switch(ev->type)
	{
	  case ME_NOTEON:
		note_on_prescan(ev);
	    break;

	  case ME_NOTEOFF:
	    resamp_cache_refer_off(ch, MIDI_EVENT_NOTE(ev), ev->time);
	    break;

	  case ME_PORTAMENTO_TIME_MSB:
	    channel[ch].portamento_time_msb = ev->a;
	    break;

	  case ME_PORTAMENTO_TIME_LSB:
	    channel[ch].portamento_time_lsb = ev->a;
	    break;

	  case ME_PORTAMENTO:
	    channel[ch].portamento = (ev->a >= 64);

	  case ME_RESET_CONTROLLERS:
	    reset_controllers(ch);
	    resamp_cache_refer_alloff(ch, ev->time);
	    break;

	  case ME_PROGRAM:
	    midi_program_change(ch, ev->a);
	    break;

	  case ME_TONE_BANK_MSB:
	    channel[ch].bank_msb = ev->a;
	    break;

	  case ME_TONE_BANK_LSB:
	    channel[ch].bank_lsb = ev->a;
	    break;

	  case ME_RESET:
	    change_system_mode(ev->a);
	    reset_midi(0);
	    break;

	  case ME_PITCHWHEEL:
	  case ME_ALL_NOTES_OFF:
	  case ME_ALL_SOUNDS_OFF:
	  case ME_MONO:
	  case ME_POLY:
	    resamp_cache_refer_alloff(ch, ev->time);
	    break;

	  case ME_DRUMPART:
	    if(midi_drumpart_change(ch, ev->a))
		midi_program_change(ch, channel[ch].program);
	    break;

	  case ME_KEYSHIFT:
	    resamp_cache_refer_alloff(ch, ev->time);
	    channel[ch].key_shift = (int)ev->a - 0x40;
	    break;

	  case ME_SCALE_TUNING:
		resamp_cache_refer_alloff(ch, ev->time);
		channel[ch].scale_tuning[ev->a] = ev->b;
		break;

	  case ME_MAINVOLUME:
	    if (ev->a > mainvolume_max) {
	      mainvolume_max = ev->a;
	      ctl->cmsg(CMSG_INFO,VERB_DEBUG,"ME_MAINVOLUME/max (CH:%d VAL:%#x)",ev->channel,ev->a);
	    }
	    break;
	}
#ifndef SUPPRESS_CHANNEL_LAYER
			}
		}
		ev->channel = orig_ch;
#endif
	ev++;
    }

    /* calculate compensation ratio */
    if (0 < mainvolume_max && mainvolume_max < 0x7f) {
      compensation_ratio = pow((double)0x7f/(double)mainvolume_max, 4);
      ctl->cmsg(CMSG_INFO,VERB_DEBUG,"Compensation ratio:%lf",compensation_ratio);
    }

    for(i = 0; i < MAX_CHANNELS; i++)
	resamp_cache_refer_alloff(i, ev->time);
    resamp_cache_create();
    prescanning_flag = 0;
}

/*! convert GS NRPN to vibrato rate ratio. */
/* from 0 to 3.0. */
static double gs_cnv_vib_rate(int rate)
{
	double ratio;

	if(rate == 0) {
		ratio = 1.6 / 100.0;
	} else if(rate == 64) {
		ratio = 1.0;
	} else if(rate <= 100) {
		ratio = (double)rate * 1.6 / 100.0;
	} else {
		ratio = (double)(rate - 101) * 1.33 / 26.0 + 1.67;
	}
	return (1.0 / ratio);
}

/*! convert GS NRPN to vibrato depth. */
/* from -9.6 cents to +9.45 cents. */
static int32 gs_cnv_vib_depth(int depth)
{
	double cent;
	cent = (double)(depth - 64) * 0.15;

	return (int32)(cent * 256.0 / 400.0);
}

/*! convert GS NRPN to vibrato delay. */
/* from 0 ms to 5074 ms. */
static int32 gs_cnv_vib_delay(int delay)
{
	double ms;
	ms = 0.2092 * exp(0.0795 * (double)delay);
	if(delay == 0) {ms = 0;}
	return (int32)((double)play_mode->rate * ms * 0.001);
}

static int last_rpn_addr(int ch)
{
	int lsb, msb, addr, i;
	struct rpn_tag_map_t *addrmap;
	struct rpn_tag_map_t {
		int addr, mask, tag;
	};
	static struct rpn_tag_map_t nrpn_addr_map[] = {
		{0x0108, 0xffff, NRPN_ADDR_0108},
		{0x0109, 0xffff, NRPN_ADDR_0109},
		{0x010a, 0xffff, NRPN_ADDR_010A},
		{0x0120, 0xffff, NRPN_ADDR_0120},
		{0x0121, 0xffff, NRPN_ADDR_0121},
		{0x0130, 0xffff, NRPN_ADDR_0130},
		{0x0131, 0xffff, NRPN_ADDR_0131},
		{0x0134, 0xffff, NRPN_ADDR_0134},
		{0x0135, 0xffff, NRPN_ADDR_0135},
		{0x0163, 0xffff, NRPN_ADDR_0163},
		{0x0164, 0xffff, NRPN_ADDR_0164},
		{0x0166, 0xffff, NRPN_ADDR_0166},
		{0x1400, 0xff00, NRPN_ADDR_1400},
		{0x1500, 0xff00, NRPN_ADDR_1500},
		{0x1600, 0xff00, NRPN_ADDR_1600},
		{0x1700, 0xff00, NRPN_ADDR_1700},
		{0x1800, 0xff00, NRPN_ADDR_1800},
		{0x1900, 0xff00, NRPN_ADDR_1900},
		{0x1a00, 0xff00, NRPN_ADDR_1A00},
		{0x1c00, 0xff00, NRPN_ADDR_1C00},
		{0x1d00, 0xff00, NRPN_ADDR_1D00},
		{0x1e00, 0xff00, NRPN_ADDR_1E00},
		{0x1f00, 0xff00, NRPN_ADDR_1F00},
		{0x3000, 0xff00, NRPN_ADDR_3000},
		{0x3100, 0xff00, NRPN_ADDR_3100},
		{0x3400, 0xff00, NRPN_ADDR_3400},
		{0x3500, 0xff00, NRPN_ADDR_3500},
		{-1, -1, 0}
	};
	static struct rpn_tag_map_t rpn_addr_map[] = {
		{0x0000, 0xffff, RPN_ADDR_0000},
		{0x0001, 0xffff, RPN_ADDR_0001},
		{0x0002, 0xffff, RPN_ADDR_0002},
		{0x0003, 0xffff, RPN_ADDR_0003},
		{0x0004, 0xffff, RPN_ADDR_0004},
		{0x0005, 0xffff, RPN_ADDR_0005},
		{0x7f7f, 0xffff, RPN_ADDR_7F7F},
		{0xffff, 0xffff, RPN_ADDR_FFFF},
		{-1, -1}
	};
	
	if (channel[ch].nrpn == -1)
		return -1;
	lsb = channel[ch].lastlrpn;
	msb = channel[ch].lastmrpn;
	if (lsb == 0xff || msb == 0xff)
		return -1;
	addr = (msb << 8 | lsb);
	if (channel[ch].nrpn)
		addrmap = nrpn_addr_map;
	else
		addrmap = rpn_addr_map;
	for (i = 0; addrmap[i].addr != -1; i++)
		if (addrmap[i].addr == (addr & addrmap[i].mask))
			return addrmap[i].tag;
	return -1;
}

static void update_channel_freq(int ch)
{
	int i, uv = upper_voices;
	for (i = 0; i < uv; i++)
		if (voice[i].status != VOICE_FREE && voice[i].channel == ch)
	recompute_freq(i);
}

static void update_rpn_map(int ch, int addr, int update_now)
{
	int val, drumflag, i, note;
	
	val = channel[ch].rpnmap[addr];
	drumflag = 0;
	switch (addr) {
	case NRPN_ADDR_0108:	/* Vibrato Rate */
		if (opt_nrpn_vibrato) {
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Vibrato Rate (CH:%d VAL:%d)", ch, val - 64);
			channel[ch].vibrato_ratio = gs_cnv_vib_rate(val);
		}
		if (update_now)
			update_channel_freq(ch);
		break;
	case NRPN_ADDR_0109:	/* Vibrato Depth */
		if (opt_nrpn_vibrato) {
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Vibrato Depth (CH:%d VAL:%d)", ch, val - 64);
			channel[ch].vibrato_depth = gs_cnv_vib_depth(val);
		}
		if (update_now)
			update_channel_freq(ch);
		break;
	case NRPN_ADDR_010A:	/* Vibrato Delay */
		if (opt_nrpn_vibrato) {
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Vibrato Delay (CH:%d VAL:%d)", ch, val);
			channel[ch].vibrato_delay = gs_cnv_vib_delay(val);
		}
		if (update_now)
			update_channel_freq(ch);
		break;
	case NRPN_ADDR_0120:	/* Filter Cutoff Frequency */
		if (opt_lpf_def) {
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Filter Cutoff (CH:%d VAL:%d)", ch, val - 64);
			channel[ch].param_cutoff_freq = val - 64;
		}
		break;
	case NRPN_ADDR_0121:	/* Filter Resonance */
		if (opt_lpf_def) {
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Filter Resonance (CH:%d VAL:%d)", ch, val - 64);
			channel[ch].param_resonance = val - 64;
		}
		break;
	case NRPN_ADDR_0130:	/* EQ BASS */
		if (opt_eq_control) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ BASS (CH:%d %.2f dB)", ch, 0.19 * (double)(val - 0x40));
			channel[ch].eq_xg.bass = val;
			recompute_part_eq_xg(&(channel[ch].eq_xg));
		}
		break;
	case NRPN_ADDR_0131:	/* EQ TREBLE */
		if (opt_eq_control) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ TREBLE (CH:%d %.2f dB)", ch, 0.19 * (double)(val - 0x40));
			channel[ch].eq_xg.treble = val;
			recompute_part_eq_xg(&(channel[ch].eq_xg));
		}
		break;
	case NRPN_ADDR_0134:	/* EQ BASS frequency */
		if (opt_eq_control) {
			if(val < 4) {val = 4;}
			else if(val > 40) {val = 40;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ BASS frequency (CH:%d %d Hz)", ch, (int32)eq_freq_table_xg[val]);
			channel[ch].eq_xg.bass_freq = val;
			recompute_part_eq_xg(&(channel[ch].eq_xg));
		}
		break;
	case NRPN_ADDR_0135:	/* EQ TREBLE frequency */
		if (opt_eq_control) {
			if(val < 28) {val = 28;}
			else if(val > 58) {val = 58;}
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"EQ TREBLE frequency (CH:%d %d Hz)", ch, (int32)eq_freq_table_xg[val]);
			channel[ch].eq_xg.treble_freq = val;
			recompute_part_eq_xg(&(channel[ch].eq_xg));
		}
		break;
	case NRPN_ADDR_0163:	/* Attack Time */
		if (opt_tva_attack) {set_envelope_time(ch, val, EG_ATTACK);}
		break;
	case NRPN_ADDR_0164:	/* EG Decay Time */
		if (opt_tva_decay) {set_envelope_time(ch, val, EG_DECAY);}
		break;
	case NRPN_ADDR_0166:	/* EG Release Time */
		if (opt_tva_release) {set_envelope_time(ch, val, EG_RELEASE);}
		break;
	case NRPN_ADDR_1400:	/* Drum Filter Cutoff (XG) */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Filter Cutoff (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
		channel[ch].drums[note]->drum_cutoff_freq = val - 64;
		break;
	case NRPN_ADDR_1500:	/* Drum Filter Resonance (XG) */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Filter Resonance (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
		channel[ch].drums[note]->drum_resonance = val - 64;
		break;
	case NRPN_ADDR_1600:	/* Drum EG Attack Time (XG) */
		drumflag = 1;
		if (opt_tva_attack) {
			val = val & 0x7f;
			note = channel[ch].lastlrpn;
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			val	-= 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Drum Instrument Attack Time (CH:%d NOTE:%d VAL:%d)",
					ch, note, val);
			channel[ch].drums[note]->drum_envelope_rate[EG_ATTACK] = val;
		}
		break;
	case NRPN_ADDR_1700:	/* Drum EG Decay Time (XG) */
		drumflag = 1;
		if (opt_tva_decay) {
			val = val & 0x7f;
			note = channel[ch].lastlrpn;
			if (channel[ch].drums[note] == NULL)
				play_midi_setup_drums(ch, note);
			val	-= 64;
			ctl->cmsg(CMSG_INFO, VERB_NOISY,
					"Drum Instrument Decay Time (CH:%d NOTE:%d VAL:%d)",
					ch, note, val);
			channel[ch].drums[note]->drum_envelope_rate[EG_DECAY1] =
				channel[ch].drums[note]->drum_envelope_rate[EG_DECAY2] = val;
		}
		break;
	case NRPN_ADDR_1800:	/* Coarse Pitch of Drum (GS) */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		channel[ch].drums[note]->coarse = val - 64;
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
			"Drum Instrument Pitch Coarse (CH:%d NOTE:%d VAL:%d)",
			ch, note, channel[ch].drums[note]->coarse);
		channel[ch].pitchfactor = 0;
		break;
	case NRPN_ADDR_1900:	/* Fine Pitch of Drum (XG) */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		channel[ch].drums[note]->fine = val - 64;
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument Pitch Fine (CH:%d NOTE:%d VAL:%d)",
				ch, note, channel[ch].drums[note]->fine);
		channel[ch].pitchfactor = 0;
		break;
	case NRPN_ADDR_1A00:	/* Level of Drum */	 
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Drum Instrument TVA Level (CH:%d NOTE:%d VAL:%d)",
				ch, note, val);
		channel[ch].drums[note]->drum_level =
				calc_drum_tva_level(ch, note, val);
		break;
	case NRPN_ADDR_1C00:	/* Panpot of Drum */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		if(val == 0) {
			val = int_rand(128);
			channel[ch].drums[note]->pan_random = 1;
		} else
			channel[ch].drums[note]->pan_random = 0;
		channel[ch].drums[note]->drum_panning = val;
		if (update_now && adjust_panning_immediately
				&& ! channel[ch].pan_random)
			adjust_drum_panning(ch, note);
		break;
	case NRPN_ADDR_1D00:	/* Reverb Send Level of Drum */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Reverb Send Level of Drum (CH:%d NOTE:%d VALUE:%d)",
				ch, note, val);
		if (channel[ch].drums[note]->reverb_level != val) {
			channel[ch].drum_effect_flag = 0;
		}
		channel[ch].drums[note]->reverb_level = val;
		break;
	case NRPN_ADDR_1E00:	/* Chorus Send Level of Drum */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Chorus Send Level of Drum (CH:%d NOTE:%d VALUE:%d)",
				ch, note, val);
		if (channel[ch].drums[note]->chorus_level != val) {
			channel[ch].drum_effect_flag = 0;
		}
		channel[ch].drums[note]->chorus_level = val;
		
		break;
	case NRPN_ADDR_1F00:	/* Variation Send Level of Drum */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Delay Send Level of Drum (CH:%d NOTE:%d VALUE:%d)",
				ch, note, val);
		if (channel[ch].drums[note]->delay_level != val) {
			channel[ch].drum_effect_flag = 0;
		}
		channel[ch].drums[note]->delay_level = val;
		break;
	case NRPN_ADDR_3000:	/* Drum EQ BASS */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		break;
	case NRPN_ADDR_3100:	/* Drum EQ TREBLE */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		break;
	case NRPN_ADDR_3400:	/* Drum EQ BASS frequency */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		break;
	case NRPN_ADDR_3500:	/* Drum EQ TREBLE frequency */
		drumflag = 1;
		note = channel[ch].lastlrpn;
		if (channel[ch].drums[note] == NULL)
			play_midi_setup_drums(ch, note);
		break;
	case RPN_ADDR_0000:		/* Pitch bend sensitivity */
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				"Pitch Bend Sensitivity (CH:%d VALUE:%d)", ch, val);
		/* for mod2mid.c, arpeggio */
		if (! IS_CURRENT_MOD_FILE && channel[ch].rpnmap[RPN_ADDR_0000] > 24)
			channel[ch].rpnmap[RPN_ADDR_0000] = 24;
		channel[ch].pitchfactor = 0;
		break;
	case RPN_ADDR_0001:		/* Master Fine Tuning */
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				"Master Fine Tuning (CH:%d VALUE:%d)", ch, val);
		channel[ch].pitchfactor = 0;
		break;
	case RPN_ADDR_0002:		/* Master Coarse Tuning */
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				"Master Coarse Tuning (CH:%d VALUE:%d)", ch, val);
		channel[ch].pitchfactor = 0;
		break;
	case RPN_ADDR_0003:		/* Tuning Program Select */
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				"Tuning Program Select (CH:%d VALUE:%d)", ch, val);
		for (i = 0; i < upper_voices; i++)
			if (voice[i].status != VOICE_FREE) {
				voice[i].temper_instant = 1;
				recompute_freq(i);
			}
		break;
	case RPN_ADDR_0004:		/* Tuning Bank Select */
		ctl->cmsg(CMSG_INFO, VERB_DEBUG,
				"Tuning Bank Select (CH:%d VALUE:%d)", ch, val);
		for (i = 0; i < upper_voices; i++)
			if (voice[i].status != VOICE_FREE) {
				voice[i].temper_instant = 1;
				recompute_freq(i);
			}
		break;
	case RPN_ADDR_0005:		/* GM2: Modulation Depth Range */
		channel[ch].mod.lfo1_pitch_depth = (((int32)channel[ch].rpnmap[RPN_ADDR_0005] << 7) | channel[ch].rpnmap_lsb[RPN_ADDR_0005]) * 100 / 128;
		ctl->cmsg(CMSG_INFO, VERB_NOISY,
				"Modulation Depth Range (CH:%d VALUE:%d)", ch, channel[ch].rpnmap[RPN_ADDR_0005]);
		break;
	case RPN_ADDR_7F7F:		/* RPN reset */
		channel[ch].rpn_7f7f_flag = 1;
		break;
	case RPN_ADDR_FFFF:		/* RPN initialize */
		/* All reset to defaults */
		channel[ch].rpn_7f7f_flag = 0;
		memset(channel[ch].rpnmap, 0, sizeof(channel[ch].rpnmap));
		channel[ch].lastlrpn = channel[ch].lastmrpn = 0;
		channel[ch].nrpn = 0;
		channel[ch].rpnmap[RPN_ADDR_0000] = 2;
		channel[ch].rpnmap[RPN_ADDR_0001] = 0x40;
		channel[ch].rpnmap[RPN_ADDR_0002] = 0x40;
		channel[ch].rpnmap_lsb[RPN_ADDR_0005] = 0x40;
		channel[ch].rpnmap[RPN_ADDR_0005] = 0;	/* +- 50 cents */
		channel[ch].pitchfactor = 0;
		break;
	}
	drumflag = 0;
	if (drumflag && midi_drumpart_change(ch, 1)) {
		midi_program_change(ch, channel[ch].program);
		if (update_now)
			ctl_prog_event(ch);
	}
}

static void seek_forward(int32 until_time)
{
    int32 i;
    int j, k, ch, orig_ch, port_ch, offset, layered;

    playmidi_seek_flag = 1;
    wrd_midi_event(WRD_START_SKIP, WRD_NOARG);
	while (MIDI_EVENT_TIME(current_event) < until_time) {
#ifndef SUPPRESS_CHANNEL_LAYER
		orig_ch = current_event->channel;
		layered = ! IS_SYSEX_EVENT_TYPE(current_event);
		for (j = 0; j < MAX_CHANNELS; j += 16) {
			port_ch = (orig_ch + j) % MAX_CHANNELS;
			offset = port_ch & ~0xf;
			for (k = offset; k < offset + 16; k++) {
				if (! layered && (j || k != offset))
					continue;
				if (layered) {
					if (! IS_SET_CHANNELMASK(
							channel[k].channel_layer, port_ch)
							|| channel[k].port_select != (orig_ch >> 4))
						continue;
					current_event->channel = k;
				}
#endif
	ch = current_event->channel;
	
	switch(current_event->type)
	{
	  case ME_PITCHWHEEL:
	    channel[ch].pitchbend = current_event->a + current_event->b * 128;
	    channel[ch].pitchfactor=0;
	    break;

	  case ME_MAINVOLUME:
	    channel[ch].volume = current_event->a;
	    break;

	  case ME_MASTER_VOLUME:
	    master_volume_ratio =
		(int32)current_event->a + 256 * (int32)current_event->b;
	    break;

	  case ME_PAN:
	    channel[ch].panning = current_event->a;
	    channel[ch].pan_random = 0;
	    break;

	  case ME_EXPRESSION:
	    channel[ch].expression=current_event->a;
	    break;

	  case ME_PROGRAM:
	    midi_program_change(ch, current_event->a);
	    break;

	  case ME_SUSTAIN:
		  channel[ch].sustain = current_event->a;
		  if (channel[ch].damper_mode == 0) {	/* half-damper is not allowed. */
			  if (channel[ch].sustain >= 64) {channel[ch].sustain = 127;}
			  else {channel[ch].sustain = 0;}
		  }
	    break;

	  case ME_SOSTENUTO:
		  channel[ch].sostenuto = (current_event->a >= 64);
	    break;

	  case ME_LEGATO_FOOTSWITCH:
        channel[ch].legato = (current_event->a >= 64);
	    break;

      case ME_HOLD2:
        break;

	  case ME_FOOT:
	    break;

	  case ME_BREATH:
	    break;

	  case ME_BALANCE:
	    break;

	  case ME_RESET_CONTROLLERS:
	    reset_controllers(ch);
	    break;

	  case ME_TONE_BANK_MSB:
	    channel[ch].bank_msb = current_event->a;
	    break;

	  case ME_TONE_BANK_LSB:
	    channel[ch].bank_lsb = current_event->a;
	    break;

	  case ME_MODULATION_WHEEL:
	    channel[ch].mod.val = current_event->a;
	    break;

	  case ME_PORTAMENTO_TIME_MSB:
	    channel[ch].portamento_time_msb = current_event->a;
	    break;

	  case ME_PORTAMENTO_TIME_LSB:
	    channel[ch].portamento_time_lsb = current_event->a;
	    break;

	  case ME_PORTAMENTO:
	    channel[ch].portamento = (current_event->a >= 64);
	    break;

	  case ME_MONO:
	    channel[ch].mono = 1;
	    break;

	  case ME_POLY:
	    channel[ch].mono = 0;
	    break;

	  case ME_SOFT_PEDAL:
		  if(opt_lpf_def) {
			  channel[ch].soft_pedal = current_event->a;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Soft Pedal (CH:%d VAL:%d)",ch,channel[ch].soft_pedal);
		  }
		  break;

	  case ME_HARMONIC_CONTENT:
		  if(opt_lpf_def) {
			  channel[ch].param_resonance = current_event->a - 64;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Harmonic Content (CH:%d VAL:%d)",ch,channel[ch].param_resonance);
		  }
		  break;

	  case ME_BRIGHTNESS:
		  if(opt_lpf_def) {
			  channel[ch].param_cutoff_freq = current_event->a - 64;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Brightness (CH:%d VAL:%d)",ch,channel[ch].param_cutoff_freq);
		  }
		  break;

	    /* RPNs */
	  case ME_NRPN_LSB:
	    channel[ch].lastlrpn = current_event->a;
	    channel[ch].nrpn = 1;
	    break;
	  case ME_NRPN_MSB:
	    channel[ch].lastmrpn = current_event->a;
	    channel[ch].nrpn = 1;
	    break;
	  case ME_RPN_LSB:
	    channel[ch].lastlrpn = current_event->a;
	    channel[ch].nrpn = 0;
	    break;
	  case ME_RPN_MSB:
	    channel[ch].lastmrpn = current_event->a;
	    channel[ch].nrpn = 0;
	    break;
	  case ME_RPN_INC:
	    if(channel[ch].rpn_7f7f_flag) /* disable */
		break;
	    if((i = last_rpn_addr(ch)) >= 0)
	    {
		if(channel[ch].rpnmap[i] < 127)
		    channel[ch].rpnmap[i]++;
		update_rpn_map(ch, i, 0);
	    }
	    break;
	case ME_RPN_DEC:
	    if(channel[ch].rpn_7f7f_flag) /* disable */
		break;
	    if((i = last_rpn_addr(ch)) >= 0)
	    {
		if(channel[ch].rpnmap[i] > 0)
		    channel[ch].rpnmap[i]--;
		update_rpn_map(ch, i, 0);
	    }
	    break;
	  case ME_DATA_ENTRY_MSB:
	    if(channel[ch].rpn_7f7f_flag) /* disable */
		break;
	    if((i = last_rpn_addr(ch)) >= 0)
	    {
		channel[ch].rpnmap[i] = current_event->a;
		update_rpn_map(ch, i, 0);
	    }
	    break;
	  case ME_DATA_ENTRY_LSB:
	    if(channel[ch].rpn_7f7f_flag) /* disable */
		break;
	    if((i = last_rpn_addr(ch)) >= 0)
	    {
		channel[ch].rpnmap_lsb[i] = current_event->a;
	    }
	    break;

	  case ME_REVERB_EFFECT:
		  if (opt_reverb_control) {
			if (ISDRUMCHANNEL(ch) && get_reverb_level(ch) != current_event->a) {channel[ch].drum_effect_flag = 0;}
			set_reverb_level(ch, current_event->a);
		  }
	    break;

	  case ME_CHORUS_EFFECT:
		if(opt_chorus_control == 1) {
			if (ISDRUMCHANNEL(ch) && channel[ch].chorus_level != current_event->a) {channel[ch].drum_effect_flag = 0;}
			channel[ch].chorus_level = current_event->a;
		} else {
			channel[ch].chorus_level = -opt_chorus_control;
		}

		if(current_event->a) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Send (CH:%d LEVEL:%d)",ch,current_event->a);
		}
		break;

	  case ME_TREMOLO_EFFECT:
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"Tremolo Send (CH:%d LEVEL:%d)",ch,current_event->a);
		break;

	  case ME_CELESTE_EFFECT:
		if(opt_delay_control) {
			if (ISDRUMCHANNEL(ch) && channel[ch].delay_level != current_event->a) {channel[ch].drum_effect_flag = 0;}
			channel[ch].delay_level = current_event->a;
			if (play_system_mode == XG_SYSTEM_MODE) {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Send (CH:%d LEVEL:%d)",ch,current_event->a);
			} else {
				ctl->cmsg(CMSG_INFO,VERB_NOISY,"Variation Send (CH:%d LEVEL:%d)",ch,current_event->a);
			}
		}
	    break;

	  case ME_ATTACK_TIME:
	  	if(!opt_tva_attack) { break; }
		set_envelope_time(ch, current_event->a, EG_ATTACK);
		break;

	  case ME_RELEASE_TIME:
	  	if(!opt_tva_release) { break; }
		set_envelope_time(ch, current_event->a, EG_RELEASE);
		break;

	  case ME_PHASER_EFFECT:
		ctl->cmsg(CMSG_INFO,VERB_NOISY,"Phaser Send (CH:%d LEVEL:%d)",ch,current_event->a);
		break;

	  case ME_RANDOM_PAN:
	    channel[ch].panning = int_rand(128);
	    channel[ch].pan_random = 1;
	    break;

	  case ME_SET_PATCH:
	    i = channel[ch].special_sample = current_event->a;
	    if(special_patch[i] != NULL)
		special_patch[i]->sample_offset = 0;
	    break;

	  case ME_TEMPO:
	    current_play_tempo = ch +
		current_event->b * 256 + current_event->a * 65536;
	    break;

	  case ME_RESET:
	    change_system_mode(current_event->a);
	    reset_midi(0);
	    break;

	  case ME_PATCH_OFFS:
	    i = channel[ch].special_sample;
	    if(special_patch[i] != NULL)
		special_patch[i]->sample_offset =
		    (current_event->a | 256 * current_event->b);
	    break;

	  case ME_WRD:
	    wrd_midi_event(ch, current_event->a | 256 * current_event->b);
	    break;

	  case ME_SHERRY:
	    wrd_sherry_event(ch |
			     (current_event->a<<8) |
			     (current_event->b<<16));
	    break;

	  case ME_DRUMPART:
	    if(midi_drumpart_change(ch, current_event->a))
		midi_program_change(ch, channel[ch].program);
	    break;

	  case ME_KEYSHIFT:
	    channel[ch].key_shift = (int)current_event->a - 0x40;
	    break;

	case ME_KEYSIG:
		if (opt_init_keysig != 8)
			break;
		current_keysig = current_event->a + current_event->b * 16;
		break;

	case ME_SCALE_TUNING:
		channel[ch].scale_tuning[current_event->a] = current_event->b;
		break;

	case ME_BULK_TUNING_DUMP:
		set_single_note_tuning(ch, current_event->a, current_event->b, 0);
		break;

	case ME_SINGLE_NOTE_TUNING:
		set_single_note_tuning(ch, current_event->a, current_event->b, 0);
		break;

	case ME_TEMPER_KEYSIG:
		current_temper_keysig = (current_event->a + 8) % 32 - 8;
		temper_adj = ((current_event->a + 8) & 0x20) ? 1 : 0;
		break;

	case ME_TEMPER_TYPE:
		channel[ch].temper_type = current_event->a;
		break;

	case ME_MASTER_TEMPER_TYPE:
		for (i = 0; i < MAX_CHANNELS; i++)
			channel[i].temper_type = current_event->a;
		break;

	case ME_USER_TEMPER_ENTRY:
		set_user_temper_entry(ch, current_event->a, current_event->b);
		break;

	  case ME_SYSEX_LSB:
	    process_sysex_event(ME_SYSEX_LSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_SYSEX_MSB:
	    process_sysex_event(ME_SYSEX_MSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_SYSEX_GS_LSB:
	    process_sysex_event(ME_SYSEX_GS_LSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_SYSEX_GS_MSB:
	    process_sysex_event(ME_SYSEX_GS_MSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_SYSEX_XG_LSB:
	    process_sysex_event(ME_SYSEX_XG_LSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_SYSEX_XG_MSB:
	    process_sysex_event(ME_SYSEX_XG_MSB,ch,current_event->a,current_event->b);
	    break;

	  case ME_EOT:
	    current_sample = current_event->time;
	    playmidi_seek_flag = 0;
	    return;
	}
#ifndef SUPPRESS_CHANNEL_LAYER
			}
		}
		current_event->channel = orig_ch;
#endif
	current_event++;
    }
    wrd_midi_event(WRD_END_SKIP, WRD_NOARG);

    playmidi_seek_flag = 0;
    if(current_event != event_list)
	current_event--;
    current_sample = until_time;
}

static void skip_to(int32 until_time)
{
  int ch;

  trace_flush();
  current_event = NULL;

  if (current_sample > until_time)
    current_sample=0;

  change_system_mode(DEFAULT_SYSTEM_MODE);
  reset_midi(0);

  buffered_count=0;
  buffer_pointer=common_buffer;
  current_event=event_list;
  current_play_tempo = 500000; /* 120 BPM */

  if (until_time)
    seek_forward(until_time);
  for(ch = 0; ch < MAX_CHANNELS; ch++)
      channel[ch].lasttime = current_sample;

  ctl_mode_event(CTLE_RESET, 0, 0, 0);
  trace_offset(until_time);

#ifdef SUPPORT_SOUNDSPEC
  soundspec_update_wave(NULL, 0);
#endif /* SUPPORT_SOUNDSPEC */
}

static int32 sync_restart(int only_trace_ok)
{
    int32 cur;

    cur = current_trace_samples();
    if(cur == -1)
    {
	if(only_trace_ok)
	    return -1;
	cur = current_sample;
    }
    aq_flush(1);
    skip_to(cur);
    return cur;
}

static int playmidi_change_rate(int32 rate, int restart)
{
    int arg;

    if(rate == play_mode->rate)
	return 1; /* Not need to change */

    if(rate < MIN_OUTPUT_RATE || rate > MAX_OUTPUT_RATE)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Out of sample rate: %d", rate);
	return -1;
    }

    if(restart)
    {
	if((midi_restart_time = current_trace_samples()) == -1)
	    midi_restart_time = current_sample;
    }
    else
	midi_restart_time = 0;

    arg = (int)rate;
    if(play_mode->acntl(PM_REQ_RATE, &arg) == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Can't change sample rate to %d", rate);
	return -1;
    }

    aq_flush(1);
    aq_setup();
    aq_set_soft_queue(-1.0, -1.0);
    free_instruments(1);
#ifdef SUPPORT_SOUNDSPEC
    soundspec_reinit();
#endif /* SUPPORT_SOUNDSPEC */
    return 0;
}

void playmidi_output_changed(int play_state)
{
    if(target_play_mode == NULL)
	return;
    play_mode = target_play_mode;

    if(play_state == 0)
    {
	/* Playing */
	if((midi_restart_time = current_trace_samples()) == -1)
	    midi_restart_time = current_sample;
    }
    else /* Not playing */
	midi_restart_time = 0;

    if(play_state != 2)
    {
	aq_flush(1);
	aq_setup();
	aq_set_soft_queue(-1.0, -1.0);
	clear_magic_instruments();
    }
    free_instruments(1);
#ifdef SUPPORT_SOUNDSPEC
    soundspec_reinit();
#endif /* SUPPORT_SOUNDSPEC */
    target_play_mode = NULL;
}

int check_apply_control(void)
{
    int rc;
    int32 val;

    if(file_from_stdin)
	return RC_NONE;
    rc = ctl->read(&val);
    switch(rc)
    {
      case RC_CHANGE_VOLUME:
	if (val>0 || amplification > -val)
	    amplification += val;
	else
	    amplification=0;
	if (amplification > MAX_AMPLIFICATION)
	    amplification=MAX_AMPLIFICATION;
	adjust_amplification();
	ctl_mode_event(CTLE_MASTER_VOLUME, 0, amplification, 0);
	break;
      case RC_SYNC_RESTART:
	aq_flush(1);
	break;
      case RC_TOGGLE_PAUSE:
	play_pause_flag = !play_pause_flag;
	ctl_pause_event(play_pause_flag, 0);
	return RC_NONE;
      case RC_TOGGLE_SNDSPEC:
#ifdef SUPPORT_SOUNDSPEC
	if(view_soundspec_flag)
	    close_soundspec();
	else
	    open_soundspec();
	if(view_soundspec_flag || ctl_speana_flag)
	    soundspec_update_wave(NULL, -1);
	return RC_NONE;
      case RC_TOGGLE_CTL_SPEANA:
	ctl_speana_flag = !ctl_speana_flag;
	if(view_soundspec_flag || ctl_speana_flag)
	    soundspec_update_wave(NULL, -1);
#endif /* SUPPORT_SOUNDSPEC */
	return RC_NONE;
      case RC_CHANGE_RATE:
	if(playmidi_change_rate(val, 0))
	    return RC_NONE;
	return RC_RELOAD;
      case RC_OUTPUT_CHANGED:
	playmidi_output_changed(1);
	return RC_RELOAD;
    }
    return rc;
}

static void voice_increment(int n)
{
    int i;
    for(i = 0; i < n; i++)
    {
	if(voices == max_voices)
	    break;
	voice[voices].status = VOICE_FREE;
	voice[voices].temper_instant = 0;
	voice[voices].chorus_link = voices;
	voices++;
    }
    if(n > 0)
	ctl_mode_event(CTLE_MAXVOICES, 1, voices, 0);
}

static void voice_decrement(int n)
{
    int i, j, lowest;
    int32 lv, v;

    /* decrease voice */
    for(i = 0; i < n && voices > 0; i++)
    {
	voices--;
	if(voice[voices].status == VOICE_FREE)
	    continue;	/* found */

	for(j = 0; j < voices; j++)
	    if(voice[j].status == VOICE_FREE)
		break;
	if(j != voices)
	{
	    voice[j] = voice[voices];
	    continue;	/* found */
	}

	/* Look for the decaying note with the lowest volume */
	lv = 0x7FFFFFFF;
	lowest = -1;
	for(j = 0; j <= voices; j++)
	{
	    if(voice[j].status & ~(VOICE_ON | VOICE_DIE))
	    {
		v = voice[j].left_mix;
		if((voice[j].panned==PANNED_MYSTERY) &&
		   (voice[j].right_mix > v))
		    v = voice[j].right_mix;
		if(v < lv)
		{
		    lv = v;
		    lowest = j;
		}
	    }
	}

	if(lowest != -1)
	{
	    cut_notes++;
	    free_voice(lowest);
	    ctl_note_event(lowest);
	    voice[lowest] = voice[voices];
	}
	else
	    lost_notes++;
    }
    if(upper_voices > voices)
	upper_voices = voices;
    if(n > 0)
	ctl_mode_event(CTLE_MAXVOICES, 1, voices, 0);
}

/* EAW -- do not throw away good notes, stop decrementing */
static void voice_decrement_conservative(int n)
{
    int i, j, lowest, finalnv;
    int32 lv, v;

    /* decrease voice */
    finalnv = voices - n;
    for(i = 1; i <= n && voices > 0; i++)
    {
	if(voice[voices-1].status == VOICE_FREE) {
	    voices--;
	    continue;	/* found */
	}

	for(j = 0; j < finalnv; j++)
	    if(voice[j].status == VOICE_FREE)
		break;
	if(j != finalnv)
	{
	    voice[j] = voice[voices-1];
	    voices--;
	    continue;	/* found */
	}

	/* Look for the decaying note with the lowest volume */
	lv = 0x7FFFFFFF;
	lowest = -1;
	for(j = 0; j < voices; j++)
	{
	    if(voice[j].status & ~(VOICE_ON | VOICE_DIE) &&
	       !(voice[j].sample->note_to_use &&
	         ISDRUMCHANNEL(voice[j].channel)))
	    {
		v = voice[j].left_mix;
		if((voice[j].panned==PANNED_MYSTERY) &&
		   (voice[j].right_mix > v))
		    v = voice[j].right_mix;
		if(v < lv)
		{
		    lv = v;
		    lowest = j;
		}
	    }
	}

	if(lowest != -1)
	{
	    voices--;
	    cut_notes++;
	    free_voice(lowest);
	    ctl_note_event(lowest);
	    voice[lowest] = voice[voices];
	}
	else break;
    }
    if(upper_voices > voices)
	upper_voices = voices;
}

void restore_voices(int save_voices)
{
#ifdef REDUCE_VOICE_TIME_TUNING
    static int old_voices = -1;
    if(old_voices == -1 || save_voices)
	old_voices = voices;
    else if (voices < old_voices)
	voice_increment(old_voices - voices);
    else
	voice_decrement(voices - old_voices);
#endif /* REDUCE_VOICE_TIME_TUNING */
}
	

static int apply_controls(void)
{
    int rc, i, jump_flag = 0;
    int32 val, cur;
    FLOAT_T r;
    ChannelBitMask tmp_chbitmask;

    /* ASCII renditions of CD player pictograms indicate approximate effect */
    do
    {
	switch(rc=ctl->read(&val))
	{
	  case RC_STOP:
	  case RC_QUIT:		/* [] */
	  case RC_LOAD_FILE:
	  case RC_NEXT:		/* >>| */
	  case RC_REALLY_PREVIOUS: /* |<< */
	  case RC_TUNE_END:	/* skip */
	    aq_flush(1);
	    return rc;

	  case RC_CHANGE_VOLUME:
	    if (val>0 || amplification > -val)
		amplification += val;
	    else
		amplification=0;
	    if (amplification > MAX_AMPLIFICATION)
		amplification=MAX_AMPLIFICATION;
	    adjust_amplification();
	    for (i=0; i<upper_voices; i++)
		if (voice[i].status != VOICE_FREE)
		{
		    recompute_amp(i);
		    apply_envelope_to_amp(i);
		}
	    ctl_mode_event(CTLE_MASTER_VOLUME, 0, amplification, 0);
	    continue;

	  case RC_CHANGE_REV_EFFB:
	  case RC_CHANGE_REV_TIME:
	    reverb_rc_event(rc, val);
	    sync_restart(0);
	    continue;

	  case RC_PREVIOUS:	/* |<< */
	    aq_flush(1);
	    if (current_sample < 2*play_mode->rate)
		return RC_REALLY_PREVIOUS;
	    return RC_RESTART;

	  case RC_RESTART:	/* |<< */
	    if(play_pause_flag)
	    {
		midi_restart_time = 0;
		ctl_pause_event(1, 0);
		continue;
	    }
	    aq_flush(1);
	    skip_to(0);
	    ctl_updatetime(0);
	    jump_flag = 1;
		midi_restart_time = 0;
	    continue;

	  case RC_JUMP:
	    if(play_pause_flag)
	    {
		midi_restart_time = val;
		ctl_pause_event(1, val);
		continue;
	    }
	    aq_flush(1);
	    if (val >= sample_count)
		return RC_TUNE_END;
	    skip_to(val);
	    ctl_updatetime(val);
	    return rc;

	  case RC_FORWARD:	/* >> */
	    if(play_pause_flag)
	    {
		midi_restart_time += val;
		if(midi_restart_time > sample_count)
		    midi_restart_time = sample_count;
		ctl_pause_event(1, midi_restart_time);
		continue;
	    }
	    cur = current_trace_samples();
	    aq_flush(1);
	    if(cur == -1)
		cur = current_sample;
	    if(val + cur >= sample_count)
		return RC_TUNE_END;
	    skip_to(val + cur);
	    ctl_updatetime(val + cur);
	    return RC_JUMP;

	  case RC_BACK:		/* << */
	    if(play_pause_flag)
	    {
		midi_restart_time -= val;
		if(midi_restart_time < 0)
		    midi_restart_time = 0;
		ctl_pause_event(1, midi_restart_time);
		continue;
	    }
	    cur = current_trace_samples();
	    aq_flush(1);
	    if(cur == -1)
		cur = current_sample;
	    if(cur > val)
	    {
		skip_to(cur - val);
		ctl_updatetime(cur - val);
	    }
	    else
	    {
		skip_to(0);
		ctl_updatetime(0);
		midi_restart_time = 0;
	    }
	    return RC_JUMP;

	  case RC_TOGGLE_PAUSE:
	    if(play_pause_flag)
	    {
		play_pause_flag = 0;
		skip_to(midi_restart_time);
	    }
	    else
	    {
		midi_restart_time = current_trace_samples();
		if(midi_restart_time == -1)
		    midi_restart_time = current_sample;
		aq_flush(1);
		play_pause_flag = 1;
	    }
	    ctl_pause_event(play_pause_flag, midi_restart_time);
	    jump_flag = 1;
	    continue;

	  case RC_KEYUP:
	  case RC_KEYDOWN:
	    note_key_offset += val;
	    current_freq_table += val;
	    current_freq_table -= floor(current_freq_table / 12.0) * 12;
	    current_temper_freq_table += val;
	    current_temper_freq_table -=
	    		floor(current_temper_freq_table / 12.0) * 12;
	    if(sync_restart(1) != -1)
		jump_flag = 1;
	    ctl_mode_event(CTLE_KEY_OFFSET, 0, note_key_offset, 0);
	    continue;

	  case RC_SPEEDUP:
	    r = 1.0;
	    for(i = 0; i < val; i++)
		r *= SPEED_CHANGE_RATE;
	    sync_restart(0);
	    midi_time_ratio /= r;
	    current_sample = (int32)(current_sample / r + 0.5);
	    trace_offset(current_sample);
	    jump_flag = 1;
	    ctl_mode_event(CTLE_TIME_RATIO, 0, 100 / midi_time_ratio + 0.5, 0);
	    continue;

	  case RC_SPEEDDOWN:
	    r = 1.0;
	    for(i = 0; i < val; i++)
		r *= SPEED_CHANGE_RATE;
	    sync_restart(0);
	    midi_time_ratio *= r;
	    current_sample = (int32)(current_sample * r + 0.5);
	    trace_offset(current_sample);
	    jump_flag = 1;
	    ctl_mode_event(CTLE_TIME_RATIO, 0, 100 / midi_time_ratio + 0.5, 0);
	    continue;

	  case RC_VOICEINCR:
	    restore_voices(0);
	    voice_increment(val);
	    if(sync_restart(1) != -1)
		jump_flag = 1;
	    restore_voices(1);
	    continue;

	  case RC_VOICEDECR:
	    restore_voices(0);
	    if(sync_restart(1) != -1)
	    {
		voices -= val;
		if(voices < 0)
		    voices = 0;
		jump_flag = 1;
	    }
	    else
		voice_decrement(val);
	    restore_voices(1);
	    continue;

	  case RC_TOGGLE_DRUMCHAN:
	    midi_restart_time = current_trace_samples();
	    if(midi_restart_time == -1)
		midi_restart_time = current_sample;
	    SET_CHANNELMASK(drumchannel_mask, val);
	    SET_CHANNELMASK(current_file_info->drumchannel_mask, val);
	    if(IS_SET_CHANNELMASK(drumchannels, val))
	    {
		UNSET_CHANNELMASK(drumchannels, val);
		UNSET_CHANNELMASK(current_file_info->drumchannels, val);
	    }
	    else
	    {
		SET_CHANNELMASK(drumchannels, val);
		SET_CHANNELMASK(current_file_info->drumchannels, val);
	    }
	    aq_flush(1);
	    return RC_RELOAD;

	  case RC_TOGGLE_SNDSPEC:
#ifdef SUPPORT_SOUNDSPEC
	    if(view_soundspec_flag)
		close_soundspec();
	    else
		open_soundspec();
	    if(view_soundspec_flag || ctl_speana_flag)
	    {
		sync_restart(0);
		soundspec_update_wave(NULL, -1);
	    }
#endif /* SUPPORT_SOUNDSPEC */
	    continue;

	  case RC_TOGGLE_CTL_SPEANA:
#ifdef SUPPORT_SOUNDSPEC
	    ctl_speana_flag = !ctl_speana_flag;
	    if(view_soundspec_flag || ctl_speana_flag)
	    {
		sync_restart(0);
		soundspec_update_wave(NULL, -1);
	    }
#endif /* SUPPORT_SOUNDSPEC */
	    continue;

	  case RC_SYNC_RESTART:
	    sync_restart(val);
	    jump_flag = 1;
	    continue;

	  case RC_RELOAD:
	    midi_restart_time = current_trace_samples();
	    if(midi_restart_time == -1)
		midi_restart_time = current_sample;
	    aq_flush(1);
	    return RC_RELOAD;

	  case RC_CHANGE_RATE:
	    if(playmidi_change_rate(val, 1))
		return RC_NONE;
	    return RC_RELOAD;

	  case RC_OUTPUT_CHANGED:
	    playmidi_output_changed(0);
	    return RC_RELOAD;

	case RC_TOGGLE_MUTE:
		TOGGLE_CHANNELMASK(channel_mute, val);
		sync_restart(0);
		jump_flag = 1;
		ctl_mode_event(CTLE_MUTE, 0,
				val, (IS_SET_CHANNELMASK(channel_mute, val)) ? 1 : 0);
		continue;

	case RC_SOLO_PLAY:
		COPY_CHANNELMASK(tmp_chbitmask, channel_mute);
		FILL_CHANNELMASK(channel_mute);
		UNSET_CHANNELMASK(channel_mute, val);
		if (! COMPARE_CHANNELMASK(tmp_chbitmask, channel_mute)) {
			sync_restart(0);
			jump_flag = 1;
			for (i = 0; i < MAX_CHANNELS; i++)
				ctl_mode_event(CTLE_MUTE, 0, i, 1);
			ctl_mode_event(CTLE_MUTE, 0, val, 0);
		}
		continue;

	case RC_MUTE_CLEAR:
		COPY_CHANNELMASK(tmp_chbitmask, channel_mute);
		CLEAR_CHANNELMASK(channel_mute);
		if (! COMPARE_CHANNELMASK(tmp_chbitmask, channel_mute)) {
			sync_restart(0);
			jump_flag = 1;
			for (i = 0; i < MAX_CHANNELS; i++)
				ctl_mode_event(CTLE_MUTE, 0, i, 0);
		}
		continue;
	}
	if(intr)
	    return RC_QUIT;
	if(play_pause_flag)
	    usleep(300000);
    } while (rc != RC_NONE || play_pause_flag);
    return jump_flag ? RC_JUMP : RC_NONE;
}

static void mix_signal(int32 *dest, int32 *src, int32 count)
{
	int32 i;
	for (i = 0; i < count; i++) {
		dest[i] += src[i];
	}
}

inline static int is_insertion_effect_xg(int ch)
{
	int i;
	for (i = 0; i < XG_INSERTION_EFFECT_NUM; i++) {
		if (insertion_effect_xg[i].part == ch) {
			return 1;
		}
	}
	for (i = 0; i < XG_VARIATION_EFFECT_NUM; i++) {
		if (variation_effect_xg[i].connection == XG_CONN_INSERTION
			&& variation_effect_xg[i].part == ch) {
			return 1;
		}
	}
	return 0;
}

#ifdef USE_DSP_EFFECT
/* do_compute_data_midi() with DSP Effect */
static void do_compute_data_midi(int32 count)
{
	int i, j, uv, stereo, n, ch, note;
	int32 *vpblist[MAX_CHANNELS];
	int channel_effect, channel_reverb, channel_chorus, channel_delay, channel_eq;
	int32 cnt = count * 2, rev_max_delay_out;
	struct DrumPartEffect *de;
	
	stereo = ! (play_mode->encoding & PE_MONO);
	n = count * ((stereo) ? 8 : 4); /* in bytes */

	memset(buffer_pointer, 0, n);
	memset(insertion_effect_buffer, 0, n);

	if (opt_reverb_control == 3) {
		rev_max_delay_out = 0x7fffffff;	/* disable */
	} else {
		rev_max_delay_out = REVERB_MAX_DELAY_OUT;
	}

	/* are effects valid? / don't supported in mono */
	channel_reverb = (stereo && (opt_reverb_control == 1
			|| opt_reverb_control == 3
			|| (opt_reverb_control < 0 && opt_reverb_control & 0x80)));
	channel_chorus = (stereo && opt_chorus_control && !opt_surround_chorus);
	channel_delay = (stereo && opt_delay_control > 0);

	/* is EQ valid? */
	channel_eq = opt_eq_control && (eq_status_gs.low_gain != 0x40 || eq_status_gs.high_gain != 0x40 ||
		play_system_mode == XG_SYSTEM_MODE);

	channel_effect = (stereo && (channel_reverb || channel_chorus
			|| channel_delay || channel_eq || opt_insertion_effect));

	uv = upper_voices;
	for(i = 0; i < uv; i++) {
		if(voice[i].status != VOICE_FREE) {
			channel[voice[i].channel].lasttime = current_sample + count;
		}
	}

	/* appropriate buffers for channels */
	if(channel_effect) {
		int buf_index = 0;
		
		if(reverb_buffer == NULL) {	/* allocating buffer for channel effect */
			reverb_buffer = (char *)safe_malloc(MAX_CHANNELS * AUDIO_BUFFER_SIZE * 8);
		}

		for(i = 0; i < MAX_CHANNELS; i++) {
			if(opt_insertion_effect && channel[i].insertion_effect) {
				vpblist[i] = insertion_effect_buffer;
			} else if(channel[i].eq_gs || (get_reverb_level(i) != DEFAULT_REVERB_SEND_LEVEL
					&& current_sample - channel[i].lasttime < rev_max_delay_out)
					|| channel[i].chorus_level > 0 || channel[i].delay_level > 0
					|| channel[i].eq_xg.valid
					|| channel[i].dry_level != 127
					|| (opt_drum_effect && ISDRUMCHANNEL(i))
					|| is_insertion_effect_xg(i)) {
				vpblist[i] = (int32*)(reverb_buffer + buf_index);
				buf_index += n;
			} else {
				vpblist[i] = buffer_pointer;
			}
			/* clear buffers of drum-part effect */
			if (opt_drum_effect && ISDRUMCHANNEL(i)) {
				for (j = 0; j < channel[i].drum_effect_num; j++) {
					if (channel[i].drum_effect[j].buf != NULL) {
						memset(channel[i].drum_effect[j].buf, 0, n);
					}
				}
			}
		}

		if(buf_index) {memset(reverb_buffer, 0, buf_index);}
	}

	for (i = 0; i < uv; i++) {
		if (voice[i].status != VOICE_FREE) {
			int32 *vpb;
			int8 flag;
			
			if (channel_effect) {
				flag = 0;
				ch = voice[i].channel;
				if (opt_drum_effect && ISDRUMCHANNEL(ch)) {
					make_drum_effect(ch);
					note = voice[i].note;
					for (j = 0; j < channel[ch].drum_effect_num; j++) {
						if (channel[ch].drum_effect[j].note == note) {
							vpb = channel[ch].drum_effect[j].buf;
							flag = 1;
						}
					}
					if (flag == 0) {vpb = vpblist[ch];}
				} else {
					vpb = vpblist[ch];
				}
			} else {
				vpb = buffer_pointer;
			}

			if(!IS_SET_CHANNELMASK(channel_mute, voice[i].channel)) {
				mix_voice(vpb, i, count);
			} else {
				free_voice(i);
				ctl_note_event(i);
			}

			if(voice[i].timeout == 1 && voice[i].timeout < current_sample) {
				free_voice(i);
				ctl_note_event(i);
			}
		}
	}

	while(uv > 0 && voice[uv - 1].status == VOICE_FREE)	{uv--;}
	upper_voices = uv;

	if(play_system_mode == XG_SYSTEM_MODE && channel_effect) {	/* XG */
		if (opt_insertion_effect) { 	/* insertion effect */
			for (i = 0; i < XG_INSERTION_EFFECT_NUM; i++) {
				if (insertion_effect_xg[i].part <= MAX_CHANNELS) {
					do_insertion_effect_xg(vpblist[insertion_effect_xg[i].part], cnt, &insertion_effect_xg[i]);
				}
			}
			for (i = 0; i < XG_VARIATION_EFFECT_NUM; i++) {
				if (variation_effect_xg[i].part <= MAX_CHANNELS) {
					do_insertion_effect_xg(vpblist[variation_effect_xg[i].part], cnt, &variation_effect_xg[i]);
				}
			}
		}
		for(i = 0; i < MAX_CHANNELS; i++) {	/* system effects */
			int32 *p;
			p = vpblist[i];
			if(p != buffer_pointer) {
				if (opt_drum_effect && ISDRUMCHANNEL(i)) {
					for (j = 0; j < channel[i].drum_effect_num; j++) {
						de = &(channel[i].drum_effect[j]);
						if (de->reverb_send > 0) {
							set_ch_reverb(de->buf, cnt, de->reverb_send);
						}
						if (de->chorus_send > 0) {
							set_ch_chorus(de->buf, cnt, de->chorus_send);
						}
						if (de->delay_send > 0) {
							set_ch_delay(de->buf, cnt, de->delay_send);
						}
						mix_signal(p, de->buf, cnt);
					}
				} else {
					if(channel_eq && channel[i].eq_xg.valid) {
						do_ch_eq_xg(p, cnt, &(channel[i].eq_xg));
					}
					if(channel_chorus && channel[i].chorus_level > 0) {
						set_ch_chorus(p, cnt, channel[i].chorus_level);
					}
					if(channel_delay && channel[i].delay_level > 0) {
						set_ch_delay(p, cnt, channel[i].delay_level);
					}
					if(channel_reverb && channel[i].reverb_level > 0
						&& current_sample - channel[i].lasttime < rev_max_delay_out) {
						set_ch_reverb(p, cnt, channel[i].reverb_level);
					}
				}
				if(channel[i].dry_level == 127) {
					set_dry_signal(p, cnt);
				} else {
					set_dry_signal_xg(p, cnt, channel[i].dry_level);
				}
			}
		}
		
		if(channel_reverb) {
			set_ch_reverb(buffer_pointer, cnt, DEFAULT_REVERB_SEND_LEVEL);
		}
		set_dry_signal(buffer_pointer, cnt);

		/* mixing signal and applying system effects */ 
		mix_dry_signal(buffer_pointer, cnt);
		if(channel_delay) {do_variation_effect1_xg(buffer_pointer, cnt);}
		if(channel_chorus) {do_ch_chorus_xg(buffer_pointer, cnt);}
		if(channel_reverb) {do_ch_reverb(buffer_pointer, cnt);}
		if(multi_eq_xg.valid) {do_multi_eq_xg(buffer_pointer, cnt);}
	} else if(channel_effect) {	/* GM & GS */
		if(opt_insertion_effect) { 	/* insertion effect */
			/* applying insertion effect */
			do_insertion_effect_gs(insertion_effect_buffer, cnt);
			/* sending insertion effect voice to channel effect */
			set_ch_chorus(insertion_effect_buffer, cnt, insertion_effect_gs.send_chorus);
			set_ch_delay(insertion_effect_buffer, cnt, insertion_effect_gs.send_delay);
			set_ch_reverb(insertion_effect_buffer, cnt,	insertion_effect_gs.send_reverb);
			if(insertion_effect_gs.send_eq_switch && channel_eq) {
				set_ch_eq_gs(insertion_effect_buffer, cnt);
			} else {
				set_dry_signal(insertion_effect_buffer, cnt);
			}
		}

		for(i = 0; i < MAX_CHANNELS; i++) {	/* system effects */
			int32 *p;	
			p = vpblist[i];
			if(p != buffer_pointer && p != insertion_effect_buffer) {
				if (opt_drum_effect && ISDRUMCHANNEL(i)) {
					for (j = 0; j < channel[i].drum_effect_num; j++) {
						de = &(channel[i].drum_effect[j]);
						if (de->reverb_send > 0) {
							set_ch_reverb(de->buf, cnt, de->reverb_send);
						}
						if (de->chorus_send > 0) {
							set_ch_chorus(de->buf, cnt, de->chorus_send);
						}
						if (de->delay_send > 0) {
							set_ch_delay(de->buf, cnt, de->delay_send);
						}
						mix_signal(p, de->buf, cnt);
					}
				} else {
					if(channel_chorus && channel[i].chorus_level > 0) {
						set_ch_chorus(p, cnt, channel[i].chorus_level);
					}
					if(channel_delay && channel[i].delay_level > 0) {
						set_ch_delay(p, cnt, channel[i].delay_level);
					}
					if(channel_reverb && channel[i].reverb_level > 0
						&& current_sample - channel[i].lasttime < rev_max_delay_out) {
						set_ch_reverb(p, cnt, channel[i].reverb_level);
					}
				}
				if(channel_eq && channel[i].eq_gs) {
					set_ch_eq_gs(p, cnt);
				} else {
					set_dry_signal(p, cnt);
				}
			}
		}
		
		if(channel_reverb) {
			set_ch_reverb(buffer_pointer, cnt, DEFAULT_REVERB_SEND_LEVEL);
		}
		set_dry_signal(buffer_pointer, cnt);

		/* mixing signal and applying system effects */ 
		mix_dry_signal(buffer_pointer, cnt);
		if(channel_eq) {do_ch_eq_gs(buffer_pointer, cnt);}
		if(channel_chorus) {do_ch_chorus(buffer_pointer, cnt);}
		if(channel_delay) {do_ch_delay(buffer_pointer, cnt);}
		if(channel_reverb) {do_ch_reverb(buffer_pointer, cnt);}
	}

	current_sample += count;
}

#else
/* do_compute_data_midi() without DSP Effect */
static void do_compute_data_midi(int32 count)
{
	int i, j, uv, stereo, n, ch, note;
	int32 *vpblist[MAX_CHANNELS];
	int vc[MAX_CHANNELS];
	int channel_reverb;
	int channel_effect;
	int32 cnt = count * 2;
	
	stereo = ! (play_mode->encoding & PE_MONO);
	n = count * ((stereo) ? 8 : 4); /* in bytes */
	/* don't supported in mono */
	channel_reverb = (stereo && (opt_reverb_control == 1
			|| opt_reverb_control == 3
			|| opt_reverb_control < 0 && opt_reverb_control & 0x80));
	memset(buffer_pointer, 0, n);

	channel_effect = (stereo && (opt_reverb_control || opt_chorus_control
			|| opt_delay_control || opt_eq_control || opt_insertion_effect));
	uv = upper_voices;
	for (i = 0; i < uv; i++)
		if (voice[i].status != VOICE_FREE)
			channel[voice[i].channel].lasttime = current_sample + count;

	if (channel_reverb) {
		int chbufidx;
		
		if (! make_rvid_flag) {
			make_rvid();
			make_rvid_flag = 1;
		}
		chbufidx = 0;
		for (i = 0; i < MAX_CHANNELS; i++) {
			vc[i] = 0;
			if (channel[i].reverb_id != -1
					&& current_sample - channel[i].lasttime
					< REVERB_MAX_DELAY_OUT) {
				if (reverb_buffer == NULL)
					reverb_buffer = (char *) safe_malloc(MAX_CHANNELS
							* AUDIO_BUFFER_SIZE * 8);
				if (channel[i].reverb_id != i)
					vpblist[i] = vpblist[channel[i].reverb_id];
				else {
					vpblist[i] = (int32 *) (reverb_buffer + chbufidx);
					chbufidx += n;
				}
			} else
				vpblist[i] = buffer_pointer;
		}
		if (chbufidx)
			memset(reverb_buffer, 0, chbufidx);
	}
	for (i = 0; i < uv; i++)
		if (voice[i].status != VOICE_FREE) {
			int32 *vpb;
			
			if (channel_reverb) {
				int ch = voice[i].channel;
				
				vpb = vpblist[ch];
				vc[ch] = 1;
			} else
				vpb = buffer_pointer;
			if (! IS_SET_CHANNELMASK(channel_mute, voice[i].channel))
				mix_voice(vpb, i, count);
			else {
				free_voice(i);
				ctl_note_event(i);
			}
			if (voice[i].timeout == 1 && voice[i].timeout < current_sample) {
				free_voice(i);
				ctl_note_event(i);
			}
		}

	while (uv > 0 && voice[uv - 1].status == VOICE_FREE)
		uv--;
	upper_voices = uv;

	if (channel_reverb) {
		int k;
		
		k = count * 2; /* calclated buffer length in int32 */
		for (i = 0; i < MAX_CHANNELS; i++) {
			int32 *p;
			
			p = vpblist[i];
			if (p != buffer_pointer && channel[i].reverb_id == i)
				set_ch_reverb(p, k, channel[i].reverb_level);
		}
		set_ch_reverb(buffer_pointer, k, DEFAULT_REVERB_SEND_LEVEL);
		do_ch_reverb(buffer_pointer, k);
	}
	current_sample += count;
}
#endif

static void do_compute_data_wav(int32 count)
{
	int i, stereo, samples, req_size, act_samples, v;

	stereo = !(play_mode->encoding & PE_MONO);
	samples = (stereo ? (count * 2) : count);
	req_size = samples * 2; /* assume 16bit */

	act_samples = tf_read(wav_buffer, 1, req_size, current_file_info->pcm_tf) / 2;
	for(i = 0; i < act_samples; i++) {
		v = (uint16)LE_SHORT(wav_buffer[i]);
		buffer_pointer[i] = (int32)((v << 16) | (v ^ 0x8000)) / 4; /* 4 : level down */
	}
	for(; i < samples; i++)
		buffer_pointer[i] = 0;

	current_sample += count;
}

static void do_compute_data_aiff(int32 count)
{
	int i, stereo, samples, req_size, act_samples, v;

	stereo = !(play_mode->encoding & PE_MONO);
	samples = (stereo ? (count * 2) : count);
	req_size = samples * 2; /* assume 16bit */

	act_samples = tf_read(wav_buffer, 1, req_size, current_file_info->pcm_tf) / 2;
	for(i = 0; i < act_samples; i++) {
		v = (uint16)BE_SHORT(wav_buffer[i]);
		buffer_pointer[i] = (int32)((v << 16) | (v ^ 0x8000)) / 4; /* 4 : level down */
	}
	for(; i < samples; i++)
		buffer_pointer[i] = 0;

	current_sample += count;
}


static void do_compute_data(int32 count)
{
    switch(current_file_info->pcm_mode)
    {
      case PCM_MODE_NON:
    	do_compute_data_midi(count);
      	break;
      case PCM_MODE_WAV:
    	do_compute_data_wav(count);
        break;
      case PCM_MODE_AIFF:
    	do_compute_data_aiff(count);
        break;
      case PCM_MODE_AU:
        break;
      case PCM_MODE_MP3:
        break;
    }    
}

static int check_midi_play_end(MidiEvent *e, int len)
{
    int i, type;

    for(i = 0; i < len; i++)
    {
	type = e[i].type;
	if(type == ME_NOTEON || type == ME_LAST || type == ME_WRD || type == ME_SHERRY)
	    return 0;
	if(type == ME_EOT)
	    return i + 1;
    }
    return 0;
}

static int compute_data(int32 count);
static int midi_play_end(void)
{
    int i, rc = RC_TUNE_END;

    check_eot_flag = 0;

    if(opt_realtime_playing && current_sample == 0)
    {
	reset_voices();
	return RC_TUNE_END;
    }

    if(upper_voices > 0)
    {
	int fadeout_cnt;

	rc = compute_data(play_mode->rate);
	if(RC_IS_SKIP_FILE(rc))
	    goto midi_end;

	for(i = 0; i < upper_voices; i++)
	    if(voice[i].status & (VOICE_ON | VOICE_SUSTAINED))
		finish_note(i);
	if(opt_realtime_playing)
	    fadeout_cnt = 3;
	else
	    fadeout_cnt = 6;
	for(i = 0; i < fadeout_cnt && upper_voices > 0; i++)
	{
	    rc = compute_data(play_mode->rate / 2);
	    if(RC_IS_SKIP_FILE(rc))
		goto midi_end;
	}

	/* kill voices */
	kill_all_voices();
	rc = compute_data(MAX_DIE_TIME);
	if(RC_IS_SKIP_FILE(rc))
	    goto midi_end;
	upper_voices = 0;
    }

    /* clear reverb echo sound */
    init_reverb();
    for(i = 0; i < MAX_CHANNELS; i++)
    {
	channel[i].reverb_level = -1;
	channel[i].reverb_id = -1;
	make_rvid_flag = 1;
    }

    /* output null sound */
    if(opt_realtime_playing)
	rc = compute_data((int32)(play_mode->rate * PLAY_INTERLEAVE_SEC/2));
    else
	rc = compute_data((int32)(play_mode->rate * PLAY_INTERLEAVE_SEC));
    if(RC_IS_SKIP_FILE(rc))
	goto midi_end;

    compute_data(0); /* flush buffer to device */

    if(ctl->trace_playing)
    {
	rc = aq_flush(0); /* Wait until play out */
	if(RC_IS_SKIP_FILE(rc))
	    goto midi_end;
    }
    else
    {
	trace_flush();
	rc = aq_soft_flush();
	if(RC_IS_SKIP_FILE(rc))
	    goto midi_end;
    }

  midi_end:
    if(RC_IS_SKIP_FILE(rc))
	aq_flush(1);

    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Playing time: ~%d seconds",
	      current_sample/play_mode->rate+2);
    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Notes cut: %d",
	      cut_notes);
    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "Notes lost totally: %d",
	      lost_notes);
    if(RC_IS_SKIP_FILE(rc))
	return rc;
    return RC_TUNE_END;
}

/* count=0 means flush remaining buffered data to output device, then
   flush the device itself */
static int compute_data(int32 count)
{
  int rc;

  if (!count)
    {
      if (buffered_count)
      {
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		    "output data (%d)", buffered_count);

#ifdef SUPPORT_SOUNDSPEC
	  soundspec_update_wave(common_buffer, buffered_count);
#endif /* SUPPORT_SOUNDSPEC */

	  if(aq_add(common_buffer, buffered_count) == -1)
	      return RC_ERROR;
      }
      buffer_pointer=common_buffer;
      buffered_count=0;
      return RC_NONE;
    }

  while ((count+buffered_count) >= audio_buffer_size)
    {
      int i;

      if((rc = apply_controls()) != RC_NONE)
	  return rc;

      do_compute_data(audio_buffer_size-buffered_count);
      count -= audio_buffer_size-buffered_count;
      ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		"output data (%d)", audio_buffer_size);

#ifdef SUPPORT_SOUNDSPEC
      soundspec_update_wave(common_buffer, audio_buffer_size);
#endif /* SUPPORT_SOUNDSPEC */

      /* fall back to linear interpolation when queue < 100% */
      if (! opt_realtime_playing && (play_mode->flag & PF_CAN_TRACE)) {
	  if (!aq_fill_buffer_flag &&
	      100 * ((double)(aq_filled() + aq_soft_filled()) /
		     aq_get_dev_queuesize()) < 99)
	      reduce_quality_flag = 1;
	  else
	      reduce_quality_flag = no_4point_interpolation;
      }

#ifdef REDUCE_VOICE_TIME_TUNING
      /* Auto voice reduce implementation by Masanao Izumo */
      if(reduce_voice_threshold &&
	 (play_mode->flag & PF_CAN_TRACE) &&
	 !aq_fill_buffer_flag &&
	 aq_get_dev_queuesize() > 0)
      {
	  /* Reduce voices if there is not enough audio device buffer */

          int nv, filled, filled_limit, rate, rate_limit;
          static int last_filled;

	  filled = aq_filled();

	  rate_limit = 75;
	  if(reduce_voice_threshold >= 0)
	  {
	      filled_limit = play_mode->rate * reduce_voice_threshold / 1000
		  + 1; /* +1 disable zero */
	  }
	  else /* Use default threshold */
	  {
	      int32 maxfill;
	      maxfill = aq_get_dev_queuesize();
	      filled_limit = REDUCE_VOICE_TIME_TUNING;
	      if(filled_limit > maxfill / 5) /* too small audio buffer */
	      {
		  rate_limit -= 100 * audio_buffer_size / maxfill / 5;
		  filled_limit = 1;
	      }
	  }

	  /* Calculate rate as it is displayed in ncurs_c.c */
	  /* The old method of calculating rate resulted in very low values
	     when using the new high order interplation methods on "slow"
	     CPUs when the queue was being drained WAY too quickly.  This
	     caused premature voice reduction under Linux, even if the queue
	     was over 2000%, leading to major voice lossage. */
	  rate = (int)(((double)(aq_filled() + aq_soft_filled()) /
                  	aq_get_dev_queuesize()) * 100 + 0.5);

          for(i = nv = 0; i < upper_voices; i++)
	      if(voice[i].status != VOICE_FREE)
	          nv++;

	  if(! opt_realtime_playing)
	  {
	      /* calculate ok_nv, the "optimum" max polyphony */
	      if (auto_reduce_polyphony && rate < 85) {
		/* average in current nv */
	        if ((rate == old_rate && nv > min_bad_nv) ||
	            (rate >= old_rate && rate < 20)) {
	        	ok_nv_total += nv;
	        	ok_nv_counts++;
	        }
	        /* increase polyphony when it is too low */
	        else if (nv == voices &&
	                 (rate > old_rate && filled > last_filled)) {
	          		ok_nv_total += nv + 1;
	          		ok_nv_counts++;
	        }
	        /* reduce polyphony when loosing buffer */
	        else if (rate < 75 &&
	        	 (rate < old_rate && filled < last_filled)) {
	        	ok_nv_total += min_bad_nv;
	    		ok_nv_counts++;
	        }
	        else goto NO_RESCALE_NV;

		/* rescale ok_nv stuff every 1 seconds */
		if (current_sample >= ok_nv_sample && ok_nv_counts > 1) {
			ok_nv_total >>= 1;
			ok_nv_counts >>= 1;
			ok_nv_sample = current_sample + (play_mode->rate);
		}

		NO_RESCALE_NV:;
	      }
	  }

	  /* EAW -- if buffer is < 75%, start reducing some voices to
	     try to let it recover.  This really helps a lot, preserves
	     decent sound, and decreases the frequency of lost ON notes */
	  if ((! opt_realtime_playing && rate < rate_limit)
	      || filled < filled_limit)
	  {
	      if(filled <= last_filled)
	      {
	          int v, kill_nv, temp_nv;

		  /* set bounds on "good" and "bad" nv */
		  if (! opt_realtime_playing && rate > 20 &&
		      nv < min_bad_nv) {
		  	min_bad_nv = nv;
	                if (max_good_nv < min_bad_nv)
	                	max_good_nv = min_bad_nv;
	          }

		  /* EAW -- count number of !ON voices */
		  /* treat chorus notes as !ON */
		  for(i = kill_nv = 0; i < upper_voices; i++) {
		      if(voice[i].status & VOICE_FREE ||
		         voice[i].cache != NULL)
		      		continue;
		      
		      if((voice[i].status & ~(VOICE_ON|VOICE_SUSTAINED) &&
			  !(voice[i].status & ~(VOICE_DIE) &&
			    voice[i].sample->note_to_use)))
				kill_nv++;
		  }

		  /* EAW -- buffer is dangerously low, drasticly reduce
		     voices to a hopefully "safe" amount */
		  if (filled < filled_limit &&
		      (opt_realtime_playing || rate < 10)) {
		      FLOAT_T n;

		      /* calculate the drastic voice reduction */
		      if(nv > kill_nv) /* Avoid division by zero */
		      {
			  n = (FLOAT_T) nv / (nv - kill_nv);
			  temp_nv = (int)(nv - nv / (n + 1));

			  /* reduce by the larger of the estimates */
			  if (kill_nv < temp_nv && temp_nv < nv)
			      kill_nv = temp_nv;
		      }
		      else kill_nv = nv - 1; /* do not kill all the voices */
		  }
		  else {
		      /* the buffer is still high enough that we can throw
		         fewer voices away; keep the ON voices, use the
		         minimum "bad" nv as a floor on voice reductions */
		      temp_nv = nv - min_bad_nv;
		      if (kill_nv > temp_nv)
		          kill_nv = temp_nv;
		  }

		  for(i = 0; i < kill_nv; i++)
		      v = reduce_voice();

		  /* lower max # of allowed voices to let the buffer recover */
		  if (auto_reduce_polyphony) {
		  	temp_nv = nv - kill_nv;
		  	ok_nv = ok_nv_total / ok_nv_counts;

		  	/* decrease it to current nv left */
		  	if (voices > temp_nv && temp_nv > ok_nv)
			    voice_decrement_conservative(voices - temp_nv);
			/* decrease it to ok_nv */
		  	else if (voices > ok_nv && temp_nv <= ok_nv)
			    voice_decrement_conservative(voices - ok_nv);
		  	/* increase the polyphony */
		  	else if (voices < ok_nv)
			    voice_increment(ok_nv - voices);
		  }

		  while(upper_voices > 0 &&
			voice[upper_voices - 1].status == VOICE_FREE)
		      upper_voices--;
	      }
	      last_filled = filled;
	  }
	  else {
	      if (! opt_realtime_playing && rate >= rate_limit &&
	          filled > last_filled) {

		    /* set bounds on "good" and "bad" nv */
		    if (rate > 85 && nv > max_good_nv) {
		  	max_good_nv = nv;
		  	if (min_bad_nv > max_good_nv)
		  	    min_bad_nv = max_good_nv;
		    }

		    if (auto_reduce_polyphony) {
		    	/* reset ok_nv stuff when out of danger */
		    	ok_nv_total = max_good_nv * ok_nv_counts;
			if (ok_nv_counts > 1) {
			    ok_nv_total >>= 1;
			    ok_nv_counts >>= 1;
			}

		    	/* restore max # of allowed voices to normal */
			restore_voices(0);
		    }
	      }

	      last_filled = filled_limit;
          }
          old_rate = rate;
      }
#endif

      if(aq_add(common_buffer, audio_buffer_size) == -1)
	  return RC_ERROR;

      buffer_pointer=common_buffer;
      buffered_count=0;
      if(current_event->type != ME_EOT)
	  ctl_timestamp();

      /* check break signals */
      VOLATILE_TOUCH(intr);
      if(intr)
	  return RC_QUIT;

      if(upper_voices == 0 && check_eot_flag &&
	 (i = check_midi_play_end(current_event, EOT_PRESEARCH_LEN)) > 0)
      {
	  if(i > 1)
	      ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
			"Last %d MIDI events are ignored", i - 1);
	  return midi_play_end();
      }
    }
  if (count>0)
    {
      do_compute_data(count);
      buffered_count += count;
      buffer_pointer += (play_mode->encoding & PE_MONO) ? count : count*2;
    }
  return RC_NONE;
}

static void update_modulation_wheel(int ch)
{
    int i, uv = upper_voices;
	channel[ch].pitchfactor = 0;
    for(i = 0; i < uv; i++)
	if(voice[i].status != VOICE_FREE && voice[i].channel == ch)
	{
	    /* Set/Reset mod-wheel */
		voice[i].vibrato_control_counter = voice[i].vibrato_phase = 0;
	    recompute_amp(i);
		apply_envelope_to_amp(i);
	    recompute_freq(i);
		recompute_voice_filter(i);
	}
}

static void drop_portamento(int ch)
{
    int i, uv = upper_voices;

    channel[ch].porta_control_ratio = 0;
    for(i = 0; i < uv; i++)
	if(voice[i].status != VOICE_FREE &&
	   voice[i].channel == ch &&
	   voice[i].porta_control_ratio)
	{
	    voice[i].porta_control_ratio = 0;
	    recompute_freq(i);
	}
    channel[ch].last_note_fine = -1;
}

static void update_portamento_controls(int ch)
{
    if(!channel[ch].portamento ||
       (channel[ch].portamento_time_msb | channel[ch].portamento_time_lsb)
       == 0)
	drop_portamento(ch);
    else
    {
	double mt, dc;
	int d;

	mt = midi_time_table[channel[ch].portamento_time_msb & 0x7F] *
	    midi_time_table2[channel[ch].portamento_time_lsb & 0x7F] *
		PORTAMENTO_TIME_TUNING;
	dc = play_mode->rate * mt;
	d = (int)(1.0 / (mt * PORTAMENTO_CONTROL_RATIO));
	d++;
	channel[ch].porta_control_ratio = (int)(d * dc + 0.5);
	channel[ch].porta_dpb = d;
    }
}

static void update_portamento_time(int ch)
{
    int i, uv = upper_voices;
    int dpb;
    int32 ratio;

    update_portamento_controls(ch);
    dpb = channel[ch].porta_dpb;
    ratio = channel[ch].porta_control_ratio;

    for(i = 0; i < uv; i++)
    {
	if(voice[i].status != VOICE_FREE &&
	   voice[i].channel == ch &&
	   voice[i].porta_control_ratio)
	{
	    voice[i].porta_control_ratio = ratio;
	    voice[i].porta_dpb = dpb;
	    recompute_freq(i);
	}
    }
}

static void update_legato_controls(int ch)
{
	double mt, dc;
	int d;

	mt = 0.06250 * PORTAMENTO_TIME_TUNING * 0.3;
	dc = play_mode->rate * mt;
	d = (int)(1.0 / (mt * PORTAMENTO_CONTROL_RATIO));
	d++;
	channel[ch].porta_control_ratio = (int)(d * dc + 0.5);
	channel[ch].porta_dpb = d;
}

int play_event(MidiEvent *ev)
{
    int32 i, j, cet;
    int k, l, ch, orig_ch, port_ch, offset, layered;

    if(play_mode->flag & PF_MIDI_EVENT)
	return play_mode->acntl(PM_REQ_MIDI, ev);
    if(!(play_mode->flag & PF_PCM_STREAM))
	return RC_NONE;

    current_event = ev;
    cet = MIDI_EVENT_TIME(ev);

    if(ctl->verbosity >= VERB_DEBUG_SILLY)
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "Midi Event %d: %s %d %d %d", cet,
		  event_name(ev->type), ev->channel, ev->a, ev->b);
    if(cet > current_sample)
    {
	int rc;


    if(midi_streaming!=0){
    	if ( (cet - current_sample) * 1000 / play_mode->rate > stream_max_compute ) {
			kill_all_voices();
			/* reset_voices(); */
/* 			ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY, "play_event: discard %d samples", cet - current_sample); */
			current_sample = cet;
		}
    }

	rc = compute_data(cet - current_sample);
	ctl_mode_event(CTLE_REFRESH, 0, 0, 0);
    if(rc == RC_JUMP)
	{
		ctl_timestamp();
		return RC_NONE;
	}
	if(rc != RC_NONE)
	    return rc;
	}

#ifndef SUPPRESS_CHANNEL_LAYER
	orig_ch = ev->channel;
	layered = ! IS_SYSEX_EVENT_TYPE(ev);
	for (k = 0; k < MAX_CHANNELS; k += 16) {
		port_ch = (orig_ch + k) % MAX_CHANNELS;
		offset = port_ch & ~0xf;
		for (l = offset; l < offset + 16; l++) {
			if (! layered && (k || l != offset))
				continue;
			if (layered) {
				if (! IS_SET_CHANNELMASK(channel[l].channel_layer, port_ch)
						|| channel[l].port_select != (orig_ch >> 4))
					continue;
				ev->channel = l;
			}
#endif
	ch = ev->channel;

    switch(ev->type)
    {
	/* MIDI Events */
      case ME_NOTEOFF:
	note_off(ev);
	break;

      case ME_NOTEON:
	note_on(ev);
	break;

      case ME_KEYPRESSURE:
	adjust_pressure(ev);
	break;

      case ME_PROGRAM:
	midi_program_change(ch, ev->a);
	ctl_prog_event(ch);
	break;

      case ME_CHANNEL_PRESSURE:
	adjust_channel_pressure(ev);
	break;

      case ME_PITCHWHEEL:
	channel[ch].pitchbend = ev->a + ev->b * 128;
	channel[ch].pitchfactor = 0;
	/* Adjust pitch for notes already playing */
	adjust_pitch(ch);
	ctl_mode_event(CTLE_PITCH_BEND, 1, ch, channel[ch].pitchbend);
	break;

	/* Controls */
      case ME_TONE_BANK_MSB:
	channel[ch].bank_msb = ev->a;
	break;

      case ME_TONE_BANK_LSB:
	channel[ch].bank_lsb = ev->a;
	break;

      case ME_MODULATION_WHEEL:
	channel[ch].mod.val = ev->a;
	update_modulation_wheel(ch);
	ctl_mode_event(CTLE_MOD_WHEEL, 1, ch, channel[ch].mod.val);
	break;

      case ME_MAINVOLUME:
	channel[ch].volume = ev->a;
	adjust_volume(ch);
	ctl_mode_event(CTLE_VOLUME, 1, ch, ev->a);
	break;

      case ME_PAN:
	channel[ch].panning = ev->a;
	channel[ch].pan_random = 0;
	if(adjust_panning_immediately && !channel[ch].pan_random)
	    adjust_panning(ch);
	ctl_mode_event(CTLE_PANNING, 1, ch, ev->a);
	break;

      case ME_EXPRESSION:
	channel[ch].expression = ev->a;
	adjust_volume(ch);
	ctl_mode_event(CTLE_EXPRESSION, 1, ch, ev->a);
	break;

      case ME_SUSTAIN:
    if (channel[ch].sustain == 0 && ev->a >= 64) {
		update_redamper_controls(ch);
	}
	channel[ch].sustain = ev->a;
	if (channel[ch].damper_mode == 0) {	/* half-damper is not allowed. */
		if (channel[ch].sustain >= 64) {channel[ch].sustain = 127;}
		else {channel[ch].sustain = 0;}
	}
	if(channel[ch].sustain == 0 && channel[ch].sostenuto == 0)
	    drop_sustain(ch);
	ctl_mode_event(CTLE_SUSTAIN, 1, ch, channel[ch].sustain);
	break;

      case ME_SOSTENUTO:
	channel[ch].sostenuto = (ev->a >= 64);
	if(channel[ch].sustain == 0 && channel[ch].sostenuto == 0)
	    drop_sustain(ch);
	else {update_sostenuto_controls(ch);}
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "Sostenuto %d", channel[ch].sostenuto);
	break;

      case ME_LEGATO_FOOTSWITCH:
    channel[ch].legato = (ev->a >= 64);
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Legato Footswitch (CH:%d VAL:%d)", ch, channel[ch].legato);
	break;

      case ME_HOLD2:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Hold2 - this function is not supported.");
	break;

      case ME_BREATH:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Breath - this function is not supported.");
	break;

      case ME_FOOT:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Foot - this function is not supported.");
	break;

      case ME_BALANCE:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Balance - this function is not supported.");
	break;

      case ME_PORTAMENTO_TIME_MSB:
	channel[ch].portamento_time_msb = ev->a;
	update_portamento_time(ch);
	break;

      case ME_PORTAMENTO_TIME_LSB:
	channel[ch].portamento_time_lsb = ev->a;
	update_portamento_time(ch);
	break;

      case ME_PORTAMENTO:
	channel[ch].portamento = (ev->a >= 64);
	if(!channel[ch].portamento)
	    drop_portamento(ch);
	break;

	  case ME_SOFT_PEDAL:
		  if(opt_lpf_def) {
			  channel[ch].soft_pedal = ev->a;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Soft Pedal (CH:%d VAL:%d)",ch,channel[ch].soft_pedal);
		  }
		  break;

	  case ME_HARMONIC_CONTENT:
		  if(opt_lpf_def) {
			  channel[ch].param_resonance = ev->a - 64;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Harmonic Content (CH:%d VAL:%d)",ch,channel[ch].param_resonance);
		  }
		  break;

	  case ME_BRIGHTNESS:
		  if(opt_lpf_def) {
			  channel[ch].param_cutoff_freq = ev->a - 64;
			  ctl->cmsg(CMSG_INFO,VERB_NOISY,"Brightness (CH:%d VAL:%d)",ch,channel[ch].param_cutoff_freq);
		  }
		  break;

      case ME_DATA_ENTRY_MSB:
	if(channel[ch].rpn_7f7f_flag) /* disable */
	    break;
	if((i = last_rpn_addr(ch)) >= 0)
	{
	    channel[ch].rpnmap[i] = ev->a;
	    update_rpn_map(ch, i, 1);
	}
	break;

      case ME_DATA_ENTRY_LSB:
	if(channel[ch].rpn_7f7f_flag) /* disable */
	    break;
	    if((i = last_rpn_addr(ch)) >= 0)
	    {
		channel[ch].rpnmap_lsb[i] = ev->a;
	    }
	break;

	case ME_REVERB_EFFECT:
		if (opt_reverb_control) {
			if (ISDRUMCHANNEL(ch) && get_reverb_level(ch) != ev->a) {channel[ch].drum_effect_flag = 0;}
			set_reverb_level(ch, ev->a);
			ctl_mode_event(CTLE_REVERB_EFFECT, 1, ch, get_reverb_level(ch));
		}
		break;

      case ME_CHORUS_EFFECT:
	if(opt_chorus_control)
	{
		if(opt_chorus_control == 1) {
			if (ISDRUMCHANNEL(ch) && channel[ch].chorus_level != ev->a) {channel[ch].drum_effect_flag = 0;}
			channel[ch].chorus_level = ev->a;
		} else {
			channel[ch].chorus_level = -opt_chorus_control;
		}
	    ctl_mode_event(CTLE_CHORUS_EFFECT, 1, ch, get_chorus_level(ch));
		if(ev->a) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Chorus Send (CH:%d LEVEL:%d)",ch,ev->a);
		}
	}
	break;

      case ME_TREMOLO_EFFECT:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Tremolo Send (CH:%d LEVEL:%d)",ch,ev->a);
	break;

      case ME_CELESTE_EFFECT:
	if(opt_delay_control) {
		if (ISDRUMCHANNEL(ch) && channel[ch].delay_level != ev->a) {channel[ch].drum_effect_flag = 0;}
		channel[ch].delay_level = ev->a;
		if (play_system_mode == XG_SYSTEM_MODE) {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Variation Send (CH:%d LEVEL:%d)",ch,ev->a);
		} else {
			ctl->cmsg(CMSG_INFO,VERB_NOISY,"Delay Send (CH:%d LEVEL:%d)",ch,ev->a);
		}
	}
	break;

	  case ME_ATTACK_TIME:
  	if(!opt_tva_attack) { break; }
	set_envelope_time(ch, ev->a, EG_ATTACK);
	break;

	  case ME_RELEASE_TIME:
  	if(!opt_tva_release) { break; }
	set_envelope_time(ch, ev->a, EG_RELEASE);
	break;

      case ME_PHASER_EFFECT:
	ctl->cmsg(CMSG_INFO,VERB_NOISY,"Phaser Send (CH:%d LEVEL:%d)",ch,ev->a);
	break;

      case ME_RPN_INC:
	if(channel[ch].rpn_7f7f_flag) /* disable */
	    break;
	if((i = last_rpn_addr(ch)) >= 0)
	{
	    if(channel[ch].rpnmap[i] < 127)
		channel[ch].rpnmap[i]++;
	    update_rpn_map(ch, i, 1);
	}
	break;

      case ME_RPN_DEC:
	if(channel[ch].rpn_7f7f_flag) /* disable */
	    break;
	if((i = last_rpn_addr(ch)) >= 0)
	{
	    if(channel[ch].rpnmap[i] > 0)
		channel[ch].rpnmap[i]--;
	    update_rpn_map(ch, i, 1);
	}
	break;

      case ME_NRPN_LSB:
	channel[ch].lastlrpn = ev->a;
	channel[ch].nrpn = 1;
	break;

      case ME_NRPN_MSB:
	channel[ch].lastmrpn = ev->a;
	channel[ch].nrpn = 1;
	break;

      case ME_RPN_LSB:
	channel[ch].lastlrpn = ev->a;
	channel[ch].nrpn = 0;
	break;

      case ME_RPN_MSB:
	channel[ch].lastmrpn = ev->a;
	channel[ch].nrpn = 0;
	break;

      case ME_ALL_SOUNDS_OFF:
	all_sounds_off(ch);
	break;

      case ME_RESET_CONTROLLERS:
	reset_controllers(ch);
	redraw_controllers(ch);
	break;

      case ME_ALL_NOTES_OFF:
	all_notes_off(ch);
	break;

      case ME_MONO:
	channel[ch].mono = 1;
	all_notes_off(ch);
	break;

      case ME_POLY:
	channel[ch].mono = 0;
	all_notes_off(ch);
	break;

	/* TiMidity Extensionals */
      case ME_RANDOM_PAN:
	channel[ch].panning = int_rand(128);
	channel[ch].pan_random = 1;
	if(adjust_panning_immediately && !channel[ch].pan_random)
	    adjust_panning(ch);
	break;

      case ME_SET_PATCH:
	i = channel[ch].special_sample = current_event->a;
	if(special_patch[i] != NULL)
	    special_patch[i]->sample_offset = 0;
	ctl_prog_event(ch);
	break;

      case ME_TEMPO:
	current_play_tempo = ch + ev->b * 256 + ev->a * 65536;
	ctl_mode_event(CTLE_TEMPO, 1, current_play_tempo, 0);
	break;

      case ME_CHORUS_TEXT:
      case ME_LYRIC:
      case ME_MARKER:
      case ME_INSERT_TEXT:
      case ME_TEXT:
      case ME_KARAOKE_LYRIC:
	i = ev->a | ((int)ev->b << 8);
	ctl_mode_event(CTLE_LYRIC, 1, i, 0);
	break;

      case ME_GSLCD:
	i = ev->a | ((int)ev->b << 8);
	ctl_mode_event(CTLE_GSLCD, 1, i, 0);
	break;

      case ME_MASTER_VOLUME:
	master_volume_ratio = (int32)ev->a + 256 * (int32)ev->b;
	adjust_master_volume();
	break;

      case ME_RESET:
	change_system_mode(ev->a);
	reset_midi(1);
	break;

      case ME_PATCH_OFFS:
	i = channel[ch].special_sample;
	if(special_patch[i] != NULL)
	    special_patch[i]->sample_offset =
		(current_event->a | 256 * current_event->b);
	break;

      case ME_WRD:
	push_midi_trace2(wrd_midi_event,
			 ch, current_event->a | (current_event->b << 8));
	break;

      case ME_SHERRY:
	push_midi_trace1(wrd_sherry_event,
			 ch | (current_event->a<<8) | (current_event->b<<16));
	break;

      case ME_DRUMPART:
	if(midi_drumpart_change(ch, current_event->a))
	{
	    /* Update bank information */
	    midi_program_change(ch, channel[ch].program);
	    ctl_mode_event(CTLE_DRUMPART, 1, ch, ISDRUMCHANNEL(ch));
	    ctl_prog_event(ch);
	}
	break;

      case ME_KEYSHIFT:
	i = (int)current_event->a - 0x40;
	if(i != channel[ch].key_shift)
	{
	    all_sounds_off(ch);
	    channel[ch].key_shift = (int8)i;
	}
	break;

	case ME_KEYSIG:
		if (opt_init_keysig != 8)
			break;
		current_keysig = current_event->a + current_event->b * 16;
		ctl_mode_event(CTLE_KEYSIG, 1, current_keysig, 0);
		if (opt_force_keysig != 8) {
			i = current_keysig - ((current_keysig < 8) ? 0 : 16), j = 0;
			while (i != opt_force_keysig && i != opt_force_keysig + 12)
				i += (i > 0) ? -5 : 7, j++;
			while (abs(j - note_key_offset) > 7)
				j += (j > note_key_offset) ? -12 : 12;
			if (abs(j - key_adjust) >= 12)
				j += (j > key_adjust) ? -12 : 12;
			note_key_offset = j;
			kill_all_voices();
			ctl_mode_event(CTLE_KEY_OFFSET, 1, note_key_offset, 0);
		}
		i = current_keysig + ((current_keysig < 8) ? 7 : -9), j = 0;
		while (i != 7)
			i += (i < 7) ? 5 : -7, j++;
		j += note_key_offset, j -= floor(j / 12.0) * 12;
		current_freq_table = j;
		break;

	case ME_SCALE_TUNING:
		resamp_cache_refer_alloff(ch, current_event->time);
		channel[ch].scale_tuning[current_event->a] = current_event->b;
		adjust_pitch(ch);
		break;

	case ME_BULK_TUNING_DUMP:
		set_single_note_tuning(ch, current_event->a, current_event->b, 0);
		break;

	case ME_SINGLE_NOTE_TUNING:
		set_single_note_tuning(ch, current_event->a, current_event->b, 1);
		break;

	case ME_TEMPER_KEYSIG:
		current_temper_keysig = (current_event->a + 8) % 32 - 8;
		temper_adj = ((current_event->a + 8) & 0x20) ? 1 : 0;
		ctl_mode_event(CTLE_TEMPER_KEYSIG, 1, current_event->a, 0);
		i = current_temper_keysig + ((current_temper_keysig < 8) ? 7 : -9);
		j = 0;
		while (i != 7)
			i += (i < 7) ? 5 : -7, j++;
		j += note_key_offset, j -= floor(j / 12.0) * 12;
		current_temper_freq_table = j;
		if (current_event->b)
			for (i = 0; i < upper_voices; i++)
				if (voice[i].status != VOICE_FREE) {
					voice[i].temper_instant = 1;
					recompute_freq(i);
				}
		break;

	case ME_TEMPER_TYPE:
		channel[ch].temper_type = current_event->a;
		ctl_mode_event(CTLE_TEMPER_TYPE, 1, ch, channel[ch].temper_type);
		if (temper_type_mute) {
			if (temper_type_mute & 1 << current_event->a
					- ((current_event->a >= 0x40) ? 0x3c : 0)) {
				SET_CHANNELMASK(channel_mute, ch);
				ctl_mode_event(CTLE_MUTE, 1, ch, 1);
			} else {
				UNSET_CHANNELMASK(channel_mute, ch);
				ctl_mode_event(CTLE_MUTE, 1, ch, 0);
			}
		}
		if (current_event->b)
			for (i = 0; i < upper_voices; i++)
				if (voice[i].status != VOICE_FREE) {
					voice[i].temper_instant = 1;
					recompute_freq(i);
				}
		break;

	case ME_MASTER_TEMPER_TYPE:
		for (i = 0; i < MAX_CHANNELS; i++) {
			channel[i].temper_type = current_event->a;
			ctl_mode_event(CTLE_TEMPER_TYPE, 1, i, channel[i].temper_type);
		}
		if (temper_type_mute) {
			if (temper_type_mute & 1 << current_event->a
					- ((current_event->a >= 0x40) ? 0x3c : 0)) {
				FILL_CHANNELMASK(channel_mute);
				for (i = 0; i < MAX_CHANNELS; i++)
					ctl_mode_event(CTLE_MUTE, 1, i, 1);
			} else {
				CLEAR_CHANNELMASK(channel_mute);
				for (i = 0; i < MAX_CHANNELS; i++)
					ctl_mode_event(CTLE_MUTE, 1, i, 0);
			}
		}
		if (current_event->b)
			for (i = 0; i < upper_voices; i++)
				if (voice[i].status != VOICE_FREE) {
					voice[i].temper_instant = 1;
					recompute_freq(i);
				}
		break;

	case ME_USER_TEMPER_ENTRY:
		set_user_temper_entry(ch, current_event->a, current_event->b);
		break;

	case ME_SYSEX_LSB:
		process_sysex_event(ME_SYSEX_LSB,ch,current_event->a,current_event->b);
	    break;

	case ME_SYSEX_MSB:
		process_sysex_event(ME_SYSEX_MSB,ch,current_event->a,current_event->b);
	    break;

	case ME_SYSEX_GS_LSB:
		process_sysex_event(ME_SYSEX_GS_LSB,ch,current_event->a,current_event->b);
	    break;

	case ME_SYSEX_GS_MSB:
		process_sysex_event(ME_SYSEX_GS_MSB,ch,current_event->a,current_event->b);
	    break;

	case ME_SYSEX_XG_LSB:
		process_sysex_event(ME_SYSEX_XG_LSB,ch,current_event->a,current_event->b);
	    break;

	case ME_SYSEX_XG_MSB:
		process_sysex_event(ME_SYSEX_XG_MSB,ch,current_event->a,current_event->b);
	    break;

	case ME_NOTE_STEP:
		i = ev->a + ((ev->b & 0x0f) << 8);
		j = ev->b >> 4;
		ctl_mode_event(CTLE_METRONOME, 1, i, j);
		if (readmidi_wrd_mode)
			wrdt->update_events();
		break;

      case ME_EOT:
	return midi_play_end();
    }
#ifndef SUPPRESS_CHANNEL_LAYER
		}
	}
	ev->channel = orig_ch;
#endif

    return RC_NONE;
}

static void set_single_note_tuning(int part, int a, int b, int rt)
{
	static int tp;	/* tuning program number */
	static int kn;	/* MIDI key number */
	static int st;	/* the nearest equal-tempered semitone */
	double f, fst;	/* fraction of semitone */
	int i;
	
	switch (part) {
	case 0:
		tp = a;
		break;
	case 1:
		kn = a, st = b;
		break;
	case 2:
		if (st == 0x7f && a == 0x7f && b == 0x7f)	/* no change */
			break;
		f = 440 * pow(2.0, (st - 69) / 12.0);
		fst = pow(2.0, (a << 7 | b) / 196608.0);
		freq_table_tuning[tp][kn] = f * fst * 1000 + 0.5;
		if (rt)
			for (i = 0; i < upper_voices; i++)
				if (voice[i].status != VOICE_FREE) {
					voice[i].temper_instant = 1;
					recompute_freq(i);
				}
		break;
	}
}

static void set_user_temper_entry(int part, int a, int b)
{
	static int tp;		/* temperament program number */
	static int ll;		/* number of formula */
	static int fh, fl;	/* applying pitch bit mask (forward) */
	static int bh, bl;	/* applying pitch bit mask (backward) */
	static int aa, bb;	/* fraction (aa/bb) */
	static int cc, dd;	/* power (cc/dd)^(ee/ff) */
	static int ee, ff;
	static int ifmax, ibmax, count;
	static double rf[11], rb[11];
	int i, j, k, l, n, m;
	double ratio[12], f, sc;
	
	switch (part) {
	case 0:
		for (i = 0; i < 11; i++)
			rf[i] = rb[i] = 1;
		ifmax = ibmax = 0;
		count = 0;
		tp = a, ll = b;
		break;
	case 1:
		fh = a, fl = b;
		break;
	case 2:
		bh = a, bl = b;
		break;
	case 3:
		aa = a, bb = b;
		break;
	case 4:
		cc = a, dd = b;
		break;
	case 5:
		ee = a, ff = b;
		for (i = 0; i < 11; i++) {
			if (((fh & 0xf) << 7 | fl) & 1 << i) {
				rf[i] *= (double) aa / bb
						* pow((double) cc / dd, (double) ee / ff);
				if (ifmax < i + 1)
					ifmax = i + 1;
			}
			if (((bh & 0xf) << 7 | bl) & 1 << i) {
				rb[i] *= (double) aa / bb
						* pow((double) cc / dd, (double) ee / ff);
				if (ibmax < i + 1)
					ibmax = i + 1;
			}
		}
		if (++count < ll)
			break;
		ratio[0] = 1;
		for (i = n = m = 0; i < ifmax; i++, m = n) {
			n += (n > 4) ? -5 : 7;
			ratio[n] = ratio[m] * rf[i];
			if (ratio[n] > 2)
				ratio[n] /= 2;
		}
		for (i = n = m = 0; i < ibmax; i++, m = n) {
			n += (n > 6) ? -7 : 5;
			ratio[n] = ratio[m] / rb[i];
			if (ratio[n] < 1)
				ratio[n] *= 2;
		}
		sc = 27 / ratio[9] / 16;	/* syntonic comma */
		for (i = 0; i < 12; i++)
			for (j = -1; j < 11; j++) {
				f = 440 * pow(2.0, (i - 9) / 12.0 + j - 5);
				for (k = 0; k < 12; k++) {
					l = i + j * 12 + k;
					if (l < 0 || l >= 128)
						continue;
					if (! (fh & 0x40)) {	/* major */
						freq_table_user[tp][i][l] =
								f * ratio[k] * 1000 + 0.5;
						freq_table_user[tp][i + 36][l] =
								f * ratio[k] * sc * 1000 + 0.5;
					}
					if (! (bh & 0x40)) {	/* minor */
						freq_table_user[tp][i + 12][l] =
								f * ratio[k] * sc * 1000 + 0.5;
						freq_table_user[tp][i + 24][l] =
								f * ratio[k] * 1000 + 0.5;
					}
				}
			}
		break;
	}
}

static int play_midi(MidiEvent *eventlist, int32 samples)
{
    int rc;
    static int play_count = 0;

/*    if (play_mode->id_character == 'M') {
	int cnt;

	convert_mod_to_midi_file(eventlist);

	play_count = 0;
	cnt = free_global_mblock();	// free unused memory
	if(cnt > 0)
	    ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		      "%d memory blocks are free", cnt);
	return RC_NONE;
    }
*/
    sample_count = samples;
    event_list = eventlist;
    lost_notes = cut_notes = 0;
    check_eot_flag = 1;

    wrd_midi_event(-1, -1); /* For initialize */

    reset_midi(0);
    if(!opt_realtime_playing &&
       allocate_cache_size > 0 &&
       !IS_CURRENT_MOD_FILE &&
       (play_mode->flag&PF_PCM_STREAM))
    {
	play_midi_prescan(eventlist);
	reset_midi(0);
    }

    rc = aq_flush(0);
    if(RC_IS_SKIP_FILE(rc))
	return rc;

    skip_to(midi_restart_time);

    if(midi_restart_time > 0) { /* Need to update interface display */
      int i;
      for(i = 0; i < MAX_CHANNELS; i++)
	redraw_controllers(i);
    }
    rc = RC_NONE;
    for(;;)
    {
	midi_restart_time = 1;
	rc = play_event(current_event);
	if(rc != RC_NONE)
	    break;
	if (midi_restart_time)    /* don't skip the first event if == 0 */
	    current_event++;
    }

    if(play_count++ > 3)
    {
	int cnt;
	play_count = 0;
	cnt = free_global_mblock();	/* free unused memory */
	if(cnt > 0)
	    ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
		      "%d memory blocks are free", cnt);
    }
    return rc;
}

static void read_header_wav(struct timidity_file* tf)
{
    char buff[44];
    tf_read( buff, 1, 44, tf);
}

static int read_header_aiff(struct timidity_file* tf)
{
    char buff[5]="    ";
    int i;
    
    for( i=0; i<100; i++ ){
    	buff[0]=buff[1]; buff[1]=buff[2]; buff[2]=buff[3];
    	tf_read( &buff[3], 1, 1, tf);
    	if( strcmp(buff,"SSND")==0 ){
            /*SSND chunk found */
    	    tf_read( &buff[0], 1, 4, tf);
    	    tf_read( &buff[0], 1, 4, tf);
	    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "aiff header read OK.");
	    return 0;
    	}
    }
    /*SSND chunk not found */
    return -1;
}

static int load_pcm_file_wav()
{
    char *filename;

    if(strcmp(pcm_alternate_file, "auto") == 0)
    {
	filename = safe_malloc(strlen(current_file_info->filename)+5);
	strcpy(filename, current_file_info->filename);
	strcat(filename, ".wav");
    }
    else if(strlen(pcm_alternate_file) >= 5 &&
	    strncasecmp(pcm_alternate_file + strlen(pcm_alternate_file) - 4,
			".wav", 4) == 0)
	filename = safe_strdup(pcm_alternate_file);
    else
	return -1;

    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "wav filename: %s", filename);
    current_file_info->pcm_tf = open_file(filename, 0, OF_SILENT);
    if( current_file_info->pcm_tf ){
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "open successed.");
	read_header_wav(current_file_info->pcm_tf);
	current_file_info->pcm_filename = filename;
	current_file_info->pcm_mode = PCM_MODE_WAV;
	return 0;
    }else{
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "open failed.");
	free(filename);
	current_file_info->pcm_filename = NULL;
	return -1;
    }
}

static int load_pcm_file_aiff()
{
    char *filename;

    if(strcmp(pcm_alternate_file, "auto") == 0)
    {
	filename = safe_malloc(strlen(current_file_info->filename)+6);
	strcpy(filename, current_file_info->filename);
	strcat( filename, ".aiff");
    }
    else if(strlen(pcm_alternate_file) >= 6 &&
	    strncasecmp(pcm_alternate_file + strlen(pcm_alternate_file) - 5,
			".aiff", 5) == 0)
	filename = safe_strdup(pcm_alternate_file);
    else
	return -1;

    ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "aiff filename: %s", filename);
    current_file_info->pcm_tf = open_file(filename, 0, OF_SILENT);
    if( current_file_info->pcm_tf ){
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "open successed.");
	read_header_aiff(current_file_info->pcm_tf);
	current_file_info->pcm_filename = filename;
	current_file_info->pcm_mode = PCM_MODE_AIFF;
	return 0;
    }else{
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		      "open failed.");
	free(filename);
	current_file_info->pcm_filename = NULL;
	return -1;
    }
}

static void load_pcm_file()
{
    if( load_pcm_file_wav()==0 ) return; /*load OK*/
    if( load_pcm_file_aiff()==0 ) return; /*load OK*/
}

static int play_midi_load_file(char *fn,
			       MidiEvent **event,
			       int32 *nsamples)
{
    int rc;
    struct timidity_file *tf;
    int32 nevents;

    *event = NULL;

    if(!strcmp(fn, "-"))
	file_from_stdin = 1;
    else
	file_from_stdin = 0;

    ctl_mode_event(CTLE_NOW_LOADING, 0, (long)fn, 0);
    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "MIDI file: %s", fn);
    if((tf = open_midi_file(fn, 1, OF_VERBOSE)) == NULL)
    {
	ctl_mode_event(CTLE_LOADING_DONE, 0, -1, 0);
	return RC_ERROR;
    }

    *event = NULL;
    rc = check_apply_control();
    if(RC_IS_SKIP_FILE(rc))
    {
	close_file(tf);
	ctl_mode_event(CTLE_LOADING_DONE, 0, 1, 0);
	return rc;
    }

    *event = read_midi_file(tf, &nevents, nsamples, fn);
    close_file(tf);

    if(*event == NULL)
    {
	ctl_mode_event(CTLE_LOADING_DONE, 0, -1, 0);
	return RC_ERROR;
    }

    ctl->cmsg(CMSG_INFO, VERB_NOISY,
	      "%d supported events, %d samples, time %d:%02d",
	      nevents, *nsamples,
	      *nsamples / play_mode->rate / 60,
	      (*nsamples / play_mode->rate) % 60);

    current_file_info->pcm_mode = PCM_MODE_NON; /*initialize*/
    if(pcm_alternate_file != NULL &&
       strcmp(pcm_alternate_file, "none") != 0 &&
       (play_mode->flag&PF_PCM_STREAM))
	load_pcm_file();

    if(!IS_CURRENT_MOD_FILE &&
       (play_mode->flag&PF_PCM_STREAM))
    {
	/* FIXME: Instruments is not need for pcm_alternate_file. */

	/* Load instruments
	 * If opt_realtime_playing, the instruments will be loaded later.
	 */
	if(!opt_realtime_playing)
	{
	    rc = RC_NONE;
	    load_missing_instruments(&rc);
	    if(RC_IS_SKIP_FILE(rc))
	    {
		/* Instrument loading is terminated */
		ctl_mode_event(CTLE_LOADING_DONE, 0, 1, 0);
		clear_magic_instruments();
		return rc;
	    }
	}
    }
    else
	clear_magic_instruments();	/* Clear load markers */

    ctl_mode_event(CTLE_LOADING_DONE, 0, 0, 0);

    return RC_NONE;
}

int play_midi_file(char *fn)
{
    int i, j, rc;
    static int last_rc = RC_NONE;
    MidiEvent *event;
    int32 nsamples;

    /* Set current file information */
    current_file_info = get_midi_file_info(fn, 1);

    rc = check_apply_control();
    if(RC_IS_SKIP_FILE(rc) && rc != RC_RELOAD)
	return rc;

    /* Reset key & speed each files */
    current_keysig = (opt_init_keysig == 8) ? 0 : opt_init_keysig;
    note_key_offset = key_adjust;
    midi_time_ratio = tempo_adjust;
	for (i = 0; i < MAX_CHANNELS; i++) {
		for (j = 0; j < 12; j++)
			channel[i].scale_tuning[j] = 0;
		channel[i].prev_scale_tuning = 0;
		channel[i].temper_type = 0;
	}
    CLEAR_CHANNELMASK(channel_mute);
	if (temper_type_mute & 1)
		FILL_CHANNELMASK(channel_mute);

    /* Reset restart offset */
    midi_restart_time = 0;

#ifdef REDUCE_VOICE_TIME_TUNING
    /* Reset voice reduction stuff */
    min_bad_nv = 256;
    max_good_nv = 1;
    ok_nv_total = 32;
    ok_nv_counts = 1;
    ok_nv = 32;
    ok_nv_sample = 0;
    old_rate = -1;
    reduce_quality_flag = no_4point_interpolation;
    restore_voices(0);
#endif

	ctl_mode_event(CTLE_METRONOME, 0, 0, 0);
	ctl_mode_event(CTLE_KEYSIG, 0, current_keysig, 0);
	ctl_mode_event(CTLE_TEMPER_KEYSIG, 0, 0, 0);
	ctl_mode_event(CTLE_KEY_OFFSET, 0, note_key_offset, 0);
	i = current_keysig + ((current_keysig < 8) ? 7 : -9), j = 0;
	while (i != 7)
		i += (i < 7) ? 5 : -7, j++;
	j += note_key_offset, j -= floor(j / 12.0) * 12;
	current_freq_table = j;
	ctl_mode_event(CTLE_TEMPO, 0, current_play_tempo, 0);
	ctl_mode_event(CTLE_TIME_RATIO, 0, 100 / midi_time_ratio + 0.5, 0);
	for (i = 0; i < MAX_CHANNELS; i++) {
		ctl_mode_event(CTLE_TEMPER_TYPE, 0, i, channel[i].temper_type);
		ctl_mode_event(CTLE_MUTE, 0, i, temper_type_mute & 1);
	}
  play_reload: /* Come here to reload MIDI file */
    rc = play_midi_load_file(fn, &event, &nsamples);
    if(RC_IS_SKIP_FILE(rc))
	goto play_end; /* skip playing */

    init_mblock(&playmidi_pool);
    ctl_mode_event(CTLE_PLAY_START, 0, nsamples, 0);
    play_mode->acntl(PM_REQ_PLAY_START, NULL);
    rc = play_midi(event, nsamples);
    play_mode->acntl(PM_REQ_PLAY_END, NULL);
    ctl_mode_event(CTLE_PLAY_END, 0, 0, 0);
    reuse_mblock(&playmidi_pool);

    for(i = 0; i < MAX_CHANNELS; i++)
	memset(channel[i].drums, 0, sizeof(channel[i].drums));

  play_end:
    if(current_file_info->pcm_tf){
    	close_file(current_file_info->pcm_tf);
    	current_file_info->pcm_tf = NULL;
    	free( current_file_info->pcm_filename );
    	current_file_info->pcm_filename = NULL;
    }
    
    if(wrdt->opened)
	wrdt->end();

    if(free_instruments_afterwards)
    {
	int cnt;
	free_instruments(0);
	cnt = free_global_mblock(); /* free unused memory */
	if(cnt > 0)
	    ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "%d memory blocks are free",
		      cnt);
    }

    free_special_patch(-1);

    if(event != NULL)
	free(event);
    if(rc == RC_RELOAD)
	goto play_reload;

    if(rc == RC_ERROR)
    {
	if(current_file_info->file_type == IS_OTHER_FILE)
	    current_file_info->file_type = IS_ERROR_FILE;
	if(last_rc == RC_REALLY_PREVIOUS)
	    return RC_REALLY_PREVIOUS;
    }
    last_rc = rc;
    return rc;
}

void dumb_pass_playing_list(int number_of_files, char *list_of_files[])
{
    #ifndef CFG_FOR_SF
    int i = 0;

    for(;;)
    {
	switch(play_midi_file(list_of_files[i]))
	{
	  case RC_REALLY_PREVIOUS:
	    if(i > 0)
		i--;
	    break;

	  default: /* An error or something */
	  case RC_NEXT:
	    if(i < number_of_files-1)
	    {
		i++;
		break;
	    }
	    aq_flush(0);

	    if(!(ctl->flags & CTLF_LIST_LOOP))
		return;
	    i = 0;
	    break;

	    case RC_QUIT:
		return;
	}
    }
    #endif
}

void default_ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
	ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "%s", lyric + 1);
}

void ctl_mode_event(int type, int trace, long arg1, long arg2)
{
    CtlEvent ce;
    ce.type = type;
    ce.v1 = arg1;
    ce.v2 = arg2;
    if(trace && ctl->trace_playing)
	push_midi_trace_ce(ctl->event, &ce);
    else
	ctl->event(&ce);
}

void ctl_note_event(int noteID)
{
    CtlEvent ce;
    ce.type = CTLE_NOTE;
    ce.v1 = voice[noteID].status;
    ce.v2 = voice[noteID].channel;
    ce.v3 = voice[noteID].note;
    ce.v4 = voice[noteID].velocity;
    if(ctl->trace_playing)
	push_midi_trace_ce(ctl->event, &ce);
    else
	ctl->event(&ce);
}

static void ctl_timestamp(void)
{
    long i, secs, voices;
    CtlEvent ce;
    static int last_secs = -1, last_voices = -1;

    secs = (long)(current_sample / (midi_time_ratio * play_mode->rate));
    for(i = voices = 0; i < upper_voices; i++)
	if(voice[i].status != VOICE_FREE)
	    voices++;
    if(secs == last_secs && voices == last_voices)
	return;
    ce.type = CTLE_CURRENT_TIME;
    ce.v1 = last_secs = secs;
    ce.v2 = last_voices = voices;
    if(ctl->trace_playing)
	push_midi_trace_ce(ctl->event, &ce);
    else
	ctl->event(&ce);
}

static void ctl_updatetime(int32 samples)
{
    long secs;
    secs = (long)(samples / (midi_time_ratio * play_mode->rate));
    ctl_mode_event(CTLE_CURRENT_TIME, 0, secs, 0);
    ctl_mode_event(CTLE_REFRESH, 0, 0, 0);
}

static void ctl_prog_event(int ch)
{
    CtlEvent ce;
    int bank, prog;

    if(IS_CURRENT_MOD_FILE)
    {
	bank = 0;
	prog = channel[ch].special_sample;
    }
    else
    {
	bank = channel[ch].bank;
	prog = channel[ch].program;
    }

    ce.type = CTLE_PROGRAM;
    ce.v1 = ch;
    ce.v2 = prog;
    ce.v3 = (long)channel_instrum_name(ch);
    ce.v4 = (bank |
	     (channel[ch].bank_lsb << 8) |
	     (channel[ch].bank_msb << 16));
    if(ctl->trace_playing)
	push_midi_trace_ce(ctl->event, &ce);
    else
	ctl->event(&ce);
}

static void ctl_pause_event(int pause, int32 s)
{
    long secs;
    secs = (long)(s / (midi_time_ratio * play_mode->rate));
    ctl_mode_event(CTLE_PAUSE, 0, pause, secs);
}

char *channel_instrum_name(int ch)
{
    char *comm;
    int bank, prog;

    if(ISDRUMCHANNEL(ch)) {
	bank = channel[ch].bank;
	if (drumset[bank] == NULL) return "";
	prog = 0;
	comm = drumset[bank]->tone[prog].comment;
	if (comm == NULL) return "";
	return comm;
    }

    if(channel[ch].program == SPECIAL_PROGRAM)
	return "Special Program";

    if(IS_CURRENT_MOD_FILE)
    {
	int pr;
	pr = channel[ch].special_sample;
	if(pr > 0 &&
	   special_patch[pr] != NULL &&
	   special_patch[pr]->name != NULL)
	    return special_patch[pr]->name;
	return "MOD";
    }

    bank = channel[ch].bank;
    prog = channel[ch].program;
    instrument_map(channel[ch].mapID, &bank, &prog);

	if (tonebank[bank] == NULL) {alloc_instrument_bank(0, bank);}
	if (tonebank[bank]->tone[prog].name) {
	    comm = tonebank[bank]->tone[prog].comment;
		if (comm == NULL) {comm = tonebank[bank]->tone[prog].name;}
	} else {
	    comm = tonebank[0]->tone[prog].comment;
		if (comm == NULL) {comm = tonebank[0]->tone[prog].name;}
	}
	
    return comm;
}


/*
 * For MIDI stream player.
 */
void playmidi_stream_init(void)
{
    int i;
    static int first = 1;

    note_key_offset = key_adjust;
    midi_time_ratio = tempo_adjust;
    CLEAR_CHANNELMASK(channel_mute);
	if (temper_type_mute & 1)
		FILL_CHANNELMASK(channel_mute);
    midi_restart_time = 0;
    if(first)
    {
	first = 0;
        init_mblock(&playmidi_pool);
	current_file_info = get_midi_file_info("TiMidity", 1);
    midi_streaming=1;
    }
    else
        reuse_mblock(&playmidi_pool);

    /* Fill in current_file_info */
    current_file_info->readflag = 1;
    current_file_info->seq_name = safe_strdup("TiMidity server");
    current_file_info->karaoke_title = current_file_info->first_text = NULL;
    current_file_info->mid = 0x7f;
    current_file_info->hdrsiz = 0;
    current_file_info->format = 0;
    current_file_info->tracks = 0;
    current_file_info->divisions = 192; /* ?? */
    current_file_info->time_sig_n = 4; /* 4/ */
    current_file_info->time_sig_d = 4; /* /4 */
    current_file_info->time_sig_c = 24; /* clock */
    current_file_info->time_sig_b = 8;  /* q.n. */
    current_file_info->samples = 0;
    current_file_info->max_channel = MAX_CHANNELS;
    current_file_info->compressed = 0;
    current_file_info->midi_data = NULL;
    current_file_info->midi_data_size = 0;
    current_file_info->file_type = IS_OTHER_FILE;

    current_play_tempo = 500000;
    check_eot_flag = 0;

    /* Setup default drums */
	COPY_CHANNELMASK(current_file_info->drumchannels, default_drumchannels);
	COPY_CHANNELMASK(current_file_info->drumchannel_mask, default_drumchannel_mask);
    for(i = 0; i < MAX_CHANNELS; i++)
	memset(channel[i].drums, 0, sizeof(channel[i].drums));
    change_system_mode(DEFAULT_SYSTEM_MODE);
    reset_midi(0);

    playmidi_tmr_reset();
}

void playmidi_tmr_reset(void)
{
    int i;

    aq_flush(0);
    current_sample = 0;
    buffered_count = 0;
    buffer_pointer = common_buffer;
    for(i = 0; i < MAX_CHANNELS; i++)
	channel[i].lasttime = 0;
    play_mode->acntl(PM_REQ_PLAY_START, NULL);
}

/*! initialize Part EQ (XG) */
void init_part_eq_xg(struct part_eq_xg *p)
{
	p->bass = 0x40;
	p->treble = 0x40;
	p->bass_freq = 0x0C;
	p->treble_freq = 0x36;
	p->valid = 0;
}

/*! recompute Part EQ (XG) */
void recompute_part_eq_xg(struct part_eq_xg *p)
{
	int8 vbass, vtreble;

	if(p->bass_freq >= 4 && p->bass_freq <= 40 && p->bass != 0x40) {
		vbass = 1;
		p->basss.q = 0.7;
		p->basss.freq = eq_freq_table_xg[p->bass_freq];
		if(p->bass == 0) {p->basss.gain = -12.0;}
		else {p->basss.gain = 0.19 * (double)(p->bass - 0x40);}
		calc_filter_shelving_low(&(p->basss));
	} else {vbass = 0;}
	if(p->treble_freq >= 28 && p->treble_freq <= 58 && p->treble != 0x40) {
		vtreble = 1;
		p->trebles.q = 0.7;
		p->trebles.freq = eq_freq_table_xg[p->treble_freq];
		if(p->treble == 0) {p->trebles.gain = -12.0;}
		else {p->trebles.gain = 0.19 * (double)(p->treble - 0x40);}
		calc_filter_shelving_high(&(p->trebles));
	} else {vtreble = 0;}
	p->valid = vbass || vtreble;
}

static void init_midi_controller(midi_controller *p)
{
	p->val = 0;
	p->pitch = 0;
	p->cutoff = 0;
	p->amp = 0.0;
	p->lfo1_rate = p->lfo2_rate = p->lfo1_tva_depth = p->lfo2_tva_depth = 0;
	p->lfo1_pitch_depth = p->lfo2_pitch_depth = p->lfo1_tvf_depth = p->lfo2_tvf_depth = 0;
	p->variation_control_depth = p->insertion_control_depth = 0;
}

static float get_midi_controller_amp(midi_controller *p)
{
	return (1.0 + (float)p->val * (1.0f / 127.0f) * p->amp);
}

static float get_midi_controller_filter_cutoff(midi_controller *p)
{
	return ((float)p->val * (1.0f / 127.0f) * (float)p->cutoff);
}

static float get_midi_controller_filter_depth(midi_controller *p)
{
	return ((float)p->val * (1.0f / 127.0f) * (float)p->lfo1_tvf_depth);
}

static int32 get_midi_controller_pitch(midi_controller *p)
{
	return ((int32)(p->val * p->pitch) << 6);
}

static int16 get_midi_controller_pitch_depth(midi_controller *p)
{
	return (int16)((float)p->val * (float)p->lfo1_pitch_depth * (1.0f / 127.0f * 256.0 / 400.0));
}

static int16 get_midi_controller_amp_depth(midi_controller *p)
{
	return (int16)((float)p->val * (float)p->lfo1_tva_depth * (1.0f / 127.0f * 256.0));
}

static void init_rx(int ch)
{
	channel[ch].rx = 0xFFFFFFFF;	/* all on */
}

static void set_rx(int ch, int32 rx, int flag)
{
	if(ch > MAX_CHANNELS) {return;}
	if(flag) {channel[ch].rx |= rx;}
	else {channel[ch].rx &= ~rx;}
}

#if 0
static int32 get_rx(int ch, int32 rx)
{
	return (channel[ch].rx & rx);
}
#endif

static void init_rx_drum(struct DrumParts *p)
{
	p->rx = 0xFFFFFFFF;	/* all on */
}

static void set_rx_drum(struct DrumParts *p, int32 rx, int flag)
{
	if(flag) {p->rx |= rx;}
	else {p->rx &= ~rx;}
}

static int32 get_rx_drum(struct DrumParts *p, int32 rx)
{
	return (p->rx & rx);
}

#if defined (DEBUG_WAV_OUTPUT)
static FILE * fp_wav_out;
#endif

MidiSong *Timidity_LoadSong(char *fn)
{
	int i, j, rc;

    /* Set current file information */
    current_file_info = get_midi_file_info(fn, 1);

    /* Reset key & speed each files */
    current_keysig = (opt_init_keysig == 8) ? 0 : opt_init_keysig;
    note_key_offset = key_adjust;
    midi_time_ratio = tempo_adjust;
	for (i = 0; i < MAX_CHANNELS; i++) {
		for (j = 0; j < 12; j++)
			channel[i].scale_tuning[j] = 0;
		channel[i].prev_scale_tuning = 0;
		channel[i].temper_type = 0;
	}
    CLEAR_CHANNELMASK(channel_mute);
	if (temper_type_mute & 1)
		FILL_CHANNELMASK(channel_mute);

    /* Reset restart offset */
    midi_restart_time = 0;

#ifdef REDUCE_VOICE_TIME_TUNING
    /* Reset voice reduction stuff */
    min_bad_nv = 256;
    max_good_nv = 1;
    ok_nv_total = 32;
    ok_nv_counts = 1;
    ok_nv = 32;
    ok_nv_sample = 0;
    old_rate = -1;
    reduce_quality_flag = no_4point_interpolation;
    restore_voices(0);
#endif

	ctl_mode_event(CTLE_METRONOME, 0, 0, 0);
	ctl_mode_event(CTLE_KEYSIG, 0, current_keysig, 0);
	ctl_mode_event(CTLE_TEMPER_KEYSIG, 0, 0, 0);
	ctl_mode_event(CTLE_KEY_OFFSET, 0, note_key_offset, 0);
	i = current_keysig + ((current_keysig < 8) ? 7 : -9), j = 0;
	while (i != 7)
		i += (i < 7) ? 5 : -7, j++;
	j += note_key_offset, j -= floor(j / 12.0) * 12;
	current_freq_table = j;
	ctl_mode_event(CTLE_TEMPO, 0, current_play_tempo, 0);
	ctl_mode_event(CTLE_TIME_RATIO, 0, 100 / midi_time_ratio + 0.5, 0);
	for (i = 0; i < MAX_CHANNELS; i++) {
		ctl_mode_event(CTLE_TEMPER_TYPE, 0, i, channel[i].temper_type);
		ctl_mode_event(CTLE_MUTE, 0, i, temper_type_mute & 1);
	}

	// Load the file
	MidiSong song, *retsong;
	if ( play_midi_load_file( fn, &song.events, &song.samples ) != RC_NONE )
	{
		// FIXME: maybe some further cleanup needed
		return 0;
	}

	// Allocate a returned structure
	retsong = (MidiSong *)safe_malloc(sizeof(*retsong));
	retsong->events = song.events;
	retsong->samples = song.samples;
	retsong->stored_size = 0;
	retsong->stored_buffer = 0;
	retsong->output_buffer = 0;
	retsong->output_size = 0;
	retsong->output_offset = 0;
	retsong->end_of_song_reached = 0;

	// Set output buffer
	outbuf_set_data( retsong );

    init_mblock(&playmidi_pool);
    ctl_mode_event( CTLE_PLAY_START, 0, retsong->samples, 0 );
    play_mode->acntl(PM_REQ_PLAY_START, NULL);

    sample_count = retsong->samples;
    event_list = retsong->events;
    lost_notes = cut_notes = 0;
    check_eot_flag = 1;

    wrd_midi_event(-1, -1); /* For initialize */

    reset_midi(0);
    if(!opt_realtime_playing &&
       allocate_cache_size > 0 &&
       !IS_CURRENT_MOD_FILE &&
       (play_mode->flag&PF_PCM_STREAM))
    {
		play_midi_prescan(retsong->events);
		reset_midi(0);
    }

    rc = aq_flush(0);
    skip_to(midi_restart_time);

#if defined DEBUG_WAV_OUTPUT
	if ( (fp_wav_out = fopen("q:\\debug.wav", "w+")) != 0 )
	{
		char * origRIFFheader=
  		"RIFF" "\377\377\377\377"
  		"WAVE" "fmt " "\020\000\000\000" "\001\000"
  		/* 22: channels */ "\001\000"
  		/* 24: frequency */ "xxxx"
  		/* 28: bytes/second */ "xxxx"
  		/* 32: bytes/sample */ "\004\000"
  		/* 34: bits/sample */ "\020\000"
  		"data" "\377\377\377\377";
		char RIFFheader[44];

		memcpy(RIFFheader, origRIFFheader, 44);
		RIFFheader[20] = 0x01; // WAVE_FORMAT_PCM
		RIFFheader[22] = 2; // stereo
		*((int *)(RIFFheader+24)) = 48000; // sample
		*((int *)(RIFFheader+28)) = *((int *)(RIFFheader+24)) * 4; // 16-bit stereo
		RIFFheader[32] = 4;
		RIFFheader[34] = 16;

		fwrite( RIFFheader, 1, 44, fp_wav_out );
	}
#endif

	return retsong;
}


void Timidity_FreeSong(MidiSong *song)
{
	// Disconnect the buffer
	outbuf_set_data( 0 );

    if ( current_file_info->pcm_tf )
	{
    	close_file(current_file_info->pcm_tf);
    	current_file_info->pcm_tf = NULL;
    	free( current_file_info->pcm_filename );
    	current_file_info->pcm_filename = NULL;
    }
    
    if ( wrdt->opened )
		wrdt->end();

    if ( free_instruments_afterwards )
    {
		free_instruments(0);
		free_global_mblock();
    }

    free_special_patch(-1);

    if( song->events )
		free( song->events );

    if( song->stored_buffer )
		free( song->stored_buffer );

	if ( reverb_buffer )
	{
		free( reverb_buffer );
		reverb_buffer = 0;
	}

	free( song );

#if defined DEBUG_WAV_OUTPUT
	if ( fp_wav_out )
	{
		fclose( fp_wav_out );
		fp_wav_out = 0;
	}
#endif
}


int Timidity_FillBuffer( MidiSong* song, void *buf, unsigned int size )
{
	if ( song->end_of_song_reached )
		return 0;

	// Init the pointer
	song->output_buffer = buf;
	song->output_size = size;
	song->output_offset = 0;

	// But first check if we have anything left in our prestored buffer
	if ( song->stored_size )
	{
		int copylength = song->output_size > song->stored_size ? song->stored_size : song->output_size;

		// Copy it
		memcpy( buf, song->stored_buffer, copylength );
		song->stored_size -= copylength;

		// In case there is still data in the stored_buffer, move it and return.
		if ( song->stored_size > 0 )
		{
			memmove( song->stored_buffer, song->stored_buffer + copylength, song->stored_size );
			song->output_offset = copylength;
			
			// and the while() loop will throw us away
		}
		else
		{
			// The buffer is filled but not completely. Free stored buffer.
			free( song->stored_buffer );
			song->stored_buffer = 0;
			song->stored_size = 0;

			// and adjust variables
			song->output_offset = copylength;
		}
	}

	// Now play the song until the buffer is full.
	// Again, since offset is zero-based, max offset is song->output_size - 1
	while ( song->output_offset < song->output_size )
	{
		int rc = play_event(current_event);

		if ( rc != RC_NONE )
		{
			song->end_of_song_reached = 1;
			break;
		}

		current_event++;
	}

	// Disconnect the buffer, and return. Note that output_offset is not necessary output_size
	// as the song might end prematurely.
	song->output_buffer = 0;
	song->output_size = 0;

#if defined DEBUG_WAV_OUTPUT
	if ( fp_wav_out )
		fwrite( buf, song->output_offset, 1, fp_wav_out );
#endif

	return song->output_offset;
}


unsigned long Timidity_Seek( MidiSong *song, unsigned long iTimePos )
{
	skip_to( iTimePos/1000*48000);
	return iTimePos;
}
