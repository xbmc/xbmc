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
 * EAW -- Quick and dirty hack to convert the TiMidity events generated from
 * a MOD file into events suitable for writing to a midi file, then write
 * the midi file.
 *
 * DONE LIST (various odd things needed for MOD->MIDI format conversion):
 *  1) MIDI does not support pitch bends > 2 octaves...
 *     scale pitch bends for a sensitivy of 24 instead of 128,
 *     pitch bends > 2 octaves wrapped back an octave and continue as normal
 *  2) moved all drum voices onto the drum channel, and all non-drum voices
 *     off of the drum channel to the first available channel (which could be
 *     a channel that has been freed by moving all of it's drum voices off
 *     of it)
 *  3) moved/duped events on to other channels to handle the drum related stuff
 *  4) force all drums to center pan, since drums originally on different
 *     channels could mess up each other's pans when they are merged on to a
 *     single channel
 *  5) added in extra stuff to keep track of and scale expression events as
 *     necessary, so that each sample can have it's own amplification setting
 *  6) turn all drum channel expression stuff into initial note volumes
 *  7) experimented with turning expression events into keypressure events,
 *     but this didn't sound good, because my XG card doesn't handle increases
 *     in keypressure, and the expression events are also needed to boost the
 *     volumes of note decays (since keypressure doesn't work on dead notes)
 *  8) emit chords for chord samples
 *  9) issue bank change events, as specified in the cfg file
 * 10) ignore redundant program changes, which often happen from drum moves
 * 11) ability to silence a sample, so it emits no note or program events
 * 12) issue port events to make midi with > 16 channels
 * 13) skip over the drum channel on the 2nd port
 * 14) kill non-looped mod samples when the play past sample end
 * 15) extended bend range to 4 octaves using note+pitchbend shifting
 * 16) automatic sample chord/pitch assignment and cfg file generation !!! :)
 * 17) can offset pitchbends to undo out of tune sample tuning (detuning)
 * 18) converts linear mod volumes into non-linear midi volumes
 * 19) other more minor things that may or may not be commented
 *
 * TODO LIST (likely to be done eventually)
 *  1) correctly implement fine tuning tweaks via extra pitch bends
 *  2) maybe issue a SYSEX event to make channel 26 non-drum for regular use?
 *  3) clean up the code some more
 *
 * WISH LIST (far less likely to ever be done):
 *  1) make an alternate output mode that outputs a separate track for every
 *     combination of sample and channel it is used on
 *  2) possibly have an automated channel merger, but it might have to strip
 *     out some pans, expression events, and pitch bends to do it
 *  3) maybe handle > 4 octave pitch bends in a better way
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "freq.h"

static char *outname = NULL;
static char *actual_outname;
static char *cfgname = NULL;

static unsigned char header[14] = { 0x4D, 0x54, 0x68, 0x64,
    0x00, 0x00, 0x00, 0x06,
    0x00, 0x01, 0x00, 0x00,
    0x00, 0x00
};
static unsigned char mtrk[4] = { 0x4D, 0x54, 0x72, 0x6B };
static unsigned char event[11];
static unsigned char dt_array[4];
static unsigned char *track_events[34];
static unsigned char *p_track_event;
static uint32 last_track_event_time[34];
static uint32 track_size[34];
static int tracks_enabled[34];
static int tracks_useless[34];
static int current_track_sample[34];
static int orig_track_expr[34];
static int current_channel_expr[34];
static int current_channel_bank[34];
static int current_channel_program[34];
static int current_channel_note[34];
static int current_track_note[34];
static int track_pans[34];
static uint32 kill_early_time[34];
static int kill_early_note[34];
static int kill_early_velocity[34];
static int kill_early_ch[34];
static int tweak_note_offset[34];
static int tweak_pb_offset[34];

static uint32 length;
static uint16 divisions, orig_divisions;
static double divisions_ratio;
static int num_dt_bytes;
static int32 value;
static int num_tracks = 0;
static int32 tempo = 500000;
static uint32 maxtime = 0;
static uint32 num_killed_early = 0;
static uint32 num_big_pitch_slides = 0;
static uint32 num_huge_pitch_slides = 0;
static int pb_sensitivity = 24;
static int old_pb_sensitivity = 128;
static float notes_per_pb = 24.0 / 8191.0;
static float pb_per_note = 8191.0 / 24.0;
static int rpn_msb = 0, rpn_lsb = 0;
static int min_enabled_track = -1, max_enabled_track, first_free_track = -1;
static int non_drums_on_drums = 0;
#define MAX_PB_SENSITIVITY 24

static int silent_samples[256];
static int sample_chords[256];
static int sample_to_program[256];
static int banks[256];
static int transpose[256];
static int is_drum_sample[256];
static int vol_amp[256];
static char linestring[256];
static int fine_tune[256];

static double samples_per_tick;
static int maxsample;
static char chord_letters[4] = { 'M', 'm', 'd', 'f' };

#define ROUND(frac)			(floor(frac + 0.5))

/* mod volumes are linear, midi volumes are x^1.66 (for generic hardware)
 * scale the mod volumes so they sound linear on non-linear midi devices
 * EXPRESSION events are the new corrected volume levels
 * MAINVOLUME events fine tune the volume correction
 *
 * lookup_table[mod_vol][expression, volume]
 */
