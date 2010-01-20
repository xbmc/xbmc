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

    mod2midi.c

    Mixer event -> MIDI event conversion
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
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "tables.h"
#include "mod.h"
#include "output.h"
#include "controls.h"
#include "unimod.h"
#include "mod2midi.h"
#include "filter.h"
#include "math.h"
#include "freq.h"


/* Define this to show all the notes touched by a bending in the
 * user interface's trace view.  This is interesting but disabled
 * because it needs tons of CPU power (tens of voices are activated
 * but unaudible). */
/* #define TRACE_SLIDE_NOTES */

/* Define this to give a volume envelope to a MOD's notes. This
 * could sound wrong with a few MODs, but gives richer sound most
 * of the time. */
#define USE_ENVELOPE


#define SETMIDIEVENT(e, at, t, ch, pa, pb) \
    { /* printf("%d %d " #t " %d %d\n", at, ch, pa, pb); */ \
      (e).time = (at); (e).type = (t); \
      (e).channel = (uint8)(ch); (e).a = (uint8)(pa); (e).b = (uint8)(pb); }

#define MIDIEVENT(at, t, ch, pa, pb) \
    { MidiEvent event; SETMIDIEVENT(event, at, t, ch, pa, pb); \
      readmidi_add_event(&event); }

/*
		   Clock
   SampleRate := ----------
		   Period
 */


#define NTSC_CLOCK 3579545.25
#define NTSC_RATE ((int32)(NTSC_CLOCK/428))

#define PAL_CLOCK 3546894.6
#define PAL_RATE ((int32)(PAL_CLOCK/428))

#define MOD_ROOT_NOTE      36

/* The internal bending register is 21-bits wide and it is made like this:
 *  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _
 * | | | | | | | | | | | | | | | | | | | | | |
 * |    8 bits     |    8 bits     |  5 bits |
 * |  note  shift  |   fine tune   |discarded|
 * '---------------'---------------'---------'
 *
 * The note shift is an `offset' field: 128 = keep this note.
 * To compute it, the values given to the pitch-wheel MIDI event are
 * multiplied by the sensitivity. We want to be able to express a full
 * 120 notes bending (8 bits for the note shift + 8 for the fine tune)
 * with 14-bit pitch-wheel event, so we want a sensitivity value that
 * forces the bottom 7 bits of the internal register to 0.  This value
 * is of course 128.
 */

#define WHEEL_SENSITIVITY 		(1 << 7)
#define WHEEL_VALUE(bend)		((bend) / WHEEL_SENSITIVITY + 0x2000)


typedef struct _ModVoice
  {
    int sample;			/* current sample ID */
    int noteon;			/* (-1 means OFF status) */
    int time;			/* time when note was activated */
    int period;			/* current frequency */
    int wheel;			/* current pitch wheel value */
    int pan;			/* current panning */
    int vol;			/* current volume */

    int32 noteson[4];		/* bit map for notes 0-127 */
  }
ModVoice;

static void mod_change_tempo (int32 at, int bpm);
static int period2note (int period, int *finetune);

static ModVoice ModV[MOD_NUM_VOICES];
static int at;

/******************** bitmap handling macros **********************************/

#define bitmapGet(map, n)	((map)[(n) >> 5] &  (1 << ((n) & 31)))
#define bitmapSet(map, n)	((map)[(n) >> 5] |= (1 << ((n) & 31)))
#define bitmapClear(map)	((map)[0] = (map)[1] = (map)[2] = (map)[3] = 0)

