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

/*
 * These are not tests, just utility functions used by the metadata tests
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FLAC/metadata.h"
#include "test_libs_common/metadata_utils.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

FLAC__bool mutils__compare_block_data_streaminfo(const FLAC__StreamMetadata_StreamInfo *block, const FLAC__StreamMetadata_StreamInfo *blockcopy)
{
	if(blockcopy->min_blocksize != block->min_blocksize) {
		printf("FAILED, min_blocksize mismatch, expected %u, got %u\n", block->min_blocksize, blockcopy->min_blocksize);
		return false;
	}
	if(blockcopy->max_blocksize != block->max_blocksize) {
		printf("FAILED, max_blocksize mismatch, expected %u, got %u\n", block->max_blocksize, blockcopy->max_blocksize);
		return false;
	}
	if(blockcopy->min_framesize != block->min_framesize) {
		printf("FAILED, min_framesize mismatch, expected %u, got %u\n", block->min_framesize, blockcopy->min_framesize);
		return false;
	}
	if(blockcopy->max_framesize != block->max_framesize) {
		printf("FAILED, max_framesize mismatch, expected %u, got %u\n", block->max_framesize, blockcopy->max_framesize);
		return false;
	}
	if(blockcopy->sample_rate != block->sample_rate) {
		printf("FAILED, sample_rate mismatch, expected %u, got %u\n", block->sample_rate, blockcopy->sample_rate);
		return false;
	}
	if(blockcopy->channels != block->channels) {
		printf("FAILED, channels mismatch, expected %u, got %u\n", block->channels, blockcopy->channels);
		return false;
	}
	if(blockcopy->bits_per_sample != block->bits_per_sample) {
		printf("FAILED, bits_per_sample mismatch, expected %u, got %u\n", block->bits_per_sample, blockcopy->bits_per_sample);
		return false;
	}
	if(blockcopy->total_samples != block->total_samples) {
#ifdef _MSC_VER
		printf("FAILED, total_samples mismatch, expected %I64u, got %I64u\n", block->total_samples, blockcopy->total_samples);
#else
		printf("FAILED, total_samples mismatch, expected %llu, got %llu\n", (unsigned long long)block->total_samples, (unsigned long long)blockcopy->total_samples);
#endif
		return false;
	}
	if(0 != memcmp(blockcopy->md5sum, block->md5sum, sizeof(block->md5sum))) {
		printf("FAILED, md5sum mismatch, expected %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X, got %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
			(unsigned)block->md5sum[0],
			(unsigned)block->md5sum[1],
			(unsigned)block->md5sum[2],
			(unsigned)block->md5sum[3],
			(unsigned)block->md5sum[4],
			(unsigned)block->md5sum[5],
			(unsigned)block->md5sum[6],
			(unsigned)block->md5sum[7],
			(unsigned)block->md5sum[8],
			(unsigned)block->md5sum[9],
			(unsigned)block->md5sum[10],
			(unsigned)block->md5sum[11],
			(unsigned)block->md5sum[12],
			(unsigned)block->md5sum[13],
			(unsigned)block->md5sum[14],
			(unsigned)block->md5sum[15],
			(unsigned)blockcopy->md5sum[0],
			(unsigned)blockcopy->md5sum[1],
			(unsigned)blockcopy->md5sum[2],
			(unsigned)blockcopy->md5sum[3],
			(unsigned)blockcopy->md5sum[4],
			(unsigned)blockcopy->md5sum[5],
			(unsigned)blockcopy->md5sum[6],
			(unsigned)blockcopy->md5sum[7],
			(unsigned)blockcopy->md5sum[8],
			(unsigned)blockcopy->md5sum[9],
			(unsigned)blockcopy->md5sum[10],
			(unsigned)blockcopy->md5sum[11],
			(unsigned)blockcopy->md5sum[12],
			(unsigned)blockcopy->md5sum[13],
			(unsigned)blockcopy->md5sum[14],
			(unsigned)blockcopy->md5sum[15]
		);
		return false;
	}
	return true;
}

FLAC__bool mutils__compare_block_data_padding(const FLAC__StreamMetadata_Padding *block, const FLAC__StreamMetadata_Padding *blockcopy, unsigned block_length)
{
	/* we don't compare the padding guts */
	(void)block, (void)blockcopy, (void)block_length;
	return true;
}