static char vol_nonlin_to_lin[128][2] = {
      0, 127,   7, 125,  11, 120,  14, 121,  16, 126,  19, 121,  21, 122,
     23, 122,  25, 122,  26, 126,  28, 125,  30, 123,  31, 126,  33, 124,
     34, 126,  36, 124,  37, 125,  38, 126,  40, 124,  41, 125,  42, 126,
     43, 127,  45, 125,  46, 125,  47, 126,  48, 126,  49, 127,  50, 127,
     52, 125,  53, 125,  54, 125,  55, 125,  56, 126,  57, 126,  58, 126,
     59, 126,  60, 126,  61, 126,  62, 126,  63, 126,  64, 126,  65, 126,
     66, 126,  67, 125,  68, 125,  69, 125,  69, 127,  70, 127,  71, 126,
     72, 126,  73, 126,  74, 126,  75, 126,  76, 125,  76, 127,  77, 127,
     78, 126,  79, 126,  80, 126,  81, 126,  81, 127,  82, 126,  83, 126,
     84, 126,  85, 126,  85, 127,  86, 126,  87, 126,  88, 126,  88, 127,
     89, 127,  90, 126,  91, 126,  91, 127,  92, 127,  93, 126,  94, 126,
     94, 127,  95, 127,  96, 126,  97, 126,  97, 127,  98, 126,  99, 126,
    100, 126, 100, 127, 101, 126, 102, 126, 102, 127, 103, 126, 104, 126,
    104, 127, 105, 127, 106, 126, 106, 127, 107, 127, 108, 126, 108, 127,
    109, 127, 110, 126, 110, 127, 111, 127, 112, 126, 112, 127, 113, 127,
    114, 126, 114, 127, 115, 127, 116, 126, 116, 127, 117, 126, 118, 126,
    118, 127, 119, 126, 120, 126, 120, 127, 121, 126, 121, 127, 122, 126,
    123, 126, 123, 127, 124, 126, 124, 127, 125, 127, 126, 126, 126, 127,
    127, 126, 127, 127 };

/*
 * Uses the volume curve specified by the -V or --volume-curve option.
 *
 * The resulting midi will sound correct on hardware / software that uses
 * the volume curve selected by the user.
 *
 * -V 1.661, or (1/log10(4)), is the default timidity volume curve for GM/XG,
 * and is thus the default for MOD->MIDI output as well, so that the
 * resulting midi will sound correct when played using default timidity
 * options.
 *
 * -V 2 should be used to generate midi for playback on MIDI hardware, since
 * the GM/GS/XG standards all specify a volume curve of X^2
 *
 * -V 1.661 will still sound good on MIDI hardware, though, and should sound
 * better than -V 2 on softsynths that (incorrectly) use linear volume curves
 * (such as TiMidity++ versions <= 2.11.3).
 *
 * Use -V 1 if you want to generate midi ONLY for linear volume devices.
 * It will sound bad when played with anything else.
 */
void fill_vol_nonlin_to_lin_table(void)
{
    int i;
    int coarse, fine;
    double power = 0;
    double inverse;
    double temp;
    double log_127;

    /* derive the power used to generate the user volume table */
    log_127 = log(127);
    for (i = 1; i <= 126; i++)
    	power += (log(user_vol_table[i]) - log_127) / (log(i) - log_127);
    power /= 126;

    /* use the inverse of the power to generate the new table */
    for (i = 1; i <= 127; i++)
    {
    	inverse = pow(i / 127.0, 1.0 / power);
       	temp = 127 * inverse;

    	coarse = floor(temp + 0.5);
	if (coarse < temp) coarse += 1;

	fine = floor(127 * temp / coarse + 0.5);

	vol_nonlin_to_lin[i][0] = coarse;
	vol_nonlin_to_lin[i][1] = fine;
    }
}

/* generate the names of the output and cfg files from the input file */
/* modified from auto_wav_output_open() in wave_a.c */
static int auto_names(const char *input_filename)
{
    char *ext, *p;

    outname = (char *) safe_realloc(outname,
			       sizeof(char) * (strlen(input_filename) + 5));
    cfgname = (char *) safe_realloc(cfgname,
			       sizeof(char) * (strlen(input_filename) + 5));

    strcpy(outname, input_filename);
    if ((ext = strrchr(outname, '.')) == NULL)
	ext = outname + strlen(outname);
    else
    {
	/* strip ".???" */
	*ext = '\0';
    }

    /* replace '.' and '#' before ext */
    for (p = outname; p < ext; p++)
	if (*p == '.' || *p == '#')
	    *p = '_';

    strcpy(cfgname, outname);
    strcat(outname, ".mid");
    strcat(cfgname, ".m2m");

    actual_outname = outname;

    return 0;
}



void initialize_m2m_stuff(void)
{

    int i;

    memset(track_events, 0, 34 * sizeof(unsigned char *));
    memset(last_track_event_time, 0, 34 * sizeof(uint32));
    memset(track_size, 0, 34 * sizeof(uint32));
    memset(tracks_enabled, 0, 34 * sizeof(int));
    memset(tracks_useless, 0, 34 * sizeof(int));
    memset(current_track_sample, 0, 34 * sizeof(int));
    memset(orig_track_expr, 0, 34 * sizeof(int));
    memset(current_channel_expr, 0, 34 * sizeof(int));
    memset(current_channel_bank, 0, 34 * sizeof(int));
    memset(current_channel_program, 0, 34 * sizeof(int));
    memset(current_channel_note, 0, 34 * sizeof(int));
    memset(current_track_note, 0, 34 * sizeof(int));

    memset(banks, 0, 256 * sizeof(int));
    memset(transpose, 0, 256 * sizeof(int));
    memset(is_drum_sample, 0, 256 * sizeof(int));
    memset(silent_samples, 0, 256 * sizeof(int));
    memset(fine_tune, 0, 256 * sizeof(int));

    /* get the names of the output and cfg files */
    auto_names(current_file_info->filename);
    if (play_mode->name != NULL)
	actual_outname = play_mode->name;
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", actual_outname);

    for (i = 0; i < 256; i++)
    {
	sample_to_program[i] = i;
	if (i > 127)
	    sample_to_program[i] = i - 127;
	sample_chords[i] = -1;
	vol_amp[i] = 100;
    }
    for (i = 0; i < 34; i++)
    {
	tracks_useless[i] = 1;
	current_track_sample[i] = 255;
	current_channel_note[i] = -1;
	current_track_note[i] = -1;
    }

    /* support to increase the number of divisions, this may be useful */
    orig_divisions = current_file_info->divisions;
    divisions = 480;		/* maximum number of divisions */
    divisions_ratio = (divisions / orig_divisions);

    num_tracks = 0;
    tempo = 500000;
    maxtime = 0;
    num_killed_early = 0;
    num_big_pitch_slides = 0;
    num_huge_pitch_slides = 0;
    pb_sensitivity = 24;
    old_pb_sensitivity = 128;
    notes_per_pb = 24.0 / 8191.0;
    pb_per_note = 8191.0 / 24.0;
    rpn_msb = 0;
    rpn_lsb = 0;
    min_enabled_track = -1;
    first_free_track = -1;
    non_drums_on_drums = 0;

    for (i = 1, maxsample = 0; i < 256; i++)
    {
	if (special_patch[i])
	    maxsample = i;
    }
}



