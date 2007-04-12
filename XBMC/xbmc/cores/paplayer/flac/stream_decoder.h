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

#ifndef FLAC__STREAM_DECODER_H
#define FLAC__STREAM_DECODER_H

#include "export.h"
#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/stream_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link flac_stream_decoder stream decoder \endlink module.
 */

/** \defgroup flac_decoder FLAC/ *_decoder.h: decoder interfaces
 *  \ingroup flac
 *
 *  \brief
 *  This module describes the three decoder layers provided by libFLAC.
 *
 * For decoding FLAC streams, libFLAC provides three layers of access.  The
 * lowest layer is non-seekable stream-level decoding, the next is seekable
 * stream-level decoding, and the highest layer is file-level decoding.  The
 * interfaces are described in the \link flac_stream_decoder stream decoder
 * \endlink, \link flac_seekable_stream_decoder seekable stream decoder
 * \endlink, and \link flac_file_decoder file decoder \endlink modules
 * respectively.  Typically you will choose the highest layer that your input
 * source will support.
 *
 * The stream decoder relies on callbacks for all input and output and has no
 * provisions for seeking.  The seekable stream decoder wraps the stream
 * decoder and exposes functions for seeking.  However, you must provide
 * extra callbacks for seek-related operations on your stream, like seek and
 * tell.  The file decoder wraps the seekable stream decoder and supplies
 * most of the callbacks internally, simplifying the processing of standard
 * files.
 */

/** \defgroup flac_stream_decoder FLAC/stream_decoder.h: stream decoder interface
 *  \ingroup flac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   FLAC__stream_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, error reporting, and metadata reporting using
 *   FLAC__stream_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using FLAC__stream_decoder_init().
 * - The program calls the FLAC__stream_decoder_process_*() functions
 *   to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with FLAC__stream_decoder_finish(),
 *   which flushes the input and output and resets the decoder to the
 *   uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__stream_decoder_delete().
 *
 * In more detail, the program will create a new instance by calling
 * FLAC__stream_decoder_new(), then call FLAC__stream_decoder_set_*()
 * functions to set the callbacks and client data, and call
 * FLAC__stream_decoder_init().  The required callbacks are:
 *
 * - Read callback - This function will be called when the decoder needs
 *   more input data.  The address of the buffer to be filled is supplied,
 *   along with the number of bytes the buffer can hold.  The callback may
 *   choose to supply less data and modify the byte count but must be careful
 *   not to overflow the buffer.  The callback then returns a status code
 *   chosen from FLAC__StreamDecoderReadStatus.
 * - Write callback - This function will be called when the decoder has
 *   decoded a single frame of data.  The decoder will pass the frame
 *   metadata as well as an array of pointers (one for each channel)
 *   pointing to the decoded audio.
 * - Metadata callback - This function will be called when the decoder has
 *   decoded a metadata block.  In a valid FLAC file there will always be
 *   one STREAMINFO block, followed by zero or more other metadata
 *   blocks.  These will be supplied by the decoder in the same order as
 *   they appear in the stream and always before the first audio frame
 *   (i.e. write callback).  The metadata block that is passed in must not
 *   be modified, and it doesn't live beyond the callback, so you should
 *   make a copy of it with FLAC__metadata_object_clone() if you will need
 *   it elsewhere.  Since metadata blocks can potentially be large, by
 *   default the decoder only calls the metadata callback for the STREAMINFO
 *   block; you can instruct the decoder to pass or filter other blocks with
 *   FLAC__stream_decoder_set_metadata_*() calls.
 * - Error callback - This function will be called whenever an error occurs
 *   during decoding.
 *
 * Once the decoder is initialized, your program will call one of several
 * functions to start the decoding process:
 *
 * - FLAC__stream_decoder_process_single() - Tells the decoder to process at
 *   most one metadata block or audio frame and return, calling either the
 *   metadata callback or write callback, respectively, once.  If the decoder
 *   loses sync it will return with only the error callback being called.
 * - FLAC__stream_decoder_process_until_end_of_metadata() - Tells the decoder
 *   to process the stream from the current location and stop upon reaching
 *   the first audio frame.  The user will get one metadata, write, or error
 *   callback per metadata block, audio frame, or sync error, respectively.
 * - FLAC__stream_decoder_process_until_end_of_stream() - Tells the decoder
 *   to process the stream from the current location until the read callback
 *   returns FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM or
 *   FLAC__STREAM_DECODER_READ_STATUS_ABORT.  The user will get one metadata,
 *   write, or error callback per metadata block, audio frame, or sync error,
 *   respectively.
 *
 * When the decoder has finished decoding (normally or through an abort),
 * the instance is finished by calling FLAC__stream_decoder_finish(), which
 * ensures the decoder is in the correct state and frees memory.  Then the
 * instance may be deleted with FLAC__stream_decoder_delete() or initialized
 * again to decode another stream.
 *
 * Note that the stream decoder has no real concept of stream position, it
 * just converts data.  To seek within a stream the callbacks have only to
 * flush the decoder using FLAC__stream_decoder_flush() and start feeding
 * data from the new position through the read callback.  The seekable
 * stream decoder does just this.
 *
 * The FLAC__stream_decoder_set_metadata_*() functions deserve special
 * attention.  By default, the decoder only calls the metadata_callback for
 * the STREAMINFO block.  These functions allow you to tell the decoder
 * explicitly which blocks to parse and return via the metadata_callback
 * and/or which to skip.  Use a FLAC__stream_decoder_set_metadata_respond_all(),
 * FLAC__stream_decoder_set_metadata_ignore() ... or FLAC__stream_decoder_set_metadata_ignore_all(),
 * FLAC__stream_decoder_set_metadata_respond() ... sequence to exactly specify which
 * blocks to return.  Remember that some metadata blocks can be big so
 * filtering out the ones you don't use can reduce the memory requirements
 * of the decoder.  Also note the special forms
 * FLAC__stream_decoder_set_metadata_respond_application(id) and
 * FLAC__stream_decoder_set_metadata_ignore_application(id) for filtering APPLICATION
 * blocks based on the application ID.
 *
 * STREAMINFO and SEEKTABLE blocks are always parsed and used internally, but
 * they still can legally be filtered from the metadata_callback.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state FLAC__STREAM_DECODER_UNINITIALIZED, i.e. after
 * FLAC__stream_decoder_new() or FLAC__stream_decoder_finish(), but
 * before FLAC__stream_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__stream_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a FLAC__StreamDecoder
 *
 *  The decoder's state can be obtained by calling FLAC__stream_decoder_get_state().
 */
