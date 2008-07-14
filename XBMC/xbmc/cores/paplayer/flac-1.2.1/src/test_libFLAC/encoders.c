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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encoders.h"
#include "FLAC/assert.h"
#include "FLAC/stream_encoder.h"
#include "share/grabbag.h"
#include "test_libs_common/file_utils_flac.h"
#include "test_libs_common/metadata_utils.h"

typedef enum {
	LAYER_STREAM = 0, /* FLAC__stream_encoder_init_[ogg_]stream() without seeking */
	LAYER_SEEKABLE_STREAM, /* FLAC__stream_encoder_init_[ogg_]stream() with seeking */
	LAYER_FILE, /* FLAC__stream_encoder_init_[ogg_]FILE() */
	LAYER_FILENAME /* FLAC__stream_encoder_init_[ogg_]file() */
} Layer;

static const char * const LayerString[] = {
	"Stream",
	"Seekable Stream",
	"FILE*",
	"Filename"
};

static FLAC__StreamMetadata streaminfo_, padding_, seektable_, application1_, application2_, vorbiscomment_, cuesheet_, picture_, unknown_;
static FLAC__StreamMetadata *metadata_sequence_[] = { &vorbiscomment_, &padding_, &seektable_, &application1_, &application2_, &cuesheet_, &picture_, &unknown_ };
static const unsigned num_metadata_ = sizeof(metadata_sequence_) / sizeof(metadata_sequence_[0]);

static const char *flacfilename(FLAC__bool is_ogg)
{
	return is_ogg? "metadata.oga" : "metadata.flac";
}

static FLAC__bool die_(const char *msg)
{
	printf("ERROR: %s\n", msg);
	return false;
}

