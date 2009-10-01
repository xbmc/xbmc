/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * internal/it.h - Internal stuff for IT playback     / / \  \
 *                 and MOD/XM/S3M conversion.        | <  /   \_
 *                                                   |  \/ /\   /
 * This header file provides access to the            \_  /  > /
 * internal structure of DUMB, and is liable            | \ / /
 * to change, mutate or cease to exist at any           |  ' /
 * moment. Include it at your own peril.                 \__/
 *
 * ...
 *
 * Seriously. You don't need access to anything in this file. All right, you
 * probably do actually. But if you use it, you will be relying on a specific
 * version of DUMB, so please check DUMB_VERSION defined in dumb.h. Please
 * contact the authors so that we can provide a public API for what you need.
 */

#ifndef INTERNAL_IT_H
#define INTERNAL_IT_H



#include <stddef.h>



/** TO DO: THINK ABOUT THE FOLLOWING:

sigdata->flags & IT_COMPATIBLE_GXX

                Bit 5: On = Link Effect G's memory with Effect E/F. Also
                            Gxx with an instrument present will cause the
                            envelopes to be retriggered. If you change a
                            sample on a row with Gxx, it'll adjust the
                            frequency of the current note according to:

                              NewFrequency = OldFrequency * NewC5 / OldC5;
*/



/* These #defines are TEMPORARY. They are used to write alternative code to
 * handle ambiguities in the format specification. The correct code in each
 * case will be determined most likely by experimentation.
 */
#define STEREO_SAMPLES_COUNT_AS_TWO
#define INVALID_ORDERS_END_SONG
#define INVALID_NOTES_CAUSE_NOTE_CUT
#define SUSTAIN_LOOP_OVERRIDES_NORMAL_LOOP
#define VOLUME_OUT_OF_RANGE_SETS_MAXIMUM



#define SIGTYPE_IT DUMB_ID('I', 'T', ' ', ' ')

#define IT_SIGNATURE            DUMB_ID('I', 'M', 'P', 'M')
#define IT_INSTRUMENT_SIGNATURE DUMB_ID('I', 'M', 'P', 'I')
#define IT_SAMPLE_SIGNATURE     DUMB_ID('I', 'M', 'P', 'S')



/* 1 minute per 4 rows, each row 6 ticks; this is divided by the tempo to get
 * the interval between ticks.
 */
#define TICK_TIME_DIVIDEND ((65536 * 60) / (4 * 6))



/* I'm not going to try to explain this, because I didn't derive it very
 * formally ;)
 */
/* #define AMIGA_DIVISOR ((float)(4.0 * 14317056.0)) */
/* I believe the following one to be more accurate. */
#define AMIGA_DIVISOR ((float)(8.0 * 7159090.5))



typedef struct IT_MIDI IT_MIDI;
typedef struct IT_FILTER_STATE IT_FILTER_STATE;
typedef struct IT_ENVELOPE IT_ENVELOPE;
typedef struct IT_INSTRUMENT IT_INSTRUMENT;
typedef struct IT_SAMPLE IT_SAMPLE;
typedef struct IT_ENTRY IT_ENTRY;
typedef struct IT_PATTERN IT_PATTERN;
typedef struct IT_PLAYING_ENVELOPE IT_PLAYING_ENVELOPE;
typedef struct IT_PLAYING IT_PLAYING;
typedef struct IT_CHANNEL IT_CHANNEL;
typedef struct IT_CHECKPOINT IT_CHECKPOINT;
typedef struct IT_CALLBACKS IT_CALLBACKS;



struct IT_MIDI
{
	unsigned char SFmacro[16][16]; // read these from 0x120
	unsigned char SFmacrolen[16];
	unsigned short SFmacroz[16]; /* Bitfield; bit 0 set = z in first position */
	unsigned char Zmacro[128][16]; // read these from 0x320
	unsigned char Zmacrolen[128];
};



struct IT_FILTER_STATE
{
	sample_t currsample, prevsample;
};



