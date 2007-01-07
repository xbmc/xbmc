/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003,2004,2005  Josh Coalson
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

#ifndef FLAC__SEEKABLE_STREAM_DECODER_H
#define FLAC__SEEKABLE_STREAM_DECODER_H

#include "export.h"
#include "stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/seekable_stream_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link flac_seekable_stream_decoder seekable stream decoder \endlink module.
 */

/** \defgroup flac_seekable_stream_decoder FLAC/seekable_stream_decoder.h: seekable stream decoder interface
 *  \ingroup flac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   FLAC__seekable_stream_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, seeking, error reporting, and metadata reporting
 *   using FLAC__seekable_stream_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using FLAC__seekable_stream_decoder_init().
 * - The program calls the FLAC__seekable_stream_decoder_process_*()
 *   functions to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with
 *   FLAC__seekable_stream_decoder_finish(), which flushes the input and
 *   output and resets the decoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__seekable_stream_decoder_delete().
 *
 * The seekable stream decoder is a wrapper around the
 * \link flac_stream_decoder stream decoder \endlink which also provides
 * seeking capability.  In addition to the Read/Write/Metadata/Error
 * callbacks of the stream decoder, the user must also provide the following:
 *
 * - Seek callback - This function will be called when the decoder wants to
 *   seek to an absolute position in the stream.
 * - Tell callback - This function will be called when the decoder wants to
 *   know the current absolute position of the stream.
 * - Length callback - This function will be called when the decoder wants
 *   to know length of the stream.  The seeking algorithm currently requires
 *   that the overall stream length be known.
 * - EOF callback - This function will be called when the decoder wants to
 *   know if it is at the end of the stream.  This could be synthesized from
 *   the tell and length callbacks but it may be more expensive that way, so
 *   there is a separate callback for it.
 *
 * Seeking is exposed through the
 * FLAC__seekable_stream_decoder_seek_absolute() method.  At any point after
 * the seekable stream decoder has been initialized, the user can call this
 * function to seek to an exact sample within the stream.  Subsequently, the
 * first time the write callback is called it will be passed a (possibly
 * partial) block starting at that sample.
 *
 * The seekable stream decoder also provides MD5 signature checking.  If
 * this is turned on before initialization,
 * FLAC__seekable_stream_decoder_finish() will report when the decoded MD5
 * signature does not match the one stored in the STREAMINFO block.  MD5
 * checking is automatically turned off (until the next
 * FLAC__seekable_stream_decoder_reset()) if there is no signature in the
 * STREAMINFO block or when a seek is attempted.
 *
 * Make sure to read the detailed description of the
 * \link flac_stream_decoder stream decoder module \endlink since the
 * seekable stream decoder inherits much of its behavior.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED, i.e. after
 * FLAC__seekable_stream_decoder_new() or
 * FLAC__seekable_stream_decoder_finish(), but before
 * FLAC__seekable_stream_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__stream_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a FLAC__SeekableStreamDecoder
 *
 *  The decoder's state can be obtained by calling FLAC__seekable_stream_decoder_get_state().
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_DECODER_OK = 0,
	/**< The decoder is in the normal OK state. */

	FLAC__SEEKABLE_STREAM_DECODER_SEEKING,
	/**< The decoder is in the process of seeking. */

	FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM,
	/**< The decoder has reached the end of the stream. */

	FLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory. */

	FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying stream decoder. */

	FLAC__SEEKABLE_STREAM_DECODER_READ_ERROR,
	/**< The read callback returned an error. */

	FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking or the seek or tell
	 * callback returned an error.
	 */

	FLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED,
	/**< FLAC__seekable_stream_decoder_init() was called when the
	 * decoder was already initialized, usually because
	 * FLAC__seekable_stream_decoder_finish() was not called.
	 */

	FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK,
	/**< FLAC__seekable_stream_decoder_init() was called without all
	 * callbacks being set.
	 */

	FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} FLAC__SeekableStreamDecoderState;

/** Maps a FLAC__SeekableStreamDecoderState to a C string.
 *
 *  Using a FLAC__SeekableStreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamDecoderStateString[];


/** Return values for the FLAC__SeekableStreamDecoder read callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK,
	/**< The read was OK and decoding can continue. */

	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__SeekableStreamDecoderReadStatus;

/** Maps a FLAC__SeekableStreamDecoderReadStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamDecoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamDecoderReadStatusString[];


/** Return values for the FLAC__SeekableStreamDecoder seek callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK,
	/**< The seek was OK and decoding can continue. */

	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__SeekableStreamDecoderSeekStatus;

/** Maps a FLAC__SeekableStreamDecoderSeekStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamDecoderSeekStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamDecoderSeekStatusString[];


/** Return values for the FLAC__SeekableStreamDecoder tell callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK,
	/**< The tell was OK and decoding can continue. */

	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__SeekableStreamDecoderTellStatus;

