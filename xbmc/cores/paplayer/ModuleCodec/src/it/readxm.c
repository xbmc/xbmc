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
 * readxm.c - Code to read a Fast Tracker II          / / \  \
 *            module from an open file.              | <  /   \_
 *                                                   |  \/ /\   /
 * By Julien Cugniere. Some bits of code taken        \_  /  > /
 * from reads3m.c.                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dumb.h"
#include "internal/it.h"



/** TODO:

 * XM_TREMOLO                        doesn't sound quite right...
 * XM_E_SET_FINETUNE                 done but not tested yet.
 * XM_SET_ENVELOPE_POSITION          todo.

 * VIBRATO conversion needs to be checked (sample/effect/volume). Plus check
   that effect memory is correct when using XM_VOLSLIDE_VIBRATO.
   - sample vibrato (instrument vibrato) is now handled correctly. - entheh

 * XM_E_SET_VIBRATO/TREMOLO_CONTROL: effectvalue&4 -> don't retrig wave when
   a new instrument is played. In retrigger_note()?. Is it worth implementing?

 * Lossy fadeout approximation. 0..31 converted to 0 --> won't fade at all.

 * Replace DUMB's sawtooth by ramp_down/ramp_up. Update XM loader.

 * A lot of things need to be reset when the end of the song is reached.

 * It seems that IT and XM don't behave the same way when dealing with
   mixed loops. When IT encounters multiple SBx (x>0) commands on the same
   row, it decrements the loop count for all, but only execute the loop of
   the last one (highest channel). FT2 only decrements the loop count of the
   last one. Not that I know of any modules using so convoluted combinations!

 * Maybe we could remove patterns that don't appear in the order table ? Or
   provide a function to "optimize" a DUMB_IT_SIGDATA ?

*/



#define XM_LINEAR_FREQUENCY        1 /* otherwise, use amiga slides */

#define XM_ENTRY_PACKED            128
#define XM_ENTRY_NOTE              1
#define XM_ENTRY_INSTRUMENT        2
#define XM_ENTRY_VOLUME            4
#define XM_ENTRY_EFFECT            8
#define XM_ENTRY_EFFECTVALUE       16

#define XM_NOTE_OFF                97

#define XM_ENVELOPE_ON             1
#define XM_ENVELOPE_SUSTAIN        2
#define XM_ENVELOPE_LOOP           4

#define XM_SAMPLE_NO_LOOP          0
#define XM_SAMPLE_FORWARD_LOOP     1
#define XM_SAMPLE_PINGPONG_LOOP    2
#define XM_SAMPLE_16BIT            16
#define XM_SAMPLE_STEREO           32

#define XM_VIBRATO_SINE            0
#define XM_VIBRATO_SQUARE          1
#define XM_VIBRATO_RAMP_DOWN       2
#define XM_VIBRATO_RAMP_UP         3



/* Probably useless :) */
static const char xm_convert_vibrato[] = {
	IT_VIBRATO_SINE,
	IT_VIBRATO_SQUARE,
	IT_VIBRATO_SAWTOOTH,
	IT_VIBRATO_SAWTOOTH
};



#define XM_MAX_SAMPLES_PER_INSTRUMENT 16



/* Extra data that doesn't fit inside IT_INSTRUMENT */
typedef struct XM_INSTRUMENT_EXTRA
{
	int n_samples;
	int vibrato_type;
	int vibrato_sweep; /* 0-0xFF */
	int vibrato_depth; /* 0-0x0F */
	int vibrato_speed; /* 0-0x3F */
}
XM_INSTRUMENT_EXTRA;



/* Frees the original block if it can't resize it or if size is 0, and acts
 * as malloc if ptr is NULL.
 */
static void *safe_realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return NULL;
	} else {
		void *new_block = realloc(ptr, size);
		if (!new_block)
			free(ptr);
		return new_block;
	}
}



/* The interpretation of the XM volume column is left to the player. Here, we
 * just filter bad values.
 */
