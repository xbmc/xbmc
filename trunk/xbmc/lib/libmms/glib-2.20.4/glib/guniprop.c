/* guniprop.c - Unicode character properties.
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <locale.h>

#include "glib.h"
#include "gunichartables.h"
#include "gmirroringtable.h"
#include "gscripttable.h"
#include "gunicodeprivate.h"
#include "galias.h"

#define ATTR_TABLE(Page) (((Page) <= G_UNICODE_LAST_PAGE_PART1) \
                          ? attr_table_part1[Page] \
                          : attr_table_part2[(Page) - 0xe00])

#define ATTTABLE(Page, Char) \
  ((ATTR_TABLE(Page) == G_UNICODE_MAX_TABLE_INDEX) ? 0 : (attr_data[ATTR_TABLE(Page)][Char]))

#define TTYPE_PART1(Page, Char) \
  ((type_table_part1[Page] >= G_UNICODE_MAX_TABLE_INDEX) \
   ? (type_table_part1[Page] - G_UNICODE_MAX_TABLE_INDEX) \
   : (type_data[type_table_part1[Page]][Char]))

#define TTYPE_PART2(Page, Char) \
  ((type_table_part2[Page] >= G_UNICODE_MAX_TABLE_INDEX) \
   ? (type_table_part2[Page] - G_UNICODE_MAX_TABLE_INDEX) \
   : (type_data[type_table_part2[Page]][Char]))

#define TYPE(Char) \
  (((Char) <= G_UNICODE_LAST_CHAR_PART1) \
   ? TTYPE_PART1 ((Char) >> 8, (Char) & 0xff) \
   : (((Char) >= 0xe0000 && (Char) <= G_UNICODE_LAST_CHAR) \
      ? TTYPE_PART2 (((Char) - 0xe0000) >> 8, (Char) & 0xff) \
      : G_UNICODE_UNASSIGNED))


#define IS(Type, Class)	(((guint)1 << (Type)) & (Class))
#define OR(Type, Rest)	(((guint)1 << (Type)) | (Rest))



#define ISALPHA(Type)	IS ((Type),				\
			    OR (G_UNICODE_LOWERCASE_LETTER,	\
			    OR (G_UNICODE_UPPERCASE_LETTER,	\
			    OR (G_UNICODE_TITLECASE_LETTER,	\
			    OR (G_UNICODE_MODIFIER_LETTER,	\
			    OR (G_UNICODE_OTHER_LETTER,		0))))))

#define ISALDIGIT(Type)	IS ((Type),				\
			    OR (G_UNICODE_DECIMAL_NUMBER,	\
			    OR (G_UNICODE_LETTER_NUMBER,	\
			    OR (G_UNICODE_OTHER_NUMBER,		\
			    OR (G_UNICODE_LOWERCASE_LETTER,	\
			    OR (G_UNICODE_UPPERCASE_LETTER,	\
			    OR (G_UNICODE_TITLECASE_LETTER,	\
			    OR (G_UNICODE_MODIFIER_LETTER,	\
			    OR (G_UNICODE_OTHER_LETTER,		0)))))))))

#define ISMARK(Type)	IS ((Type),				\
			    OR (G_UNICODE_NON_SPACING_MARK,	\
			    OR (G_UNICODE_COMBINING_MARK,	\
			    OR (G_UNICODE_ENCLOSING_MARK,	0))))

#define ISZEROWIDTHTYPE(Type)	IS ((Type),			\
			    OR (G_UNICODE_NON_SPACING_MARK,	\
			    OR (G_UNICODE_ENCLOSING_MARK,	\
			    OR (G_UNICODE_FORMAT,		0))))

/**
 * g_unichar_isalnum:
 * @c: a Unicode character
 * 
 * Determines whether a character is alphanumeric.
 * Given some UTF-8 text, obtain a character value
 * with g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is an alphanumeric character
 **/
gboolean
g_unichar_isalnum (gunichar c)
{
  return ISALDIGIT (TYPE (c)) ? TRUE : FALSE;
}

/**
 * g_unichar_isalpha:
 * @c: a Unicode character
 * 
 * Determines whether a character is alphabetic (i.e. a letter).
 * Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is an alphabetic character
 **/
gboolean
g_unichar_isalpha (gunichar c)
{
  return ISALPHA (TYPE (c)) ? TRUE : FALSE;
}


/**
 * g_unichar_iscntrl:
 * @c: a Unicode character
 * 
 * Determines whether a character is a control character.
 * Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is a control character
 **/
gboolean
g_unichar_iscntrl (gunichar c)
{
  return TYPE (c) == G_UNICODE_CONTROL;
}

/**
 * g_unichar_isdigit:
 * @c: a Unicode character
 * 
 * Determines whether a character is numeric (i.e. a digit).  This
 * covers ASCII 0-9 and also digits in other languages/scripts.  Given
 * some UTF-8 text, obtain a character value with g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is a digit
 **/
gboolean
g_unichar_isdigit (gunichar c)
{
  return TYPE (c) == G_UNICODE_DECIMAL_NUMBER;
}


/**
 * g_unichar_isgraph:
 * @c: a Unicode character
 * 
 * Determines whether a character is printable and not a space
 * (returns %FALSE for control characters, format characters, and
 * spaces). g_unichar_isprint() is similar, but returns %TRUE for
 * spaces. Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is printable unless it's a space
 **/
