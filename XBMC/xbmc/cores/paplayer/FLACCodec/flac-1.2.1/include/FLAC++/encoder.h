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

#ifndef FLACPP__ENCODER_H
#define FLACPP__ENCODER_H

#include "export.h"

#include "FLAC/stream_encoder.h"
#include "decoder.h"
#include "metadata.h"


/** \file include/FLAC++/encoder.h
 *
 *  \brief
 *  This module contains the classes which implement the various
 *  encoders.
 *
 *  See the detailed documentation in the
 *  \link flacpp_encoder encoder \endlink module.
 */

/** \defgroup flacpp_encoder FLAC++/encoder.h: encoder classes
 *  \ingroup flacpp
 *
 *  \brief
 *  This module describes the encoder layers provided by libFLAC++.
 *
 * The libFLAC++ encoder classes are object wrappers around their
 * counterparts in libFLAC.  All encoding layers available in
 * libFLAC are also provided here.  The interface is very similar;
 * make sure to read the \link flac_encoder libFLAC encoder module \endlink.
 *
 * There are only two significant differences here.  First, instead of
 * passing in C function pointers for callbacks, you inherit from the
 * encoder class and provide implementations for the callbacks in your
 * derived class; because of this there is no need for a 'client_data'
 * property.
 *
 * Second, there are two stream encoder classes.  FLAC::Encoder::Stream
 * is used for the same cases that FLAC__stream_encoder_init_stream() /
 * FLAC__stream_encoder_init_ogg_stream() are used, and FLAC::Encoder::File
 * is used for the same cases that
 * FLAC__stream_encoder_init_FILE() and FLAC__stream_encoder_init_file() /
 * FLAC__stream_encoder_init_ogg_FILE() and FLAC__stream_encoder_init_ogg_file()
 * are used.
 */

namespace FLAC {
	namespace Encoder {

		/** \ingroup flacpp_encoder
		 *  \brief
		 *  This class wraps the ::FLAC__StreamEncoder.  If you are
		 *  encoding to a file, FLAC::Encoder::File may be more
		 *  convenient.
		 *
		 * The usage of this class is similar to FLAC__StreamEncoder,
		 * except instead of providing callbacks to
		 * FLAC__stream_encoder_init*_stream(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call init() or init_ogg().  The rest of
		 * the calls work the same as in the C layer.
		 *
		 * Only the write callback is mandatory.  The others are
		 * optional; this class provides default implementations that do
		 * nothing.  In order for some STREAMINFO and SEEKTABLE data to
		 * be written properly, you must overide seek_callback() and
		 * tell_callback(); see FLAC__stream_encoder_init_stream() as to
		 * why.
		 */
		class FLACPP_API Stream {
		public:
			/** This class is a wrapper around FLAC__StreamEncoderState.
			 */
			class FLACPP_API State {
			public:
				inline State(::FLAC__StreamEncoderState state): state_(state) { }
				inline operator ::FLAC__StreamEncoderState() const { return state_; }
				inline const char *as_cstring() const { return ::FLAC__StreamEncoderStateString[state_]; }
				inline const char *resolved_as_cstring(const Stream &encoder) const { return ::FLAC__stream_encoder_get_resolved_state_string(encoder.encoder_); }
			protected:
				::FLAC__StreamEncoderState state_;
			};

			Stream();
			virtual ~Stream();

			//@{
			/** Call after construction to check the that the object was created
			 *  successfully.  If not, use get_state() to find out why not.
			 *
			 */
			virtual bool is_valid() const;
			inline operator bool() const { return is_valid(); } ///< See is_valid()
			//@}