// This function is so tiny now, should we inline it?
static void it_xm_convert_volume(int volume, IT_ENTRY *entry)
{
	entry->mask |= IT_ENTRY_VOLPAN;
	entry->volpan = volume;

	switch (volume >> 4) {
		case 0xA: /* set vibrato speed */
		case 0xB: /* vibrato */
		case 0xF: /* tone porta */
		case 0x6: /* vol slide up */
		case 0x7: /* vol slide down */
		case 0x8: /* fine vol slide up */
		case 0x9: /* fine vol slide down */
		case 0xC: /* set panning */
		case 0xD: /* pan slide left */
		case 0xE: /* pan slide right */
		case 0x1: /* set volume */
		case 0x2: /* set volume */
		case 0x3: /* set volume */
		case 0x4: /* set volume */
			break;

		case 0x5:
			if (volume == 0x50)
				break; /* set volume */
			/* else fall through */

		default:
			entry->mask &= ~IT_ENTRY_VOLPAN;
			break;
	}
}



static int it_xm_read_pattern(IT_PATTERN *pattern, DUMBFILE *f, int n_channels, unsigned char *buffer)
{
	int size;
	int pos;
	int channel;
	int row;
	int effect, effectvalue;
	IT_ENTRY *entry;

	/* pattern header size */
	if (dumbfile_igetl(f) != 0x09) {
		TRACE("XM error: unexpected pattern header size\n");
		return -1;
	}

	/* pattern data packing type */
	if (dumbfile_getc(f) != 0) {
		TRACE("XM error: unexpected pattern packing type\n");
		return -1;
	}

	pattern->n_rows = dumbfile_igetw(f);  /* 1..256 */
	size = dumbfile_igetw(f);
	pattern->n_entries = 0;

	if (dumbfile_error(f))
		return -1;

	if (size == 0)
		return 0;

	if (size > 1280 * n_channels) {
		TRACE("XM error: pattern data size > %d bytes\n", 1280 * n_channels);
		return -1;
	}

	if (dumbfile_getnc(buffer, size, f) < size)
		return -1;

	/* compute number of entries */
	pattern->n_entries = 0;
	pos = channel = row = 0;
	while (pos < size) {
		if (!(buffer[pos] & XM_ENTRY_PACKED) || (buffer[pos] & 31))
			pattern->n_entries++;

		channel++;
		if (channel >= n_channels) {
			channel = 0;
			row++;
			pattern->n_entries++;
		}

		if (buffer[pos] & XM_ENTRY_PACKED) {
			static const char offset[] = { 0, 1, 1, 2, 1, 2, 2, 3,   1, 2, 2, 3, 2, 3, 3, 4,
			                               1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5 };
			pos += 1 + offset[buffer[pos] & 31];
		} else {
			pos += 5;
		}
	}

	if (row != pattern->n_rows) {
		TRACE("XM error: wrong number of rows in pattern data\n");
		return -1;
	}

	pattern->entry = malloc(pattern->n_entries * sizeof(*pattern->entry));
	if (!pattern->entry)
		return -1;

	/* read the entries */
	entry = pattern->entry;
	pos = channel = row = 0;
	while (pos < size) {
		unsigned char mask;

		if (buffer[pos] & XM_ENTRY_PACKED)
			mask = buffer[pos++] & 31;
		else
			mask = 31;

		if (mask) {
			ASSERT(entry < pattern->entry + pattern->n_entries);

			entry->channel = channel;
			entry->mask = 0;

			if (mask & XM_ENTRY_NOTE) {
				int note = buffer[pos++]; /* 1-96 <=> C0-B7 */
				entry->note = (note == XM_NOTE_OFF) ? (IT_NOTE_OFF) : (note-1);
				entry->mask |= IT_ENTRY_NOTE;
			}

			if (mask & XM_ENTRY_INSTRUMENT) {
				entry->instrument = buffer[pos++]; /* 1-128 */
				entry->mask |= IT_ENTRY_INSTRUMENT;
			}

			if (mask & XM_ENTRY_VOLUME)
				it_xm_convert_volume(buffer[pos++], entry);

			effect = effectvalue = 0;
			if (mask & XM_ENTRY_EFFECT)      effect = buffer[pos++];
			if (mask & XM_ENTRY_EFFECTVALUE) effectvalue = buffer[pos++];
			_dumb_it_xm_convert_effect(effect, effectvalue, entry);

			entry++;
		}

		channel++;
		if (channel >= n_channels) {
			channel = 0;
			row++;
			IT_SET_END_ROW(entry);
			entry++;
		}
	}

	return 0;
}