int create_m2m_cfg_file(char *cfgname)
{

    FILE *cfgout;
    int i, chord, chord_type, chord_subtype;
    char line[81];
    char chord_str[3];
    char program_str[17];

    cfgout = fopen(cfgname, "wb");
    if (!cfgout)
    {
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "Could not open cfg file %s for writing", cfgname);
	return 0;
    }

    fprintf(cfgout, "%s\t%s\t\t%s\t%s\t%s\n\n",
    	    "# Sample", "Program", "Transpose", "FineTuning", "%Volume");

    for (i = 1; i <= maxsample; i++)
    {
	memset(chord_str, 0, 3 * sizeof(char));

	if (special_patch[i])
	{
	    chord = sample_chords[i];
	    if (chord >= 0)
	    {
		chord_type = chord / 3;
		chord_subtype = chord % 3;
		chord_str[0] = chord_letters[chord_type];
		if (chord_subtype)
		    chord_str[1] = '0' + chord_subtype;
	    }

	    sprintf(program_str, "%d%s", sample_to_program[i], chord_str);

	    sprintf(line, "%d\t\t%s\t\t%d\t\t!%.6f\t100\n",
		    i, program_str, transpose[i],
		    fine_tune[i] * notes_per_pb);
	}
	else
	{
	    sprintf(line, "# %d unused\n", i);
	}

	fprintf(cfgout, "%s", line);
    }
    fclose(cfgout);

    return 1;
}



void read_m2m_cfg_file(void)
{

    FILE *mod_cfg_file;
    float freq;
    int i, pitch;
    char *chord_str;
    int chord, chord_type, chord_subtype;
    char line[81];
    int mod_sample, program, trans, amp;
    char program_str[20], finetune_str[20];
    char *slash_pos;

    /* read in cfg file stuff, create new one if doesn't exist */
    mod_cfg_file = fopen(cfgname, "rb");
    if (!mod_cfg_file)
    {
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "Couldn't open '%s' cfg file.  Creating %s ...", cfgname,
		  cfgname);
	for (i = 1; i <= maxsample; i++)
	{
	    if (special_patch[i])
	    {
		chord = special_patch[i]->sample->chord;
		freq = special_patch[i]->sample->root_freq_detected;
		pitch = assign_pitch_to_freq(freq);
		fine_tune[i] = (-36.37631656f +
				 17.31234049f * log(freq) - pitch) *
				 pb_per_note;

		sprintf(line,
			"Sample %3d Freq %10.4f Pitch %3d Transpose %4d",
			i, freq, pitch,
			special_patch[i]->sample->transpose_detected);
		if (chord >= 0)
		{
		    sprintf(line, "%s Chord %c Subtype %d",
			    line, chord_letters[chord / 3], chord % 3);
		}

		ctl->cmsg(CMSG_INFO, VERB_NORMAL, "%s", line);

		transpose[i] = special_patch[i]->sample->transpose_detected;
		sample_chords[i] = chord;
	    }
	}
	create_m2m_cfg_file(cfgname);
	mod_cfg_file = fopen(cfgname, "rb");
    }

    if (mod_cfg_file)
    {
	while (fgets(linestring, 256, mod_cfg_file) != NULL)
	{
	    if (*linestring == '#' || *linestring == '\n' ||
		*linestring == '\r')
		    continue;

	    sscanf(linestring, "%d %s %d %s %d\n",
		   &mod_sample, program_str, &trans, finetune_str, &amp);

	    if (strchr(program_str, '!'))
	    	silent_samples[mod_sample] = 1;

	    program = labs(atoi(program_str));
	    if ((slash_pos = strchr(program_str, '/')))
	    {
		banks[mod_sample] = program;
		program = labs(atoi(slash_pos + 1));
	    }
	    sample_to_program[mod_sample] = program;

	    transpose[mod_sample] = trans;

	    if (strchr(finetune_str, '!'))
		fine_tune[mod_sample] = 0;
	    else
		fine_tune[mod_sample] = atof(finetune_str) * pb_per_note;

	    vol_amp[mod_sample] = amp;

	    if (strchr(program_str, '*'))
		is_drum_sample[mod_sample] = 1;
	    else if ((chord_str = strchr(program_str, 'M')))
	    {
		chord_type = strchr(chord_letters, 'M') - chord_letters;
		chord_subtype = atoi(chord_str + 1);
		sample_chords[mod_sample] = chord_type * 3 + chord_subtype;
	    }
	    else if ((chord_str = strchr(program_str, 'm')))
	    {
		chord_type = strchr(chord_letters, 'm') - chord_letters;
		chord_subtype = atoi(chord_str + 1);
		sample_chords[mod_sample] = chord_type * 3 + chord_subtype;
	    }
	    else if ((chord_str = strchr(program_str, 'd')))
	    {
		chord_type = strchr(chord_letters, 'd') - chord_letters;
		chord_subtype = atoi(chord_str + 1);
		sample_chords[mod_sample] = chord_type * 3 + chord_subtype;
	    }
	    else if ((chord_str = strchr(program_str, 'f')))
	    {
		chord_type = strchr(chord_letters, 'f') - chord_letters;
		chord_subtype = atoi(chord_str + 1);
		sample_chords[mod_sample] = chord_type * 3 + chord_subtype;
	    }
	}
	fclose(mod_cfg_file);
    }
    else
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "Couldn't open mod cfg file!  Proceeding without it.");
}