static FLAC__bool die_s_(const char *msg, const FLAC__StreamEncoder *encoder)
{
	FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(encoder);

	if(msg)
		printf("FAILED, %s", msg);
	else
		printf("FAILED");

	printf(", state = %u (%s)\n", (unsigned)state, FLAC__StreamEncoderStateString[state]);
	if(state == FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR) {
		FLAC__StreamDecoderState dstate = FLAC__stream_encoder_get_verify_decoder_state(encoder);
		printf("      verify decoder state = %u (%s)\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);
	}

	return false;
}

static void init_metadata_blocks_(void)
{
	mutils__init_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static void free_metadata_blocks_(void)
{
	mutils__free_metadata_blocks(&streaminfo_, &padding_, &seektable_, &application1_, &application2_, &vorbiscomment_, &cuesheet_, &picture_, &unknown_);
}

static FLAC__StreamEncoderReadStatus stream_encoder_read_callback_(const FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	FILE *f = (FILE*)client_data;
	(void)encoder;
	if(*bytes > 0) {
		*bytes = fread(buffer, sizeof(FLAC__byte), *bytes, f);
		if(ferror(f))
			return FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
		else if(*bytes == 0)
			return FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_ENCODER_READ_STATUS_ABORT;
}

static FLAC__StreamEncoderWriteStatus stream_encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	FILE *f = (FILE*)client_data;
	(void)encoder, (void)samples, (void)current_frame;
	if(fwrite(buffer, 1, bytes, f) != bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static FLAC__StreamEncoderSeekStatus stream_encoder_seek_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FILE *f = (FILE*)client_data;
	(void)encoder;
	if(fseek(f, (long)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

static FLAC__StreamEncoderTellStatus stream_encoder_tell_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FILE *f = (FILE*)client_data;
	long pos;
	(void)encoder;
	if((pos = ftell(f)) < 0)
		return FLAC__STREAM_ENCODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
	}
}

static void stream_encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)encoder, (void)metadata, (void)client_data;
}

static void stream_encoder_progress_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
{
	(void)encoder, (void)bytes_written, (void)samples_written, (void)frames_written, (void)total_frames_estimate, (void)client_data;
}

static FLAC__bool test_stream_encoder(Layer layer, FLAC__bool is_ogg)
{
	FLAC__StreamEncoder *encoder;
	FLAC__StreamEncoderInitStatus init_status;
	FLAC__StreamEncoderState state;
	FLAC__StreamDecoderState dstate;
	FILE *file = 0;
	FLAC__int32 samples[1024];
	FLAC__int32 *samples_array[1];
	unsigned i;

	samples_array[0] = samples;

	printf("\n+++ libFLAC unit test: FLAC__StreamEncoder (layer: %s, format: %s)\n\n", LayerString[layer], is_ogg? "Ogg FLAC":"FLAC");

	printf("testing FLAC__stream_encoder_new()... ");
	encoder = FLAC__stream_encoder_new();
	if(0 == encoder) {
		printf("FAILED, returned NULL\n");
		return false;
	}
	printf("OK\n");

	if(is_ogg) {
		printf("testing FLAC__stream_encoder_set_ogg_serial_number()... ");
		if(!FLAC__stream_encoder_set_ogg_serial_number(encoder, file_utils__ogg_serial_number))
			return die_s_("returned false", encoder);
		printf("OK\n");
	}

	printf("testing FLAC__stream_encoder_set_verify()... ");
	if(!FLAC__stream_encoder_set_verify(encoder, true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_streamable_subset()... ");
	if(!FLAC__stream_encoder_set_streamable_subset(encoder, true))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_channels()... ");
	if(!FLAC__stream_encoder_set_channels(encoder, streaminfo_.data.stream_info.channels))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_bits_per_sample()... ");
	if(!FLAC__stream_encoder_set_bits_per_sample(encoder, streaminfo_.data.stream_info.bits_per_sample))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_sample_rate()... ");
	if(!FLAC__stream_encoder_set_sample_rate(encoder, streaminfo_.data.stream_info.sample_rate))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_compression_level()... ");
	if(!FLAC__stream_encoder_set_compression_level(encoder, (unsigned)(-1)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_blocksize()... ");
	if(!FLAC__stream_encoder_set_blocksize(encoder, streaminfo_.data.stream_info.min_blocksize))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_mid_side_stereo()... ");
	if(!FLAC__stream_encoder_set_do_mid_side_stereo(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_loose_mid_side_stereo()... ");
	if(!FLAC__stream_encoder_set_loose_mid_side_stereo(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_max_lpc_order()... ");
	if(!FLAC__stream_encoder_set_max_lpc_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_qlp_coeff_precision()... ");
	if(!FLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_qlp_coeff_prec_search()... ");
	if(!FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_escape_coding()... ");
	if(!FLAC__stream_encoder_set_do_escape_coding(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_do_exhaustive_model_search()... ");
	if(!FLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_min_residual_partition_order()... ");
	if(!FLAC__stream_encoder_set_min_residual_partition_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_max_residual_partition_order()... ");
	if(!FLAC__stream_encoder_set_max_residual_partition_order(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_rice_parameter_search_dist()... ");
	if(!FLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_total_samples_estimate()... ");
	if(!FLAC__stream_encoder_set_total_samples_estimate(encoder, streaminfo_.data.stream_info.total_samples))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_set_metadata()... ");
	if(!FLAC__stream_encoder_set_metadata(encoder, metadata_sequence_, num_metadata_))
		return die_s_("returned false", encoder);
	printf("OK\n");

	if(layer < LAYER_FILENAME) {
		printf("opening file for FLAC output... ");
		file = fopen(flacfilename(is_ogg), "w+b");
		if(0 == file) {
			printf("ERROR (%s)\n", strerror(errno));
			return false;
		}
		printf("OK\n");
	}

	switch(layer) {
		case LAYER_STREAM:
			printf("testing FLAC__stream_encoder_init_%sstream()... ", is_ogg? "ogg_":"");
			init_status = is_ogg?
				FLAC__stream_encoder_init_ogg_stream(encoder, /*read_callback=*/0, stream_encoder_write_callback_, /*seek_callback=*/0, /*tell_callback=*/0, stream_encoder_metadata_callback_, /*client_data=*/file) :
				FLAC__stream_encoder_init_stream(encoder, stream_encoder_write_callback_, /*seek_callback=*/0, /*tell_callback=*/0, stream_encoder_metadata_callback_, /*client_data=*/file);
			break;
		case LAYER_SEEKABLE_STREAM:
			printf("testing FLAC__stream_encoder_init_%sstream()... ", is_ogg? "ogg_":"");
			init_status = is_ogg?
				FLAC__stream_encoder_init_ogg_stream(encoder, stream_encoder_read_callback_, stream_encoder_write_callback_, stream_encoder_seek_callback_, stream_encoder_tell_callback_, /*metadata_callback=*/0, /*client_data=*/file) :
				FLAC__stream_encoder_init_stream(encoder, stream_encoder_write_callback_, stream_encoder_seek_callback_, stream_encoder_tell_callback_, /*metadata_callback=*/0, /*client_data=*/file);
			break;
		case LAYER_FILE:
			printf("testing FLAC__stream_encoder_init_%sFILE()... ", is_ogg? "ogg_":"");
			init_status = is_ogg?
				FLAC__stream_encoder_init_ogg_FILE(encoder, file, stream_encoder_progress_callback_, /*client_data=*/0) :
				FLAC__stream_encoder_init_FILE(encoder, file, stream_encoder_progress_callback_, /*client_data=*/0);
			break;
		case LAYER_FILENAME:
			printf("testing FLAC__stream_encoder_init_%sfile()... ", is_ogg? "ogg_":"");
			init_status = is_ogg?
				FLAC__stream_encoder_init_ogg_file(encoder, flacfilename(is_ogg), stream_encoder_progress_callback_, /*client_data=*/0) :
				FLAC__stream_encoder_init_file(encoder, flacfilename(is_ogg), stream_encoder_progress_callback_, /*client_data=*/0);
			break;
		default:
			die_("internal error 001");
			return false;
	}
	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
		return die_s_(0, encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_state()... ");
	state = FLAC__stream_encoder_get_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)state, FLAC__StreamEncoderStateString[state]);

	printf("testing FLAC__stream_encoder_get_verify_decoder_state()... ");
	dstate = FLAC__stream_encoder_get_verify_decoder_state(encoder);
	printf("returned state = %u (%s)... OK\n", (unsigned)dstate, FLAC__StreamDecoderStateString[dstate]);

	{
		FLAC__uint64 absolute_sample;
		unsigned frame_number;
		unsigned channel;
		unsigned sample;
		FLAC__int32 expected;
		FLAC__int32 got;

		printf("testing FLAC__stream_encoder_get_verify_decoder_error_stats()... ");
		FLAC__stream_encoder_get_verify_decoder_error_stats(encoder, &absolute_sample, &frame_number, &channel, &sample, &expected, &got);
		printf("OK\n");
	}

	printf("testing FLAC__stream_encoder_get_verify()... ");
	if(FLAC__stream_encoder_get_verify(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_streamable_subset()... ");
	if(FLAC__stream_encoder_get_streamable_subset(encoder) != true) {
		printf("FAILED, expected true, got false\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_mid_side_stereo()... ");
	if(FLAC__stream_encoder_get_do_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_loose_mid_side_stereo()... ");
	if(FLAC__stream_encoder_get_loose_mid_side_stereo(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_channels()... ");
	if(FLAC__stream_encoder_get_channels(encoder) != streaminfo_.data.stream_info.channels) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.channels, FLAC__stream_encoder_get_channels(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_bits_per_sample()... ");
	if(FLAC__stream_encoder_get_bits_per_sample(encoder) != streaminfo_.data.stream_info.bits_per_sample) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.bits_per_sample, FLAC__stream_encoder_get_bits_per_sample(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_sample_rate()... ");
	if(FLAC__stream_encoder_get_sample_rate(encoder) != streaminfo_.data.stream_info.sample_rate) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.sample_rate, FLAC__stream_encoder_get_sample_rate(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_blocksize()... ");
	if(FLAC__stream_encoder_get_blocksize(encoder) != streaminfo_.data.stream_info.min_blocksize) {
		printf("FAILED, expected %u, got %u\n", streaminfo_.data.stream_info.min_blocksize, FLAC__stream_encoder_get_blocksize(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_max_lpc_order()... ");
	if(FLAC__stream_encoder_get_max_lpc_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_max_lpc_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_qlp_coeff_precision()... ");
	(void)FLAC__stream_encoder_get_qlp_coeff_precision(encoder);
	/* we asked the encoder to auto select this so we accept anything */
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_qlp_coeff_prec_search()... ");
	if(FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_escape_coding()... ");
	if(FLAC__stream_encoder_get_do_escape_coding(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_do_exhaustive_model_search()... ");
	if(FLAC__stream_encoder_get_do_exhaustive_model_search(encoder) != false) {
		printf("FAILED, expected false, got true\n");
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_min_residual_partition_order()... ");
	if(FLAC__stream_encoder_get_min_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_min_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_max_residual_partition_order()... ");
	if(FLAC__stream_encoder_get_max_residual_partition_order(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_max_residual_partition_order(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_rice_parameter_search_dist()... ");
	if(FLAC__stream_encoder_get_rice_parameter_search_dist(encoder) != 0) {
		printf("FAILED, expected %u, got %u\n", 0, FLAC__stream_encoder_get_rice_parameter_search_dist(encoder));
		return false;
	}
	printf("OK\n");

	printf("testing FLAC__stream_encoder_get_total_samples_estimate()... ");
	if(FLAC__stream_encoder_get_total_samples_estimate(encoder) != streaminfo_.data.stream_info.total_samples) {
#ifdef _MSC_VER
		printf("FAILED, expected %I64u, got %I64u\n", streaminfo_.data.stream_info.total_samples, FLAC__stream_encoder_get_total_samples_estimate(encoder));
#else
		printf("FAILED, expected %llu, got %llu\n", (unsigned long long)streaminfo_.data.stream_info.total_samples, (unsigned long long)FLAC__stream_encoder_get_total_samples_estimate(encoder));
#endif
		return false;
	}
	printf("OK\n");

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	printf("testing FLAC__stream_encoder_process()... ");
	if(!FLAC__stream_encoder_process(encoder, (const FLAC__int32 * const *)samples_array, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_process_interleaved()... ");
	if(!FLAC__stream_encoder_process_interleaved(encoder, samples, sizeof(samples) / sizeof(FLAC__int32)))
		return die_s_("returned false", encoder);
	printf("OK\n");

	printf("testing FLAC__stream_encoder_finish()... ");
	if(!FLAC__stream_encoder_finish(encoder))
		return die_s_("returned false", encoder);
	printf("OK\n");

	if(layer < LAYER_FILE)
		fclose(file);

	printf("testing FLAC__stream_encoder_delete()... ");
	FLAC__stream_encoder_delete(encoder);
	printf("OK\n");

	printf("\nPASSED!\n");

	return true;
}

FLAC__bool test_encoders(void)
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
