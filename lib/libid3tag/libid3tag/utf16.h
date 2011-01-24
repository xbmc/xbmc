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
 * $Id: utf16.h,v 1.8 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_UTF16_H
# define LIBID3TAG_UTF16_H

# include "id3tag.h"

enum id3_utf16_byteorder {
  ID3_UTF16_BYTEORDER_ANY,
  ID3_UTF16_BYTEORDER_BE,
  ID3_UTF16_BYTEORDER_LE
};

id3_length_t id3_utf16_length(id3_utf16_t const *);
id3_length_t id3_utf16_size(id3_utf16_t const *);

id3_length_t id3_utf16_decodechar(id3_utf16_t const *, id3_ucs4_t *);
id3_length_t id3_utf16_encodechar(id3_utf16_t *, id3_ucs4_t);

void id3_utf16_decode(id3_utf16_t const *, id3_ucs4_t *);
void id3_utf16_encode(id3_utf16_t *, id3_ucs4_t const *);

id3_length_t id3_utf16_put(id3_byte_t **, id3_utf16_t,
			   enum id3_utf16_byteorder);
id3_utf16_t id3_utf16_get(id3_byte_t const **, enum id3_utf16_byteorder);

id3_length_t id3_utf16_serialize(id3_byte_t **, id3_ucs4_t const *,
				 enum id3_utf16_byteorder, int);
id3_ucs4_t *id3_utf16_deserialize(id3_byte_t const **, id3_length_t,
				  enum id3_utf16_byteorder);

void id3_utf16_free(id3_utf16_t*);

# endif