FLAC__bool mutils__compare_block_data_application(const FLAC__StreamMetadata_Application *block, const FLAC__StreamMetadata_Application *blockcopy, unsigned block_length)
{
	if(block_length < sizeof(block->id)) {
		printf("FAILED, bad block length = %u\n", block_length);
		return false;
	}
	if(0 != memcmp(blockcopy->id, block->id, sizeof(block->id))) {
		printf("FAILED, id mismatch, expected %02X%02X%02X%02X, got %02X%02X%02X%02X\n",
			(unsigned)block->id[0],
			(unsigned)block->id[1],
			(unsigned)block->id[2],
			(unsigned)block->id[3],
			(unsigned)blockcopy->id[0],
			(unsigned)blockcopy->id[1],
			(unsigned)blockcopy->id[2],
			(unsigned)blockcopy->id[3]
		);
		return false;
	}
	if(0 == block->data || 0 == blockcopy->data) {
		if(block->data != blockcopy->data) {
			printf("FAILED, data mismatch (%s's data pointer is null)\n", 0==block->data?"original":"copy");
			return false;
		}
		else if(block_length - sizeof(block->id) > 0) {
			printf("FAILED, data pointer is null but block length is not 0\n");
			return false;
		}
	}
	else {
		if(block_length - sizeof(block->id) == 0) {
			printf("FAILED, data pointer is not null but block length is 0\n");
			return false;
		}
		else if(0 != memcmp(blockcopy->data, block->data, block_length - sizeof(block->id))) {
			printf("FAILED, data mismatch\n");
			return false;
		}
	}
	return true;
}

FLAC__bool mutils__compare_block_data_seektable(const FLAC__StreamMetadata_SeekTable *block, const FLAC__StreamMetadata_SeekTable *blockcopy)
{
	unsigned i;
	if(blockcopy->num_points != block->num_points) {
		printf("FAILED, num_points mismatch, expected %u, got %u\n", block->num_points, blockcopy->num_points);
		return false;
	}
	for(i = 0; i < block->num_points; i++) {
		if(blockcopy->points[i].sample_number != block->points[i].sample_number) {
#ifdef _MSC_VER
			printf("FAILED, points[%u].sample_number mismatch, expected %I64u, got %I64u\n", i, block->points[i].sample_number, blockcopy->points[i].sample_number);
#else
			printf("FAILED, points[%u].sample_number mismatch, expected %llu, got %llu\n", i, (unsigned long long)block->points[i].sample_number, (unsigned long long)blockcopy->points[i].sample_number);
#endif
			return false;
		}
		if(blockcopy->points[i].stream_offset != block->points[i].stream_offset) {
#ifdef _MSC_VER
			printf("FAILED, points[%u].stream_offset mismatch, expected %I64u, got %I64u\n", i, block->points[i].stream_offset, blockcopy->points[i].stream_offset);
#else
			printf("FAILED, points[%u].stream_offset mismatch, expected %llu, got %llu\n", i, (unsigned long long)block->points[i].stream_offset, (unsigned long long)blockcopy->points[i].stream_offset);
#endif
			return false;
		}
		if(blockcopy->points[i].frame_samples != block->points[i].frame_samples) {
			printf("FAILED, points[%u].frame_samples mismatch, expected %u, got %u\n", i, block->points[i].frame_samples, blockcopy->points[i].frame_samples);
			return false;
		}
	}
	return true;
}