static int it_xm_make_envelope(IT_ENVELOPE *envelope, const unsigned short *data, int y_offset)
{
	int i, pos;

	if (envelope->n_nodes > 12) {
		TRACE("XM error: wrong number of envelope nodes (%d)\n", envelope->n_nodes);
		envelope->n_nodes = 0;
		return -1;
	}

	pos = 0;
	for (i = 0; i < envelope->n_nodes; i++) {
		envelope->node_t[i] = data[pos++];
		if (data[pos] > 64) {
			TRACE("XM error: out-of-range envelope node (node_y[%d]=%d)\n", i, data[pos]);
			envelope->n_nodes = 0;
			return -1;
		}
		envelope->node_y[i] = (signed char)(data[pos++] + y_offset);
	}

	return 0;
}



static int it_xm_read_instrument(IT_INSTRUMENT *instrument, XM_INSTRUMENT_EXTRA *extra, DUMBFILE *f)
{
	unsigned long size, bytes_read;
	unsigned short vol_points[24];
	unsigned short pan_points[24];
	int i, type;

	/* Header size. Tends to be more than the actual size of the structure.
	 * So unread bytes must be skipped before reading the first sample
	 * header.
	 */
	size = dumbfile_igetl(f);

	dumbfile_getnc(instrument->name, 22, f);
	instrument->name[22] = 0;
	instrument->filename[0] = 0;
	dumbfile_skip(f, 1);  /* Instrument type. Should be 0, but seems random. */
	extra->n_samples = dumbfile_igetw(f);

	if (dumbfile_error(f) || (unsigned int)extra->n_samples > XM_MAX_SAMPLES_PER_INSTRUMENT)
		return -1;

	bytes_read = 4 + 22 + 1 + 2;

	if (extra->n_samples) {
		/* sample header size */
		if (dumbfile_igetl(f) != 0x28) {
			TRACE("XM error: unexpected sample header size\n");
			return -1;
		}

		/* sample map */
		for (i = 0; i < 96; i++) {
			instrument->map_sample[i] = dumbfile_getc(f) + 1;
			instrument->map_note[i] = i;
		}

		if (dumbfile_error(f))
			return 1;

		/* volume/panning envelopes */
		for (i = 0; i < 24; i++)
			vol_points[i] = dumbfile_igetw(f);
		for (i = 0; i < 24; i++)
			pan_points[i] = dumbfile_igetw(f);

		instrument->volume_envelope.n_nodes = dumbfile_getc(f);
		instrument->pan_envelope.n_nodes = dumbfile_getc(f);

		if (dumbfile_error(f))
			return -1;

		instrument->volume_envelope.sus_loop_start = dumbfile_getc(f);
		instrument->volume_envelope.loop_start = dumbfile_getc(f);
		instrument->volume_envelope.loop_end = dumbfile_getc(f);

		instrument->pan_envelope.sus_loop_start = dumbfile_getc(f);
		instrument->pan_envelope.loop_start = dumbfile_getc(f);
		instrument->pan_envelope.loop_end = dumbfile_getc(f);

		/* The envelope handler for XM files won't use sus_loop_end. */

		type = dumbfile_getc(f);
		instrument->volume_envelope.flags = 0;
		if ((type & XM_ENVELOPE_ON) && instrument->volume_envelope.n_nodes)
			instrument->volume_envelope.flags |= IT_ENVELOPE_ON;
		if (type & XM_ENVELOPE_LOOP)    instrument->volume_envelope.flags |= IT_ENVELOPE_LOOP_ON;
#if 1
		if (type & XM_ENVELOPE_SUSTAIN) instrument->volume_envelope.flags |= IT_ENVELOPE_SUSTAIN_LOOP;
#else // This is now handled in itrender.c
		/* let's avoid fading out when reaching the last envelope node */
		if (!(type & XM_ENVELOPE_LOOP)) {
			instrument->volume_envelope.loop_start = instrument->volume_envelope.n_nodes-1;
			instrument->volume_envelope.loop_end = instrument->volume_envelope.n_nodes-1;
		}
		instrument->volume_envelope.flags |= IT_ENVELOPE_LOOP_ON;
#endif

		type = dumbfile_getc(f);
		instrument->pan_envelope.flags = 0;
		if ((type & XM_ENVELOPE_ON) && instrument->pan_envelope.n_nodes)
			instrument->pan_envelope.flags |= IT_ENVELOPE_ON;
		if (type & XM_ENVELOPE_LOOP)    instrument->pan_envelope.flags |= IT_ENVELOPE_LOOP_ON; // should this be here?
		if (type & XM_ENVELOPE_SUSTAIN) instrument->pan_envelope.flags |= IT_ENVELOPE_SUSTAIN_LOOP;

		if (it_xm_make_envelope(&instrument->volume_envelope, vol_points, 0) != 0) {
			TRACE("XM error: volume envelope\n");
			if (instrument->volume_envelope.flags & IT_ENVELOPE_ON) return -1;
		}

		if (it_xm_make_envelope(&instrument->pan_envelope, pan_points, -32) != 0) {
			TRACE("XM error: pan envelope\n");
			if (instrument->pan_envelope.flags & IT_ENVELOPE_ON) return -1;
		}

		instrument->pitch_envelope.flags = 0;

		extra->vibrato_type = dumbfile_getc(f);
		extra->vibrato_sweep = dumbfile_getc(f);
		extra->vibrato_depth = dumbfile_getc(f);
		extra->vibrato_speed = dumbfile_getc(f);

		if (dumbfile_error(f) || extra->vibrato_type >= 4)
			return -1;

		/** WARNING: lossy approximation */
		instrument->fadeout = (dumbfile_igetw(f)*128 + 64)/0xFFF;

		dumbfile_skip(f, 2); /* reserved */

		bytes_read += 4 + 96 + 48 + 48 + 14*1 + 2 + 2;
	} else
		for (i = 0; i < 96; i++)
			instrument->map_sample[i] = 0;

	if (dumbfile_skip(f, size - bytes_read))
		return -1;

	instrument->new_note_action = NNA_NOTE_CUT;
	instrument->dup_check_type = DCT_OFF;
	instrument->dup_check_action = DCA_NOTE_CUT;
	instrument->pp_separation = 0;
	instrument->pp_centre = 60; /* C-5 */
	instrument->global_volume = 128;
	instrument->default_pan = 32;
	instrument->random_volume = 0;
	instrument->random_pan = 0;
	instrument->filter_cutoff = 0;
	instrument->filter_resonance = 0;

	return 0;
}



