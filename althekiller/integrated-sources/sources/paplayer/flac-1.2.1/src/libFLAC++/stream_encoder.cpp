/* libFLAC++ - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FLAC++/encoder.h"
#include "FLAC++/metadata.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace FLAC {
	namespace Encoder {

		// ------------------------------------------------------------
		//
		// Stream
		//
		// ------------------------------------------------------------

		Stream::Stream():
		encoder_(::FLAC__stream_encoder_new())
		{ }

		Stream::~Stream()
		{
			if(0 != encoder_) {
				(void)::FLAC__stream_encoder_finish(encoder_);
				::FLAC__stream_encoder_delete(encoder_);
			}
		}

		bool Stream::is_valid() const
		{
			return 0 != encoder_;
		}

		bool Stream::set_ogg_serial_number(long value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_ogg_serial_number(encoder_, value);
		}

		bool Stream::set_verify(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_verify(encoder_, value);
		}

		bool Stream::set_streamable_subset(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_streamable_subset(encoder_, value);
		}

		bool Stream::set_channels(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_channels(encoder_, value);
		}

		bool Stream::set_bits_per_sample(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_bits_per_sample(encoder_, value);
		}

		bool Stream::set_sample_rate(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_sample_rate(encoder_, value);
		}

		bool Stream::set_compression_level(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_compression_level(encoder_, value);
		}

		bool Stream::set_blocksize(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_blocksize(encoder_, value);
		}

		bool Stream::set_do_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_do_mid_side_stereo(encoder_, value);
		}

		bool Stream::set_loose_mid_side_stereo(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_loose_mid_side_stereo(encoder_, value);
		}

		bool Stream::set_apodization(const char *specification)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_apodization(encoder_, specification);
		}

		bool Stream::set_max_lpc_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_max_lpc_order(encoder_, value);
		}

		bool Stream::set_qlp_coeff_precision(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_qlp_coeff_precision(encoder_, value);
		}

		bool Stream::set_do_qlp_coeff_prec_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder_, value);
		}

		bool Stream::set_do_escape_coding(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_do_escape_coding(encoder_, value);
		}

		bool Stream::set_do_exhaustive_model_search(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_do_exhaustive_model_search(encoder_, value);
		}

		bool Stream::set_min_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_min_residual_partition_order(encoder_, value);
		}

		bool Stream::set_max_residual_partition_order(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_max_residual_partition_order(encoder_, value);
		}

		bool Stream::set_rice_parameter_search_dist(unsigned value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_rice_parameter_search_dist(encoder_, value);
		}

		bool Stream::set_total_samples_estimate(FLAC__uint64 value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_total_samples_estimate(encoder_, value);
		}

		bool Stream::set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_set_metadata(encoder_, metadata, num_blocks);
		}

		bool Stream::set_metadata(FLAC::Metadata::Prototype **metadata, unsigned num_blocks)
		{
			FLAC__ASSERT(is_valid());
#if (defined _MSC_VER) || (defined __BORLANDC__) || (defined __SUNPRO_CC)
			// MSVC++ can't handle:
			// ::FLAC__StreamMetadata *m[num_blocks];
			// so we do this ugly workaround
			::FLAC__StreamMetadata **m = new ::FLAC__StreamMetadata*[num_blocks];
#else
			::FLAC__StreamMetadata *m[num_blocks];
#endif
			for(unsigned i = 0; i < num_blocks; i++) {
				// we can get away with the const_cast since we know the encoder will only correct the is_last flags
				m[i] = const_cast< ::FLAC__StreamMetadata*>((const ::FLAC__StreamMetadata*)metadata[i]);
			}
#if (defined _MSC_VER) || (defined __BORLANDC__) || (defined __SUNPRO_CC)
			// complete the hack
			const bool ok = (bool)::FLAC__stream_encoder_set_metadata(encoder_, m, num_blocks);
			delete [] m;
			return ok;
#else
			return (bool)::FLAC__stream_encoder_set_metadata(encoder_, m, num_blocks);
#endif
		}

		Stream::State Stream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__stream_encoder_get_state(encoder_));
		}

		Decoder::Stream::State Stream::get_verify_decoder_state() const
		{
			FLAC__ASSERT(is_valid());
			return Decoder::Stream::State(::FLAC__stream_encoder_get_verify_decoder_state(encoder_));
		}

		void Stream::get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
		{
			FLAC__ASSERT(is_valid());
			::FLAC__stream_encoder_get_verify_decoder_error_stats(encoder_, absolute_sample, frame_number, channel, sample, expected, got);
		}

		bool Stream::get_verify() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_verify(encoder_);
		}

		bool Stream::get_streamable_subset() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_streamable_subset(encoder_);
		}

		bool Stream::get_do_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_do_mid_side_stereo(encoder_);
		}

		bool Stream::get_loose_mid_side_stereo() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_loose_mid_side_stereo(encoder_);
		}

		unsigned Stream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_channels(encoder_);
		}

		unsigned Stream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_bits_per_sample(encoder_);
		}

		unsigned Stream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_sample_rate(encoder_);
		}

		unsigned Stream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_blocksize(encoder_);
		}

		unsigned Stream::get_max_lpc_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_max_lpc_order(encoder_);
		}

		unsigned Stream::get_qlp_coeff_precision() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_qlp_coeff_precision(encoder_);
		}

		bool Stream::get_do_qlp_coeff_prec_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder_);
		}

		bool Stream::get_do_escape_coding() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_do_escape_coding(encoder_);
		}

		bool Stream::get_do_exhaustive_model_search() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_get_do_exhaustive_model_search(encoder_);
		}

		unsigned Stream::get_min_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_min_residual_partition_order(encoder_);
		}

		unsigned Stream::get_max_residual_partition_order() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_max_residual_partition_order(encoder_);
		}

		unsigned Stream::get_rice_parameter_search_dist() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_rice_parameter_search_dist(encoder_);
		}

		FLAC__uint64 Stream::get_total_samples_estimate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_get_total_samples_estimate(encoder_);
		}

		::FLAC__StreamEncoderInitStatus Stream::init()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_stream(encoder_, write_callback_, seek_callback_, tell_callback_, metadata_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus Stream::init_ogg()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_ogg_stream(encoder_, read_callback_, write_callback_, seek_callback_, tell_callback_, metadata_callback_, /*client_data=*/(void*)this);
		}

		bool Stream::finish()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_finish(encoder_);
		}

		bool Stream::process(const FLAC__int32 * const buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_process(encoder_, buffer, samples);
		}

		bool Stream::process_interleaved(const FLAC__int32 buffer[], unsigned samples)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_encoder_process_interleaved(encoder_, buffer, samples);
		}

		::FLAC__StreamEncoderReadStatus Stream::read_callback(FLAC__byte buffer[], size_t *bytes)
		{
			(void)buffer, (void)bytes;
			return ::FLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED;
		}

		::FLAC__StreamEncoderSeekStatus Stream::seek_callback(FLAC__uint64 absolute_byte_offset)
		{
			(void)absolute_byte_offset;
			return ::FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
		}

		::FLAC__StreamEncoderTellStatus Stream::tell_callback(FLAC__uint64 *absolute_byte_offset)
		{
			(void)absolute_byte_offset;
			return ::FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED;
		}

		void Stream::metadata_callback(const ::FLAC__StreamMetadata *metadata)
		{
			(void)metadata;
		}

		::FLAC__StreamEncoderReadStatus Stream::read_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

		::FLAC__StreamEncoderWriteStatus Stream::write_callback_(const ::FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(buffer, bytes, samples, current_frame);
		}

		::FLAC__StreamEncoderSeekStatus Stream::seek_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->seek_callback(absolute_byte_offset);
		}

		::FLAC__StreamEncoderTellStatus Stream::tell_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->tell_callback(absolute_byte_offset);
		}

		void Stream::metadata_callback_(const ::FLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		// ------------------------------------------------------------
		//
		// File
		//
		// ------------------------------------------------------------

		File::File():
			Stream()
		{ }

		File::~File()
		{
		}

		::FLAC__StreamEncoderInitStatus File::init(FILE *file)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_FILE(encoder_, file, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init(const char *filename)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_file(encoder_, filename, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init(const std::string &filename)
		{
			return init(filename.c_str());
		}

		::FLAC__StreamEncoderInitStatus File::init_ogg(FILE *file)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_ogg_FILE(encoder_, file, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init_ogg(const char *filename)
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_encoder_init_ogg_file(encoder_, filename, progress_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamEncoderInitStatus File::init_ogg(const std::string &filename)
		{
			return init_ogg(filename.c_str());
		}

		// This is a dummy to satisfy the pure virtual from Stream; the
		// read callback will never be called since we are initializing
		// with FLAC__stream_decoder_init_FILE() or
		// FLAC__stream_decoder_init_file() and those supply the read
		// callback internally.
		::FLAC__StreamEncoderWriteStatus File::write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame)
		{
			(void)buffer, (void)bytes, (void)samples, (void)current_frame;
			FLAC__ASSERT(false);
			return ::FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR; // double protection
		}

		void File::progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate)
		{
			(void)bytes_written, (void)samples_written, (void)frames_written, (void)total_frames_estimate;
		}

		void File::progress_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data)
		{
			(void)encoder;
			FLAC__ASSERT(0 != client_data);
			File *instance = reinterpret_cast<File *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->progress_callback(bytes_written, samples_written, frames_written, total_frames_estimate);
		}

	}
}
