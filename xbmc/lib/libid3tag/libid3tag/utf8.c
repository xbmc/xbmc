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
 * $Id: utf8.c,v 1.9 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>

# include "id3tag.h"
# include "utf8.h"
# include "ucs4.h"

/*
 * NAME:	utf8->length()
 * DESCRIPTION:	return the number of ucs4 chars represented by a utf8 string
 */
id3_length_t id3_utf8_length(id3_utf8_t const *utf8)
{
  id3_length_t length = 0;

  while (*utf8) {
    if ((utf8[0] & 0x80) == 0x00)
      ++length;
    else if ((utf8[0] & 0xe0) == 0xc0 &&
	     (utf8[1] & 0xc0) == 0x80) {
      if (((utf8[0] & 0x1fL) << 6) >= 0x00000080L) {
	++length;
	utf8 += 1;
      }
    }
    else if ((utf8[0] & 0xf0) == 0xe0 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x0fL) << 12) |
	   ((utf8[1] & 0x3fL) <<  6)) >= 0x00000800L) {
	++length;
	utf8 += 2;
      }
    }
    else if ((utf8[0] & 0xf8) == 0xf0 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x07L) << 18) |
	   ((utf8[1] & 0x3fL) << 12)) >= 0x00010000L) {
	++length;
	utf8 += 3;
      }
    }
    else if ((utf8[0] & 0xfc) == 0xf8 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80 &&
	     (utf8[4] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x03L) << 24) |
	   ((utf8[0] & 0x3fL) << 18)) >= 0x00200000L) {
	++length;
	utf8 += 4;
      }
    }
    else if ((utf8[0] & 0xfe) == 0xfc &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80 &&
	     (utf8[4] & 0xc0) == 0x80 &&
	     (utf8[5] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x01L) << 30) |
	   ((utf8[0] & 0x3fL) << 24)) >= 0x04000000L) {
	++length;
	utf8 += 5;
      }
    }

    ++utf8;
  }

  return length;
}

/*
 * NAME:	utf8->size()
 * DESCRIPTION:	return the encoding size of a utf8 string
 */
id3_length_t id3_utf8_size(id3_utf8_t const *utf8)
{
  id3_utf8_t const *ptr = utf8;

  while (*ptr)
    ++ptr;

  return ptr - utf8 + 1;
}

/*
 * NAME:	utf8->ucs4duplicate()
 * DESCRIPTION:	duplicate and decode a utf8 string into ucs4
 */
id3_ucs4_t *id3_utf8_ucs4duplicate(id3_utf8_t const *utf8)
{
  id3_ucs4_t *ucs4;

  ucs4 = malloc((id3_utf8_length(utf8) + 1) * sizeof(*ucs4));
  if (ucs4)
    id3_utf8_decode(utf8, ucs4);

  return release(ucs4);
}

/*
 * NAME:	utf8->decodechar()
 * DESCRIPTION:	decode a series of utf8 chars into a single ucs4 char
 */
id3_length_t id3_utf8_decodechar(id3_utf8_t const *utf8, id3_ucs4_t *ucs4)
{
  id3_utf8_t const *start = utf8;

  while (1) {
    if ((utf8[0] & 0x80) == 0x00) {
      *ucs4 = utf8[0];
      return utf8 - start + 1;
    }
    else if ((utf8[0] & 0xe0) == 0xc0 &&
	     (utf8[1] & 0xc0) == 0x80) {
      *ucs4 =
	((utf8[0] & 0x1fL) << 6) |
	((utf8[1] & 0x3fL) << 0);
      if (*ucs4 >= 0x00000080L)
	return utf8 - start + 2;
    }
    else if ((utf8[0] & 0xf0) == 0xe0 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80) {
      *ucs4 =
	((utf8[0] & 0x0fL) << 12) |
	((utf8[1] & 0x3fL) <<  6) |
	((utf8[2] & 0x3fL) <<  0);
      if (*ucs4 >= 0x00000800L)
	return utf8 - start + 3;
    }
    else if ((utf8[0] & 0xf8) == 0xf0 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80) {
      *ucs4 =
	((utf8[0] & 0x07L) << 18) |
	((utf8[1] & 0x3fL) << 12) |
	((utf8[2] & 0x3fL) <<  6) |
	((utf8[3] & 0x3fL) <<  0);
      if (*ucs4 >= 0x00010000L)
	return utf8 - start + 4;
    }
    else if ((utf8[0] & 0xfc) == 0xf8 &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80 &&
	     (utf8[4] & 0xc0) == 0x80) {
      *ucs4 =
	((utf8[0] & 0x03L) << 24) |
	((utf8[1] & 0x3fL) << 18) |
	((utf8[2] & 0x3fL) << 12) |
	((utf8[3] & 0x3fL) <<  6) |
	((utf8[4] & 0x3fL) <<  0);
      if (*ucs4 >= 0x00200000L)
	return utf8 - start + 5;
    }
    else if ((utf8[0] & 0xfe) == 0xfc &&
	     (utf8[1] & 0xc0) == 0x80 &&
	     (utf8[2] & 0xc0) == 0x80 &&
	     (utf8[3] & 0xc0) == 0x80 &&
	     (utf8[4] & 0xc0) == 0x80 &&
	     (utf8[5] & 0xc0) == 0x80) {
      *ucs4 =
	((utf8[0] & 0x01L) << 30) |
	((utf8[1] & 0x3fL) << 24) |
	((utf8[2] & 0x3fL) << 18) |
	((utf8[3] & 0x3fL) << 12) |
	((utf8[4] & 0x3fL) <<  6) |
	((utf8[5] & 0x3fL) <<  0);
      if (*ucs4 >= 0x04000000L)
	return utf8 - start + 6;
    }

    ++utf8;
  }
}

