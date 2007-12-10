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

#include "encoders.h"
#include "FLAC/assert.h"
#include "FLAC++/encoder.h"
#include "share/grabbag.h"
extern "C" {
#include "test_libs_common/file_utils_flac.h"
#include "test_libs_common/metadata_utils.h"
}
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	LAYER_STREAM = 0, /* FLAC__stream_encoder_init_stream() without seeking */
	LAYER_SEEKABLE_STREAM, /* FLAC__stream_encoder_init_stream() with seeking */
	LAYER_FILE, /* FLAC__stream_encoder_init_FILE() */
	LAYER_FILENAME /* FLAC__stream_encoder_init_file() */
} Layer;

static const char * const LayerString[] = {
	"Stream",
	"Seekable Stream",
	"FILE*",
	"Filename"
};

static ::FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, picture_, unknown_;
static ::FLAC__StreamMetadata *metadata_sequence_[] = { &vorbiscomment_, &padding_, &seektable_, &application1_, &application2_, &cuesheet_, &picture_, &unknown_ };
static const unsigned num_metadata_ = sizeof(metadata_sequence_) / sizeof(metadata_sequence_[0]);

static const char *flacfilename(bool is_ogg)
{
	return is_ogg? "metadata.oga" : "metadata.flac";
}

static bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static bool die_s_(const char *msg, const FLAC::Encoder::Stream *encoder)
{
	FLAC::Encoder::Stream::State state = encoder->get_state();

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)((::FLAC__StreamEncoderState)state), state.as_cstring());
	if(state == ::FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
		FLAC::Decoder::Stream::State dstate = encoder->get_verify_decoder_state();
		printf("      verify decoder state = %u (%s)\n", (unsigned)((::FLAC__StreamDecoderState)dstate), dstate.as_cstring());
	}

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

class StreamEncoder : public FLAC::Encoder::Stream {
public:
	Layer layer_;
	FILE *file_;

	StreamEncoder(Layer layer): FLAC::Encoder::Stream(), layer_(layer), file_(0) { }
	~StreamEncoder() { }

	// from FLAC::Encoder::Stream
	::FLAC__StreamEncoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
	::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame);
	::FLAC__StreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
	::FLAC__StreamEncoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
	void metadata_callback(const ::FLAC__StreamMetadata *metadata);
};

::FLAC__StreamEncoderReadStatus StreamEncoder::read_callback(FLAC__byte buffer[], size_t *bytes)
{
	if(*bytes > 0) {
		*bytes = fread(buffer, sizeof(FLAC__byte), *bytes, file_);
		if(ferror(file_))
			return ::FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
		else if(*bytes == 0)
			return ::FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
		else
			return ::FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
	}
	else
		return ::FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
}