gboolean
g_unichar_isgraph (gunichar c)
{
  return !IS (TYPE(c),
	      OR (G_UNICODE_CONTROL,
	      OR (G_UNICODE_FORMAT,
	      OR (G_UNICODE_UNASSIGNED,
	      OR (G_UNICODE_SURROGATE,
	      OR (G_UNICODE_SPACE_SEPARATOR,
	     0))))));
}

/**
 * g_unichar_islower:
 * @c: a Unicode character
 * 
 * Determines whether a character is a lowercase letter.
 * Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is a lowercase letter
 **/
gboolean
g_unichar_islower (gunichar c)
{
  return TYPE (c) == G_UNICODE_LOWERCASE_LETTER;
}


/**
 * g_unichar_isprint:
 * @c: a Unicode character
 * 
 * Determines whether a character is printable.
 * Unlike g_unichar_isgraph(), returns %TRUE for spaces.
 * Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is printable
 **/
gboolean
g_unichar_isprint (gunichar c)
{
  return !IS (TYPE(c),
	      OR (G_UNICODE_CONTROL,
	      OR (G_UNICODE_FORMAT,
	      OR (G_UNICODE_UNASSIGNED,
	      OR (G_UNICODE_SURROGATE,
	     0)))));
}

/**
 * g_unichar_ispunct:
 * @c: a Unicode character
 * 
 * Determines whether a character is punctuation or a symbol.
 * Given some UTF-8 text, obtain a character value with
 * g_utf8_get_char().
 * 
 * Return value: %TRUE if @c is a punctuation or symbol character
 **/
gboolean
g_unichar_ispunct (gunichar c)
{
  return IS (TYPE(c),
	     OR (G_UNICODE_CONNECT_PUNCTUATION,
	     OR (G_UNICODE_DASH_PUNCTUATION,
	     OR (G_UNICODE_CLOSE_PUNCTUATION,
	     OR (G_UNICODE_FINAL_PUNCTUATION,
	     OR (G_UNICODE_INITIAL_PUNCTUATION,
	     OR (G_UNICODE_OTHER_PUNCTUATION,
	     OR (G_UNICODE_OPEN_PUNCTUATION,
	     OR (G_UNICODE_CURRENCY_SYMBOL,
	     OR (G_UNICODE_MODIFIER_SYMBOL,
	     OR (G_UNICODE_MATH_SYMBOL,
	     OR (G_UNICODE_OTHER_SYMBOL,
	    0)))))))))))) ? TRUE : FALSE;
}

/**
 * g_unichar_isspace:
 * @c: a Unicode character
 * 
 * Determines whether a character is a space, tab, or line separator
 * (newline, carriage return, etc.).  Given some UTF-8 text, obtain a
 * character value with g_utf8_get_char().
 *
 * (Note: don't use this to do word breaking; you have to use
 * Pango or equivalent to get word breaking right, the algorithm
 * is fairly complex.)
 *  
 * Return value: %TRUE if @c is a space character
 **/
gboolean
g_unichar_isspace (gunichar c)
{
  switch (c)
    {
      /* special-case these since Unicode thinks they are not spaces */
    case '\t':
    case '\n':
    case '\r':
    case '\f':
      return TRUE;
      break;
      
    default:
      {
	return IS (TYPE(c),
	           OR (G_UNICODE_SPACE_SEPARATOR,
	           OR (G_UNICODE_LINE_SEPARATOR,
                   OR (G_UNICODE_PARAGRAPH_SEPARATOR,
		  0)))) ? TRUE : FALSE;
      }
      break;
    }
}

/**
 * g_unichar_ismark:
 * @c: a Unicode character
 *
 * Determines whether a character is a mark (non-spacing mark,
 * combining mark, or enclosing mark in Unicode speak).
 * Given some UTF-8 text, obtain a character value
 * with g_utf8_get_char().
 *
 * Note: in most cases where isalpha characters are allowed,
 * ismark characters should be allowed to as they are essential
 * for writing most European languages as well as many non-Latin
 * scripts.
 *
 * Return value: %TRUE if @c is a mark character
 *
 * Since: 2.14
 **/
gboolean
g_unichar_ismark (gunichar c)
{
  return ISMARK (TYPE (c));
}

/**
 * g_unichar_isupper:
 * @c: a Unicode character
 * 
 * Determines if a character is uppercase.
 * 
 * Return value: %TRUE if @c is an uppercase character
 **/
gboolean
g_unichar_isupper (gunichar c)
{
  return TYPE (c) == G_UNICODE_UPPERCASE_LETTER;
}

/**
 * g_unichar_istitle:
 * @c: a Unicode character
 * 
 * Determines if a character is titlecase. Some characters in
 * Unicode which are composites, such as the DZ digraph
 * have three case variants instead of just two. The titlecase
 * form is used at the beginning of a word where only the
 * first letter is capitalized. The titlecase form of the DZ
 * digraph is U+01F2 LATIN CAPITAL LETTTER D WITH SMALL LETTER Z.
 * 
 * Return value: %TRUE if the character is titlecase
 **/
gboolean
g_unichar_istitle (gunichar c)
{
  unsigned int i;
  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
    if (title_table[i][0] == c)
      return TRUE;
  return FALSE;
}