/* Fill the delta time byte array used in the mod->midi output routine */
int set_dt_array(unsigned char *dt_array, int32 delta_time)
{
    int num_dt_bytes = 0, i = 0;
    int32 dt_byte;

    if (delta_time < 0)
    {
	ctl->cmsg(CMSG_INFO, VERB_NORMAL, "WTF?  Delta Time = %ld",
		  delta_time);
	delta_time = 0;
    }

    /* Delta time is variable length, max of 4 bytes long.
       Each byte contains 7 bits of the number.
       Every byte but the last one has an MSB of 1, while the last byte
       has an MSB of 0 to indicate it is the last byte.
     */

    dt_byte = (delta_time & 0x0FFFFFFF) >> 21;
    if (dt_byte)
    {
	dt_array[i++] = dt_byte | 0x80;
	num_dt_bytes = 4;
    }

    dt_byte = (delta_time & 0x001FFFFF) >> 14;
    if (dt_byte || num_dt_bytes)
    {
	dt_array[i++] = dt_byte | 0x80;
	if (!num_dt_bytes)
	    num_dt_bytes = 3;
    }

    dt_byte = (delta_time & 0x00003FFF) >> 7;
    if (dt_byte || num_dt_bytes)
    {
	dt_array[i++] = dt_byte | 0x80;
	if (!num_dt_bytes)
	    num_dt_bytes = 2;
    }

    dt_byte = delta_time & 0x0000007F;
    dt_array[i] = dt_byte;
    if (!num_dt_bytes)
	num_dt_bytes = 1;

    return num_dt_bytes;
}



void scan_ahead_for_m2m_tweaks(MidiEvent * ev, int midi_ch, int midi_note,
			       int samplenum)
{

    int ch, event_type, init_ch, init_note, init_velocity;
    int32 bend = 0, lowbend = 0, highbend = 0;
    int32 pb_offset1 = 0, pb_offset2 = 0;
    int note_offset1 = 0, note_offset2 = 0;
    uint32 init_time, cur_time, old_time, cut_time = 0;
    Sample *sp;
    double a, bent_length = 0, delta_length;
    uint32 length;
    float root_freq, pitch, freq;

    init_time = cur_time = old_time = ev->time;
    init_ch = ev->channel;
    init_note = ev->a;
    init_velocity = ev->b;
    sp = special_patch[samplenum]->sample;
    root_freq = pitch_freq_table[36];
    length = sp->data_length >> FRACTION_BITS;

    /* check for a pitch bend prior to the note */
    if ((ev-1)->type == ME_PITCHWHEEL && (ev-1)->channel == init_ch)
    {
	bend = (ev-1)->b * 128 + (ev-1)->a;
	bend = ROUND((bend - 0x2000) * old_pb_sensitivity /
	             (float) pb_sensitivity);
	bend += fine_tune[samplenum];

	if (bend < lowbend)
	    lowbend = bend;
	if (bend > highbend)
	    highbend = bend;

	pitch = init_note + notes_per_pb * bend;
	freq = 13.75 * exp((pitch - 9) * 0.05776226505f);
    }
    else
        freq = pitch_freq_table[init_note];

    a = (freq * sp->sample_rate) /
	(root_freq * play_mode->rate);

    for (++ev; ev->type != ME_EOT; ev++)
    {
	event_type = ev->type;
	if (event_type != ME_ALL_NOTES_OFF &&
	    event_type != ME_NOTEOFF && event_type != ME_PITCHWHEEL)
		continue;

	ch = ev->channel;
	if (ch != init_ch)
	    continue;

	cur_time = ev->time;
	if (event_type == ME_ALL_NOTES_OFF)
	    break;

	if (event_type == ME_NOTEOFF)
	{
	    if (ev->a == init_note)
		break;
	    continue;
	}

	delta_length = (cur_time - old_time) * a;
	if (!cut_time && bent_length + delta_length > length)
	{
	    cut_time = ROUND(cur_time - (bent_length + delta_length - length) /
			     delta_length * (cur_time - old_time));
	}
	bent_length += delta_length;
	old_time = cur_time;

	bend = ev->b * 128 + ev->a;
	bend = ROUND((bend - 0x2000) * old_pb_sensitivity /
	             (float) pb_sensitivity);
	bend += fine_tune[samplenum];

	if (bend < lowbend)
	    lowbend = bend;
	if (bend > highbend)
	    highbend = bend;
	    
	pitch = init_note + notes_per_pb * bend;
	freq = 13.75 * exp((pitch - 9) * 0.05776226505f);

	a = (freq * sp->sample_rate) /
	    (root_freq * play_mode->rate);
    }

    delta_length = (cur_time - old_time) * a;
    if (!cut_time && bent_length + delta_length > length)
    {
	cut_time = ROUND(cur_time - (bent_length + delta_length - length) /
			 delta_length * (cur_time - old_time));
    }
    bent_length += delta_length;

    if (highbend > 8191)
    {
	pb_offset1 = highbend - 8191;
	note_offset1 = ceil(notes_per_pb * pb_offset1);
	pb_offset1 = -note_offset1 * pb_per_note;
    }
    if (lowbend < -8191)
    {
	pb_offset2 = lowbend - -8191;
	note_offset2 = floor(notes_per_pb * pb_offset2);
	pb_offset2 = -note_offset2 * pb_per_note;
    }

    if (note_offset1 > -note_offset2)
    {
	tweak_note_offset[midi_ch] = note_offset1;
	tweak_pb_offset[midi_ch] = pb_offset1;
    }
    else
    {
	tweak_note_offset[midi_ch] = note_offset2;
	tweak_pb_offset[midi_ch] = pb_offset2;
    }

    if (note_offset1 || note_offset2)
	num_big_pitch_slides++;
    if (highbend - lowbend > 16382)
	num_huge_pitch_slides++;

    /* hmm, need to kill this one early */
    kill_early_time[init_ch] = 0;
    if (!(sp->modes & MODES_LOOPING))
    {
	if (bent_length > length)
	{
	    kill_early_note[init_ch] = midi_note + tweak_note_offset[midi_ch];
	    kill_early_velocity[init_ch] = init_velocity;
	    kill_early_time[init_ch] = cut_time;
	    kill_early_ch[init_ch] = midi_ch;
	}
    }
}