/** Maps a FLAC__SeekableStreamDecoderTellStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamDecoderTellStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamDecoderTellStatusString[];


/** Return values for the FLAC__SeekableStreamDecoder length callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK,
	/**< The length call was OK and decoding can continue. */

	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__SeekableStreamDecoderLengthStatus;

/** Maps a FLAC__SeekableStreamDecoderLengthStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamDecoderLengthStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamDecoderLengthStatusString[];


/***********************************************************************
 *
 * class FLAC__SeekableStreamDecoder : public FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__SeekableStreamDecoderProtected;
struct FLAC__SeekableStreamDecoderPrivate;
/** The opaque structure definition for the seekable stream decoder type.
 *  See the
 *  \link flac_seekable_stream_decoder seekable stream decoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct FLAC__SeekableStreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__SeekableStreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__SeekableStreamDecoder;

/** Signature for the read callback.
 *  See FLAC__seekable_stream_decoder_set_read_callback()
 *  and FLAC__StreamDecoderReadCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  buffer   A pointer to a location for the callee to store
 *                  data to be decoded.
 * \param  bytes    A pointer to the size of the buffer.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderReadStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamDecoderReadStatus (*FLAC__SeekableStreamDecoderReadCallback)(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

/** Signature for the seek callback.
 *  See FLAC__seekable_stream_decoder_set_seek_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  absolute_byte_offset  The offset from the beginning of the stream
 *                               to seek to.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderSeekStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamDecoderSeekStatus (*FLAC__SeekableStreamDecoderSeekCallback)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);

/** Signature for the tell callback.
 *  See FLAC__seekable_stream_decoder_set_tell_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  absolute_byte_offset  A pointer to storage for the current offset
 *                               from the beginning of the stream.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderTellStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamDecoderTellStatus (*FLAC__SeekableStreamDecoderTellCallback)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);

/** Signature for the length callback.
 *  See FLAC__seekable_stream_decoder_set_length_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  stream_length  A pointer to storage for the length of the stream
 *                        in bytes.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderLengthStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamDecoderLengthStatus (*FLAC__SeekableStreamDecoderLengthCallback)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);

/** Signature for the EOF callback.
 *  See FLAC__seekable_stream_decoder_set_eof_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__bool
 *    \c true if the currently at the end of the stream, else \c false.
 */
typedef FLAC__bool (*FLAC__SeekableStreamDecoderEofCallback)(const FLAC__SeekableStreamDecoder *decoder, void *client_data);

/** Signature for the write callback.
 *  See FLAC__seekable_stream_decoder_set_write_callback()
 *  and FLAC__StreamDecoderWriteCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  frame    The description of the decoded frame.
 * \param  buffer   An array of pointers to decoded channels of data.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderWriteStatus (*FLAC__SeekableStreamDecoderWriteCallback)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);

/** Signature for the metadata callback.
 *  See FLAC__seekable_stream_decoder_set_metadata_callback()
 *  and FLAC__StreamDecoderMetadataCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  metadata The decoded metadata block.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 */
typedef void (*FLAC__SeekableStreamDecoderMetadataCallback)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);

/** Signature for the error callback.
 *  See FLAC__seekable_stream_decoder_set_error_callback()
 *  and FLAC__StreamDecoderErrorCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  status   The error encountered by the decoder.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_decoder_set_client_data().
 */
typedef void (*FLAC__SeekableStreamDecoderErrorCallback)(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new seekable stream decoder instance.  The instance is created
 *  with default settings; see the individual
 *  FLAC__seekable_stream_decoder_set_*() functions for each setting's
 *  default.
 *
 * \retval FLAC__SeekableStreamDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC_API FLAC__SeekableStreamDecoder *FLAC__seekable_stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
FLAC_API void FLAC__seekable_stream_decoder_delete(FLAC__SeekableStreamDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the "MD5 signature checking" flag.  If \c true, the decoder will
 *  compute the MD5 signature of the unencoded audio data while decoding
 *  and compare it to the signature from the STREAMINFO block, if it
 *  exists, during FLAC__seekable_stream_decoder_finish().
 *
 *  MD5 signature checking will be turned off (until the next
 *  FLAC__seekable_stream_decoder_reset()) if there is no signature in
 *  the STREAMINFO block or when a seek is attempted.
 *
 * \default \c false
 * \param  decoder  A decoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value);

/** Set the read callback.
 *  This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_read_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value);

/** Set the seek callback.
 *  The supplied function will be called when the decoder needs to seek
 *  the input stream.  The decoder will pass the absolute byte offset
 *  to seek to, 0 meaning the beginning of the stream.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value);

/** Set the tell callback.
 *  The supplied function will be called when the decoder wants to know
 *  the current position of the stream.  The callback should return the
 *  byte offset from the beginning of the stream.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value);

/** Set the length callback.
 *  The supplied function will be called when the decoder wants to know
 *  the total length of the stream in bytes.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value);

/** Set the eof callback.
 *  The supplied function will be called when the decoder needs to know
 *  if the end of the stream has been reached.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value);

/** Set the write callback.
 *  This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_write_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value);

/** Set the metadata callback.
 *  This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value);

/** Set the error callback.
 *  This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_error_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value);

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_client_data(FLAC__SeekableStreamDecoder *decoder, void *value);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_respond().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_respond_application().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_respond_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_all(FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_ignore().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_ignore_application().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_set_metadata_ignore_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_all(FLAC__SeekableStreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__SeekableStreamDecoderState
 *    The current decoder state.
 */
FLAC_API FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder);