FLAC__bool mutils__compare_block_data_vorbiscomment(const FLAC__StreamMetadata_VorbisComment *block, const FLAC__StreamMetadata_VorbisComment *blockcopy)
{
	unsigned i;
	if(blockcopy->vendor_string.length != block->vendor_string.length) {
		printf("FAILED, vendor_string.length mismatch, expected %u, got %u\n", block->vendor_string.length, blockcopy->vendor_string.length);
		return false;
	}
	if(0 == block->vendor_string.entry || 0 == blockcopy->vendor_string.entry) {
		if(block->vendor_string.entry != blockcopy->vendor_string.entry) {
			printf("FAILED, vendor_string.entry mismatch\n");
			return false;
		}
	}
	else if(0 != memcmp(blockcopy->vendor_string.entry, block->vendor_string.entry, block->vendor_string.length)) {
		printf("FAILED, vendor_string.entry mismatch\n");
		return false;
	}
	if(blockcopy->num_comments != block->num_comments) {
		printf("FAILED, num_comments mismatch, expected %u, got %u\n", block->num_comments, blockcopy->num_comments);
		return false;
	}
	for(i = 0; i < block->num_comments; i++) {
		if(blockcopy->comments[i].length != block->comments[i].length) {
			printf("FAILED, comments[%u].length mismatch, expected %u, got %u\n", i, block->comments[i].length, blockcopy->comments[i].length);
			return false;
		}
		if(0 == block->comments[i].entry || 0 == blockcopy->comments[i].entry) {
			if(block->comments[i].entry != blockcopy->comments[i].entry) {
				printf("FAILED, comments[%u].entry mismatch\n", i);
				return false;
			}
		}
		else {
			if(0 != memcmp(blockcopy->comments[i].entry, block->comments[i].entry, block->comments[i].length)) {
				printf("FAILED, comments[%u].entry mismatch\n", i);
				return false;
			}
		}
	}
	return true;
}