typedef enum {

	FLAC__STREAM_DECODER_SEARCH_FOR_METADATA = 0,
	/**< The decoder is ready to search for metadata. */

	FLAC__STREAM_DECODER_READ_METADATA,
	/**< The decoder is ready to or is in the process of reading metadata. */

	FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC,
	/**< The decoder is ready to or is in the process of searching for the frame sync code. */

	FLAC__STREAM_DECODER_READ_FRAME,
	/**< The decoder is ready to or is in the process of reading a frame. */

	FLAC__STREAM_DECODER_END_OF_STREAM,
	/**< The decoder has reached the end of the stream. */

	FLAC__STREAM_DECODER_ABORTED,
	/**< The decoder was aborted by the read callback. */

	FLAC__STREAM_DECODER_UNPARSEABLE_STREAM,
	/**< The decoder encountered reserved fields in use in the stream. */

	FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory. */

	FLAC__STREAM_DECODER_ALREADY_INITIALIZED,
	/**< FLAC__stream_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * FLAC__stream_decoder_finish() was not called.
	 */

	FLAC__STREAM_DECODER_INVALID_CALLBACK,
	/**< FLAC__stream_decoder_init() was called without all callbacks being set. */

	FLAC__STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} FLAC__StreamDecoderState;

/** Maps a FLAC__StreamDecoderState to a C string.
 *
 *  Using a FLAC__StreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__StreamDecoderStateString[];


/** Return values for the FLAC__StreamDecoder read callback.
 */
