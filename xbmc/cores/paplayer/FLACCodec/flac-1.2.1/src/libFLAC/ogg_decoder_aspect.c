/* libFLAC - Free Lossless Audio Codec
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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h> /* for memcpy() */
#include "FLAC/assert.h"
#include "private/ogg_decoder_aspect.h"
#include "private/ogg_mapping.h"

#ifdef max
#undef max
#endif
#define max(x,y) ((x)>(y)?(x):(y))

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__bool FLAC__ogg_decoder_aspect_init(FLAC__OggDecoderAspect *aspect)
{
	/* we will determine the serial number later if necessary */
	if(ogg_stream_init(&aspect->stream_state, aspect->serial_number) != 0)
		return false;

	if(ogg_sync_init(&aspect->sync_state) != 0)
		return false;

	aspect->version_major = ~(0u);
	aspect->version_minor = ~(0u);

	aspect->need_serial_number = aspect->use_first_serial_number;

	aspect->end_of_stream = false;
	aspect->have_working_page = false;

	return true;
}

void FLAC__ogg_decoder_aspect_finish(FLAC__OggDecoderAspect *aspect)
{
	(void)ogg_sync_clear(&aspect->sync_state);
	(void)ogg_stream_clear(&aspect->stream_state);
}

void FLAC__ogg_decoder_aspect_set_serial_number(FLAC__OggDecoderAspect *aspect, long value)
{
	aspect->use_first_serial_number = false;
	aspect->serial_number = value;
}

void FLAC__ogg_decoder_aspect_set_defaults(FLAC__OggDecoderAspect *aspect)
{
	aspect->use_first_serial_number = true;
}

void FLAC__ogg_decoder_aspect_flush(FLAC__OggDecoderAspect *aspect)
{
	(void)ogg_stream_reset(&aspect->stream_state);
	(void)ogg_sync_reset(&aspect->sync_state);
	aspect->end_of_stream = false;
	aspect->have_working_page = false;
}

void FLAC__ogg_decoder_aspect_reset(FLAC__OggDecoderAspect *aspect)
{
	FLAC__ogg_decoder_aspect_flush(aspect);

	if(aspect->use_first_serial_number)
		aspect->need_serial_number = true;
}