void m2m_kill_notes_early(MidiEvent * ev, double time)
{

    int i, j;
    int chord, chord_type, chord_subtype;
    int extra, newnote;
    uint32 kill_time;
    int kill_sample;
    int kill_ch;
    int kill_n;

    /* kill notes early if unlooped samples are causing problems */
    for (j = 0; j < 34; j++)
    {
	if (kill_early_time[j] && kill_early_time[j] <= ev->time)
	{
	    kill_sample = current_track_sample[j];
	    kill_ch = kill_early_ch[j];
	    kill_time = ROUND(time - ((ev->time - kill_early_time[j]) *
			      divisions_ratio) / samples_per_tick);

	    /* looks like we got screwed by a tempo event, so skip it */
	    if (kill_time < last_track_event_time[kill_ch])
	    {
		kill_early_time[j] = 0;
		continue;
	    }
	    /* yet another tempo muck up */
	    if (current_channel_note[kill_ch] != kill_early_note[j])
	    {
		kill_early_time[j] = 0;
		continue;
	    }
	    if (kill_time > maxtime)
		maxtime = kill_time;

	    kill_n = 3;
	    newnote = kill_early_note[j];
	    while (newnote > 127)
	    	newnote -= 12;
	    while (newnote < 0)
	    	newnote += 12;
	    event[0] = 0x80 | (kill_ch & 0x0F);
	    event[1] = newnote;
	    event[2] = kill_early_velocity[j];
	    current_track_note[j] = -1;
	    current_channel_note[kill_ch] = -1;

	    /* resize the track event array */
	    length = track_size[kill_ch];
	    num_dt_bytes = set_dt_array(dt_array, kill_time -
					last_track_event_time[kill_ch]);
	    track_size[kill_ch] += kill_n + num_dt_bytes;
	    track_events[kill_ch] =
		safe_realloc(track_events[kill_ch],
			track_size[kill_ch] * sizeof(unsigned char));

	    /* save the delta_time */
	    p_track_event = track_events[kill_ch] + length;
	    for (i = 0; i < num_dt_bytes; i++)
	    {
		p_track_event[i] = dt_array[i];
	    }

	    /* save the events */
	    p_track_event += num_dt_bytes;
	    for (i = 0; i < kill_n; i++)
	    {
		p_track_event[i] = event[i];
	    }

	    /* spawn extra events for chords */
	    chord = sample_chords[kill_sample];
	    if (chord >= 0)
	    {
		extra = 2;
		length = track_size[kill_ch];
		track_size[kill_ch] += 4 * extra;
		track_events[kill_ch] =
		    safe_realloc(track_events[kill_ch],
			    track_size[kill_ch] * sizeof(unsigned char));
		p_track_event = track_events[kill_ch] + length;
		for (i = 0; i < 3; i++)
		{
		    chord_type = chord / 3;
		    chord_subtype = chord % 3;
		    newnote = event[kill_n - 2] +
			chord_table[chord_type][chord_subtype][i];
		    if (newnote == event[kill_n - 2])
			continue;
		    while (newnote > 127)
		    	newnote -= 12;
		    while (newnote < 0)
		    	newnote += 12;
		    p_track_event[0] = 0x00;
		    p_track_event[1] = event[kill_n - 3];
		    p_track_event[2] = newnote;
		    p_track_event[3] = event[kill_n - 1];
		    p_track_event += 4;
		}
	    }

	    last_track_event_time[kill_ch] = kill_time;
	    kill_early_time[j] = 0;
	    num_killed_early++;
	}
    }
}



void m2m_prescan(MidiEvent * ev)
{

    int i, ch;

    /* find out which tracks will wind up with notes on them */
    for (; ev->type != ME_EOT; ev++)
    {
	if (ev->type == ME_NOTEON || ev->type == ME_SET_PATCH)
	{
	    ch = ev->channel;
	    if (ch >= 25)
		ch++;

	    if (ev->type == ME_NOTEON)
	    {
		if (silent_samples[current_track_sample[ch]])
		    continue;
		tracks_useless[ch] = 0;

		/* move drums to drum channel */
		if (is_drum_sample[current_track_sample[ch]])
		    ch = 9;
		else if (ch == 9)
		{
		    non_drums_on_drums = 1;
		    continue;
		}

		if (!tracks_enabled[ch])
		{
		    tracks_enabled[ch] = 1;
		    num_tracks++;
		}
	    }
	    else
		current_track_sample[ch] = ev->a;
	}
    }
    for (i = 0; i < 34; i++)
    {
	if (!tracks_enabled[i])
	{
	    if (i != 9 && i != 25 && first_free_track < 0)
		first_free_track = i;
	}
	else
	{
	    if (min_enabled_track < 0)
		min_enabled_track = i;
	    max_enabled_track = i;
	}
    }

    /* all tracks were filled, set it to the last track anyways */
    if (first_free_track < 0)
	first_free_track = 63;

    /* we're going to add another track to move stuff off of the drums */
    if (non_drums_on_drums)
    {
	tracks_enabled[first_free_track] = 1;
	num_tracks++;
    }

    /* re-initialize to unseen sample number to make sure it's not a drum */
    for (i = 0; i < 34; i++)
    {
	current_track_sample[i] = 255;
    }

    /* Initialize Port Numbers for channels > 15 */
    for (i = 0; i < 34; i++)
    {
	if (tracks_enabled[i])
	{
	    length = track_size[i];
	    track_size[i] += 5;
	    track_events[i] = safe_realloc(track_events[i], track_size[i] *
				      sizeof(unsigned char));
	    p_track_event = track_events[i] + length;
	    /* Port Change Event */
	    p_track_event[0] = 0x00;
	    p_track_event[1] = 0xFF;
	    p_track_event[2] = 0x21;
	    p_track_event[3] = 0x01;
	    p_track_event[4] = i / 16;
	}
    }

    /* Issue Initial Drum Stuff */
    length = track_size[9];
    track_size[9] += 15;
    track_events[9] = safe_realloc(track_events[9], track_size[9] *
			      sizeof(unsigned char));
    p_track_event = track_events[9] + length;
    /* program change to Standard Drums */
    p_track_event[0] = 0x00;
    p_track_event[1] = 0xC9;
    p_track_event[2] = 0x00;
    /* set initial volume */
    p_track_event[3] = 0x00;
    p_track_event[4] = 0xB9;
    p_track_event[5] = 0x07;
    p_track_event[6] = 127;
    /* set initial expression */
    p_track_event[7] = 0x00;
    p_track_event[8] = 0xB9;
    p_track_event[9] = 0x0B;
    p_track_event[10] = 127;
    /* set center pan */
    p_track_event[11] = 0x00;
    p_track_event[12] = 0xB9;
    p_track_event[13] = 0x0A;
    p_track_event[14] = 64;
}



