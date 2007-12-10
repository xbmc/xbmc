/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "utils.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *CHANNEL_MASK_TAG = "WAVEFORMATEXTENSIBLE_CHANNEL_MASK";

int flac__utils_verbosity_ = 2;

static FLAC__bool local__parse_uint64_(const char *s, FLAC__uint64 *value)
{
	FLAC__uint64 ret = 0;
	char c;

	if(*s == '\0')
		return false;

	while('\0' != (c = *s++))
		if(c >= '0' && c <= '9')
			ret = ret * 10 + (c - '0');
		else
			return false;

	*value = ret;
	return true;
}

static FLAC__bool local__parse_timecode_(const char *s, double *value)
{
	double ret;
	unsigned i;
	char c;

	/* parse [0-9][0-9]*: */
	c = *s++;
	if(c >= '0' && c <= '9')
		i = (c - '0');
	else
		return false;
	while(':' != (c = *s++)) {
		if(c >= '0' && c <= '9')
			i = i * 10 + (c - '0');
		else
			return false;
	}
	ret = (double)i * 60.;

	/* parse [0-9]*[.,]?[0-9]* i.e. a sign-less rational number (. or , OK for fractional seconds, to support different locales) */
	if(strspn(s, "1234567890.,") != strlen(s))
		return false;
	{
		const char *p = strpbrk(s, ".,");
		if(p && 0 != strpbrk(++p, ".,"))
			return false;
	}
	ret += atof(s);

	*value = ret;
	return true;
}

static FLAC__bool local__parse_cue_(const char *s, const char *end, unsigned *track, unsigned *index)
{
	FLAC__bool got_track = false, got_index = false;
	unsigned t = 0, i = 0;
	char c;

	while(end? s < end : *s != '\0') {
		c = *s++;
		if(c >= '0' && c <= '9') {
			t = t * 10 + (c - '0');
			got_track = true;
		}
		else if(c == '.')
			break;
		else
			return false;
	}
	while(end? s < end : *s != '\0') {
		c = *s++;
		if(c >= '0' && c <= '9') {
			i = i * 10 + (c - '0');
			got_index = true;
		}
		else
			return false;
	}
	*track = t;
	*index = i;
	return got_track && got_index;
}

/*
 * this only works with sorted cuesheets (the spec strongly recommends but
 * does not require sorted cuesheets).  but if it's not sorted, picking a
 * nearest cue point has no significance.
 */
static FLAC__uint64 local__find_closest_cue_(const FLAC__StreamMetadata_CueSheet *cuesheet, unsigned track, unsigned index, FLAC__uint64 total_samples, FLAC__bool look_forward)
{
	int t, i;
	if(look_forward) {
		for(t = 0; t < (int)cuesheet->num_tracks; t++)
			for(i = 0; i < (int)cuesheet->tracks[t].num_indices; i++)
				if(cuesheet->tracks[t].number > track || (cuesheet->tracks[t].number == track && cuesheet->tracks[t].indices[i].number >= index))
					return cuesheet->tracks[t].offset + cuesheet->tracks[t].indices[i].offset;
		return total_samples;
	}
	else {
		for(t = (int)cuesheet->num_tracks - 1; t >= 0; t--)
			for(i = (int)cuesheet->tracks[t].num_indices - 1; i >= 0; i--)
				if(cuesheet->tracks[t].number < track || (cuesheet->tracks[t].number == track && cuesheet->tracks[t].indices[i].number <= index))
					return cuesheet->tracks[t].offset + cuesheet->tracks[t].indices[i].offset;
		return 0;
	}
}

void flac__utils_printf(FILE *stream, int level, const char *format, ...)
{
	if(flac__utils_verbosity_ >= level) {
		va_list args;

		FLAC__ASSERT(0 != format);

		va_start(args, format);

		(void) vfprintf(stream, format, args);

		va_end(args);
	}
}

#ifdef FLAC__VALGRIND_TESTING
size_t flac__utils_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if(!ferror(stream))
		fflush(stream);
	return ret;
}
#endif

FLAC__bool flac__utils_parse_skip_until_specification(const char *s, utils__SkipUntilSpecification *spec)
{
	FLAC__uint64 val;
	FLAC__bool is_negative = false;

	FLAC__ASSERT(0 != spec);

	spec->is_relative = false;
	spec->value_is_samples = true;
	spec->value.samples = 0;

	if(0 != s) {
		if(s[0] == '-') {
			is_negative = true;
			spec->is_relative = true;
			s++;
		}
		else if(s[0] == '+') {
			spec->is_relative = true;
			s++;
		}

		if(local__parse_uint64_(s, &val)) {
			spec->value_is_samples = true;
			spec->value.samples = (FLAC__int64)val;
			if(is_negative)
				spec->value.samples = -(spec->value.samples);
		}
		else {
			double d;
			if(!local__parse_timecode_(s, &d))
				return false;
			spec->value_is_samples = false;
			spec->value.seconds = d;
			if(is_negative)
				spec->value.seconds = -(spec->value.seconds);
		}
	}

	return true;
}

