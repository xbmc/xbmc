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
 * reads3m.c - Code to read a ScreamTracker 3         / / \  \
 *             module from an open file.             | <  /   \_
 *                                                   |  \/ /\   /
 * By entheh.                                         \_  /  > /
 *                                                      | \ / /
 *                                                      |  ' /
 *                                                       \__/
 */

// IT_STEREO... :o
#include <stdlib.h>
#include <string.h>

#include "dumb.h"
#include "internal/it.h"



/** WARNING: this is duplicated in itread.c */
static int it_seek(DUMBFILE *f, long offset)
{
	long pos = dumbfile_pos(f);

	if (pos > offset)
		return -1;

	if (pos < offset)
		if (dumbfile_skip(f, offset - pos))
			return -1;

	return 0;
}



static int it_s3m_read_sample_header(IT_SAMPLE *sample, long *offset, DUMBFILE *f)
{
	unsigned char type;
	int flags;

	type = dumbfile_getc(f);

	if (type > 1) {
		/** WARNING: no adlib support */
	}

	dumbfile_getnc(sample->filename, 13, f);
	sample->filename[13] = 0;

	*offset = dumbfile_igetw(f) << 4;

	sample->length = dumbfile_igetl(f);
	sample->loop_start = dumbfile_igetl(f);
	sample->loop_end = dumbfile_igetl(f);

	sample->default_volume = dumbfile_getc(f);

	dumbfile_skip(f, 1);

	if (dumbfile_getc(f) != 0)
		/* Sample is packed apparently (or error reading from file). We don't
		 * know how to read packed samples.
		 */
		return -1;

	flags = dumbfile_getc(f);

	sample->C5_speed = dumbfile_igetl(f) << 1;

	/* Skip four unused bytes and three internal variables. */
	dumbfile_skip(f, 4+2+2+4);

	dumbfile_getnc(sample->name, 28, f);
	sample->name[28] = 0;

	if (type == 0) {
		/* Looks like no-existy. Anyway, there's for sure no 'SCRS' ... */
		sample->flags &= ~IT_SAMPLE_EXISTS;
		return dumbfile_error(f);
	}

	if (dumbfile_mgetl(f) != DUMB_ID('S','C','R','S'))
		return -1;

	sample->global_volume = 64;

	sample->flags = IT_SAMPLE_EXISTS;
	if (flags & 1) sample->flags |= IT_SAMPLE_LOOP;
	if (flags & 2) sample->flags |= IT_SAMPLE_STEREO;
	if (flags & 4) sample->flags |= IT_SAMPLE_16BIT;

	sample->default_pan = 0; // 0 = don't use, or 160 = centre?

	if (sample->length <= 0)
		sample->flags &= ~IT_SAMPLE_EXISTS;
	else if (sample->flags & IT_SAMPLE_LOOP) {
		if ((unsigned int)sample->loop_end > (unsigned int)sample->length)
			sample->flags &= ~IT_SAMPLE_LOOP;
		else if ((unsigned int)sample->loop_start >= (unsigned int)sample->loop_end)
			sample->flags &= ~IT_SAMPLE_LOOP;
		else
			/* ScreamTracker seems not to save what comes after the loop end
			 * point, but rather to assume it is a duplicate of what comes at
			 * the loop start point. I am not completely sure of this though.
			 * It is easy to evade; simply truncate the sample.
			 */
			sample->length = sample->loop_end;
	}


	//Do we need to set all these?
	sample->vibrato_speed = 0;
	sample->vibrato_depth = 0;
	sample->vibrato_rate = 0;
	sample->vibrato_waveform = IT_VIBRATO_SINE;

	return dumbfile_error(f);
}



