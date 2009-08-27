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
#include "FLAC/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned grabbag__cuesheet_msf_to_frame(unsigned minutes, unsigned seconds, unsigned frames)
{
	return ((minutes * 60) + seconds) * 75 + frames;
}

void grabbag__cuesheet_frame_to_msf(unsigned frame, unsigned *minutes, unsigned *seconds, unsigned *frames)
{
	*frames = frame % 75;
	frame /= 75;
	*seconds = frame % 60;
	frame /= 60;
	*minutes = frame;
}

/* since we only care about values >= 0 or error, returns < 0 for any illegal string, else value */
static int local__parse_int_(const char *s)
{
	int ret = 0;
	char c;

	if(*s == '\0')
		return -1;

	while('\0' != (c = *s++))
		if(c >= '0' && c <= '9')
			ret = ret * 10 + (c - '0');
		else
			return -1;

	return ret;
}

/* since we only care about values >= 0 or error, returns < 0 for any illegal string, else value */
static FLAC__int64 local__parse_int64_(const char *s)
{
	FLAC__int64 ret = 0;
	char c;

	if(*s == '\0')
		return -1;

	while('\0' != (c = *s++))
		if(c >= '0' && c <= '9')
			ret = ret * 10 + (c - '0');
		else
			return -1;

	return ret;
}

/* accept '[0-9]+:[0-9][0-9]?:[0-9][0-9]?', but max second of 59 and max frame of 74, e.g. 0:0:0, 123:45:67
 * return sample number or <0 for error
 */
static FLAC__int64 local__parse_msf_(const char *s)
{
	FLAC__int64 ret, field;
	char c;

	c = *s++;
	if(c >= '0' && c <= '9')
		field = (c - '0');
	else
		return -1;
	while(':' != (c = *s++)) {
		if(c >= '0' && c <= '9')
			field = field * 10 + (c - '0');
		else
			return -1;
	}

	ret = field * 60 * 44100;

	c = *s++;
	if(c >= '0' && c <= '9')
		field = (c - '0');
	else
		return -1;
	if(':' != (c = *s++)) {
		if(c >= '0' && c <= '9') {
			field = field * 10 + (c - '0');
			c = *s++;
			if(c != ':')
				return -1;
		}
		else
			return -1;
	}

	if(field >= 60)
		return -1;

	ret += field * 44100;

	c = *s++;
	if(c >= '0' && c <= '9')
		field = (c - '0');
	else
		return -1;
	if('\0' != (c = *s++)) {
		if(c >= '0' && c <= '9') {
			field = field * 10 + (c - '0');
			c = *s++;
		}
		else
			return -1;
	}

	if(c != '\0')
		return -1;

	if(field >= 75)
		return -1;

	ret += field * (44100 / 75);

	return ret;
}

static char *local__get_field_(char **s, FLAC__bool allow_quotes)
{
	FLAC__bool has_quote = false;
	char *p;

	FLAC__ASSERT(0 != s);

	if(0 == *s)
		return 0;

	/* skip leading whitespace */
	while(**s && 0 != strchr(" \t\r\n", **s))
		(*s)++;

	if(**s == 0) {
		*s = 0;
		return 0;
	}

	if(allow_quotes && (**s == '"')) {
		has_quote = true;
		(*s)++;
		if(**s == 0) {
			*s = 0;
			return 0;
		}
	}

	p = *s;

	if(has_quote) {
		*s = strchr(*s, '\"');
		/* if there is no matching end quote, it's an error */
		if(0 == *s)
			p = *s = 0;
		else {
			**s = '\0';
			(*s)++;
		}
	}
	else {
		while(**s && 0 == strchr(" \t\r\n", **s))
			(*s)++;
		if(**s) {
			**s = '\0';
			(*s)++;
		}
		else
			*s = 0;
	}

	return p;
}

