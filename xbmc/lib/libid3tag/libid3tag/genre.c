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
 * $Id: genre.c,v 1.8 2004/01/23 09:41:32 rob Exp $
 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "global.h"

# include "id3tag.h"
# include "ucs4.h"

/* genres are stored in ucs4 format */
# include "genre.dat"

# define NGENRES  (sizeof(genre_table) / sizeof(genre_table[0]))

/*
 * NAME:	genre->index()
 * DESCRIPTION:	return an ID3v1 genre string indexed by number
 */
id3_ucs4_t const *id3_genre_index(unsigned int index)
{
  return (index < NGENRES) ? genre_table[index] : 0;
}

/*
 * NAME:	genre->name()
 * DESCRIPTION:	translate an ID3v2 genre number/keyword to its full name
 */
id3_ucs4_t const *id3_genre_name(id3_ucs4_t const *string)
{
  id3_ucs4_t const *ptr;
  static id3_ucs4_t const genre_remix[] = { 'R', 'e', 'm', 'i', 'x', 0 };
  static id3_ucs4_t const genre_cover[] = { 'C', 'o', 'v', 'e', 'r', 0 };
  unsigned long number;

  if (string == 0 || *string == 0)
    return id3_ucs4_empty;

  if (string[0] == 'R' && string[1] == 'X' && string[2] == 0)
    return genre_remix;
  if (string[0] == 'C' && string[1] == 'R' && string[2] == 0)
    return genre_cover;

  for (ptr = string; *ptr; ++ptr) {
    if (*ptr < '0' || *ptr > '9')
      return string;
  }

	// check for "(number)" genre strings
  if (string[0] == '(')
    number = id3_ucs4_getnumber(&string[1]);
  else
  	number = id3_ucs4_getnumber(string);

  return (number < NGENRES) ? genre_table[number] : string;
}

/*
 * NAME:	translate()
 * DESCRIPTION:	return a canonicalized character for testing genre equivalence
 */
static
id3_ucs4_t translate(id3_ucs4_t ch)
{
  if (ch) {
    if (ch >= 'A' && ch <= 'Z')
      ch += 'a' - 'A';

    if (ch < 'a' || ch > 'z')
      ch = ID3_UCS4_REPLACEMENTCHAR;
  }

  return ch;
}

/*
 * NAME:	compare()
 * DESCRIPTION:	test two ucs4 genre strings for equivalence
 */
static
int compare(id3_ucs4_t const *str1, id3_ucs4_t const *str2)
{
  id3_ucs4_t c1, c2;

  if (str1 == str2)
    return 1;

  do {
    do
      c1 = translate(*str1++);
    while (c1 == ID3_UCS4_REPLACEMENTCHAR);

    do
      c2 = translate(*str2++);
    while (c2 == ID3_UCS4_REPLACEMENTCHAR);
  }
  while (c1 && c1 == c2);

  return c1 == c2;
}

/*
 * NAME:	genre->number()
 * DESCRIPTION:	translate an ID3v2 genre name/number to its ID3v1 index number
 */
int id3_genre_number(id3_ucs4_t const *string)
{
  id3_ucs4_t const *ptr;
  int i;

  if (string == 0 || *string == 0)
    return -1;

  for (ptr = string; *ptr; ++ptr) {
    if (*ptr < '0' || *ptr > '9')
      break;
  }

  if (*ptr == 0) {
    unsigned long number;

    number = id3_ucs4_getnumber(string);

    return (number <= 0xff) ? number : -1;
  }

  for (i = 0; i < NGENRES; ++i) {
    if (compare(string, genre_table[i]))
      return i;
  }

  /* no equivalent */

  return -1;
}
