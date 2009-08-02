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

#include <errno.h>
#include <string.h>
#include "options.h"
#include "utils.h"
#include "FLAC/assert.h"
#include "share/grabbag.h" /* for grabbag__picture_parse_specification() etc */

#include "operations_shorthand.h"

static FLAC__bool import_pic_from(const char *filename, FLAC__StreamMetadata **picture, const char *specification, FLAC__bool *needs_write);
static FLAC__bool export_pic_to(const char *filename, const FLAC__StreamMetadata *picture, const char *pic_filename);

FLAC__bool do_shorthand_operation__picture(const char *filename, FLAC__Metadata_Chain *chain, const Operation *operation, FLAC__bool *needs_write)
{
	FLAC__bool ok = true, has_type1 = false, has_type2 = false;
	FLAC__StreamMetadata *picture = 0;
	FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();

	if(0 == iterator)
		die("out of memory allocating iterator");

	FLAC__metadata_iterator_init(iterator, chain);

	switch(operation->type) {
		case OP__IMPORT_PICTURE_FROM:
			ok = import_pic_from(filename, &picture, operation->argument.specification.value, needs_write);
			if(ok) {
				/* append PICTURE block */
				while(FLAC__metadata_iterator_next(iterator))
					;
				if(!FLAC__metadata_iterator_insert_block_after(iterator, picture)) {
					print_error_with_chain_status(chain, "%s: ERROR: adding new PICTURE block to metadata", filename);
					FLAC__metadata_object_delete(picture);
					ok = false;
				}
			}
			if(ok) {
				/* check global PICTURE constraints (max 1 block each of type=1 and type=2) */
				while(FLAC__metadata_iterator_prev(iterator))
					;
				do {
					FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
					if(block->type == FLAC__METADATA_TYPE_PICTURE) {
						if(block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD) {
							if(has_type1) {
								print_error_with_chain_status(chain, "%s: ERROR: FLAC stream can only have one 32x32 standard icon (type=1) PICTURE block", filename);
								ok = false;
							}
							has_type1 = true;
						}
						else if(block->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON) {
							if(has_type2) {
								print_error_with_chain_status(chain, "%s: ERROR: FLAC stream can only have one icon (type=2) PICTURE block", filename);
								ok = false;
							}
							has_type2 = true;
						}
					}
				} while(FLAC__metadata_iterator_next(iterator));
			}
			break;
		case OP__EXPORT_PICTURE_TO:
			{
				const Argument_BlockNumber *a = operation->argument.export_picture_to.block_number_link;
				int block_number = (a && a->num_entries > 0)? (int)a->entries[0] : -1;
				unsigned i = 0;
				do {
					FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
					if(block->type == FLAC__METADATA_TYPE_PICTURE && (block_number < 0 || i == (unsigned)block_number))
						picture = block;
					i++;
				} while(FLAC__metadata_iterator_next(iterator) && 0 == picture);
				if(0 == picture) {
					if(block_number < 0)
						fprintf(stderr, "%s: ERROR: FLAC file has no PICTURE block\n", filename);
					else
						fprintf(stderr, "%s: ERROR: FLAC file has no PICTURE block at block #%d\n", filename, block_number);
					ok = false;
				}
				else
					ok = export_pic_to(filename, picture, operation->argument.filename.value);
			}
			break;
		default:
			ok = false;
			FLAC__ASSERT(0);
			break;
	};

	FLAC__metadata_iterator_delete(iterator);
	return ok;
}

/*
 * local routines
 */

FLAC__bool import_pic_from(const char *filename, FLAC__StreamMetadata **picture, const char *specification, FLAC__bool *needs_write)
{
	const char *error_message;

	if(0 == specification || strlen(specification) == 0) {
		fprintf(stderr, "%s: ERROR: empty picture specification\n", filename);
		return false;
	}

	*picture = grabbag__picture_parse_specification(specification, &error_message);

	if(0 == *picture) {
		fprintf(stderr, "%s: ERROR: while parsing picture specification \"%s\": %s\n", filename, specification, error_message);
		return false;
	}

	if(!FLAC__format_picture_is_legal(&(*picture)->data.picture, &error_message)) {
		fprintf(stderr, "%s: ERROR: new PICTURE block for \"%s\" is illegal: %s\n", filename, specification, error_message);
		return false;
	}

	*needs_write = true;
	return true;
}

FLAC__bool export_pic_to(const char *filename, const FLAC__StreamMetadata *picture, const char *pic_filename)
{
	FILE *f;
	const FLAC__uint32 len = picture->data.picture.data_length;

	if(0 == pic_filename || strlen(pic_filename) == 0) {
		fprintf(stderr, "%s: ERROR: empty export file name\n", filename);
		return false;
	}
	if(0 == strcmp(pic_filename, "-"))
		f = grabbag__file_get_binary_stdout();
	else
		f = fopen(pic_filename, "wb");

	if(0 == f) {
		fprintf(stderr, "%s: ERROR: can't open export file %s: %s\n", filename, pic_filename, strerror(errno));
		return false;
	}

	if(fwrite(picture->data.picture.data, 1, len, f) != len) {
		fprintf(stderr, "%s: ERROR: writing PICTURE data to file\n", filename);
		return false;
	}

	if(f != stdout)
		fclose(f);

	return true;
}
