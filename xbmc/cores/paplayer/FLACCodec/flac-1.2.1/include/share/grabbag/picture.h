/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2006,2007  Josh Coalson
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

/* This .h cannot be included by itself; #include "share/grabbag.h" instead. */

#ifndef GRABBAG__PICTURE_H
#define GRABBAG__PICTURE_H

#include "FLAC/metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

/* spec should be of the form "[TYPE]|MIME_TYPE|[DESCRIPTION]|[WIDTHxHEIGHTxDEPTH[/COLORS]]|FILE", e.g.
 *   "|image/jpeg|||cover.jpg"
 *   "4|image/jpeg||300x300x24|backcover.jpg"
 *   "|image/png|description|300x300x24/71|cover.png"
 *   "-->|image/gif||300x300x24/71|http://blah.blah.blah/cover.gif"
 *
 * empty type means default to FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER
 * empty resolution spec means to get from the file (cannot get used with "-->" linked images)
 * spec and error_message must not be NULL
 */
FLAC__StreamMetadata *grabbag__picture_parse_specification(const char *spec, const char **error_message);

#ifdef __cplusplus
}
#endif

#endif