#define IT_ENVELOPE_ON                1
#define IT_ENVELOPE_LOOP_ON           2
#define IT_ENVELOPE_SUSTAIN_LOOP      4
#define IT_ENVELOPE_PITCH_IS_FILTER 128

struct IT_ENVELOPE
{
	unsigned char flags;
	unsigned char n_nodes;
	unsigned char loop_start;
	unsigned char loop_end;
	unsigned char sus_loop_start;
	unsigned char sus_loop_end;
	signed char node_y[25];
	unsigned short node_t[25];
};



#define NNA_NOTE_CUT      0
#define NNA_NOTE_CONTINUE 1
#define NNA_NOTE_OFF      2
#define NNA_NOTE_FADE     3

#define DCT_OFF        0
#define DCT_NOTE       1
#define DCT_SAMPLE     2
#define DCT_INSTRUMENT 3

#define DCA_NOTE_CUT  0
#define DCA_NOTE_OFF  1
#define DCA_NOTE_FADE 2

struct IT_INSTRUMENT
{
	unsigned char name[27];
	unsigned char filename[14];

	int fadeout;

	IT_ENVELOPE volume_envelope;
	IT_ENVELOPE pan_envelope;
	IT_ENVELOPE pitch_envelope;

	unsigned char new_note_action;
	unsigned char dup_check_type;
	unsigned char dup_check_action;
	unsigned char pp_separation;
	unsigned char pp_centre;
	unsigned char global_volume;
	unsigned char default_pan;
	unsigned char random_volume;
	unsigned char random_pan;

	unsigned char filter_cutoff;
	unsigned char filter_resonance;

	unsigned char map_note[120];
	unsigned short map_sample[120];
};



#define IT_SAMPLE_EXISTS              1
#define IT_SAMPLE_16BIT               2
#define IT_SAMPLE_STEREO              4
#define IT_SAMPLE_LOOP               16
#define IT_SAMPLE_SUS_LOOP           32
#define IT_SAMPLE_PINGPONG_LOOP      64
#define IT_SAMPLE_PINGPONG_SUS_LOOP 128

#define IT_VIBRATO_SINE     0
#define IT_VIBRATO_SAWTOOTH 1 /* Ramp down */
#define IT_VIBRATO_SQUARE   2
#define IT_VIBRATO_RANDOM   3

struct IT_SAMPLE
{
	unsigned char name[29];
	unsigned char filename[14];
	unsigned char flags;
	unsigned char global_volume;
	unsigned char default_volume;
	unsigned char default_pan;
	/* default_pan:
	 *   0-255 for XM
	 *   ignored for MOD
	 *   otherwise, 0-64, and add 128 to enable
	 */

	long length;
	long loop_start;
	long loop_end;
	long C5_speed;
	long sus_loop_start;
	long sus_loop_end;

	unsigned char vibrato_speed;
	unsigned char vibrato_depth;
	unsigned char vibrato_rate;
	unsigned char vibrato_waveform;

	void *data;
};



#define IT_ENTRY_NOTE       1
#define IT_ENTRY_INSTRUMENT 2
#define IT_ENTRY_VOLPAN     4
#define IT_ENTRY_EFFECT     8

#define IT_SET_END_ROW(entry) ((entry)->channel = 255)
#define IT_IS_END_ROW(entry) ((entry)->channel >= DUMB_IT_N_CHANNELS)

#define IT_NOTE_OFF 255
#define IT_NOTE_CUT 254

#define IT_ENVELOPE_SHIFT 8

#define IT_SURROUND 100
#define IT_IS_SURROUND(pan) ((pan) > 64)
#define IT_IS_SURROUND_SHIFTED(pan) ((pan) > 64 << IT_ENVELOPE_SHIFT)