/**
 * g_unichar_isxdigit:
 * @c: a Unicode character.
 * 
 * Determines if a character is a hexidecimal digit.
 * 
 * Return value: %TRUE if the character is a hexadecimal digit
 **/
gboolean
g_unichar_isxdigit (gunichar c)
{
  return ((c >= 'a' && c <= 'f')
	  || (c >= 'A' && c <= 'F')
	  || (TYPE (c) == G_UNICODE_DECIMAL_NUMBER));
}

/**
 * g_unichar_isdefined:
 * @c: a Unicode character
 * 
 * Determines if a given character is assigned in the Unicode
 * standard.
 *
 * Return value: %TRUE if the character has an assigned value
 **/
gboolean
g_unichar_isdefined (gunichar c)
{
  return !IS (TYPE(c),
	      OR (G_UNICODE_UNASSIGNED,
	      OR (G_UNICODE_SURROGATE,
	     0)));
}

/**
 * g_unichar_iszerowidth:
 * @c: a Unicode character
 * 
 * Determines if a given character typically takes zero width when rendered.
 * The return value is %TRUE for all non-spacing and enclosing marks
 * (e.g., combining accents), format characters, zero-width
 * space, but not U+00AD SOFT HYPHEN.
 *
 * A typical use of this function is with one of g_unichar_iswide() or
 * g_unichar_iswide_cjk() to determine the number of cells a string occupies
 * when displayed on a grid display (terminals).  However, note that not all
 * terminals support zero-width rendering of zero-width marks.
 *
 * Return value: %TRUE if the character has zero width
 *
 * Since: 2.14
 **/
gboolean
g_unichar_iszerowidth (gunichar c)
{
  if (G_UNLIKELY (c == 0x00AD))
    return FALSE;

  if (G_UNLIKELY (ISZEROWIDTHTYPE (TYPE (c))))
    return TRUE;

  if (G_UNLIKELY ((c >= 0x1160 && c < 0x1200) ||
		  c == 0x200B))
    return TRUE;

  return FALSE;
}

struct Interval
{
  gunichar start, end;
};

static int
interval_compare (const void *key, const void *elt)
{
  gunichar c = GPOINTER_TO_UINT (key);
  struct Interval *interval = (struct Interval *)elt;

  if (c < interval->start)
    return -1;
  if (c > interval->end)
    return +1;

  return 0;
}

/**
 * g_unichar_iswide:
 * @c: a Unicode character
 * 
 * Determines if a character is typically rendered in a double-width
 * cell.
 * 
 * Return value: %TRUE if the character is wide
 **/
gboolean
g_unichar_iswide (gunichar c)
{
  /* sorted list of intervals of East_Asian_Width = W and F characters
   * from Unicode 5.1.0.  produced by mungling output of:
   * grep ';[FW]\>' EastAsianWidth.txt */
  static const struct Interval wide[] = {
    {0x1100, 0x1159}, {0x115F, 0x115F}, {0x2329, 0x232A}, {0x2E80, 0x2E99},
    {0x2E9B, 0x2EF3}, {0x2F00, 0x2FD5}, {0x2FF0, 0x2FFB}, {0x3000, 0x303E},
    {0x3041, 0x3096}, {0x3099, 0x30FF}, {0x3105, 0x312D}, {0x3131, 0x318E},
    {0x3190, 0x31B7}, {0x31C0, 0x31E3}, {0x31F0, 0x321E}, {0x3220, 0x3243},
    {0x3250, 0x32FE}, {0x3300, 0x4DB5}, {0x4E00, 0x9FC3}, {0xA000, 0xA48C},
    {0xA490, 0xA4C6}, {0xAC00, 0xD7A3}, {0xF900, 0xFA2D}, {0xFA30, 0xFA6A},
    {0xFA70, 0xFAD9}, {0xFE10, 0xFE19}, {0xFE30, 0xFE52}, {0xFE54, 0xFE66},
    {0xFE68, 0xFE6B}, {0xFF01, 0xFF60}, {0xFFE0, 0xFFE6}, {0x20000, 0x2FFFD},
    {0x30000, 0x3FFFD}
  };

  if (bsearch (GUINT_TO_POINTER (c), wide, G_N_ELEMENTS (wide), sizeof wide[0],
	       interval_compare))
    return TRUE;

  return FALSE;
}


/**
 * g_unichar_iswide_cjk:
 * @c: a Unicode character
 * 
 * Determines if a character is typically rendered in a double-width
 * cell under legacy East Asian locales.  If a character is wide according to
 * g_unichar_iswide(), then it is also reported wide with this function, but
 * the converse is not necessarily true.  See the
 * <ulink url="http://www.unicode.org/reports/tr11/">Unicode Standard
 * Annex #11</ulink> for details.
 *
 * If a character passes the g_unichar_iswide() test then it will also pass
 * this test, but not the other way around.  Note that some characters may
 * pas both this test and g_unichar_iszerowidth().
 * 
 * Return value: %TRUE if the character is wide in legacy East Asian locales
 *
 * Since: 2.12
 */