FLAC__OggDecoderAspectReadStatus FLAC__ogg_decoder_aspect_read_callback_wrapper(FLAC__OggDecoderAspect *aspect, FLAC__byte buffer[], size_t *bytes, FLAC__OggDecoderAspectReadCallbackProxy read_callback, const FLAC__StreamDecoder *decoder, void *client_data)
{
	static const size_t OGG_BYTES_CHUNK = 8192;
	const size_t bytes_requested = *bytes;

	/*
	 * The FLAC decoding API uses pull-based reads, whereas Ogg decoding
	 * is push-based.  In libFLAC, when you ask to decode a frame, the
	 * decoder will eventually call the read callback to supply some data,
	 * but how much it asks for depends on how much free space it has in
	 * its internal buffer.  It does not try to grow its internal buffer
	 * to accomodate a whole frame because then the internal buffer size
	 * could not be limited, which is necessary in embedded applications.
	 *
	 * Ogg however grows its internal buffer until a whole page is present;
	 * only then can you get decoded data out.  So we can't just ask for
	 * the same number of bytes from Ogg, then pass what's decoded down to
	 * libFLAC.  If what libFLAC is asking for will not contain a whole
	 * page, then we will get no data from ogg_sync_pageout(), and at the
	 * same time cannot just read more data from the client for the purpose
	 * of getting a whole decoded page because the decoded size might be
	 * larger than libFLAC's internal buffer.
	 *
	 * Instead, whenever this read callback wrapper is called, we will
	 * continually request data from the client until we have at least one
	 * page, and manage pages internally so that we can send pieces of
	 * pages down to libFLAC in such a way that we obey its size
	 * requirement.  To limit the amount of callbacks, we will always try
	 * to read in enough pages to return the full number of bytes
	 * requested.
	 */
	*bytes = 0;
	while (*bytes < bytes_requested && !aspect->end_of_stream) {
		if (aspect->have_working_page) {
			if (aspect->have_working_packet) {
				size_t n = bytes_requested - *bytes;
				if ((size_t)aspect->working_packet.bytes <= n) {
					/* the rest of the packet will fit in the buffer */
					n = aspect->working_packet.bytes;
					memcpy(buffer, aspect->working_packet.packet, n);
					*bytes += n;
					buffer += n;
					aspect->have_working_packet = false;
				}
				else {
					/* only n bytes of the packet will fit in the buffer */
					memcpy(buffer, aspect->working_packet.packet, n);
					*bytes += n;
					buffer += n;
					aspect->working_packet.packet += n;
					aspect->working_packet.bytes -= n;
				}
			}
			else {
				/* try and get another packet */
				const int ret = ogg_stream_packetout(&aspect->stream_state, &aspect->working_packet);
				if (ret > 0) {
					aspect->have_working_packet = true;
					/* if it is the first header packet, check for magic and a supported Ogg FLAC mapping version */
					if (aspect->working_packet.bytes > 0 && aspect->working_packet.packet[0] == FLAC__OGG_MAPPING_FIRST_HEADER_PACKET_TYPE) {
						const FLAC__byte *b = aspect->working_packet.packet;
						const unsigned header_length =
							FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH +
							FLAC__OGG_MAPPING_MAGIC_LENGTH +
							FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH +
							FLAC__OGG_MAPPING_VERSION_MINOR_LENGTH +
							FLAC__OGG_MAPPING_NUM_HEADERS_LENGTH;
						if (aspect->working_packet.bytes < (long)header_length)
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC;
						b += FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH;
						if (memcmp(b, FLAC__OGG_MAPPING_MAGIC, FLAC__OGG_MAPPING_MAGIC_LENGTH))
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC;
						b += FLAC__OGG_MAPPING_MAGIC_LENGTH;
						aspect->version_major = (unsigned)(*b);
						b += FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH;
						aspect->version_minor = (unsigned)(*b);
						if (aspect->version_major != 1)
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_UNSUPPORTED_MAPPING_VERSION;
						aspect->working_packet.packet += header_length;
						aspect->working_packet.bytes -= header_length;
					}
				}
				else if (ret == 0) {
					aspect->have_working_page = false;
				}
				else { /* ret < 0 */
					/* lost sync, we'll leave the working page for the next call */
					return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
				}
			}
		}
		else {
			/* try and get another page */
			const int ret = ogg_sync_pageout(&aspect->sync_state, &aspect->working_page);
			if (ret > 0) {
				/* got a page, grab the serial number if necessary */
				if(aspect->need_serial_number) {
					aspect->stream_state.serialno = aspect->serial_number = ogg_page_serialno(&aspect->working_page);
					aspect->need_serial_number = false;
				}
				if(ogg_stream_pagein(&aspect->stream_state, &aspect->working_page) == 0) {
					aspect->have_working_page = true;
					aspect->have_working_packet = false;
				}
				/* else do nothing, could be a page from another stream */
			}
			else if (ret == 0) {
				/* need more data */
				const size_t ogg_bytes_to_read = max(bytes_requested - *bytes, OGG_BYTES_CHUNK);
				char *oggbuf = ogg_sync_buffer(&aspect->sync_state, ogg_bytes_to_read);

				if(0 == oggbuf) {
					return FLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;
				}
				else {
					size_t ogg_bytes_read = ogg_bytes_to_read;

					switch(read_callback(decoder, (FLAC__byte*)oggbuf, &ogg_bytes_read, client_data)) {
						case FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK:
							break;
						case FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM:
							aspect->end_of_stream = true;
							break;
						case FLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT:
							return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT;
						default:
							FLAC__ASSERT(0);
					}

					if(ogg_sync_wrote(&aspect->sync_state, ogg_bytes_read) < 0) {
						/* double protection; this will happen if the read callback returns more bytes than the max requested, which would overflow Ogg's internal buffer */
						FLAC__ASSERT(0);
						return FLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
					}
				}
			}
			else { /* ret < 0 */
				/* lost sync, bail out */
				return FLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC;
			}
		}
	}

	if (aspect->end_of_stream && *bytes == 0) {
		return FLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
	}

	return FLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
}