#define IT_SET_SPEED              1
#define IT_JUMP_TO_ORDER          2
#define IT_BREAK_TO_ROW           3
#define IT_VOLUME_SLIDE           4
#define IT_PORTAMENTO_DOWN        5
#define IT_PORTAMENTO_UP          6
#define IT_TONE_PORTAMENTO        7
#define IT_VIBRATO                8
#define IT_TREMOR                 9
#define IT_ARPEGGIO              10
#define IT_VOLSLIDE_VIBRATO      11
#define IT_VOLSLIDE_TONEPORTA    12
#define IT_SET_CHANNEL_VOLUME    13
#define IT_CHANNEL_VOLUME_SLIDE  14
#define IT_SET_SAMPLE_OFFSET     15
#define IT_PANNING_SLIDE         16
#define IT_RETRIGGER_NOTE        17
#define IT_TREMOLO               18
#define IT_S                     19
#define IT_SET_SONG_TEMPO        20
#define IT_FINE_VIBRATO          21
#define IT_SET_GLOBAL_VOLUME     22
#define IT_GLOBAL_VOLUME_SLIDE   23
#define IT_SET_PANNING           24
#define IT_PANBRELLO             25
#define IT_MIDI_MACRO            26 //see MIDI.TXT

/* Some effects needed for XM compatibility */
#define IT_XM_PORTAMENTO_DOWN       27
#define IT_XM_PORTAMENTO_UP         28
#define IT_XM_FINE_VOLSLIDE_DOWN    29
#define IT_XM_FINE_VOLSLIDE_UP      30
#define IT_XM_RETRIGGER_NOTE        31
#define IT_XM_KEY_OFF               32
#define IT_XM_SET_ENVELOPE_POSITION 33

#define IT_N_EFFECTS                34

/* These represent the top nibble of the command value. */
#define IT_S_SET_FILTER              0 /* Greyed out in IT... */
#define IT_S_SET_GLISSANDO_CONTROL   1 /* Greyed out in IT... */
#define IT_S_FINETUNE                2 /* Greyed out in IT... */
#define IT_S_SET_VIBRATO_WAVEFORM    3
#define IT_S_SET_TREMOLO_WAVEFORM    4
#define IT_S_SET_PANBRELLO_WAVEFORM  5
#define IT_S_FINE_PATTERN_DELAY      6
#define IT_S7                        7
#define IT_S_SET_PAN                 8
#define IT_S_SET_SURROUND_SOUND      9
#define IT_S_SET_HIGH_OFFSET        10
#define IT_S_PATTERN_LOOP           11
#define IT_S_DELAYED_NOTE_CUT       12
#define IT_S_NOTE_DELAY             13
#define IT_S_PATTERN_DELAY          14
#define IT_S_SET_MIDI_MACRO         15

/*
S0x Set filter
S1x Set glissando control
S2x Set finetune


S3x Set vibrato waveform to type x
S4x Set tremelo waveform to type x
S5x Set panbrello waveform to type x
  Waveforms for commands S3x, S4x and S5x:
    0: Sine wave
    1: Ramp down
    2: Square wave
    3: Random wave
S6x Pattern delay for x ticks
S70 Past note cut
S71 Past note off
S72 Past note fade
S73 Set NNA to note cut
S74 Set NNA to continue
S75 Set NNA to note off
S76 Set NNA to note fade
S77 Turn off volume envelope
S78 Turn on volume envelope
S79 Turn off panning envelope
S7A Turn on panning envelope
S7B Turn off pitch envelope
S7C Turn on pitch envelope
S8x Set panning position
S91 Set surround sound
SAy Set high value of sample offset yxx00h
SB0 Set loopback point
SBx Loop x times to loopback point
SCx Note cut after x ticks
SDx Note delay for x ticks
SEx Pattern delay for x rows
SFx Set parameterised MIDI Macro
*/

struct IT_ENTRY
{
	unsigned char channel; /* End of row if channel >= DUMB_IT_N_CHANNELS */
	unsigned char mask;
	unsigned char note;
	unsigned char instrument;
	unsigned char volpan;
	unsigned char effect;
	unsigned char effectvalue;
};