gboolean
g_unichar_iswide_cjk (gunichar c)
{
  /* sorted list of intervals of East_Asian_Width = A and F characters
   * from Unicode 5.1.0.  produced by mungling output of:
   * grep ';[A]\>' EastAsianWidth.txt */
  static const struct Interval ambiguous[] = {
    {0x00A1, 0x00A1}, {0x00A4, 0x00A4}, {0x00A7, 0x00A8}, {0x00AA, 0x00AA},
    {0x00AD, 0x00AE}, {0x00B0, 0x00B4}, {0x00B6, 0x00BA}, {0x00BC, 0x00BF},
    {0x00C6, 0x00C6}, {0x00D0, 0x00D0}, {0x00D7, 0x00D8}, {0x00DE, 0x00E1},
    {0x00E6, 0x00E6}, {0x00E8, 0x00EA}, {0x00EC, 0x00ED}, {0x00F0, 0x00F0},
    {0x00F2, 0x00F3}, {0x00F7, 0x00FA}, {0x00FC, 0x00FC}, {0x00FE, 0x00FE},
    {0x0101, 0x0101}, {0x0111, 0x0111}, {0x0113, 0x0113}, {0x011B, 0x011B},
    {0x0126, 0x0127}, {0x012B, 0x012B}, {0x0131, 0x0133}, {0x0138, 0x0138},
    {0x013F, 0x0142}, {0x0144, 0x0144}, {0x0148, 0x014B}, {0x014D, 0x014D},
    {0x0152, 0x0153}, {0x0166, 0x0167}, {0x016B, 0x016B}, {0x01CE, 0x01CE},
    {0x01D0, 0x01D0}, {0x01D2, 0x01D2}, {0x01D4, 0x01D4}, {0x01D6, 0x01D6},
    {0x01D8, 0x01D8}, {0x01DA, 0x01DA}, {0x01DC, 0x01DC}, {0x0251, 0x0251},
    {0x0261, 0x0261}, {0x02C4, 0x02C4}, {0x02C7, 0x02C7}, {0x02C9, 0x02CB},
    {0x02CD, 0x02CD}, {0x02D0, 0x02D0}, {0x02D8, 0x02DB}, {0x02DD, 0x02DD},
    {0x02DF, 0x02DF}, {0x0300, 0x036F}, {0x0391, 0x03A1}, {0x03A3, 0x03A9},
    {0x03B1, 0x03C1}, {0x03C3, 0x03C9}, {0x0401, 0x0401}, {0x0410, 0x044F},
    {0x0451, 0x0451}, {0x2010, 0x2010}, {0x2013, 0x2016}, {0x2018, 0x2019},
    {0x201C, 0x201D}, {0x2020, 0x2022}, {0x2024, 0x2027}, {0x2030, 0x2030},
    {0x2032, 0x2033}, {0x2035, 0x2035}, {0x203B, 0x203B}, {0x203E, 0x203E},
    {0x2074, 0x2074}, {0x207F, 0x207F}, {0x2081, 0x2084}, {0x20AC, 0x20AC},
    {0x2103, 0x2103}, {0x2105, 0x2105}, {0x2109, 0x2109}, {0x2113, 0x2113},
    {0x2116, 0x2116}, {0x2121, 0x2122}, {0x2126, 0x2126}, {0x212B, 0x212B},
    {0x2153, 0x2154}, {0x215B, 0x215E}, {0x2160, 0x216B}, {0x2170, 0x2179},
    {0x2190, 0x2199}, {0x21B8, 0x21B9}, {0x21D2, 0x21D2}, {0x21D4, 0x21D4},
    {0x21E7, 0x21E7}, {0x2200, 0x2200}, {0x2202, 0x2203}, {0x2207, 0x2208},
    {0x220B, 0x220B}, {0x220F, 0x220F}, {0x2211, 0x2211}, {0x2215, 0x2215},
    {0x221A, 0x221A}, {0x221D, 0x2220}, {0x2223, 0x2223}, {0x2225, 0x2225},
    {0x2227, 0x222C}, {0x222E, 0x222E}, {0x2234, 0x2237}, {0x223C, 0x223D},
    {0x2248, 0x2248}, {0x224C, 0x224C}, {0x2252, 0x2252}, {0x2260, 0x2261},
    {0x2264, 0x2267}, {0x226A, 0x226B}, {0x226E, 0x226F}, {0x2282, 0x2283},
    {0x2286, 0x2287}, {0x2295, 0x2295}, {0x2299, 0x2299}, {0x22A5, 0x22A5},
    {0x22BF, 0x22BF}, {0x2312, 0x2312}, {0x2460, 0x24E9}, {0x24EB, 0x254B},
    {0x2550, 0x2573}, {0x2580, 0x258F}, {0x2592, 0x2595}, {0x25A0, 0x25A1},
    {0x25A3, 0x25A9}, {0x25B2, 0x25B3}, {0x25B6, 0x25B7}, {0x25BC, 0x25BD},
    {0x25C0, 0x25C1}, {0x25C6, 0x25C8}, {0x25CB, 0x25CB}, {0x25CE, 0x25D1},
    {0x25E2, 0x25E5}, {0x25EF, 0x25EF}, {0x2605, 0x2606}, {0x2609, 0x2609},
    {0x260E, 0x260F}, {0x2614, 0x2615}, {0x261C, 0x261C}, {0x261E, 0x261E},
    {0x2640, 0x2640}, {0x2642, 0x2642}, {0x2660, 0x2661}, {0x2663, 0x2665},
    {0x2667, 0x266A}, {0x266C, 0x266D}, {0x266F, 0x266F}, {0x273D, 0x273D},
    {0x2776, 0x277F}, {0xE000, 0xF8FF}, {0xFE00, 0xFE0F}, {0xFFFD, 0xFFFD},
    {0xE0100, 0xE01EF}, {0xF0000, 0xFFFFD}, {0x100000, 0x10FFFD}
  };

  if (g_unichar_iswide (c))
    return TRUE;

  if (bsearch (GUINT_TO_POINTER (c), ambiguous, G_N_ELEMENTS (ambiguous), sizeof ambiguous[0],
	       interval_compare))
    return TRUE;

  return FALSE;
}


