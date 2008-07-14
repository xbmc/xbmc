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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _MSC_VER || defined __MINGW32__
#if _MSC_VER <= 1600 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#endif
#include "decoders.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h" // for ::FLAC__metadata_object_is_equal()
#include "FLAC++/decoder.h"
#include "share/grabbag.h"
extern "C" {
#include "test_libs_common/file_utils_flac.h"
#include "test_libs_common/metadata_utils.h"
}

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

typedef enum {
	LAYER_STREAM = 0, /* FLAC__stream_decoder_init_stream() without seeking */
	LAYER_SEEKABLE_STREAM, /* FLAC__stream_decoder_init_stream() with seeking */
	LAYER_FILE, /* FLAC__stream_decoder_init_FILE() */
	LAYER_FILENAME /* FLAC__stream_decoder_init_file() */
} Layer;

static const char * const LayerString[] = {
	"Stream",
	"Seekable Stream",
	"FILE*",
	"Filename"
};

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, picture_, unknown_;
static ::FLAC__StreamMetadata *expected_metadata_sequence_[9];
static unsigned num_expected_;
static off_t flacfilesize_;

static const char *flacfilename(bool is_ogg)
{
	return is_ogg? "metadata.oga" : "metadata.flac";
}

static bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_s_(const char *msg, const FLAC::Decoder::Stream *decoder)
{
	FLAC::Decoder::Stream::State state = decoder->get_state();

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());

	return false;
}