void flac__utils_canonicalize_skip_until_specification(utils__SkipUntilSpecification *spec, unsigned sample_rate)
{
	FLAC__ASSERT(0 != spec);
	if(!spec->value_is_samples) {
		spec->value.samples = (FLAC__int64)(spec->value.seconds * (double)sample_rate);
		spec->value_is_samples = true;
	}
}

FLAC__bool flac__utils_parse_cue_specification(const char *s, utils__CueSpecification *spec)
{
	const char *start = s, *end = 0;

	FLAC__ASSERT(0 != spec);

	spec->has_start_point = spec->has_end_point = false;

	s = strchr(s, '-');

	if(0 != s) {
		if(s == start)
			start = 0;
		end = s+1;
		if(*end == '\0')
			end = 0;
	}

	if(start) {
		if(!local__parse_cue_(start, s, &spec->start_track, &spec->start_index))
			return false;
		spec->has_start_point = true;
	}

	if(end) {
		if(!local__parse_cue_(end, 0, &spec->end_track, &spec->end_index))
			return false;
		spec->has_end_point = true;
	}

	return true;
}

void flac__utils_canonicalize_cue_specification(const utils__CueSpecification *cue_spec, const FLAC__StreamMetadata_CueSheet *cuesheet, FLAC__uint64 total_samples, utils__SkipUntilSpecification *skip_spec, utils__SkipUntilSpecification *until_spec)
{
	FLAC__ASSERT(0 != cue_spec);
	FLAC__ASSERT(0 != cuesheet);
	FLAC__ASSERT(0 != total_samples);
	FLAC__ASSERT(0 != skip_spec);
	FLAC__ASSERT(0 != until_spec);

	skip_spec->is_relative = false;
	skip_spec->value_is_samples = true;

	until_spec->is_relative = false;
	until_spec->value_is_samples = true;

	if(cue_spec->has_start_point)
		skip_spec->value.samples = local__find_closest_cue_(cuesheet, cue_spec->start_track, cue_spec->start_index, total_samples, /*look_forward=*/false);
	else
		skip_spec->value.samples = 0;

	if(cue_spec->has_end_point)
		until_spec->value.samples = local__find_closest_cue_(cuesheet, cue_spec->end_track, cue_spec->end_index, total_samples, /*look_forward=*/true);
	else
		until_spec->value.samples = total_samples;
}

FLAC__bool flac__utils_set_channel_mask_tag(FLAC__StreamMetadata *object, FLAC__uint32 channel_mask)
{
	FLAC__StreamMetadata_VorbisComment_Entry entry = { 0, 0 };
	char tag[128];

	FLAC__ASSERT(object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(strlen(CHANNEL_MASK_TAG+1+2+16+1) <= sizeof(tag)); /* +1 for =, +2 for 0x, +16 for digits, +1 for NUL */
	entry.entry = (FLAC__byte*)tag;
#if defined _MSC_VER || defined __MINGW32__
	if((entry.length = _snprintf(tag, sizeof(tag), "%s=0x%04X", CHANNEL_MASK_TAG, (unsigned)channel_mask)) >= sizeof(tag))
#else
	if((entry.length = snprintf(tag, sizeof(tag), "%s=0x%04X", CHANNEL_MASK_TAG, (unsigned)channel_mask)) >= sizeof(tag))
#endif
		return false;
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(object, entry, /*all=*/true, /*copy=*/true))
		return false;
	return true;
}

FLAC__bool flac__utils_get_channel_mask_tag(const FLAC__StreamMetadata *object, FLAC__uint32 *channel_mask)
{
	int offset;
	unsigned val;
	char *p;
	FLAC__ASSERT(object);
	FLAC__ASSERT(object->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 > (offset = FLAC__metadata_object_vorbiscomment_find_entry_from(object, /*offset=*/0, CHANNEL_MASK_TAG)))
		return false;
	if(object->data.vorbis_comment.comments[offset].length < strlen(CHANNEL_MASK_TAG)+4)
		return false;
	if(0 == (p = strchr((const char *)object->data.vorbis_comment.comments[offset].entry, '='))) /* should never happen, but just in case */
		return false;
	if(strncmp(p, "=0x", 3))
		return false;
	if(sscanf(p+3, "%x", &val) != 1)
		return false;
	*channel_mask = val;
	return true;
}
