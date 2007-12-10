/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
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

#include "vorbiscomment.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "share/grabbag.h" /* for grabbag__file_get_filesize() */
#include "share/utf8.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * This struct and the following 4 static functions are copied from
 * ../metaflac/.  Maybe someday there will be a convenience
 * library for Vorbis comment parsing.
 */
typedef struct {
	char *field; /* the whole field as passed on the command line, i.e. "NAME=VALUE" */
	char *field_name;
	/* according to the vorbis spec, field values can contain \0 so simple C strings are not enough here */
	unsigned field_value_length;
	char *field_value;
	FLAC__bool field_value_from_file; /* true if field_value holds a filename for the value, false for plain value */
} Argument_VcField;

static void die(const char *message)
{
	FLAC__ASSERT(0 != message);
	fprintf(stderr, "ERROR: %s\n", message);
	exit(1);
}

static char *local_strdup(const char *source)
{
	char *ret;
	FLAC__ASSERT(0 != source);
	if(0 == (ret = strdup(source)))
		die("out of memory during strdup()");
	return ret;
}

static FLAC__bool parse_vorbis_comment_field(const char *field_ref, char **field, char **name, char **value, unsigned *length, const char **violation)
{
	static const char * const violations[] = {
		"field name contains invalid character",
		"field contains no '=' character"
	};

	char *p, *q, *s;

	if(0 != field)
		*field = local_strdup(field_ref);

	s = local_strdup(field_ref);

	if(0 == (p = strchr(s, '='))) {
		free(s);
		*violation = violations[1];
		return false;
	}
	*p++ = '\0';

	for(q = s; *q; q++) {
		if(*q < 0x20 || *q > 0x7d || *q == 0x3d) {
			free(s);
			*violation = violations[0];
			return false;
		}
	}

	*name = local_strdup(s);
	*value = local_strdup(p);
	*length = strlen(p);

	free(s);
	return true;
}

/* slight modification: no 'filename' arg, and errors are passed back in 'violation' instead of printed to stderr */
static FLAC__bool set_vc_field(FLAC__StreamMetadata *block, const Argument_VcField *field, FLAC__bool *needs_write, FLAC__bool raw, const char **violation)
{
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	char *converted;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != field);
	FLAC__ASSERT(0 != needs_write);

	if(field->field_value_from_file) {
		/* read the file into 'data' */
		FILE *f = 0;
		char *data = 0;
		const off_t size = grabbag__file_get_filesize(field->field_value);
		if(size < 0) {
			*violation = "can't open file for tag value";
			return false;
		}
		if(size >= 0x100000) { /* magic arbitrary limit, actual format limit is near 16MB */
			*violation = "file for tag value is too large";
			return false;
		}
		if(0 == (data = malloc(size+1)))
			die("out of memory allocating tag value");
		data[size] = '\0';
		if(0 == (f = fopen(field->field_value, "rb")) || fread(data, 1, size, f) != (size_t)size) {
			free(data);
			if(f)
				fclose(f);
			*violation = "error while reading file for tag value";
			return false;
		}
		fclose(f);
		if(strlen(data) != (size_t)size) {
			free(data);
			*violation = "file for tag value has embedded NULs";
			return false;
		}

		/* move 'data' into 'converted', converting to UTF-8 if necessary */
		if(raw) {
			converted = data;
		}
		else if(utf8_encode(data, &converted) >= 0) {
			free(data);
		}
		else {
			free(data);
			*violation = "error converting file contents to UTF-8 for tag value";
			return false;
		}

		/* create and entry and append it */
		if(!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, field->field_name, converted)) {
			free(converted);
			*violation = "file for tag value is not valid UTF-8";
			return false;
		}
		free(converted);
		if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/false)) {
			*violation = "memory allocation failure";
			return false;
		}

		*needs_write = true;
		return true;
	}
	else {
		FLAC__bool needs_free = false;
		if(raw) {
			entry.entry = (FLAC__byte *)field->field;
		}
		else if(utf8_encode(field->field, &converted) >= 0) {
			entry.entry = (FLAC__byte *)converted;
			needs_free = true;
		}
		else {
			*violation = "error converting comment to UTF-8";
			return false;
		}
		entry.length = strlen((const char *)entry.entry);
		if(!FLAC__format_vorbiscomment_entry_is_legal(entry.entry, entry.length)) {
			if(needs_free)
				free(converted);
			/*
			 * our previous parsing has already established that the field
			 * name is OK, so it must be the field value
			 */
			*violation = "tag value for is not valid UTF-8";
			return false;
		}

		if(!FLAC__metadata_object_vorbiscomment_append_comment(block, entry, /*copy=*/true)) {
			if(needs_free)
				free(converted);
			*violation = "memory allocation failure";
			return false;
		}

		*needs_write = true;
		if(needs_free)
			free(converted);
		return true;
	}
}

/*
 * The rest of the code is novel
 */

static void free_field(Argument_VcField *obj)
{
	if(0 != obj->field)
		free(obj->field);
	if(0 != obj->field_name)
		free(obj->field_name);
	if(0 != obj->field_value)
		free(obj->field_value);
}

FLAC__bool flac__vorbiscomment_add(FLAC__StreamMetadata *block, const char *comment, FLAC__bool value_from_file, FLAC__bool raw, const char **violation)
{
	Argument_VcField parsed;
	FLAC__bool dummy;

	FLAC__ASSERT(0 != block);
	FLAC__ASSERT(block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__ASSERT(0 != comment);

	memset(&parsed, 0, sizeof(parsed));

	parsed.field_value_from_file = value_from_file;
	if(!parse_vorbis_comment_field(comment, &(parsed.field), &(parsed.field_name), &(parsed.field_value), &(parsed.field_value_length), violation)) {
		free_field(&parsed);
		return false;
	}

	if(!set_vc_field(block, &parsed, &dummy, raw, violation)) {
		free_field(&parsed);
		return false;
	}
	else {
		free_field(&parsed);
		return true;
	}
}
