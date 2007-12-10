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

#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcpy()/memset() */
#if defined _MSC_VER || defined __MINGW32__
#include <sys/utime.h> /* for utime() */
#include <io.h> /* for chmod() */
#if _MSC_VER <= 1600 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#else
#include <sys/types.h> /* some flavors of BSD (like OS X) require this to get time_t */
#include <utime.h> /* for utime() */
#include <unistd.h> /* for chown(), unlink() */
#endif
#include <sys/stat.h> /* for stat(), maybe chmod() */
#include "FLAC/assert.h"
#include "FLAC/stream_decoder.h"
#include "FLAC/metadata.h"
#include "share/grabbag.h"
#include "test_libs_common/file_utils_flac.h"
#include "test_libs_common/metadata_utils.h"
#include "metadata.h"


/******************************************************************************
	The general strategy of these tests (for interface levels 1 and 2) is
	to create a dummy FLAC file with a known set of initial metadata
	blocks, then keep a mirror locally of what we expect the metadata to be
	after each operation.  Then testing becomes a simple matter of running
	a FLAC__StreamDecoder over the dummy file after each operation, comparing
	the decoded metadata to what's in our local copy.  If there are any
	differences in the metadata, or the actual audio data is corrupted, we
	will catch it while decoding.
******************************************************************************/

typedef struct {
	FLAC__bool error_occurred;
} decoder_client_struct;

typedef struct {
	FLAC__StreamMetadata *blocks[64];
	unsigned num_blocks;
} our_metadata_struct;

/* our copy of the metadata in flacfilename() */
static our_metadata_struct our_metadata_;

/* the current block number that corresponds to the position of the iterator we are testing */
static unsigned mc_our_block_number_ = 0;

static const char *flacfilename(FLAC__bool is_ogg)
{
	return is_ogg? "metadata.oga" : "metadata.flac";
}

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_c_(const char *msg, FLAC__Metadata_ChainStatus status)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%s\n", FLAC__Metadata_ChainStatusString[status]);
	return false;
}

static FLAC__bool die_ss_(const char *msg, FLAC__Metadata_SimpleIterator *iterator)
{
	printf("ERROR: %s\n", msg);
	printf("       status=%s\n", FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iterator)]);
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

/* functions for working with our metadata copy */

static FLAC__bool replace_in_our_metadata_(FLAC__StreamMetadata *block, unsigned position, FLAC__bool copy)
{
	unsigned i;
	FLAC__StreamMetadata *obj = block;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	if(copy) {
		if(0 == (obj = FLAC__metadata_object_clone(block)))
			return die_("during FLAC__metadata_object_clone()");
	}
	FLAC__metadata_object_delete(our_metadata_.blocks[position]);
	our_metadata_.blocks[position] = obj;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->is_last = false;
	our_metadata_.blocks[i]->is_last = true;

	return true;
}

static FLAC__bool insert_to_our_metadata_(FLAC__StreamMetadata *block, unsigned position, FLAC__bool copy)
{
	unsigned i;
	FLAC__StreamMetadata *obj = block;
	if(copy) {
		if(0 == (obj = FLAC__metadata_object_clone(block)))
			return die_("during FLAC__metadata_object_clone()");
	}
	if(position > our_metadata_.num_blocks) {
		position = our_metadata_.num_blocks;
	}
	else {
		for(i = our_metadata_.num_blocks; i > position; i--)
			our_metadata_.blocks[i] = our_metadata_.blocks[i-1];
	}
	our_metadata_.blocks[position] = obj;
	our_metadata_.num_blocks++;

	/* set the is_last flags */
	for(i = 0; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i]->is_last = false;
	our_metadata_.blocks[i]->is_last = true;

	return true;
}

static void delete_from_our_metadata_(unsigned position)
{
	unsigned i;
	FLAC__ASSERT(position < our_metadata_.num_blocks);
	FLAC__metadata_object_delete(our_metadata_.blocks[position]);
	for(i = position; i < our_metadata_.num_blocks - 1; i++)
		our_metadata_.blocks[i] = our_metadata_.blocks[i+1];
	our_metadata_.num_blocks--;

	/* set the is_last flags */
	if(our_metadata_.num_blocks > 0) {
		for(i = 0; i < our_metadata_.num_blocks - 1; i++)
			our_metadata_.blocks[i]->is_last = false;
		our_metadata_.blocks[i]->is_last = true;
	}
}

/*
 * This wad of functions supports filename- and callback-based chain reading/writing.
 * Everything up to set_file_stats_() is copied from libFLAC/metadata_iterators.c
 */
static FLAC__bool open_tempfile_(const char *filename, FILE **tempfile, char **tempfilename)
{
	static const char *tempfile_suffix = ".metadata_edit";

	if(0 == (*tempfilename = (char*)malloc(strlen(filename) + strlen(tempfile_suffix) + 1)))
		return false;
	strcpy(*tempfilename, filename);
	strcat(*tempfilename, tempfile_suffix);

	if(0 == (*tempfile = fopen(*tempfilename, "wb")))
		return false;

	return true;
}

static void cleanup_tempfile_(FILE **tempfile, char **tempfilename)
{
	if(0 != *tempfile) {
		(void)fclose(*tempfile);
		*tempfile = 0;
	}

	if(0 != *tempfilename) {
		(void)unlink(*tempfilename);
		free(*tempfilename);
		*tempfilename = 0;
	}
}

static FLAC__bool transport_tempfile_(const char *filename, FILE **tempfile, char **tempfilename)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != tempfile);
	FLAC__ASSERT(0 != tempfilename);
	FLAC__ASSERT(0 != *tempfilename);

	if(0 != *tempfile) {
		(void)fclose(*tempfile);
		*tempfile = 0;
	}

#if defined _MSC_VER || defined __MINGW32__ || defined __EMX__
	/* on some flavors of windows, rename() will fail if the destination already exists */
	if(unlink(filename) < 0) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}
#endif

	if(0 != rename(*tempfilename, filename)) {
		cleanup_tempfile_(tempfile, tempfilename);
		return false;
	}

	cleanup_tempfile_(tempfile, tempfilename);

	return true;
}

static FLAC__bool get_file_stats_(const char *filename, struct stat *stats)
{
	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);
	return (0 == stat(filename, stats));
}

