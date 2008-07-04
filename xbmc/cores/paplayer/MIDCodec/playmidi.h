/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   playmidi.h

   */

typedef struct {
  int32 time;
  uint8 channel, type, a, b;
} MidiEvent;

/* Midi events */
#define ME_NONE 	0
#define ME_NOTEON	1
#define ME_NOTEOFF	2
#define ME_KEYPRESSURE	3
#define ME_MAINVOLUME	4
#define ME_PAN		5
#define ME_SUSTAIN	6
#define ME_EXPRESSION	7
#define ME_PITCHWHEEL	8
#define ME_PROGRAM	9
#define ME_TEMPO	10
#define ME_PITCH_SENS	11

#define ME_ALL_SOUNDS_OFF	12
#define ME_RESET_CONTROLLERS	13
#define ME_ALL_NOTES_OFF	14
#define ME_TONE_BANK	15

#define ME_LYRIC	16

#define ME_EOT		99

typedef struct {
  int
    bank, program, volume, sustain, panning, pitchbend, expression, 
    mono, /* one note only on this channel -- not implemented yet */
    pitchsens;
  /* chorus, reverb... Coming soon to a 300-MHz, eight-way superscalar
     processor near you */
  float
    pitchfactor; /* precomputed pitch bend factor to save some fdiv's */
} Channel;

/* Causes the instrument's default panning to be used. */
#define NO_PANNING -1

typedef struct {
  uint8
    status, channel, note, velocity;
  Sample *sample;
  int32
    orig_frequency, frequency,
    sample_offset, sample_increment,
    envelope_volume, envelope_target, envelope_increment,
    tremolo_sweep, tremolo_sweep_position,
    tremolo_phase, tremolo_phase_increment,
    vibrato_sweep, vibrato_sweep_position;
  
  final_volume_t left_mix, right_mix;

  float
    left_amp, right_amp, tremolo_volume;
  int32
    vibrato_sample_increment[VIBRATO_SAMPLE_INCREMENTS];
  int
    vibrato_phase, vibrato_control_ratio, vibrato_control_counter,
    envelope_stage, control_counter, panning, panned;

} Voice;

/* Voice status options: */
#define VOICE_FREE 0
#define VOICE_ON 1
#define VOICE_SUSTAINED 2
#define VOICE_OFF 3
#define VOICE_DIE 4

/* Voice panned options: */
#define PANNED_MYSTERY 0
#define PANNED_LEFT 1
#define PANNED_RIGHT 2
#define PANNED_CENTER 3
/* Anything but PANNED_MYSTERY only uses the left volume */

extern Channel channel[16];
extern Voice voice[MAX_VOICES];

extern int32 control_ratio, amp_with_poly, amplification;
extern int32 drumchannels;
extern int adjust_panning_immediately;
extern int voices;
extern int32 sample_count;

#define ISDRUMCHANNEL(c) ((drumchannels & (1<<(c))))

extern int play_midi(MidiEvent *el, int32 events, int32 samples);
extern int play_midi_file(char *fn);
extern void dumb_pass_playing_list(int number_of_files, char *list_of_files[]);