/* I (entheh) have two XM files saved by a very naughty program. After a
 * 16-bit sample, it saved a rogue byte. The length of the sample was indeed
 * an odd number, incremented to include the rogue byte.
 *
 * In this function we are converting sample lengths and loop points so they
 * are measured in samples. This means we forget about the extra bytes, and
 * they don't get skipped. So we fail trying to read the next instrument.
 *
 * To get around this, this function returns the number of rogue bytes that
 * won't be accounted for by reading sample->length samples. It returns a
 * negative number on failure.
 */
static int it_xm_read_sample_header(IT_SAMPLE *sample, DUMBFILE *f)
{
	int type;
	int relative_note_number; /* relative to C4 */
	int finetune;
	int roguebytes;
	int roguebytesmask;

	sample->length         = dumbfile_igetl(f);
	sample->loop_start     = dumbfile_igetl(f);
	sample->loop_end       = sample->loop_start + dumbfile_igetl(f);
	sample->global_volume  = 64;
	sample->default_volume = dumbfile_getc(f);
	finetune               = (signed char)dumbfile_getc(f); /* -128..127 <=> -1 semitone .. +127/128 of a semitone */
	type                   = dumbfile_getc(f);
	sample->default_pan    = dumbfile_getc(f); /* 0-255 */
	relative_note_number   = (signed char)dumbfile_getc(f);

	dumbfile_skip(f, 1);  /* reserved */

	dumbfile_getnc(sample->name, 22, f);
	sample->name[22] = 0;

	sample->filename[0] = 0;

	if (dumbfile_error(f))
		return -1;

	sample->C5_speed = (long)(16726.0*pow(DUMB_SEMITONE_BASE, relative_note_number)*pow(DUMB_PITCH_BASE, finetune*2));

	sample->flags = IT_SAMPLE_EXISTS;

	roguebytes = (int)sample->length;
	roguebytesmask = 3;

	if (type & XM_SAMPLE_16BIT) {
		sample->flags |= IT_SAMPLE_16BIT;
		sample->length >>= 1;
		sample->loop_start >>= 1;
		sample->loop_end >>= 1;
	} else
		roguebytesmask >>= 1;

	if (type & XM_SAMPLE_STEREO) {
		sample->flags |= IT_SAMPLE_STEREO;
		sample->length >>= 1;
		sample->loop_start >>= 1;
		sample->loop_end >>= 1;
	} else
		roguebytesmask >>= 1;

	roguebytes &= roguebytesmask;

	if ((unsigned int)sample->loop_start < (unsigned int)sample->loop_end) {
		if (type & XM_SAMPLE_FORWARD_LOOP) sample->flags |= IT_SAMPLE_LOOP;
		if (type & XM_SAMPLE_PINGPONG_LOOP) sample->flags |= IT_SAMPLE_LOOP | IT_SAMPLE_PINGPONG_LOOP;
	}

	if (sample->length <= 0)
		sample->flags &= ~IT_SAMPLE_EXISTS;
	else if ((unsigned int)sample->loop_end > (unsigned int)sample->length)
		sample->flags &= ~IT_SAMPLE_LOOP;
	else if ((unsigned int)sample->loop_start >= (unsigned int)sample->loop_end)
		sample->flags &= ~IT_SAMPLE_LOOP;

	return roguebytes;
}