static void set_file_stats_(const char *filename, struct stat *stats)
{
	struct utimbuf srctime;

	FLAC__ASSERT(0 != filename);
	FLAC__ASSERT(0 != stats);

	srctime.actime = stats->st_atime;
	srctime.modtime = stats->st_mtime;
	(void)chmod(filename, stats->st_mode);
	(void)utime(filename, &srctime);
#if !defined _MSC_VER && !defined __MINGW32__ && !defined __EMX__
	(void)chown(filename, stats->st_uid, -1);
	(void)chown(filename, -1, stats->st_gid);
#endif
}

#ifdef FLAC__VALGRIND_TESTING
static size_t chain_write_cb_(const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle)
{
	FILE *stream = (FILE*)handle;
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if(!ferror(stream))
		fflush(stream);
	return ret;
}
#endif

static int chain_seek_cb_(FLAC__IOHandle handle, FLAC__int64 offset, int whence)
{
	off_t o = (off_t)offset;
	FLAC__ASSERT(offset == o);
	return fseeko((FILE*)handle, o, whence);
}

static FLAC__int64 chain_tell_cb_(FLAC__IOHandle handle)
{
	return ftello((FILE*)handle);
}

static int chain_eof_cb_(FLAC__IOHandle handle)
{
	return feof((FILE*)handle);
}

static FLAC__bool write_chain_(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats, FLAC__bool filename_based, const char *filename)
{
	if(filename_based)
		return FLAC__metadata_chain_write(chain, use_padding, preserve_file_stats);
	else {
		FLAC__IOCallbacks callbacks;

		memset(&callbacks, 0, sizeof(callbacks));
		callbacks.read = (FLAC__IOCallback_Read)fread;
#ifdef FLAC__VALGRIND_TESTING
		callbacks.write = chain_write_cb_;
#else
		callbacks.write = (FLAC__IOCallback_Write)fwrite;
#endif
		callbacks.seek = chain_seek_cb_;
		callbacks.eof = chain_eof_cb_;

		if(FLAC__metadata_chain_check_if_tempfile_needed(chain, use_padding)) {
			struct stat stats;
			FILE *file, *tempfile = 0;
			char *tempfilename;
			if(preserve_file_stats) {
				if(!get_file_stats_(filename, &stats))
					return false;
			}
			if(0 == (file = fopen(filename, "rb")))
				return false; /*@@@@ chain status still says OK though */
			if(!open_tempfile_(filename, &tempfile, &tempfilename)) {
				fclose(file);
				cleanup_tempfile_(&tempfile, &tempfilename);
				return false; /*@@@@ chain status still says OK though */
			}
			if(!FLAC__metadata_chain_write_with_callbacks_and_tempfile(chain, use_padding, (FLAC__IOHandle)file, callbacks, (FLAC__IOHandle)tempfile, callbacks)) {
				fclose(file);
				fclose(tempfile);
				return false;
			}
			fclose(file);
			fclose(tempfile);
			file = tempfile = 0;
			if(!transport_tempfile_(filename, &tempfile, &tempfilename))
				return false;
			if(preserve_file_stats)
				set_file_stats_(filename, &stats);
		}
		else {
			FILE *file = fopen(filename, "r+b");
			if(0 == file)
				return false; /*@@@@ chain status still says OK though */
			if(!FLAC__metadata_chain_write_with_callbacks(chain, use_padding, (FLAC__IOHandle)file, callbacks))
				return false;
			fclose(file);
		}
	}

	return true;
}

static FLAC__bool read_chain_(FLAC__Metadata_Chain *chain, const char *filename, FLAC__bool filename_based, FLAC__bool is_ogg)
{
	if(filename_based)
		return is_ogg?
			FLAC__metadata_chain_read_ogg(chain, flacfilename(is_ogg)) :
			FLAC__metadata_chain_read(chain, flacfilename(is_ogg))
		;
	else {
		FLAC__IOCallbacks callbacks;

		memset(&callbacks, 0, sizeof(callbacks));
		callbacks.read = (FLAC__IOCallback_Read)fread;
		callbacks.seek = chain_seek_cb_;
		callbacks.tell = chain_tell_cb_;

		{
			FLAC__bool ret;
			FILE *file = fopen(filename, "rb");
			if(0 == file)
				return false; /*@@@@ chain status still says OK though */
			ret = is_ogg?
				FLAC__metadata_chain_read_ogg_with_callbacks(chain, (FLAC__IOHandle)file, callbacks) :
				FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, callbacks)
			;
			fclose(file);
			return ret;
		}
	}
}

/* function for comparing our metadata to a FLAC__Metadata_Chain */

static FLAC__bool compare_chain_(FLAC__Metadata_Chain *chain, unsigned current_position, FLAC__StreamMetadata *current_block)
{
	unsigned i;
	FLAC__Metadata_Iterator *iterator;
	FLAC__StreamMetadata *block;
	FLAC__bool next_ok = true;

	FLAC__ASSERT(0 != chain);

	printf("\tcomparing chain... ");
	fflush(stdout);

	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	i = 0;
	do {
		printf("%u... ", i);
		fflush(stdout);

		if(0 == (block = FLAC__metadata_iterator_get_block(iterator))) {
			FLAC__metadata_iterator_delete(iterator);
			return die_("getting block from iterator");
		}

		if(!mutils__compare_block(our_metadata_.blocks[i], block)) {
			FLAC__metadata_iterator_delete(iterator);
			return die_("metadata block mismatch");
		}

		i++;
		next_ok = FLAC__metadata_iterator_next(iterator);
	} while(i < our_metadata_.num_blocks && next_ok);

	FLAC__metadata_iterator_delete(iterator);

	if(next_ok)
		return die_("chain has more blocks than expected");

	if(i < our_metadata_.num_blocks)
		return die_("short block count in chain");

	if(0 != current_block) {
		printf("CURRENT_POSITION... ");
		fflush(stdout);

		if(!mutils__compare_block(our_metadata_.blocks[current_position], current_block))
			return die_("metadata block mismatch");
	}

	printf("PASSED\n");

	return true;
}

/* decoder callbacks for checking the file */

static FLAC__StreamDecoderWriteStatus decoder_write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	(void)decoder, (void)buffer, (void)client_data;

	if(
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
		(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
	) {
		printf("content... ");
		fflush(stdout);
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

/* this version pays no attention to the metadata */
static void decoder_metadata_callback_null_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)decoder, (void)metadata, (void)client_data;

	printf("%d... ", mc_our_block_number_);
	fflush(stdout);

	mc_our_block_number_++;
}