static FLAC__bool local__cuesheet_parse_(FILE *file, const char **error_message, unsigned *last_line_read, FLAC__StreamMetadata *cuesheet, FLAC__bool is_cdda, FLAC__uint64 lead_out_offset)
{
#if defined _MSC_VER || defined __MINGW32__ || defined __EMX__
#define FLAC__STRCASECMP stricmp
#else
#define FLAC__STRCASECMP strcasecmp
#endif
	char buffer[4096], *line, *field;
	unsigned forced_leadout_track_num = 0;
	FLAC__uint64 forced_leadout_track_offset = 0;
	int in_track_num = -1, in_index_num = -1;
	FLAC__bool disc_has_catalog = false, track_has_flags = false, track_has_isrc = false, has_forced_leadout = false;
	FLAC__StreamMetadata_CueSheet *cs = &cuesheet->data.cue_sheet;

	cs->lead_in = is_cdda? 2 * 44100 /* The default lead-in size for CD-DA */ : 0;
	cs->is_cd = is_cdda;

	while(0 != fgets(buffer, sizeof(buffer), file)) {
		(*last_line_read)++;
		line = buffer;

		{
			size_t linelen = strlen(line);
			if((linelen == sizeof(buffer)-1) && line[linelen-1] != '\n') {
				*error_message = "line too long";
				return false;
			}
		}

		if(0 != (field = local__get_field_(&line, /*allow_quotes=*/false))) {
			if(0 == FLAC__STRCASECMP(field, "CATALOG")) {
				if(disc_has_catalog) {
					*error_message = "found multiple CATALOG commands";
					return false;
				}
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/true))) {
					*error_message = "CATALOG is missing catalog number";
					return false;
				}
				if(strlen(field) >= sizeof(cs->media_catalog_number)) {
					*error_message = "CATALOG number is too long";
					return false;
				}
				if(is_cdda && (strlen(field) != 13 || strspn(field, "0123456789") != 13)) {
					*error_message = "CD-DA CATALOG number must be 13 decimal digits";
					return false;
				}
				strcpy(cs->media_catalog_number, field);
				disc_has_catalog = true;
			}
			else if(0 == FLAC__STRCASECMP(field, "FLAGS")) {
				if(track_has_flags) {
					*error_message = "found multiple FLAGS commands";
					return false;
				}
				if(in_track_num < 0 || in_index_num >= 0) {
					*error_message = "FLAGS command must come after TRACK but before INDEX";
					return false;
				}
				while(0 != (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					if(0 == FLAC__STRCASECMP(field, "PRE"))
						cs->tracks[cs->num_tracks-1].pre_emphasis = 1;
				}
				track_has_flags = true;
			}
			else if(0 == FLAC__STRCASECMP(field, "INDEX")) {
				FLAC__int64 xx;
				FLAC__StreamMetadata_CueSheet_Track *track = &cs->tracks[cs->num_tracks-1];
				if(in_track_num < 0) {
					*error_message = "found INDEX before any TRACK";
					return false;
				}
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					*error_message = "INDEX is missing index number";
					return false;
				}
				in_index_num = local__parse_int_(field);
				if(in_index_num < 0) {
					*error_message = "INDEX has invalid index number";
					return false;
				}
				FLAC__ASSERT(cs->num_tracks > 0);
				if(track->num_indices == 0) {
					/* it's the first index point of the track */
					if(in_index_num > 1) {
						*error_message = "first INDEX number of a TRACK must be 0 or 1";
						return false;
					}
				}
				else {
					if(in_index_num != track->indices[track->num_indices-1].number + 1) {
						*error_message = "INDEX numbers must be sequential";
						return false;
					}
				}
				if(is_cdda && in_index_num > 99) {
					*error_message = "CD-DA INDEX number must be between 0 and 99, inclusive";
					return false;
				}
				/*@@@ search for duplicate track number? */
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					*error_message = "INDEX is missing an offset after the index number";
					return false;
				}
				xx = local__parse_msf_(field);
				if(xx < 0) {
					if(is_cdda) {
						*error_message = "illegal INDEX offset (not of the form MM:SS:FF)";
						return false;
					}
					xx = local__parse_int64_(field);
					if(xx < 0) {
						*error_message = "illegal INDEX offset";
						return false;
					}
				}
				if(is_cdda && cs->num_tracks == 1 && cs->tracks[0].num_indices == 0 && xx != 0) {
					*error_message = "first INDEX of first TRACK must have an offset of 00:00:00";
					return false;
				}
				if(is_cdda && track->num_indices > 0 && (FLAC__uint64)xx <= track->indices[track->num_indices-1].offset) {
					*error_message = "CD-DA INDEX offsets must increase in time";
					return false;
				}
				/* fill in track offset if it's the first index of the track */
				if(track->num_indices == 0)
					track->offset = (FLAC__uint64)xx;
				if(is_cdda && cs->num_tracks > 1) {
					const FLAC__StreamMetadata_CueSheet_Track *prev = &cs->tracks[cs->num_tracks-2];
					if((FLAC__uint64)xx <= prev->offset + prev->indices[prev->num_indices-1].offset) {
						*error_message = "CD-DA INDEX offsets must increase in time";
						return false;
					}
				}
				if(!FLAC__metadata_object_cuesheet_track_insert_blank_index(cuesheet, cs->num_tracks-1, track->num_indices)) {
					*error_message = "memory allocation error";
					return false;
				}
				track->indices[track->num_indices-1].offset = (FLAC__uint64)xx - track->offset;
				track->indices[track->num_indices-1].number = in_index_num;
			}
			else if(0 == FLAC__STRCASECMP(field, "ISRC")) {
				char *l, *r;
				if(track_has_isrc) {
					*error_message = "found multiple ISRC commands";
					return false;
				}
				if(in_track_num < 0 || in_index_num >= 0) {
					*error_message = "ISRC command must come after TRACK but before INDEX";
					return false;
				}
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					*error_message = "ISRC is missing ISRC number";
					return false;
				}
				/* strip out dashes */
				for(l = r = field; *r; r++) {
					if(*r != '-')
						*l++ = *r;
				}
				*l = '\0';
				if(strlen(field) != 12 || strspn(field, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") < 5 || strspn(field+5, "1234567890") != 7) {
					*error_message = "invalid ISRC number";
					return false;
				}
				strcpy(cs->tracks[cs->num_tracks-1].isrc, field);
				track_has_isrc = true;
			}
			else if(0 == FLAC__STRCASECMP(field, "TRACK")) {
				if(cs->num_tracks > 0) {
					const FLAC__StreamMetadata_CueSheet_Track *prev = &cs->tracks[cs->num_tracks-1];
					if(
						prev->num_indices == 0 ||
						(
						 	is_cdda &&
							(
								(prev->num_indices == 1 && prev->indices[0].number != 1) ||
								(prev->num_indices == 2 && prev->indices[0].number != 1 && prev->indices[1].number != 1)
							)
						)
					) {
						*error_message = is_cdda?
							"previous TRACK must specify at least one INDEX 01" :
							"previous TRACK must specify at least one INDEX";
						return false;
					}
				}
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					*error_message = "TRACK is missing track number";
					return false;
				}
				in_track_num = local__parse_int_(field);
				if(in_track_num < 0) {
					*error_message = "TRACK has invalid track number";
					return false;
				}
				if(in_track_num == 0) {
					*error_message = "TRACK number must be greater than 0";
					return false;
				}
				if(is_cdda) {
					if(in_track_num > 99) {
						*error_message = "CD-DA TRACK number must be between 1 and 99, inclusive";
						return false;
					}
				}
				else {
					if(in_track_num == 255) {
						*error_message = "TRACK number 255 is reserved for the lead-out";
						return false;
					}
					else if(in_track_num > 255) {
						*error_message = "TRACK number must be between 1 and 254, inclusive";
						return false;
					}
				}
				if(is_cdda && cs->num_tracks > 0 && in_track_num != cs->tracks[cs->num_tracks-1].number + 1) {
					*error_message = "CD-DA TRACK numbers must be sequential";
					return false;
				}
				/*@@@ search for duplicate track number? */
				if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					*error_message = "TRACK is missing a track type after the track number";
					return false;
				}
				if(!FLAC__metadata_object_cuesheet_insert_blank_track(cuesheet, cs->num_tracks)) {
					*error_message = "memory allocation error";
					return false;
				}
				cs->tracks[cs->num_tracks-1].number = in_track_num;
				cs->tracks[cs->num_tracks-1].type = (0 == FLAC__STRCASECMP(field, "AUDIO"))? 0 : 1; /*@@@ should we be more strict with the value here? */
				in_index_num = -1;
				track_has_flags = false;
				track_has_isrc = false;
			}
			else if(0 == FLAC__STRCASECMP(field, "REM")) {
				if(0 != (field = local__get_field_(&line, /*allow_quotes=*/false))) {
					if(0 == strcmp(field, "FLAC__lead-in")) {
						FLAC__int64 xx;
						if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
							*error_message = "FLAC__lead-in is missing offset";
							return false;
						}
						xx = local__parse_int64_(field);
						if(xx < 0) {
							*error_message = "illegal FLAC__lead-in offset";
							return false;
						}
						if(is_cdda && xx % 588 != 0) {
							*error_message = "illegal CD-DA FLAC__lead-in offset, must be even multiple of 588 samples";
							return false;
						}
						cs->lead_in = (FLAC__uint64)xx;
					}
					else if(0 == strcmp(field, "FLAC__lead-out")) {
						int track_num;
						FLAC__int64 offset;
						if(has_forced_leadout) {
							*error_message = "multiple FLAC__lead-out commands";
							return false;
						}
						if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
							*error_message = "FLAC__lead-out is missing track number";
							return false;
						}
						track_num = local__parse_int_(field);
						if(track_num < 0) {
							*error_message = "illegal FLAC__lead-out track number";
							return false;
						}
						forced_leadout_track_num = (unsigned)track_num;
						/*@@@ search for duplicate track number? */
						if(0 == (field = local__get_field_(&line, /*allow_quotes=*/false))) {
							*error_message = "FLAC__lead-out is missing offset";
							return false;
						}
						offset = local__parse_int64_(field);
						if(offset < 0) {
							*error_message = "illegal FLAC__lead-out offset";
							return false;
						}
						forced_leadout_track_offset = (FLAC__uint64)offset;
						if(forced_leadout_track_offset != lead_out_offset) {
							*error_message = "FLAC__lead-out offset does not match end-of-stream offset";
							return false;
						}
						has_forced_leadout = true;
					}
				}
			}
		}
	}

	if(cs->num_tracks == 0) {
		*error_message = "there must be at least one TRACK command";
		return false;
	}
	else {
		const FLAC__StreamMetadata_CueSheet_Track *prev = &cs->tracks[cs->num_tracks-1];
		if(
			prev->num_indices == 0 ||
			(
				is_cdda &&
				(
					(prev->num_indices == 1 && prev->indices[0].number != 1) ||
					(prev->num_indices == 2 && prev->indices[0].number != 1 && prev->indices[1].number != 1)
				)
			)
		) {
			*error_message = is_cdda?
				"previous TRACK must specify at least one INDEX 01" :
				"previous TRACK must specify at least one INDEX";
			return false;
		}
	}

	if(!has_forced_leadout) {
		forced_leadout_track_num = is_cdda? 170 : 255;
		forced_leadout_track_offset = lead_out_offset;
	}
	if(!FLAC__metadata_object_cuesheet_insert_blank_track(cuesheet, cs->num_tracks)) {
		*error_message = "memory allocation error";
		return false;
	}
	cs->tracks[cs->num_tracks-1].number = forced_leadout_track_num;
	cs->tracks[cs->num_tracks-1].offset = forced_leadout_track_offset;

	if(!feof(file)) {
		*error_message = "read error";
		return false;
	}
	return true;
