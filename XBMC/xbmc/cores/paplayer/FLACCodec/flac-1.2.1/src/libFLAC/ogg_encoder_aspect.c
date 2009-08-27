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

#include <string.h> /* for memset() */
#include "FLAC/assert.h"
#include "private/ogg_encoder_aspect.h"
#include "private/ogg_mapping.h"

static const FLAC__byte FLAC__OGG_MAPPING_VERSION_MAJOR = 1;
static const FLAC__byte FLAC__OGG_MAPPING_VERSION_MINOR = 0;

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__bool FLAC__ogg_encoder_aspect_init(FLAC__OggEncoderAspect *aspect)
{
	/* we will determine the serial number later if necessary */
	if(ogg_stream_init(&aspect->stream_state, aspect->serial_number) != 0)
		return false;

	aspect->seen_magic = false;
	aspect->is_first_packet = true;
	aspect->samples_written = 0;

	return true;
}

void FLAC__ogg_encoder_aspect_finish(FLAC__OggEncoderAspect *aspect)
{
	(void)ogg_stream_clear(&aspect->stream_state);
	/*@@@ what about the page? */
}

void FLAC__ogg_encoder_aspect_set_serial_number(FLAC__OggEncoderAspect *aspect, long value)
{
	aspect->serial_number = value;
}

FLAC__bool FLAC__ogg_encoder_aspect_set_num_metadata(FLAC__OggEncoderAspect *aspect, unsigned value)
{
	if(value < (1u << FLAC__OGG_MAPPING_NUM_HEADERS_LEN)) {
		aspect->num_metadata = value;
		return true;
	}
	else
		return false;
}

void FLAC__ogg_encoder_aspect_set_defaults(FLAC__OggEncoderAspect *aspect)
{
	aspect->serial_number = 0;
	aspect->num_metadata = 0;
}

/*
 * The basic FLAC -> Ogg mapping goes like this:
 *
 * - 'fLaC' magic and STREAMINFO block get combined into the first
 *   packet.  The packet is prefixed with
 *   + the one-byte packet type 0x7F
 *   + 'FLAC' magic
 *   + the 2 byte Ogg FLAC mapping version number
 *   + tne 2 byte big-endian # of header packets
 * - The first packet is flushed to the first page.
 * - Each subsequent metadata block goes into its own packet.
 * - Each metadata packet is flushed to page (this is not required,
 *   the mapping only requires that a flush must occur after all
 *   metadata is written).
 * - Each subsequent FLAC audio frame goes into its own packet.
 *
 * WATCHOUT:
 * This depends on the behavior of FLAC__StreamEncoder that we get a
 * separate write callback for the fLaC magic, and then separate write
 * callbacks for each metadata block and audio frame.
 */