/**
 * g_unichar_toupper:
 * @c: a Unicode character
 * 
 * Converts a character to uppercase.
 * 
 * Return value: the result of converting @c to uppercase.
 *               If @c is not an lowercase or titlecase character,
 *               or has no upper case equivalent @c is returned unchanged.
 **/
gunichar
g_unichar_toupper (gunichar c)
{
  int t = TYPE (c);
  if (t == G_UNICODE_LOWERCASE_LETTER)
    {
      gunichar val = ATTTABLE (c >> 8, c & 0xff);
      if (val >= 0x1000000)
	{
	  const gchar *p = special_case_table + val - 0x1000000;
          val = g_utf8_get_char (p);
	}
      /* Some lowercase letters, e.g., U+000AA, FEMININE ORDINAL INDICATOR,
       * do not have an uppercase equivalent, in which case val will be
       * zero. 
       */
      return val ? val : c;
    }
  else if (t == G_UNICODE_TITLECASE_LETTER)
    {
      unsigned int i;
      for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
	{
	  if (title_table[i][0] == c)
	    return title_table[i][1];
	}
    }
  return c;
}

/**
 * g_unichar_tolower:
 * @c: a Unicode character.
 * 
 * Converts a character to lower case.
 * 
 * Return value: the result of converting @c to lower case.
 *               If @c is not an upperlower or titlecase character,
 *               or has no lowercase equivalent @c is returned unchanged.
 **/
gunichar
g_unichar_tolower (gunichar c)
{
  int t = TYPE (c);
  if (t == G_UNICODE_UPPERCASE_LETTER)
    {
      gunichar val = ATTTABLE (c >> 8, c & 0xff);
      if (val >= 0x1000000)
	{
	  const gchar *p = special_case_table + val - 0x1000000;
	  return g_utf8_get_char (p);
	}
      else
	{
	  /* Not all uppercase letters are guaranteed to have a lowercase
	   * equivalent.  If this is the case, val will be zero. */
	  return val ? val : c;
	}
    }
  else if (t == G_UNICODE_TITLECASE_LETTER)
    {
      unsigned int i;
      for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
	{
	  if (title_table[i][0] == c)
	    return title_table[i][2];
	}
    }
  return c;
}

/**
 * g_unichar_totitle:
 * @c: a Unicode character
 * 
 * Converts a character to the titlecase.
 * 
 * Return value: the result of converting @c to titlecase.
 *               If @c is not an uppercase or lowercase character,
 *               @c is returned unchanged.
 **/
gunichar
g_unichar_totitle (gunichar c)
{
  unsigned int i;
  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
    {
      if (title_table[i][0] == c || title_table[i][1] == c
	  || title_table[i][2] == c)
	return title_table[i][0];
    }
    
  if (TYPE (c) == G_UNICODE_LOWERCASE_LETTER)
    return g_unichar_toupper (c);

  return c;
}

/**
 * g_unichar_digit_value:
 * @c: a Unicode character
 *
 * Determines the numeric value of a character as a decimal
 * digit.
 *
 * Return value: If @c is a decimal digit (according to
 * g_unichar_isdigit()), its numeric value. Otherwise, -1.
 **/
int
g_unichar_digit_value (gunichar c)
{
  if (TYPE (c) == G_UNICODE_DECIMAL_NUMBER)
    return ATTTABLE (c >> 8, c & 0xff);
  return -1;
}

/**
 * g_unichar_xdigit_value:
 * @c: a Unicode character
 *
 * Determines the numeric value of a character as a hexidecimal
 * digit.
 *
 * Return value: If @c is a hex digit (according to
 * g_unichar_isxdigit()), its numeric value. Otherwise, -1.
 **/
int
g_unichar_xdigit_value (gunichar c)
{
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (TYPE (c) == G_UNICODE_DECIMAL_NUMBER)
    return ATTTABLE (c >> 8, c & 0xff);
  return -1;
}

/**
 * g_unichar_type:
 * @c: a Unicode character
 * 
 * Classifies a Unicode character by type.
 * 
 * Return value: the type of the character.
 **/
GUnicodeType
g_unichar_type (gunichar c)
{
  return TYPE (c);
}

/*
 * Case mapping functions
 */

typedef enum {
  LOCALE_NORMAL,
  LOCALE_TURKIC,
  LOCALE_LITHUANIAN
} LocaleType;

static LocaleType
get_locale_type (void)
{
#ifdef G_OS_WIN32
  char *tem = g_win32_getlocale ();
  char locale[2];

  locale[0] = tem[0];
  locale[1] = tem[1];
  g_free (tem);
#else
  const char *locale = setlocale (LC_CTYPE, NULL);
#endif

  switch (locale[0])
    {
   case 'a':
      if (locale[1] == 'z')
	return LOCALE_TURKIC;
      break;
    case 'l':
      if (locale[1] == 't')
	return LOCALE_LITHUANIAN;
      break;
    case 't':
      if (locale[1] == 'r')
	return LOCALE_TURKIC;
      break;
    }

  return LOCALE_NORMAL;
}