static int it_xm_read_sample_data(IT_SAMPLE *sample, unsigned char roguebytes, DUMBFILE *f)
{
	int old;
	long i;
	long truncated_size;
	int n_channels;
	long datasize;

	if (!(sample->flags & IT_SAMPLE_EXISTS))
		return dumbfile_skip(f, roguebytes);

	/* let's get rid of the sample data coming after the end of the loop */
	if ((sample->flags & IT_SAMPLE_LOOP) && sample->loop_end < sample->length) {
		truncated_size = sample->length - sample->loop_end;
		sample->length = sample->loop_end;
	} else {
		truncated_size = 0;
	}

	n_channels = sample->flags & IT_SAMPLE_STEREO ? 2 : 1;
	datasize = sample->length * n_channels;

	sample->data = malloc(datasize * (sample->flags & IT_SAMPLE_16BIT ? 2 : 1));
	if (!sample->data)
		return -1;

	/* sample data is stored as signed delta values */
	old = 0;
	if (sample->flags & IT_SAMPLE_16BIT)
		for (i = 0; i < sample->length; i++)
			((short *)sample->data)[i*n_channels] = old += dumbfile_igetw(f);
	else
		for (i = 0; i < sample->length; i++)
			((signed char *)sample->data)[i*n_channels] = old += dumbfile_getc(f);

	/* skip truncated data */
	dumbfile_skip(f, (sample->flags & IT_SAMPLE_16BIT) ? (2*truncated_size) : (truncated_size));

	if (sample->flags & IT_SAMPLE_STEREO) {
		old = 0;
		if (sample->flags & IT_SAMPLE_16BIT)
			for (i = 1; i < datasize; i += 2)
				((short *)sample->data)[i] = old += dumbfile_igetw(f);
		else
			for (i = 1; i < datasize; i += 2)
				((signed char *)sample->data)[i] = old += dumbfile_getc(f);

		/* skip truncated data */
		dumbfile_skip(f, (sample->flags & IT_SAMPLE_16BIT) ? (2*truncated_size) : (truncated_size));
	}

	dumbfile_skip(f, roguebytes);

	if (dumbfile_error(f))
		return -1;

	return 0;
}



/* "Real programmers don't document. If it was hard to write,
 *  it should be hard to understand."
 *
 * (Never trust the documentation provided with a tracker.
 *  Real files are the only truth...)
 */