struct IT_PATTERN
{
	int n_rows;
	int n_entries;
	IT_ENTRY *entry;
};



#define IT_STEREO            1
#define IT_USE_INSTRUMENTS   4
#define IT_LINEAR_SLIDES     8 /* If not set, use Amiga slides */
#define IT_OLD_EFFECTS      16
#define IT_COMPATIBLE_GXX   32

/* Make sure IT_WAS_AN_XM and IT_WAS_A_MOD aren't set accidentally */
#define IT_REAL_FLAGS       63

#define IT_WAS_AN_XM        64 /* Set for both XMs and MODs */
#define IT_WAS_A_MOD       128

#define IT_ORDER_END  255
#define IT_ORDER_SKIP 254

struct DUMB_IT_SIGDATA
{
	unsigned char name[29];

	unsigned char *song_message;

	int n_orders;
	int n_instruments;
	int n_samples;
	int n_patterns;

	int flags;

	int global_volume;
	int mixing_volume;
	int speed;
	int tempo;
	int pan_separation;

	unsigned char channel_pan[DUMB_IT_N_CHANNELS];
	unsigned char channel_volume[DUMB_IT_N_CHANNELS];

	unsigned char *order;
	unsigned char restart_position; /* for XM compatiblity */

	IT_INSTRUMENT *instrument;
	IT_SAMPLE *sample;
	IT_PATTERN *pattern;

	IT_MIDI *midi;

	IT_CHECKPOINT *checkpoint;
};



struct IT_PLAYING_ENVELOPE
{
	int next_node;
	int tick;
	int value;
};



#define IT_PLAYING_BACKGROUND 1
#define IT_PLAYING_SUSTAINOFF 2
#define IT_PLAYING_FADING     4
#define IT_PLAYING_DEAD       8

struct IT_PLAYING
{
	int flags;

	IT_CHANNEL *channel;
	IT_SAMPLE *sample;
	IT_INSTRUMENT *instrument;
	IT_INSTRUMENT *env_instrument;

	unsigned short sampnum;
	unsigned char instnum;

	unsigned char channel_volume;

	unsigned char volume;
	unsigned short pan;

	unsigned char note;

	unsigned char filter_cutoff;
	unsigned char filter_resonance;

	unsigned short true_filter_cutoff;   /* These incorporate the filter envelope, and will not */
	unsigned char true_filter_resonance; /* be changed if they would be set to 127<<8 and 0.    */

	unsigned char vibrato_speed;
	unsigned char vibrato_depth;
	unsigned char vibrato_n; /* May be specified twice: volpan & effect. */
	unsigned char vibrato_time;

	unsigned char tremolo_speed;
	unsigned char tremolo_depth;
	unsigned char tremolo_time;

	unsigned char sample_vibrato_time;
	int sample_vibrato_depth; /* Starts at rate?0:depth, increases by rate */

	int slide;
	float delta;

	IT_PLAYING_ENVELOPE volume_envelope;
	IT_PLAYING_ENVELOPE pan_envelope;
	IT_PLAYING_ENVELOPE pitch_envelope;

	int fadeoutcount;

	IT_FILTER_STATE filter_state[2]; /* Left and right */

	DUMB_RESAMPLER resampler;

	/* time_lost is used to emulate Impulse Tracker's sample looping
	 * characteristics. When time_lost is added to pos, the result represents
	 * the position in the theoretical version of the sample where all loops
	 * have been expanded. If this is stored, the resampling helpers will
	 * safely convert it for use with new loop boundaries. The situation is
	 * slightly more complicated if dir == -1 when the change takes place; we
	 * must reflect pos off the loop end point and set dir to 1 before
	 * proceeding.
	 */
	long time_lost;
};



#define IT_CHANNEL_MUTED 1

struct IT_CHANNEL
{
	int flags;

	unsigned char volume;
	signed char volslide;
	signed char xm_volslide;
	signed char panslide;