static gint
output_marks (const char **p_inout,
	      char        *out_buffer,
	      gboolean     remove_dot)
{
  const char *p = *p_inout;
  gint len = 0;
  
  while (*p)
    {
      gunichar c = g_utf8_get_char (p);
      
      if (ISMARK (TYPE (c)))
	{
	  if (!remove_dot || c != 0x307 /* COMBINING DOT ABOVE */)
	    len += g_unichar_to_utf8 (c, out_buffer ? out_buffer + len : NULL);
	  p = g_utf8_next_char (p);
	}
      else
	break;
    }

  *p_inout = p;
  return len;
}

static gint
output_special_case (gchar *out_buffer,
		     int    offset,
		     int    type,
		     int    which)
{
  const gchar *p = special_case_table + offset;
  gint len;

  if (type != G_UNICODE_TITLECASE_LETTER)
    p = g_utf8_next_char (p);

  if (which == 1)
    p += strlen (p) + 1;

  len = strlen (p);
  if (out_buffer)
    memcpy (out_buffer, p, len);

  return len;
}

static gsize
real_toupper (const gchar *str,
	      gssize       max_len,
	      gchar       *out_buffer,
	      LocaleType   locale_type)
{
  const gchar *p = str;
  const char *last = NULL;
  gsize len = 0;
  gboolean last_was_i = FALSE;

  while ((max_len < 0 || p < str + max_len) && *p)
    {
      gunichar c = g_utf8_get_char (p);
      int t = TYPE (c);
      gunichar val;

      last = p;
      p = g_utf8_next_char (p);

      if (locale_type == LOCALE_LITHUANIAN)
	{
	  if (c == 'i')
	    last_was_i = TRUE;
	  else 
	    {
	      if (last_was_i)
		{
		  /* Nasty, need to remove any dot above. Though
		   * I think only E WITH DOT ABOVE occurs in practice
		   * which could simplify this considerably.
		   */
		  gsize decomp_len, i;
		  gunichar *decomp;

		  decomp = g_unicode_canonical_decomposition (c, &decomp_len);
		  for (i=0; i < decomp_len; i++)
		    {
		      if (decomp[i] != 0x307 /* COMBINING DOT ABOVE */)
			len += g_unichar_to_utf8 (g_unichar_toupper (decomp[i]), out_buffer ? out_buffer + len : NULL);
		    }
		  g_free (decomp);
		  
		  len += output_marks (&p, out_buffer ? out_buffer + len : NULL, TRUE);

		  continue;
		}

	      if (!ISMARK (t))
		last_was_i = FALSE;
	    }
	}
      
      if (locale_type == LOCALE_TURKIC && c == 'i')
	{
	  /* i => LATIN CAPITAL LETTER I WITH DOT ABOVE */
	  len += g_unichar_to_utf8 (0x130, out_buffer ? out_buffer + len : NULL); 
	}
      else if (c == 0x0345)	/* COMBINING GREEK YPOGEGRAMMENI */
	{
	  /* Nasty, need to move it after other combining marks .. this would go away if
	   * we normalized first.
	   */
	  len += output_marks (&p, out_buffer ? out_buffer + len : NULL, FALSE);

	  /* And output as GREEK CAPITAL LETTER IOTA */
	  len += g_unichar_to_utf8 (0x399, out_buffer ? out_buffer + len : NULL); 	  
	}
      else if (IS (t,
		   OR (G_UNICODE_LOWERCASE_LETTER,
		   OR (G_UNICODE_TITLECASE_LETTER,
		  0))))
	{
	  val = ATTTABLE (c >> 8, c & 0xff);

	  if (val >= 0x1000000)
	    {
	      len += output_special_case (out_buffer ? out_buffer + len : NULL, val - 0x1000000, t,
					  t == G_UNICODE_LOWERCASE_LETTER ? 0 : 1);
	    }
	  else
	    {
	      if (t == G_UNICODE_TITLECASE_LETTER)
		{
		  unsigned int i;
		  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
		    {
		      if (title_table[i][0] == c)
			{
			  val = title_table[i][1];
			  break;
			}
		    }
		}

	      /* Some lowercase letters, e.g., U+000AA, FEMININE ORDINAL INDICATOR,
	       * do not have an uppercase equivalent, in which case val will be
	       * zero. */
	      len += g_unichar_to_utf8 (val ? val : c, out_buffer ? out_buffer + len : NULL);
	    }
	}
      else
	{
	  gsize char_len = g_utf8_skip[*(guchar *)last];

	  if (out_buffer)
	    memcpy (out_buffer + len, last, char_len);

	  len += char_len;
	}

    }

  return len;
}

/**
 * g_utf8_strup:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 * 
 * Converts all Unicode characters in the string that have a case
 * to uppercase. The exact manner that this is done depends
 * on the current locale, and may result in the number of
 * characters in the string increasing. (For instance, the
 * German ess-zet will be changed to SS.)
 * 
 * Return value: a newly allocated string, with all characters
 *    converted to uppercase.  
 **/