static char significantDigitsLessOne[256] = {
    -1,							/* 1 */
    0,							/* 2 */
    1, 1,						/* 4 */
    2, 2, 2, 2,						/* 8 */
    3, 3, 3, 3, 3, 3, 3, 3,				/* 16 */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	/* 32 */

    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	/* 64 */
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,

    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,	/* 128 */
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,

    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	/* 256 */
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

/******************************************************************************/

void
mod_change_tempo (int32 at, int bpm)
{
  int32 tempo;
  int c, a, b;

  tempo = 60000000 / bpm;
  c = (tempo & 0xff);
  a = ((tempo >> 8) & 0xff);
  b = ((tempo >> 16) & 0xff);
  MIDIEVENT (at, ME_TEMPO, c, b, a);
}

int
period2note (int period, int *finetune)
{
  static int period_table[121] =
  {
  /*  C     C#    D     D#    E     F     F#    G     G#    A     A#    B  */
     13696,12928,12192,11520,10848,10240, 9664, 9120, 8608, 8096, 7680, 7248,
      6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4048, 3840, 3624,
      3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2024, 1920, 1812,
      1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  906,
       856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
       428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,
       214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,
       107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56,
	53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28,
	27,   25,   24,   22,   21,   20,   19,   18,   17,   16,   15,   14,
	  
	-100 /* just a guard */
  };

  int note;
  int l, r, m;

  if (period < 14 || period > 13696)
  {
    ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "BAD period %d\n", period);
    return -1;
  }

  /* bin search */
  l = 0;
  r = 120;
  while (l < r)
    {
      m = (l + r) / 2;
      if (period_table[m] >= period)
	l = m + 1;
      else
	r = m;
    }
  note = l - 1;

  /*
   * 112 >= note >= 0
   * period_table[note] >= period > period_table[note + 1]
   */

  if (period_table[note] == period)
    *finetune = 0;
  else {
    /* Pick the closest note even if it is higher than this one.
     * (e.g. 721 - 720 < 762 - 721 ----> pick 720)    */
    if (period - period_table[note + 1] < period_table[note] - period)
      note++;
    
    /* fine tune completion */
    *finetune = ((period_table[note] - period) << 8) /
		   (period_table[note] - period_table[note + 1]);

    *finetune <<= 5;
  }
  return note;
}

/********** Interface to mod.c */

void
Voice_SetVolume (UBYTE v, UWORD vol)
{
  if (v >= MOD_NUM_VOICES)
    return;

  /* MOD volume --> MIDI volume */
  vol >>= 1;
  /* if (vol < 0) vol = 0; *//* UNSIGNED! */
  if (vol > 127) vol = 127;

  if (ModV[v].vol != vol) {
    ModV[v].vol = vol;
    MIDIEVENT (at, ME_EXPRESSION, v, vol, 0);
  }
}

void
Voice_SetPeriod (UBYTE v, ULONG period)
{
  int new_noteon, bend;

  if (v >= MOD_NUM_VOICES)
    return;

  ModV[v].period = period;
  if (ModV[v].noteon < 0)
    return;

  new_noteon = period2note (ModV[v].period, &bend);
#ifndef TRACE_SLIDE_NOTES
  bend += (new_noteon - ModV[v].noteon) << 13;
  new_noteon = ModV[v].noteon;
#endif
  bend = WHEEL_VALUE(bend);

  if (ModV[v].noteon != new_noteon)
    {
      MIDIEVENT(at, ME_KEYPRESSURE, v, ModV[v].noteon, 1);

      if (new_noteon < 0)
        {
	  ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Strange period %d",
			  ModV[v].period);
	  return;
	}
      else if (!bitmapGet(ModV[v].noteson, new_noteon))
	{
	  MIDIEVENT(ModV[v].time, ME_NOTEON, v, new_noteon, 1);
	  bitmapSet(ModV[v].noteson, new_noteon);
	}

    }

  if (ModV[v].wheel != bend)
    {
      ModV[v].wheel = bend;
      MIDIEVENT (at, ME_PITCHWHEEL, v, bend & 0x7F, (bend >> 7) & 0x7F);
    }

  if (ModV[v].noteon != new_noteon)
    {
      MIDIEVENT(at, ME_KEYPRESSURE, v, new_noteon, 127);
      ModV[v].noteon = new_noteon;
    }

}

void
Voice_SetPanning (UBYTE v, ULONG pan)
{
  if (v >= MOD_NUM_VOICES)
    return;
  if (pan == PAN_SURROUND)
    pan = PAN_CENTER; /* :-( */

  if (pan != ModV[v].pan) {
    ModV[v].pan = pan;
    MIDIEVENT(at, ME_PAN, v, pan * 127 / PAN_RIGHT, 0);
  }
}

void
Voice_Play (UBYTE v, SAMPLE * s, ULONG start)
{
  int new_noteon, bend;
  if (v >= MOD_NUM_VOICES)
    return;

  if (ModV[v].noteon != -1)
    Voice_Stop (v);

  new_noteon = period2note (ModV[v].period, &bend);
  bend = WHEEL_VALUE(bend);
  if (new_noteon < 0) {
    ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
			  "Strange period %d",
			  ModV[v].period);
    return;
  }

  ModV[v].noteon = new_noteon;
  ModV[v].time = at;
  bitmapSet(ModV[v].noteson, new_noteon);

  if (ModV[v].sample != s->id)
    {
      ModV[v].sample = s->id;
      MIDIEVENT(at, ME_SET_PATCH, v, ModV[v].sample, 0);
    }

  if (start > 0)
    {
      int a, b;
      a = (start & 0xff);
      b = ((start >> 8) & 0xff);
      MIDIEVENT (at, ME_PATCH_OFFS, v, a, b);
    }

  if (ModV[v].wheel != bend)
    {
      ModV[v].wheel = bend;
      MIDIEVENT (at, ME_PITCHWHEEL, v, bend & 0x7F, (bend >> 7) & 0x7F);
    }
 
  MIDIEVENT (at, ME_NOTEON, v, ModV[v].noteon, 127);
}

