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

#ifndef FLACPP__DECODER_H
#define FLACPP__DECODER_H

#include "export.h"

#include <string>
#include "FLAC/stream_decoder.h"


/** \file include/FLAC++/decoder.h
 *
 *  \brief
 *  This module contains the classes which implement the various
 *  decoders.
 *
 *  See the detailed documentation in the
 *  \link flacpp_decoder decoder \endlink module.
 */

/** \defgroup flacpp_decoder FLAC++/decoder.h: decoder classes
 *  \ingroup flacpp
 *
 *  \brief
 *  This module describes the decoder layers provided by libFLAC++.
 *
 * The libFLAC++ decoder classes are object wrappers around their
 * counterparts in libFLAC.  All decoding layers available in
 * libFLAC are also provided here.  The interface is very similar;
 * make sure to read the \link flac_decoder libFLAC decoder module \endlink.
 *
 * There are only two significant differences here.  First, instead of
 * passing in C function pointers for callbacks, you inherit from the
 * decoder class and provide implementations for the callbacks in your
 * derived class; because of this there is no need for a 'client_data'
 * property.
 *
 * Second, there are two stream decoder classes.  FLAC::Decoder::Stream
 * is used for the same cases that FLAC__stream_decoder_init_stream() /
 * FLAC__stream_decoder_init_ogg_stream() are used, and FLAC::Decoder::File
 * is used for the same cases that
 * FLAC__stream_decoder_init_FILE() and FLAC__stream_decoder_init_file() /
 * FLAC__stream_decoder_init_ogg_FILE() and FLAC__stream_decoder_init_ogg_file()
 * are used.
 */

namespace FLAC {
	namespace Decoder {

		/** \ingroup flacpp_decoder
		 *  \brief
		 *  This class wraps the ::FLAC__StreamDecoder.  If you are
		 *  decoding from a file, FLAC::Decoder::File may be more
		 *  convenient.
		 *
		 * The usage of this class is similar to FLAC__StreamDecoder,
		 * except instead of providing callbacks to
		 * FLAC__stream_decoder_init*_stream(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call init() or init_ogg().  The rest
		 * of the calls work the same as in the C layer.
		 *
		 * Only the read, write, and error callbacks are mandatory.  The
		 * others are optional; this class provides default
		 * implementations that do nothing.  In order for seeking to work
		 * you must overide seek_callback(), tell_callback(),
		 * length_callback(), and eof_callback().
		 */
		class FLACPP_API Stream {
		public:
			/** This class is a wrapper around FLAC__StreamDecoderState.
			 */
			class FLACPP_API State {
			public:
				inline State(::FLAC__StreamDecoderState state): state_(state) { }
				inline operator ::FLAC__StreamDecoderState() const { return state_; }
				inline const char *as_cstring() const { return ::FLAC__StreamDecoderStateString[state_]; }
				inline const char *resolved_as_cstring(const Stream &decoder) const { return ::FLAC__stream_decoder_get_resolved_state_string(decoder.decoder_); }
			protected:
				::FLAC__StreamDecoderState state_;
			};

			Stream();
			virtual ~Stream();

			//@{
			/** Call after construction to check the that the object was created
			 *  successfully.  If not, use get_state() to find out why not.
			 */
			virtual bool is_valid() const;
			inline operator bool() const { return is_valid(); } ///< See is_valid()
			//@}

			virtual bool set_ogg_serial_number(long value);                        ///< See FLAC__stream_decoder_set_ogg_serial_number()
			virtual bool set_md5_checking(bool value);                             ///< See FLAC__stream_decoder_set_md5_checking()
			virtual bool set_metadata_respond(::FLAC__MetadataType type);          ///< See FLAC__stream_decoder_set_metadata_respond()
			virtual bool set_metadata_respond_application(const FLAC__byte id[4]); ///< See FLAC__stream_decoder_set_metadata_respond_application()
			virtual bool set_metadata_respond_all();                               ///< See FLAC__stream_decoder_set_metadata_respond_all()
			virtual bool set_metadata_ignore(::FLAC__MetadataType type);           ///< See FLAC__stream_decoder_set_metadata_ignore()
			virtual bool set_metadata_ignore_application(const FLAC__byte id[4]);  ///< See FLAC__stream_decoder_set_metadata_ignore_application()
			virtual bool set_metadata_ignore_all();                                ///< See FLAC__stream_decoder_set_metadata_ignore_all()

			/* get_state() is not virtual since we want subclasses to be able to return their own state */
			State get_state() const;                                          ///< See FLAC__stream_decoder_get_state()
			virtual bool get_md5_checking() const;                            ///< See FLAC__stream_decoder_get_md5_checking()
			virtual FLAC__uint64 get_total_samples() const;                   ///< See FLAC__stream_decoder_get_total_samples()
			virtual unsigned get_channels() const;                            ///< See FLAC__stream_decoder_get_channels()
			virtual ::FLAC__ChannelAssignment get_channel_assignment() const; ///< See FLAC__stream_decoder_get_channel_assignment()
			virtual unsigned get_bits_per_sample() const;                     ///< See FLAC__stream_decoder_get_bits_per_sample()
			virtual unsigned get_sample_rate() const;                         ///< See FLAC__stream_decoder_get_sample_rate()
			virtual unsigned get_blocksize() const;                           ///< See FLAC__stream_decoder_get_blocksize()
			virtual bool get_decode_position(FLAC__uint64 *position) const;   ///< See FLAC__stream_decoder_get_decode_position()

