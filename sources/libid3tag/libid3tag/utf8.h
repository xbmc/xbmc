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
 * $Id: utf8.h,v 1.7 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_UTF8_H
# define LIBID3TAG_UTF8_H

# include "id3tag.h"

id3_length_t id3_utf8_length(id3_utf8_t const *);
id3_length_t id3_utf8_size(id3_utf8_t const *);

id3_length_t id3_utf8_decodechar(id3_utf8_t const *, id3_ucs4_t *);
id3_length_t id3_utf8_encodechar(id3_utf8_t *, id3_ucs4_t);

void id3_utf8_decode(id3_utf8_t const *, id3_ucs4_t *);
void id3_utf8_encode(id3_utf8_t *, id3_ucs4_t const *);

id3_length_t id3_utf8_put(id3_byte_t **, id3_utf8_t);
id3_utf8_t id3_utf8_get(id3_byte_t const **);

id3_length_t id3_utf8_serialize(id3_byte_t **, id3_ucs4_t const *, int);
id3_ucs4_t *id3_utf8_deserialize(id3_byte_t const **, id3_length_t);

void id3_utf8_free(id3_utf8_t *);

# endif