FLAC__bool mutils__compare_block_data_cuesheet(const FLAC__StreamMetadata_CueSheet *block, const FLAC__StreamMetadata_CueSheet *blockcopy)
{
	unsigned i, j;

	if(0 != strcmp(blockcopy->media_catalog_number, block->media_catalog_number)) {
		printf("FAILED, media_catalog_number mismatch, expected %s, got %s\n", block->media_catalog_number, blockcopy->media_catalog_number);
		return false;
	}
	if(blockcopy->lead_in != block->lead_in) {
#ifdef _MSC_VER
		printf("FAILED, lead_in mismatch, expected %I64u, got %I64u\n", block->lead_in, blockcopy->lead_in);
#else
		printf("FAILED, lead_in mismatch, expected %llu, got %llu\n", (unsigned long long)block->lead_in, (unsigned long long)blockcopy->lead_in);
#endif
		return false;
	}
	if(blockcopy->is_cd != block->is_cd) {
		printf("FAILED, is_cd mismatch, expected %u, got %u\n", (unsigned)block->is_cd, (unsigned)blockcopy->is_cd);
		return false;
	}
	if(blockcopy->num_tracks != block->num_tracks) {
		printf("FAILED, num_tracks mismatch, expected %u, got %u\n", block->num_tracks, blockcopy->num_tracks);
		return false;
	}
	for(i = 0; i < block->num_tracks; i++) {
		if(blockcopy->tracks[i].offset != block->tracks[i].offset) {
#ifdef _MSC_VER
			printf("FAILED, tracks[%u].offset mismatch, expected %I64u, got %I64u\n", i, block->tracks[i].offset, blockcopy->tracks[i].offset);
#else
			printf("FAILED, tracks[%u].offset mismatch, expected %llu, got %llu\n", i, (unsigned long long)block->tracks[i].offset, (unsigned long long)blockcopy->tracks[i].offset);
#endif
			return false;
		}
		if(blockcopy->tracks[i].number != block->tracks[i].number) {
			printf("FAILED, tracks[%u].number mismatch, expected %u, got %u\n", i, (unsigned)block->tracks[i].number, (unsigned)blockcopy->tracks[i].number);
			return false;
		}
		if(blockcopy->tracks[i].num_indices != block->tracks[i].num_indices) {
			printf("FAILED, tracks[%u].num_indices mismatch, expected %u, got %u\n", i, (unsigned)block->tracks[i].num_indices, (unsigned)blockcopy->tracks[i].num_indices);
			return false;
		}
		/* num_indices == 0 means lead-out track so only the track offset and number are valid */
		if(block->tracks[i].num_indices > 0) {
			if(0 != strcmp(blockcopy->tracks[i].isrc, block->tracks[i].isrc)) {
				printf("FAILED, tracks[%u].isrc mismatch, expected %s, got %s\n", i, block->tracks[i].isrc, blockcopy->tracks[i].isrc);
				return false;
			}
			if(blockcopy->tracks[i].type != block->tracks[i].type) {
				printf("FAILED, tracks[%u].type mismatch, expected %u, got %u\n", i, (unsigned)block->tracks[i].type, (unsigned)blockcopy->tracks[i].type);
				return false;
			}
			if(blockcopy->tracks[i].pre_emphasis != block->tracks[i].pre_emphasis) {
				printf("FAILED, tracks[%u].pre_emphasis mismatch, expected %u, got %u\n", i, (unsigned)block->tracks[i].pre_emphasis, (unsigned)blockcopy->tracks[i].pre_emphasis);
				return false;
			}
			if(0 == block->tracks[i].indices || 0 == blockcopy->tracks[i].indices) {
				if(block->tracks[i].indices != blockcopy->tracks[i].indices) {
					printf("FAILED, tracks[%u].indices mismatch\n", i);
					return false;
				}
			}
			else {
				for(j = 0; j < block->tracks[i].num_indices; j++) {
					if(blockcopy->tracks[i].indices[j].offset != block->tracks[i].indices[j].offset) {
#ifdef _MSC_VER
						printf("FAILED, tracks[%u].indices[%u].offset mismatch, expected %I64u, got %I64u\n", i, j, block->tracks[i].indices[j].offset, blockcopy->tracks[i].indices[j].offset);
#else
						printf("FAILED, tracks[%u].indices[%u].offset mismatch, expected %llu, got %llu\n", i, j, (unsigned long long)block->tracks[i].indices[j].offset, (unsigned long long)blockcopy->tracks[i].indices[j].offset);
#endif
						return false;
					}
					if(blockcopy->tracks[i].indices[j].number != block->tracks[i].indices[j].number) {
						printf("FAILED, tracks[%u].indices[%u].number mismatch, expected %u, got %u\n", i, j, (unsigned)block->tracks[i].indices[j].number, (unsigned)blockcopy->tracks[i].indices[j].number);
						return false;
					}
				}
			}
		}
	}
	return true;
}