/* this version is used when we want to compare to our metadata copy */
static void decoder_metadata_callback_compare_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;

	(void)decoder;

	/* don't bother checking if we've already hit an error */
	if(dcd->error_occurred)
		return;

	printf("%d... ", mc_our_block_number_);
	fflush(stdout);

	if(mc_our_block_number_ >= our_metadata_.num_blocks) {
		(void)die_("got more metadata blocks than expected");
		dcd->error_occurred = true;
	}
	else {
		if(!mutils__compare_block(our_metadata_.blocks[mc_our_block_number_], metadata)) {
			(void)die_("metadata block mismatch");
			dcd->error_occurred = true;
		}
	}
	mc_our_block_number_++;
}

static void decoder_error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	decoder_client_struct *dcd = (decoder_client_struct*)client_data;
	(void)decoder;

	dcd->error_occurred = true;
	printf("ERROR: got error callback, status = %s (%u)\n", FLAC__StreamDecoderErrorStatusString[status], (unsigned)status);
}

static FLAC__bool generate_file_(FLAC__bool include_extras, FLAC__bool is_ogg)
{
	FLAC__StreamMetadata streaminfo, vorbiscomment, *cuesheet, picture, padding;
	FLAC__StreamMetadata *metadata[4];
	unsigned i = 0, n = 0;

	printf("generating %sFLAC file for test\n", is_ogg? "Ogg " : "");

	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	streaminfo.is_last = false;
	streaminfo.type = FLAC__METADATA_TYPE_STREAMINFO;
	streaminfo.length = FLAC__STREAM_METADATA_STREAMINFO_LENGTH;
	streaminfo.data.stream_info.min_blocksize = 576;
	streaminfo.data.stream_info.max_blocksize = 576;
	streaminfo.data.stream_info.min_framesize = 0;
	streaminfo.data.stream_info.max_framesize = 0;
	streaminfo.data.stream_info.sample_rate = 44100;
	streaminfo.data.stream_info.channels = 1;
	streaminfo.data.stream_info.bits_per_sample = 8;
	streaminfo.data.stream_info.total_samples = 0;
	memset(streaminfo.data.stream_info.md5sum, 0, 16);

	{
		const unsigned vendor_string_length = (unsigned)strlen(FLAC__VENDOR_STRING);
		vorbiscomment.is_last = false;
		vorbiscomment.type = FLAC__METADATA_TYPE_VORBIS_COMMENT;
		vorbiscomment.length = (4 + vendor_string_length) + 4;
		vorbiscomment.data.vorbis_comment.vendor_string.length = vendor_string_length;
		vorbiscomment.data.vorbis_comment.vendor_string.entry = malloc_or_die_(vendor_string_length+1);
		memcpy(vorbiscomment.data.vorbis_comment.vendor_string.entry, FLAC__VENDOR_STRING, vendor_string_length+1);
		vorbiscomment.data.vorbis_comment.num_comments = 0;
		vorbiscomment.data.vorbis_comment.comments = 0;
	}

	{
		if (0 == (cuesheet = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET)))
			return die_("priming our metadata");
		cuesheet->is_last = false;
		strcpy(cuesheet->data.cue_sheet.media_catalog_number, "bogo-MCN");
		cuesheet->data.cue_sheet.lead_in = 123;
		cuesheet->data.cue_sheet.is_cd = false;
		if (!FLAC__metadata_object_cuesheet_insert_blank_track(cuesheet, 0))
			return die_("priming our metadata");
		cuesheet->data.cue_sheet.tracks[0].number = 1;
		if (!FLAC__metadata_object_cuesheet_track_insert_blank_index(cuesheet, 0, 0))
			return die_("priming our metadata");
	}

	{
		picture.is_last = false;
		picture.type = FLAC__METADATA_TYPE_PICTURE;
		picture.length =
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
		picture.data.picture.type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;
		picture.data.picture.mime_type = strdup_or_die_("image/jpeg");
		picture.length += strlen(picture.data.picture.mime_type);
		picture.data.picture.description = (FLAC__byte*)strdup_or_die_("desc");
		picture.length += strlen((const char *)picture.data.picture.description);
		picture.data.picture.width = 300;
		picture.data.picture.height = 300;
		picture.data.picture.depth = 24;
		picture.data.picture.colors = 0;
		picture.data.picture.data = (FLAC__byte*)strdup_or_die_("SOMEJPEGDATA");
		picture.data.picture.data_length = strlen((const char *)picture.data.picture.data);
		picture.length += picture.data.picture.data_length;
	}

	padding.is_last = true;
	padding.type = FLAC__METADATA_TYPE_PADDING;
	padding.length = 1234;

	metadata[n++] = &vorbiscomment;
	if(include_extras) {
		metadata[n++] = cuesheet;
		metadata[n++] = &picture;
	}
	metadata[n++] = &padding;

	if(
		!insert_to_our_metadata_(&streaminfo, i++, /*copy=*/true) ||
		!insert_to_our_metadata_(&vorbiscomment, i++, /*copy=*/true) ||
		(include_extras && !insert_to_our_metadata_(cuesheet, i++, /*copy=*/false)) ||
		(include_extras && !insert_to_our_metadata_(&picture, i++, /*copy=*/true)) ||
		!insert_to_our_metadata_(&padding, i++, /*copy=*/true)
	)
		return die_("priming our metadata");

	if(!file_utils__generate_flacfile(is_ogg, flacfilename(is_ogg), 0, 512 * 1024, &streaminfo, metadata, n))
		return die_("creating the encoded file");

	free(vorbiscomment.data.vorbis_comment.vendor_string.entry);
	free(picture.data.picture.mime_type);
	free(picture.data.picture.description);
	free(picture.data.picture.data);
	if(!include_extras)
		FLAC__metadata_object_delete(cuesheet);

	return true;
}

