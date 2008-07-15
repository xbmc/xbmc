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

   instrum.h

   */

typedef struct {
  int32
    loop_start, loop_end, data_length,
    sample_rate, low_freq, high_freq, root_freq;
  int32
    envelope_rate[6], envelope_offset[6];
  float
    volume;
  sample_t *data;
  int32 
    tremolo_sweep_increment, tremolo_phase_increment, 
    vibrato_sweep_increment, vibrato_control_ratio;
  uint8
    tremolo_depth, vibrato_depth,
    modes;
  int8
    panning, note_to_use;
} Sample;

/* Bits in modes: */
#define MODES_16BIT	(1<<0)
#define MODES_UNSIGNED	(1<<1)
#define MODES_LOOPING	(1<<2)
#define MODES_PINGPONG	(1<<3)
#define MODES_REVERSE	(1<<4)
#define MODES_SUSTAIN	(1<<5)
#define MODES_ENVELOPE	(1<<6)

typedef struct {
  int samples;
  Sample *sample;
} Instrument;

typedef struct {
  char *name;
  Instrument *instrument;
  int note, amp, pan, strip_loop, strip_envelope, strip_tail;
} ToneBankElement;

/* A hack to delay instrument loading until after reading the
   entire MIDI file. */
#define MAGIC_LOAD_INSTRUMENT ((Instrument *)(-1))

typedef struct {
  ToneBankElement tone[128];
} ToneBank;

extern ToneBank *tonebank[], *drumset[];

extern Instrument *default_instrument;
extern int default_program;
extern int antialiasing_allowed;
extern int fast_decay;
extern int free_instruments_afterwards;

#define SPECIAL_PROGRAM -1

extern int load_missing_instruments(void);
extern void free_instruments(void);
extern int set_default_instrument(char *name);