typedef enum {

	FLAC__STREAM_DECODER_READ_STATUS_CONTINUE,
	/**< The read was OK and decoding can continue. */

	FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM,
	/**< The read was attempted at the end of the stream. */

	FLAC__STREAM_DECODER_READ_STATUS_ABORT
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__StreamDecoderReadStatus;

/** Maps a FLAC__StreamDecoderReadStatus to a C string.
 *
 *  Using a FLAC__StreamDecoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__StreamDecoderReadStatusString[];


/** Return values for the FLAC__StreamDecoder write callback.
 */
typedef enum {

	FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE,
	/**< The write was OK and decoding can continue. */

	FLAC__STREAM_DECODER_WRITE_STATUS_ABORT
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} FLAC__StreamDecoderWriteStatus;

/** Maps a FLAC__StreamDecoderWriteStatus to a C string.
 *
 *  Using a FLAC__StreamDecoderWriteStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__StreamDecoderWriteStatusString[];


/** Possible values passed in to the FLAC__StreamDecoder error callback.
 */
typedef enum {

	FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC,
	/**< An error in the stream caused the decoder to lose synchronization. */

	FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER,
	/**< The decoder encountered a corrupted frame header. */

	FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH
	/**< The frame's data did not match the CRC in the footer. */

} FLAC__StreamDecoderErrorStatus;

/** Maps a FLAC__StreamDecoderErrorStatus to a C string.
 *
 *  Using a FLAC__StreamDecoderErrorStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__StreamDecoderErrorStatusString[];


/***********************************************************************
 *
 * class FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__StreamDecoderProtected;
struct FLAC__StreamDecoderPrivate;
/** The opaque structure definition for the stream decoder type.
 *  See the \link flac_stream_decoder stream decoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct FLAC__StreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__StreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__StreamDecoder;

/** Signature for the read callback.
 *  See FLAC__stream_decoder_set_read_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  buffer   A pointer to a location for the callee to store
 *                  data to be decoded.
 * \param  bytes    A pointer to the size of the buffer.  On entry
 *                  to the callback, it contains the maximum number
 *                  of bytes that may be stored in \a buffer.  The
 *                  callee must set it to the actual number of bytes
 *                  stored (0 in case of error or end-of-stream) before
 *                  returning.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderReadStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderReadStatus (*FLAC__StreamDecoderReadCallback)(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

/** Signature for the write callback.
 *  See FLAC__stream_decoder_set_write_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  frame    The description of the decoded frame.  See
 *                  FLAC__Frame.
 * \param  buffer   An array of pointers to decoded channels of data.
 *                  Each pointer will point to an array of signed
 *                  samples of length \a frame->header.blocksize.
 *                  Currently, the channel order has no meaning
 *                  except for stereo streams; in this case channel
 *                  0 is left and 1 is right.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderWriteStatus (*FLAC__StreamDecoderWriteCallback)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);

/** Signature for the metadata callback.
 *  See FLAC__stream_decoder_set_metadata_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  metadata The decoded metadata block.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_decoder_set_client_data().
 */
typedef void (*FLAC__StreamDecoderMetadataCallback)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);

/** Signature for the error callback.
 *  See FLAC__stream_decoder_set_error_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  status   The error encountered by the decoder.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_decoder_set_client_data().
 */
typedef void (*FLAC__StreamDecoderErrorCallback)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new stream decoder instance.  The instance is created with
 *  default settings; see the individual FLAC__stream_decoder_set_*()
 *  functions for each setting's default.
 *
 * \retval FLAC__StreamDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC_API FLAC__StreamDecoder *FLAC__stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
FLAC_API void FLAC__stream_decoder_delete(FLAC__StreamDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the read callback.
 *  The supplied function will be called when the decoder needs more input
 *  data.  The address of the buffer to be filled is supplied, along with
 *  the number of bytes the buffer can hold.  The callback may choose to
 *  supply less data and modify the byte count but must be careful not to
 *  overflow the buffer.  The callback then returns a status code chosen
 *  from FLAC__StreamDecoderReadStatus.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_read_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback value);

