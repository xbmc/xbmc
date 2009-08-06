/* metaflac - Command-line FLAC metadata editor
 * Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#include "utils.h"
#include "FLAC/assert.h"
#include "FLAC/stream_decoder.h"
#include "FLAC/metadata.h"
#include "share/grabbag.h"
#include "operations_shorthand.h"

static FLAC__bool populate_seekpoint_values(const char *filename, FLAC__StreamMetadata *block, FLAC__bool *needs_write);

FLAC__bool do_shorthand_operation__add_seekpoints(const char *filename, FLAC__Metadata_Chain *chain, const char *specification, FLAC__bool *needs_write)
{
	FLAC__bool ok = true, found_seektable_block = false;
	FLAC__StreamMetadata *block = 0;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
	FLAC__uint64 total_samples = 0;
	unsigned sample_rate = 0;

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	do {
		block = FLAC__metadata_iterator_get_block(iterator);
		if(block->type == FLAC__METADATA_TYPE_STREAMINFO) {
			sample_rate = block->data.stream_info.sample_rate;
			total_samples = block->data.stream_info.total_samples;
		}
		else if(block->type == FLAC__METADATA_TYPE_SEEKTABLE)
			found_seektable_block = true;
	} while(!found_seektable_block && FLAC__metadata_iterator_next(iterator));

	if(total_samples == 0) {
		fprintf(stderr, "%s: ERROR: cannot add seekpoints because STREAMINFO block does not specify total_samples\n", filename);
		return false;
	}

	if(!found_seektable_block) {
		/* create a new block */
		block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_SEEKTABLE);
		if(0 == block)
			die("out of memory allocating SEEKTABLE block");
		while(FLAC__metadata_iterator_prev(iterator))
			;
		if(!FLAC__metadata_iterator_insert_block_after(iterator, block)) {
			print_error_with_chain_status(chain, "%s: ERROR: adding new SEEKTABLE block to metadata", filename);
			FLAC__metadata_object_delete(block);
			return false;
		}
		/* iterator is left pointing to new block */
		FLAC__ASSERT(FLAC__metadata_iterator_get_block(iterator) == block);
	}

	FLAC__metadata_iterator_delete(iterator);

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_SEEKTABLE);

	if(!grabbag__seektable_convert_specification_to_template(specification, /*only_explicit_placeholders=*/false, total_samples, sample_rate, block, /*spec_has_real_points=*/0)) {
		fprintf(stderr, "%s: ERROR (internal) preparing seektable with seekpoints\n", filename);
		return false;
	}

	ok = populate_seekpoint_values(filename, block, needs_write);

	if(ok)
		(void) FLAC__format_seektable_sort(&block->data.seek_table);

	return ok;
}

/*
 * local routines
 */

typedef struct {
	FLAC__StreamMetadata_SeekTable *seektable_template;
	FLAC__uint64 samples_written;
	FLAC__uint64 audio_offset, last_offset;
	unsigned first_seekpoint_to_check;
	FLAC__bool error_occurred;
	FLAC__StreamDecoderErrorStatus error_status;
} ClientData;

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	ClientData *cd = (ClientData*)client_data;

	(void)buffer;
	FLAC__ASSERT(0 != cd);

	if(!cd->error_occurred) {
		const unsigned blocksize = frame->header.blocksize;
		const FLAC__uint64 frame_first_sample = cd->samples_written;
		const FLAC__uint64 frame_last_sample = frame_first_sample + (FLAC__uint64)blocksize - 1;
		FLAC__uint64 test_sample;
		unsigned i;
		for(i = cd->first_seekpoint_to_check; i < cd->seektable_template->num_points; i++) {
			test_sample = cd->seektable_template->points[i].sample_number;
			if(test_sample > frame_last_sample) {
				break;
			}
			else if(test_sample >= frame_first_sample) {
				cd->seektable_template->points[i].sample_number = frame_first_sample;
				cd->seektable_template->points[i].stream_offset = cd->last_offset - cd->audio_offset;
				cd->seektable_template->points[i].frame_samples = blocksize;
				cd->first_seekpoint_to_check++;
				/* DO NOT: "break;" and here's why:
				 * The seektable template may contain more than one target
				 * sample for any given frame; we will keep looping, generating
				 * duplicate seekpoints for them, and we'll clean it up later,
				 * just before writing the seektable back to the metadata.
				 */
			}
			else {
				cd->first_seekpoint_to_check++;
			}
		}
		cd->samples_written += blocksize;
		if(!FLAC__stream_decoder_get_decode_position(decoder, &cd->last_offset))
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
}

static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	ClientData *cd = (ClientData*)client_data;

	(void)decoder;
	FLAC__ASSERT(0 != cd);

	if(!cd->error_occurred) { /* don't let multiple errors overwrite the first one */
		cd->error_occurred = true;
		cd->error_status = status;
	}
}

FLAC__bool populate_seekpoint_values(const char *filename, FLAC__StreamMetadata *block, FLAC__bool *needs_write)
{
	FLAC__StreamDecoder *decoder;
	ClientData client_data;
	FLAC__bool ok = true;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_SEEKTABLE);

	client_data.seektable_template = &block->data.seek_table;
	client_data.samples_written = 0;
	/* client_data.audio_offset must be determined later */
	client_data.first_seekpoint_to_check = 0;
	client_data.error_occurred = false;

	decoder = FLAC__stream_decoder_new();

	if(0 == decoder) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) creating the decoder instance\n", filename);
		return false;
	}

	FLAC__stream_decoder_set_md5_checking(decoder, false);
	FLAC__stream_decoder_set_metadata_ignore_all(decoder);

	if(FLAC__stream_decoder_init_file(decoder, filename, write_callback_, /*metadata_callback=*/0, error_callback_, &client_data) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) initializing the decoder instance (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
		ok = false;
	}

	if(ok && !FLAC__stream_decoder_process_until_end_of_metadata(decoder)) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
		ok = false;
	}

	if(ok && !FLAC__stream_decoder_get_decode_position(decoder, &client_data.audio_offset)) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file\n", filename);
		ok = false;
	}
	client_data.last_offset = client_data.audio_offset;

	if(ok && !FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%s)\n", filename, FLAC__stream_decoder_get_resolved_state_string(decoder));
		ok = false;
	}

	if(ok && client_data.error_occurred) {
		fprintf(stderr, "%s: ERROR (--add-seekpoint) decoding file (%u:%s)\n", filename, (unsigned)client_data.error_status, FLAC__StreamDecoderErrorStatusString[client_data.error_status]);
		ok = false;
	}

	*needs_write = true;
	FLAC__stream_decoder_delete(decoder);
	return ok;
}