void
Voice_Stop (UBYTE v)
{
  int32 j;
  int n;

  if (v >= MOD_NUM_VOICES)
    return;

  if (ModV[v].noteon == -1)
    return;

#define TURN_OFF_8(base, ofs) 						\
  while (j & (0xFFL << (ofs))) {					\
    n = ofs + significantDigitsLessOne[(unsigned char) (j >> (ofs))];	\
    MIDIEVENT (at, ME_NOTEOFF, v, (base) + n, 63);			\
    j ^= 1 << n;							\
  }

#define TURN_OFF_32(base)						\
  do {									\
    TURN_OFF_8((base), 24)						\
    TURN_OFF_8((base), 16)						\
    TURN_OFF_8((base), 8)						\
    TURN_OFF_8((base), 0)						\
  } while(0)

  if ((j = ModV[v].noteson[0]) != 0) TURN_OFF_32(0);
  if ((j = ModV[v].noteson[1]) != 0) TURN_OFF_32(32);
  if ((j = ModV[v].noteson[2]) != 0) TURN_OFF_32(64);
  if ((j = ModV[v].noteson[3]) != 0) TURN_OFF_32(96);
  bitmapClear(ModV[v].noteson);
  ModV[v].noteon = -1;
}

BOOL
Voice_Stopped (UBYTE v)
{
  return (v >= MOD_NUM_VOICES) || (ModV[v].noteon == -1);
}

void
Voice_TickDone ()
{
  at++;
}

void
Voice_NewTempo (UWORD bpm, UWORD sngspd)
{
  mod_change_tempo(at, bpm);
}

void
Voice_EndPlaying ()
{
  int v;

  at += 48 / (60.0/125.0);		/* 1 second */
  for(v = 0; v < MOD_NUM_VOICES; v++)
    MIDIEVENT(at, ME_ALL_NOTES_OFF, v, 0, 0);
}

void
Voice_StartPlaying ()
{
  int v;

  readmidi_set_track(0, 1);

  current_file_info->divisions = 24;

  for(v = 0; v < MOD_NUM_VOICES; v++)
    {
	ModV[v].sample = -1;
	ModV[v].noteon = -1;
	ModV[v].time = -1;
	ModV[v].period = 0;
	ModV[v].wheel = 0x2000;
	ModV[v].vol = 127;
	ModV[v].pan = (v & 1) ? 127 : 0;
	bitmapClear(ModV[v].noteson);

	MIDIEVENT(0, ME_PAN, v, ModV[v].pan, 0);
        MIDIEVENT(0, ME_SET_PATCH, v, 1, 0);
        MIDIEVENT(0, ME_MAINVOLUME, v, 127, 0);
        MIDIEVENT(0, ME_EXPRESSION, v, 127, 0);
	/* MIDIEVENT(0, ME_MONO, v, 0, 0); */
	MIDIEVENT(0, ME_RPN_LSB, v, 0, 0);
	MIDIEVENT(0, ME_RPN_MSB, v, 0, 0);
	MIDIEVENT(0, ME_DATA_ENTRY_MSB, v, WHEEL_SENSITIVITY, 0);
	MIDIEVENT(0, ME_DRUMPART, v, 0, 0);
    }

  at = 1;
}

/* convert from 8bit value to fractional offset (15.15) */
static int32 env_offset(int offset)
{
    return (int32)offset << (7+15);
}

/* calculate ramp rate in fractional unit;
 * diff = 8bit, time = msec
 */
static int32 env_rate(int diff, double msec)
{
    double rate;

    if(msec < 6)
	msec = 6;
    if(diff == 0)
	diff = 255;
    diff <<= (7+15);
    rate = ((double)diff / play_mode->rate) * control_ratio * 1000.0 / msec;
    if(fast_decay)
	rate *= 2;
    return (int32)rate;
}

