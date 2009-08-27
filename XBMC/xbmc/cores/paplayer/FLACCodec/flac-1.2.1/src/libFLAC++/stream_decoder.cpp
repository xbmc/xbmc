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

#include "FLAC++/decoder.h"
#include "FLAC/assert.h"

#ifdef _MSC_VER
// warning C4800: 'int' : forcing to bool 'true' or 'false' (performance warning)
#pragma warning ( disable : 4800 )
#endif

namespace FLAC {
	namespace Decoder {

		// ------------------------------------------------------------
		//
		// Stream
		//
		// ------------------------------------------------------------

		Stream::Stream():
		decoder_(::FLAC__stream_decoder_new())
		{ }

		Stream::~Stream()
		{
			if(0 != decoder_) {
				(void)::FLAC__stream_decoder_finish(decoder_);
				::FLAC__stream_decoder_delete(decoder_);
			}
		}

		bool Stream::is_valid() const
		{
			return 0 != decoder_;
		}

		bool Stream::set_ogg_serial_number(long value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_ogg_serial_number(decoder_, value);
		}

		bool Stream::set_md5_checking(bool value)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_md5_checking(decoder_, value);
		}

		bool Stream::set_metadata_respond(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond(decoder_, type);
		}

		bool Stream::set_metadata_respond_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond_application(decoder_, id);
		}

		bool Stream::set_metadata_respond_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_respond_all(decoder_);
		}

		bool Stream::set_metadata_ignore(::FLAC__MetadataType type)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore(decoder_, type);
		}

		bool Stream::set_metadata_ignore_application(const FLAC__byte id[4])
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore_application(decoder_, id);
		}

		bool Stream::set_metadata_ignore_all()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_set_metadata_ignore_all(decoder_);
		}

		Stream::State Stream::get_state() const
		{
			FLAC__ASSERT(is_valid());
			return State(::FLAC__stream_decoder_get_state(decoder_));
		}

		bool Stream::get_md5_checking() const
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_get_md5_checking(decoder_);
		}

		FLAC__uint64 Stream::get_total_samples() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_total_samples(decoder_);
		}

		unsigned Stream::get_channels() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_channels(decoder_);
		}

		::FLAC__ChannelAssignment Stream::get_channel_assignment() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_channel_assignment(decoder_);
		}

		unsigned Stream::get_bits_per_sample() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_bits_per_sample(decoder_);
		}

		unsigned Stream::get_sample_rate() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_sample_rate(decoder_);
		}

		unsigned Stream::get_blocksize() const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_blocksize(decoder_);
		}

		bool Stream::get_decode_position(FLAC__uint64 *position) const
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_get_decode_position(decoder_, position);
		}

		::FLAC__StreamDecoderInitStatus Stream::init()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_init_stream(decoder_, read_callback_, seek_callback_, tell_callback_, length_callback_, eof_callback_, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus Stream::init_ogg()
		{
			FLAC__ASSERT(is_valid());
			return ::FLAC__stream_decoder_init_ogg_stream(decoder_, read_callback_, seek_callback_, tell_callback_, length_callback_, eof_callback_, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		bool Stream::finish()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_finish(decoder_);
		}

		bool Stream::flush()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_flush(decoder_);
		}

		bool Stream::reset()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_reset(decoder_);
		}

		bool Stream::process_single()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_single(decoder_);
		}

		bool Stream::process_until_end_of_metadata()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_until_end_of_metadata(decoder_);
		}

		bool Stream::process_until_end_of_stream()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_process_until_end_of_stream(decoder_);
		}

		bool Stream::skip_single_frame()
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_skip_single_frame(decoder_);
		}

		bool Stream::seek_absolute(FLAC__uint64 sample)
		{
			FLAC__ASSERT(is_valid());
			return (bool)::FLAC__stream_decoder_seek_absolute(decoder_, sample);
		}

		::FLAC__StreamDecoderSeekStatus Stream::seek_callback(FLAC__uint64 absolute_byte_offset)
		{
			(void)absolute_byte_offset;
			return ::FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
		}

		::FLAC__StreamDecoderTellStatus Stream::tell_callback(FLAC__uint64 *absolute_byte_offset)
		{
			(void)absolute_byte_offset;
			return ::FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
		}

		::FLAC__StreamDecoderLengthStatus Stream::length_callback(FLAC__uint64 *stream_length)
		{
			(void)stream_length;
			return ::FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
		}

		bool Stream::eof_callback()
		{
			return false;
		}

		void Stream::metadata_callback(const ::FLAC__StreamMetadata *metadata)
		{
			(void)metadata;
		}

		::FLAC__StreamDecoderReadStatus Stream::read_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->read_callback(buffer, bytes);
		}

		::FLAC__StreamDecoderSeekStatus Stream::seek_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->seek_callback(absolute_byte_offset);
		}

		::FLAC__StreamDecoderTellStatus Stream::tell_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->tell_callback(absolute_byte_offset);
		}

		::FLAC__StreamDecoderLengthStatus Stream::length_callback_(const ::FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->length_callback(stream_length);
		}

		FLAC__bool Stream::eof_callback_(const ::FLAC__StreamDecoder *decoder, void *client_data)
		{
			(void) decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->eof_callback();
		}

		::FLAC__StreamDecoderWriteStatus Stream::write_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			return instance->write_callback(frame, buffer);
		}

		void Stream::metadata_callback_(const ::FLAC__StreamDecoder *decoder, const ::FLAC__StreamMetadata *metadata, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->metadata_callback(metadata);
		}

		void Stream::error_callback_(const ::FLAC__StreamDecoder *decoder, ::FLAC__StreamDecoderErrorStatus status, void *client_data)
		{
			(void)decoder;
			FLAC__ASSERT(0 != client_data);
			Stream *instance = reinterpret_cast<Stream *>(client_data);
			FLAC__ASSERT(0 != instance);
			instance->error_callback(status);
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

		::FLAC__StreamDecoderInitStatus File::init(FILE *file)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__stream_decoder_init_FILE(decoder_, file, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init(const char *filename)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__stream_decoder_init_file(decoder_, filename, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init(const std::string &filename)
		{
			return init(filename.c_str());
		}

		::FLAC__StreamDecoderInitStatus File::init_ogg(FILE *file)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__stream_decoder_init_ogg_FILE(decoder_, file, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init_ogg(const char *filename)
		{
			FLAC__ASSERT(0 != decoder_);
			return ::FLAC__stream_decoder_init_ogg_file(decoder_, filename, write_callback_, metadata_callback_, error_callback_, /*client_data=*/(void*)this);
		}

		::FLAC__StreamDecoderInitStatus File::init_ogg(const std::string &filename)
		{
			return init_ogg(filename.c_str());
		}

		// This is a dummy to satisfy the pure virtual from Stream; the
		// read callback will never be called since we are initializing
		// with FLAC__stream_decoder_init_FILE() or
		// FLAC__stream_decoder_init_file() and those supply the read
		// callback internally.
		::FLAC__StreamDecoderReadStatus File::read_callback(FLAC__byte buffer[], size_t *bytes)
		{
			(void)buffer, (void)bytes;
			FLAC__ASSERT(false);
			return ::FLAC__STREAM_DECODER_READ_STATUS_ABORT; // double protection
		}

	}
}