static DUMB_IT_SIGDATA *it_xm_load_sigdata(DUMBFILE *f)
{
	DUMB_IT_SIGDATA *sigdata;
	char id_text[18];

	int flags;
	int n_channels;
	int total_samples;
	int i, j;

	/* check ID text */
	if (dumbfile_getnc(id_text, 17, f) < 17)
		return NULL;
	id_text[17] = 0;
	if (strcmp(id_text, "Extended Module: ") != 0) {
		TRACE("XM error: Not an Extended Module\n");
		return NULL;
	}

	sigdata = malloc(sizeof(*sigdata));
	if (!sigdata)
		return NULL;

	/* song name */
	if (dumbfile_getnc(sigdata->name, 20, f) < 20) {
		free(sigdata);
		return NULL;
	}
	sigdata->name[20] = 0;

	if (dumbfile_getc(f) != 0x1A) {
		TRACE("XM error: 0x1A not found\n");
		free(sigdata);
		return NULL;
	}

	/* tracker name */
	if (dumbfile_skip(f, 20)) {
		free(sigdata);
		return NULL;
	}

	/* version number */
	if (dumbfile_igetw(f) != 0x0104) {
		TRACE("XM error: wrong format version\n");
		free(sigdata);
		return NULL;
	}

	/*
		------------------
		---   Header   ---
		------------------
	*/

	/* header size */
	if (dumbfile_igetl(f) != 0x0114) {
		TRACE("XM error: unexpected header size\n");
		free(sigdata);
		return NULL;
	}

	sigdata->song_message = NULL;
	sigdata->order = NULL;
	sigdata->instrument = NULL;
	sigdata->sample = NULL;
	sigdata->pattern = NULL;
	sigdata->midi = NULL;
	sigdata->checkpoint = NULL;

	sigdata->n_samples        = 0;
	sigdata->n_orders         = dumbfile_igetw(f);
	sigdata->restart_position = dumbfile_igetw(f);
	n_channels                = dumbfile_igetw(f); /* max 32 but we'll be lenient */
	sigdata->n_patterns       = dumbfile_igetw(f);
	sigdata->n_instruments    = dumbfile_igetw(f); /* max 128 */
	flags                     = dumbfile_igetw(f);
	sigdata->speed            = dumbfile_igetw(f);
	if (sigdata->speed == 0) sigdata->speed = 6; // Should we? What about tempo?
	sigdata->tempo            = dumbfile_igetw(f);

	/* sanity checks */
	if (dumbfile_error(f) || sigdata->n_orders <= 0 || sigdata->n_orders > 256 || sigdata->n_patterns > 256 || sigdata->n_instruments > 128 || n_channels > DUMB_IT_N_CHANNELS) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	//if (sigdata->restart_position >= sigdata->n_orders)
		//sigdata->restart_position = 0;

	/* order table */
	sigdata->order = malloc(sigdata->n_orders*sizeof(*sigdata->order));
	if (!sigdata->order) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}
	dumbfile_getnc(sigdata->order, sigdata->n_orders, f);
	dumbfile_skip(f, 256 - sigdata->n_orders);

	if (dumbfile_error(f)) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	/*
		--------------------
		---   Patterns   ---
		--------------------
	*/

	sigdata->pattern = malloc(sigdata->n_patterns * sizeof(*sigdata->pattern));
	if (!sigdata->pattern) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}
	for (i = 0; i < sigdata->n_patterns; i++)
		sigdata->pattern[i].entry = NULL;

	{
		unsigned char *buffer = malloc(1280 * n_channels); /* 256 rows * 5 bytes */
		if (!buffer) {
			_dumb_it_unload_sigdata(sigdata);
			return NULL;
		}
		for (i = 0; i < sigdata->n_patterns; i++) {
			if (it_xm_read_pattern(&sigdata->pattern[i], f, n_channels, buffer) != 0) {
				free(buffer);
				_dumb_it_unload_sigdata(sigdata);
				return NULL;
			}
		}
		free(buffer);
	}

	/*
		-----------------------------------
		---   Instruments and Samples   ---
		-----------------------------------
	*/

	sigdata->instrument = malloc(sigdata->n_instruments * sizeof(*sigdata->instrument));
	if (!sigdata->instrument) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	/* With XM, samples are not global, they're part of an instrument. In a
	 * file, each instrument is stored with its samples. Because of this, I
	 * don't know how to find how many samples are present in the file. Thus
	 * I have to do n_instruments reallocation on sigdata->sample.
	 * Looking at FT2, it doesn't seem possible to have more than 16 samples
	 * per instrument (even though n_samples is stored as 2 bytes). So maybe
	 * we could allocate a 128*16 array of samples, and shrink it back to the
	 * correct size when we know it?
	 * Alternatively, I could allocate samples by blocks of N (still O(n)),
	 * or double the number of allocated samples when I need more (O(log n)).
	 */
	total_samples = 0;
	sigdata->sample = NULL;

	for (i = 0; i < sigdata->n_instruments; i++) {
		XM_INSTRUMENT_EXTRA extra;

		if (it_xm_read_instrument(&sigdata->instrument[i], &extra, f) < 0) {
			TRACE("XM error: instrument %d\n", i+1);
			_dumb_it_unload_sigdata(sigdata);
			return NULL;
		}

		if (extra.n_samples) {
			unsigned char roguebytes[XM_MAX_SAMPLES_PER_INSTRUMENT];

			/* adjust instrument sample map (make indices absolute) */
			for (j = 0; j < 96; j++)
				sigdata->instrument[i].map_sample[j] += total_samples;

			sigdata->sample = safe_realloc(sigdata->sample, sizeof(*sigdata->sample)*(total_samples+extra.n_samples));
			if (!sigdata->sample) {
				_dumb_it_unload_sigdata(sigdata);
				return NULL;
			}
			for (j = total_samples; j < total_samples+extra.n_samples; j++)
				sigdata->sample[j].data = NULL;

			/* read instrument's samples */
			for (j = 0; j < extra.n_samples; j++) {
				IT_SAMPLE *sample = &sigdata->sample[total_samples+j];
				int b = it_xm_read_sample_header(sample, f);
				if (b < 0) {
					_dumb_it_unload_sigdata(sigdata);
					return NULL;
				}
				roguebytes[j] = b;
				// Any reason why these can't be set inside it_xm_read_sample_header()?
				sample->vibrato_speed = extra.vibrato_speed;
				sample->vibrato_depth = extra.vibrato_depth;
				sample->vibrato_rate = extra.vibrato_sweep;
				/* Rate and sweep don't match, but the difference is
				 * accounted for in itrender.c.
				 */
				sample->vibrato_waveform = xm_convert_vibrato[extra.vibrato_type];
			}
			for (j = 0; j < extra.n_samples; j++) {
				if (it_xm_read_sample_data(&sigdata->sample[total_samples+j], roguebytes[j], f) != 0) {
					_dumb_it_unload_sigdata(sigdata);
					return NULL;
				}
			}
			total_samples += extra.n_samples;
		}
	}

	sigdata->n_samples = total_samples;

	sigdata->flags = IT_WAS_AN_XM | IT_OLD_EFFECTS | IT_COMPATIBLE_GXX | IT_STEREO | IT_USE_INSTRUMENTS;
	// Are we OK with IT_COMPATIBLE_GXX off?
	//
	// When specifying note + instr + tone portamento, and an old note is still playing (even after note off):
	// - If Compatible Gxx is on, the new note will be triggered only if the instrument _changes_.
	// - If Compatible Gxx is off, the new note will always be triggered, provided the instrument is specified.
	// - FT2 seems to do the latter (unconfirmed).

	// Err, wait. XM playback has its own code. The change made to the IT
	// playbackc code didn't affect XM playback. Forget this then. There's
	// still a bug in XM playback though, and it'll need some investigation...
	// tomorrow...

	// UPDATE: IT_COMPATIBLE_GXX is required to be on, so that tone porta has
	// separate memory from portamento.

	if (flags & XM_LINEAR_FREQUENCY)
		sigdata->flags |= IT_LINEAR_SLIDES;

	sigdata->global_volume = 128;
	sigdata->mixing_volume = 48;
	sigdata->pan_separation = 128;

	memset(sigdata->channel_volume, 64, DUMB_IT_N_CHANNELS);
	memset(sigdata->channel_pan, 32, DUMB_IT_N_CHANNELS);

	_dumb_it_fix_invalid_orders(sigdata);

	return sigdata;
}