			virtual ::FLAC__StreamDecoderInitStatus init();      ///< Seek FLAC__stream_decoder_init_stream()
			virtual ::FLAC__StreamDecoderInitStatus init_ogg();  ///< Seek FLAC__stream_decoder_init_ogg_stream()

			virtual bool finish(); ///< See FLAC__stream_decoder_finish()

			virtual bool flush(); ///< See FLAC__stream_decoder_flush()
			virtual bool reset(); ///< See FLAC__stream_decoder_reset()

			virtual bool process_single();                ///< See FLAC__stream_decoder_process_single()
			virtual bool process_until_end_of_metadata(); ///< See FLAC__stream_decoder_process_until_end_of_metadata()
			virtual bool process_until_end_of_stream();   ///< See FLAC__stream_decoder_process_until_end_of_stream()
			virtual bool skip_single_frame();             ///< See FLAC__stream_decoder_skip_single_frame()

			virtual bool seek_absolute(FLAC__uint64 sample); ///< See FLAC__stream_decoder_seek_absolute()
		protected:
			/// see FLAC__StreamDecoderReadCallback
			virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) = 0;

			/// see FLAC__StreamDecoderSeekCallback
			virtual ::FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);

			/// see FLAC__StreamDecoderTellCallback
			virtual ::FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);

			/// see FLAC__StreamDecoderLengthCallback
			virtual ::FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);

			/// see FLAC__StreamDecoderEofCallback
			virtual bool eof_callback();

			/// see FLAC__StreamDecoderWriteCallback
			virtual ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]) = 0;

			/// see FLAC__StreamDecoderMetadataCallback
			virtual void metadata_callback(const ::FLAC__StreamMetadata *metadata);

			/// see FLAC__StreamDecoderErrorCallback
			virtual void error_callback(::FLAC__StreamDecoderErrorStatus status) = 0;

#if (defined _MSC_VER) || (defined __BORLANDC__) || (defined __GNUG__ && (__GNUG__ < 2 || (__GNUG__ == 2 && __GNUC_MINOR__ < 96))) || (defined __SUNPRO_CC)
			// lame hack: some MSVC/GCC versions can't see a protected decoder_ from nested State::resolved_as_cstring()
			friend State;
#endif
			::FLAC__StreamDecoder *decoder_;

			static ::FLAC__StreamDecoderReadStatus read_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
			static ::FLAC__StreamDecoderSeekStatus seek_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
			static ::FLAC__StreamDecoderTellStatus tell_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
			static ::FLAC__StreamDecoderLengthStatus length_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
			static FLAC__bool eof_callback_(const ::FLAC__StreamDecoder *decoder, void *client_data);
			static ::FLAC__StreamDecoderWriteStatus write_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
			static void metadata_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data);
			static void error_callback_(const ::FLAC__StreamDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data);
		private:
			// Private and undefined so you can't use them:
			Stream(const Stream &);
			void operator=(const Stream &);
		};

		/** \ingroup flacpp_decoder
		 *  \brief
		 *  This class wraps the ::FLAC__StreamDecoder.  If you are
		 *  not decoding from a file, you may need to use
		 *  FLAC::Decoder::Stream.
		 *
		 * The usage of this class is similar to FLAC__StreamDecoder,
		 * except instead of providing callbacks to
		 * FLAC__stream_decoder_init*_FILE() or
		 * FLAC__stream_decoder_init*_file(), you will inherit from this
		 * class and override the virtual callback functions with your
		 * own implementations, then call init() or init_off().  The rest
		 * of the calls work the same as in the C layer.
		 *
		 * Only the write, and error callbacks from FLAC::Decoder::Stream
		 * are mandatory.  The others are optional; this class provides
		 * full working implementations for all other callbacks and
		 * supports seeking.
		 */
		class FLACPP_API File: public Stream {
		public:
			File();
			virtual ~File();

			virtual ::FLAC__StreamDecoderInitStatus init(FILE *file);                      ///< See FLAC__stream_decoder_init_FILE()
			virtual ::FLAC__StreamDecoderInitStatus init(const char *filename);            ///< See FLAC__stream_decoder_init_file()
			virtual ::FLAC__StreamDecoderInitStatus init(const std::string &filename);     ///< See FLAC__stream_decoder_init_file()
			virtual ::FLAC__StreamDecoderInitStatus init_ogg(FILE *file);                  ///< See FLAC__stream_decoder_init_ogg_FILE()
			virtual ::FLAC__StreamDecoderInitStatus init_ogg(const char *filename);        ///< See FLAC__stream_decoder_init_ogg_file()
			virtual ::FLAC__StreamDecoderInitStatus init_ogg(const std::string &filename); ///< See FLAC__stream_decoder_init_ogg_file()
		protected:
			// this is a dummy implementation to satisfy the pure virtual in Stream that is actually supplied internally by the C layer
			virtual ::FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes);
		private:
			// Private and undefined so you can't use them:
			File(const File &);
			void operator=(const File &);
		};

	}
}

#endif
