/* test_libFLAC++ - Unit tester for libFLAC++
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

#include "FLAC/assert.h"
#include "FLAC++/metadata.h"
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp() */

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application_, vorbiscomment_, cuesheet_, picture_;

static bool die_(const char *msg)
{
	printf("FAILED, %s\n", msg);
	return false;
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

static char *strdup_or_die_(const char *s)
{
	char *x = strdup(s);
	if(0 == x) {
		fprintf(stderr, "ERROR: out of memory copying string \"%s\"\n", s);
		exit(1);
	}
	return x;
}

static bool index_is_equal_(const ::FLAC__StreamMetadata_CueSheet_Index &index, const ::FLAC__StreamMetadata_CueSheet_Index &indexcopy)
{
	if(indexcopy.offset != index.offset)
		return false;
	if(indexcopy.number != index.number)
		return false;
	return true;
}

static bool track_is_equal_(const ::FLAC__StreamMetadata_CueSheet_Track *track, const ::FLAC__StreamMetadata_CueSheet_Track *trackcopy)
{
	unsigned i;

	if(trackcopy->offset != track->offset)
		return false;
	if(trackcopy->number != track->number)
		return false;
	if(0 != strcmp(trackcopy->isrc, track->isrc))
		return false;
	if(trackcopy->type != track->type)
		return false;
	if(trackcopy->pre_emphasis != track->pre_emphasis)
		return false;
	if(trackcopy->num_indices != track->num_indices)
		return false;
	if(0 == track->indices || 0 == trackcopy->indices) {
		if(track->indices != trackcopy->indices)
			return false;
	}
	else {
		for(i = 0; i < track->num_indices; i++) {
			if(!index_is_equal_(trackcopy->indices[i], track->indices[i]))
				return false;
		}
	}
	return true;
}

static void init_metadata_blocks_()
{
	streaminfo_.is_last = false;
	streaminfo_.type = ::FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo_.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo_.data.stream_info.min_blocksize = 576;
	streaminfo_.data.stream_info.max_blocksize = 576;
	streaminfo_.data.stream_info.min_framesize = 0;
	streaminfo_.data.stream_info.max_framesize = 0;
	streaminfo_.data.stream_info.sample_rate = 44100;
	streaminfo_.data.stream_info.channels = 1;
	streaminfo_.data.stream_info.bits_per_sample = 8;
	streaminfo_.data.stream_info.total_samples = 0;
	memset(streaminfo_.data.stream_info.md5sum, 0, 16);

	padding_.is_last = false;
	padding_.type = ::FLAC__METADATA_TYPE_PADDING;
	padding_.length = 1234;

	seektable_.is_last = false;
	seektable_.type = ::FLAC__METADATA_TYPE_SEEKTABLE;
	seektable_.data.seek_table.num_points = 2;
	seektable_.length = seektable_.data.seek_table.num_points * FLAC__STREAM_METADATA_SEEKPOINT_LENGTH;
	seektable_.data.seek_table.points = (::FLAC__StreamMetadata_SeekPoint*)malloc_or_die_(seektable_.data.seek_table.num_points * sizeof(::FLAC__StreamMetadata_SeekPoint));
	seektable_.data.seek_table.points[0].sample_number = 0;
	seektable_.data.seek_table.points[0].stream_offset = 0;
	seektable_.data.seek_table.points[0].frame_samples = streaminfo_.data.stream_info.min_blocksize;
	seektable_.data.seek_table.points[1].sample_number = ::FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
	seektable_.data.seek_table.points[1].stream_offset = 1000;
	seektable_.data.seek_table.points[1].frame_samples = streaminfo_.data.stream_info.min_blocksize;

	application_.is_last = false;
	application_.type = ::FLAC__METADATA_TYPE_APPLICATION;
	application_.length = 8;
	memcpy(application_.data.application.id, "\xfe\xdc\xba\x98", 4);
	application_.data.application.data = (FLAC__byte*)malloc_or_die_(4);
	memcpy(application_.data.application.data, "\xf0\xe1\xd2\xc3", 4);

	vorbiscomment_.is_last = false;
	vorbiscomment_.type = ::FLAC__METADATA_TYPE_VORBIS_COMMENT;
	vorbiscomment_.length = (4 + 5) + 4 + (4 + 12) + (4 + 12);
	vorbiscomment_.data.vorbis_comment.vendor_string.length = 5;
	vorbiscomment_.data.vorbis_comment.vendor_string.entry = (FLAC__byte*)malloc_or_die_(5+1);
	memcpy(vorbiscomment_.data.vorbis_comment.vendor_string.entry, "name0", 5+1);
	vorbiscomment_.data.vorbis_comment.num_comments = 2;
	vorbiscomment_.data.vorbis_comment.comments = (::FLAC__StreamMetadata_VorbisComment_Entry*)malloc_or_die_(vorbiscomment_.data.vorbis_comment.num_comments * sizeof(::FLAC__StreamMetadata_VorbisComment_Entry));
	vorbiscomment_.data.vorbis_comment.comments[0].length = 12;
	vorbiscomment_.data.vorbis_comment.comments[0].entry = (FLAC__byte*)malloc_or_die_(12+1);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[0].entry, "name2=value2", 12+1);
	vorbiscomment_.data.vorbis_comment.comments[1].length = 12;
	vorbiscomment_.data.vorbis_comment.comments[1].entry = (FLAC__byte*)malloc_or_die_(12+1);
	memcpy(vorbiscomment_.data.vorbis_comment.comments[1].entry, "name3=value3", 12+1);

	cuesheet_.is_last = false;
	cuesheet_.type = ::FLAC__METADATA_TYPE_CUESHEET;
	cuesheet_.length =
		/* cuesheet guts */
		(
			FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
			FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
			FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
			FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
			FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
		) / 8 +
		/* 2 tracks */
		2 * (
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
	memset(cuesheet_.data.cue_sheet.media_catalog_number, 0, sizeof(cuesheet_.data.cue_sheet.media_catalog_number));
	cuesheet_.data.cue_sheet.media_catalog_number[0] = 'j';
	cuesheet_.data.cue_sheet.media_catalog_number[1] = 'C';
	cuesheet_.data.cue_sheet.lead_in = 159;
	cuesheet_.data.cue_sheet.is_cd = true;
	cuesheet_.data.cue_sheet.num_tracks = 2;
	cuesheet_.data.cue_sheet.tracks = (FLAC__StreamMetadata_CueSheet_Track*)malloc_or_die_(cuesheet_.data.cue_sheet.num_tracks * sizeof(FLAC__StreamMetadata_CueSheet_Track));
	cuesheet_.data.cue_sheet.tracks[0].offset = 1;
	cuesheet_.data.cue_sheet.tracks[0].number = 1;
	memcpy(cuesheet_.data.cue_sheet.tracks[0].isrc, "ACBDE1234567", sizeof(cuesheet_.data.cue_sheet.tracks[0].isrc));
	cuesheet_.data.cue_sheet.tracks[0].type = 0;
	cuesheet_.data.cue_sheet.tracks[0].pre_emphasis = 1;
	cuesheet_.data.cue_sheet.tracks[0].num_indices = 2;
	cuesheet_.data.cue_sheet.tracks[0].indices = (FLAC__StreamMetadata_CueSheet_Index*)malloc_or_die_(cuesheet_.data.cue_sheet.tracks[0].num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index));
	cuesheet_.data.cue_sheet.tracks[0].indices[0].offset = 0;
	cuesheet_.data.cue_sheet.tracks[0].indices[0].number = 0;
	cuesheet_.data.cue_sheet.tracks[0].indices[1].offset = 1234567890;
	cuesheet_.data.cue_sheet.tracks[0].indices[1].number = 1;
	cuesheet_.data.cue_sheet.tracks[1].offset = 2345678901u;
	cuesheet_.data.cue_sheet.tracks[1].number = 2;
	memcpy(cuesheet_.data.cue_sheet.tracks[1].isrc, "ACBDE7654321", sizeof(cuesheet_.data.cue_sheet.tracks[1].isrc));
	cuesheet_.data.cue_sheet.tracks[1].type = 1;
	cuesheet_.data.cue_sheet.tracks[1].pre_emphasis = 0;
	cuesheet_.data.cue_sheet.tracks[1].num_indices = 1;
	cuesheet_.data.cue_sheet.tracks[1].indices = (FLAC__StreamMetadata_CueSheet_Index*)malloc_or_die_(cuesheet_.data.cue_sheet.tracks[1].num_indices * sizeof(FLAC__StreamMetadata_CueSheet_Index));
	cuesheet_.data.cue_sheet.tracks[1].indices[0].offset = 0;
	cuesheet_.data.cue_sheet.tracks[1].indices[0].number = 1;

	picture_.is_last = true;
	picture_.type = FLAC__METADATA_TYPE_PICTURE;
	picture_.length =
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
	picture_.data.picture.type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;
	picture_.data.picture.mime_type = strdup_or_die_("image/jpeg");
	picture_.length += strlen(picture_.data.picture.mime_type);
	picture_.data.picture.description = (FLAC__byte*)strdup_or_die_("desc");
	picture_.length += strlen((const char *)picture_.data.picture.description);
	picture_.data.picture.width = 300;
	picture_.data.picture.height = 300;
	picture_.data.picture.depth = 24;
	picture_.data.picture.colors = 0;
	picture_.data.picture.data = (FLAC__byte*)strdup_or_die_("SOMEJPEGDATA");
	picture_.data.picture.data_length = strlen((const char *)picture_.data.picture.data);
	picture_.length += picture_.data.picture.data_length;
}

