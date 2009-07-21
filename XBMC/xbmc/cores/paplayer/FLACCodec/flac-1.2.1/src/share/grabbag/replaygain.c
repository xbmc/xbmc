/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "share/grabbag.h"
#include "share/replaygain_analysis.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for chmod() */
#endif
#include <sys/stat.h> /* for stat(), maybe chmod() */

#ifdef local_min
#undef local_min
#endif
#define local_min(a,b) ((a)<(b)?(a):(b))

#ifdef local_max
#undef local_max
#endif
#define local_max(a,b) ((a)>(b)?(a):(b))

static const char *reference_format_ = "%s=%2.1f dB";
static const char *gain_format_ = "%s=%+2.2f dB";
static const char *peak_format_ = "%s=%1.8f";

static double album_peak_, title_peak_;

const unsigned GRABBAG__REPLAYGAIN_MAX_TAG_SPACE_REQUIRED = 190;
/*
	FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + 29 + 1 + 8 +
	FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + 21 + 1 + 10 +
	FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + 21 + 1 + 12 +
	FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + 21 + 1 + 10 +
	FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + 21 + 1 + 12
*/

const FLAC__byte * const GRABBAG__REPLAYGAIN_TAG_REFERENCE_LOUDNESS = (const FLAC__byte * const)"REPLAYGAIN_REFERENCE_LOUDNESS";
const FLAC__byte * const GRABBAG__REPLAYGAIN_TAG_TITLE_GAIN = (const FLAC__byte * const)"REPLAYGAIN_TRACK_GAIN";
const FLAC__byte * const GRABBAG__REPLAYGAIN_TAG_TITLE_PEAK = (const FLAC__byte * const)"REPLAYGAIN_TRACK_PEAK";
const FLAC__byte * const GRABBAG__REPLAYGAIN_TAG_ALBUM_GAIN = (const FLAC__byte * const)"REPLAYGAIN_ALBUM_GAIN";
const FLAC__byte * const GRABBAG__REPLAYGAIN_TAG_ALBUM_PEAK = (const FLAC__byte * const)"REPLAYGAIN_ALBUM_PEAK";


static FLAC__bool get_file_stats_(const char *filename, struct stat *stats)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);
	return (0 == stat(filename, stats));
}

static void set_file_stats_(const char *filename, struct stat *stats)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);

	(void)chmod(filename, stats->st_mode);
}

static FLAC__bool append_tag_(FLAC__StreamMetadata *block, const char *format, const FLAC__byte *name, float value)
{
	char buffer[256];
	char *saved_locale;
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != format);
	FLAC__ASSERT(0 != name);

	buffer[sizeof(buffer)-1] = '\0';
	/*
	 * We need to save the old locale and switch to "C" because the locale
	 * influences the formatting of %f and we want it a certain way.
	 */
	saved_locale = strdup(setlocale(LC_ALL, 0));
	if (0 == saved_locale)
		return false;
	setlocale(LC_ALL, "C");
#if defined _MSC_VER || defined __MINGW32__
	_snprintf(buffer, sizeof(buffer)-1, format, name, value);
#else
	snprintf(buffer, sizeof(buffer)-1, format, name, value);
#endif
	setlocale(LC_ALL, saved_locale);
	free(saved_locale);

	entry.entry = (FLAC__byte *)buffer;
	entry.length = strlen(buffer);

	return FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true);
}

FLAC__bool grabbag__replaygain_is_valid_sample_frequency(unsigned sample_frequency)
{
	static const unsigned valid_sample_rates[] = {
		8000,
		11025,
		12000,
		16000,
		22050,
		24000,
		32000,
		44100,
		48000
	};
	static const unsigned n_valid_sample_rates = sizeof(valid_sample_rates) / sizeof(valid_sample_rates[0]);

	unsigned i;

	for(i = 0; i < n_valid_sample_rates; i++)
		if(sample_frequency == valid_sample_rates[i])
			return true;
	return false;
}

FLAC__bool grabbag__replaygain_init(unsigned sample_frequency)
{
	title_peak_ = album_peak_ = 0.0;
	return InitGainAnalysis((long)sample_frequency) == INIT_GAIN_ANALYSIS_OK;
}