void m2m_process_events(MidiEvent * ev)
{

    int i;
    int moved_to_drums;
    int event_type;
    int skip_ch_expr_flag;
    uint32 oldtime = 0, deltatime;
    double time = 0;
    int chord, chord_type, chord_subtype;
    int ch, n, old_ch, newnote, mod_sample, expression;
    int extra;

    /* go through the list for real this time */
    for (; ev->type != ME_EOT; ev++)
    {
	/* convert timidity times to midi event times */
	samples_per_tick = (double) play_mode->rate * (double) tempo /
	    (double) 1000000 / (double) orig_divisions;
	deltatime = ev->time - oldtime;
	oldtime = ev->time;
	time += (deltatime * divisions_ratio) / samples_per_tick;
	if (time > maxtime)
	    maxtime = time;

	m2m_kill_notes_early(ev, time);

	n = 0;
	ch = ev->channel;
	if (ch >= 25)
	    ch++;
	if (ev->type != ME_TEMPO && tracks_useless[ch])
	    continue;
	if (ev->type != ME_TEMPO)
	    mod_sample = current_track_sample[ch];

	/* skip silent sample events */
	if (silent_samples[mod_sample] &&
	    (ev->type == ME_NOTEON || ev->type == ME_NOTEOFF ||
	     ev->type == ME_KEYPRESSURE)) continue;
	if (ev->type == ME_SET_PATCH && silent_samples[ev->a])
	{
	    continue;
	}

	if (ev->type == ME_EXPRESSION && silent_samples[ev->a] &&
	    !current_track_note[ch])
	{
	    orig_track_expr[ch] = ev->a;
	    continue;
	}

	skip_ch_expr_flag = 0;
	moved_to_drums = 0;
	old_ch = ch;
	if (ev->type != ME_TEMPO && ev->type != ME_SET_PATCH)
	{
	    /* move non-drums off to first free channel, drums to drum channel */
	    if (ch == 9 &&
		(!is_drum_sample[mod_sample] ||
		 (ev->type != ME_NOTEON &&
		  ev->type != ME_NOTEOFF && ev->type != ME_KEYPRESSURE)))
	    {
		ch = first_free_track;
	    }
	    else if (is_drum_sample[mod_sample])
	    {
		if (ch != 9)
		{
		    ch = 9;
		    moved_to_drums = 1;
		}
	    }
	}

	event_type = ev->type;
	switch (ev->type)
	{
	case ME_NOTEOFF:
	    n = 3;

	    if (is_drum_sample[mod_sample])
		newnote = sample_to_program[mod_sample];
	    else
	    {
		newnote = ev->a + transpose[mod_sample];
		newnote += tweak_note_offset[ch];
	    }
	    while (newnote > 127)
	    	newnote -= 12;
	    while (newnote < 0)
	    	newnote += 12;

	    event[0] = 0x80 | (ch & 0x0F);
	    event[1] = newnote;
	    event[2] = ev->b;

	    /* only issue a NOTEOFF is there is a note playing on the ch */
	    if (ch != 9 && current_channel_note[ch] == -1)
		n = 0;

	    current_channel_note[ch] = -1;
	    current_track_note[old_ch] = -1;
	    break;

	case ME_NOTEON:
	    n = 3;

	    if (is_drum_sample[mod_sample])
		newnote = sample_to_program[mod_sample];
	    else
	    {
		newnote = ev->a + transpose[mod_sample];
		scan_ahead_for_m2m_tweaks(ev, ch, newnote, mod_sample);
		newnote += tweak_note_offset[ch];
	    }
	    while (newnote > 127)
	    	newnote -= 12;
	    while (newnote < 0)
	    	newnote += 12;

	    event[0] = 0x90 | (ch & 0x0F);
	    event[1] = newnote;
	    event[2] = ev->b;

	    expression = ROUND(orig_track_expr[old_ch] *
			       vol_amp[mod_sample] / 100.0);

	    /* max expression at 127 */
	    if (expression > 127)
		expression = 127;

	    if (is_drum_sample[mod_sample])
	    {
		event[2] = vol_nonlin_to_lin[expression][0];
	    }
	    /* current expression may not be what's wanted for the sample */
	    /* HACK -- insert a prior expression event */
	    else if (expression != current_channel_expr[ch])
	    {
	    	/* NOTEON event */
		n = 11;
		event[7] = 0x00;
		event[8] = event[0];
		event[9] = event[1];
		event[10] = event[2];

		/* non-linear expression event */
		event[0] = 0xB0 | (ch & 0x0F);
		event[1] = 0x0B;
		event[2] = vol_nonlin_to_lin[expression][0];

		/* non-linear volume event */
		event[3] = 0x00;
		event[4] = 0xB0 | (ch & 0x0F);
		event[5] = 0x07;
		event[6] = vol_nonlin_to_lin[expression][1];
		current_channel_expr[ch] = expression;
	    }

	    current_channel_note[ch] = newnote;
	    current_track_note[old_ch] = newnote;
	    break;

	case ME_KEYPRESSURE:
	    n = 3;

	    if (is_drum_sample[mod_sample])
		newnote = sample_to_program[mod_sample];
	    else
	    {
		newnote = ev->a + transpose[mod_sample];
		newnote += tweak_note_offset[ch];
	    }
	    while (newnote > 127)
	    	newnote -= 12;
	    while (newnote < 0)
	    	newnote += 12;

	    event[0] = 0xA0 | (ch & 0x0F);
	    event[1] = newnote;
	    event[2] = ev->b;
	    break;

	case ME_SET_PATCH:
	    n = 2;
	    current_track_sample[old_ch] = ev->a;

	    if (is_drum_sample[ev->a])
	    {
		ch = 9;
		current_channel_program[ch] = sample_to_program[ev->a];

		/* don't emit any event at all */
		n = 0;

		/* change drum banks if necessary */
		if (banks[ev->a] != current_channel_bank[ch])
		{
		    if (n)
			event[n++] = 0x00;
		    event[n++] = 0xC0 | (ch & 0x0F);
		    event[n++] = current_channel_bank[ch] = banks[ev->a];
		}
	    }
	    else
	    {
		if (ch == 9)
		    ch = first_free_track;

		/* program already set, no need to change it */
		if (sample_to_program[ev->a] == current_channel_program[ch] &&
		    banks[ev->a] == current_channel_bank[ch])
		{
		    n = 0;
		}
		/* no need to change bank, it's already set correctly */
		else if (banks[ev->a] == current_channel_bank[ch])
		{
		    event[0] = 0xC0 | (ch & 0x0F);
		    event[1] = sample_to_program[ev->a];
		}
		/* need to change bank to that of the new instrument */
		else
		{
		    n = 10;
		    /* Bank Select MSB */
		    event[0] = 0xB0 | (ch & 0x0F);
		    event[1] = 0x00;
		    event[2] = banks[ev->a];
		    current_channel_bank[ch] = banks[ev->a];

		    /* Bank Select LSB */
		    event[3] = 0x00;
		    event[4] = 0xB0 | (ch & 0x0F);
		    event[5] = 0x20;
		    event[6] = 0;

		    /* Program Change */
		    event[7] = 0x00;
		    event[8] = 0xC0 | (ch & 0x0F);
		    event[9] = sample_to_program[ev->a];
		}
		current_channel_program[ch] = sample_to_program[ev->a];
	    }
	    break;

	case ME_PITCHWHEEL:
	    n = 3;
	    event[0] = 0xE0 | (ch & 0x0F);

	    /* max pitch bend sensitivity is 24, scale stuff */
	    value = ev->b * 128 + ev->a;
	    value = ROUND((value - 0x2000) * old_pb_sensitivity /
	                  (float) pb_sensitivity);
	    value += fine_tune[mod_sample];
	    value += tweak_pb_offset[ch];

	    /* fudge stuff by an octave multiple if it's out of bounds */
	    /* this hasn't been tested much, but it seems to sound OK */
	    if (value > 8191)
	    {
		value = (value % 8191) % 4095 + 4096;
	    }
	    else if (value < -8191)
	    {
		value = -value;
		value = (value % 8191) % 4095 + 4096;
		value = -value;
	    }

	    /* add the offset back in, save it */
	    value += 0x2000;
	    event[1] = value & 0x7F;
	    event[2] = value >> 7;
	    break;

	case ME_DATA_ENTRY_MSB:
	    n = 3;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x06;
	    event[2] = ev->a;

	    /* pitch sensitivity maxes out at 24, not 128 */
	    if (rpn_msb == 0 && rpn_lsb == 0)
	    {
		old_pb_sensitivity = pb_sensitivity = ev->a;
		if (pb_sensitivity > MAX_PB_SENSITIVITY)
		    pb_sensitivity = MAX_PB_SENSITIVITY;
		event[2] = pb_sensitivity;
		notes_per_pb = pb_sensitivity / 8191.0;
		pb_per_note = 8191.0 / pb_sensitivity;
	    }
	    break;

	case ME_MAINVOLUME:
	    n = 3;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x07;
	    event[2] = ev->a;
	    break;

	case ME_PAN:
	    n = 3;
	    track_pans[ch] = ev->a;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x0A;
	    event[2] = ev->a;
	    break;

	case ME_EXPRESSION:
	    n = 7;

	    orig_track_expr[old_ch] = ev->a;

	    expression = ROUND(ev->a * vol_amp[mod_sample] / 100.0);

	    /* max expression at 127 */
	    if (expression > 127)
		expression = 127;

	    if (current_channel_expr[ch] == expression)
		skip_ch_expr_flag = 1;
	    else
		current_channel_expr[ch] = expression;

#ifdef MUTATE_EXPRESSION_TO_KEYPRESSURE
	    /* HACK - mutate it into a KEYPRESSUE event */
	    /* but only if there's a note playing */
	    if (current_track_note[old_ch] >= 0) {
	    	event_type = ME_KEYPRESSURE;
		event[0] = 0xA0 | (ch & 0x0F);
		event[1] = current_track_note[old_ch];
		event[2] = expression;
	    }
	    else
	    	n = 0;
#endif

	    /* non-linear expression event */
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x0B;
	    event[2] = vol_nonlin_to_lin[expression][0];

	    /* non-linear volume event */
	    event[3] = 0x00;
	    event[4] = 0xB0 | (ch & 0x0F);
	    event[5] = 0x07;
	    event[6] = vol_nonlin_to_lin[expression][1];
	    break;

	case ME_RPN_LSB:
	    n = 3;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x64;
	    event[2] = ev->a;
	    rpn_lsb = ev->a;
	    break;

	case ME_RPN_MSB:
	    n = 3;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 0x65;
	    event[2] = ev->a;
	    rpn_msb = ev->a;
	    break;

	case ME_TEMPO:
	    n = 6;
	    event[0] = 0xFF;
	    event[1] = 0x51;
	    event[2] = 3;
	    event[3] = ev->a;
	    event[4] = ev->b;
	    event[5] = ch;
	    tempo = ch + ev->b * 256 + ev->a * 65536;
	    break;

	case ME_ALL_NOTES_OFF:
	    n = 3;
	    event[0] = 0xB0 | (ch & 0x0F);
	    event[1] = 123;
	    event[2] = 0;
	    break;

/*	case ME_DRUMPART:
	    break;
*/
	}

	/* I here by decree that all tempo events shall go on the first ch */
	if (event_type == ME_TEMPO)
	    ch = min_enabled_track;

	/* ah ha, we shall keep this event */
	if (n)
	{
	    if (!(ch == 9 && (event_type == ME_EXPRESSION ||
	    		      event_type == ME_PITCHWHEEL ||
	    		      event_type == ME_PAN)) && !skip_ch_expr_flag)
	    {
		/* resize the track event array */
		length = track_size[ch];
		num_dt_bytes = set_dt_array(dt_array, time -
					    last_track_event_time[ch]);
		track_size[ch] += n + num_dt_bytes;
		track_events[ch] = safe_realloc(track_events[ch], track_size[ch] *
					   sizeof(unsigned char));

		/* save the delta_time */
		p_track_event = track_events[ch] + length;
		for (i = 0; i < num_dt_bytes; i++)
		{
		    p_track_event[i] = dt_array[i];
		}

		/* save the events */
		p_track_event += num_dt_bytes;
		for (i = 0; i < n; i++)
		{
		    p_track_event[i] = event[i];
		}

		/* spawn extra events for chords */
		/* don't forget that there could be a preceeding expr event */
		chord = sample_chords[mod_sample];
		if (chord >= 0 &&
		    (event_type == ME_NOTEON ||
		     event_type == ME_NOTEOFF || event_type == ME_KEYPRESSURE))
		{
		    extra = 2;
		    length = track_size[ch];
		    track_size[ch] += 4 * extra;
		    track_events[ch] =
			safe_realloc(track_events[ch],
				track_size[ch] * sizeof(unsigned char));
		    p_track_event = track_events[ch] + length;
		    for (i = 0; i < 3; i++)
		    {
			chord_type = chord / 3;
			chord_subtype = chord % 3;
			newnote = event[n - 2] +
			    chord_table[chord_type][chord_subtype][i];
			if (newnote == event[n - 2])
			    continue;
			while (newnote > 127)
			    newnote -= 12;
			while (newnote < 0)
			    newnote += 12;
			p_track_event[0] = 0x00;
			p_track_event[1] = event[n - 3];
			p_track_event[2] = newnote;
			p_track_event[3] = event[n - 1];
			p_track_event += 4;
		    }
		}

		last_track_event_time[ch] = time;
	    }

	    /* moved it for drums, issue control events to old channel too */
	    if (moved_to_drums && old_ch != first_free_track &&
		event_type != ME_NOTEON &&
		event_type != ME_NOTEOFF &&
		event_type != ME_KEYPRESSURE &&
		!(event_type == ME_EXPRESSION &&
		  (current_channel_note[old_ch] < 0 ||
		   current_channel_expr[old_ch] == expression)) &&
		event_type != ME_TEMPO)
	    {
		/* resize the track event array */
		length = track_size[old_ch];
		num_dt_bytes = set_dt_array(dt_array, time -
					    last_track_event_time[old_ch]);
		track_size[old_ch] += n + num_dt_bytes;
		track_events[old_ch] = safe_realloc(track_events[old_ch],
					       track_size[old_ch] *
					       sizeof(unsigned char));

		/* save the delta_time */
		p_track_event = track_events[old_ch] + length;
		for (i = 0; i < num_dt_bytes; i++)
		{
		    p_track_event[i] = dt_array[i];
		}

		/* replace channel nibble with old channel */
		event[0] = (event[0] & 0xF0) | (old_ch & 0x0F);

		/* save the events */
		p_track_event += num_dt_bytes;
		for (i = 0; i < n; i++)
		{
		    p_track_event[i] = event[i];
		}
		last_track_event_time[old_ch] = time;
	    }
	}
    }
}