static int it_s3m_read_sample_data(IT_SAMPLE *sample, int ffi, DUMBFILE *f)
{
	long n;

	long datasize = sample->length;
	if (sample->flags & IT_SAMPLE_STEREO) datasize <<= 1;

	sample->data = malloc(datasize * (sample->flags & IT_SAMPLE_16BIT ? 2 : 1));
	if (!sample->data)
		return -1;

	if (sample->flags & IT_SAMPLE_STEREO) {
		if (sample->flags & IT_SAMPLE_16BIT) {
			for (n = 0; n < datasize; n += 2)
				((short *)sample->data)[n] = dumbfile_igetw(f);
			for (n = 1; n < datasize; n += 2)
				((short *)sample->data)[n] = dumbfile_igetw(f);
		} else {
			for (n = 0; n < datasize; n += 2)
				((signed char *)sample->data)[n] = dumbfile_getc(f);
			for (n = 1; n < datasize; n += 2)
				((signed char *)sample->data)[n] = dumbfile_getc(f);
		}
	} else if (sample->flags & IT_SAMPLE_16BIT)
		for (n = 0; n < sample->length; n++)
			((short *)sample->data)[n] = dumbfile_igetw(f);
	else
		for (n = 0; n < sample->length; n++)
			((signed char *)sample->data)[n] = dumbfile_getc(f);

	if (dumbfile_error(f))
		return -1;

	if (ffi != 1) {
		/* Convert to signed. */
		if (sample->flags & IT_SAMPLE_16BIT)
			for (n = 0; n < datasize; n++)
				((short *)sample->data)[n] ^= 0x8000;
		else
			for (n = 0; n < datasize; n++)
				((signed char *)sample->data)[n] ^= 0x80;
	}

	return 0;
}



static int it_s3m_read_pattern(IT_PATTERN *pattern, DUMBFILE *f, unsigned char *buffer)
{
	int buflen = 0;
	int bufpos = 0;

	IT_ENTRY *entry;

	unsigned char channel;

	/* Haha, this is hilarious!
	 *
	 * Well, after some experimentation, it seems that different S3M writers
	 * define the format in different ways. The S3M docs say that the first
	 * two bytes hold the "length of [the] packed pattern", and the packed
	 * pattern data follow. Judging by the contents of ARMANI.S3M, packaged
	 * with ScreamTracker itself, the measure of length _includes_ the two
	 * bytes used to store the length; in other words, we should read
	 * (length - 2) more bytes. However, aryx.s3m, packaged with ModPlug
	 * Tracker, excludes these two bytes, so (length) more bytes must be
	 * read.
	 *
	 * Call me crazy, but I just find it insanely funny that the format was
	 * misunderstood in this way :D
	 *
	 * Now we can't just risk reading two extra bytes, because then we
	 * overshoot, and DUMBFILEs don't support backward seeking (for a good
	 * reason). Luckily, there is a way. We can read the data little by
	 * little, and stop when we have 64 rows in memory. Provided we protect
	 * against buffer overflow, this method should work with all sensibly
	 * written S3M files. If you find one for which it does not work, please
	 * let me know at entheh@users.sf.net so I can look at it.
	 */

	/* Discard the length. */
	dumbfile_skip(f, 2);

	if (dumbfile_error(f))
		return -1;

	pattern->n_rows = 0;
	pattern->n_entries = 0;

	/* Read in the pattern data, little by little, and work out how many
	 * entries we need room for. Sorry, but this is just so funny...
	 */
	for (;;) {
		unsigned char b = buffer[buflen++] = dumbfile_getc(f);

#if 1
		static const unsigned char used[8] = {0, 2, 1, 3, 2, 4, 3, 5};
		channel = b & 31;
		b >>= 5;
		pattern->n_entries++;
		if (b) {
			if (buflen + used[b] >= 65536) return -1;
			dumbfile_getnc(buffer + buflen, used[b], f);
			buflen += used[b];
		} else {
			/* End of row */
			if (++pattern->n_rows == 64) break;
			if (buflen >= 65536) return -1;
		}
#else
		if (b == 0) {
			/* End of row */
			pattern->n_entries++;
			if (++pattern->n_rows == 64) break;
			if (buflen >= 65536) return -1;
		} else {
			static const unsigned char used[8] = {0, 2, 1, 3, 2, 4, 3, 5};
			channel = b & 31;
			b >>= 5;
			if (b) {
				pattern->n_entries++;
				if (buflen + used[b] >= 65536) return -1;
				dumbfile_getnc(buffer + buflen, used[b], f);
				buflen += used[b];
			}
		}
#endif

		/* We have ensured that buflen < 65536 at this point, so it is safe
		 * to iterate and read at least one more byte without checking.
		 * However, now would be a good time to check for errors reading from
		 * the file.
		 */

		if (dumbfile_error(f))
			return -1;
	}

	pattern->entry = malloc(pattern->n_entries * sizeof(*pattern->entry));

	if (!pattern->entry)
		return -1;

	entry = pattern->entry;

	while (bufpos < buflen) {
		unsigned char b = buffer[bufpos++];

#if 1
		if (!(b & ~31))
#else
		if (b == 0)
#endif
		{
			/* End of row */
			IT_SET_END_ROW(entry);
			entry++;
			continue;
		}

		channel = b & 31;

		if (b & 224) {
			entry->mask = 0;
			entry->channel = channel;

			if (b & 32) {
				unsigned char n = buffer[bufpos++];
				if (n != 255) {
					if (n == 254)
						entry->note = IT_NOTE_CUT;
					else
						entry->note = (n >> 4) * 12 + (n & 15);
					entry->mask |= IT_ENTRY_NOTE;
				}

				entry->instrument = buffer[bufpos++];
				if (entry->instrument)
					entry->mask |= IT_ENTRY_INSTRUMENT;
			}

			if (b & 64) {
				entry->volpan = buffer[bufpos++];
				if (entry->volpan != 255)
					entry->mask |= IT_ENTRY_VOLPAN;
			}

			if (b & 128) {
				entry->effect = buffer[bufpos++];
				entry->effectvalue = buffer[bufpos++];
				if (entry->effect != 255) {
					entry->mask |= IT_ENTRY_EFFECT;
					if (entry->effect == IT_BREAK_TO_ROW)
						entry->effectvalue -= (entry->effectvalue >> 4) * 6;
				}
				/** WARNING: ARGH! CONVERT TEH EFFECTS!@~ */
			}

			entry++;
		}
	}

	ASSERT(entry == pattern->entry + pattern->n_entries);

	return 0;
}