FLAC__bool mutils__compare_block_data_picture(const FLAC__StreamMetadata_Picture *block, const FLAC__StreamMetadata_Picture *blockcopy)
{
	size_t len, lencopy;
	if(blockcopy->type != block->type) {
		printf("FAILED, type mismatch, expected %u, got %u\n", (unsigned)block->type, (unsigned)blockcopy->type);
		return false;
	}
	len = strlen(block->mime_type);
	lencopy = strlen(blockcopy->mime_type);
	if(lencopy != len) {
		printf("FAILED, mime_type length mismatch, expected %u, got %u\n", (unsigned)len, (unsigned)lencopy);
		return false;
	}
	if(strcmp(blockcopy->mime_type, block->mime_type)) {
		printf("FAILED, mime_type mismatch, expected %s, got %s\n", block->mime_type, blockcopy->mime_type);
		return false;
	}
	len = strlen((const char *)block->description);
	lencopy = strlen((const char *)blockcopy->description);
	if(lencopy != len) {
		printf("FAILED, description length mismatch, expected %u, got %u\n", (unsigned)len, (unsigned)lencopy);
		return false;
	}
	if(strcmp((const char *)blockcopy->description, (const char *)block->description)) {
		printf("FAILED, description mismatch, expected %s, got %s\n", block->description, blockcopy->description);
		return false;
	}
	if(blockcopy->width != block->width) {
		printf("FAILED, width mismatch, expected %u, got %u\n", block->width, blockcopy->width);
		return false;
	}
	if(blockcopy->height != block->height) {
		printf("FAILED, height mismatch, expected %u, got %u\n", block->height, blockcopy->height);
		return false;
	}
	if(blockcopy->depth != block->depth) {
		printf("FAILED, depth mismatch, expected %u, got %u\n", block->depth, blockcopy->depth);
		return false;
	}
	if(blockcopy->colors != block->colors) {
		printf("FAILED, colors mismatch, expected %u, got %u\n", block->colors, blockcopy->colors);
		return false;
	}
	if(blockcopy->data_length != block->data_length) {
		printf("FAILED, data_length mismatch, expected %u, got %u\n", block->data_length, blockcopy->data_length);
		return false;
	}
	if(memcmp(blockcopy->data, block->data, block->data_length)) {
		printf("FAILED, data mismatch\n");
		return false;
	}
	return true;
}

FLAC__bool mutils__compare_block_data_unknown(const FLAC__StreamMetadata_Unknown *block, const FLAC__StreamMetadata_Unknown *blockcopy, unsigned block_length)
{
	if(0 == block->data || 0 == blockcopy->data) {
		if(block->data != blockcopy->data) {
			printf("FAILED, data mismatch (%s's data pointer is null)\n", 0==block->data?"original":"copy");
			return false;
		}
		else if(block_length > 0) {
			printf("FAILED, data pointer is null but block length is not 0\n");
			return false;
		}
	}
	else {
		if(block_length == 0) {
			printf("FAILED, data pointer is not null but block length is 0\n");
			return false;
		}
		else if(0 != memcmp(blockcopy->data, block->data, block_length)) {
			printf("FAILED, data mismatch\n");
			return false;
		}
	}
	return true;
}

FLAC__bool mutils__compare_block(const FLAC__StreamMetadata *block, const FLAC__StreamMetadata *blockcopy)
{
	if(blockcopy->type != block->type) {
		printf("FAILED, type mismatch, expected %s, got %s\n", FLAC__MetadataTypeString[block->type], FLAC__MetadataTypeString[blockcopy->type]);
		return false;
	}
	if(blockcopy->is_last != block->is_last) {
		printf("FAILED, is_last mismatch, expected %u, got %u\n", (unsigned)block->is_last, (unsigned)blockcopy->is_last);
		return false;
	}
	if(blockcopy->length != block->length) {
		printf("FAILED, length mismatch, expected %u, got %u\n", block->length, blockcopy->length);
		return false;
	}
	switch(block->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			return mutils__compare_block_data_streaminfo(&block->data.stream_info, &blockcopy->data.stream_info);
		case FLAC__METADATA_TYPE_PADDING:
			return mutils__compare_block_data_padding(&block->data.padding, &blockcopy->data.padding, block->length);
		case FLAC__METADATA_TYPE_APPLICATION:
			return mutils__compare_block_data_application(&block->data.application, &blockcopy->data.application, block->length);
		case FLAC__METADATA_TYPE_SEEKTABLE:
			return mutils__compare_block_data_seektable(&block->data.seek_table, &blockcopy->data.seek_table);
		case FLAC__METADATA_TYPE_VORBIS_COMMENT:
			return mutils__compare_block_data_vorbiscomment(&block->data.vorbis_comment, &blockcopy->data.vorbis_comment);
		case FLAC__METADATA_TYPE_CUESHEET:
			return mutils__compare_block_data_cuesheet(&block->data.cue_sheet, &blockcopy->data.cue_sheet);
		case FLAC__METADATA_TYPE_PICTURE:
			return mutils__compare_block_data_picture(&block->data.picture, &blockcopy->data.picture);
		default:
			return mutils__compare_block_data_unknown(&block->data.unknown, &blockcopy->data.unknown, block->length);
	}
}

