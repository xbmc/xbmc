/*
 * libid3tag - ID3 tag manipulation library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: util.c,v 1.9 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>
# include <zlib.h>

# include "id3tag.h"
# include "util.h"

/*
 * NAME:	util->unsynchronise()
 * DESCRIPTION:	perform (in-place) unsynchronisation
 */
id3_length_t id3_util_unsynchronise(id3_byte_t *data, id3_length_t length)
{
  id3_length_t bytes = 0, count;
  id3_byte_t *end = data + length;
  id3_byte_t const *ptr;

  if (length == 0)
    return 0;

  for (ptr = data; ptr < end - 1; ++ptr) {
    if (ptr[0] == 0xff && (ptr[1] == 0x00 || (ptr[1] & 0xe0) == 0xe0))
      ++bytes;
  }

  if (bytes) {
    ptr  = end;
    end += bytes;

    *--end = *--ptr;

    for (count = bytes; count; *--end = *--ptr) {
      if (ptr[-1] == 0xff && (ptr[0] == 0x00 || (ptr[0] & 0xe0) == 0xe0)) {
	*--end = 0x00;
	--count;
      }
    }
  }

  return length + bytes;
}

/*
 * NAME:	util->deunsynchronise()
 * DESCRIPTION:	undo unsynchronisation (in-place)
 */
id3_length_t id3_util_deunsynchronise(id3_byte_t *data, id3_length_t length)
{
  id3_byte_t const *old, *end = data + length;
  id3_byte_t *new;

  if (length == 0)
    return 0;

  for (old = new = data; old < end - 1; ++old) {
    *new++ = *old;
    if (old[0] == 0xff && old[1] == 0x00)
      ++old;
  }

  *new++ = *old;

  return new - data;
}

/*
 * NAME:	util->compress()
 * DESCRIPTION:	perform zlib deflate method compression
 */
id3_byte_t *id3_util_compress(id3_byte_t const *data, id3_length_t length,
			      id3_length_t *newlength)
{
  id3_byte_t *compressed;

  *newlength  = length + 12;
  *newlength += *newlength / 1000;

  compressed = malloc(*newlength);
  if (compressed) {
    if (compress2(compressed, newlength, data, length,
		  Z_BEST_COMPRESSION) != Z_OK ||
	*newlength >= length) {
      free(compressed);
      compressed = 0;
    }
    else {
      id3_byte_t *resized;

      resized = realloc(compressed, *newlength ? *newlength : 1);
      if (resized)
	compressed = resized;
    }
  }

  return compressed;
}

/*
 * NAME:	util->decompress()
 * DESCRIPTION:	undo zlib deflate method compression
 */
id3_byte_t *id3_util_decompress(id3_byte_t const *data, id3_length_t length,
				id3_length_t newlength)
{
  id3_byte_t *decompressed;

  decompressed = malloc(newlength ? newlength : 1);
  if (decompressed) {
    id3_length_t size;

    size = newlength;

    if (uncompress(decompressed, &size, data, length) != Z_OK ||
	size != newlength) {
      free(decompressed);
      decompressed = 0;
    }
  }

  return decompressed;
}
