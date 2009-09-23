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

#include "options.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include <string.h>
#include "operations_shorthand.h"

FLAC__bool do_shorthand_operation__streaminfo(const char *filename, FLAC__bool prefix_with_filename, FLAC__Metadata_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	unsigned i;
	FLAC__bool ok = true;
	FLAC__StreamMetadata *block;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	block = FLAC__metadata_iterator_get_block(iterator);

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_STREAMINFO);

	if(prefix_with_filename)
		printf("%s:", filename);

	switch(operation->type) {
		case OP__SHOW_MD5SUM:
			for(i = 0; i < 16; i++)
				printf("%02x", block->data.stream_info.md5sum[i]);
			printf("\n");
			break;
		case OP__SHOW_MIN_BLOCKSIZE:
			printf("%u\n", block->data.stream_info.min_blocksize);
			break;
		case OP__SHOW_MAX_BLOCKSIZE:
			printf("%u\n", block->data.stream_info.max_blocksize);
			break;
		case OP__SHOW_MIN_FRAMESIZE:
			printf("%u\n", block->data.stream_info.min_framesize);
			break;
		case OP__SHOW_MAX_FRAMESIZE:
			printf("%u\n", block->data.stream_info.max_framesize);
			break;
		case OP__SHOW_SAMPLE_RATE:
			printf("%u\n", block->data.stream_info.sample_rate);
			break;
		case OP__SHOW_CHANNELS:
			printf("%u\n", block->data.stream_info.channels);
			break;
		case OP__SHOW_BPS:
			printf("%u\n", block->data.stream_info.bits_per_sample);
			break;
		case OP__SHOW_TOTAL_SAMPLES:
#ifdef _MSC_VER
			printf("%I64u\n", block->data.stream_info.total_samples);
#else
			printf("%llu\n", (unsigned long long)block->data.stream_info.total_samples);
#endif
			break;
		case OP__SET_MD5SUM:
			memcpy(block->data.stream_info.md5sum, operation->argument.streaminfo_md5.value, 16);
			*needs_write = true;
			break;
		case OP__SET_MIN_BLOCKSIZE:
			block->data.stream_info.min_blocksize = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_MAX_BLOCKSIZE:
			block->data.stream_info.max_blocksize = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_MIN_FRAMESIZE:
			block->data.stream_info.min_framesize = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_MAX_FRAMESIZE:
			block->data.stream_info.max_framesize = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_SAMPLE_RATE:
			block->data.stream_info.sample_rate = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_CHANNELS:
			block->data.stream_info.channels = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_BPS:
			block->data.stream_info.bits_per_sample = operation->argument.streaminfo_uint32.value;
			*needs_write = true;
			break;
		case OP__SET_TOTAL_SAMPLES:
			block->data.stream_info.total_samples = operation->argument.streaminfo_uint64.value;
			*needs_write = true;
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	FLAC__metadata_iterator_delete(iterator);

	return ok;
}
