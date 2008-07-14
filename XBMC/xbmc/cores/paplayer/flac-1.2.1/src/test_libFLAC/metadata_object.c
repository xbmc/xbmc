/* test_libFLAC - Unit tester for libFLAC
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

#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "test_libs_common/metadata_utils.h"
#include "metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static FLAC__byte *make_dummydata_(FLAC__byte *dummydata, unsigned len)
{
	FLAC__byte *ret;

	if(0 == (ret = (FLAC__byte*)malloc(len))) {
		printf("FAILED, malloc error\n");
		exit(1);
	}
	else
		memcpy(ret, dummydata, len);

	return ret;
}

static FLAC__bool compare_track_(const FLAC__StreamMetadata_CueSheet_Track *from, const FLAC__StreamMetadata_CueSheet_Track *to)
{
	unsigned i;

	if(from->offset != to->offset) {
#ifdef _MSC_VER
		printf("FAILED, track offset mismatch, expected %I64u, got %I64u\n", to->offset, from->offset);
#else
		printf("FAILED, track offset mismatch, expected %llu, got %llu\n", (unsigned long long)to->offset, (unsigned long long)from->offset);
#endif
		return false;
	}
	if(from->number != to->number) {
		printf("FAILED, track number mismatch, expected %u, got %u\n", (unsigned)to->number, (unsigned)from->number);
		return false;
	}
	if(0 != strcmp(from->isrc, to->isrc)) {
		printf("FAILED, track number mismatch, expected %s, got %s\n", to->isrc, from->isrc);
		return false;
	}
	if(from->type != to->type) {
		printf("FAILED, track type mismatch, expected %u, got %u\n", (unsigned)to->type, (unsigned)from->type);
		return false;
	}
	if(from->pre_emphasis != to->pre_emphasis) {
		printf("FAILED, track pre_emphasis mismatch, expected %u, got %u\n", (unsigned)to->pre_emphasis, (unsigned)from->pre_emphasis);
		return false;
	}
	if(from->num_indices != to->num_indices) {
		printf("FAILED, track num_indices mismatch, expected %u, got %u\n", (unsigned)to->num_indices, (unsigned)from->num_indices);
		return false;
	}
	if(0 == to->indices || 0 == from->indices) {
		if(to->indices != from->indices) {
			printf("FAILED, track indices mismatch\n");
			return false;
		}
	}
	else {
		for(i = 0; i < to->num_indices; i++) {
			if(from->indices[i].offset != to->indices[i].offset) {
#ifdef _MSC_VER
				printf("FAILED, track indices[%u].offset mismatch, expected %I64u, got %I64u\n", i, to->indices[i].offset, from->indices[i].offset);
#else
				printf("FAILED, track indices[%u].offset mismatch, expected %llu, got %llu\n", i, (unsigned long long)to->indices[i].offset, (unsigned long long)from->indices[i].offset);
#endif
				return false;
			}
			if(from->indices[i].number != to->indices[i].number) {
				printf("FAILED, track indices[%u].number mismatch, expected %u, got %u\n", i, (unsigned)to->indices[i].number, (unsigned)from->indices[i].number);
				return false;
			}
		}
	}

	return true;
}

static FLAC__bool compare_seekpoint_array_(const FLAC__StreamMetadata_SeekPoint *from, const FLAC__StreamMetadata_SeekPoint *to, unsigned n)
{
	unsigned i;

	FLAC__ASSERT(0 != from);
	FLAC__ASSERT(0 != to);

	for(i = 0; i < n; i++) {
		if(from[i].sample_number != to[i].sample_number) {
#ifdef _MSC_VER
			printf("FAILED, point[%u].sample_number mismatch, expected %I64u, got %I64u\n", i, to[i].sample_number, from[i].sample_number);
#else
			printf("FAILED, point[%u].sample_number mismatch, expected %llu, got %llu\n", i, (unsigned long long)to[i].sample_number, (unsigned long long)from[i].sample_number);
#endif
			return false;
		}
		if(from[i].stream_offset != to[i].stream_offset) {
#ifdef _MSC_VER
			printf("FAILED, point[%u].stream_offset mismatch, expected %I64u, got %I64u\n", i, to[i].stream_offset, from[i].stream_offset);
#else
			printf("FAILED, point[%u].stream_offset mismatch, expected %llu, got %llu\n", i, (unsigned long long)to[i].stream_offset, (unsigned long long)from[i].stream_offset);
#endif
			return false;
		}
		if(from[i].frame_samples != to[i].frame_samples) {
			printf("FAILED, point[%u].frame_samples mismatch, expected %u, got %u\n", i, to[i].frame_samples, from[i].frame_samples);
			return false;
		}
	}

	return true;
}

static FLAC__bool check_seektable_(const FLAC__StreamMetadata *block, unsigned num_points, const FLAC__StreamMetadata_SeekPoint *array)
{
	const unsigned expected_length = num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;

	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	if(block->data.seek_table.num_points != num_points) {
		printf("FAILED, expected %u point, got %u\n", num_points, block->data.seek_table.num_points);
		return false;
	}
	if(0 == array) {
		if(0 != block->data.seek_table.points) {
			printf("FAILED, 'points' pointer is not null\n");
			return false;
		}
	}
	else {
		if(!compare_seekpoint_array_(block->data.seek_table.points, array, num_points))
			return false;
	}
	printf("OK\n");

	return true;
}

static void entry_new_(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field)
{
	entry->length = strlen(field);
	entry->entry = (FLAC__byte*)malloc(entry->length+1);
	FLAC__ASSERT(0 != entry->entry);
	memcpy(entry->entry, field, entry->length);
	entry->entry[entry->length] = '\0';
}

static void entry_clone_(FLAC__StreamMetadata_VorbisComment_Entry *entry)
{
	FLAC__byte *x = (FLAC__byte*)malloc(entry->length+1);
	FLAC__ASSERT(0 != x);
	memcpy(x, entry->entry, entry->length);
	x[entry->length] = '\0';
	entry->entry = x;
}

static void vc_calc_len_(FLAC__StreamMetadata *block)
{
	const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
	unsigned i;

	block->length = FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
	block->length += vc->vendor_string.length;
	block->length += FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN / 8;
	for(i = 0; i < vc->num_comments; i++) {
		block->length += FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN / 8;
		block->length += vc->comments[i].length;
	}
}

static void vc_resize_(FLAC__StreamMetadata *block, unsigned num)
{
	FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;

	if(vc->num_comments != 0) {
		FLAC__ASSERT(0 != vc->comments);
		if(num < vc->num_comments) {
			unsigned i;
			for(i = num; i < vc->num_comments; i++) {
				if(0 != vc->comments[i].entry)
					free(vc->comments[i].entry);
			}
		}
	}
	if(num == 0) {
		if(0 != vc->comments) {
			free(vc->comments);
			vc->comments = 0;
		}
	}
	else {
		vc->comments = (FLAC__StreamMetadata_VorbisComment_Entry*)realloc(vc->comments, sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*num);
		FLAC__ASSERT(0 != vc->comments);
		if(num > vc->num_comments)
			memset(vc->comments+vc->num_comments, 0, sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*(num-vc->num_comments));
	}

	vc->num_comments = num;
	vc_calc_len_(block);
}

static int vc_find_from_(FLAC__StreamMetadata *block, const char *name, unsigned start)
{
	const unsigned n = strlen(name);
	unsigned i;
	for(i = start; i < block->data.vorbis_comment.num_comments; i++) {
		const FLAC__StreamMetadata_VorbisComment_Entry *entry = &block->data.vorbis_comment.comments[i];
		if(entry->length > n && 0 == strncmp((const char *)entry->entry, name, n) && entry->entry[n] == '=')
			return (int)i;
	}
	return -1;
}

static void vc_set_vs_new_(FLAC__StreamMetadata_VorbisComment_Entry *entry, FLAC__StreamMetadata *block, const char *field)
{
	if(0 != block->data.vorbis_comment.vendor_string.entry)
		free(block->data.vorbis_comment.vendor_string.entry);
	entry_new_(entry, field);
	block->data.vorbis_comment.vendor_string = *entry;
	vc_calc_len_(block);
}

static void vc_set_new_(FLAC__StreamMetadata_VorbisComment_Entry *entry, FLAC__StreamMetadata *block, unsigned pos, const char *field)
{
	if(0 != block->data.vorbis_comment.comments[pos].entry)
		free(block->data.vorbis_comment.comments[pos].entry);
	entry_new_(entry, field);
	block->data.vorbis_comment.comments[pos] = *entry;
	vc_calc_len_(block);
}

static void vc_insert_new_(FLAC__StreamMetadata_VorbisComment_Entry *entry, FLAC__StreamMetadata *block, unsigned pos, const char *field)
{
	vc_resize_(block, block->data.vorbis_comment.num_comments+1);
	memmove(&block->data.vorbis_comment.comments[pos+1], &block->data.vorbis_comment.comments[pos], sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*(block->data.vorbis_comment.num_comments-1-pos));
	memset(&block->data.vorbis_comment.comments[pos], 0, sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
	vc_set_new_(entry, block, pos, field);
	vc_calc_len_(block);
}

static void vc_delete_(FLAC__StreamMetadata *block, unsigned pos)
{
	if(0 != block->data.vorbis_comment.comments[pos].entry)
		free(block->data.vorbis_comment.comments[pos].entry);
	memmove(&block->data.vorbis_comment.comments[pos], &block->data.vorbis_comment.comments[pos+1], sizeof(FLAC__StreamMetadata_VorbisComment_Entry)*(block->data.vorbis_comment.num_comments-pos-1));
	block->data.vorbis_comment.comments[block->data.vorbis_comment.num_comments-1].entry = 0;
	block->data.vorbis_comment.comments[block->data.vorbis_comment.num_comments-1].length = 0;
	vc_resize_(block, block->data.vorbis_comment.num_comments-1);
	vc_calc_len_(block);
}

static void vc_replace_new_(FLAC__StreamMetadata_VorbisComment_Entry *entry, FLAC__StreamMetadata *block, const char *field, FLAC__bool all)
{
	int index;
	char field_name[256];
	const char *eq = strchr(field, '=');
	FLAC__ASSERT(eq>field && (unsigned)(eq-field) < sizeof(field_name));
	memcpy(field_name, field, eq-field);
	field_name[eq-field]='\0';

	index = vc_find_from_(block, field_name, 0);
	if(index < 0)
		vc_insert_new_(entry, block, block->data.vorbis_comment.num_comments, field);
	else {
		vc_set_new_(entry, block, (unsigned)index, field);
		if(all) {
			for(index = index+1; index >= 0 && (unsigned)index < block->data.vorbis_comment.num_comments; )
				if((index = vc_find_from_(block, field_name, (unsigned)index)) >= 0)
					vc_delete_(block, (unsigned)index);
		}
	}

	vc_calc_len_(block);
}

static void track_new_(FLAC__StreamMetadata_CueSheet_Track *track, FLAC__uint64 offset, FLAC__byte number, const char *isrc, FLAC__bool data, FLAC__bool pre_em)
{
	track->offset = offset;
	track->number = number;
	memcpy(track->isrc, isrc, sizeof(track->isrc));
	track->type = data;
	track->pre_emphasis = pre_em;
	track->num_indices = 0;
	track->indices = 0;
}

static void track_clone_(FLAC__StreamMetadata_CueSheet_Track *track)
{
	if(track->num_indices > 0) {
		size_t bytes = sizeof(FLAC__StreamMetadata_CueSheet_Index) * track->num_indices;
		FLAC__StreamMetadata_CueSheet_Index *x = (FLAC__StreamMetadata_CueSheet_Index*)malloc(bytes);
		FLAC__ASSERT(0 != x);
		memcpy(x, track->indices, bytes);
		track->indices = x;
	}
}

static void cs_calc_len_(FLAC__StreamMetadata *block)
{
	const FLAC__StreamMetadata_CueSheet *cs = &block->data.cue_sheet;
	unsigned i;

	block->length = (
		FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
		FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
		FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
	) / 8;
	block->length += cs->num_tracks * (
		FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
	) / 8;
	for(i = 0; i < cs->num_tracks; i++) {
		block->length += cs->tracks[i].num_indices * (
			FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
		) / 8;
	}
}

static void tr_resize_(FLAC__StreamMetadata *block, unsigned track_num, unsigned num)
{
	FLAC__StreamMetadata_CueSheet_Track *tr;

	FLAC__ASSERT(track_num < block->data.cue_sheet.num_tracks);

	tr = &block->data.cue_sheet.tracks[track_num];

	if(tr->num_indices != 0) {
		FLAC__ASSERT(0 != tr->indices);
	}
	if(num == 0) {
		if(0 != tr->indices) {
			free(tr->indices);
			tr->indices = 0;
		}
	}
	else {
		tr->indices = (FLAC__StreamMetadata_CueSheet_Index*)realloc(tr->indices, sizeof(FLAC__StreamMetadata_CueSheet_Index)*num);
		FLAC__ASSERT(0 != tr->indices);
		if(num > tr->num_indices)
			memset(tr->indices+tr->num_indices, 0, sizeof(FLAC__StreamMetadata_CueSheet_Index)*(num-tr->num_indices));
	}

	tr->num_indices = num;
	cs_calc_len_(block);
}

static void tr_set_new_(FLAC__StreamMetadata *block, unsigned track_num, unsigned pos, FLAC__StreamMetadata_CueSheet_Index index)
{
	FLAC__StreamMetadata_CueSheet_Track *tr;

	FLAC__ASSERT(track_num < block->data.cue_sheet.num_tracks);

	tr = &block->data.cue_sheet.tracks[track_num];

	FLAC__ASSERT(pos < tr->num_indices);

	tr->indices[pos] = index;

	cs_calc_len_(block);
}

static void tr_insert_new_(FLAC__StreamMetadata *block, unsigned track_num, unsigned pos, FLAC__StreamMetadata_CueSheet_Index index)
{
	FLAC__StreamMetadata_CueSheet_Track *tr;

	FLAC__ASSERT(track_num < block->data.cue_sheet.num_tracks);

	tr = &block->data.cue_sheet.tracks[track_num];

	FLAC__ASSERT(pos <= tr->num_indices);

	tr_resize_(block, track_num, tr->num_indices+1);
	memmove(&tr->indices[pos+1], &tr->indices[pos], sizeof(FLAC__StreamMetadata_CueSheet_Index)*(tr->num_indices-1-pos));
	tr_set_new_(block, track_num, pos, index);
	cs_calc_len_(block);
}

static void tr_delete_(FLAC__StreamMetadata *block, unsigned track_num, unsigned pos)
{
	FLAC__StreamMetadata_CueSheet_Track *tr;

	FLAC__ASSERT(track_num < block->data.cue_sheet.num_tracks);

	tr = &block->data.cue_sheet.tracks[track_num];

	FLAC__ASSERT(pos <= tr->num_indices);

	memmove(&tr->indices[pos], &tr->indices[pos+1], sizeof(FLAC__StreamMetadata_CueSheet_Index)*(tr->num_indices-pos-1));
	tr_resize_(block, track_num, tr->num_indices-1);
	cs_calc_len_(block);
}

static void cs_resize_(FLAC__StreamMetadata *block, unsigned num)
{
	FLAC__StreamMetadata_CueSheet *cs = &block->data.cue_sheet;

	if(cs->num_tracks != 0) {
		FLAC__ASSERT(0 != cs->tracks);
		if(num < cs->num_tracks) {
			unsigned i;
			for(i = num; i < cs->num_tracks; i++) {
				if(0 != cs->tracks[i].indices)
					free(cs->tracks[i].indices);
			}
		}
	}
	if(num == 0) {
		if(0 != cs->tracks) {
			free(cs->tracks);
			cs->tracks = 0;
		}
	}
	else {
		cs->tracks = (FLAC__StreamMetadata_CueSheet_Track*)realloc(cs->tracks, sizeof(FLAC__StreamMetadata_CueSheet_Track)*num);
		FLAC__ASSERT(0 != cs->tracks);
		if(num > cs->num_tracks)
			memset(cs->tracks+cs->num_tracks, 0, sizeof(FLAC__StreamMetadata_CueSheet_Track)*(num-cs->num_tracks));
	}

	cs->num_tracks = num;
	cs_calc_len_(block);
}

static void cs_set_new_(FLAC__StreamMetadata_CueSheet_Track *track, FLAC__StreamMetadata *block, unsigned pos, FLAC__uint64 offset, FLAC__byte number, const char *isrc, FLAC__bool data, FLAC__bool pre_em)
{
	track_new_(track, offset, number, isrc, data, pre_em);
	block->data.cue_sheet.tracks[pos] = *track;
	cs_calc_len_(block);
}

static void cs_insert_new_(FLAC__StreamMetadata_CueSheet_Track *track, FLAC__StreamMetadata *block, unsigned pos, FLAC__uint64 offset, FLAC__byte number, const char *isrc, FLAC__bool data, FLAC__bool pre_em)
{
	cs_resize_(block, block->data.cue_sheet.num_tracks+1);
	memmove(&block->data.cue_sheet.tracks[pos+1], &block->data.cue_sheet.tracks[pos], sizeof(FLAC__StreamMetadata_CueSheet_Track)*(block->data.cue_sheet.num_tracks-1-pos));
	cs_set_new_(track, block, pos, offset, number, isrc, data, pre_em);
	cs_calc_len_(block);
}

static void cs_delete_(FLAC__StreamMetadata *block, unsigned pos)
{
	if(0 != block->data.cue_sheet.tracks[pos].indices)
		free(block->data.cue_sheet.tracks[pos].indices);
	memmove(&block->data.cue_sheet.tracks[pos], &block->data.cue_sheet.tracks[pos+1], sizeof(FLAC__StreamMetadata_CueSheet_Track)*(block->data.cue_sheet.num_tracks-pos-1));
	block->data.cue_sheet.tracks[block->data.cue_sheet.num_tracks-1].indices = 0;
	block->data.cue_sheet.tracks[block->data.cue_sheet.num_tracks-1].num_indices = 0;
	cs_resize_(block, block->data.cue_sheet.num_tracks-1);
	cs_calc_len_(block);
}

static void pi_set_mime_type(FLAC__StreamMetadata *block, const char *s)
{
	if(block->data.picture.mime_type) {
		block->length -= strlen(block->data.picture.mime_type);
		free(block->data.picture.mime_type);
	}
	block->data.picture.mime_type = strdup(s);
	FLAC__ASSERT(block->data.picture.mime_type);
	block->length += strlen(block->data.picture.mime_type);
}

static void pi_set_description(FLAC__StreamMetadata *block, const FLAC__byte *s)
{
	if(block->data.picture.description) {
		block->length -= strlen((const char *)block->data.picture.description);
		free(block->data.picture.description);
	}
	block->data.picture.description = (FLAC__byte*)strdup((const char *)s);
	FLAC__ASSERT(block->data.picture.description);
	block->length += strlen((const char *)block->data.picture.description);
}

static void pi_set_data(FLAC__StreamMetadata *block, const FLAC__byte *data, FLAC__uint32 len)
{
	if(block->data.picture.data) {
		block->length -= block->data.picture.data_length;
		free(block->data.picture.data);
	}
	block->data.picture.data = (FLAC__byte*)strdup((const char *)data);
	FLAC__ASSERT(block->data.picture.data);
	block->data.picture.data_length = len;
	block->length += len;
}

FLAC__bool test_metadata_object(void)
{
	FLAC__StreamMetadata *block, *blockcopy, *vorbiscomment, *cuesheet, *picture;
	FLAC__StreamMetadata_SeekPoint seekpoint_array[14];
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	FLAC__StreamMetadata_CueSheet_Index index;
	FLAC__StreamMetadata_CueSheet_Track track;
	unsigned i, expected_length, seekpoints;
	int j;
	static FLAC__byte dummydata[4] = { 'a', 'b', 'c', 'd' };

	printf("\n+++ libFLAC unit test: metadata objects\n\n");


	printf("testing STREAMINFO\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing PADDING\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = 0;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing APPLICATION\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(copy)... ");
	if(!FLAC__metadata_object_application_set_data(block, dummydata, sizeof(dummydata), true/*copy*/)) {
		printf("FAILED, returned false\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_application_set_data(own)... ");
	if(!FLAC__metadata_object_application_set_data(block, make_dummydata_(dummydata, sizeof(dummydata)), sizeof(dummydata), false/*own*/)) {
		printf("FAILED, returned false\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8) + sizeof(dummydata);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	if(0 != memcmp(block->data.application.data, dummydata, sizeof(dummydata))) {
		printf("FAILED, data mismatch\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing SEEKTABLE\n");

	for(i = 0; i < sizeof(seekpoint_array) / sizeof(FLAC__StreamMetadata_SeekPoint); i++) {
		seekpoint_array[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		seekpoint_array[i].stream_offset = 0;
		seekpoint_array[i].frame_samples = 0;
	}

	seekpoints = 0;
	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, 0))
		return false;

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	seekpoints = 2;
	printf("testing FLAC__metadata_object_seektable_resize_points(grow to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints = 1;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_seektable_is_legal()...");
	if(!FLAC__metadata_object_seektable_is_legal(block)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	seekpoints = 0;
	printf("testing FLAC__metadata_object_seektable_resize_points(shrink to %u)...", seekpoints);
	if(!FLAC__metadata_object_seektable_resize_points(block, seekpoints)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, 0))
		return false;

	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 0, seekpoint_array[0])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[0].sample_number = 1;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on beginning of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 0, seekpoint_array[0])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[1].sample_number = 2;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on middle of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 1, seekpoint_array[1])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[3].sample_number = 3;
	seekpoints++;
	printf("testing FLAC__metadata_object_seektable_insert_point() on end of non-empty array...");
	if(!FLAC__metadata_object_seektable_insert_point(block, 3, seekpoint_array[3])) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	seekpoint_array[2].sample_number = seekpoint_array[3].sample_number;
	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on middle of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on end of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints--;
	printf("testing FLAC__metadata_object_seektable_delete_point() on beginning of array...");
	if(!FLAC__metadata_object_seektable_delete_point(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array+1))
		return false;

	printf("testing FLAC__metadata_object_seektable_set_point()...");
	FLAC__metadata_object_seektable_set_point(block, 0, seekpoint_array[0]);
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(block);
	printf("OK\n");

	/* seektable template functions */

	for(i = 0; i < sizeof(seekpoint_array) / sizeof(FLAC__StreamMetadata_SeekPoint); i++) {
		seekpoint_array[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		seekpoint_array[i].stream_offset = 0;
		seekpoint_array[i].frame_samples = 0;
	}

	seekpoints = 0;
	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, 0))
		return false;

	seekpoints += 2;
	printf("testing FLAC__metadata_object_seekpoint_template_append_placeholders()... ");
	if(!FLAC__metadata_object_seektable_template_append_placeholders(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[seekpoints++].sample_number = 7;
	printf("testing FLAC__metadata_object_seekpoint_template_append_point()... ");
	if(!FLAC__metadata_object_seektable_template_append_point(block, 7)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	{
		FLAC__uint64 nums[2] = { 3, 7 };
		seekpoint_array[seekpoints++].sample_number = nums[0];
		seekpoint_array[seekpoints++].sample_number = nums[1];
		printf("testing FLAC__metadata_object_seekpoint_template_append_points()... ");
		if(!FLAC__metadata_object_seektable_template_append_points(block, nums, sizeof(nums)/sizeof(FLAC__uint64))) {
			printf("FAILED, returned false\n");
			return false;
		}
		if(!check_seektable_(block, seekpoints, seekpoint_array))
			return false;
	}

	seekpoint_array[seekpoints++].sample_number = 0;
	seekpoint_array[seekpoints++].sample_number = 10;
	seekpoint_array[seekpoints++].sample_number = 20;
	printf("testing FLAC__metadata_object_seekpoint_template_append_spaced_points()... ");
	if(!FLAC__metadata_object_seektable_template_append_spaced_points(block, 3, 30)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoints--;
	seekpoint_array[0].sample_number = 0;
	seekpoint_array[1].sample_number = 3;
	seekpoint_array[2].sample_number = 7;
	seekpoint_array[3].sample_number = 10;
	seekpoint_array[4].sample_number = 20;
	seekpoint_array[5].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seekpoint_array[6].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	printf("testing FLAC__metadata_object_seekpoint_template_sort(compact=true)... ");
	if(!FLAC__metadata_object_seektable_template_sort(block, /*compact=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!FLAC__metadata_object_seektable_is_legal(block)) {
		printf("FAILED, seek table is illegal\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_seekpoint_template_sort(compact=false)... ");
	if(!FLAC__metadata_object_seektable_template_sort(block, /*compact=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!FLAC__metadata_object_seektable_is_legal(block)) {
		printf("FAILED, seek table is illegal\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[seekpoints++].sample_number = 0;
	seekpoint_array[seekpoints++].sample_number = 10;
	seekpoint_array[seekpoints++].sample_number = 20;
	printf("testing FLAC__metadata_object_seekpoint_template_append_spaced_points_by_samples()... ");
	if(!FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(block, 10, 30)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	seekpoint_array[seekpoints++].sample_number = 0;
	seekpoint_array[seekpoints++].sample_number = 11;
	seekpoint_array[seekpoints++].sample_number = 22;
	printf("testing FLAC__metadata_object_seekpoint_template_append_spaced_points_by_samples()... ");
	if(!FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(block, 11, 30)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!check_seektable_(block, seekpoints, seekpoint_array))
		return false;

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing VORBIS_COMMENT\n");

	{
		FLAC__StreamMetadata_VorbisComment_Entry entry_;
		char *field_name, *field_value;

		printf("testing FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair()... ");
		if(!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry_, "name", "value")) {
			printf("FAILED, returned false\n");
			return false;
		}
		if(strcmp((const char *)entry_.entry, "name=value")) {
			printf("FAILED, field mismatch\n");
			return false;
		}
		printf("OK\n");

		printf("testing FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair()... ");
		if(!FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(entry_, &field_name, &field_value)) {
			printf("FAILED, returned false\n");
			return false;
		}
		if(strcmp(field_name, "name")) {
			printf("FAILED, field name mismatch\n");
			return false;
		}
		if(strcmp(field_value, "value")) {
			printf("FAILED, field value mismatch\n");
			return false;
		}
		printf("OK\n");

		printf("testing FLAC__metadata_object_vorbiscomment_entry_matches()... ");
		if(!FLAC__metadata_object_vorbiscomment_entry_matches(entry_, field_name, strlen(field_name))) {
			printf("FAILED, expected true, returned false\n");
			return false;
		}
		printf("OK\n");

		printf("testing FLAC__metadata_object_vorbiscomment_entry_matches()... ");
		if(FLAC__metadata_object_vorbiscomment_entry_matches(entry_, "blah", strlen("blah"))) {
			printf("FAILED, expected false, returned true\n");
			return false;
		}
		printf("OK\n");

		free(entry_.entry);
		free(field_name);
		free(field_value);
	}

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + strlen(FLAC__VENDOR_STRING) + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN/8);
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	vorbiscomment = FLAC__metadata_object_clone(block);
	if(0 == vorbiscomment) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 2);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(grow to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 1);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(shrink to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 0);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(shrink to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(copy) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(copy) on non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name2=field2");
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	vc_resize_(vorbiscomment, 0);
	printf("testing FLAC__metadata_object_vorbiscomment_resize_comments(shrink to %u)...", vorbiscomment->data.vorbis_comment.num_comments);
	if(!FLAC__metadata_object_vorbiscomment_resize_comments(block, vorbiscomment->data.vorbis_comment.num_comments)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on beginning of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name2=field2");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on middle of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name3=field3");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 1, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 3, "name4=field4");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 3, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 4, "name3=field3dup1");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 4, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(copy) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 5, "name3=field3dup1");
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 5, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, "name3")) != 1) {
		printf("FAILED, expected 1, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, j+1, "name3")) != 4) {
		printf("FAILED, expected 4, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, j+1, "name3")) != 5) {
		printf("FAILED, expected 5, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, "name2")) != 0) {
		printf("FAILED, expected 0, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, j+1, "name2")) != -1) {
		printf("FAILED, expected -1, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_find_entry_from()...");
	if((j = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, "blah")) != -1) {
		printf("FAILED, expected -1, got %d\n", j);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_replace_comment(first, copy)...");
	vc_replace_new_(&entry, vorbiscomment, "name3=field3new1", /*all=*/false);
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(block, entry, /*all=*/false, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	if(block->data.vorbis_comment.num_comments != 6) {
		printf("FAILED, expected 6 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_replace_comment(all, copy)...");
	vc_replace_new_(&entry, vorbiscomment, "name3=field3new2", /*all=*/true);
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(block, entry, /*all=*/true, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	if(block->data.vorbis_comment.num_comments != 4) {
		printf("FAILED, expected 4 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on middle of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on end of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on beginning of array...");
	vc_delete_(vorbiscomment, 0);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(copy) on non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "rem0=val0");
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(copy) on non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 2, "rem0=val1");
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(copy) on non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 3, "rem0=val2");
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_remove_entry_matching(\"blah\")...");
	if((j = FLAC__metadata_object_vorbiscomment_remove_entry_matching(block, "blah")) != 0) {
		printf("FAILED, expected 0, got %d\n", j);
		return false;
	}
	if(block->data.vorbis_comment.num_comments != 4) {
		printf("FAILED, expected 4 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_remove_entry_matching(\"rem0\")...");
	vc_delete_(vorbiscomment, 1);
	if((j = FLAC__metadata_object_vorbiscomment_remove_entry_matching(block, "rem0")) != 1) {
		printf("FAILED, expected 1, got %d\n", j);
		return false;
	}
	if(block->data.vorbis_comment.num_comments != 3) {
		printf("FAILED, expected 3 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_remove_entries_matching(\"blah\")...");
	if((j = FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, "blah")) != 0) {
		printf("FAILED, expected 0, got %d\n", j);
		return false;
	}
	if(block->data.vorbis_comment.num_comments != 3) {
		printf("FAILED, expected 3 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_remove_entries_matching(\"rem0\")...");
	vc_delete_(vorbiscomment, 1);
	vc_delete_(vorbiscomment, 1);
	if((j = FLAC__metadata_object_vorbiscomment_remove_entries_matching(block, "rem0")) != 2) {
		printf("FAILED, expected 2, got %d\n", j);
		return false;
	}
	if(block->data.vorbis_comment.num_comments != 1) {
		printf("FAILED, expected 1 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comment(copy)...");
	vc_set_new_(&entry, vorbiscomment, 0, "name5=field5");
	FLAC__metadata_object_vorbiscomment_set_comment(block, 0, entry, /*copy=*/true);
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(copy)...");
	vc_set_vs_new_(&entry, vorbiscomment, "name6=field6");
	FLAC__metadata_object_vorbiscomment_set_vendor_string(block, entry, /*copy=*/true);
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(vorbiscomment);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	vorbiscomment = FLAC__metadata_object_clone(block);
	if(0 == vorbiscomment) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(own) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_append_comment(own) on non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name2=field2");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(vorbiscomment);
	FLAC__metadata_object_delete(block);
	printf("OK\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	vorbiscomment = FLAC__metadata_object_clone(block);
	if(0 == vorbiscomment) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name1=field1");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on beginning of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 0, "name2=field2");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 0, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on middle of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 1, "name3=field3");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 1, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 3, "name4=field4");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 3, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 4, "name3=field3dup1");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 4, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_insert_comment(own) on end of non-empty array...");
	vc_insert_new_(&entry, vorbiscomment, 5, "name3=field3dup1");
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_insert_comment(block, 5, entry, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_replace_comment(first, own)...");
	vc_replace_new_(&entry, vorbiscomment, "name3=field3new1", /*all=*/false);
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(block, entry, /*all=*/false, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	if(block->data.vorbis_comment.num_comments != 6) {
		printf("FAILED, expected 6 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_replace_comment(all, own)...");
	vc_replace_new_(&entry, vorbiscomment, "name3=field3new2", /*all=*/true);
	entry_clone_(&entry);
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(block, entry, /*all=*/true, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	if(block->data.vorbis_comment.num_comments != 4) {
		printf("FAILED, expected 4 comments, got %u\n", block->data.vorbis_comment.num_comments);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on middle of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on end of array...");
	vc_delete_(vorbiscomment, 2);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_delete_comment() on beginning of array...");
	vc_delete_(vorbiscomment, 0);
	if(!FLAC__metadata_object_vorbiscomment_delete_comment(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_comment(own)...");
	vc_set_new_(&entry, vorbiscomment, 0, "name5=field5");
	entry_clone_(&entry);
	FLAC__metadata_object_vorbiscomment_set_comment(block, 0, entry, /*copy=*/false);
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_vorbiscomment_set_vendor_string(own)...");
	vc_set_vs_new_(&entry, vorbiscomment, "name6=field6");
	entry_clone_(&entry);
	FLAC__metadata_object_vorbiscomment_set_vendor_string(block, entry, /*copy=*/false);
	if(!mutils__compare_block(vorbiscomment, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(vorbiscomment);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing CUESHEET\n");

	{
		FLAC__StreamMetadata_CueSheet_Track *track_, *trackcopy_;

		printf("testing FLAC__metadata_object_cuesheet_track_new()... ");
		track_ = FLAC__metadata_object_cuesheet_track_new();
		if(0 == track_) {
			printf("FAILED, returned NULL\n");
			return false;
		}
		printf("OK\n");

		printf("testing FLAC__metadata_object_cuesheet_track_clone()... ");
		trackcopy_ = FLAC__metadata_object_cuesheet_track_clone(track_);
		if(0 == trackcopy_) {
			printf("FAILED, returned NULL\n");
			return false;
		}
		if(!compare_track_(trackcopy_, track_))
			return false;
		printf("OK\n");

		printf("testing FLAC__metadata_object_cuesheet_track_delete()... ");
		FLAC__metadata_object_cuesheet_track_delete(trackcopy_);
		FLAC__metadata_object_cuesheet_track_delete(track_);
		printf("OK\n");
	}


	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = (
		FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
		FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
		FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
	) / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	cuesheet = FLAC__metadata_object_clone(block);
	if(0 == cuesheet) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	cs_resize_(cuesheet, 2);
	printf("testing FLAC__metadata_object_cuesheet_resize_tracks(grow to %u)...", cuesheet->data.cue_sheet.num_tracks);
	if(!FLAC__metadata_object_cuesheet_resize_tracks(block, cuesheet->data.cue_sheet.num_tracks)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	cs_resize_(cuesheet, 1);
	printf("testing FLAC__metadata_object_cuesheet_resize_tracks(shrink to %u)...", cuesheet->data.cue_sheet.num_tracks);
	if(!FLAC__metadata_object_cuesheet_resize_tracks(block, cuesheet->data.cue_sheet.num_tracks)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	cs_resize_(cuesheet, 0);
	printf("testing FLAC__metadata_object_cuesheet_resize_tracks(shrink to %u)...", cuesheet->data.cue_sheet.num_tracks);
	if(!FLAC__metadata_object_cuesheet_resize_tracks(block, cuesheet->data.cue_sheet.num_tracks)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(copy) on empty array...");
	cs_insert_new_(&track, cuesheet, 0, 0, 1, "ABCDE1234567", false, false);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 0, &track, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(copy) on beginning of non-empty array...");
	cs_insert_new_(&track, cuesheet, 0, 10, 2, "BBCDE1234567", false, false);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 0, &track, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(copy) on middle of non-empty array...");
	cs_insert_new_(&track, cuesheet, 1, 20, 3, "CBCDE1234567", false, false);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 1, &track, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(copy) on end of non-empty array...");
	cs_insert_new_(&track, cuesheet, 3, 30, 4, "DBCDE1234567", false, false);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 3, &track, /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_blank_track() on end of non-empty array...");
	cs_insert_new_(&track, cuesheet, 4, 0, 0, "\0\0\0\0\0\0\0\0\0\0\0\0", false, false);
	if(!FLAC__metadata_object_cuesheet_insert_blank_track(block, 4)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on end of array...");
	cs_delete_(cuesheet, 4);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 4)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on middle of array...");
	cs_delete_(cuesheet, 2);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on end of array...");
	cs_delete_(cuesheet, 2);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on beginning of array...");
	cs_delete_(cuesheet, 0);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_set_track(copy)...");
	cs_set_new_(&track, cuesheet, 0, 40, 5, "EBCDE1234567", false, false);
	FLAC__metadata_object_cuesheet_set_track(block, 0, &track, /*copy=*/true);
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	tr_resize_(cuesheet, 0, 2);
	printf("testing FLAC__metadata_object_cuesheet_track_resize_indices(grow to %u)...", cuesheet->data.cue_sheet.tracks[0].num_indices);
	if(!FLAC__metadata_object_cuesheet_track_resize_indices(block, 0, cuesheet->data.cue_sheet.tracks[0].num_indices)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	tr_resize_(cuesheet, 0, 1);
	printf("testing FLAC__metadata_object_cuesheet_track_resize_indices(shrink to %u)...", cuesheet->data.cue_sheet.tracks[0].num_indices);
	if(!FLAC__metadata_object_cuesheet_track_resize_indices(block, 0, cuesheet->data.cue_sheet.tracks[0].num_indices)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	tr_resize_(cuesheet, 0, 0);
	printf("testing FLAC__metadata_object_cuesheet_track_resize_indices(shrink to %u)...", cuesheet->data.cue_sheet.tracks[0].num_indices);
	if(!FLAC__metadata_object_cuesheet_track_resize_indices(block, 0, cuesheet->data.cue_sheet.tracks[0].num_indices)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	index.offset = 0;
	index.number = 1;
	printf("testing FLAC__metadata_object_cuesheet_track_insert_index() on empty array...");
	tr_insert_new_(cuesheet, 0, 0, index);
	if(!FLAC__metadata_object_cuesheet_track_insert_index(block, 0, 0, index)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	index.offset = 10;
	index.number = 2;
	printf("testing FLAC__metadata_object_cuesheet_track_insert_index() on beginning of non-empty array...");
	tr_insert_new_(cuesheet, 0, 0, index);
	if(!FLAC__metadata_object_cuesheet_track_insert_index(block, 0, 0, index)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	index.offset = 20;
	index.number = 3;
	printf("testing FLAC__metadata_object_cuesheet_track_insert_index() on middle of non-empty array...");
	tr_insert_new_(cuesheet, 0, 1, index);
	if(!FLAC__metadata_object_cuesheet_track_insert_index(block, 0, 1, index)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	index.offset = 30;
	index.number = 4;
	printf("testing FLAC__metadata_object_cuesheet_track_insert_index() on end of non-empty array...");
	tr_insert_new_(cuesheet, 0, 3, index);
	if(!FLAC__metadata_object_cuesheet_track_insert_index(block, 0, 3, index)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	index.offset = 0;
	index.number = 0;
	printf("testing FLAC__metadata_object_cuesheet_track_insert_blank_index() on end of non-empty array...");
	tr_insert_new_(cuesheet, 0, 4, index);
	if(!FLAC__metadata_object_cuesheet_track_insert_blank_index(block, 0, 4)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_track_delete_index() on end of array...");
	tr_delete_(cuesheet, 0, 4);
	if(!FLAC__metadata_object_cuesheet_track_delete_index(block, 0, 4)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_track_delete_index() on middle of array...");
	tr_delete_(cuesheet, 0, 2);
	if(!FLAC__metadata_object_cuesheet_track_delete_index(block, 0, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_track_delete_index() on end of array...");
	tr_delete_(cuesheet, 0, 2);
	if(!FLAC__metadata_object_cuesheet_track_delete_index(block, 0, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_track_delete_index() on beginning of array...");
	tr_delete_(cuesheet, 0, 0);
	if(!FLAC__metadata_object_cuesheet_track_delete_index(block, 0, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(cuesheet);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	cuesheet = FLAC__metadata_object_clone(block);
	if(0 == cuesheet) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(own) on empty array...");
	cs_insert_new_(&track, cuesheet, 0, 60, 7, "GBCDE1234567", false, false);
	track_clone_(&track);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 0, &track, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(own) on beginning of non-empty array...");
	cs_insert_new_(&track, cuesheet, 0, 70, 8, "HBCDE1234567", false, false);
	track_clone_(&track);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 0, &track, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(own) on middle of non-empty array...");
	cs_insert_new_(&track, cuesheet, 1, 80, 9, "IBCDE1234567", false, false);
	track_clone_(&track);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 1, &track, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_insert_track(own) on end of non-empty array...");
	cs_insert_new_(&track, cuesheet, 3, 90, 10, "JBCDE1234567", false, false);
	track_clone_(&track);
	if(!FLAC__metadata_object_cuesheet_insert_track(block, 3, &track, /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on middle of array...");
	cs_delete_(cuesheet, 2);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on end of array...");
	cs_delete_(cuesheet, 2);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 2)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_delete_track() on beginning of array...");
	cs_delete_(cuesheet, 0);
	if(!FLAC__metadata_object_cuesheet_delete_track(block, 0)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_set_track(own)...");
	cs_set_new_(&track, cuesheet, 0, 100, 11, "KBCDE1234567", false, false);
	track_clone_(&track);
	FLAC__metadata_object_cuesheet_set_track(block, 0, &track, /*copy=*/false);
	if(!mutils__compare_block(cuesheet, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_cuesheet_is_legal()...");
	{
		const char *violation;
		if(FLAC__metadata_object_cuesheet_is_legal(block, /*check_cd_da_subset=*/true, &violation)) {
			printf("FAILED, returned true when expecting false\n");
			return false;
		}
		printf("returned false as expected, violation=\"%s\" OK\n", violation);
	}

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(cuesheet);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	printf("testing PICTURE\n");

	printf("testing FLAC__metadata_object_new()... ");
	block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PICTURE);
	if(0 == block) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	expected_length = (
		FLAC__STREAM_METADATA_PICTURE_TYPE_LEN +
		FLAC__STREAM_METADATA_PICTURE_MIME_TYPE_LENGTH_LEN +
		FLAC__STREAM_METADATA_PICTURE_DESCRIPTION_LENGTH_LEN +
		FLAC__STREAM_METADATA_PICTURE_WIDTH_LEN +
		FLAC__STREAM_METADATA_PICTURE_HEIGHT_LEN +
		FLAC__STREAM_METADATA_PICTURE_DEPTH_LEN +
		FLAC__STREAM_METADATA_PICTURE_COLORS_LEN +
		FLAC__STREAM_METADATA_PICTURE_DATA_LENGTH_LEN
	) / 8;
	if(block->length != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block->length);
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	picture = FLAC__metadata_object_clone(block);
	if(0 == picture) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	pi_set_mime_type(picture, "image/png\t");
	printf("testing FLAC__metadata_object_picture_set_mime_type(copy)...");
	if(!FLAC__metadata_object_picture_set_mime_type(block, "image/png\t", /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned true when expecting false\n");
			return false;
		}
		printf("returned false as expected, violation=\"%s\" OK\n", violation);
	}

	pi_set_mime_type(picture, "image/png");
	printf("testing FLAC__metadata_object_picture_set_mime_type(copy)...");
	if(!FLAC__metadata_object_picture_set_mime_type(block, "image/png", /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(!FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned false, violation=\"%s\"\n", violation);
			return false;
		}
		printf("OK\n");
	}

	pi_set_description(picture, (const FLAC__byte *)"DESCRIPTION\xff");
	printf("testing FLAC__metadata_object_picture_set_description(copy)...");
	if(!FLAC__metadata_object_picture_set_description(block, (FLAC__byte *)"DESCRIPTION\xff", /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned true when expecting false\n");
			return false;
		}
		printf("returned false as expected, violation=\"%s\" OK\n", violation);
	}

	pi_set_description(picture, (const FLAC__byte *)"DESCRIPTION");
	printf("testing FLAC__metadata_object_picture_set_description(copy)...");
	if(!FLAC__metadata_object_picture_set_description(block, (FLAC__byte *)"DESCRIPTION", /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(!FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned false, violation=\"%s\"\n", violation);
			return false;
		}
		printf("OK\n");
	}


	pi_set_data(picture, (const FLAC__byte*)"PNGDATA", strlen("PNGDATA"));
	printf("testing FLAC__metadata_object_picture_set_data(copy)...");
	if(!FLAC__metadata_object_picture_set_data(block, (FLAC__byte*)"PNGDATA", strlen("PNGDATA"), /*copy=*/true)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	pi_set_mime_type(picture, "image/png\t");
	printf("testing FLAC__metadata_object_picture_set_mime_type(own)...");
	if(!FLAC__metadata_object_picture_set_mime_type(block, strdup("image/png\t"), /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned true when expecting false\n");
			return false;
		}
		printf("returned false as expected, violation=\"%s\" OK\n", violation);
	}

	pi_set_mime_type(picture, "image/png");
	printf("testing FLAC__metadata_object_picture_set_mime_type(own)...");
	if(!FLAC__metadata_object_picture_set_mime_type(block, strdup("image/png"), /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(!FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned false, violation=\"%s\"\n", violation);
			return false;
		}
		printf("OK\n");
	}

	pi_set_description(picture, (const FLAC__byte *)"DESCRIPTION\xff");
	printf("testing FLAC__metadata_object_picture_set_description(own)...");
	if(!FLAC__metadata_object_picture_set_description(block, (FLAC__byte *)strdup("DESCRIPTION\xff"), /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned true when expecting false\n");
			return false;
		}
		printf("returned false as expected, violation=\"%s\" OK\n", violation);
	}

	pi_set_description(picture, (const FLAC__byte *)"DESCRIPTION");
	printf("testing FLAC__metadata_object_picture_set_description(own)...");
	if(!FLAC__metadata_object_picture_set_description(block, (FLAC__byte *)strdup("DESCRIPTION"), /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_picture_is_legal()...");
	{
		const char *violation;
		if(!FLAC__metadata_object_picture_is_legal(block, &violation)) {
			printf("FAILED, returned false, violation=\"%s\"\n", violation);
			return false;
		}
		printf("OK\n");
	}

	pi_set_data(picture, (const FLAC__byte*)"PNGDATA", strlen("PNGDATA"));
	printf("testing FLAC__metadata_object_picture_set_data(own)...");
	if(!FLAC__metadata_object_picture_set_data(block, (FLAC__byte*)strdup("PNGDATA"), strlen("PNGDATA"), /*copy=*/false)) {
		printf("FAILED, returned false\n");
		return false;
	}
	if(!mutils__compare_block(picture, block))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_clone()... ");
	blockcopy = FLAC__metadata_object_clone(block);
	if(0 == blockcopy) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	if(!mutils__compare_block(block, blockcopy))
		return false;
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(blockcopy);
	printf("OK\n");

	printf("testing FLAC__metadata_object_delete()... ");
	FLAC__metadata_object_delete(picture);
	FLAC__metadata_object_delete(block);
	printf("OK\n");


	return true;
}