::FLAC__StreamEncoderWriteStatus StreamEncoder::write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)
{
	(void)samples, (void)current_frame;

	if(fwrite(buffer, 1, bytes, file_) != bytes)
		return ::FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	else
		return ::FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

::FLAC__StreamEncoderSeekStatus StreamEncoder::seek_callback(FLAC__uint64 absolute_byte_offset)
{
	if(layer_==LAYER_STREAM)
		return ::FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
	else if(fseek(file_, (long)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

::FLAC__StreamEncoderTellStatus StreamEncoder::tell_callback(FLAC__uint64 *absolute_byte_offset)
{
	long pos;
	if(layer_==LAYER_STREAM)
		return ::FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED;
	else if((pos = ftell(file_)) < 0)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
	}
}

void StreamEncoder::metadata_callback(const ::FLAC__StreamMetadata *metadata)
{
	(void)metadata;
}

class FileEncoder : public FLAC::Encoder::File {
public:
	Layer layer_;

	FileEncoder(Layer layer): FLAC::Encoder::File(), layer_(layer) { }
	~FileEncoder() { }

	// from FLAC::Encoder::File
	void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);
};

void FileEncoder::progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)
{
	(void)bytes_written, (void)samples_written, (void)frames_written, (void)total_frames_estimate;
}

static FLAC::Encoder::Stream *new_by_layer(Layer layer)
{
	if(layer < LAYER_FILE)
		return new StreamEncoder(layer);
	else
		return new FileEncoder(layer);
}

static bool test_stream_encoder(Layer layer, bool is_ogg)
{
	FLAC::Encoder::Stream *encoder;
	::FLAC__StreamEncoderInitStatus init_status;
	FILE *file = 0;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1] = { samples };
	unsigned i;

	printf("\n+++ libFLAC++ unit test: FLAC::Encoder::%s (layer: %s, format: %s)\n\n", layer<LAYER_FILE? "Stream":"File", LayerString[layer], is_ogg? "Ogg FLAC":"FLAC");

	printf("allocating encoder instance... ");
	encoder = new_by_layer(layer);
	if(0 == encoder) {
		printf("FAILED, new returned NULL\n");
		return false;
	}
	printf("OK\n");

	printf("testing is_valid()... ");
	if(!encoder->is_valid()) {
		printf("FAILED, returned false\n");
		return false;
	}
	printf("OK\n");

	if(is_ogg) {
		printf("testing set_ogg_serial_number()... ");
		if(!encoder->set_ogg_serial_number(file_utils__ogg_serial_number))
			return die_s_("returned false", encoder);
		printf("OK\n");
	}

	printf("testing set_verify()... ");
	if(!encoder->set_verify(true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_streamable_subset()... ");
	if(!encoder->set_streamable_subset(true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_channels()... ");
	if(!encoder->set_channels(streaminfo_.data.stream_info.channels))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_bits_per_sample()... ");
	if(!encoder->set_bits_per_sample(streaminfo_.data.stream_info.bits_per_sample))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_sample_rate()... ");
	if(!encoder->set_sample_rate(streaminfo_.data.stream_info.sample_rate))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_compression_level()... ");
	if(!encoder->set_compression_level((unsigned)(-1)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_blocksize()... ");
	if(!encoder->set_blocksize(streaminfo_.data.stream_info.min_blocksize))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_do_mid_side_stereo()... ");
	if(!encoder->set_do_mid_side_stereo(false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_loose_mid_side_stereo()... ");
	if(!encoder->set_loose_mid_side_stereo(false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_max_lpc_order()... ");
	if(!encoder->set_max_lpc_order(0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_qlp_coeff_precision()... ");
	if(!encoder->set_qlp_coeff_precision(0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_do_qlp_coeff_prec_search()... ");
	if(!encoder->set_do_qlp_coeff_prec_search(false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_do_escape_coding()... ");
	if(!encoder->set_do_escape_coding(false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_do_exhaustive_model_search()... ");
	if(!encoder->set_do_exhaustive_model_search(false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_min_residual_partition_order()... ");
	if(!encoder->set_min_residual_partition_order(0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_max_residual_partition_order()... ");
	if(!encoder->set_max_residual_partition_order(0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_rice_parameter_search_dist()... ");
	if(!encoder->set_rice_parameter_search_dist(0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_total_samples_estimate()... ");
	if(!encoder->set_total_samples_estimate(streaminfo_.data.stream_info.total_samples))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing set_metadata()... ");
	if(!encoder->set_metadata(metadata_sequence_, num_metadata_))
		return die_s_("returned false", encoder);
	printf("OK\n");

	if(layer < LAYER_FILENAME) {
		printf("opening file for FLAC output... ");
		file = ::fopen(flacfilename(is_ogg), "w+b");
		if(0 == file) {
			printf("ERROR (%s)\n", strerror(errno));
			return false;
		}
		printf("OK\n");
		if(layer < LAYER_FILE)
			dynamic_cast<StreamEncoder*>(encoder)->file_ = file;
	}

	switch(layer) {
		case LAYER_STREAM:
		case LAYER_SEEKABLE_STREAM:
			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg? encoder->init_ogg() : encoder->init();
			break;
		case LAYER_FILE:
			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg?
				dynamic_cast<FLAC::Encoder::File*>(encoder)->init_ogg(file) :
				dynamic_cast<FLAC::Encoder::File*>(encoder)->init(file);
			break;
		case LAYER_FILENAME:
			printf("testing init%s()... ", is_ogg? "_ogg":"");
			init_status = is_ogg?
				dynamic_cast<FLAC::Encoder::File*>(encoder)->init_ogg(flacfilename(is_ogg)) :
				dynamic_cast<FLAC::Encoder::File*>(encoder)->init(flacfilename(is_ogg));
			break;
		default:
			die_("internal error 001");
			return false;
	}
	if(init_status != ::FLAC__STREAM_ENCODER_INIT_STATUS_OK)
		return die_s_(0, encoder);
	printf("OK\n");

	printf("testing get_state()... ");
	FLAC::Encoder::Stream::State state = encoder->get_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamEncoderState)state), state.as_cstring());

	printf("testing get_verify_decoder_state()... ");
	FLAC::Decoder::Stream::State dstate = encoder->get_verify_decoder_state();
	printf("returned state = %u (%s)... OK\n", (unsigned)((::FLAC__StreamDecoderState)dstate), dstate.as_cstring());

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing get_verify_decoder_error_stats()... ");
		encoder->get_verify_decoder_error_stats(&absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing get_verify()... ");
	if(encoder->get_verify() != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_streamable_subset()... ");
	if(encoder->get_streamable_subset() != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_mid_side_stereo()... ");
	if(encoder->get_do_mid_side_stereo() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_loose_mid_side_stereo()... ");
	if(encoder->get_loose_mid_side_stereo() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_channels()... ");
	if(encoder->get_channels() != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, encoder->get_channels());
		return false;
	}
	printf("OK\n");

	printf("testing get_bits_per_sample()... ");
	if(encoder->get_bits_per_sample() != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, encoder->get_bits_per_sample());
		return false;
	}
	printf("OK\n");

	printf("testing get_sample_rate()... ");
	if(encoder->get_sample_rate() != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, encoder->get_sample_rate());
		return false;
	}
	printf("OK\n");

	printf("testing get_blocksize()... ");
	if(encoder->get_blocksize() != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, encoder->get_blocksize());
		return false;
	}
	printf("OK\n");

	printf("testing get_max_lpc_order()... ");
	if(encoder->get_max_lpc_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_max_lpc_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_qlp_coeff_precision()... ");
	(void)encoder->get_qlp_coeff_precision();
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing get_do_qlp_coeff_prec_search()... ");
	if(encoder->get_do_qlp_coeff_prec_search() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_escape_coding()... ");
	if(encoder->get_do_escape_coding() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_do_exhaustive_model_search()... ");
	if(encoder->get_do_exhaustive_model_search() != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing get_min_residual_partition_order()... ");
	if(encoder->get_min_residual_partition_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_min_residual_partition_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_max_residual_partition_order()... ");
	if(encoder->get_max_residual_partition_order() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_max_residual_partition_order());
		return false;
	}
	printf("OK\n");

	printf("testing get_rice_parameter_search_dist()... ");
	if(encoder->get_rice_parameter_search_dist() != 0) {
		printf("FAILED, expected %u, got %u\n", 0, encoder->get_rice_parameter_search_dist());
		return false;
	}
	printf("OK\n");

	printf("testing get_total_samples_estimate()... ");
	if(encoder->get_total_samples_estimate() != streaminfo_.data.stream_info.total_samples) {
#ifdef _MSC_VER
		printf("FAILED, expected %I64u, got %I64u\n", streaminfo_.data.stream_info.total_samples, encoder->get_total_samples_estimate());
#else
		printf("FAILED, expected %llu, got %llu\n", (unsigned long long)streaminfo_.data.stream_info.total_samples, (unsigned long long)encoder->get_total_samples_estimate());
#endif
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing process()... ");
	if(!encoder->process(samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing process_interleaved()... ");
	if(!encoder->process_interleaved(samples, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing finish()... ");
	if(!encoder->finish()) {
		FLAC::Encoder::Stream::State state = encoder->get_state();
		printf("FAILED, returned false, state = %u (%s)\n", (unsigned)((::FLAC__StreamEncoderState)state), state.as_cstring());
		return false;
	}
	printf("OK\n");

	if(layer < LAYER_FILE)
		::fclose(dynamic_cast<StreamEncoder*>(encoder)->file_);

	printf("freeing encoder instance... ");
	delete encoder;
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

bool test_encoders()
{
	FLAC__bool is_ogg = false;

	while(1) {
		init_metadata_blocks_();

		if(!test_stream_encoder(LAYER_STREAM, is_ogg))
			return false;

		if(!test_stream_encoder(LAYER_SEEKABLE_STREAM, is_ogg))
			return false;

		if(!test_stream_encoder(LAYER_FILE, is_ogg))
			return false;

		if(!test_stream_encoder(LAYER_FILENAME, is_ogg))
			return false;

		(void) grabbag__file_remove_file(flacfilename(is_ogg));

		free_metadata_blocks_();

		if(!FLAC_API_SUPPORTS_OGG_FLAC || is_ogg)
			break;
		is_ogg = true;
	}

	return true;
}