/** Set the write callback.
 *  The supplied function will be called when the decoder has decoded a
 *  single frame of data.  The decoder will pass the frame metadata as
 *  well as an array of pointers (one for each channel) pointing to the
 *  decoded audio.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_write_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderWriteCallback value);

/** Set the metadata callback.
 *  The supplied function will be called when the decoder has decoded a metadata
 *  block.  In a valid FLAC file there will always be one STREAMINFO block,
 *  followed by zero or more other metadata blocks.  These will be supplied
 *  by the decoder in the same order as they appear in the stream and always
 *  before the first audio frame (i.e. write callback).  The metadata block
 *  that is passed in must not be modified, and it doesn't live beyond the
 *  callback, so you should make a copy of it with
 *  FLAC__metadata_object_clone() if you will need it elsewhere.  Since
 *  metadata blocks can potentially be large, by default the decoder only
 *  calls the metadata callback for the STREAMINFO block; you can instruct
 *  the decoder to pass or filter other blocks with
 *  FLAC__stream_decoder_set_metadata_*() calls.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderMetadataCallback value);

/** Set the error callback.
 *  The supplied function will be called whenever an error occurs during
 *  decoding.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_error_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorCallback value);

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
FLAC_API FLAC__bool FLAC__stream_decoder_set_client_data(FLAC__StreamDecoder *decoder, void *value);

/** Direct the decoder to pass on all metadata blocks of type \a type.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond(FLAC__StreamDecoder *decoder, FLAC__MetadataType type);

/** Direct the decoder to pass on all APPLICATION metadata blocks of the
 *  given \a id.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to pass on all metadata blocks of any type.
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond_all(FLAC__StreamDecoder *decoder);

/** Direct the decoder to filter out all metadata blocks of type \a type.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore(FLAC__StreamDecoder *decoder, FLAC__MetadataType type);

/** Direct the decoder to filter out all APPLICATION metadata blocks of
 *  the given \a id.
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
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4]);

/** Direct the decoder to filter out all metadata blocks of any type.
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore_all(FLAC__StreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The current decoder state.
 */
FLAC_API FLAC__StreamDecoderState FLAC__stream_decoder_get_state(const FLAC__StreamDecoder *decoder);

/** Get the current decoder state as a C string.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval const char *
 *    The decoder state as a C string.  Do not modify the contents.
 */
FLAC_API const char *FLAC__stream_decoder_get_resolved_state_string(const FLAC__StreamDecoder *decoder);

/** Get the current number of channels in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__stream_decoder_get_channels(const FLAC__StreamDecoder *decoder);

/** Get the current channel assignment in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__ChannelAssignment
 *    See above.
 */
FLAC_API FLAC__ChannelAssignment FLAC__stream_decoder_get_channel_assignment(const FLAC__StreamDecoder *decoder);

/** Get the current sample resolution in the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__stream_decoder_get_bits_per_sample(const FLAC__StreamDecoder *decoder);

/** Get the current sample rate in Hz of the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__stream_decoder_get_sample_rate(const FLAC__StreamDecoder *decoder);

/** Get the current blocksize of the stream being decoded.
 *  Will only be valid after decoding has started and will contain the
 *  value from the most recently decoded frame header.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
FLAC_API unsigned FLAC__stream_decoder_get_blocksize(const FLAC__StreamDecoder *decoder);

/** Initialize the decoder instance.
 *  Should be called after FLAC__stream_decoder_new() and
 *  FLAC__stream_decoder_set_*() but before any of the
 *  FLAC__stream_decoder_process_*() functions.  Will set and return the
 *  decoder state, which will be FLAC__STREAM_DECODER_SEARCH_FOR_METADATA
 *  if initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    \c FLAC__STREAM_DECODER_SEARCH_FOR_METADATA if initialization was
 *    successful; see FLAC__StreamDecoderState for the meanings of other
 *    return values.
 */
FLAC_API FLAC__StreamDecoderState FLAC__stream_decoder_init(FLAC__StreamDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  FLAC__STREAM_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before FLAC__stream_decoder_delete()
 *  but it is good practice to match every FLAC__stream_decoder_init()
 *  with a FLAC__stream_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 */
FLAC_API void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder);

/** Flush the stream input.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    error occurs.
 */
FLAC_API FLAC__bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder);

/** Reset the decoding process.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c FLAC__STREAM_DECODER_SEARCH_FOR_METADATA.  This is similar to
 *  FLAC__stream_decoder_finish() except that the settings are
 *  preserved; there is no need to call FLAC__stream_decoder_init()
 *  before decoding again.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    error occurs.
 */
FLAC_API FLAC__bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder);