void shrink_huge_sample (Sample *sp)
{
    sample_t *orig_data;
    sample_t *new_data;
    unsigned int rate, new_rate;
    uint32 data_length, new_data_length;
    double loop_start, loop_end;
    double scale, scale2;
    double x, xfrac;
    double y;
    sample_t y1, y2, y3, y4;
    uint32 i, xtrunc;

    data_length = sp->data_length;
    if (data_length < (1 << FRACTION_BITS) - 1)
	return;
    loop_start = sp->loop_start;
    loop_end = sp->loop_end;
    rate = sp->sample_rate;

    scale = ((1 << (31 - FRACTION_BITS)) - 2.0) / data_length;
    new_rate = rate * scale;
    scale = new_rate / (float) rate;
    scale2 = (float) rate / new_rate;

    new_data_length = data_length * scale;
    loop_start *= scale;
    loop_end *= scale;

    ctl->cmsg(CMSG_INFO, VERB_NORMAL,
        "Sample too large (%ld): resampling down to %ld samples",
        data_length, new_data_length);
    
    orig_data = sp->data;
    new_data = calloc(new_data_length + 1, sizeof(sample_t));

    new_data[0] = orig_data[0];
    for (i = 1; i < new_data_length; i++)
    {
	x = i * scale2;
	xtrunc = (uint32) x;
	xfrac = x - xtrunc;
	
	if (xtrunc >= data_length - 1)
	{
	    if (xtrunc == data_length)
	    	new_data[i] = orig_data[data_length];
	    else	/* linear interpolation */
	    {
	        y2 = orig_data[data_length - 1];
	        y3 = orig_data[data_length];
	        new_data[i] = ceil((y2 + (y3 - y2) * xfrac) - 0.5);
	    }
	}
	else		/* cspline interpolation */
	{
	    y1 = orig_data[xtrunc - 1];
	    y2 = orig_data[xtrunc];
	    y3 = orig_data[xtrunc + 1];
	    y4 = orig_data[xtrunc + 2];

	    y = (6*y3 + (5*y4 - 11*y3 + 7*y2 - y1) *
		0.25 * (xfrac+1) * (xfrac-1)) * xfrac;
	    y = (((6*y2 + (5*y1 - 11*y2 + 7*y3 - y4) * 
		0.25 * xfrac * (xfrac-2)) * (1-xfrac)) + y) / 6.0;

	    if (y > 32767) y = 32767;
	    if (y < -32767) y = -32767;

	    new_data[i] = ceil(y - 0.5);
	}
    }

    free(sp->data);
    sp->data = new_data;
    sp->sample_rate = new_rate;

    sp->data_length = new_data_length << FRACTION_BITS;
    sp->loop_start = loop_start * (1 << FRACTION_BITS);
    sp->loop_end = loop_end * (1 << FRACTION_BITS);
}