FLAC__StreamEncoderWriteStatus FLAC__ogg_encoder_aspect_write_callback_wrapper(FLAC__OggEncoderAspect *aspect, const FLAC__byte buffer[], size_t bytes, unsigned samples, unsigned current_frame, FLAC__bool is_last_block, FLAC__OggEncoderAspectWriteCallbackProxy write_callback, void *encoder, void *client_data)
{
	/* WATCHOUT:
	 * This depends on the behavior of FLAC__StreamEncoder that 'samples'
	 * will be 0 for metadata writes.
	 */
	const FLAC__bool is_metadata = (samples == 0);

	/*
	 * Treat fLaC magic packet specially.  We will note when we see it, then
	 * wait until we get the STREAMINFO and prepend it in that packet
	 */
	if(aspect->seen_magic) {
		ogg_packet packet;
		FLAC__byte synthetic_first_packet_body[
			FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH +
			FLAC__OGG_MAPPING_MAGIC_LENGTH +
			FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH +
			FLAC__OGG_MAPPING_VERSION_MINOR_LENGTH +
			FLAC__OGG_MAPPING_NUM_HEADERS_LENGTH +
			FLAC__STREAM_SYNC_LENGTH +
			FLAC__STREAM_METADATA_HEADER_LENGTH +
			FLAC__STREAM_METADATA_STREAMINFO_LENGTH
		];

		memset(&packet, 0, sizeof(packet));
		packet.granulepos = aspect->samples_written + samples;

		if(aspect->is_first_packet) {
			FLAC__byte *b = synthetic_first_packet_body;
			if(bytes != FLAC__STREAM_METADATA_HEADER_LENGTH + FLAC__STREAM_METADATA_STREAMINFO_LENGTH) {
				/*
				 * If we get here, our assumption about the way write callbacks happen
				 * (explained above) is wrong
				 */
				FLAC__ASSERT(0);
				return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
			}
			/* add first header packet type */
			*b = FLAC__OGG_MAPPING_FIRST_HEADER_PACKET_TYPE;
			b += FLAC__OGG_MAPPING_PACKET_TYPE_LENGTH;
			/* add 'FLAC' mapping magic */
			memcpy(b, FLAC__OGG_MAPPING_MAGIC, FLAC__OGG_MAPPING_MAGIC_LENGTH);
			b += FLAC__OGG_MAPPING_MAGIC_LENGTH;
			/* add Ogg FLAC mapping major version number */
			memcpy(b, &FLAC__OGG_MAPPING_VERSION_MAJOR, FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH);
			b += FLAC__OGG_MAPPING_VERSION_MAJOR_LENGTH;
			/* add Ogg FLAC mapping minor version number */
			memcpy(b, &FLAC__OGG_MAPPING_VERSION_MINOR, FLAC__OGG_MAPPING_VERSION_MINOR_LENGTH);
			b += FLAC__OGG_MAPPING_VERSION_MINOR_LENGTH;
			/* add number of header packets */
			*b = (FLAC__byte)(aspect->num_metadata >> 8);
			b++;
			*b = (FLAC__byte)(aspect->num_metadata);
			b++;
			/* add native FLAC 'fLaC' magic */
			memcpy(b, FLAC__STREAM_SYNC_STRING, FLAC__STREAM_SYNC_LENGTH);
			b += FLAC__STREAM_SYNC_LENGTH;
			/* add STREAMINFO */
			memcpy(b, buffer, bytes);
			FLAC__ASSERT(b + bytes - synthetic_first_packet_body == sizeof(synthetic_first_packet_body));
			packet.packet = (unsigned char *)synthetic_first_packet_body;
			packet.bytes = sizeof(synthetic_first_packet_body);

			packet.b_o_s = 1;
			aspect->is_first_packet = false;
		}
		else {
			packet.packet = (unsigned char *)buffer;
			packet.bytes = bytes;
		}

		if(is_last_block) {
			/* we used to check:
			 * FLAC__ASSERT(total_samples_estimate == 0 || total_samples_estimate == aspect->samples_written + samples);
			 * but it's really not useful since total_samples_estimate is an estimate and can be inexact
			 */
			packet.e_o_s = 1;
		}

		if(ogg_stream_packetin(&aspect->stream_state, &packet) != 0)
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

		/*@@@ can't figure out a way to pass a useful number for 'samples' to the write_callback, so we'll just pass 0 */
		if(is_metadata) {
			while(ogg_stream_flush(&aspect->stream_state, &aspect->page) != 0) {
				if(write_callback(encoder, aspect->page.header, aspect->page.header_len, 0, current_frame, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
				if(write_callback(encoder, aspect->page.body, aspect->page.body_len, 0, current_frame, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
			}
		}
		else {
			while(ogg_stream_pageout(&aspect->stream_state, &aspect->page) != 0) {
				if(write_callback(encoder, aspect->page.header, aspect->page.header_len, 0, current_frame, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
				if(write_callback(encoder, aspect->page.body, aspect->page.body_len, 0, current_frame, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
					return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
			}
		}
	}
	else if(is_metadata && current_frame == 0 && samples == 0 && bytes == 4 && 0 == memcmp(buffer, FLAC__STREAM_SYNC_STRING, sizeof(FLAC__STREAM_SYNC_STRING))) {
		aspect->seen_magic = true;
	}
	else {
		/*
		 * If we get here, our assumption about the way write callbacks happen
		 * explained above is wrong
		 */
		FLAC__ASSERT(0);
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	}

	aspect->samples_written += samples;

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}