static void free_metadata_blocks_()
{
	free(seektable_.data.seek_table.points);
	free(application_.data.application.data);
	free(vorbiscomment_.data.vorbis_comment.vendor_string.entry);
	free(vorbiscomment_.data.vorbis_comment.comments[0].entry);
	free(vorbiscomment_.data.vorbis_comment.comments[1].entry);
	free(vorbiscomment_.data.vorbis_comment.comments);
	free(cuesheet_.data.cue_sheet.tracks[0].indices);
	free(cuesheet_.data.cue_sheet.tracks[1].indices);
	free(cuesheet_.data.cue_sheet.tracks);
	free(picture_.data.picture.mime_type);
	free(picture_.data.picture.description);
	free(picture_.data.picture.data);
}

bool test_metadata_object_streaminfo()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::StreamInfo\n");

	printf("testing StreamInfo::StreamInfo()... ");
	FLAC::Metadata::StreamInfo block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing StreamInfo::StreamInfo(const StreamInfo &)... +\n");
	printf("        StreamInfo::operator!=(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing StreamInfo::~StreamInfo()... ");
	}
	printf("OK\n");

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata &)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(streaminfo_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata *)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(&streaminfo_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy(&streaminfo_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::StreamInfo(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&streaminfo_);
		FLAC::Metadata::StreamInfo blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy;
		blockcopy.assign(&streaminfo_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        StreamInfo::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&streaminfo_);
		FLAC::Metadata::StreamInfo blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != streaminfo_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const StreamInfo &)... +\n");
	printf("        StreamInfo::operator==(const StreamInfo &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        StreamInfo::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = streaminfo_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == streaminfo_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        StreamInfo::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::StreamInfo blockcopy = &streaminfo_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == streaminfo_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing StreamInfo::set_min_blocksize()... ");
	block.set_min_blocksize(streaminfo_.data.stream_info.min_blocksize);
	printf("OK\n");

	printf("testing StreamInfo::set_max_blocksize()... ");
	block.set_max_blocksize(streaminfo_.data.stream_info.max_blocksize);
	printf("OK\n");

	printf("testing StreamInfo::set_min_framesize()... ");
	block.set_min_framesize(streaminfo_.data.stream_info.min_framesize);
	printf("OK\n");

	printf("testing StreamInfo::set_max_framesize()... ");
	block.set_max_framesize(streaminfo_.data.stream_info.max_framesize);
	printf("OK\n");

	printf("testing StreamInfo::set_sample_rate()... ");
	block.set_sample_rate(streaminfo_.data.stream_info.sample_rate);
	printf("OK\n");

	printf("testing StreamInfo::set_channels()... ");
	block.set_channels(streaminfo_.data.stream_info.channels);
	printf("OK\n");

	printf("testing StreamInfo::set_bits_per_sample()... ");
	block.set_bits_per_sample(streaminfo_.data.stream_info.bits_per_sample);
	printf("OK\n");

	printf("testing StreamInfo::set_total_samples()... ");
	block.set_total_samples(streaminfo_.data.stream_info.total_samples);
	printf("OK\n");

	printf("testing StreamInfo::set_md5sum()... ");
	block.set_md5sum(streaminfo_.data.stream_info.md5sum);
	printf("OK\n");

	printf("testing StreamInfo::get_min_blocksize()... ");
	if(block.get_min_blocksize() != streaminfo_.data.stream_info.min_blocksize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_max_blocksize()... ");
	if(block.get_max_blocksize() != streaminfo_.data.stream_info.max_blocksize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_min_framesize()... ");
	if(block.get_min_framesize() != streaminfo_.data.stream_info.min_framesize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_max_framesize()... ");
	if(block.get_max_framesize() != streaminfo_.data.stream_info.max_framesize)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_sample_rate()... ");
	if(block.get_sample_rate() != streaminfo_.data.stream_info.sample_rate)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_channels()... ");
	if(block.get_channels() != streaminfo_.data.stream_info.channels)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_bits_per_sample()... ");
	if(block.get_bits_per_sample() != streaminfo_.data.stream_info.bits_per_sample)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_total_samples()... ");
	if(block.get_total_samples() != streaminfo_.data.stream_info.total_samples)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing StreamInfo::get_md5sum()... ");
	if(0 != memcmp(block.get_md5sum(), streaminfo_.data.stream_info.md5sum, 16))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::StreamInfo *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::StreamInfo *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing StreamInfo::~StreamInfo()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_padding()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::Padding\n");

	printf("testing Padding::Padding()... ");
	FLAC::Metadata::Padding block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = 0;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing Padding::Padding(const Padding &)... +\n");
	printf("        Padding::operator!=(const Padding &)... ");
	{
		FLAC::Metadata::Padding blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing Padding::~Padding()... ");
	}
	printf("OK\n");

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Padding blockcopy(padding_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy(&padding_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy(&padding_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::Padding(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&padding_);
		FLAC::Metadata::Padding blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy;
		blockcopy.assign(&padding_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Padding::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&padding_);
		FLAC::Metadata::Padding blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != padding_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const Padding &)... +\n");
	printf("        Padding::operator==(const Padding &)... ");
	{
		FLAC::Metadata::Padding blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Padding::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Padding blockcopy = padding_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == padding_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Padding::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Padding blockcopy = &padding_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == padding_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Padding::set_length()... ");
	block.set_length(padding_.length);
	printf("OK\n");

	printf("testing Prototype::get_length()... ");
	if(block.get_length() != padding_.length)
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::Padding *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::Padding *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing Padding::~Padding()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_application()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::Application\n");

	printf("testing Application::Application()... ");
	FLAC::Metadata::Application block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = FLAC__STREAM_METADATA_APPLICATION_ID_LEN / 8;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing Application::Application(const Application &)... +\n");
	printf("        Application::operator!=(const Application &)... ");
	{
		FLAC::Metadata::Application blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing Application::~Application()... ");
	}
	printf("OK\n");

	printf("testing Application::Application(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Application blockcopy(application_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::Application(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy(&application_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::Application(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy(&application_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::Application(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&application_);
		FLAC::Metadata::Application blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy;
		blockcopy.assign(&application_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Application::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&application_);
		FLAC::Metadata::Application blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != application_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const Application &)... +\n");
	printf("        Application::operator==(const Application &)... ");
	{
		FLAC::Metadata::Application blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Application::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Application blockcopy = application_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == application_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Application::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Application blockcopy = &application_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == application_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Application::set_id()... ");
	block.set_id(application_.data.application.id);
	printf("OK\n");

	printf("testing Application::set_data()... ");
	block.set_data(application_.data.application.data, application_.length - sizeof(application_.data.application.id), /*copy=*/true);
	printf("OK\n");

	printf("testing Application::get_id()... ");
	if(0 != memcmp(block.get_id(), application_.data.application.id, sizeof(application_.data.application.id)))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing Application::get_data()... ");
	if(0 != memcmp(block.get_data(), application_.data.application.data, application_.length - sizeof(application_.data.application.id)))
		return die_("value mismatch, doesn't match previously set value");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::Application *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::Application *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing Application::~Application()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_seektable()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::SeekTable\n");

	printf("testing SeekTable::SeekTable()... ");
	FLAC::Metadata::SeekTable block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = 0;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing SeekTable::SeekTable(const SeekTable &)... +\n");
	printf("        SeekTable::operator!=(const SeekTable &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing SeekTable::~SeekTable()... ");
	}
	printf("OK\n");

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata &)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(seektable_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata *)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(&seektable_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy(&seektable_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::SeekTable(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&seektable_);
		FLAC::Metadata::SeekTable blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy;
		blockcopy.assign(&seektable_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        SeekTable::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&seektable_);
		FLAC::Metadata::SeekTable blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != seektable_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const SeekTable &)... +\n");
	printf("        SeekTable::operator==(const SeekTable &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        SeekTable::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = seektable_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == seektable_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        SeekTable::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::SeekTable blockcopy = &seektable_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == seektable_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing SeekTable::insert_point() x 3... ");
	if(!block.insert_point(0, seektable_.data.seek_table.points[1]))
		return die_("returned false");
	if(!block.insert_point(0, seektable_.data.seek_table.points[1]))
		return die_("returned false");
	if(!block.insert_point(1, seektable_.data.seek_table.points[0]))
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::is_legal()... ");
	if(block.is_legal())
		return die_("returned true");
	printf("OK\n");

	printf("testing SeekTable::set_point()... ");
	block.set_point(0, seektable_.data.seek_table.points[0]);
	printf("OK\n");

	printf("testing SeekTable::delete_point()... ");
	if(!block.delete_point(0))
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::is_legal()... ");
	if(!block.is_legal())
		return die_("returned false");
	printf("OK\n");

	printf("testing SeekTable::get_num_points()... ");
	if(block.get_num_points() != seektable_.data.seek_table.num_points)
		return die_("number mismatch");
	printf("OK\n");

	printf("testing SeekTable::operator!=(const ::FLAC__StreamMetadata &)... ");
	if(block != seektable_)
		return die_("data mismatch");
	printf("OK\n");

	printf("testing SeekTable::get_point()... ");
	if(
		block.get_point(1).sample_number != seektable_.data.seek_table.points[1].sample_number ||
		block.get_point(1).stream_offset != seektable_.data.seek_table.points[1].stream_offset ||
		block.get_point(1).frame_samples != seektable_.data.seek_table.points[1].frame_samples
	)
		return die_("point mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::SeekTable *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::SeekTable *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing SeekTable::~SeekTable()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_vorbiscomment()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::VorbisComment::Entry\n");

	printf("testing Entry::Entry()... ");
	{
		FLAC::Metadata::VorbisComment::Entry entry1;
		if(!entry1.is_valid())
			return die_("!is_valid()");
		printf("OK\n");

		printf("testing Entry::~Entry()... ");
	}
	printf("OK\n");

	printf("testing Entry::Entry(const char *field, unsigned field_length)... ");
	FLAC::Metadata::VorbisComment::Entry entry2("name2=value2", strlen("name2=value2"));
	if(!entry2.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	{
		printf("testing Entry::Entry(const char *field)... ");
		FLAC::Metadata::VorbisComment::Entry entry2z("name2=value2");
		if(!entry2z.is_valid())
			return die_("!is_valid()");
		if(strcmp(entry2.get_field(), entry2z.get_field()))
			return die_("bad value");
		printf("OK\n");
	}

	printf("testing Entry::Entry(const char *field_name, const char *field_value, unsigned field_value_length)... ");
	FLAC::Metadata::VorbisComment::Entry entry3("name3", "value3", strlen("value3"));
	if(!entry3.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	{
		printf("testing Entry::Entry(const char *field_name, const char *field_value)... ");
		FLAC::Metadata::VorbisComment::Entry entry3z("name3", "value3");
		if(!entry3z.is_valid())
			return die_("!is_valid()");
		if(strcmp(entry3.get_field(), entry3z.get_field()))
			return die_("bad value");
		printf("OK\n");
	}

	printf("testing Entry::Entry(const Entry &entry)... ");
	{
		FLAC::Metadata::VorbisComment::Entry entry2copy(entry2);
		if(!entry2copy.is_valid())
			return die_("!is_valid()");
		printf("OK\n");

		printf("testing Entry::~Entry()... ");
	}
	printf("OK\n");

	printf("testing Entry::operator=(const Entry &entry)... ");
	FLAC::Metadata::VorbisComment::Entry entry1 = entry2;
	if(!entry2.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	printf("testing Entry::get_field_length()... ");
	if(entry1.get_field_length() != strlen("name2=value2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_name_length()... ");
	if(entry1.get_field_name_length() != strlen("name2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_value_length()... ");
	if(entry1.get_field_value_length() != strlen("value2"))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_entry()... ");
	{
		::FLAC__StreamMetadata_VorbisComment_Entry entry = entry1.get_entry();
		if(entry.length != strlen("name2=value2"))
			return die_("entry length mismatch");
		if(0 != memcmp(entry.entry, "name2=value2", entry.length))
			return die_("entry value mismatch");
	}
	printf("OK\n");

	printf("testing Entry::get_field()... ");
	if(0 != memcmp(entry1.get_field(), "name2=value2", strlen("name2=value2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_name()... ");
	if(0 != memcmp(entry1.get_field_name(), "name2", strlen("name2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::get_field_value()... ");
	if(0 != memcmp(entry1.get_field_value(), "value2", strlen("value2")))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Entry::set_field_name()... ");
	if(!entry1.set_field_name("name1"))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_name(), "name1", strlen("name1")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name1=value2", strlen("name1=value2")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field_value(const char *field_value, unsigned field_value_length)... ");
	if(!entry1.set_field_value("value1", strlen("value1")))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_value(), "value1", strlen("value1")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name1=value1", strlen("name1=value1")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field_value(const char *field_value)... ");
	if(!entry1.set_field_value("value1"))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_value(), "value1", strlen("value1")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name1=value1", strlen("name1=value1")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field(const char *field, unsigned field_length)... ");
	if(!entry1.set_field("name0=value0", strlen("name0=value0")))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_name(), "name0", strlen("name0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field_value(), "value0", strlen("value0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name0=value0", strlen("name0=value0")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("testing Entry::set_field(const char *field)... ");
	if(!entry1.set_field("name0=value0"))
		return die_("returned false");
	if(0 != memcmp(entry1.get_field_name(), "name0", strlen("name0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field_value(), "value0", strlen("value0")))
		return die_("value mismatch");
	if(0 != memcmp(entry1.get_field(), "name0=value0", strlen("name0=value0")))
		return die_("entry mismatch");
	printf("OK\n");

	printf("PASSED\n\n");


	printf("testing class FLAC::Metadata::VorbisComment\n");

	printf("testing VorbisComment::VorbisComment()... ");
	FLAC::Metadata::VorbisComment block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = (FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN/8 + strlen(::FLAC__VENDOR_STRING) + FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN/8);
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing VorbisComment::VorbisComment(const VorbisComment &)... +\n");
	printf("        VorbisComment::operator!=(const VorbisComment &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing VorbisComment::~VorbisComment()... ");
	}
	printf("OK\n");

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata &)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(vorbiscomment_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata *)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(&vorbiscomment_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy(&vorbiscomment_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::VorbisComment(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&vorbiscomment_);
		FLAC::Metadata::VorbisComment blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy;
		blockcopy.assign(&vorbiscomment_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        VorbisComment::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&vorbiscomment_);
		FLAC::Metadata::VorbisComment blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != vorbiscomment_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const VorbisComment &)... +\n");
	printf("        VorbisComment::operator==(const VorbisComment &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        VorbisComment::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = vorbiscomment_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == vorbiscomment_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        VorbisComment::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::VorbisComment blockcopy = &vorbiscomment_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == vorbiscomment_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing VorbisComment::get_num_comments()... ");
	if(block.get_num_comments() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing VorbisComment::set_vendor_string()... ");
	if(!block.set_vendor_string((const FLAC__byte *)"mame0"))
		return die_("returned false");
	printf("OK\n");
	vorbiscomment_.data.vorbis_comment.vendor_string.entry[0] = 'm';

	printf("testing VorbisComment::get_vendor_string()... ");
	if(strlen((const char *)block.get_vendor_string()) != vorbiscomment_.data.vorbis_comment.vendor_string.length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_vendor_string(), vorbiscomment_.data.vorbis_comment.vendor_string.entry, vorbiscomment_.data.vorbis_comment.vendor_string.length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::append_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.append_comment(entry3))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::append_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.append_comment(entry2))
		return die_("returned false");
	if(block.get_comment(1).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(1).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::delete_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.delete_comment(0))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length[0] mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value[0] mismatch");
	printf("OK\n");

	printf("testing VorbisComment::delete_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.delete_comment(0))
		return die_("returned false");
	if(block.get_num_comments() != 0)
		return die_("block mismatch, expected num_comments = 0");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(0, entry3))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(0, entry3))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::insert_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.insert_comment(1, entry2))
		return die_("returned false");
	if(block.get_comment(1).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(1).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::set_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.set_comment(0, entry2))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing VorbisComment::delete_comment()... +\n");
	printf("        VorbisComment::get_comment()... ");
	if(!block.delete_comment(0))
		return die_("returned false");
	if(block.get_comment(0).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[0].length)
		return die_("length[0] mismatch");
	if(0 != memcmp(block.get_comment(0).get_field(), vorbiscomment_.data.vorbis_comment.comments[0].entry, vorbiscomment_.data.vorbis_comment.comments[0].length))
		return die_("value[0] mismatch");
	if(block.get_comment(1).get_field_length() != vorbiscomment_.data.vorbis_comment.comments[1].length)
		return die_("length[1] mismatch");
	if(0 != memcmp(block.get_comment(1).get_field(), vorbiscomment_.data.vorbis_comment.comments[1].entry, vorbiscomment_.data.vorbis_comment.comments[1].length))
		return die_("value[0] mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::VorbisComment *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::VorbisComment *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing VorbisComment::~VorbisComment()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_cuesheet()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::CueSheet::Track\n");

	printf("testing Track::Track()... ");
	FLAC::Metadata::CueSheet::Track track0;
	if(!track0.is_valid())
		return die_("!is_valid()");
	printf("OK\n");

	{
		printf("testing Track::get_track()... ");
		const ::FLAC__StreamMetadata_CueSheet_Track *trackp = track0.get_track();
		if(0 == trackp)
			return die_("returned pointer is NULL");
		printf("OK\n");

		printf("testing Track::Track(const ::FLAC__StreamMetadata_CueSheet_Track*)... ");
		FLAC::Metadata::CueSheet::Track track2(trackp);
		if(!track2.is_valid())
			return die_("!is_valid()");
		if(!track_is_equal_(track2.get_track(), trackp))
			return die_("copy is not equal");
		printf("OK\n");

		printf("testing Track::~Track()... ");
	}
	printf("OK\n");

	printf("testing Track::Track(const Track &track)... ");
	{
		FLAC::Metadata::CueSheet::Track track0copy(track0);
		if(!track0copy.is_valid())
			return die_("!is_valid()");
		if(!track_is_equal_(track0copy.get_track(), track0.get_track()))
			return die_("copy is not equal");
		printf("OK\n");

		printf("testing Track::~Track()... ");
	}
	printf("OK\n");

	printf("testing Track::operator=(const Track &track)... ");
	FLAC::Metadata::CueSheet::Track track1 = track0;
	if(!track0.is_valid())
		return die_("!is_valid()");
	if(!track_is_equal_(track1.get_track(), track0.get_track()))
		return die_("copy is not equal");
	printf("OK\n");

	printf("testing Track::get_offset()... ");
	if(track1.get_offset() != 0)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::get_number()... ");
	if(track1.get_number() != 0)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::get_isrc()... ");
	if(0 != memcmp(track1.get_isrc(), "\0\0\0\0\0\0\0\0\0\0\0\0\0", 13))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::get_type()... ");
	if(track1.get_type() != 0)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::get_pre_emphasis()... ");
	if(track1.get_pre_emphasis() != 0)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::get_num_indices()... ");
	if(track1.get_num_indices() != 0)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::set_offset()... ");
	track1.set_offset(588);
	if(track1.get_offset() != 588)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::set_number()... ");
	track1.set_number(1);
	if(track1.get_number() != 1)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::set_isrc()... ");
	track1.set_isrc("ABCDE1234567");
	if(0 != memcmp(track1.get_isrc(), "ABCDE1234567", 13))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::set_type()... ");
	track1.set_type(1);
	if(track1.get_type() != 1)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Track::set_pre_emphasis()... ");
	track1.set_pre_emphasis(1);
	if(track1.get_pre_emphasis() != 1)
		return die_("value mismatch");
	printf("OK\n");

	printf("PASSED\n\n");

	printf("testing class FLAC::Metadata::CueSheet\n");

	printf("testing CueSheet::CueSheet()... ");
	FLAC::Metadata::CueSheet block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
	expected_length = (
		FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN +
		FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN +
		FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN +
		FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN +
		FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
	) / 8;
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing CueSheet::CueSheet(const CueSheet &)... +\n");
	printf("        CueSheet::operator!=(const CueSheet &)... ");
	{
		FLAC::Metadata::CueSheet blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing CueSheet::~CueSheet()... ");
	}
	printf("OK\n");

	printf("testing CueSheet::CueSheet(const ::FLAC__StreamMetadata &)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::CueSheet blockcopy(cuesheet_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::CueSheet(const ::FLAC__StreamMetadata *)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::CueSheet blockcopy(&cuesheet_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::CueSheet(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::CueSheet blockcopy(&cuesheet_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::CueSheet(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&cuesheet_);
		FLAC::Metadata::CueSheet blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::CueSheet blockcopy;
		blockcopy.assign(&cuesheet_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        CueSheet::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&cuesheet_);
		FLAC::Metadata::CueSheet blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != cuesheet_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::operator=(const CueSheet &)... +\n");
	printf("        CueSheet::operator==(const CueSheet &)... ");
	{
		FLAC::Metadata::CueSheet blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        CueSheet::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::CueSheet blockcopy = cuesheet_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == cuesheet_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        CueSheet::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::CueSheet blockcopy = &cuesheet_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == cuesheet_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing CueSheet::get_media_catalog_number()... ");
	if(0 != strcmp(block.get_media_catalog_number(), ""))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::get_lead_in()... ");
	if(block.get_lead_in() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing CueSheet::get_is_cd()... ");
	if(block.get_is_cd())
		return die_("value mismatch, expected false");
	printf("OK\n");

	printf("testing CueSheet::get_num_tracks()... ");
	if(block.get_num_tracks() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing CueSheet::set_media_catalog_number()... ");
	{
		char mcn[129];
		memset(mcn, 0, sizeof(mcn));
		strcpy(mcn, "1234567890123");
		block.set_media_catalog_number(mcn);
		if(0 != memcmp(block.get_media_catalog_number(), mcn, sizeof(mcn)))
			return die_("value mismatch");
	}
	printf("OK\n");

	printf("testing CueSheet::set_lead_in()... ");
	block.set_lead_in(588);
	if(block.get_lead_in() != 588)
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::set_is_cd()... ");
	block.set_is_cd(true);
	if(!block.get_is_cd())
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::insert_track()... +\n");
	printf("        CueSheet::get_track()... ");
	if(!block.insert_track(0, track0))
		return die_("returned false");
	if(!track_is_equal_(block.get_track(0).get_track(), track0.get_track()))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::insert_track()... +\n");
	printf("        CueSheet::get_track()... ");
	if(!block.insert_track(1, track1))
		return die_("returned false");
	if(!track_is_equal_(block.get_track(1).get_track(), track1.get_track()))
		return die_("value mismatch");
	printf("OK\n");

	::FLAC__StreamMetadata_CueSheet_Index index0;
	index0.offset = 588*4;
	index0.number = 1;

	printf("testing CueSheet::insert_index(0)... +\n");
	printf("        CueSheet::get_track()... +\n");
	printf("        CueSheet::Track::get_index()... ");
	if(!block.insert_index(0, 0, index0))
		return die_("returned false");
	if(!index_is_equal_(block.get_track(0).get_index(0), index0))
		return die_("value mismatch");
	printf("OK\n");

	index0.offset = 588*5;
	printf("testing CueSheet::Track::set_index()... ");
	{
		FLAC::Metadata::CueSheet::Track track_ = block.get_track(0);
		track_.set_index(0, index0);
		if(!index_is_equal_(track_.get_index(0), index0))
			return die_("value mismatch");
	}
	printf("OK\n");

	index0.offset = 588*6;
	printf("testing CueSheet::set_index()... ");
	block.set_index(0, 0, index0);
	if(!index_is_equal_(block.get_track(0).get_index(0), index0))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::delete_index()... ");
	if(!block.delete_index(0, 0))
		return die_("returned false");
	if(block.get_track(0).get_num_indices() != 0)
		return die_("num_indices mismatch");
	printf("OK\n");


	printf("testing CueSheet::set_track()... +\n");
	printf("        CueSheet::get_track()... ");
	if(!block.set_track(0, track1))
		return die_("returned false");
	if(!track_is_equal_(block.get_track(0).get_track(), track1.get_track()))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing CueSheet::delete_track()... ");
	if(!block.delete_track(0))
		return die_("returned false");
	if(block.get_num_tracks() != 1)
		return die_("num_tracks mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::CueSheet *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::CueSheet *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing CueSheet::~CueSheet()... ");
	delete clone_;
	printf("OK\n");

	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object_picture()
{
	unsigned expected_length;

	printf("testing class FLAC::Metadata::Picture\n");

	printf("testing Picture::Picture()... ");
	FLAC::Metadata::Picture block;
	if(!block.is_valid())
		return die_("!block.is_valid()");
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
	if(block.get_length() != expected_length) {
		printf("FAILED, bad length, expected %u, got %u\n", expected_length, block.get_length());
		return false;
	}
	printf("OK\n");

	printf("testing Picture::Picture(const Picture &)... +\n");
	printf("        Picture::operator!=(const Picture &)... ");
	{
		FLAC::Metadata::Picture blockcopy(block);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != block)
			return die_("copy is not identical to original");
		printf("OK\n");

		printf("testing Picture::~Picture()... ");
	}
	printf("OK\n");

	printf("testing Picture::Picture(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Picture blockcopy(picture_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::Picture(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Picture blockcopy(&picture_);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::Picture(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Picture blockcopy(&picture_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::Picture(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&picture_);
		FLAC::Metadata::Picture blockcopy(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::assign(const ::FLAC__StreamMetadata *, copy=true)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Picture blockcopy;
		blockcopy.assign(&picture_, /*copy=*/true);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::assign(const ::FLAC__StreamMetadata *, copy=false)... +\n");
	printf("        Picture::operator!=(const ::FLAC__StreamMetadata *)... ");
	{
		::FLAC__StreamMetadata *copy = ::FLAC__metadata_object_clone(&picture_);
		FLAC::Metadata::Picture blockcopy;
		blockcopy.assign(copy, /*copy=*/false);
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(blockcopy != picture_)
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::operator=(const Picture &)... +\n");
	printf("        Picture::operator==(const Picture &)... ");
	{
		FLAC::Metadata::Picture blockcopy = block;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == block))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::operator=(const ::FLAC__StreamMetadata &)... +\n");
	printf("        Picture::operator==(const ::FLAC__StreamMetadata &)... ");
	{
		FLAC::Metadata::Picture blockcopy = picture_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == picture_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::operator=(const ::FLAC__StreamMetadata *)... +\n");
	printf("        Picture::operator==(const ::FLAC__StreamMetadata *)... ");
	{
		FLAC::Metadata::Picture blockcopy = &picture_;
		if(!blockcopy.is_valid())
			return die_("!blockcopy.is_valid()");
		if(!(blockcopy == picture_))
			return die_("copy is not identical to original");
		printf("OK\n");
	}

	printf("testing Picture::get_type()... ");
	if(block.get_type() != ::FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER)
		return die_("value mismatch, expected ::FLAC__STREAM_METADATA_PICTURE_TYPE_OTHER");
	printf("OK\n");

	printf("testing Picture::set_type()... +\n");
	printf("        Picture::get_type()... ");
	block.set_type(::FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA);
	if(block.get_type() != ::FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA)
		return die_("value mismatch, expected ::FLAC__STREAM_METADATA_PICTURE_TYPE_MEDIA");
	printf("OK\n");

	printf("testing Picture::set_mime_type()... ");
	if(!block.set_mime_type("qmage/jpeg"))
		return die_("returned false");
	printf("OK\n");
	picture_.data.picture.mime_type[0] = 'q';

	printf("testing Picture::get_mime_type()... ");
	if(0 != strcmp(block.get_mime_type(), picture_.data.picture.mime_type))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Picture::set_description()... ");
	if(!block.set_description((const FLAC__byte*)"qesc"))
		return die_("returned false");
	printf("OK\n");
	picture_.data.picture.description[0] = 'q';

	printf("testing Picture::get_description()... ");
	if(0 != strcmp((const char *)block.get_description(), (const char *)picture_.data.picture.description))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing Picture::get_width()... ");
	if(block.get_width() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing Picture::set_width()... +\n");
	printf("        Picture::get_width()... ");
	block.set_width(400);
	if(block.get_width() != 400)
		return die_("value mismatch, expected 400");
	printf("OK\n");

	printf("testing Picture::get_height()... ");
	if(block.get_height() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing Picture::set_height()... +\n");
	printf("        Picture::get_height()... ");
	block.set_height(200);
	if(block.get_height() != 200)
		return die_("value mismatch, expected 200");
	printf("OK\n");

	printf("testing Picture::get_depth()... ");
	if(block.get_depth() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing Picture::set_depth()... +\n");
	printf("        Picture::get_depth()... ");
	block.set_depth(16);
	if(block.get_depth() != 16)
		return die_("value mismatch, expected 16");
	printf("OK\n");

	printf("testing Picture::get_colors()... ");
	if(block.get_colors() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing Picture::set_colors()... +\n");
	printf("        Picture::get_colors()... ");
	block.set_colors(1u>16);
	if(block.get_colors() != 1u>16)
		return die_("value mismatch, expected 2^16");
	printf("OK\n");

	printf("testing Picture::get_data_length()... ");
	if(block.get_data_length() != 0)
		return die_("value mismatch, expected 0");
	printf("OK\n");

	printf("testing Picture::set_data()... ");
	if(!block.set_data((const FLAC__byte*)"qOMEJPEGDATA", strlen("qOMEJPEGDATA")))
		return die_("returned false");
	printf("OK\n");
	picture_.data.picture.data[0] = 'q';

	printf("testing Picture::get_data()... ");
	if(block.get_data_length() != picture_.data.picture.data_length)
		return die_("length mismatch");
	if(0 != memcmp(block.get_data(), picture_.data.picture.data, picture_.data.picture.data_length))
		return die_("value mismatch");
	printf("OK\n");

	printf("testing FLAC::Metadata::clone(const FLAC::Metadata::Prototype *)... ");
	FLAC::Metadata::Prototype *clone_ = FLAC::Metadata::clone(&block);
	if(0 == clone_)
		return die_("returned NULL");
	if(0 == dynamic_cast<FLAC::Metadata::Picture *>(clone_))
		return die_("downcast is NULL");
	if(*dynamic_cast<FLAC::Metadata::Picture *>(clone_) != block)
		return die_("clone is not identical");
	printf("OK\n");
	printf("testing Picture::~Picture()... ");
	delete clone_;
	printf("OK\n");


	printf("PASSED\n\n");
	return true;
}

bool test_metadata_object()
{
	printf("\n+++ libFLAC++ unit test: metadata objects\n\n");

	init_metadata_blocks_();

	if(!test_metadata_object_streaminfo())
		return false;

	if(!test_metadata_object_padding())
		return false;

	if(!test_metadata_object_application())
		return false;

	if(!test_metadata_object_seektable())
		return false;

	if(!test_metadata_object_vorbiscomment())
		return false;

	if(!test_metadata_object_cuesheet())
		return false;

	if(!test_metadata_object_picture())
		return false;

	free_metadata_blocks_();

	return true;
}