/** Get the state of the underlying stream decoder.
 *  Useful when the seekable stream decoder state is
 *  \c FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The stream decoder state.
 */
FLAC_API FLAC__StreamDecoderState FLAC__seekable_stream_decoder_get_stream_decoder_state(const FLAC__SeekableStreamDecoder *decoder);

/** Get the current decoder state as a C string.
 *  This version automatically resolves
 *  \c FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR by getting the
 *  stream decoder's state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval const char *
 *    The decoder state as a C string.  Do not modify the contents.
 */
FLAC_API const char *FLAC__seekable_stream_decoder_get_resolved_state_string(const FLAC__SeekableStreamDecoder *decoder);

/** Get the "MD5 signature checking" flag.
 *  This is the value of the setting, not whether or not the decoder is
 *  currently checking the MD5 (remember, it can be turned off automatically
 *  by a seek).  When the decoder is reset the flag will be restored to the
 *  value returned by this function.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__seekable_stream_decoder_get_channels(const FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__ChannelAssignment
 *    See above.
 */
FLAC_API FLAC__ChannelAssignment FLAC__seekable_stream_decoder_get_channel_assignment(const FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__seekable_stream_decoder_get_bits_per_sample(const FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__seekable_stream_decoder_get_sample_rate(const FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__seekable_stream_decoder_get_blocksize(const FLAC__SeekableStreamDecoder *decoder);

/** Returns the decoder's current read position within the stream.
 *  The position is the byte offset from the start of the stream.
 *  Bytes before this position have been fully decoded.  Note that
 *  there may still be undecoded bytes in the decoder's read FIFO.
 *  The returned position is correct even after a seek.
 *
 * \param  decoder   A decoder instance to query.
 * \param  position  Address at which to return the desired position.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code position != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, \c false if there was an error from
 *    the 'tell' callback.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_get_decode_position(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *position);

/** Initialize the decoder instance.
 *  Should be called after FLAC__seekable_stream_decoder_new() and
 *  FLAC__seekable_stream_decoder_set_*() but before any of the
 *  FLAC__seekable_stream_decoder_process_*() functions.  Will set and return
 *  the decoder state, which will be FLAC__SEEKABLE_STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__SeekableStreamDecoderState
 *    \c FLAC__SEEKABLE_STREAM_DECODER_OK if initialization was
 *    successful; see FLAC__SeekableStreamDecoderState for the meanings
 *    of other return values.
 */
FLAC_API FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_init(FLAC__SeekableStreamDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before
 *  FLAC__seekable_stream_decoder_delete() but it is good practice to match
 *  every FLAC__seekable_stream_decoder_init() with a
 *  FLAC__seekable_stream_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if MD5 checking is on AND a STREAMINFO block was available
 *    AND the MD5 signature in the STREAMINFO block was non-zero AND the
 *    signature does not match the one computed by the decoder; else
 *    \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_finish(FLAC__SeekableStreamDecoder *decoder);

/** Flush the stream input.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c FLAC__SEEKABLE_STREAM_DECODER_OK.  This will also turn off MD5
 *  checking.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_flush(FLAC__SeekableStreamDecoder *decoder);

/** Reset the decoding process.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c FLAC__SEEKABLE_STREAM_DECODER_OK.  This is similar to
 *  FLAC__seekable_stream_decoder_finish() except that the settings are
 *  preserved; there is no need to call FLAC__seekable_stream_decoder_init()
 *  before decoding again.  MD5 checking will be restored to its original
 *  setting.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_reset(FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_single(FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_metadata(FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_stream(FLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__StreamDecoder; see
 *  FLAC__stream_decoder_skip_single_frame().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_skip_single_frame(FLAC__SeekableStreamDecoder *decoder);

/** Flush the input and seek to an absolute sample.
 *  Decoding will resume at the given sample.  Note that because of
 *  this, the next write callback may contain a partial block.
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