/*
 * NAME:	utf8->encodechar()
 * DESCRIPTION:	encode a single ucs4 char into a series of up to 6 utf8 chars
 */
id3_length_t id3_utf8_encodechar(id3_utf8_t *utf8, id3_ucs4_t ucs4)
{
  if (ucs4 <= 0x0000007fL) {
    utf8[0] = ucs4;

    return 1;
  }
  else if (ucs4 <= 0x000007ffL) {
    utf8[0] = 0xc0 | ((ucs4 >>  6) & 0x1f);
    utf8[1] = 0x80 | ((ucs4 >>  0) & 0x3f);

    return 2;
  }
  else if (ucs4 <= 0x0000ffffL) {
    utf8[0] = 0xe0 | ((ucs4 >> 12) & 0x0f);
    utf8[1] = 0x80 | ((ucs4 >>  6) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >>  0) & 0x3f);

    return 3;
  }
  else if (ucs4 <= 0x001fffffL) {
    utf8[0] = 0xf0 | ((ucs4 >> 18) & 0x07);
    utf8[1] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >>  6) & 0x3f);
    utf8[3] = 0x80 | ((ucs4 >>  0) & 0x3f);

    return 4;
  }
  else if (ucs4 <= 0x03ffffffL) {
    utf8[0] = 0xf8 | ((ucs4 >> 24) & 0x03);
    utf8[1] = 0x80 | ((ucs4 >> 18) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[3] = 0x80 | ((ucs4 >>  6) & 0x3f);
    utf8[4] = 0x80 | ((ucs4 >>  0) & 0x3f);

    return 5;
  }
  else if (ucs4 <= 0x7fffffffL) {
    utf8[0] = 0xfc | ((ucs4 >> 30) & 0x01);
    utf8[1] = 0x80 | ((ucs4 >> 24) & 0x3f);
    utf8[2] = 0x80 | ((ucs4 >> 18) & 0x3f);
    utf8[3] = 0x80 | ((ucs4 >> 12) & 0x3f);
    utf8[4] = 0x80 | ((ucs4 >>  6) & 0x3f);
    utf8[5] = 0x80 | ((ucs4 >>  0) & 0x3f);

    return 6;
  }

  /* default */

  return id3_utf8_encodechar(utf8, ID3_UCS4_REPLACEMENTCHAR);
}

/*
 * NAME:	utf8->decode()
 * DESCRIPTION:	decode a complete utf8 string into a ucs4 string
 */
void id3_utf8_decode(id3_utf8_t const *utf8, id3_ucs4_t *ucs4)
{
  do
    utf8 += id3_utf8_decodechar(utf8, ucs4);
  while (*ucs4++);
}

/*
 * NAME:	utf8->encode()
 * DESCRIPTION:	encode a complete ucs4 string into a utf8 string
 */
void id3_utf8_encode(id3_utf8_t *utf8, id3_ucs4_t const *ucs4)
{
  do
    utf8 += id3_utf8_encodechar(utf8, *ucs4);
  while (*ucs4++);
}

/*
 * NAME:	utf8->put()
 * DESCRIPTION:	serialize a single utf8 character
 */
id3_length_t id3_utf8_put(id3_byte_t **ptr, id3_utf8_t utf8)
{
  if (ptr)
    *(*ptr)++ = utf8;

  return 1;
}

/*
 * NAME:	utf8->get()
 * DESCRIPTION:	deserialize a single utf8 character
 */
id3_utf8_t id3_utf8_get(id3_byte_t const **ptr)
{
  return *(*ptr)++;
}

/*
 * NAME:	utf8->serialize()
 * DESCRIPTION:	serialize a ucs4 string using utf8 encoding
 */
id3_length_t id3_utf8_serialize(id3_byte_t **ptr, id3_ucs4_t const *ucs4,
				int terminate)
{
  id3_length_t size = 0;
  id3_utf8_t utf8[6], *out;

  while (*ucs4) {
    switch (id3_utf8_encodechar(out = utf8, *ucs4++)) {
    case 6: size += id3_utf8_put(ptr, *out++);
    case 5: size += id3_utf8_put(ptr, *out++);
    case 4: size += id3_utf8_put(ptr, *out++);
    case 3: size += id3_utf8_put(ptr, *out++);
    case 2: size += id3_utf8_put(ptr, *out++);
    case 1: size += id3_utf8_put(ptr, *out++);
    case 0: break;
    }
  }

  if (terminate)
    size += id3_utf8_put(ptr, 0);

  return size;
}

/*
 * NAME:	utf8->deserialize()
 * DESCRIPTION:	deserialize a ucs4 string using utf8 encoding
 */
id3_ucs4_t *id3_utf8_deserialize(id3_byte_t const **ptr, id3_length_t length)
{
  id3_byte_t const *end;
  id3_utf8_t *utf8ptr, *utf8;
  id3_ucs4_t *ucs4;

  end = *ptr + length;

  utf8 = malloc((length + 1) * sizeof(*utf8));
  if (utf8 == 0)
    return 0;

  utf8ptr = utf8;
  while (end - *ptr > 0 && (*utf8ptr = id3_utf8_get(ptr)))
    ++utf8ptr;

  *utf8ptr = 0;

  ucs4 = malloc((id3_utf8_length(utf8) + 1) * sizeof(*ucs4));
  if (ucs4)
    id3_utf8_decode(utf8, ucs4);

  free(utf8);

  return ucs4;
}

/*
 * NAME:	utf8->free()
 * DESCRIPTION:	free memory of a string
 */
void id3_utf8_free(id3_utf8_t * utf8)
{
  free(utf8);
}