gchar *
g_utf8_strup (const gchar *str,
	      gssize       len)
{
  gsize result_len;
  LocaleType locale_type;
  gchar *result;

  g_return_val_if_fail (str != NULL, NULL);

  locale_type = get_locale_type ();
  
  /*
   * We use a two pass approach to keep memory management simple
   */
  result_len = real_toupper (str, len, NULL, locale_type);
  result = g_malloc (result_len + 1);
  real_toupper (str, len, result, locale_type);
  result[result_len] = '\0';

  return result;
}

/* traverses the string checking for characters with combining class == 230
 * until a base character is found */
static gboolean
has_more_above (const gchar *str)
{
  const gchar *p = str;
  gint combining_class;

  while (*p)
    {
      combining_class = g_unichar_combining_class (g_utf8_get_char (p));
      if (combining_class == 230)
        return TRUE;
      else if (combining_class == 0)
        break;

      p = g_utf8_next_char (p);
    }

  return FALSE;
}

static gsize
real_tolower (const gchar *str,
	      gssize       max_len,
	      gchar       *out_buffer,
	      LocaleType   locale_type)
{
  const gchar *p = str;
  const char *last = NULL;
  gsize len = 0;

  while ((max_len < 0 || p < str + max_len) && *p)
    {
      gunichar c = g_utf8_get_char (p);
      int t = TYPE (c);
      gunichar val;

      last = p;
      p = g_utf8_next_char (p);

      if (locale_type == LOCALE_TURKIC && c == 'I')
	{
          if (g_utf8_get_char (p) == 0x0307)
            {
              /* I + COMBINING DOT ABOVE => i (U+0069) */
              len += g_unichar_to_utf8 (0x0069, out_buffer ? out_buffer + len : NULL); 
              p = g_utf8_next_char (p);
            }
          else
            {
              /* I => LATIN SMALL LETTER DOTLESS I */
              len += g_unichar_to_utf8 (0x131, out_buffer ? out_buffer + len : NULL); 
            }
        }
      /* Introduce an explicit dot above when lowercasing capital I's and J's
       * whenever there are more accents above. [SpecialCasing.txt] */
      else if (locale_type == LOCALE_LITHUANIAN && 
               (c == 0x00cc || c == 0x00cd || c == 0x0128))
        {
          len += g_unichar_to_utf8 (0x0069, out_buffer ? out_buffer + len : NULL); 
          len += g_unichar_to_utf8 (0x0307, out_buffer ? out_buffer + len : NULL); 

          switch (c)
            {
            case 0x00cc: 
              len += g_unichar_to_utf8 (0x0300, out_buffer ? out_buffer + len : NULL); 
              break;
            case 0x00cd: 
              len += g_unichar_to_utf8 (0x0301, out_buffer ? out_buffer + len : NULL); 
              break;
            case 0x0128: 
              len += g_unichar_to_utf8 (0x0303, out_buffer ? out_buffer + len : NULL); 
              break;
            }
        }
      else if (locale_type == LOCALE_LITHUANIAN && 
               (c == 'I' || c == 'J' || c == 0x012e) && 
               has_more_above (p))
        {
          len += g_unichar_to_utf8 (g_unichar_tolower (c), out_buffer ? out_buffer + len : NULL); 
          len += g_unichar_to_utf8 (0x0307, out_buffer ? out_buffer + len : NULL); 
        }
      else if (c == 0x03A3)	/* GREEK CAPITAL LETTER SIGMA */
	{
	  if ((max_len < 0 || p < str + max_len) && *p)
	    {
	      gunichar next_c = g_utf8_get_char (p);
	      int next_type = TYPE(next_c);

	      /* SIGMA mapps differently depending on whether it is
	       * final or not. The following simplified test would
	       * fail in the case of combining marks following the
	       * sigma, but I don't think that occurs in real text.
	       * The test here matches that in ICU.
	       */
	      if (ISALPHA (next_type)) /* Lu,Ll,Lt,Lm,Lo */
		val = 0x3c3;	/* GREEK SMALL SIGMA */
	      else
		val = 0x3c2;	/* GREEK SMALL FINAL SIGMA */
	    }
	  else
	    val = 0x3c2;	/* GREEK SMALL FINAL SIGMA */

	  len += g_unichar_to_utf8 (val, out_buffer ? out_buffer + len : NULL);
	}
      else if (IS (t,
		   OR (G_UNICODE_UPPERCASE_LETTER,
		   OR (G_UNICODE_TITLECASE_LETTER,
		  0))))
	{
	  val = ATTTABLE (c >> 8, c & 0xff);

	  if (val >= 0x1000000)
	    {
	      len += output_special_case (out_buffer ? out_buffer + len : NULL, val - 0x1000000, t, 0);
	    }
	  else
	    {
	      if (t == G_UNICODE_TITLECASE_LETTER)
		{
		  unsigned int i;
		  for (i = 0; i < G_N_ELEMENTS (title_table); ++i)
		    {
		      if (title_table[i][0] == c)
			{
			  val = title_table[i][2];
			  break;
			}
		    }
		}

	      /* Not all uppercase letters are guaranteed to have a lowercase
	       * equivalent.  If this is the case, val will be zero. */
	      len += g_unichar_to_utf8 (val ? val : c, out_buffer ? out_buffer + len : NULL);
	    }
	}
      else
	{
	  gsize char_len = g_utf8_skip[*(guchar *)last];

	  if (out_buffer)
	    memcpy (out_buffer + len, last, char_len);

	  len += char_len;
	}

    }

  return len;
}