void load_module_samples (SAMPLE * s, int numsamples, int ntsc)
{
    int i;

    for(i = 1; numsamples--; i++, s++)
    {
	Sample *sp;
	char name[23];

	if(!s->data)
	    continue;

	ctl->cmsg(CMSG_INFO, VERB_DEBUG,
		  "MOD Sample %d (%.22s)", i, s->samplename);

	special_patch[i] =
	    (SpecialPatch *)safe_malloc(sizeof(SpecialPatch));
	special_patch[i]->type = INST_MOD;
	special_patch[i]->samples = 1;
	special_patch[i]->sample = sp =
	    (Sample *)safe_malloc(sizeof(Sample));
	memset(sp, 0, sizeof(Sample));
	strncpy(name, s->samplename, 22);
	name[22] = '\0';
	code_convert(name, NULL, 23, NULL, "ASCII");
	if(name[0] == '\0')
	    special_patch[i]->name = NULL;
	else
	    special_patch[i]->name = safe_strdup(name);
	special_patch[i]->sample_offset = 0;

	sp->data = (sample_t *)s->data;
	sp->data_alloced = 1;
	sp->data_length = s->length;
	sp->loop_start = s->loopstart;
	sp->loop_end   = s->loopend;

        /* The sample must be padded out by 1 extra sample, so that
           the interpolation routines won't cause a "pop" by reading
           random data beyond data_length */
	sp->data = (sample_t *) realloc(sp->data,
					(sp->data_length + 1) *
					sizeof(sample_t));
        sp->data[sp->data_length] = 0;

	/* Stereo instruments (SF_STEREO) are dithered by libunimod into mono */
	sp->modes = MODES_UNSIGNED;
	if (s->flags & SF_SIGNED)  sp->modes ^= MODES_UNSIGNED;
	if (s->flags & SF_LOOP)    sp->modes ^= MODES_LOOPING;
	if (s->flags & SF_BIDI)    sp->modes ^= MODES_PINGPONG;
	if (s->flags & SF_REVERSE) sp->modes ^= MODES_REVERSE;
	if (s->flags & SF_16BITS)  sp->modes ^= MODES_16BIT;

#ifdef USE_ENVELOPE
	/* envelope (0,1:attack, 2:sustain, 3,4,5:release) */
	sp->modes |= MODES_ENVELOPE;

	/* attack */
	sp->envelope_offset[0] = env_offset(255);
	sp->envelope_rate[0]   = env_rate(255, 0.0);	/* fastest */
	sp->envelope_offset[1] = sp->envelope_offset[0];
	sp->envelope_rate[1]   = 0; /* skip this stage */
	/* sustain */
	sp->envelope_offset[2] = sp->envelope_offset[1];
	sp->envelope_rate[2]   = 0;
	/* release */
	sp->envelope_offset[3] = env_offset(0);
	sp->envelope_rate[3]   = env_rate(255, 80.0);	/* 80 msec */
	sp->envelope_offset[4] = sp->envelope_offset[3];
	sp->envelope_rate[4]   = 0; /* skip this stage */
	sp->envelope_offset[5] = sp->envelope_offset[4];
	sp->envelope_rate[5]   = 0; /* skip this stage, then the voice is
				       disappeared */
#endif
 	sp->sample_rate = PAL_RATE >> s->divfactor;
	sp->low_freq = 0;
	sp->high_freq = 0x7fffffff;
	sp->root_freq = freq_table[MOD_ROOT_NOTE];
	sp->volume = 1.0;		/* I guess it should use globvol... */
	sp->panning = s->panning == PAN_SURROUND ? 64 : s->panning * 128 / 255;
	sp->note_to_use = 0;
	sp->low_vel = 0;
	sp->high_vel = 127;
	sp->tremolo_sweep_increment =
		sp->tremolo_phase_increment = sp->tremolo_depth =
		sp->vibrato_sweep_increment = sp->vibrato_control_ratio = sp->vibrato_depth = 0;
	sp->cutoff_freq = sp->resonance = sp->tremolo_to_pitch = 
		sp->tremolo_to_fc = sp->modenv_to_pitch = sp->modenv_to_fc =
		sp->vel_to_fc = sp->key_to_fc = sp->vel_to_resonance = 0;
	sp->envelope_velf_bpo = sp->modenv_velf_bpo =
		sp->vel_to_fc_threshold = 64;
	sp->key_to_fc_bpo = 60;
	sp->scale_freq = 60;
	sp->scale_factor = 1024;
	memset(sp->envelope_velf, 0, sizeof(sp->envelope_velf));
	memset(sp->envelope_keyf, 0, sizeof(sp->envelope_keyf));
	memset(sp->modenv_velf, 0, sizeof(sp->modenv_velf));
	memset(sp->modenv_keyf, 0, sizeof(sp->modenv_keyf));
	memset(sp->modenv_rate, 0, sizeof(sp->modenv_rate));
	memset(sp->modenv_offset, 0, sizeof(sp->modenv_offset));
	sp->envelope_delay = sp->modenv_delay =
		sp->tremolo_delay = sp->vibrato_delay = 0;
	sp->sample_type = SF_SAMPLETYPE_MONO;
	sp->sf_sample_link = -1;
	sp->sf_sample_index = 0;

	if (sp->data_length >= (1 << (31 - FRACTION_BITS)) - 1)
	    shrink_huge_sample(sp);
	else
	{
	    sp->data_length <<= FRACTION_BITS;
	    sp->loop_start <<= FRACTION_BITS;
	    sp->loop_end <<= FRACTION_BITS;
	}

	/* pitch detection for mod->midi file conversion and surround chorus */
	if (play_mode->id_character == 'M' ||
	    opt_surround_chorus)
	{
	    sp->chord = -1;
	    sp->root_freq_detected = freq_fourier(sp, &(sp->chord));
	    sp->transpose_detected =
		assign_pitch_to_freq(sp->root_freq_detected) -
		assign_pitch_to_freq(sp->root_freq / 1024.0);
	}

	/* If necessary do some anti-aliasing filtering  */
	if (antialiasing_allowed)
	  antialiasing((int16 *)sp->data, sp->data_length / 2,
		       sp->sample_rate, play_mode->rate);

	s->data = NULL;		/* Avoid free-ing */
	s->id = i;
    }
}