static FLAC__bool test_file_(FLAC__bool is_ogg, FLAC__StreamDecoderMetadataCallback metadata_callback)
{
	const char *filename = flacfilename(is_ogg);
	FLAC__StreamDecoder *decoder;
	decoder_client_struct decoder_client_data;

	FLAC__ASSERT(0 != metadata_callback);

	mc_our_block_number_ = 0;
	decoder_client_data.error_occurred = false;

	printf("\ttesting '%s'... ", filename);
	fflush(stdout);

	if(0 == (decoder = FLAC__stream_decoder_new()))
		return die_("couldn't allocate decoder instance");

	FLAC__stream_decoder_set_md5_checking(decoder, true);
	FLAC__stream_decoder_set_metadata_respond_all(decoder);
	if(
		(is_ogg?
			FLAC__stream_decoder_init_ogg_file(decoder, filename, decoder_write_callback_, metadata_callback, decoder_error_callback_, &decoder_client_data) :
			FLAC__stream_decoder_init_file(decoder, filename, decoder_write_callback_, metadata_callback, decoder_error_callback_, &decoder_client_data)
		) != FLAC__STREAM_DECODER_INIT_STATUS_OK
	) {
		(void)FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return die_("initializing decoder\n");
	}
	if(!FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
		(void)FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
		return die_("decoding file\n");
	}

	(void)FLAC__stream_decoder_finish(decoder);
	FLAC__stream_decoder_delete(decoder);

	if(decoder_client_data.error_occurred)
		return false;

	if(mc_our_block_number_ != our_metadata_.num_blocks)
		return die_("short metadata block count");

	printf("PASSED\n");
	return true;
}

static FLAC__bool change_stats_(const char *filename, FLAC__bool read_only)
{
	if(!grabbag__file_change_stats(filename, read_only))
		return die_("during grabbag__file_change_stats()");

	return true;
}

static FLAC__bool remove_file_(const char *filename)
{
	while(our_metadata_.num_blocks > 0)
		delete_from_our_metadata_(0);

	if(!grabbag__file_remove_file(filename))
		return die_("removing file");

	return true;
}

