/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#include "fribidi_config.h"
#ifndef FRIBIDI_NO_CHARSETS

#include "fribidi_char_sets.h"

typedef struct
{
  /* Convert the character string "s" to unicode string "us" and
     return it's length. */
  int (*charset_to_unicode) (char *s, int length,
			     /* output */
			     FriBidiChar *us);
  /* Convert the unicode string "us" with length "length" to character
     string "s" and return it's length. */
  int (*unicode_to_charset) (FriBidiChar *us, int length,
			     /* output */
			     char *s);
  /* Charset's name. */
  char *name;
  /* Charset's title. */
  char *title;
  /* Comments, if any. */
  char *(*desc) (void);
  /* Some charsets like CapRTL may need to change some fribidis tables, by
     calling this function, they can do this changes. */
  fribidi_boolean (*enter) (void);
  /* Some charsets like CapRTL may need to change some fribidis tables, by
     calling this function, they can undo their changes, perhaps to enter
     another mode. */
  fribidi_boolean (*leave) (void);
}
FriBidiCharSetHandler;

/* Each charset must define the functions and strings named below
   (in _FRIBIDI_ADD_CHAR_SET) or define them as NULL, if not any. */

#define _FRIBIDI_ADD_CHAR_SET(CHAR_SET, char_set) \
  { \
    fribidi_##char_set##_to_unicode, \
    fribidi_unicode_to_##char_set, \
    fribidi_char_set_name_##char_set, \
    fribidi_char_set_title_##char_set, \
    fribidi_char_set_desc_##char_set, \
    fribidi_char_set_enter_##char_set, \
    fribidi_char_set_leave_##char_set, \
  },

FriBidiCharSetHandler fribidi_char_sets[FRIBIDI_CHAR_SETS_NUM + 1] = {
  {NULL, NULL, "Not Implemented", NULL, NULL, NULL},
#include "fribidi_char_sets.i"
};

#undef _FRIBIDI_ADD_CHAR_SET

static char
toupper (char c)
{
  return c < 'a' || c > 'z' ? c : c + 'A' - 'a';
}

static int
fribidi_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && toupper (*s1) == toupper (*s2))
    {
      s1++;
      s2++;
    }
  return *s1 - *s2;
}

/* Return the charset which name is "s". */
FRIBIDI_API FriBidiCharSet
fribidi_parse_charset (char *s)
{
  int i;

  for (i = FRIBIDI_CHAR_SETS_NUM; i; i--)
    if (fribidi_strcasecmp (s, fribidi_char_sets[i].name) == 0)
      return i;

  return FRIBIDI_CHAR_SET_NOT_FOUND;
}


/* Convert the character string "s" in charset "char_set" to unicode
   string "us" and return it's length. */
FRIBIDI_API int
fribidi_charset_to_unicode (FriBidiCharSet char_set, char *s, int length,
			    /* output */
			    FriBidiChar *us)
{
  fribidi_char_set_enter (char_set);
  return fribidi_char_sets[char_set].charset_to_unicode == NULL ? 0 :
    (*fribidi_char_sets[char_set].charset_to_unicode) (s, length, us);
}

/* Convert the unicode string "us" with length "length" to character
   string "s" in charset "char_set" and return it's length. */
FRIBIDI_API int
fribidi_unicode_to_charset (FriBidiCharSet char_set,
			    FriBidiChar *us, int length,
			    /* output */
			    char *s)
{
  fribidi_char_set_enter (char_set);
  return fribidi_char_sets[char_set].unicode_to_charset == NULL ? 0 :
    (*fribidi_char_sets[char_set].unicode_to_charset) (us, length, s);
}

/* Return the string containing the name of the charset. */
FRIBIDI_API char *
fribidi_char_set_name (FriBidiCharSet char_set)
{
  return fribidi_char_sets[char_set].name == NULL ? (char *) "" :
    fribidi_char_sets[char_set].name;
}

/* Return the string containing the title of the charset. */
FRIBIDI_API char *
fribidi_char_set_title (FriBidiCharSet char_set)
{
  return fribidi_char_sets[char_set].title == NULL ?
    fribidi_char_set_name (char_set) : fribidi_char_sets[char_set].title;
}

/* Return the string containing the comments about the charset, if any. */
FRIBIDI_API char *
fribidi_char_set_desc (FriBidiCharSet char_set)
{
  return fribidi_char_sets[char_set].desc == NULL ?
    NULL : fribidi_char_sets[char_set].desc ();
}

static FriBidiCharSet current_char_set = FRIBIDI_CHAR_SET_DEFAULT;

/* Some charsets like CapRTL may need to change some fribidis tables, by
   calling this function, they can do this changes. */
FRIBIDI_API fribidi_boolean
fribidi_char_set_enter (FriBidiCharSet char_set)
{
  if (char_set != current_char_set && fribidi_char_sets[char_set].enter)
    {
      fribidi_char_set_leave (current_char_set);
      current_char_set = char_set;
      return (*fribidi_char_sets[char_set].enter) ();
    }
  else
    return FRIBIDI_TRUE;
}

/* Some charsets like CapRTL may need to change some fribidis tables, by
   calling this function, they can undo their changes, maybe to enter
   another mode. */
FRIBIDI_API fribidi_boolean
fribidi_char_set_leave (FriBidiCharSet char_set)
{
  if (char_set == current_char_set && fribidi_char_sets[char_set].leave)
    return (*fribidi_char_sets[char_set].leave) ();
  else
    return FRIBIDI_TRUE;
}


/* Interface version 1, deprecated, just for compatibility. */

#include <string.h>

FRIBIDI_API int
fribidi_charset_to_unicode_1 (FriBidiCharSet char_set, char *s,
			      /* output */
			      FriBidiChar *us)
{
  return fribidi_charset_to_unicode (char_set, s, strlen (s), us);
}

/* Also old character sets. */

#define FRIBIDI_TO_UNICODE_DEFINE_1(cs)	\
	int fribidi_##cs##_to_unicode_1 (char *s, FriBidiChar *us)	\
	{	\
	  return fribidi_##cs##_to_unicode (s, strlen(s), us);	\
	}
FRIBIDI_TO_UNICODE_DEFINE_1 (utf8)
FRIBIDI_TO_UNICODE_DEFINE_1 (cap_rtl)
FRIBIDI_TO_UNICODE_DEFINE_1 (iso8859_6)
FRIBIDI_TO_UNICODE_DEFINE_1 (iso8859_8)
FRIBIDI_TO_UNICODE_DEFINE_1 (cp1255)
FRIBIDI_TO_UNICODE_DEFINE_1 (cp1256) FRIBIDI_TO_UNICODE_DEFINE_1 (isiri_3342)
#undef FRIBIDI_TO_UNICODE_DEFINE_1
#endif
