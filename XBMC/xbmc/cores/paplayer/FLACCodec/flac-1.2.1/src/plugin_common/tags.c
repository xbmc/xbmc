/* plugin_common - Routines common to several plugins
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tags.h"
#include "FLAC/assert.h"
#include "FLAC/metadata.h"
#include "share/alloc.h"

#ifndef FLaC__INLINE
#define FLaC__INLINE
#endif


static FLaC__INLINE size_t local__wide_strlen(const FLAC__uint16 *s)
{
	size_t n = 0;
	while(*s++)
		n++;
	return n;
}

/*
 * also disallows non-shortest-form encodings, c.f.
 *   http://www.unicode.org/versions/corrigendum1.html
 * and a more clear explanation at the end of this section:
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
 */
static FLaC__INLINE size_t local__utf8len(const FLAC__byte *utf8)
{
	FLAC__ASSERT(0 != utf8);
	if ((utf8[0] & 0x80) == 0) {
		return 1;
	}
	else if ((utf8[0] & 0xE0) == 0xC0 && (utf8[1] & 0xC0) == 0x80) {
		if ((utf8[0] & 0xFE) == 0xC0) /* overlong sequence check */
			return 0;
		return 2;
	}
	else if ((utf8[0] & 0xF0) == 0xE0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80) {
		if (utf8[0] == 0xE0 && (utf8[1] & 0xE0) == 0x80) /* overlong sequence check */
			return 0;
		/* illegal surrogates check (U+D800...U+DFFF and U+FFFE...U+FFFF) */
		if (utf8[0] == 0xED && (utf8[1] & 0xE0) == 0xA0) /* D800-DFFF */
			return 0;
		if (utf8[0] == 0xEF && utf8[1] == 0xBF && (utf8[2] & 0xFE) == 0xBE) /* FFFE-FFFF */
			return 0;
		return 3;
	}
	else if ((utf8[0] & 0xF8) == 0xF0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80) {
		if (utf8[0] == 0xF0 && (utf8[1] & 0xF0) == 0x80) /* overlong sequence check */
			return 0;
		return 4;
	}
	else if ((utf8[0] & 0xFC) == 0xF8 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80 && (utf8[4] & 0xC0) == 0x80) {
		if (utf8[0] == 0xF8 && (utf8[1] & 0xF8) == 0x80) /* overlong sequence check */
			return 0;
		return 5;
	}
	else if ((utf8[0] & 0xFE) == 0xFC && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80 && (utf8[4] & 0xC0) == 0x80 && (utf8[5] & 0xC0) == 0x80) {
		if (utf8[0] == 0xFC && (utf8[1] & 0xFC) == 0x80) /* overlong sequence check */
			return 0;
		return 6;
	}
	else {
		return 0;
	}
}


static FLaC__INLINE size_t local__utf8_to_ucs2(const FLAC__byte *utf8, FLAC__uint16 *ucs2)
{
	const size_t len = local__utf8len(utf8);

	FLAC__ASSERT(0 != ucs2);

	if (len == 1)
		*ucs2 = *utf8;
	else if (len == 2)
		*ucs2 = (*utf8 & 0x3F)<<6 | (*(utf8+1) & 0x3F);
	else if (len == 3)
		*ucs2 = (*utf8 & 0x1F)<<12 | (*(utf8+1) & 0x3F)<<6 | (*(utf8+2) & 0x3F);
	else
		*ucs2 = '?';

	return len;
}

static FLAC__uint16 *local__convert_utf8_to_ucs2(const char *src, unsigned length)
{
	FLAC__uint16 *out;
	size_t chars = 0;

	FLAC__ASSERT(0 != src);

	/* calculate length */
	{
		const unsigned char *s, *end;
		for (s=(const unsigned char *)src, end=s+length; s<end; chars++) {
			const unsigned n = local__utf8len(s);
			if (n == 0)
				return 0;
			s += n;
		}
		FLAC__ASSERT(s == end);
	}

	/* allocate */
	out = (FLAC__uint16*)safe_malloc_mul_2op_(chars, /*times*/sizeof(FLAC__uint16));
	if (0 == out) {
		FLAC__ASSERT(0);
		return 0;
	}

	/* convert */
	{
		const unsigned char *s = (const unsigned char *)src;
		FLAC__uint16 *u = out;
		for ( ; chars; chars--)
			s += local__utf8_to_ucs2(s, u++);
	}

	return out;
}