static void *malloc_or_die_(size_t size)
{
	void *x = malloc(size);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory allocating %u bytes\n", (unsigned)size);
		exit(1);
	}
	return x;
}

static void *calloc_or_die_(size_t n, size_t size)
{
	void *x = calloc(n, size);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory allocating %u bytes\n", (unsigned)n * (unsigned)size);
		exit(1);
	}
	return x;
}

static char *strdup_or_die_(const char *s)
{
	char *x = strdup(s);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory copying string \"%s\"\n", s);
		exit(1);
	}
	return x;
}

void mutils__init_metadata_blocks(
	FLAC__StreamMetadata *streaminfo,
	FLAC__StreamMetadata *padding,
	FLAC__StreamMetadata *seektable,
	FLAC__StreamMetadata *application1,
	FLAC__StreamMetadata *application2,
	FLAC__StreamMetadata *vorbiscomment,
	FLAC__StreamMetadata *cuesheet,
	FLAC__StreamMetadata *picture,
	FLAC__StreamMetadata *unknown
)
{
	/*
		most of the actual numbers and data in the blocks don't matter,
		we just want to make sure the decoder parses them correctly

		remember, the metadata interface gets tested after the decoders,
		so we do all the metadata manipulation here without it.
	*/

	/* min/max_framesize and md5sum don't get written at first, so we have to leave them 0 */
	streaminfo->is_last = false;
	streaminfo->type = FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo->length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo->data.stream_info.min_blocksize = 576;
	streaminfo->data.stream_info.max_blocksize = 576;
	streaminfo->data.stream_info.min_framesize = 0;
	streaminfo->data.stream_info.max_framesize = 0;
	streaminfo->data.stream_info.sample_rate = 44100;
	streaminfo->data.stream_info.channels = 1;
	streaminfo->data.stream_info.bits_per_sample = 8;
	streaminfo->data.stream_info.total_samples = 0;
	memset(streaminfo->data.stream_info.md5sum, 0, 16);

	padding->is_last = false;
	padding->type = FLAC__METADATA_TYPE_PADDING;
	padding->length = 1234;

	seektable->is_last = false;
	seektable->type = FLAC__METADATA_TYPE_SEEKTABLE;
	seektable->data.seek_table.num_points = 2;
	seektable->length = seektable->data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable->data.seek_table.points = (FLAC__StreamMetadata_SeekPoint*)malloc_or_die_(seektable->data.seek_table.num_points * sizeof(FLAC__StreamMetadata_SeekPoint));
	seektable->data.seek_table.points[0].sample_number = 0;
	seektable->data.seek_table.points[0].stream_offset = 0;
	seektable->data.seek_table.points[0].frame_samples = streaminfo->data.stream_info.min_blocksize;
	seektable->data.seek_table.points[1].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable->data.seek_table.points[1].stream_offset = 1000;
	seektable->data.seek_table.points[1].frame_samples = streaminfo->data.stream_info.min_blocksize;

	application1->is_last = false;
	application1->type = FLAC__METADATA_TYPE_APPLICATION;
	application1->length = 8;
	memcpy(application1->data.application.id, "\xfe\xdc\xba\x98", 4);
	application1->data.application.data = (FLAC__byte*)malloc_or_die_(4);
	memcpy(application1->data.application.data, "\xf0\xe1\xd2\xc3", 4);

	application2->is_last = false;
	application2->type = FLAC__METADATA_TYPE_APPLICATION;
	application2->length = 4;
	memcpy(application2->data.application.id, "\x76\x54\x32\x10", 4);
	application2->data.application.data = 0;

	{
		const unsigned vendor_string_length = (unsigned)strlen(FLAC__VENDOR_STRING);
		vorbiscomment->is_last = false;
		vorbiscomment->type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		vorbiscomment->length = (4 + vendor_string_length) + 4 + (4 + 5) + (4 + 0);
		vorbiscomment->data.vorbis_comment.vendor_string.length = vendor_string_length;
		vorbiscomment->data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(vendor_string_length+1);
		memcpy(vorbiscomment->data.vorbis_comment.vendor_string.entry, FLAC__VENDOR_STRING, vendor_string_length+1);
		vorbiscomment->data.vorbis_comment.num_comments = 2;
		vorbiscomment->data.vorbis_comment.comments = (FLAC__StreamMetadata_VorbisComment_Entry*)malloc_or_die_(vorbiscomment->data.vorbis_comment.num_comments * sizeof(FLAC__StreamMetadata_VorbisComment_Entry));
		vorbiscomment->data.vorbis_comment.comments[0].length = 5;
		vorbiscomment->data.vorbis_comment.comments[0].entry = (FLAC__byte*)malloc_or_die_(5+1);
		memcpy(vorbiscomment->data.vorbis_comment.comments[0].entry, "ab=cd", 5+1);
		vorbiscomment->data.vorbis_comment.comments[1].length = 0;
		vorbiscomment->data.vorbis_comment.comments[1].entry = 0;
	}

	cuesheet->is_last = false;
	cuesheet->type = FLAC__METADATA_TYPE_CUESHEET;
	cuesheet->length =
		/* cuesheet guts */
		(
			FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
			FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
			FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
			FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
		) / 8 +
		/* 2 tracks */
		3 * (
			FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN +
			FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
		) / 8 +
		/* 3 index points */
		3 * (
			FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
		) / 8
	;
	memset(cuesheet->data.cue_sheet.media_catalog_number, 0, sizeof(cuesheet->data.cue_sheet.media_catalog_number));
	cuesheet->data.cue_sheet.media_catalog_number[0] = 'j';
	cuesheet->data.cue_sheet.media_catalog_number[1] = 'C';
	cuesheet->data.cue_sheet.lead_in = 2 * 44100;
	cuesheet->data.cue_sheet.is_cd = true;
	cuesheet->data.cue_sheet.num_tracks = 3;
	cuesheet->data.cue_sheet.tracks = (FLAC__StreamMetadata_CueSheet_Track*)calloc_or_die_(cuesheet->data.cue_sheet.num_tracks, sizeof(FLAC__StreamMetadata_CueSheet_Track));
	cuesheet->data.cue_sheet.tracks[0].offset = 0;
	cuesheet->data.cue_sheet.tracks[0].number = 1;
	memcpy(cuesheet->data.cue_sheet.tracks[0].isrc, "ACBDE1234567", sizeof(cuesheet->data.cue_sheet.tracks[0].isrc));
	cuesheet->data.cue_sheet.tracks[0].type = 0;
	cuesheet->data.cue_sheet.tracks[0].pre_emphasis = 1;
	cuesheet->data.cue_sheet.tracks[0].num_indices = 2;
	cuesheet->data.cue_sheet.tracks[0].indices = (FLAC__StreamMetadata_CueSheet_Index*)malloc_or_die_(cuesheet->data.cue_sheet.tracks[0].num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index));
	cuesheet->data.cue_sheet.tracks[0].indices[0].offset = 0;
	cuesheet->data.cue_sheet.tracks[0].indices[0].number = 0;
	cuesheet->data.cue_sheet.tracks[0].indices[1].offset = 123 * 588;
	cuesheet->data.cue_sheet.tracks[0].indices[1].number = 1;
	cuesheet->data.cue_sheet.tracks[1].offset = 1234 * 588;
	cuesheet->data.cue_sheet.tracks[1].number = 2;
	memcpy(cuesheet->data.cue_sheet.tracks[1].isrc, "ACBDE7654321", sizeof(cuesheet->data.cue_sheet.tracks[1].isrc));
	cuesheet->data.cue_sheet.tracks[1].type = 1;
	cuesheet->data.cue_sheet.tracks[1].pre_emphasis = 0;
	cuesheet->data.cue_sheet.tracks[1].num_indices = 1;
	cuesheet->data.cue_sheet.tracks[1].indices = (FLAC__StreamMetadata_CueSheet_Index*)malloc_or_die_(cuesheet->data.cue_sheet.tracks[1].num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index));
	cuesheet->data.cue_sheet.tracks[1].indices[0].offset = 0;
	cuesheet->data.cue_sheet.tracks[1].indices[0].number = 1;
	cuesheet->data.cue_sheet.tracks[2].offset = 12345 * 588;
	cuesheet->data.cue_sheet.tracks[2].number = 170;
	cuesheet->data.cue_sheet.tracks[2].num_indices = 0;

	picture->is_last = false;
	picture->type = FLAC__METADATA_TYPE_PICTURE;
	picture->length =
		(
			FLAC__STREAM_METADATA_PICTURE_TYPE_LEN +
			FLAC__STREAM_METADATA_PICTURE_MIME_TYPE_LENGTH_LEN + /* will add the length for the string later */
			FLAC__STREAM_METADATA_PICTURE_DESCRIPTION_LENGTH_LEN + /* will add the length for the string later */
			FLAC__STREAM_METADATA_PICTURE_WIDTH_LEN +
			FLAC__STREAM_METADATA_PICTURE_HEIGHT_LEN +
			FLAC__STREAM_METADATA_PICTURE_DEPTH_LEN +
			FLAC__STREAM_METADATA_PICTURE_COLORS_LEN +
			FLAC__STREAM_METADATA_PICTURE_DATA_LENGTH_LEN /* will add the length for the data later */
		) / 8
	;
	picture->data.picture.type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;
	picture->data.picture.mime_type = strdup_or_die_("image/jpeg");
	picture->length += strlen(picture->data.picture.mime_type);
	picture->data.picture.description = (FLAC__byte*)strdup_or_die_("desc");
	picture->length += strlen((const char *)picture->data.picture.description);
	picture->data.picture.width = 300;
	picture->data.picture.height = 300;
	picture->data.picture.depth = 24;
	picture->data.picture.colors = 0;
	picture->data.picture.data = (FLAC__byte*)strdup_or_die_("SOMEJPEGDATA");
	picture->data.picture.data_length = strlen((const char *)picture->data.picture.data);
	picture->length += picture->data.picture.data_length;

	unknown->is_last = true;
	unknown->type = 126;
	unknown->length = 8;
	unknown->data.unknown.data = (FLAC__byte*)malloc_or_die_(unknown->length);
	memcpy(unknown->data.unknown.data, "\xfe\xdc\xba\x98\xf0\xe1\xd2\xc3", unknown->length);
}

void mutils__free_metadata_blocks(
	FLAC__StreamMetadata *streaminfo,
	FLAC__StreamMetadata *padding,
	FLAC__StreamMetadata *seektable,
	FLAC__StreamMetadata *application1,
	FLAC__StreamMetadata *application2,
	FLAC__StreamMetadata *vorbiscomment,
	FLAC__StreamMetadata *cuesheet,
	FLAC__StreamMetadata *picture,
	FLAC__StreamMetadata *unknown
)
{
	(void)streaminfo, (void)padding, (void)application2;
	free(seektable->data.seek_table.points);
	free(application1->data.application.data);
	free(vorbiscomment->data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment->data.vorbis_comment.comments[0].entry);
	free(vorbiscomment->data.vorbis_comment.comments);
	free(cuesheet->data.cue_sheet.tracks[0].indices);
	free(cuesheet->data.cue_sheet.tracks[1].indices);
	free(cuesheet->data.cue_sheet.tracks);
	free(picture->data.picture.mime_type);
	free(picture->data.picture.description);
	free(picture->data.picture.data);
	free(unknown->data.unknown.data);
}
