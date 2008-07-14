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
#include "FLAC/stream_encoder.h"
#include "test_libs_common/file_utils_flac.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> /* for stat() */

#ifdef min
#undef min
#endif
#define min(a,b) ((a)<(b)?(a):(b))

const long file_utils__ogg_serial_number = 12345;

#ifdef FLAC__VALGRIND_TESTING
static size_t local__fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if(!ferror(stream))
		fflush(stream);
	return ret;
}
#else
#define local__fwrite fwrite
#endif

typedef struct {
	FILE *file;
} encoder_client_struct;

static FLAC__StreamEncoderWriteStatus encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	encoder_client_struct *ecd = (encoder_client_struct*)client_data;

	(void)encoder, (void)samples, (void)current_frame;

	if(local__fwrite(buffer, 1, bytes, ecd->file) != bytes)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	else
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static void encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	(void)encoder, (void)metadata, (void)client_data;
}

FLAC__bool file_utils__generate_flacfile(FLAC__bool is_ogg, const char *output_filename, off_t *output_filesize, unsigned length, const FLAC__StreamMetadata *streaminfo, FLAC__StreamMetadata **metadata, unsigned num_metadata)
{
	FLAC__int32 samples[1024];
	FLAC__StreamEncoder *encoder;
	FLAC__StreamEncoderInitStatus init_status;
	encoder_client_struct encoder_client_data;
	unsigned i, n;

	FLAC__ASSERT(0 != output_filename);
	FLAC__ASSERT(0 != streaminfo);
	FLAC__ASSERT(streaminfo->type == FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__ASSERT((streaminfo->is_last && num_metadata == 0) || (!streaminfo->is_last && num_metadata > 0));

	if(0 == (encoder_client_data.file = fopen(output_filename, "wb")))
		return false;

	encoder = FLAC__stream_encoder_new();
	if(0 == encoder) {
		fclose(encoder_client_data.file);
		return false;
	}

	FLAC__stream_encoder_set_ogg_serial_number(encoder, file_utils__ogg_serial_number);
	FLAC__stream_encoder_set_verify(encoder, true);
	FLAC__stream_encoder_set_streamable_subset(encoder, true);
	FLAC__stream_encoder_set_do_mid_side_stereo(encoder, false);
	FLAC__stream_encoder_set_loose_mid_side_stereo(encoder, false);
	FLAC__stream_encoder_set_channels(encoder, streaminfo->data.stream_info.channels);
	FLAC__stream_encoder_set_bits_per_sample(encoder, streaminfo->data.stream_info.bits_per_sample);
	FLAC__stream_encoder_set_sample_rate(encoder, streaminfo->data.stream_info.sample_rate);
	FLAC__stream_encoder_set_blocksize(encoder, streaminfo->data.stream_info.min_blocksize);
	FLAC__stream_encoder_set_max_lpc_order(encoder, 0);
	FLAC__stream_encoder_set_qlp_coeff_precision(encoder, 0);
	FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder, false);
	FLAC__stream_encoder_set_do_escape_coding(encoder, false);
	FLAC__stream_encoder_set_do_exhaustive_model_search(encoder, false);
	FLAC__stream_encoder_set_min_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_max_residual_partition_order(encoder, 0);
	FLAC__stream_encoder_set_rice_parameter_search_dist(encoder, 0);
	FLAC__stream_encoder_set_total_samples_estimate(encoder, streaminfo->data.stream_info.total_samples);
	FLAC__stream_encoder_set_metadata(encoder, metadata, num_metadata);

	if(is_ogg)
		init_status = FLAC__stream_encoder_init_ogg_stream(encoder, /*read_callback=*/0, encoder_write_callback_, /*seek_callback=*/0, /*tell_callback=*/0, encoder_metadata_callback_, &encoder_client_data);
	else
		init_status = FLAC__stream_encoder_init_stream(encoder, encoder_write_callback_, /*seek_callback=*/0, /*tell_callback=*/0, encoder_metadata_callback_, &encoder_client_data);

	if(init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		fclose(encoder_client_data.file);
		return false;
	}

	/* init the dummy sample buffer */
	for(i = 0; i < sizeof(samples) / sizeof(FLAC__int32); i++)
		samples[i] = i & 7;

	while(length > 0) {
		n = min(length, sizeof(samples) / sizeof(FLAC__int32));

		if(!FLAC__stream_encoder_process_interleaved(encoder, samples, n)) {
			fclose(encoder_client_data.file);
			return false;
		}

		length -= n;
	}

	(void)FLAC__stream_encoder_finish(encoder);

	fclose(encoder_client_data.file);

	FLAC__stream_encoder_delete(encoder);

	if(0 != output_filesize) {
		struct stat filestats;

		if(stat(output_filename, &filestats) != 0)
			return false;
		else
			*output_filesize = filestats.st_size;
	}

	return true;
}