			virtual bool set_ogg_serial_number(long value);                 ///< See FLAC__stream_encoder_set_ogg_serial_number()
			virtual bool set_verify(bool value);                            ///< See FLAC__stream_encoder_set_verify()
			virtual bool set_streamable_subset(bool value);                 ///< See FLAC__stream_encoder_set_streamable_subset()
			virtual bool set_channels(unsigned value);                      ///< See FLAC__stream_encoder_set_channels()
			virtual bool set_bits_per_sample(unsigned value);               ///< See FLAC__stream_encoder_set_bits_per_sample()
			virtual bool set_sample_rate(unsigned value);                   ///< See FLAC__stream_encoder_set_sample_rate()
			virtual bool set_compression_level(unsigned value);             ///< See FLAC__stream_encoder_set_compression_level()
			virtual bool set_blocksize(unsigned value);                     ///< See FLAC__stream_encoder_set_blocksize()
			virtual bool set_do_mid_side_stereo(bool value);                ///< See FLAC__stream_encoder_set_do_mid_side_stereo()
			virtual bool set_loose_mid_side_stereo(bool value);             ///< See FLAC__stream_encoder_set_loose_mid_side_stereo()
			virtual bool set_apodization(const char *specification);        ///< See FLAC__stream_encoder_set_apodization()
			virtual bool set_max_lpc_order(unsigned value);                 ///< See FLAC__stream_encoder_set_max_lpc_order()
			virtual bool set_qlp_coeff_precision(unsigned value);           ///< See FLAC__stream_encoder_set_qlp_coeff_precision()
			virtual bool set_do_qlp_coeff_prec_search(bool value);          ///< See FLAC__stream_encoder_set_do_qlp_coeff_prec_search()
			virtual bool set_do_escape_coding(bool value);                  ///< See FLAC__stream_encoder_set_do_escape_coding()
			virtual bool set_do_exhaustive_model_search(bool value);        ///< See FLAC__stream_encoder_set_do_exhaustive_model_search()
			virtual bool set_min_residual_partition_order(unsigned value);  ///< See FLAC__stream_encoder_set_min_residual_partition_order()
			virtual bool set_max_residual_partition_order(unsigned value);  ///< See FLAC__stream_encoder_set_max_residual_partition_order()
			virtual bool set_rice_parameter_search_dist(unsigned value);    ///< See FLAC__stream_encoder_set_rice_parameter_search_dist()
			virtual bool set_total_samples_estimate(FLAC__uint64 value);    ///< See FLAC__stream_encoder_set_total_samples_estimate()
			virtual bool set_metadata(::FLAC__StreamMetadata **metadata, unsigned num_blocks);    ///< See FLAC__stream_encoder_set_metadata()
			virtual bool set_metadata(FLAC::Metadata::Prototype **metadata, unsigned num_blocks); ///< See FLAC__stream_encoder_set_metadata()

			/* get_state() is not virtual since we want subclasses to be able to return their own state */
			State get_state() const;                                   ///< See FLAC__stream_encoder_get_state()
			virtual Decoder::Stream::State get_verify_decoder_state() const; ///< See FLAC__stream_encoder_get_verify_decoder_state()
			virtual void get_verify_decoder_error_stats(FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got); ///< See FLAC__stream_encoder_get_verify_decoder_error_stats()
			virtual bool     get_verify() const;                       ///< See FLAC__stream_encoder_get_verify()
			virtual bool     get_streamable_subset() const;            ///< See FLAC__stream_encoder_get_streamable_subset()
			virtual bool     get_do_mid_side_stereo() const;           ///< See FLAC__stream_encoder_get_do_mid_side_stereo()
			virtual bool     get_loose_mid_side_stereo() const;        ///< See FLAC__stream_encoder_get_loose_mid_side_stereo()
			virtual unsigned get_channels() const;                     ///< See FLAC__stream_encoder_get_channels()
			virtual unsigned get_bits_per_sample() const;              ///< See FLAC__stream_encoder_get_bits_per_sample()
			virtual unsigned get_sample_rate() const;                  ///< See FLAC__stream_encoder_get_sample_rate()
			virtual unsigned get_blocksize() const;                    ///< See FLAC__stream_encoder_get_blocksize()
			virtual unsigned get_max_lpc_order() const;                ///< See FLAC__stream_encoder_get_max_lpc_order()
			virtual unsigned get_qlp_coeff_precision() const;          ///< See FLAC__stream_encoder_get_qlp_coeff_precision()
			virtual bool     get_do_qlp_coeff_prec_search() const;     ///< See FLAC__stream_encoder_get_do_qlp_coeff_prec_search()
			virtual bool     get_do_escape_coding() const;             ///< See FLAC__stream_encoder_get_do_escape_coding()
			virtual bool     get_do_exhaustive_model_search() const;   ///< See FLAC__stream_encoder_get_do_exhaustive_model_search()
			virtual unsigned get_min_residual_partition_order() const; ///< See FLAC__stream_encoder_get_min_residual_partition_order()
			virtual unsigned get_max_residual_partition_order() const; ///< See FLAC__stream_encoder_get_max_residual_partition_order()
			virtual unsigned get_rice_parameter_search_dist() const;   ///< See FLAC__stream_encoder_get_rice_parameter_search_dist()
			virtual FLAC__uint64 get_total_samples_estimate() const;   ///< See FLAC__stream_encoder_get_total_samples_estimate()