void m2m_output_midi_file(void)
{

    FILE *outfile;
    int extra;
    int i, j;

    outfile = fopen(actual_outname, "wb");
    if (!outfile)
    {
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "Uh oh, can't open '%s' output file.  Bombing out...",
		  actual_outname);
	return;
    }

    /* finish up the file header */
    header[10] = (num_tracks & 0xFF00) >> 8;
    header[11] = num_tracks & 0x00FF;
    header[12] = (divisions & 0xFF00) >> 8;
    header[13] = divisions & 0x00FF;

    /* output the file header */
    for (i = 0; i < 14; i++)
    {
	fprintf(outfile, "%c", header[i]);
    }

    /* output each track */
    for (i = 0; i < 34; i++)
    {
	if (!tracks_enabled[i])
	    continue;

	/* do the track header */
	for (j = 0; j < 4; j++)
	{
	    fprintf(outfile, "%c", mtrk[j]);
	}

	length = track_size[i] + 4;
	extra = 4;

	ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Track %d Size %d", i, length);
	fprintf(outfile, "%c", (length & 0xFF000000) >> 24);
	fprintf(outfile, "%c", (length & 0x00FF0000) >> 16);
	fprintf(outfile, "%c", (length & 0x0000FF00) >> 8);
	fprintf(outfile, "%c", length & 0x000000FF);

	/* write the events */
	p_track_event = track_events[i];
	for (j = 0; j < length - extra; j++, p_track_event++)
	{
	    fprintf(outfile, "%c", *p_track_event);
	}

	/* write the terminal event */
	fprintf(outfile, "%c", 0x00);
	fprintf(outfile, "%c", 0xFF);
	fprintf(outfile, "%c", 0x2F);
	fprintf(outfile, "%c", 0x00);
    }
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Number of tracks actually used: %d",
	      num_tracks);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Track accepting drum refugees: %d",
	      first_free_track);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL,
	      "Number of unlooped notes killed early: %ld", num_killed_early);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL,
	      "Number of pitch slides > 2 octaves: %ld", num_big_pitch_slides);
    ctl->cmsg(CMSG_INFO, VERB_NORMAL,
	      "Number of pitch slides > 4 octaves: %ld",
	      num_huge_pitch_slides);

    fclose(outfile);
}



void convert_mod_to_midi_file(MidiEvent * ev)
{

    int i;

    change_system_mode(DEFAULT_SYSTEM_MODE);

    /* use user volume curve if specified, rather than the default */
    if (opt_user_volume_curve)
	fill_vol_nonlin_to_lin_table();

    initialize_m2m_stuff();

    /* this either isn't a MOD, or it doesn't have any samples... */
    if (!maxsample)
    {
	ctl->cmsg(CMSG_INFO, VERB_NORMAL,
		  "Aborting!  This doesn't look like a MOD file!");
	return;
    }

    read_m2m_cfg_file();

    m2m_prescan(ev);
    m2m_process_events(ev);
    m2m_output_midi_file();

    /* free track event arrays */
    for (i = 0; i < 34; i++) {
	if (track_events[i])
	    free(track_events[i]);
    }
}