/** WARNING: this is duplicated in itread.c - also bad practice to use the same struct name unless they are unified in a header */
/* Currently we assume the sample data are stored after the sample headers in
 * module files. This assumption may be unjustified; let me know if you have
 * trouble.
 */

#define IT_COMPONENT_INSTRUMENT 1
#define IT_COMPONENT_PATTERN    2
#define IT_COMPONENT_SAMPLE     3

typedef struct IT_COMPONENT
{
	unsigned char type;
	unsigned char n;
	long offset;
	short sampfirst; /* component[sampfirst] = first sample data after this */
	short sampnext; /* sampnext is used to create linked lists of sample data */
}
IT_COMPONENT;



static int it_component_compare(const void *e1, const void *e2)
{
	return ((const IT_COMPONENT *)e1)->offset -
	       ((const IT_COMPONENT *)e2)->offset;
}



static DUMB_IT_SIGDATA *it_s3m_load_sigdata(DUMBFILE *f)
{
	DUMB_IT_SIGDATA *sigdata;

	int flags, cwtv, ffi;
	int default_pan_present;

	IT_COMPONENT *component;
	int n_components = 0;

	int n;

	unsigned char *buffer;

	sigdata = malloc(sizeof(*sigdata));
	if (!sigdata) return NULL;

	dumbfile_getnc(sigdata->name, 28, f);
	sigdata->name[28] = 0;

	if (dumbfile_getc(f) != 0x1A || dumbfile_getc(f) != 16) {
		free(sigdata);
		return NULL;
	}

	dumbfile_skip(f, 2);

	sigdata->song_message = NULL;
	sigdata->order = NULL;
	sigdata->instrument = NULL;
	sigdata->sample = NULL;
	sigdata->pattern = NULL;
	sigdata->midi = NULL;
	sigdata->checkpoint = NULL;

	sigdata->n_orders = dumbfile_igetw(f);
	sigdata->n_instruments = 0;
	sigdata->n_samples = dumbfile_igetw(f);
	sigdata->n_patterns = dumbfile_igetw(f);

	if (dumbfile_error(f) || sigdata->n_orders <= 0 || sigdata->n_samples > 256 || sigdata->n_patterns > 256) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	sigdata->order = malloc(sigdata->n_orders);
	if (!sigdata->order) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	if (sigdata->n_samples) {
		sigdata->sample = malloc(sigdata->n_samples * sizeof(*sigdata->sample));
		if (!sigdata->sample) {
			_dumb_it_unload_sigdata(sigdata);
			return NULL;
		}
		for (n = 0; n < sigdata->n_samples; n++)
			sigdata->sample[n].data = NULL;
	}

	if (sigdata->n_patterns) {
		sigdata->pattern = malloc(sigdata->n_patterns * sizeof(*sigdata->pattern));
		if (!sigdata->pattern) {
			_dumb_it_unload_sigdata(sigdata);
			return NULL;
		}
		for (n = 0; n < sigdata->n_patterns; n++)
			sigdata->pattern[n].entry = NULL;
	}

	flags = dumbfile_igetw(f);

	cwtv = dumbfile_igetw(f);

	if (cwtv == 0x1300) {
		/** WARNING: volume slides on every frame */
	}

	ffi = dumbfile_igetw(f);

	/** WARNING: which ones? */
	sigdata->flags = IT_STEREO | IT_OLD_EFFECTS | IT_COMPATIBLE_GXX;

	if (dumbfile_mgetl(f) != DUMB_ID('S','C','R','M')) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	sigdata->global_volume = dumbfile_getc(f) << 1;
	sigdata->speed = dumbfile_getc(f);
	if (sigdata->speed == 0) sigdata->speed = 6; // Should we? What about tempo?
	sigdata->tempo = dumbfile_getc(f);
	/*master_volume = */dumbfile_getc(f); // 7 bits; +128 for stereo
	//what do we do with master_volume? it's not the same as mixing volume...
	sigdata->mixing_volume = 48;

	/* Skip GUS Ultra Click Removal byte. */
	dumbfile_getc(f);

	default_pan_present = dumbfile_getc(f);

	dumbfile_skip(f, 8);

	/* Skip Special Custom Data Pointer. */
	/** WARNING: investigate this? */
	dumbfile_igetw(f);

	/* Channel settings for 32 channels, 255=unused, +128=disabled */
	{
		int i;
		for (i = 0; i < 32; i++) {
			int c = dumbfile_getc(f);
			if (!(c & (128 | 16))) { /* +128=disabled, +16=Adlib */
				sigdata->channel_volume[i] = 64;
				sigdata->channel_pan[i] = c & 8 ? 12 : 3;
				/** WARNING: ah, but it should be 7 for mono... */
			} else {
				/** WARNING: this could be improved if we support channel muting... */
				sigdata->channel_volume[i] = 0;
				sigdata->channel_pan[i] = 7;
			}
		}
	}

	/* Orders, byte each, length = sigdata->n_orders (should be even) */
	dumbfile_getnc(sigdata->order, sigdata->n_orders, f);
	sigdata->restart_position = 0;

	component = malloc(768*sizeof(*component));
	if (!component) {
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	for (n = 0; n < sigdata->n_samples; n++) {
		component[n_components].type = IT_COMPONENT_SAMPLE;
		component[n_components].n = n;
		component[n_components].offset = dumbfile_igetw(f) << 4;
		component[n_components].sampfirst = -1;
		n_components++;
	}

	for (n = 0; n < sigdata->n_patterns; n++) {
		long offset = dumbfile_igetw(f) << 4;
		if (offset) {
			component[n_components].type = IT_COMPONENT_PATTERN;
			component[n_components].n = n;
			component[n_components].offset = offset;
			component[n_components].sampfirst = -1;
			n_components++;
		} else {
			/** WARNING: Empty 64-row pattern ... ? (this does happen!) */
			sigdata->pattern[n].n_rows = 64;
			sigdata->pattern[n].n_entries = 0;
		}
	}

	qsort(component, n_components, sizeof(IT_COMPONENT), &it_component_compare);

	/* I found a really dumb S3M file that claimed to contain default pan
	 * data but didn't contain any. Programs would load it by reading part of
	 * the first instrument header, assuming the data to be default pan
	 * positions, and then rereading the instrument module. We cannot do this
	 * without obfuscating the file input model, so we insert an extra check
	 * here that we won't overrun the start of the first component.
	 */
	if (default_pan_present == 252 && component[0].offset >= dumbfile_pos(f) + 32) {
		/* Channel default pan positions */
		int i;
		for (i = 0; i < 32; i++) {
			int c = dumbfile_getc(f);
			if (c & 32)
				sigdata->channel_pan[i] = c & 15;
		}
	}

	{
		int i;
		for (i = 0; i < 32; i++) {
			sigdata->channel_pan[i] -= (sigdata->channel_pan[i] & 8) >> 3;
			sigdata->channel_pan[i] = ((int)sigdata->channel_pan[i] << 5) / 7;
		}
	}

	sigdata->pan_separation = 128;

	if (dumbfile_error(f)) {
		free(component);
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	buffer = malloc(65536);
	if (!buffer) {
		free(component);
		_dumb_it_unload_sigdata(sigdata);
		return NULL;
	}

	for (n = 0; n < n_components; n++) {
		long offset;
		int m;

		if (it_seek(f, component[n].offset)) {
			free(buffer);
			free(component);
			_dumb_it_unload_sigdata(sigdata);
			return NULL;
		}

		switch (component[n].type) {

			case IT_COMPONENT_PATTERN:
				if (it_s3m_read_pattern(&sigdata->pattern[component[n].n], f, buffer)) {
					free(buffer);
					free(component);
					_dumb_it_unload_sigdata(sigdata);
					return NULL;
				}
				break;

			case IT_COMPONENT_SAMPLE:
				if (it_s3m_read_sample_header(&sigdata->sample[component[n].n], &offset, f)) {
					free(buffer);
					free(component);
					_dumb_it_unload_sigdata(sigdata);
					return NULL;
				}

				if (sigdata->sample[component[n].n].flags & IT_SAMPLE_EXISTS) {
					short *sample;

					for (m = n + 1; m < n_components; m++)
						if (component[m].offset > offset)
							break;
					m--;

					sample = &component[m].sampfirst;

					while (*sample >= 0 && component[*sample].offset <= offset)
						sample = &component[*sample].sampnext;

					component[n].sampnext = *sample;
					*sample = n;

					component[n].offset = offset;
				}
		}

		m = component[n].sampfirst;

		while (m >= 0) {
			if (it_seek(f, component[m].offset)) {
				free(buffer);
				free(component);
				_dumb_it_unload_sigdata(sigdata);
				return NULL;
			}

			if (it_s3m_read_sample_data(&sigdata->sample[component[m].n], ffi, f)) {
				free(buffer);
				free(component);
				_dumb_it_unload_sigdata(sigdata);
				return NULL;
			}

			m = component[m].sampnext;
		}
	}

	free(buffer);
	free(component);

	_dumb_it_fix_invalid_orders(sigdata);

	return sigdata;
}



DUH *dumb_read_s3m_quick(DUMBFILE *f)
{
	sigdata_t *sigdata;

	DUH_SIGTYPE_DESC *descptr = &_dumb_sigtype_it;

	sigdata = it_s3m_load_sigdata(f);

	if (!sigdata)
		return NULL;

	{
		const char *tag[1][2];
		tag[0][0] = "TITLE";
		tag[0][1] = ((DUMB_IT_SIGDATA *)sigdata)->name;
		return make_duh(-1, 1, (const char *const (*)[2])tag, 1, &descptr, &sigdata);
	}
}