FLAC__bool grabbag__replaygain_analyze(const FLAC__int32 * const input[], FLAC__bool is_stereo, unsigned bps, unsigned samples)
{
	/* using a small buffer improves data locality; we'd like it to fit easily in the dcache */
	static Float_t lbuffer[2048], rbuffer[2048];
	static const unsigned nbuffer = sizeof(lbuffer) / sizeof(lbuffer[0]);
	FLAC__int32 block_peak = 0, s;
	unsigned i, j;

	FLAC__ASSERT(bps >= 4 && bps <= FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE);
	FLAC__ASSERT(FLAC__MIN_BITS_PER_SAMPLE == 4);
	/*
	 * We use abs() on a FLAC__int32 which is undefined for the most negative value.
	 * If the reference codec ever handles 32bps we will have to write a special
	 * case here.
	 */
	FLAC__ASSERT(FLAC__REFERENCE_CODEC_MAX_BITS_PER_SAMPLE < 32);

	if(bps == 16) {
		if(is_stereo) {
			j = 0;
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++, j++) {
					s = input[0][j];
					lbuffer[i] = (Float_t)s;
					s = abs(s);
					block_peak = local_max(block_peak, s);

					s = input[1][j];
					rbuffer[i] = (Float_t)s;
					s = abs(s);
					block_peak = local_max(block_peak, s);
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, rbuffer, n, 2) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
		else {
			j = 0;
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++, j++) {
					s = input[0][j];
					lbuffer[i] = (Float_t)s;
					s = abs(s);
					block_peak = local_max(block_peak, s);
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, 0, n, 1) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
	}
	else { /* bps must be < 32 according to above assertion */
		const double scale = (
			(bps > 16)?
				(double)1. / (double)(1u << (bps - 16)) :
				(double)(1u << (16 - bps))
		);

		if(is_stereo) {
			j = 0;
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++, j++) {
					s = input[0][j];
					lbuffer[i] = (Float_t)(scale * (double)s);
					s = abs(s);
					block_peak = local_max(block_peak, s);

					s = input[1][j];
					rbuffer[i] = (Float_t)(scale * (double)s);
					s = abs(s);
					block_peak = local_max(block_peak, s);
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, rbuffer, n, 2) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
		else {
			j = 0;
			while(samples > 0) {
				const unsigned n = local_min(samples, nbuffer);
				for(i = 0; i < n; i++, j++) {
					s = input[0][j];
					lbuffer[i] = (Float_t)(scale * (double)s);
					s = abs(s);
					block_peak = local_max(block_peak, s);
				}
				samples -= n;
				if(AnalyzeSamples(lbuffer, 0, n, 1) != GAIN_ANALYSIS_OK)
					return false;
			}
		}
	}

	{
		const double peak_scale = (double)(1u << (bps - 1));
		double peak = (double)block_peak / peak_scale;
		if(peak > title_peak_)
			title_peak_ = peak;
		if(peak > album_peak_)
			album_peak_ = peak;
	}

	return true;
}

void grabbag__replaygain_get_album(float *gain, float *peak)
{
	*gain = (float)GetAlbumGain();
	*peak = (float)album_peak_;
	album_peak_ = 0.0;
}

void grabbag__replaygain_get_title(float *gain, float *peak)
{
	*gain = (float)GetTitleGain();
	*peak = (float)title_peak_;
	title_peak_ = 0.0;
}


typedef struct {
	unsigned channels;
	unsigned bits_per_sample;
	unsigned sample_rate;
	FLAC__bool error;
} DecoderInstance;

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	DecoderInstance *instance = (DecoderInstance*)client_data;
	const unsigned bits_per_sample = frame->header.bits_per_sample;
	const unsigned channels = frame->header.channels;
	const unsigned sample_rate = frame->header.sample_rate;
	const unsigned samples = frame->header.blocksize;

	(void)decoder;

	if(
		!instance->error &&
		(channels == 2 || channels == 1) &&
		bits_per_sample == instance->bits_per_sample &&
		channels == instance->channels &&
		sample_rate == instance->sample_rate
	) {
		instance->error = !grabbag__replaygain_analyze(buffer, channels==2, bits_per_sample, samples);
	}
	else {
		instance->error = true;
	}

	if(!instance->error)
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	else
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	DecoderInstance *instance = (DecoderInstance*)client_data;

	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		instance->bits_per_sample = metadata->data.stream_info.bits_per_sample;
		instance->channels = metadata->data.stream_info.channels;
		instance->sample_rate = metadata->data.stream_info.sample_rate;

		if(instance->channels != 1 && instance->channels != 2) {
			instance->error = true;
			return;
		}

		if(!grabbag__replaygain_is_valid_sample_frequency(instance->sample_rate)) {
			instance->error = true;
			return;
		}
	}
}