static FLAC__bool test_level_0_(void)
{
	FLAC__StreamMetadata streaminfo;
	FLAC__StreamMetadata *tags = 0;
	FLAC__StreamMetadata *cuesheet = 0;
	FLAC__StreamMetadata *picture = 0;

	printf("\n\n++++++ testing level 0 interface\n");

	if(!generate_file_(/*include_extras=*/true, /*is_ogg=*/false))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_null_))
		return false;

	printf("testing FLAC__metadata_get_streaminfo()... ");

	if(!FLAC__metadata_get_streaminfo(flacfilename(/*is_ogg=*/false), &streaminfo))
		return die_("during FLAC__metadata_get_streaminfo()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(streaminfo.data.stream_info.channels != 1)
		return die_("mismatch in streaminfo.data.stream_info.channels");
	if(streaminfo.data.stream_info.bits_per_sample != 8)
		return die_("mismatch in streaminfo.data.stream_info.bits_per_sample");
	if(streaminfo.data.stream_info.sample_rate != 44100)
		return die_("mismatch in streaminfo.data.stream_info.sample_rate");
	if(streaminfo.data.stream_info.min_blocksize != 576)
		return die_("mismatch in streaminfo.data.stream_info.min_blocksize");
	if(streaminfo.data.stream_info.max_blocksize != 576)
		return die_("mismatch in streaminfo.data.stream_info.max_blocksize");

	printf("OK\n");

	printf("testing FLAC__metadata_get_tags()... ");

	if(!FLAC__metadata_get_tags(flacfilename(/*is_ogg=*/false), &tags))
		return die_("during FLAC__metadata_get_tags()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(tags->data.vorbis_comment.num_comments != 0)
		return die_("mismatch in tags->data.vorbis_comment.num_comments");

	printf("OK\n");

	FLAC__metadata_object_delete(tags);

	printf("testing FLAC__metadata_get_cuesheet()... ");

	if(!FLAC__metadata_get_cuesheet(flacfilename(/*is_ogg=*/false), &cuesheet))
		return die_("during FLAC__metadata_get_cuesheet()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(cuesheet->data.cue_sheet.lead_in != 123)
		return die_("mismatch in cuesheet->data.cue_sheet.lead_in");

	printf("OK\n");

	FLAC__metadata_object_delete(cuesheet);

	printf("testing FLAC__metadata_get_picture()... ");

	if(!FLAC__metadata_get_picture(flacfilename(/*is_ogg=*/false), &picture, /*type=*/(FLAC__StreamMetadata_Picture_Type)(-1), /*mime_type=*/0, /*description=*/0, /*max_width=*/(unsigned)(-1), /*max_height=*/(unsigned)(-1), /*max_depth=*/(unsigned)(-1), /*max_colors=*/(unsigned)(-1)))
		return die_("during FLAC__metadata_get_picture()");

	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(picture->data.picture.type != FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER)
		return die_("mismatch in picture->data.picture.type");

	printf("OK\n");

	FLAC__metadata_object_delete(picture);

	if(!remove_file_(flacfilename(/*is_ogg=*/false)))
		return false;

	return true;
}

static FLAC__bool test_level_1_(void)
{
	FLAC__Metadata_SimpleIterator *iterator;
	FLAC__StreamMetadata *block, *app, *padding;
	FLAC__byte data[1000];
	unsigned our_current_position = 0;

	/* initialize 'data' to avoid Valgrind errors */
	memset(data, 0, sizeof(data));

	printf("\n\n++++++ testing level 1 interface\n");

	/************************************************************/

	printf("simple iterator on read-only file\n");

	if(!generate_file_(/*include_extras=*/false, /*is_ogg=*/false))
		return false;

	if(!change_stats_(flacfilename(/*is_ogg=*/false), /*read_only=*/true))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_null_))
		return false;

	if(0 == (iterator = FLAC__metadata_simple_iterator_new()))
		return die_("FLAC__metadata_simple_iterator_new()");

	if(!FLAC__metadata_simple_iterator_init(iterator, flacfilename(/*is_ogg=*/false), /*read_only=*/false, /*preserve_file_stats=*/false))
		return die_("FLAC__metadata_simple_iterator_init() returned false");

	printf("is writable = %u\n", (unsigned)FLAC__metadata_simple_iterator_is_writable(iterator));
	if(FLAC__metadata_simple_iterator_is_writable(iterator))
		return die_("iterator claims file is writable when tester thinks it should not be; are you running as root?\n");

	printf("iterate forwards\n");

	if(FLAC__metadata_simple_iterator_get_block_type(iterator) != FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type from FLAC__metadata_simple_iterator_get_block_type()");
	if(0 == (block = FLAC__metadata_simple_iterator_get_block(iterator)))
		return die_("getting block 0");
	if(block->type != FLAC__METADATA_TYPE_STREAMINFO)
		return die_("expected STREAMINFO type");
	if(block->is_last)
		return die_("expected is_last to be false");
	if(block->length != FLAC__STREAM_METADATA_STREAMINFO_LENGTH)
		return die_("bad STREAMINFO length");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(block->data.stream_info.channels != 1)
		return die_("mismatch in channels");
	if(block->data.stream_info.bits_per_sample != 8)
		return die_("mismatch in bits_per_sample");
	if(block->data.stream_info.sample_rate != 44100)
		return die_("mismatch in sample_rate");
	if(block->data.stream_info.min_blocksize != 576)
		return die_("mismatch in min_blocksize");
	if(block->data.stream_info.max_blocksize != 576)
		return die_("mismatch in max_blocksize");
	FLAC__metadata_object_delete(block);

	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("forward iterator ended early");
	our_current_position++;

	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("forward iterator ended early");
	our_current_position++;

	if(FLAC__metadata_simple_iterator_get_block_type(iterator) != FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type from FLAC__metadata_simple_iterator_get_block_type()");
	if(0 == (block = FLAC__metadata_simple_iterator_get_block(iterator)))
		return die_("getting block 2");
	if(block->type != FLAC__METADATA_TYPE_PADDING)
		return die_("expected PADDING type");
	if(!block->is_last)
		return die_("expected is_last to be true");
	/* check to see if some basic data matches (c.f. generate_file_()) */
	if(block->length != 1234)
		return die_("bad PADDING length");
	FLAC__metadata_object_delete(block);

	if(FLAC__metadata_simple_iterator_next(iterator))
		return die_("forward iterator returned true but should have returned false");

	printf("iterate backwards\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("reverse iterator ended early");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("reverse iterator ended early");
	if(FLAC__metadata_simple_iterator_prev(iterator))
		return die_("reverse iterator returned true but should have returned false");

	printf("testing FLAC__metadata_simple_iterator_set_block() on read-only file...\n");

	if(!FLAC__metadata_simple_iterator_set_block(iterator, (FLAC__StreamMetadata*)99, false))
		printf("OK: FLAC__metadata_simple_iterator_set_block() returned false like it should\n");
	else
		return die_("FLAC__metadata_simple_iterator_set_block() returned true but shouldn't have");

	FLAC__metadata_simple_iterator_delete(iterator);

	/************************************************************/

	printf("simple iterator on writable file\n");

	if(!change_stats_(flacfilename(/*is_ogg=*/false), /*read-only=*/false))
		return false;

	printf("creating APPLICATION block\n");

	if(0 == (app = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)");
	memcpy(app->data.application.id, "duh", (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));

	printf("creating PADDING block\n");

	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)");
	padding->length = 20;

	if(0 == (iterator = FLAC__metadata_simple_iterator_new()))
		return die_("FLAC__metadata_simple_iterator_new()");

	if(!FLAC__metadata_simple_iterator_init(iterator, flacfilename(/*is_ogg=*/false), /*read_only=*/false, /*preserve_file_stats=*/false))
		return die_("FLAC__metadata_simple_iterator_init() returned false");
	our_current_position = 0;

	printf("is writable = %u\n", (unsigned)FLAC__metadata_simple_iterator_is_writable(iterator));

	printf("[S]VP\ttry to write over STREAMINFO block...\n");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, false))
		printf("\tFLAC__metadata_simple_iterator_set_block() returned false like it should\n");
	else
		return die_("FLAC__metadata_simple_iterator_set_block() returned true but shouldn't have");

	printf("[S]VP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tinsert PADDING after, don't expand into padding\n");
	padding->length = 25;
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	printf("SVP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PP\tinsert PADDING after, don't expand into padding\n");
	padding->length = 30;
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[P]PP\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PPP\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VPPP\tdelete (STREAMINFO block), must fail\n");
	if(FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false) should have returned false", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("[S]VPPP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]PPP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]PP\tdelete (middle block), replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, true))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, true)", iterator);
	our_current_position--;

	printf("S[V]PPP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]\tdelete (last block), replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, true))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	our_current_position--;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]\tdelete (last block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VP\tset STREAMINFO (change sample rate)\n");
	FLAC__ASSERT(our_current_position == 0);
	block = FLAC__metadata_simple_iterator_get_block(iterator);
	block->data.stream_info.sample_rate = 32000;
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, block, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, block, false)", iterator);
	FLAC__metadata_object_delete(block);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("[S]VP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tinsert APPLICATION after, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'e'; /* twiddle the id so that our comparison doesn't miss transposition */
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + app->length;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVA[P]\tset APPLICATION, expand into padding of exceeding size\n");
	app->data.application.id[0] = 'f'; /* twiddle the id */
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);
	if(!insert_to_our_metadata_(app, our_current_position, /*copy=*/true))
		return false;
	our_metadata_.blocks[our_current_position+1]->length -= (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) + app->length;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]P\tset APPLICATION (grow), don't expand into padding\n");
	app->data.application.id[0] = 'g'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, false)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]P\tset APPLICATION (shrink), don't fill in with padding\n");
	app->data.application.id[0] = 'h'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, 12, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, false)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]P\tset APPLICATION (grow), expand into padding of exceeding size\n");
	app->data.application.id[0] = 'i'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, sizeof(data), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length -= (sizeof(data) - 12);
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]P\tset APPLICATION (shrink), fill in with padding\n");
	app->data.application.id[0] = 'j'; /* twiddle the id */
	if(!FLAC__metadata_object_application_set_data(app, data, 23, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = sizeof(data) - 23 - FLAC__STREAM_METADATA_HEADER_LENGTH;
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVAA[P]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVAAP[P]\tset PADDING (shrink), don't fill in with padding\n");
	padding->length = 5;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, padding, false)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVAAP[P]\tset APPLICATION (grow)\n");
	app->data.application.id[0] = 'k'; /* twiddle the id */
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, false)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVAAP[A]\tset PADDING (equal)\n");
	padding->length = 27;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, padding, false)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVAAP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVAA[P]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVA[A]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVA[P]\tinsert PADDING after\n");
	padding->length = 5;
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVAP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVA[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[A]PP\tset APPLICATION (grow), try to expand into padding which is too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 32, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), try to expand into padding which is 'close' but still too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 60, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), expand into padding which will leave 0-length pad\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 87, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 0;
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PP\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 91, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tset APPLICATION (grow), expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 100, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	our_metadata_.blocks[our_current_position]->is_last = true;
	if(!FLAC__metadata_simple_iterator_set_block(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tset PADDING (equal size)\n");
	padding->length = app->length;
	if(!replace_in_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_set_block(iterator, padding, true))
		return die_ss_("FLAC__metadata_simple_iterator_set_block(iterator, padding, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[P]\tinsert PADDING after\n");
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVP[P]\tinsert PADDING after\n");
	padding->length = 5;
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, padding, false)", iterator);
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return false;

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SVPP[P]\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SVP[P]P\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("SV[P]PP\tprev\n");
	if(!FLAC__metadata_simple_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]PPP\tinsert APPLICATION after, try to expand into padding which is too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 101, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]PPP\tinsert APPLICATION after, try to expand into padding which is 'close' but still too small\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 97, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PPP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]PPP\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 100, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]PP\tinsert APPLICATION after, expand into padding which will leave 0-length pad\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 96, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 0;
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]PP\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]PP\tnext\n");
	if(!FLAC__metadata_simple_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tdelete (middle block), don't replace with padding\n");
	if(!FLAC__metadata_simple_iterator_delete_block(iterator, false))
		return die_ss_("FLAC__metadata_simple_iterator_delete_block(iterator, false)", iterator);
	delete_from_our_metadata_(our_current_position--);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("S[V]P\tinsert APPLICATION after, expand into padding which is exactly consumed\n");
	if(!FLAC__metadata_object_application_set_data(app, data, 1, true))
		return die_("setting APPLICATION data");
	if(!insert_to_our_metadata_(app, ++our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true))
		return die_ss_("FLAC__metadata_simple_iterator_insert_block_after(iterator, app, true)", iterator);

	if(!test_file_(/*is_ogg=*/false, decoder_metadata_callback_compare_))
		return false;

	printf("delete simple iterator\n");

	FLAC__metadata_simple_iterator_delete(iterator);

	FLAC__metadata_object_delete(app);
	FLAC__metadata_object_delete(padding);

	if(!remove_file_(flacfilename(/*is_ogg=*/false)))
		return false;

	return true;
}