static FLaC__INLINE size_t local__ucs2len(FLAC__uint16 ucs2)
{
	if (ucs2 < 0x0080)
		return 1;
	else if (ucs2 < 0x0800)
		return 2;
	else
		return 3;
}

static FLaC__INLINE size_t local__ucs2_to_utf8(FLAC__uint16 ucs2, FLAC__byte *utf8)
{
	if (ucs2 < 0x080) {
		utf8[0] = (FLAC__byte)ucs2;
		return 1;
	}
	else if (ucs2 < 0x800) {
		utf8[0] = 0xc0 | (ucs2 >> 6);
		utf8[1] = 0x80 | (ucs2 & 0x3f);
		return 2;
	}
	else {
		utf8[0] = 0xe0 | (ucs2 >> 12);
		utf8[1] = 0x80 | ((ucs2 >> 6) & 0x3f);
		utf8[2] = 0x80 | (ucs2 & 0x3f);
		return 3;
	}
}

static char *local__convert_ucs2_to_utf8(const FLAC__uint16 *src, unsigned length)
{
	char *out;
	size_t len = 0, n;

	FLAC__ASSERT(0 != src);

	/* calculate length */
	{
		unsigned i;
		for (i = 0; i < length; i++) {
			n = local__ucs2len(src[i]);
			if(len + n < len) /* overflow check */
				return 0;
			len += n;
		}
	}

	/* allocate */
	out = (char*)safe_malloc_mul_2op_(len, /*times*/sizeof(char));
	if (0 == out)
		return 0;

	/* convert */
	{
		unsigned char *u = (unsigned char *)out;
		for ( ; *src; src++)
			u += local__ucs2_to_utf8(*src, u);
		local__ucs2_to_utf8(*src, u);
	}

	return out;
}


FLAC__bool FLAC_plugin__tags_get(const char *filename, FLAC__StreamMetadata **tags)
{
	if(!FLAC__metadata_get_tags(filename, tags))
		if(0 == (*tags = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT)))
			return false;
	return true;
}

FLAC__bool FLAC_plugin__tags_set(const char *filename, const FLAC__StreamMetadata *tags)
{
	FLAC__Metadata_Chain *chain;
	FLAC__Metadata_Iterator *iterator;
	FLAC__StreamMetadata *block;
	FLAC__bool got_vorbis_comments = false;
	FLAC__bool ok;

	if(0 == (chain = FLAC__metadata_chain_new()))
		return false;

	if(!FLAC__metadata_chain_read(chain, filename)) {
		FLAC__metadata_chain_delete(chain);
		return false;
	}

	if(0 == (iterator = FLAC__metadata_iterator_new())) {
		FLAC__metadata_chain_delete(chain);
		return false;
	}

	FLAC__metadata_iterator_init(iterator, chain);

	do {
		if(FLAC__metadata_iterator_get_block_type(iterator) == FLAC__METADATA_TYPE_VORBIS_COMMENT)
			got_vorbis_comments = true;
	} while(!got_vorbis_comments && FLAC__metadata_iterator_next(iterator));

	if(0 == (block = FLAC__metadata_object_clone(tags))) {
		FLAC__metadata_chain_delete(chain);
		FLAC__metadata_iterator_delete(iterator);
		return false;
	}

	if(got_vorbis_comments)
		ok = FLAC__metadata_iterator_set_block(iterator, block);
	else
		ok = FLAC__metadata_iterator_insert_block_after(iterator, block);

	FLAC__metadata_iterator_delete(iterator);

	if(ok) {
		FLAC__metadata_chain_sort_padding(chain);
		ok = FLAC__metadata_chain_write(chain, /*use_padding=*/true, /*preserve_file_stats=*/true);
	}

	FLAC__metadata_chain_delete(chain);

	return ok;
}