static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	DecoderInstance *instance = (DecoderInstance*)client_data;

	(void)decoder, (void)status;

	instance->error = true;
}

const char *grabbag__replaygain_analyze_file(const char *filename, float *title_gain, float *title_peak)
{
	DecoderInstance instance;
	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();

	if(0 == decoder)
		return "memory allocation error";

	instance.error = false;

	/* It does these three by default but lets be explicit: */
	FLAC__stream_decoder_set_md5_checking(decoder, false);
	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);

	if(FLAC__stream_decoder_init_file(decoder, filename, write_callback_, metadata_callback_, error_callback_, &instance) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		FLAC__stream_decoder_delete(decoder);
		return "initializing decoder";
	}

	if(!FLAC__stream_decoder_process_until_end_of_stream(decoder) || instance.error) {
		FLAC__stream_decoder_delete(decoder);
		return "decoding file";
	}

	FLAC__stream_decoder_delete(decoder);

	grabbag__replaygain_get_title(title_gain, title_peak);

	return 0;
}

const char *grabbag__replaygain_store_to_vorbiscomment(FLAC__StreamMetadata *block, float album_gain, float album_peak, float title_gain, float title_peak)
{
	const char *error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_reference(block)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_title(block, title_gain, title_peak)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_album(block, album_gain, album_peak)))
		return error;

	return 0;
}

const char *grabbag__replaygain_store_to_vorbiscomment_reference(FLAC__StreamMetadata *block)
{
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if(FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, (const char *)GRABBAG__REPLAYGAIN_TAG_REFERENCE_LOUDNESS) < 0)
		return "memory allocation error";

	if(!append_tag_(block, reference_format_, GRABBAG__REPLAYGAIN_TAG_REFERENCE_LOUDNESS, ReplayGainReferenceLoudness))
		return "memory allocation error";

	return 0;
}

const char *grabbag__replaygain_store_to_vorbiscomment_album(FLAC__StreamMetadata *block, float album_gain, float album_peak)
{
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if(
		FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, (const char *)GRABBAG__REPLAYGAIN_TAG_ALBUM_GAIN) < 0 ||
		FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, (const char *)GRABBAG__REPLAYGAIN_TAG_ALBUM_PEAK) < 0
	)
		return "memory allocation error";

	if(
		!append_tag_(block, gain_format_, GRABBAG__REPLAYGAIN_TAG_ALBUM_GAIN, album_gain) ||
		!append_tag_(block, peak_format_, GRABBAG__REPLAYGAIN_TAG_ALBUM_PEAK, album_peak)
	)
		return "memory allocation error";

	return 0;
}

const char *grabbag__replaygain_store_to_vorbiscomment_title(FLAC__StreamMetadata *block, float title_gain, float title_peak)
{
	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	if(
		FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, (const char *)GRABBAG__REPLAYGAIN_TAG_TITLE_GAIN) < 0 ||
		FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, (const char *)GRABBAG__REPLAYGAIN_TAG_TITLE_PEAK) < 0
	)
		return "memory allocation error";

	if(
		!append_tag_(block, gain_format_, GRABBAG__REPLAYGAIN_TAG_TITLE_GAIN, title_gain) ||
		!append_tag_(block, peak_format_, GRABBAG__REPLAYGAIN_TAG_TITLE_PEAK, title_peak)
	)
		return "memory allocation error";

	return 0;
}

