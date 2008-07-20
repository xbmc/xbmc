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
 * $Id: parse.c,v 1.9 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# include <stdlib.h>
# include <string.h>

# include "id3tag.h"
# include "parse.h"
# include "latin1.h"
# include "utf16.h"
# include "utf8.h"

signed long id3_parse_int(id3_byte_t const **ptr, unsigned int bytes)
{
  signed long value = 0;

  assert(bytes >= 1 && bytes <= 4);

  if (**ptr & 0x80)
    value = ~0;

  switch (bytes) {
  case 4: value = (value << 8) | *(*ptr)++;
  case 3: value = (value << 8) | *(*ptr)++;
  case 2: value = (value << 8) | *(*ptr)++;
  case 1: value = (value << 8) | *(*ptr)++;
  }

  return value;
}

unsigned long id3_parse_uint(id3_byte_t const **ptr, unsigned int bytes)
{
  unsigned long value = 0;

  assert(bytes >= 1 && bytes <= 4);

  switch (bytes) {
  case 4: value = (value << 8) | *(*ptr)++;
  case 3: value = (value << 8) | *(*ptr)++;
  case 2: value = (value << 8) | *(*ptr)++;
  case 1: value = (value << 8) | *(*ptr)++;
  }

  return value;
}

unsigned long id3_parse_syncsafe(id3_byte_t const **ptr, unsigned int bytes)
{
  unsigned long value = 0;

  assert(bytes == 4 || bytes == 5);

  switch (bytes) {
  case 5: value = (value << 4) | (*(*ptr)++ & 0x0f);
  case 4: value = (value << 7) | (*(*ptr)++ & 0x7f);
          value = (value << 7) | (*(*ptr)++ & 0x7f);
	  value = (value << 7) | (*(*ptr)++ & 0x7f);
	  value = (value << 7) | (*(*ptr)++ & 0x7f);
  }

  return value;
}

void id3_parse_immediate(id3_byte_t const **ptr, unsigned int bytes,
			 char *value)
{
  assert(value);
  assert(bytes == 8 || bytes == 4 || bytes == 3);

  switch (bytes) {
  case 8: *value++ = *(*ptr)++;
          *value++ = *(*ptr)++;
	  *value++ = *(*ptr)++;
	  *value++ = *(*ptr)++;
  case 4: *value++ = *(*ptr)++;
  case 3: *value++ = *(*ptr)++;
          *value++ = *(*ptr)++;
	  *value++ = *(*ptr)++;
  }

  *value = 0;
}

id3_latin1_t *id3_parse_latin1(id3_byte_t const **ptr, id3_length_t length,
			       int full)
{
  id3_byte_t const *end;
  int terminated = 0;
  id3_latin1_t *latin1;

  end = memchr(*ptr, 0, length);
  if (end == 0)
    end = *ptr + length;
  else {
    length = end - *ptr;
    terminated = 1;
  }

  latin1 = malloc(length + 1);
  if (latin1) {
    memcpy(latin1, *ptr, length);
    latin1[length] = 0;

    if (!full) {
      id3_latin1_t *check;

      for (check = latin1; *check; ++check) {
	if (*check == '\n')
	  *check = ' ';
      }
    }
  }

  *ptr += length + terminated;

  return latin1;
}

id3_ucs4_t *id3_parse_string(id3_byte_t const **ptr, id3_length_t length,
			     enum id3_field_textencoding encoding, int full)
{
  id3_ucs4_t *ucs4 = 0;
  enum id3_utf16_byteorder byteorder = ID3_UTF16_BYTEORDER_ANY;

  switch (encoding) {
  case ID3_FIELD_TEXTENCODING_ISO_8859_1:
    ucs4 = id3_latin1_deserialize(ptr, length);

    break;

  case ID3_FIELD_TEXTENCODING_UTF_16BE:
    byteorder = ID3_UTF16_BYTEORDER_BE;
  case ID3_FIELD_TEXTENCODING_UTF_16:
    ucs4 = id3_utf16_deserialize(ptr, length, byteorder);

    break;

  case ID3_FIELD_TEXTENCODING_UTF_8:
    ucs4 = id3_utf8_deserialize(ptr, length);

    break;
  }

  if (ucs4 && !full) {
    id3_ucs4_t *check;

    for (check = ucs4; *check; ++check) {
      if (*check == '\n')
	*check = ' ';
    }
  }

  return ucs4;
}

id3_byte_t *id3_parse_binary(id3_byte_t const **ptr, id3_length_t length)
{
  id3_byte_t *data;

  if (length == 0)
    return malloc(1);

  data = malloc(length);
  if (data)
    memcpy(data, *ptr, length);

  *ptr += length;

  return data;
}