static void init_metadata_blocks_()
{
	mutils__init_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static void free_metadata_blocks_()
{
	mutils__free_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static bool generate_file_(FLAC__bool is_ogg)
{
	printf("\n\ngenerating %sFLAC file for decoder tests...\n", is_ogg? "Ogg ":"");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;
	/* WATCHOUT: for Ogg FLAC the encoder should move the VORBIS_COMMENT block to the front, right after STREAMINFO */

	if(!file_utils__generate_flacfile(is_ogg, flacfilename(is_ogg), &flacfilesize_, 512 * 1024, &streaminfo_, expected_metadata_sequence_, num_expected_))
		return die_("creating the encoded file");

	return true;
}


class DecoderCommon {
public:
	Layer layer_;
	unsigned current_metadata_number_;
	bool ignore_errors_;
	bool error_occurred_;

	DecoderCommon(Layer layer): layer_(layer), current_metadata_number_(0), ignore_errors_(false), error_occurred_(false) { }
	::FLAC__StreamDecoderWriteStatus common_write_callback_(const ::FLAC__Frame *frame);
	void common_metadata_callback_(const ::FLAC__StreamMetadata *metadata);
	void common_error_callback_(::FLAC__StreamDecoderErrorStatus status);
};

::FLAC__StreamDecoderWriteStatus DecoderCommon::common_write_callback_(const ::FLAC__Frame *frame)
{
	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	if(
		(frame->header.number_type == ::FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER && frame->header.number.frame_number == 0) ||
		(frame->header.number_type == ::FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER && frame->header.number.sample_number == 0)
	) {
		printf("content... ");
		fflush(stdout);
	}

	return ::FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void DecoderCommon::common_metadata_callback_(const ::FLAC__StreamMetadata *metadata)
{
	if(error_occurred_)
		return;

	printf("%d... ", current_metadata_number_);
	fflush(stdout);

	if(current_metadata_number_ >= num_expected_) {
		(void)die_("got more metadata blocks than expected");
		error_occurred_ = true;
	}
	else {
		if(!::FLAC__metadata_object_is_equal(expected_metadata_sequence_[current_metadata_number_], metadata)) {
			(void)die_("metadata block mismatch");
			error_occurred_ = true;
		}
	}
	current_metadata_number_++;
}

void DecoderCommon::common_error_callback_(::FLAC__StreamDecoderErrorStatus status)
{
	if(!ignore_errors_) {
		printf("ERROR: got error callback: err = %u (%s)\n", (unsigned)status, ::FLAC__StreamDecoderErrorStatusString[status]);
		error_occurred_ = true;
	}
}

class StreamDecoder : public FLAC::Decoder::Stream, public DecoderCommon {
public:
	FILE *file_;

	StreamDecoder(Layer layer): FLAC::Decoder::Stream(), DecoderCommon(layer), file_(0) { }
	~StreamDecoder() { }

	// from FLAC::Decoder::Stream
	::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
	::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
	bool eof_callback();
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool test_respond(bool is_ogg);
};

::FLAC__StreamDecoderReadStatus StreamDecoder::read_callback(FLAC__byte buffer[], size_t *bytes)
{
	const size_t requested_bytes = *bytes;

	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	if(feof(file_)) {
		*bytes = 0;
		return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(requested_bytes > 0) {
		*bytes = ::fread(buffer, 1, requested_bytes, file_);
		if(*bytes == 0) {
			if(feof(file_))
				return ::FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			else
				return ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else {
			return ::FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
		return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

::FLAC__StreamDecoderSeekStatus StreamDecoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if(layer_ == LAYER_STREAM)
		return ::FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;

	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

	if(fseeko(file_, (off_t)absolute_byte_offset, SEEK_SET) < 0) {
		error_occurred_ = true;
		return ::FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

	return ::FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

::FLAC__StreamDecoderTellStatus StreamDecoder::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
	if(layer_ == LAYER_STREAM)
		return ::FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;

	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

	off_t offset = ftello(file_);
	*absolute_byte_offset = (FLAC__uint64)offset;

	if(offset < 0) {
		error_occurred_ = true;
		return ::FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}

	return ::FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

::FLAC__StreamDecoderLengthStatus StreamDecoder::length_callback(FLAC__uint64 *stream_length)
{
	if(layer_ == LAYER_STREAM)
		return ::FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;

	if(error_occurred_)
		return ::FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	*stream_length = (FLAC__uint64)flacfilesize_;
	return ::FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool StreamDecoder::eof_callback()
{
	if(layer_ == LAYER_STREAM)
		return false;

	if(error_occurred_)
		return true;

	return (bool)feof(file_);
}

::FLAC__StreamDecoderWriteStatus StreamDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;

	return common_write_callback_(frame);
}

void StreamDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	common_metadata_callback_(metadata);
}

void StreamDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	common_error_callback_(status);
}

bool StreamDecoder::test_respond(bool is_ogg)
{
	::FLAC__StreamDecoderInitStatus init_status;

	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	printf("testing init%s()... ", is_ogg? "_ogg":"");
	init_status = is_ogg? init_ogg() : init();
	if(init_status != ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return die_s_(0, this);
	printf("OK\n");

	current_metadata_number_ = 0;

	if(fseeko(file_, 0, SEEK_SET) < 0) {
		printf("FAILED rewinding input, errno = %d\n", errno);
		return false;
	}

	printf("testing process_until_end_of_stream()... ");
	if(!process_until_end_of_stream()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	printf("testing finish()... ");
	if(!finish()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	return true;
}

class FileDecoder : public FLAC::Decoder::File, public DecoderCommon {
public:
	FileDecoder(Layer layer): FLAC::Decoder::File(), DecoderCommon(layer) { }
	~FileDecoder() { }

	// from FLAC::Decoder::Stream
	::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
	void error_callback(::FLAC__StreamDecoderErrorStatus status);

	bool test_respond(bool is_ogg);
};

::FLAC__StreamDecoderWriteStatus FileDecoder::write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[])
{
	(void)buffer;
	return common_write_callback_(frame);
}

void FileDecoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	common_metadata_callback_(metadata);
}

void FileDecoder::error_callback(::FLAC__StreamDecoderErrorStatus status)
{
	common_error_callback_(status);
}

bool FileDecoder::test_respond(bool is_ogg)
{
	::FLAC__StreamDecoderInitStatus init_status;

	if(!set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	switch(layer_) {
		case LAYER_FILE:
			{
				printf("opening %sFLAC file... ", is_ogg? "Ogg ":"");
				FILE *file = ::fopen(flacfilename(is_ogg), "rb");
				if(0 == file) {
					printf("ERROR (%s)\n", strerror(errno));
					return false;
				}
				printf("OK\n");

				printf("testing init%s()... ", is_ogg? "_ogg":"");
				init_status = is_ogg? init_ogg(file) : init(file);
			}
			break;
		case LAYER_FILENAME:
			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg? init_ogg(flacfilename(is_ogg)) : init(flacfilename(is_ogg));
			break;
		default:
			die_("internal error 001");
			return false;
	}
	if(init_status != ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return die_s_(0, this);
	printf("OK\n");

	current_metadata_number_ = 0;

	printf("testing process_until_end_of_stream()... ");
	if(!process_until_end_of_stream()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	printf("testing finish()... ");
	if(!finish()) {
		State state = get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	return true;
}


static FLAC::Decoder::Stream *new_by_layer(Layer layer)
{
	if(layer < LAYER_FILE)
		return new StreamDecoder(layer);
	else
		return new FileDecoder(layer);
}

static bool test_stream_decoder(Layer layer, bool is_ogg)
{
	FLAC::Decoder::Stream *decoder;
	::FLAC__StreamDecoderInitStatus init_status;
	bool expect;

	printf("\n+++ libFLAC++ unit test: FLAC::Decoder::%s (layer: %s, format: %s)\n\n", layer<LAYER_FILE? "Stream":"File", LayerString[layer], is_ogg? "Ogg FLAC" : "FLAC");

	//
	// test new -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new_by_layer(layer);
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test new -> init -> delete
	//
	printf("allocating decoder instance... ");
	decoder = new_by_layer(layer);
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing init%s()... ", is_ogg? "_ogg":"");
	switch(layer) {
		case LAYER_STREAM:
		case LAYER_SEEKABLE_STREAM:
			dynamic_cast<StreamDecoder*>(decoder)->file_ = stdin;
			init_status = is_ogg? decoder->init_ogg() : decoder->init();
			break;
		case LAYER_FILE:
			init_status = is_ogg?
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init_ogg(stdin) :
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init(stdin);
			break;
		case LAYER_FILENAME:
			init_status = is_ogg?
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init_ogg(flacfilename(is_ogg)) :
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init(flacfilename(is_ogg));
			break;
		default:
			die_("internal error 006");
			return false;
	}
	if(init_status != ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return die_s_(0, decoder);
	printf("OK\n");

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	//
	// test normal usage
	//
	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;

	printf("allocating decoder instance... ");
	decoder = new_by_layer(layer);
	if(0 == decoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!decoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	if(is_ogg) {
		printf("testing set_ogg_serial_number()... ");
		if(!decoder->set_ogg_serial_number(file_utils__ogg_serial_number))
			return die_s_("returned false", decoder);
		printf("OK\n");
	}

	if(!decoder->set_md5_checking(true)) {
		printf("FAILED at set_md5_checking(), returned false\n");
		return false;
	}

	switch(layer) {
		case LAYER_STREAM:
		case LAYER_SEEKABLE_STREAM:
			printf("opening %sFLAC file... ", is_ogg? "Ogg ":"");
			dynamic_cast<StreamDecoder*>(decoder)->file_ = ::fopen(flacfilename(is_ogg), "rb");
			if(0 == dynamic_cast<StreamDecoder*>(decoder)->file_) {
				printf("ERROR (%s)\n", strerror(errno));
				return false;
			}
			printf("OK\n");

			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg? decoder->init_ogg() : decoder->init();
			break;
		case LAYER_FILE:
			{
				printf("opening FLAC file... ");
				FILE *file = ::fopen(flacfilename(is_ogg), "rb");
				if(0 == file) {
					printf("ERROR (%s)\n", strerror(errno));
					return false;
				}
				printf("OK\n");

				printf("testing init%s()... ", is_ogg? "_ogg":"");
				init_status = is_ogg?
					dynamic_cast<FLAC::Decoder::File*>(decoder)->init_ogg(file) :
					dynamic_cast<FLAC::Decoder::File*>(decoder)->init(file);
			}
			break;
		case LAYER_FILENAME:
			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg?
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init_ogg(flacfilename(is_ogg)) :
				dynamic_cast<FLAC::Decoder::File*>(decoder)->init(flacfilename(is_ogg));
			break;
		default:
			die_("internal error 009");
			return false;
	}
	if(init_status != ::FLAC__STREAM_DECODER_INIT_STATUS_OK)
		return die_s_(0, decoder);
	printf("OK\n");

	printf("testing get_state()... ");
	FLAC::Decoder::Stream::State state = decoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());

	dynamic_cast<DecoderCommon*>(decoder)->current_metadata_number_ = 0;
	dynamic_cast<DecoderCommon*>(decoder)->ignore_errors_ = false;
	dynamic_cast<DecoderCommon*>(decoder)->error_occurred_ = false;

	printf("testing get_md5_checking()... ");
	if(!decoder->get_md5_checking()) {
		printf("FAILED, returned false, expected true\n");
		return false;
	}
	printf("OK\n");

	printf("testing process_until_end_of_metadata()... ");
	if(!decoder->process_until_end_of_metadata())
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing process_single()... ");
	if(!decoder->process_single())
		return die_s_("returned false", decoder);
	printf("OK\n");

	printf("testing skip_single_frame()... ");
	if(!decoder->skip_single_frame())
		return die_s_("returned false", decoder);
	printf("OK\n");

	if(layer < LAYER_FILE) {
		printf("testing flush()... ");
		if(!decoder->flush())
			return die_s_("returned false", decoder);
		printf("OK\n");

		dynamic_cast<DecoderCommon*>(decoder)->ignore_errors_ = true;
		printf("testing process_single()... ");
		if(!decoder->process_single())
			return die_s_("returned false", decoder);
		printf("OK\n");
		dynamic_cast<DecoderCommon*>(decoder)->ignore_errors_ = false;
	}

	expect = (layer != LAYER_STREAM);
	printf("testing seek_absolute()... ");
	if(decoder->seek_absolute(0) != expect)
		return die_s_(expect? "returned false" : "returned true", decoder);
	printf("OK\n");

	printf("testing process_until_end_of_stream()... ");
	if(!decoder->process_until_end_of_stream())
		return die_s_("returned false", decoder);
	printf("OK\n");

	expect = (layer != LAYER_STREAM);
	printf("testing seek_absolute()... ");
	if(decoder->seek_absolute(0) != expect)
		return die_s_(expect? "returned false" : "returned true", decoder);
	printf("OK\n");

	printf("testing get_channels()... ");
	{
		unsigned channels = decoder->get_channels();
		if(channels != streaminfo_.data.stream_info.channels) {
			printf("FAILED, returned %u, expected %u\n", channels, streaminfo_.data.stream_info.channels);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	{
		unsigned bits_per_sample = decoder->get_bits_per_sample();
		if(bits_per_sample != streaminfo_.data.stream_info.bits_per_sample) {
			printf("FAILED, returned %u, expected %u\n", bits_per_sample, streaminfo_.data.stream_info.bits_per_sample);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	{
		unsigned sample_rate = decoder->get_sample_rate();
		if(sample_rate != streaminfo_.data.stream_info.sample_rate) {
			printf("FAILED, returned %u, expected %u\n", sample_rate, streaminfo_.data.stream_info.sample_rate);
			return false;
		}
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	{
		unsigned blocksize = decoder->get_blocksize();
		/* value could be anything since we're at the last block, so accept any reasonable answer */
		printf("returned %u... %s\n", blocksize, blocksize>0? "OK" : "FAILED");
		if(blocksize == 0)
			return false;
	}

	printf("testing get_channel_assignment()... ");
	{
		::FLAC__ChannelAssignment ca = decoder->get_channel_assignment();
		printf("returned %u (%s)... OK\n", (unsigned)ca, ::FLAC__ChannelAssignmentString[ca]);
	}

	if(layer < LAYER_FILE) {
		printf("testing reset()... ");
		if(!decoder->reset())
			return die_s_("returned false", decoder);
		printf("OK\n");

		if(layer == LAYER_STREAM) {
			/* after a reset() we have to rewind the input ourselves */
			printf("rewinding input... ");
			if(fseeko(dynamic_cast<StreamDecoder*>(decoder)->file_, 0, SEEK_SET) < 0) {
				printf("FAILED, errno = %d\n", errno);
				return false;
			}
			printf("OK\n");
		}

		dynamic_cast<DecoderCommon*>(decoder)->current_metadata_number_ = 0;

		printf("testing process_until_end_of_stream()... ");
		if(!decoder->process_until_end_of_stream())
			return die_s_("returned false", decoder);
		printf("OK\n");
	}

	printf("testing finish()... ");
	if(!decoder->finish()) {
		FLAC::Decoder::Stream::State state = decoder->get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	/*
	 * respond all
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	if(is_ogg) { /* encoder moves vorbis comment after streaminfo according to ogg mapping */
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application1_;
		expected_metadata_sequence_[num_expected_++] = &application2_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}
	else {
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application1_;
		expected_metadata_sequence_[num_expected_++] = &application2_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * respond all, ignore VORBIS_COMMENT
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &streaminfo_;
	expected_metadata_sequence_[num_expected_++] = &padding_;
	expected_metadata_sequence_[num_expected_++] = &seektable_;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;
	expected_metadata_sequence_[num_expected_++] = &cuesheet_;
	expected_metadata_sequence_[num_expected_++] = &picture_;
	expected_metadata_sequence_[num_expected_++] = &unknown_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * respond all, ignore APPLICATION
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	if(is_ogg) { /* encoder moves vorbis comment after streaminfo according to ogg mapping */
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}
	else {
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	if(is_ogg) { /* encoder moves vorbis comment after streaminfo according to ogg mapping */
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application2_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}
	else {
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application2_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * respond all, ignore APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #2)... ");
	if(!decoder->set_metadata_ignore_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	if(is_ogg) { /* encoder moves vorbis comment after streaminfo according to ogg mapping */
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}
	else {
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all, respond VORBIS_COMMENT
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(VORBIS_COMMENT)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all, respond APPLICATION
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all, respond APPLICATION id of app#1 & app#2
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #2)... ");
	if(!decoder->set_metadata_respond_application(application2_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application1_;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * respond all, ignore APPLICATION, respond APPLICATION id of app#1
	 */

	printf("testing set_metadata_respond_all()... ");
	if(!decoder->set_metadata_respond_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore(APPLICATION)... ");
	if(!decoder->set_metadata_ignore(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond_application(of app block #1)... ");
	if(!decoder->set_metadata_respond_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	if(is_ogg) { /* encoder moves vorbis comment after streaminfo according to ogg mapping */
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application1_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}
	else {
		expected_metadata_sequence_[num_expected_++] = &streaminfo_;
		expected_metadata_sequence_[num_expected_++] = &padding_;
		expected_metadata_sequence_[num_expected_++] = &seektable_;
		expected_metadata_sequence_[num_expected_++] = &application1_;
		expected_metadata_sequence_[num_expected_++] = &vorbiscomment_;
		expected_metadata_sequence_[num_expected_++] = &cuesheet_;
		expected_metadata_sequence_[num_expected_++] = &picture_;
		expected_metadata_sequence_[num_expected_++] = &unknown_;
	}

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	/*
	 * ignore all, respond APPLICATION, ignore APPLICATION id of app#1
	 */

	printf("testing set_metadata_ignore_all()... ");
	if(!decoder->set_metadata_ignore_all()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_respond(APPLICATION)... ");
	if(!decoder->set_metadata_respond(FLAC__METADATA_TYPE_APPLICATION)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	printf("testing set_metadata_ignore_application(of app block #1)... ");
	if(!decoder->set_metadata_ignore_application(application1_.data.application.id)) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	num_expected_ = 0;
	expected_metadata_sequence_[num_expected_++] = &application2_;

	if(!(layer < LAYER_FILE? dynamic_cast<StreamDecoder*>(decoder)->test_respond(is_ogg) : dynamic_cast<FileDecoder*>(decoder)->test_respond(is_ogg)))
		return false;

	if(layer < LAYER_FILE) /* for LAYER_FILE, FLAC__stream_decoder_finish() closes the file */
		::fclose(dynamic_cast<StreamDecoder*>(decoder)->file_);

	printf("freeing decoder instance... ");
	delete decoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

bool test_decoders()
{
	FLAC__bool is_ogg = false;

	while(1) {
		init_metadata_blocks_();

		if(!generate_file_(is_ogg))
			return false;

		if(!test_stream_decoder(LAYER_STREAM, is_ogg))
			return false;

		if(!test_stream_decoder(LAYER_SEEKABLE_STREAM, is_ogg))
			return false;

		if(!test_stream_decoder(LAYER_FILE, is_ogg))
			return false;

		if(!test_stream_decoder(LAYER_FILENAME, is_ogg))
			return false;

		(void) grabbag__file_remove_file(flacfilename(is_ogg));

		free_metadata_blocks_();

		if(!FLAC_API_SUPPORTS_OGG_FLAC || is_ogg)
			break;
		is_ogg = true;
	}

	return true;
}