			virtual ::FLAC__StreamEncoderInitStatus init();            ///< See FLAC__stream_encoder_init_stream()
			virtual ::FLAC__StreamEncoderInitStatus init_ogg();        ///< See FLAC__stream_encoder_init_ogg_stream()

			virtual bool finish(); ///< See FLAC__stream_encoder_finish()

			virtual bool process(const FLAC__int32 * const buffer[], unsigned samples);     ///< See FLAC__stream_encoder_process()
			virtual bool process_interleaved(const FLAC__int32 buffer[], unsigned samples); ///< See FLAC__stream_encoder_process_interleaved()
		protected:
			/// See FLAC__StreamEncoderReadCallback
			virtual ::FLAC__StreamEncoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);

			/// See FLAC__StreamEncoderWriteCallback
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame) = 0;

			/// See FLAC__StreamEncoderSeekCallback
			virtual ::FLAC__StreamEncoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);

			/// See FLAC__StreamEncoderTellCallback
			virtual ::FLAC__StreamEncoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);

			/// See FLAC__StreamEncoderMetadataCallback
			virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);

#if (defined _MSC_VER) || (defined __BORLANDC__) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected encoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			::FLAC__StreamEncoder *encoder_;

			static ::FLAC__StreamEncoderReadStatus read_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
			static ::FLAC__StreamEncoderWriteStatus write_callback_(const ::FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, void *client_data);
			static ::FLAC__StreamEncoderSeekStatus seek_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);
			static ::FLAC__StreamEncoderTellStatus tell_callback_(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
			static void metadata_callback_(const ::FLAC__StreamEncoder *encoder, const ::FLAC__StreamMetadata *metadata, void *client_data);
		private:
			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/** \ingroup flacpp_encoder
		 *  \brief
		 *  This class wraps the ::FLAC__StreamEncoder.  If you are
		 *  not encoding to a file, you may need to use
		 *  FLAC::Encoder::Stream.
		 *
		 * The usage of this class is similar to FLAC__StreamEncoder,
		 * except instead of providing callbacks to
		 * FLAC__stream_encoder_init*_FILE() or
		 * FLAC__stream_encoder_init*_file(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call init() or init_ogg().  The rest
		 * of the calls work the same as in the C layer.
		 *
		 * There are no mandatory callbacks; all the callbacks from
		 * FLAC::Encoder::Stream are implemented here fully and support
		 * full post-encode STREAMINFO and SEEKTABLE updating.  There is
		 * only an optional progress callback which you may override to
		 * get periodic reports on the progress of the encode.
		 */
		class FLACPP_API File: public Stream {
		public:
			File();
			virtual ~File();

			virtual ::FLAC__StreamEncoderInitStatus init(FILE *file);                      ///< See FLAC__stream_encoder_init_FILE()
			virtual ::FLAC__StreamEncoderInitStatus init(const char *filename);            ///< See FLAC__stream_encoder_init_file()
			virtual ::FLAC__StreamEncoderInitStatus init(const std::string &filename);     ///< See FLAC__stream_encoder_init_file()
			virtual ::FLAC__StreamEncoderInitStatus init_ogg(FILE *file);                  ///< See FLAC__stream_encoder_init_ogg_FILE()
			virtual ::FLAC__StreamEncoderInitStatus init_ogg(const char *filename);        ///< See FLAC__stream_encoder_init_ogg_file()
			virtual ::FLAC__StreamEncoderInitStatus init_ogg(const std::string &filename); ///< See FLAC__stream_encoder_init_ogg_file()
		protected:
			/// See FLAC__StreamEncoderProgressCallback
			virtual void progress_callback(FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate);

			/// This is a dummy implementation to satisfy the pure virtual in Stream that is actually supplied internally by the C layer
			virtual ::FLAC__StreamEncoderWriteStatus write_callback(const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame);
		private:
			static void progress_callback_(const ::FLAC__StreamEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);

			// Private and undefined so you can't use them:
			File(const Stream &);
			void operator=(const Stream &);
		};

	}
}

#endif