/** Decode one metadata block or audio frame.
 *  This version instructs the decoder to decode a either a single metadata
 *  block or a single frame and stop, unless the callbacks return a fatal
 *  error or the read callback returns
 *  \c FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM.
 *
 *  As the decoder needs more input it will call the read callback.
 *  Depending on what was decoded, the metadata or write callback will be
 *  called with the decoded metadata block or audio frame, unless an error
 *  occurred.  If the decoder loses sync it will call the error callback
 *  instead.
 *
 *  Unless there is a fatal read error or end of stream, this function
 *  will return once one whole frame is decoded.  In other words, if the
 *  stream is not synchronized or points to a corrupt frame header, the
 *  decoder will continue to try and resync until it gets to a valid
 *  frame, then decode one frame, then return.  If the decoder points to
 *  frame whose frame CRC in the frame footer does not match the
 *  computed frame CRC, this function will issue a
 *  FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH error to the
 *  error callback, and return, having decoded one complete, although
 *  corrupt, frame.  (Such corrupted frames are sent as silence of the
 *  correct length to the write callback.)
 *
 * \param  decoder  An initialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    FLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC_API FLAC__bool FLAC__stream_decoder_process_single(FLAC__StreamDecoder *decoder);

/** Decode until the end of the metadata.
 *  This version instructs the decoder to decode from the current position
 *  and continue until all the metadata has been read, or until the
 *  callbacks return a fatal error or the read callback returns
 *  \c FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM.
 *
 *  As the decoder needs more input it will call the read callback.
 *  As each metadata block is decoded, the metadata callback will be called
 *  with the decoded metadata.  If the decoder loses sync it will call the
 *  error callback.
 *
 * \param  decoder  An initialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    FLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC_API FLAC__bool FLAC__stream_decoder_process_until_end_of_metadata(FLAC__StreamDecoder *decoder);

/** Decode until the end of the stream.
 *  This version instructs the decoder to decode from the current position
 *  and continue until the end of stream (the read callback returns
 *  \c FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM), or until the
 *  callbacks return a fatal error.
 *
 *  As the decoder needs more input it will call the read callback.
 *  As each metadata block and frame is decoded, the metadata or write
 *  callback will be called with the decoded metadata or frame.  If the
 *  decoder loses sync it will call the error callback.
 *
 * \param  decoder  An initialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), else \c true;
 *    in any case, check the decoder state with
 *    FLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC_API FLAC__bool FLAC__stream_decoder_process_until_end_of_stream(FLAC__StreamDecoder *decoder);

/** Skip one audio frame.
 *  This version instructs the decoder to 'skip' a single frame and stop,
 *  unless the callbacks return a fatal error or the read callback returns
 *  \c FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM.
 *
 *  The decoding flow is the same as what occurs when
 *  FLAC__stream_decoder_process_single() is called to process an audio
 *  frame, except that this function does not decode the parsed data into
 *  PCM or call the write callback.  The integrity of the frame is still
 *  checked the same way as in the other process functions.
 *
 *  This function will return once one whole frame is skipped, in the
 *  same way that FLAC__stream_decoder_process_single() will return once
 *  one whole frame is decoded.
 *
 *  This function, when used from the higher FLAC__SeekableStreamDecoder
 *  layer, can be used in more quickly determining FLAC frame boundaries
 *  when decoding of the actual data is not needed, for example when an
 *  application is separating a FLAC stream into frames for editing or
 *  storing in a container.  To do this, the application can use
 *  FLAC__seekable_stream_decoder_skip_single_frame() to quickly advance
 *  to the next frame, then use
 *  FLAC__seekable_stream_decoder_get_decode_position() to find the new
 *  frame boundary.
 *
 *  This function should only be called when the stream has advanced
 *  past all the metadata, otherwise it will return \c false.
 *
 * \param  decoder  An initialized decoder instance not in a metadata
 *                  state.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if any read or write error occurred (except
 *    \c FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC), or if the decoder
 *    is in the FLAC__STREAM_DECODER_SEARCH_FOR_METADATA or
 *    FLAC__STREAM_DECODER_READ_METADATA state, else \c true;
 *    in any case, check the decoder state with
 *    FLAC__stream_decoder_get_state() to see what went wrong or to
 *    check for lost synchronization (a sign of stream corruption).
 */
FLAC_API FLAC__bool FLAC__stream_decoder_skip_single_frame(FLAC__StreamDecoder *decoder);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
