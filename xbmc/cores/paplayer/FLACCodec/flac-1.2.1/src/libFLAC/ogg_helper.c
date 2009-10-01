/* libFLAC - Free Lossless Audio Codec
 * Copyright (C) 2004,2005,2006,2007  Josh Coalson
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

#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcmp(), memcpy() */
#include "FLAC/assert.h"
#include "share/alloc.h"
#include "private/ogg_helper.h"
#include "protected/stream_encoder.h"


static FLAC__bool full_read_(FLAC__StreamEncoder *encoder, FLAC__byte *buffer, size_t bytes, FLAC__StreamEncoderReadCallback read_callback, void *client_data)
{
	while(bytes > 0) {
		size_t bytes_read = bytes;
		switch(read_callback(encoder, buffer, &bytes_read, client_data)) {
			case FLAC__STREAM_ENCODER_READ_STATUS_CONTINUE:
				bytes -= bytes_read;
				buffer += bytes_read;
				break;
			case FLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM:
				if(bytes_read == 0) {
					encoder->protected_->state = FLAC__STREAM_ENCODER_OGG_ERROR;
					return false;
				}
				bytes -= bytes_read;
				buffer += bytes_read;
				break;
			case FLAC__STREAM_ENCODER_READ_STATUS_ABORT:
				encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
				return false;
			case FLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED:
				return false;
			default:
				/* double protection: */
				FLAC__ASSERT(0);
				encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
				return false;
		}
	}

	return true;
}

void simple_ogg_page__init(ogg_page *page)
{
	page->header = 0;
	page->header_len = 0;
	page->body = 0;
	page->body_len = 0;
}

void simple_ogg_page__clear(ogg_page *page)
{
	if(page->header)
		free(page->header);
	if(page->body)
		free(page->body);
	simple_ogg_page__init(page);
}

FLAC__bool simple_ogg_page__get_at(FLAC__StreamEncoder *encoder, FLAC__uint64 position, ogg_page *page, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderReadCallback read_callback, void *client_data)
{
	static const unsigned OGG_HEADER_FIXED_PORTION_LEN = 27;
	static const unsigned OGG_MAX_HEADER_LEN = 27/*OGG_HEADER_FIXED_PORTION_LEN*/ + 255;
	FLAC__byte crc[4];
	FLAC__StreamEncoderSeekStatus seek_status;

	FLAC__ASSERT(page->header == 0);
	FLAC__ASSERT(page->header_len == 0);
	FLAC__ASSERT(page->body == 0);
	FLAC__ASSERT(page->body_len == 0);

	/* move the stream pointer to the supposed beginning of the page */
	if(0 == seek_callback)
		return false;
	if((seek_status = seek_callback((FLAC__StreamEncoder*)encoder, position, client_data)) != FLAC__STREAM_ENCODER_SEEK_STATUS_OK) {
		if(seek_status == FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR)
			encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
		return false;
	}

	/* allocate space for the page header */
	if(0 == (page->header = (unsigned char *)safe_malloc_(OGG_MAX_HEADER_LEN))) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	/* read in the fixed part of the page header (up to but not including
	 * the segment table */
	if(!full_read_(encoder, page->header, OGG_HEADER_FIXED_PORTION_LEN, read_callback, client_data))
		return false;

	page->header_len = OGG_HEADER_FIXED_PORTION_LEN + page->header[26];

	/* check to see if it's a correct, "simple" page (one packet only) */
	if(
		memcmp(page->header, "OggS", 4) ||               /* doesn't start with OggS */
		(page->header[5] & 0x01) ||                      /* continued packet */
		memcmp(page->header+6, "\0\0\0\0\0\0\0\0", 8) || /* granulepos is non-zero */
		page->header[26] == 0                            /* packet is 0-size */
	) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_OGG_ERROR;
		return false;
	}

	/* read in the segment table */
	if(!full_read_(encoder, page->header + OGG_HEADER_FIXED_PORTION_LEN, page->header[26], read_callback, client_data))
		return false;

	{
		unsigned i;

		/* check to see that it specifies a single packet */
		for(i = 0; i < (unsigned)page->header[26] - 1; i++) {
			if(page->header[i + OGG_HEADER_FIXED_PORTION_LEN] != 255) {
				encoder->protected_->state = FLAC__STREAM_ENCODER_OGG_ERROR;
				return false;
			}
		}

		page->body_len = 255 * i + page->header[i + OGG_HEADER_FIXED_PORTION_LEN];
	}

	/* allocate space for the page body */
	if(0 == (page->body = (unsigned char *)safe_malloc_(page->body_len))) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	/* read in the page body */
	if(!full_read_(encoder, page->body, page->body_len, read_callback, client_data))
		return false;

	/* check the CRC */
	memcpy(crc, page->header+22, 4);
	ogg_page_checksum_set(page);
	if(memcmp(crc, page->header+22, 4)) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_OGG_ERROR;
		return false;
	}

	return true;
}

FLAC__bool simple_ogg_page__set_at(FLAC__StreamEncoder *encoder, FLAC__uint64 position, ogg_page *page, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderWriteCallback write_callback, void *client_data)
{
	FLAC__StreamEncoderSeekStatus seek_status;

	FLAC__ASSERT(page->header != 0);
	FLAC__ASSERT(page->header_len != 0);
	FLAC__ASSERT(page->body != 0);
	FLAC__ASSERT(page->body_len != 0);

	/* move the stream pointer to the supposed beginning of the page */
	if(0 == seek_callback)
		return false;
	if((seek_status = seek_callback((FLAC__StreamEncoder*)encoder, position, client_data)) != FLAC__STREAM_ENCODER_SEEK_STATUS_OK) {
		if(seek_status == FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR)
			encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
		return false;
	}

	ogg_page_checksum_set(page);

	/* re-write the page */
	if(write_callback((FLAC__StreamEncoder*)encoder, page->header, page->header_len, 0, 0, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
		return false;
	}
	if(write_callback((FLAC__StreamEncoder*)encoder, page->body, page->body_len, 0, 0, client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		encoder->protected_->state = FLAC__STREAM_ENCODER_CLIENT_ERROR;
		return false;
	}

	return true;
}