#if 0 // no fucking way, dude!

/* The length returned is the time required to play from the beginning of the
 * file to the last row of the last order (which is when the player will
 * loop). Depending on the song, the sound might stop sooner.
 * Due to fixed point roundoffs, I think this is only reliable to the second.
 * Full precision could be achieved by using a double during the computation,
 * or maybe a LONG_LONG.
 */
long it_compute_length(const DUMB_IT_SIGDATA *sigdata)
{
	IT_PATTERN *pattern;
	int tempo, speed;
	int loop_start[IT_N_CHANNELS];
	char loop_count[IT_N_CHANNELS];
	int order, entry;
	int row_first_entry = 0;
	int jump, jump_dest;
	int delay, fine_delay;
	int i;
	long t;

	if (!sigdata)
		return 0;

	tempo = sigdata->tempo;
	speed = sigdata->speed;
	order = entry = 0;
	jump = jump_dest = 0;
	t = 0;

	/* for each PATTERN */
	for (order = 0; order < sigdata->n_orders; order++) {

		if (sigdata->order[order] == IT_ORDER_END) break;
		if (sigdata->order[order] == IT_ORDER_SKIP) continue;

		for (i = 0; i < IT_N_CHANNELS; i++)
			loop_count[i] = -1;

		pattern = &sigdata->pattern[ sigdata->order[order] ];
		entry = 0;
		if (jump == IT_BREAK_TO_ROW) {
			int row = 0;
			while (row < jump_dest)
				if (pattern->entry[entry++].channel >= IT_N_CHANNELS)
					row++;
		}

		/* for each ROW */
		while (entry < pattern->n_entries) {
			row_first_entry = entry;
			delay = fine_delay = 0;
			jump = 0;

			/* for each note NOTE */
			while (entry < pattern->n_entries && pattern->entry[entry].channel < IT_N_CHANNELS) {
				int value   = pattern->entry[entry].effectvalue;
				int channel = pattern->entry[entry].channel;

				switch (pattern->entry[entry].effect) {

					case IT_SET_SPEED: speed = value; break;

					case IT_JUMP_TO_ORDER:
						if (value <= order) /* infinite loop */
							return 0;
						jump = IT_JUMP_TO_ORDER;
						jump_dest = value;
						break;

					case IT_BREAK_TO_ROW:
						jump = IT_BREAK_TO_ROW;
						jump_dest = value;
						break;

					case IT_S:
						switch (HIGH(value)) {
							case IT_S_PATTERN_DELAY:      delay      = LOW(value); break;
							case IT_S_FINE_PATTERN_DELAY: fine_delay = LOW(value); break;
							case IT_S_PATTERN_LOOP:
								if (LOW(value) == 0) {
									loop_start[channel] = row_first_entry;
								} else {
									if (loop_count[channel] == -1)
										loop_count[channel] = LOW(value);

									if (loop_count[channel]) {
										jump = IT_S_PATTERN_LOOP;
										jump_dest = loop_start[channel];
									}
									loop_count[channel]--;
								}
								break;
						}
						break;

					case IT_SET_SONG_TEMPO:
						switch (HIGH(value)) { /* slides happen every non-row frames */
							case 0:  tempo = tempo - LOW(value)*(speed-1); break;
							case 1:  tempo = tempo + LOW(value)*(speed-1); break;
							default: tempo = value;
						}
						tempo = MID(32, tempo, 255);
						break;
				}

				entry++;
			}

			/* end of ROW */
			entry++;
			t += TICK_TIME_DIVIDEND * (speed*(1+delay) + fine_delay) / tempo;

			if (jump == IT_JUMP_TO_ORDER) {
				order = jump_dest - 1;
				break;
			} else if (jump == IT_BREAK_TO_ROW)
				break;
			else if (jump == IT_S_PATTERN_LOOP)
				entry = jump_dest - 1;
		}

		/* end of PATTERN */
	}

	return t;
}

#endif /* 0 */



DUH *dumb_read_xm_quick(DUMBFILE *f)
{
	sigdata_t *sigdata;

	DUH_SIGTYPE_DESC *descptr = &_dumb_sigtype_it;

	sigdata = it_xm_load_sigdata(f);

	if (!sigdata)
		return NULL;

	{
		const char *tag[1][2];
		tag[0][0] = "TITLE";
		tag[0][1] = ((DUMB_IT_SIGDATA *)sigdata)->name;
		return make_duh(-1, 1, (const char *const (*)[2])tag, 1, &descptr, &sigdata);
	}
}
