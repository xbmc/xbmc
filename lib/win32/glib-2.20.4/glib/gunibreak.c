/* gunibreak.c - line break properties
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include "glib.h"
#include "gunibreak.h"
#include "galias.h"

#define TPROP_PART1(Page, Char) \
  ((break_property_table_part1[Page] >= G_UNICODE_MAX_TABLE_INDEX) \
   ? (break_property_table_part1[Page] - G_UNICODE_MAX_TABLE_INDEX) \
   : (break_property_data[break_property_table_part1[Page]][Char]))

#define TPROP_PART2(Page, Char) \
  ((break_property_table_part2[Page] >= G_UNICODE_MAX_TABLE_INDEX) \
   ? (break_property_table_part2[Page] - G_UNICODE_MAX_TABLE_INDEX) \
   : (break_property_data[break_property_table_part2[Page]][Char]))

#define PROP(Char) \
  (((Char) <= G_UNICODE_LAST_CHAR_PART1) \
   ? TPROP_PART1 ((Char) >> 8, (Char) & 0xff) \
   : (((Char) >= 0xe0000 && (Char) <= G_UNICODE_LAST_CHAR) \
      ? TPROP_PART2 (((Char) - 0xe0000) >> 8, (Char) & 0xff) \
      : G_UNICODE_BREAK_UNKNOWN))

/**
 * g_unichar_break_type:
 * @c: a Unicode character
 * 
 * Determines the break type of @c. @c should be a Unicode character
 * (to derive a character from UTF-8 encoded text, use
 * g_utf8_get_char()). The break type is used to find word and line
 * breaks ("text boundaries"), Pango implements the Unicode boundary
 * resolution algorithms and normally you would use a function such
 * as pango_break() instead of caring about break types yourself.
 * 
 * Return value: the break type of @c
 **/
GUnicodeBreakType
g_unichar_break_type (gunichar c)
{
  return PROP (c);
}

#define __G_UNIBREAK_C__
#include "galiasdef.c"