/**
 * g_utf8_strdown:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 * 
 * Converts all Unicode characters in the string that have a case
 * to lowercase. The exact manner that this is done depends
 * on the current locale, and may result in the number of
 * characters in the string changing.
 * 
 * Return value: a newly allocated string, with all characters
 *    converted to lowercase.  
 **/
gchar *
g_utf8_strdown (const gchar *str,
		gssize       len)
{
  gsize result_len;
  LocaleType locale_type;
  gchar *result;

  g_return_val_if_fail (str != NULL, NULL);

  locale_type = get_locale_type ();
  
  /*
   * We use a two pass approach to keep memory management simple
   */
  result_len = real_tolower (str, len, NULL, locale_type);
  result = g_malloc (result_len + 1);
  real_tolower (str, len, result, locale_type);
  result[result_len] = '\0';

  return result;
}

/**
 * g_utf8_casefold:
 * @str: a UTF-8 encoded string
 * @len: length of @str, in bytes, or -1 if @str is nul-terminated.
 * 
 * Converts a string into a form that is independent of case. The
 * result will not correspond to any particular case, but can be
 * compared for equality or ordered with the results of calling
 * g_utf8_casefold() on other strings.
 * 
 * Note that calling g_utf8_casefold() followed by g_utf8_collate() is
 * only an approximation to the correct linguistic case insensitive
 * ordering, though it is a fairly good one. Getting this exactly
 * right would require a more sophisticated collation function that
 * takes case sensitivity into account. GLib does not currently
 * provide such a function.
 * 
 * Return value: a newly allocated string, that is a
 *   case independent form of @str.
 **/
gchar *
g_utf8_casefold (const gchar *str,
		 gssize       len)
{
  GString *result;
  const char *p;

  g_return_val_if_fail (str != NULL, NULL);

  result = g_string_new (NULL);
  p = str;
  while ((len < 0 || p < str + len) && *p)
    {
      gunichar ch = g_utf8_get_char (p);

      int start = 0;
      int end = G_N_ELEMENTS (casefold_table);

      if (ch >= casefold_table[start].ch &&
          ch <= casefold_table[end - 1].ch)
	{
	  while (TRUE)
	    {
	      int half = (start + end) / 2;
	      if (ch == casefold_table[half].ch)
		{
		  g_string_append (result, casefold_table[half].data);
		  goto next;
		}
	      else if (half == start)
		break;
	      else if (ch > casefold_table[half].ch)
		start = half;
	      else
		end = half;
	    }
	}

      g_string_append_unichar (result, g_unichar_tolower (ch));
      
    next:
      p = g_utf8_next_char (p);
    }

  return g_string_free (result, FALSE); 
}

/**
 * g_unichar_get_mirror_char:
 * @ch: a Unicode character
 * @mirrored_ch: location to store the mirrored character
 * 
 * In Unicode, some characters are <firstterm>mirrored</firstterm>. This
 * means that their images are mirrored horizontally in text that is laid
 * out from right to left. For instance, "(" would become its mirror image,
 * ")", in right-to-left text.
 *
 * If @ch has the Unicode mirrored property and there is another unicode
 * character that typically has a glyph that is the mirror image of @ch's
 * glyph and @mirrored_ch is set, it puts that character in the address
 * pointed to by @mirrored_ch.  Otherwise the original character is put.
 *
 * Return value: %TRUE if @ch has a mirrored character, %FALSE otherwise
 *
 * Since: 2.4
 **/
gboolean
g_unichar_get_mirror_char (gunichar ch,
                           gunichar *mirrored_ch)
{
  gboolean found;
  gunichar mirrored;

  mirrored = GLIB_GET_MIRRORING(ch);

  found = ch != mirrored;
  if (mirrored_ch)
    *mirrored_ch = mirrored;

  return found;

}

#define G_SCRIPT_TABLE_MIDPOINT (G_N_ELEMENTS (g_script_table) / 2)

static inline GUnicodeScript
g_unichar_get_script_bsearch (gunichar ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (g_script_table) - 1;
  static int saved_mid = G_SCRIPT_TABLE_MIDPOINT;
  int mid = saved_mid;


  do 
    {
      if (ch < g_script_table[mid].start)
	upper = mid - 1;
      else if (ch >= g_script_table[mid].start + g_script_table[mid].chars)
	lower = mid + 1;
      else
	return g_script_table[saved_mid = mid].script;

      mid = (lower + upper) / 2;
    }
  while (lower <= upper);

  return G_UNICODE_SCRIPT_UNKNOWN;
}

/**
 * g_unichar_get_script:
 * @ch: a Unicode character
 * 
 * Looks up the #GUnicodeScript for a particular character (as defined 
 * by Unicode Standard Annex #24). No check is made for @ch being a
 * valid Unicode character; if you pass in invalid character, the
 * result is undefined.
 *
 * This function is equivalent to pango_script_for_unichar() and the
 * two are interchangeable.
 * 
 * Return value: the #GUnicodeScript for the character.
 *
 * Since: 2.14
 */
GUnicodeScript
g_unichar_get_script (gunichar ch)
{
  if (ch < G_EASY_SCRIPTS_RANGE)
    return g_script_easy_table[ch];
  else 
    return g_unichar_get_script_bsearch (ch); 
}


#define __G_UNIPROP_C__
#include "galiasdef.c"
