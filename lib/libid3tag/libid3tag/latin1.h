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
 * $Id: latin1.h,v 1.8 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBID3TAG_LATIN1_H
# define LIBID3TAG_LATIN1_H

# include "id3tag.h"

id3_length_t id3_latin1_length(id3_latin1_t const *);
id3_length_t id3_latin1_size(id3_latin1_t const *);

void id3_latin1_copy(id3_latin1_t *, id3_latin1_t const *);
id3_latin1_t *id3_latin1_duplicate(id3_latin1_t const *);

id3_length_t id3_latin1_decodechar(id3_latin1_t const *, id3_ucs4_t *);
id3_length_t id3_latin1_encodechar(id3_latin1_t *, id3_ucs4_t);

void id3_latin1_decode(id3_latin1_t const *, id3_ucs4_t *);
void id3_latin1_encode(id3_latin1_t *, id3_ucs4_t const *);

id3_length_t id3_latin1_put(id3_byte_t **, id3_latin1_t);
id3_latin1_t id3_latin1_get(id3_byte_t const **);

id3_length_t id3_latin1_serialize(id3_byte_t **, id3_ucs4_t const *, int);
id3_ucs4_t *id3_latin1_deserialize(id3_byte_t const **, id3_length_t);
void id3_latin1_free(id3_latin1_t*);

# endif