static const char *store_to_file_pre_(const char *filename, FLAC__Metadata_Chain **chain, FLAC__StreamMetadata **block)
{
	FLAC__Metadata_Iterator *iterator;
	const char *error;
	FLAC__bool found_vc_block = false;

	if(0 == (*chain = FLAC__metadata_chain_new()))
		return "memory allocation error";

	if(!FLAC__metadata_chain_read(*chain, filename)) {
		error = FLAC__Metadata_ChainStatusString[FLAC__metadata_chain_status(*chain)];
		FLAC__metadata_chain_delete(*chain);
		return error;
	}

	if(0 == (iterator = FLAC__metadata_iterator_new())) {
		FLAC__metadata_chain_delete(*chain);
		return "memory allocation error";
	}

	FLAC__metadata_iterator_init(iterator, *chain);

	do {
		*block = FLAC__metadata_iterator_get_block(iterator);
		if((*block)->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			found_vc_block = true;
	} while(!found_vc_block && FLAC__metadata_iterator_next(iterator));

	if(!found_vc_block) {
		/* create a new block */
		*block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
		if(0 == *block) {
			FLAC__metadata_chain_delete(*chain);
			FLAC__metadata_iterator_delete(iterator);
			return "memory allocation error";
		}
		while(FLAC__metadata_iterator_next(iterator))
			;
		if(!FLAC__metadata_iterator_insert_block_after(iterator, *block)) {
			error = FLAC__Metadata_ChainStatusString[FLAC__metadata_chain_status(*chain)];
			FLAC__metadata_chain_delete(*chain);
			FLAC__metadata_iterator_delete(iterator);
			return error;
		}
		/* iterator is left pointing to new block */
		FLAC__ASSERT(FLAC__metadata_iterator_get_block(iterator) == *block);
	}

	FLAC__metadata_iterator_delete(iterator);

	FLAC__ASSERT(0 != *block);
	FLAC__ASSERT((*block)->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	return 0;
}

static const char *store_to_file_post_(const char *filename, FLAC__Metadata_Chain *chain, FLAC__bool preserve_modtime)
{
	struct stat stats;
	const FLAC__bool have_stats = get_file_stats_(filename, &stats);

	(void)grabbag__file_change_stats(filename, /*read_only=*/false);

	FLAC__metadata_chain_sort_padding(chain);
	if(!FLAC__metadata_chain_write(chain, /*use_padding=*/true, preserve_modtime)) {
		FLAC__metadata_chain_delete(chain);
		return FLAC__Metadata_ChainStatusString[FLAC__metadata_chain_status(chain)];
	}

	FLAC__metadata_chain_delete(chain);

	if(have_stats)
		set_file_stats_(filename, &stats);

	return 0;
}

const char *grabbag__replaygain_store_to_file(const char *filename, float album_gain, float album_peak, float title_gain, float title_peak, FLAC__bool preserve_modtime)
{
	FLAC__Metadata_Chain *chain;
	FLAC__StreamMetadata *block;
	const char *error;

	if(0 != (error = store_to_file_pre_(filename, &chain, &block)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment(block, album_gain, album_peak, title_gain, title_peak))) {
		FLAC__metadata_chain_delete(chain);
		return error;
	}

	if(0 != (error = store_to_file_post_(filename, chain, preserve_modtime)))
		return error;

	return 0;
}

const char *grabbag__replaygain_store_to_file_reference(const char *filename, FLAC__bool preserve_modtime)
{
	FLAC__Metadata_Chain *chain;
	FLAC__StreamMetadata *block;
	const char *error;

	if(0 != (error = store_to_file_pre_(filename, &chain, &block)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_reference(block))) {
		FLAC__metadata_chain_delete(chain);
		return error;
	}

	if(0 != (error = store_to_file_post_(filename, chain, preserve_modtime)))
		return error;

	return 0;
}

const char *grabbag__replaygain_store_to_file_album(const char *filename, float album_gain, float album_peak, FLAC__bool preserve_modtime)
{
	FLAC__Metadata_Chain *chain;
	FLAC__StreamMetadata *block;
	const char *error;

	if(0 != (error = store_to_file_pre_(filename, &chain, &block)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_album(block, album_gain, album_peak))) {
		FLAC__metadata_chain_delete(chain);
		return error;
	}

	if(0 != (error = store_to_file_post_(filename, chain, preserve_modtime)))
		return error;

	return 0;
}

const char *grabbag__replaygain_store_to_file_title(const char *filename, float title_gain, float title_peak, FLAC__bool preserve_modtime)
{
	FLAC__Metadata_Chain *chain;
	FLAC__StreamMetadata *block;
	const char *error;

	if(0 != (error = store_to_file_pre_(filename, &chain, &block)))
		return error;

	if(0 != (error = grabbag__replaygain_store_to_vorbiscomment_title(block, title_gain, title_peak))) {
		FLAC__metadata_chain_delete(chain);
		return error;
	}

	if(0 != (error = store_to_file_post_(filename, chain, preserve_modtime)))
		return error;

	return 0;
}

static FLAC__bool parse_double_(const FLAC__StreamMetadata_VorbisComment_Entry *entry, double *val)
{
	char s[32], *end;
	const char *p, *q;
	double v;

	FLAC__ASSERT(0 != entry);
	FLAC__ASSERT(0 != val);

	p = (const char *)entry->entry;
	q = strchr(p, '=');
	if(0 == q)
		return false;
	q++;
	memset(s, 0, sizeof(s)-1);
	strncpy(s, q, local_min(sizeof(s)-1, entry->length - (q-p)));

	v = strtod(s, &end);
	if(end == s)
		return false;

	*val = v;
	return true;
}

FLAC__bool grabbag__replaygain_load_from_vorbiscomment(const FLAC__StreamMetadata *block, FLAC__bool album_mode, FLAC__bool strict, double *reference, double *gain, double *peak)
{
	int reference_offset, gain_offset, peak_offset;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(0 != reference);
	FLAC__ASSERT(0 != gain);
	FLAC__ASSERT(0 != peak);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);

	/* Default to current level until overridden by a detected tag; this
	 * will always be true until we change replaygain_analysis.c
	 */
	*reference = ReplayGainReferenceLoudness;

	if(0 <= (reference_offset = FLAC__metadata_object_vorbiscomment_find_entry_from(block, /*offset=*/0, (const char *)GRABBAG__REPLAYGAIN_TAG_REFERENCE_LOUDNESS)))
		(void)parse_double_(block->data.vorbis_comment.comments + reference_offset, reference);

	if(0 > (gain_offset = FLAC__metadata_object_vorbiscomment_find_entry_from(block, /*offset=*/0, (const char *)(album_mode? GRABBAG__REPLAYGAIN_TAG_ALBUM_GAIN : GRABBAG__REPLAYGAIN_TAG_TITLE_GAIN))))
		return !strict && grabbag__replaygain_load_from_vorbiscomment(block, !album_mode, /*strict=*/true, reference, gain, peak);
	if(0 > (peak_offset = FLAC__metadata_object_vorbiscomment_find_entry_from(block, /*offset=*/0, (const char *)(album_mode? GRABBAG__REPLAYGAIN_TAG_ALBUM_PEAK : GRABBAG__REPLAYGAIN_TAG_TITLE_PEAK))))
		return !strict && grabbag__replaygain_load_from_vorbiscomment(block, !album_mode, /*strict=*/true, reference, gain, peak);

	if(!parse_double_(block->data.vorbis_comment.comments + gain_offset, gain))
		return !strict && grabbag__replaygain_load_from_vorbiscomment(block, !album_mode, /*strict=*/true, reference, gain, peak);
	if(!parse_double_(block->data.vorbis_comment.comments + peak_offset, peak))
		return !strict && grabbag__replaygain_load_from_vorbiscomment(block, !album_mode, /*strict=*/true, reference, gain, peak);

	return true;
}

double grabbag__replaygain_compute_scale_factor(double peak, double gain, double preamp, FLAC__bool prevent_clipping)
{
	double scale;
	FLAC__ASSERT(peak >= 0.0);
 	gain += preamp;
	scale = (float) pow(10.0, gain * 0.05);
	if(prevent_clipping && peak > 0.0) {
		const double max_scale = (float)(1.0 / peak);
		if(scale > max_scale)
			scale = max_scale;
	}
	return scale;
}
