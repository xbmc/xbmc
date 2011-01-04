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
 * $Id: ucs4.c,v 1.13 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include <stdlib.h>

# include "id3tag.h"
# include "ucs4.h"
# include "latin1.h"
# include "utf16.h"
# include "utf8.h"

id3_ucs4_t const id3_ucs4_empty[] = { 0 };

/*
 * NAME:	ucs4->length()
 * DESCRIPTION:	return the number of ucs4 chars represented by a ucs4 string
 */
id3_length_t id3_ucs4_length(id3_ucs4_t const *ucs4)
{
  id3_ucs4_t const *ptr = ucs4;

  while (*ptr)
    ++ptr;

  return ptr - ucs4;
}

/*
 * NAME:	ucs4->size()
 * DESCRIPTION:	return the encoding size of a ucs4 string
 */
id3_length_t id3_ucs4_size(id3_ucs4_t const *ucs4)
{
  return id3_ucs4_length(ucs4) + 1;
}

/*
 * NAME:	ucs4->latin1size()
 * DESCRIPTION:	return the encoding size of a latin1-encoded ucs4 string
 */
id3_length_t id3_ucs4_latin1size(id3_ucs4_t const *ucs4)
{
  return id3_ucs4_size(ucs4);
}

/*
 * NAME:	ucs4->utf16size()
 * DESCRIPTION:	return the encoding size of a utf16-encoded ucs4 string
 */
id3_length_t id3_ucs4_utf16size(id3_ucs4_t const *ucs4)
{
  id3_length_t size = 0;

  while (*ucs4) {
    ++size;
    if (*ucs4 >= 0x00010000L &&
	*ucs4 <= 0x0010ffffL)
      ++size;

    ++ucs4;
  }

  return size + 1;
}

/*
 * NAME:	ucs4->utf8size()
 * DESCRIPTION:	return the encoding size of a utf8-encoded ucs4 string
 */
id3_length_t id3_ucs4_utf8size(id3_ucs4_t const *ucs4)
{
  id3_length_t size = 0;

  while (*ucs4) {
    if (*ucs4 <= 0x0000007fL)
      size += 1;
    else if (*ucs4 <= 0x000007ffL)
      size += 2;
    else if (*ucs4 <= 0x0000ffffL)
      size += 3;
    else if (*ucs4 <= 0x001fffffL)
      size += 4;
    else if (*ucs4 <= 0x03ffffffL)
      size += 5;
    else if (*ucs4 <= 0x7fffffffL)
      size += 6;
    else
      size += 2;  /* based on U+00B7 replacement char */

    ++ucs4;
  }

  return size + 1;
}

/*
 * NAME:	ucs4->latin1duplicate()
 * DESCRIPTION:	duplicate and encode a ucs4 string into latin1
 */
id3_latin1_t *id3_ucs4_latin1duplicate(id3_ucs4_t const *ucs4)
{
  id3_latin1_t *latin1;

  latin1 = malloc(id3_ucs4_latin1size(ucs4) * sizeof(*latin1));
  if (latin1)
    id3_latin1_encode(latin1, ucs4);

  return latin1;
}

/*
 * NAME:	ucs4->utf16duplicate()
 * DESCRIPTION:	duplicate and encode a ucs4 string into utf16
 */
id3_utf16_t *id3_ucs4_utf16duplicate(id3_ucs4_t const *ucs4)
{
  id3_utf16_t *utf16;

  utf16 = malloc(id3_ucs4_utf16size(ucs4) * sizeof(*utf16));
  if (utf16)
    id3_utf16_encode(utf16, ucs4);

  return utf16;
}

/*
 * NAME:	ucs4->utf8duplicate()
 * DESCRIPTION:	duplicate and encode a ucs4 string into utf8
 */
id3_utf8_t *id3_ucs4_utf8duplicate(id3_ucs4_t const *ucs4)
{
  id3_utf8_t *utf8;

  utf8 = malloc(id3_ucs4_utf8size(ucs4) * sizeof(*utf8));
  if (utf8)
    id3_utf8_encode(utf8, ucs4);

  return utf8;
}

/*
 * NAME:	ucs4->copy()
 * DESCRIPTION:	copy a ucs4 string
 */
void id3_ucs4_copy(id3_ucs4_t *dest, id3_ucs4_t const *src)
{
  while ((*dest++ = *src++))
    ;
}

/*
 * NAME:	ucs4->duplicate()
 * DESCRIPTION:	duplicate a ucs4 string
 */
id3_ucs4_t *id3_ucs4_duplicate(id3_ucs4_t const *src)
{
  id3_ucs4_t *ucs4;

  ucs4 = malloc(id3_ucs4_size(src) * sizeof(*ucs4));
  if (ucs4)
    id3_ucs4_copy(ucs4, src);

  return ucs4;
}

/*
 * NAME:	ucs4->putnumber()
 * DESCRIPTION:	write a ucs4 string containing a (positive) decimal number
 */
void id3_ucs4_putnumber(id3_ucs4_t *ucs4, unsigned long number)
{
  int digits[10], *digit;

  digit = digits;

  do {
    *digit++ = number % 10;
    number  /= 10;
  }
  while (number);

  while (digit != digits)
    *ucs4++ = '0' + *--digit;

  *ucs4 = 0;
}

/*
 * NAME:	ucs4->getnumber()
 * DESCRIPTION:	read a ucs4 string containing a (positive) decimal number
 */
unsigned long id3_ucs4_getnumber(id3_ucs4_t const *ucs4)
{
  unsigned long number = 0;

  while (*ucs4 >= '0' && *ucs4 <= '9')
    number = 10 * number + (*ucs4++ - '0');

  return number;
}

/*
 * NAME:	ucs4->free()
 * DESCRIPTION:	free memory of a string
 */
void id3_ucs4_free(id3_ucs4_t *ucs4)
{
  free(ucs4);
}