void FLAC_plugin__tags_destroy(FLAC__StreamMetadata **tags)
{
	FLAC__metadata_object_delete(*tags);
	*tags = 0;
}

const char *FLAC_plugin__tags_get_tag_utf8(const FLAC__StreamMetadata *tags, const char *name)
{
	const int i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, name);
	return (i < 0? 0 : strchr((const char *)tags->data.vorbis_comment.comments[i].entry, '=')+1);
}

FLAC__uint16 *FLAC_plugin__tags_get_tag_ucs2(const FLAC__StreamMetadata *tags, const char *name)
{
	const char *utf8 = FLAC_plugin__tags_get_tag_utf8(tags, name);
	if(0 == utf8)
		return 0;
	return local__convert_utf8_to_ucs2(utf8, strlen(utf8)+1); /* +1 for terminating null */
}

int FLAC_plugin__tags_delete_tag(FLAC__StreamMetadata *tags, const char *name)
{
	return FLAC__metadata_object_vorbiscomment_remove_entries_matching(tags, name);
}

int FLAC_plugin__tags_delete_all(FLAC__StreamMetadata *tags)
{
	int n = (int)tags->data.vorbis_comment.num_comments;
	if(n > 0) {
		if(!FLAC__metadata_object_vorbiscomment_resize_comments(tags, 0))
			n = -1;
	}
	return n;
}

FLAC__bool FLAC_plugin__tags_add_tag_utf8(FLAC__StreamMetadata *tags, const char *name, const char *value, const char *separator)
{
	int i;

	FLAC__ASSERT(0 != tags);
	FLAC__ASSERT(0 != name);
	FLAC__ASSERT(0 != value);

	if(separator && (i = FLAC__metadata_object_vorbiscomment_find_entry_from(tags, /*offset=*/0, name)) >= 0) {
		FLAC__StreamMetadata_VorbisComment_Entry *entry = tags->data.vorbis_comment.comments+i;
		const size_t value_len = strlen(value);
		const size_t separator_len = strlen(separator);
		FLAC__byte *new_entry;
		if(0 == (new_entry = (FLAC__byte*)safe_realloc_add_4op_(entry->entry, entry->length, /*+*/value_len, /*+*/separator_len, /*+*/1)))
			return false;
		memcpy(new_entry+entry->length, separator, separator_len);
		entry->length += separator_len;
		memcpy(new_entry+entry->length, value, value_len);
		entry->length += value_len;
		new_entry[entry->length] = '\0';
		entry->entry = new_entry;
	}
	else {
		FLAC__StreamMetadata_VorbisComment_Entry entry;
		if(!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, name, value))
			return false;
		FLAC__metadata_object_vorbiscomment_append_comment(tags, entry, /*copy=*/false);
	}
	return true;
}

FLAC__bool FLAC_plugin__tags_set_tag_ucs2(FLAC__StreamMetadata *tags, const char *name, const FLAC__uint16 *value, FLAC__bool replace_all)
{
	FLAC__StreamMetadata_VorbisComment_Entry entry;

	FLAC__ASSERT(0 != tags);
	FLAC__ASSERT(0 != name);
	FLAC__ASSERT(0 != value);

	{
		char *utf8 = local__convert_ucs2_to_utf8(value, local__wide_strlen(value)+1); /* +1 for the terminating null */
		if(0 == utf8)
			return false;
		if(!FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, name, utf8)) {
			free(utf8);
			return false;
		}
		free(utf8);
	}
	if(!FLAC__metadata_object_vorbiscomment_replace_comment(tags, entry, replace_all, /*copy=*/false))
		return false;
	return true;
}