	/* xm_volslide is used for volume slides done in the volume column in an
	 * XM file, since it seems the volume column slide is applied first,
	 * followed by clamping, followed by the effects column slide. IT does
	 * not exhibit this behaviour, so xm_volslide is maintained at zero.
	 */

	unsigned char pan;
	unsigned short truepan;

	unsigned char channelvolume;
	signed char channelvolslide;

	unsigned char instrument;
	unsigned char note;

	unsigned char SFmacro;

	unsigned char filter_cutoff;
	unsigned char filter_resonance;

	unsigned char key_off_count;
	unsigned char note_cut_count;
	unsigned char note_delay_count;
	IT_ENTRY *note_delay_entry;

	int arpeggio;
	unsigned char retrig;
	unsigned char xm_retrig;
	int retrig_tick;

	unsigned char tremor;
	unsigned char tremor_time; /* Bit 6 set if note on; bit 7 set if tremor active. */

	int portamento;
	int toneporta;
	unsigned char destnote;

	/** WARNING - for neatness, should one or both of these be in the IT_PLAYING struct? */
	unsigned short sample;
	unsigned char truenote;

	unsigned char midi_state;

	signed char lastvolslide;
	unsigned char lastDKL;
	unsigned char lastEF; /* Doubles as last portamento up for XM files */
	unsigned char lastG;
	unsigned char lastHspeed;
	unsigned char lastHdepth;
	unsigned char lastRspeed;
	unsigned char lastRdepth;
	unsigned char lastI;
	unsigned char lastJ; /* Doubles as last portamento down for XM files */
	unsigned char lastN;
	unsigned char lastO;
	unsigned char high_offset;
	unsigned char lastP;
	unsigned char lastQ;
	unsigned char lastS;
	unsigned char pat_loop_row;
	unsigned char pat_loop_count;
	unsigned char pat_loop_end_row; /* Used to catch infinite pattern loops */
	unsigned char lastW;

	unsigned char xm_lastE1;
	unsigned char xm_lastE2;
	unsigned char xm_lastEA;
	unsigned char xm_lastEB;
	unsigned char xm_lastX1;
	unsigned char xm_lastX2;

	IT_PLAYING *playing;
};



struct DUMB_IT_SIGRENDERER
{
	DUMB_IT_SIGDATA *sigdata;

	int n_channels;

	unsigned char globalvolume;
	signed char globalvolslide;

	unsigned char tempo;
	signed char temposlide;

	IT_CHANNEL channel[DUMB_IT_N_CHANNELS];

	IT_PLAYING *playing[DUMB_IT_N_NNA_CHANNELS];

	int tick;
	int speed;
	int rowcount;

	int order; /* Set to -1 if the song is terminated by a callback. */
	int row;
	int processorder;
	int processrow;
	int breakrow;
	int pat_loop_row;

	int n_rows;

	IT_ENTRY *entry_start;
	IT_ENTRY *entry;
	IT_ENTRY *entry_end;

	long time_left; /* Time before the next tick is processed */
	int sub_time_left;

	DUMB_CLICK_REMOVER **click_remover;

	IT_CALLBACKS *callbacks;
};



struct IT_CHECKPOINT
{
	IT_CHECKPOINT *next;
	long time;
	DUMB_IT_SIGRENDERER *sigrenderer;
};



struct IT_CALLBACKS
{
	int (*loop)(void *data);
	void *loop_data;
	/* Return 1 to prevent looping; the music will terminate abruptly. If you
	 * want to make the music stop but allow samples to fade (beware, as they
	 * might not fade at all!), use dumb_it_sr_set_speed() and set the speed
	 * to 0. Note that xm_speed_zero() will not be called if you set the
	 * speed manually, and also that this will work for IT and S3M files even
	 * though the music can't stop in this way by itself.
	 */

	int (*xm_speed_zero)(void *data);
	void *xm_speed_zero_data;
	/* Return 1 to terminate the mod, without letting samples fade. */

