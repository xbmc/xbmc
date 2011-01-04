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
 * $Id: latin1.c,v 1.10 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>

# include "id3tag.h"
# include "latin1.h"
# include "ucs4.h"

/*
 * NAME:	latin1->length()
 * DESCRIPTION:	return the number of ucs4 chars represented by a latin1 string
 */
id3_length_t id3_latin1_length(id3_latin1_t const *latin1)
{
  id3_latin1_t const *ptr = latin1;

  while (*ptr)
    ++ptr;

  return ptr - latin1;
}

/*
 * NAME:	latin1->size()
 * DESCRIPTION:	return the encoding size of a latin1 string
 */
id3_length_t id3_latin1_size(id3_latin1_t const *latin1)
{
  return id3_latin1_length(latin1) + 1;
}

/*
 * NAME:	latin1->copy()
 * DESCRIPTION:	copy a latin1 string
 */
void id3_latin1_copy(id3_latin1_t *dest, id3_latin1_t const *src)
{
  while ((*dest++ = *src++))
    ;
}

/*
 * NAME:	latin1->duplicate()
 * DESCRIPTION:	duplicate a latin1 string
 */
id3_latin1_t *id3_latin1_duplicate(id3_latin1_t const *src)
{
  id3_latin1_t *latin1;

  latin1 = malloc(id3_latin1_size(src) * sizeof(*latin1));
  if (latin1)
    id3_latin1_copy(latin1, src);

  return latin1;
}

/*
 * NAME:	latin1->ucs4duplicate()
 * DESCRIPTION:	duplicate and decode a latin1 string into ucs4
 */
id3_ucs4_t *id3_latin1_ucs4duplicate(id3_latin1_t const *latin1)
{
  id3_ucs4_t *ucs4;

  ucs4 = malloc((id3_latin1_length(latin1) + 1) * sizeof(*ucs4));
  if (ucs4)
    id3_latin1_decode(latin1, ucs4);

  return ucs4;
}

/*
 * NAME:	latin1->decodechar()
 * DESCRIPTION:	decode a (single) latin1 char into a single ucs4 char
 */
id3_length_t id3_latin1_decodechar(id3_latin1_t const *latin1,
				   id3_ucs4_t *ucs4)
{
  *ucs4 = *latin1;

  return 1;
}

/*
 * NAME:	latin1->encodechar()
 * DESCRIPTION:	encode a single ucs4 char into a (single) latin1 char
 */
id3_length_t id3_latin1_encodechar(id3_latin1_t *latin1, id3_ucs4_t ucs4)
{
  *latin1 = ucs4;
  if (ucs4 > 0x000000ffL)
    *latin1 = ID3_UCS4_REPLACEMENTCHAR;

  return 1;
}

/*
 * NAME:	latin1->decode()
 * DESCRIPTION:	decode a complete latin1 string into a ucs4 string
 */
void id3_latin1_decode(id3_latin1_t const *latin1, id3_ucs4_t *ucs4)
{
  do
    latin1 += id3_latin1_decodechar(latin1, ucs4);
  while (*ucs4++);
}

/*
 * NAME:	latin1->encode()
 * DESCRIPTION:	encode a complete ucs4 string into a latin1 string
 */
void id3_latin1_encode(id3_latin1_t *latin1, id3_ucs4_t const *ucs4)
{
  do
    latin1 += id3_latin1_encodechar(latin1, *ucs4);
  while (*ucs4++);
}

/*
 * NAME:	latin1->put()
 * DESCRIPTION:	serialize a single latin1 character
 */
id3_length_t id3_latin1_put(id3_byte_t **ptr, id3_latin1_t latin1)
{
  if (ptr)
    *(*ptr)++ = latin1;

  return 1;
}

/*
 * NAME:	latin1->get()
 * DESCRIPTION:	deserialize a single latin1 character
 */
id3_latin1_t id3_latin1_get(id3_byte_t const **ptr)
{
  return *(*ptr)++;
}

/*
 * NAME:	latin1->serialize()
 * DESCRIPTION:	serialize a ucs4 string using latin1 encoding
 */
id3_length_t id3_latin1_serialize(id3_byte_t **ptr, id3_ucs4_t const *ucs4,
				  int terminate)
{
  id3_length_t size = 0;
  id3_latin1_t latin1[1], *out;

  while (*ucs4) {
    switch (id3_latin1_encodechar(out = latin1, *ucs4++)) {
    case 1: size += id3_latin1_put(ptr, *out++);
    case 0: break;
    }
  }

  if (terminate)
    size += id3_latin1_put(ptr, 0);

  return size;
}

/*
 * NAME:	latin1->deserialize()
 * DESCRIPTION:	deserialize a ucs4 string using latin1 encoding
 */
id3_ucs4_t *id3_latin1_deserialize(id3_byte_t const **ptr, id3_length_t length)
{
  id3_byte_t const *end;
  id3_latin1_t *latin1ptr, *latin1;
  id3_ucs4_t *ucs4;

  end = *ptr + length;

  latin1 = malloc((length + 1) * sizeof(*latin1));
  if (latin1 == 0)
    return 0;

  latin1ptr = latin1;
  while (end - *ptr > 0 && (*latin1ptr = id3_latin1_get(ptr)))
    ++latin1ptr;

  *latin1ptr = 0;

  ucs4 = malloc((id3_latin1_length(latin1) + 1) * sizeof(*ucs4));
  if (ucs4)
    id3_latin1_decode(latin1, ucs4);

  free(latin1);

  return ucs4;
}

/*
 * NAME:	latin1->free()
 * DESCRIPTION:	free memory of a string
 */
void id3_latin1_free(id3_latin1_t* latin1)
{
  free(latin1);
}