#undef FLAC__STRCASECMP
}

FLAC__StreamMetadata *grabbag__cuesheet_parse(FILE *file, const char **error_message, unsigned *last_line_read, FLAC__bool is_cdda, FLAC__uint64 lead_out_offset)
{
	FLAC__StreamMetadata *cuesheet;

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != error_message);
	FLAC__ASSERT(0 != last_line_read);

	*last_line_read = 0;
	cuesheet = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET);

	if(0 == cuesheet) {
		*error_message = "memory allocation error";
		return 0;
	}

	if(!local__cuesheet_parse_(file, error_message, last_line_read, cuesheet, is_cdda, lead_out_offset)) {
		FLAC__metadata_object_delete(cuesheet);
		return 0;
	}

	return cuesheet;
}

void grabbag__cuesheet_emit(FILE *file, const FLAC__StreamMetadata *cuesheet, const char *file_reference)
{
	const FLAC__StreamMetadata_CueSheet *cs;
	unsigned track_num, index_num;

	FLAC__ASSERT(0 != file);
	FLAC__ASSERT(0 != cuesheet);
	FLAC__ASSERT(cuesheet->type == FLAC__METADATA_TYPE_CUESHEET);

	cs = &cuesheet->data.cue_sheet;

	if(*(cs->media_catalog_number))
		fprintf(file, "CATALOG %s\n", cs->media_catalog_number);
	fprintf(file, "FILE %s\n", file_reference);

	for(track_num = 0; track_num < cs->num_tracks-1; track_num++) {
		const FLAC__StreamMetadata_CueSheet_Track *track = cs->tracks + track_num;

		fprintf(file, "  TRACK %02u %s\n", (unsigned)track->number, track->type == 0? "AUDIO" : "DATA");

		if(track->pre_emphasis)
			fprintf(file, "    FLAGS PRE\n");
		if(*(track->isrc))
			fprintf(file, "    ISRC %s\n", track->isrc);

		for(index_num = 0; index_num < track->num_indices; index_num++) {
			const FLAC__StreamMetadata_CueSheet_Index *index = track->indices + index_num;

			fprintf(file, "    INDEX %02u ", (unsigned)index->number);
			if(cs->is_cd) {
				const unsigned logical_frame = (unsigned)((track->offset + index->offset) / (44100 / 75));
				unsigned m, s, f;
				grabbag__cuesheet_frame_to_msf(logical_frame, &m, &s, &f);
				fprintf(file, "%02u:%02u:%02u\n", m, s, f);
			}
			else
#ifdef _MSC_VER
				fprintf(file, "%I64u\n", track->offset + index->offset);
#else
				fprintf(file, "%llu\n", (unsigned long long)(track->offset + index->offset));
#endif
		}
	}

#ifdef _MSC_VER
	fprintf(file, "REM FLAC__lead-in %I64u\n", cs->lead_in);
	fprintf(file, "REM FLAC__lead-out %u %I64u\n", (unsigned)cs->tracks[track_num].number, cs->tracks[track_num].offset);
#else
	fprintf(file, "REM FLAC__lead-in %llu\n", (unsigned long long)cs->lead_in);
	fprintf(file, "REM FLAC__lead-out %u %llu\n", (unsigned)cs->tracks[track_num].number, (unsigned long long)cs->tracks[track_num].offset);
#endif
}