	int (*midi)(void *data, int channel, unsigned char byte);
	void *midi_data;
	/* Return 1 to prevent DUMB from subsequently interpreting the MIDI bytes
	 * itself. In other words, return 1 if the Zxx macros in an IT file are
	 * controlling filters and shouldn't be.
	 */
};



void _dumb_it_end_sigrenderer(sigrenderer_t *sigrenderer);
void _dumb_it_unload_sigdata(sigdata_t *vsigdata);

extern DUH_SIGTYPE_DESC _dumb_sigtype_it;



#define XM_APPREGIO                0
#define XM_PORTAMENTO_UP           1
#define XM_PORTAMENTO_DOWN         2
#define XM_TONE_PORTAMENTO         3
#define XM_VIBRATO                 4
#define XM_VOLSLIDE_TONEPORTA      5
#define XM_VOLSLIDE_VIBRATO        6
#define XM_TREMOLO                 7
#define XM_SET_PANNING             8
#define XM_SAMPLE_OFFSET           9
#define XM_VOLUME_SLIDE            10 /* A */
#define XM_POSITION_JUMP           11 /* B */
#define XM_SET_CHANNEL_VOLUME      12 /* C */
#define XM_PATTERN_BREAK           13 /* D */
#define XM_E                       14 /* E */
#define XM_SET_TEMPO_BPM           15 /* F */
#define XM_SET_GLOBAL_VOLUME       16 /* G */
#define XM_GLOBAL_VOLUME_SLIDE     17 /* H */
#define XM_KEY_OFF                 20 /* K (undocumented) */
#define XM_SET_ENVELOPE_POSITION   21 /* L */
#define XM_PANNING_SLIDE           25 /* P */
#define XM_MULTI_RETRIG            27 /* R */
#define XM_TREMOR                  29 /* T */
#define XM_X                       33 /* X */
#define XM_N_EFFECTS               (10+26)

#define XM_E_SET_FILTER            0x0
#define XM_E_FINE_PORTA_UP         0x1
#define XM_E_FINE_PORTA_DOWN       0x2
#define XM_E_SET_GLISSANDO_CONTROL 0x3
#define XM_E_SET_VIBRATO_CONTROL   0x4
#define XM_E_SET_FINETUNE          0x5
#define XM_E_SET_LOOP              0x6
#define XM_E_SET_TREMOLO_CONTROL   0x7
#define XM_E_RETRIG_NOTE           0x9
#define XM_E_FINE_VOLSLIDE_UP      0xA
#define XM_E_FINE_VOLSLIDE_DOWN    0xB
#define XM_E_NOTE_CUT              0xC
#define XM_E_NOTE_DELAY            0xD
#define XM_E_PATTERN_DELAY         0xE

#define XM_X_EXTRAFINE_PORTA_UP    1
#define XM_X_EXTRAFINE_PORTA_DOWN  2

/* To make my life a bit simpler during conversion, effect E:xy is converted
 * to effect number EBASE+x:y. The same applies to effect X, and IT's S. That
 * way, these effects can be manipulated like regular effects.
 */
#define EBASE              (XM_N_EFFECTS)
#define XBASE              (EBASE+16)
#define SBASE              (IT_N_EFFECTS)

#define EFFECT_VALUE(x, y) (((x)<<4)|(y))
#define HIGH(v)            ((v)>>4)
#define LOW(v)             ((v)&0x0F)
#define SET_HIGH(v, x)     v = (((x)<<4)|((v)&0x0F))
#define SET_LOW(v, y)      v = (((v)&0xF0)|(y))
#define BCD_TO_NORMAL(v)   (HIGH(v)*10+LOW(v))



#if 0
unsigned char **_dumb_malloc2(int w, int h);
void _dumb_free2(unsigned char **line);
#endif

void _dumb_it_xm_convert_effect(int effect, int value, IT_ENTRY *entry);
int _dumb_it_fix_invalid_orders(DUMB_IT_SIGDATA *sigdata);



#endif /* INTERNAL_IT_H */