static FLAC__bool test_level_2_(FLAC__bool filename_based, FLAC__bool is_ogg)
{
	FLAC__Metadata_Iterator *iterator;
	FLAC__Metadata_Chain *chain;
	FLAC__StreamMetadata *block, *app, *padding;
	FLAC__byte data[2000];
	unsigned our_current_position;

	/* initialize 'data' to avoid Valgrind errors */
	memset(data, 0, sizeof(data));

	printf("\n\n++++++ testing level 2 interface (%s-based, %s FLAC)\n", filename_based? "filename":"callback", is_ogg? "Ogg":"native");

	printf("generate read-only file\n");

	if(!generate_file_(/*include_extras=*/false, is_ogg))
		return false;

	if(!change_stats_(flacfilename(is_ogg), /*read_only=*/true))
		return false;

	printf("create chain\n");

	if(0 == (chain = FLAC__metadata_chain_new()))
		return die_("allocating chain");

	printf("read chain\n");

	if(!read_chain_(chain, flacfilename(is_ogg), filename_based, is_ogg))
		return die_c_("reading chain", FLAC__metadata_chain_status(chain));

	printf("[S]VP\ttest initial metadata\n");

	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	if(is_ogg)
		goto end;

	printf("switch file to read-write\n");

	if(!change_stats_(flacfilename(is_ogg), /*read-only=*/false))
		return false;

	printf("create iterator\n");
	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	our_current_position = 0;

	FLAC__metadata_iterator_init(iterator, chain);

	if(0 == (block = FLAC__metadata_iterator_get_block(iterator)))
		return die_("getting block from iterator");

	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	printf("[S]VP\tmodify STREAMINFO, write\n");

	block->data.stream_info.sample_rate = 32000;
	if(!replace_in_our_metadata_(block, our_current_position, /*copy=*/true))
		return die_("copying object");

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/true, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, true)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("[S]VP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]P\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\treplace PADDING with identical-size APPLICATION\n");
	if(0 == (block = FLAC__metadata_iterator_get_block(iterator)))
		return die_("getting block from iterator");
	if(0 == (app = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)))
		return die_("FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION)");
	memcpy(app->data.application.id, "duh", (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));
	if(!FLAC__metadata_object_application_set_data(app, data, block->length-(FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8), true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tshrink APPLICATION, don't use padding\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 26, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tgrow APPLICATION, don't use padding\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 28, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tgrow APPLICATION, use padding, but last block is not padding\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 36, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tshrink APPLICATION, use padding, last block is not padding, but delta is too small for new PADDING block\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 33, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tshrink APPLICATION, use padding, last block is not padding, delta is enough for new PADDING block\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 29, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	padding->length = 0;
	if(!insert_to_our_metadata_(padding, our_current_position+1, /*copy=*/false))
		return die_("internal error");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tshrink APPLICATION, use padding, last block is padding\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 16, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length = 13;
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding, but delta is too small\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 50, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding of exceeding size\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 56, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	our_metadata_.blocks[our_current_position+1]->length -= (56 - 50);
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]P\tgrow APPLICATION, use padding, last block is padding of exact size\n");
	if(0 == (app = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("copying object");
	if(!FLAC__metadata_object_application_set_data(app, data, 67, true))
		return die_("setting APPLICATION data");
	if(!replace_in_our_metadata_(app, our_current_position, /*copy=*/true))
		return die_("copying object");
	delete_from_our_metadata_(our_current_position+1);
	if(!FLAC__metadata_iterator_set_block(iterator, app))
		return die_c_("FLAC__metadata_iterator_set_block(iterator, app)", FLAC__metadata_chain_status(chain));

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV[A]\tprev\n");
	if(!FLAC__metadata_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("S[V]A\tprev\n");
	if(!FLAC__metadata_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]VA\tinsert PADDING before STREAMINFO (should fail)\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	padding->length = 30;
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		printf("\tFLAC__metadata_iterator_insert_block_before() returned false like it should\n");
	else
		return die_("FLAC__metadata_iterator_insert_block_before() should have returned false");

	printf("[S]VP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]A\tinsert PADDING after\n");
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_after(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_after(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("SV[P]A\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("creating PADDING block");
	padding->length = 17;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("SV[P]PA\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_clone(our_metadata_.blocks[our_current_position])))
		return die_("creating PADDING block");
	padding->length = 0;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("SV[P]PPA\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVP[P]PA\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVPP[P]A\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SVPPP[A]\tinsert PADDING after\n");
	if(0 == (padding = FLAC__metadata_object_clone(our_metadata_.blocks[2])))
		return die_("creating PADDING block");
	padding->length = 57;
	if(!insert_to_our_metadata_(padding, ++our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_after(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_after(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("SVPPPA[P]\tinsert PADDING before\n");
	if(0 == (padding = FLAC__metadata_object_clone(our_metadata_.blocks[2])))
		return die_("creating PADDING block");
	padding->length = 99;
	if(!insert_to_our_metadata_(padding, our_current_position, /*copy=*/true))
		return die_("copying metadata");
	if(!FLAC__metadata_iterator_insert_block_before(iterator, padding))
		return die_("FLAC__metadata_iterator_insert_block_before(iterator, padding)");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("delete iterator\n");
	FLAC__metadata_iterator_delete(iterator);
	our_current_position = 0;

	printf("SVPPPAPP\tmerge padding\n");
	FLAC__metadata_chain_merge_padding(chain);
	our_metadata_.blocks[2]->length += (FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[3]->length);
	our_metadata_.blocks[2]->length += (FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[4]->length);
	our_metadata_.blocks[6]->length += (FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[7]->length);
	delete_from_our_metadata_(7);
	delete_from_our_metadata_(4);
	delete_from_our_metadata_(3);

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SVPAP\tsort padding\n");
	FLAC__metadata_chain_sort_padding(chain);
	our_metadata_.blocks[4]->length += (FLAC__STREAM_METADATA_HEADER_LENGTH + our_metadata_.blocks[2]->length);
	delete_from_our_metadata_(2);

	if(!write_chain_(chain, /*use_padding=*/true, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, true, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("create iterator\n");
	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	our_current_position = 0;

	FLAC__metadata_iterator_init(iterator, chain);

	printf("[S]VAP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("S[V]AP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[A]P\tdelete middle block, replace with padding\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	padding->length = 71;
	if(!replace_in_our_metadata_(padding, our_current_position--, /*copy=*/false))
		return die_("copying object");
	if(!FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/true))
		return die_c_("FLAC__metadata_iterator_delete_block(iterator, true)", FLAC__metadata_chain_status(chain));

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[V]PP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]P\tdelete middle block, don't replace with padding\n");
	delete_from_our_metadata_(our_current_position--);
	if(!FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/false))
		return die_c_("FLAC__metadata_iterator_delete_block(iterator, false)", FLAC__metadata_chain_status(chain));

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[V]P\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tdelete last block, replace with padding\n");
	if(0 == (padding = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING)))
		return die_("creating PADDING block");
	padding->length = 219;
	if(!replace_in_our_metadata_(padding, our_current_position--, /*copy=*/false))
		return die_("copying object");
	if(!FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/true))
		return die_c_("FLAC__metadata_iterator_delete_block(iterator, true)", FLAC__metadata_chain_status(chain));

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[V]P\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");
	our_current_position++;

	printf("SV[P]\tdelete last block, don't replace with padding\n");
	delete_from_our_metadata_(our_current_position--);
	if(!FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/false))
		return die_c_("FLAC__metadata_iterator_delete_block(iterator, false)", FLAC__metadata_chain_status(chain));

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("S[V]\tprev\n");
	if(!FLAC__metadata_iterator_prev(iterator))
		return die_("iterator ended early\n");
	our_current_position--;

	printf("[S]V\tdelete STREAMINFO block, should fail\n");
	if(FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/false))
		return die_("FLAC__metadata_iterator_delete_block() on STREAMINFO should have failed but didn't");

	if(!compare_chain_(chain, our_current_position, FLAC__metadata_iterator_get_block(iterator)))
		return false;

	printf("delete iterator\n");
	FLAC__metadata_iterator_delete(iterator);
	our_current_position = 0;

	printf("SV\tmerge padding\n");
	FLAC__metadata_chain_merge_padding(chain);

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

	printf("SV\tsort padding\n");
	FLAC__metadata_chain_sort_padding(chain);

	if(!write_chain_(chain, /*use_padding=*/false, /*preserve_file_stats=*/false, filename_based, flacfilename(is_ogg)))
		return die_c_("during FLAC__metadata_chain_write(chain, false, false)", FLAC__metadata_chain_status(chain));
	if(!compare_chain_(chain, 0, 0))
		return false;
	if(!test_file_(is_ogg, decoder_metadata_callback_compare_))
		return false;

end:
	printf("delete chain\n");

	FLAC__metadata_chain_delete(chain);

	if(!remove_file_(flacfilename(is_ogg)))
		return false;

	return true;
}

static FLAC__bool test_level_2_misc_(FLAC__bool is_ogg)
{
	FLAC__Metadata_Iterator *iterator;
	FLAC__Metadata_Chain *chain;
	FLAC__IOCallbacks callbacks;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.read = (FLAC__IOCallback_Read)fread;
#ifdef FLAC__VALGRIND_TESTING
	callbacks.write = chain_write_cb_;
#else
	callbacks.write = (FLAC__IOCallback_Write)fwrite;
#endif
	callbacks.seek = chain_seek_cb_;
	callbacks.tell = chain_tell_cb_;
	callbacks.eof = chain_eof_cb_;

	printf("\n\n++++++ testing level 2 interface (mismatched read/write protections)\n");

	printf("generate file\n");

	if(!generate_file_(/*include_extras=*/false, is_ogg))
		return false;

	printf("create chain\n");

	if(0 == (chain = FLAC__metadata_chain_new()))
		return die_("allocating chain");

	printf("read chain (filename-based)\n");

	if(!FLAC__metadata_chain_read(chain, flacfilename(is_ogg)))
		return die_c_("reading chain", FLAC__metadata_chain_status(chain));

	printf("write chain with wrong method FLAC__metadata_chain_write_with_callbacks()\n");
	{
		if(FLAC__metadata_chain_write_with_callbacks(chain, /*use_padding=*/false, 0, callbacks))
			return die_c_("mismatched write should have failed", FLAC__metadata_chain_status(chain));
		if(FLAC__metadata_chain_status(chain) != FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH)
			return die_c_("expected FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH", FLAC__metadata_chain_status(chain));
		printf("  OK: FLAC__metadata_chain_write_with_callbacks() returned false,FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH like it should\n");
	}

	printf("read chain (filename-based)\n");

	if(!FLAC__metadata_chain_read(chain, flacfilename(is_ogg)))
		return die_c_("reading chain", FLAC__metadata_chain_status(chain));

	printf("write chain with wrong method FLAC__metadata_chain_write_with_callbacks_and_tempfile()\n");
	{
		if(FLAC__metadata_chain_write_with_callbacks_and_tempfile(chain, /*use_padding=*/false, 0, callbacks, 0, callbacks))
			return die_c_("mismatched write should have failed", FLAC__metadata_chain_status(chain));
		if(FLAC__metadata_chain_status(chain) != FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH)
			return die_c_("expected FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH", FLAC__metadata_chain_status(chain));
		printf("  OK: FLAC__metadata_chain_write_with_callbacks_and_tempfile() returned false,FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH like it should\n");
	}

	printf("read chain (callback-based)\n");
	{
		FILE *file = fopen(flacfilename(is_ogg), "rb");
		if(0 == file)
			return die_("opening file");
		if(!FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, callbacks)) {
			fclose(file);
			return die_c_("reading chain", FLAC__metadata_chain_status(chain));
		}
		fclose(file);
	}

	printf("write chain with wrong method FLAC__metadata_chain_write()\n");
	{
		if(FLAC__metadata_chain_write(chain, /*use_padding=*/false, /*preserve_file_stats=*/false))
			return die_c_("mismatched write should have failed", FLAC__metadata_chain_status(chain));
		if(FLAC__metadata_chain_status(chain) != FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH)
			return die_c_("expected FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH", FLAC__metadata_chain_status(chain));
		printf("  OK: FLAC__metadata_chain_write() returned false,FLAC__METADATA_CHAIN_STATUS_READ_WRITE_MISMATCH like it should\n");
	}

	printf("read chain (callback-based)\n");
	{
		FILE *file = fopen(flacfilename(is_ogg), "rb");
		if(0 == file)
			return die_("opening file");
		if(!FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, callbacks)) {
			fclose(file);
			return die_c_("reading chain", FLAC__metadata_chain_status(chain));
		}
		fclose(file);
	}

	printf("testing FLAC__metadata_chain_check_if_tempfile_needed()... ");

	if(!FLAC__metadata_chain_check_if_tempfile_needed(chain, /*use_padding=*/false))
		printf("OK: FLAC__metadata_chain_check_if_tempfile_needed() returned false like it should\n");
	else
		return die_("FLAC__metadata_chain_check_if_tempfile_needed() returned true but shouldn't have");

	printf("write chain with wrong method FLAC__metadata_chain_write_with_callbacks_and_tempfile()\n");
	{
		if(FLAC__metadata_chain_write_with_callbacks_and_tempfile(chain, /*use_padding=*/false, 0, callbacks, 0, callbacks))
			return die_c_("mismatched write should have failed", FLAC__metadata_chain_status(chain));
		if(FLAC__metadata_chain_status(chain) != FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL)
			return die_c_("expected FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL", FLAC__metadata_chain_status(chain));
		printf("  OK: FLAC__metadata_chain_write_with_callbacks_and_tempfile() returned false,FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL like it should\n");
	}

	printf("read chain (callback-based)\n");
	{
		FILE *file = fopen(flacfilename(is_ogg), "rb");
		if(0 == file)
			return die_("opening file");
		if(!FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, callbacks)) {
			fclose(file);
			return die_c_("reading chain", FLAC__metadata_chain_status(chain));
		}
		fclose(file);
	}

	printf("create iterator\n");
	if(0 == (iterator = FLAC__metadata_iterator_new()))
		return die_("allocating memory for iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	printf("[S]VP\tnext\n");
	if(!FLAC__metadata_iterator_next(iterator))
		return die_("iterator ended early\n");

	printf("S[V]P\tdelete VORBIS_COMMENT, write\n");
	if(!FLAC__metadata_iterator_delete_block(iterator, /*replace_with_padding=*/false))
		return die_c_("block delete failed\n", FLAC__metadata_chain_status(chain));

	printf("testing FLAC__metadata_chain_check_if_tempfile_needed()... ");

	if(FLAC__metadata_chain_check_if_tempfile_needed(chain, /*use_padding=*/false))
		printf("OK: FLAC__metadata_chain_check_if_tempfile_needed() returned true like it should\n");
	else
		return die_("FLAC__metadata_chain_check_if_tempfile_needed() returned false but shouldn't have");

	printf("write chain with wrong method FLAC__metadata_chain_write_with_callbacks()\n");
	{
		if(FLAC__metadata_chain_write_with_callbacks(chain, /*use_padding=*/false, 0, callbacks))
			return die_c_("mismatched write should have failed", FLAC__metadata_chain_status(chain));
		if(FLAC__metadata_chain_status(chain) != FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL)
			return die_c_("expected FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL", FLAC__metadata_chain_status(chain));
		printf("  OK: FLAC__metadata_chain_write_with_callbacks() returned false,FLAC__METADATA_CHAIN_STATUS_WRONG_WRITE_CALL like it should\n");
	}

	printf("delete iterator\n");

	FLAC__metadata_iterator_delete(iterator);

	printf("delete chain\n");

	FLAC__metadata_chain_delete(chain);

	if(!remove_file_(flacfilename(is_ogg)))
		return false;

	return true;
}

FLAC__bool test_metadata_file_manipulation(void)
{
	printf("\n+++ libFLAC unit test: metadata manipulation\n\n");

	our_metadata_.num_blocks = 0;

	if(!test_level_0_())
		return false;

	if(!test_level_1_())
		return false;

	if(!test_level_2_(/*filename_based=*/true, /*is_ogg=*/false)) /* filename-based */
		return false;
	if(!test_level_2_(/*filename_based=*/false, /*is_ogg=*/false)) /* callback-based */
		return false;
	if(!test_level_2_misc_(/*is_ogg=*/false))
		return false;

	if(FLAC_API_SUPPORTS_OGG_FLAC) {
		if(!test_level_2_(/*filename_based=*/true, /*is_ogg=*/true)) /* filename-based */
			return false;
		if(!test_level_2_(/*filename_based=*/false, /*is_ogg=*/true)) /* callback-based */
			return false;
#if 0
		/* when ogg flac write is supported, will have to add this: */
		if(!test_level_2_misc_(/*is_ogg=*/true))
			return false;
#endif
	}

	return true;
}
