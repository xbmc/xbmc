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

#ifndef FLAC__PLUGIN_COMMON__TAGS_H
#define FLAC__PLUGIN_COMMON__TAGS_H

#include "FLAC/format.h"

FLAC__bool FLAC_plugin__tags_get(const char *filename, FLAC__StreamMetadata **tags);
FLAC__bool FLAC_plugin__tags_set(const char *filename, const FLAC__StreamMetadata *tags);

/*
 * Deletes the tags object and sets '*tags' to NULL.
 */
void FLAC_plugin__tags_destroy(FLAC__StreamMetadata **tags);

/*
 * Gets the value (in UTF-8) of the first tag with the given name (NULL if no
 * such tag exists).
 */
const char *FLAC_plugin__tags_get_tag_utf8(const FLAC__StreamMetadata *tags, const char *name);

/*
 * Gets the value (in UCS-2) of the first tag with the given name (NULL if no
 * such tag exists).
 *
 * NOTE: the returned string is malloc()ed and must be free()d by the caller.
 */
FLAC__uint16 *FLAC_plugin__tags_get_tag_ucs2(const FLAC__StreamMetadata *tags, const char *name);

/*
 * Removes all tags with the given 'name'.  Returns the number of tags removed,
 * or -1 on memory allocation error.
 */
int FLAC_plugin__tags_delete_tag(FLAC__StreamMetadata *tags, const char *name);

/*
 * Removes all tags.  Returns the number of tags removed, or -1 on memory
 * allocation error.
 */
int FLAC_plugin__tags_delete_all(FLAC__StreamMetadata *tags);

/*
 * Adds a "name=value" tag to the tags.  'value' must be in UTF-8.  If
 * 'separator' is non-NULL and 'tags' already contains a tag for 'name', the
 * first such tag's value is appended with separator, then value.
 */
FLAC__bool FLAC_plugin__tags_add_tag_utf8(FLAC__StreamMetadata *tags, const char *name, const char *value, const char *separator);

/*
 * Adds a "name=value" tag to the tags.  'value' must be in UCS-2.  If 'tags'
 * already contains a tag or tags for 'name', then they will be replaced
 * according to 'replace_all': if 'replace_all' is false, only the first such
 * tag will be replaced; if true, all matching tags will be replaced by the one
 * new tag. 
 */
FLAC__bool FLAC_plugin__tags_set_tag_ucs2(FLAC__StreamMetadata *tags, const char *name, const FLAC__uint16 *value, FLAC__bool replace_all);

#endif
