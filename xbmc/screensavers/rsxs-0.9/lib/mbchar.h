/* Multibyte character data type.
   Copyright (C) 2001, 2005-2006 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by Bruno Haible <bruno@clisp.org>.  */

/* A multibyte character is a short subsequence of a char* string,
   representing a single wide character.

   We use multibyte characters instead of wide characters because of
   the following goals:
   1) correct multibyte handling, i.e. operate according to the LC_CTYPE
      locale,
   2) ease of maintenance, i.e. the maintainer needs not know all details
      of the ISO C 99 standard,
   3) don't fail grossly if the input is not in the encoding set by the
      locale, because often different encodings are in use in the same
      countries (ISO-8859-1/UTF-8, EUC-JP/Shift_JIS, ...),
   4) fast in the case of ASCII characters,
   5) portability, i.e. don't make unportable assumptions about wchar_t.

   Multibyte characters are only accessed through the mb* macros.

   mb_ptr (mbc)
     return a pointer to the beginning of the multibyte sequence.

   mb_len (mbc)
     returns the number of bytes occupied by the multibyte sequence.
     Always > 0.

   mb_iseq (mbc, sc)
     returns true if mbc is the standard ASCII character sc.

   mb_isnul (mbc)
     returns true if mbc is the nul character.

   mb_cmp (mbc1, mbc2)
     returns a positive, zero, or negative value depending on whether mbc1
     sorts after, same or before mbc2.

   mb_casecmp (mbc1, mbc2)
     returns a positive, zero, or negative value depending on whether mbc1
     sorts after, same or before mbc2, modulo upper/lowercase conversion.

   mb_equal (mbc1, mbc2)
     returns true if mbc1 and mbc2 are equal.

   mb_caseequal (mbc1, mbc2)
     returns true if mbc1 and mbc2 are equal modulo upper/lowercase conversion.

   mb_isalnum (mbc)
     returns true if mbc is alphanumeric.

   mb_isalpha (mbc)
     returns true if mbc is alphabetic.

   mb_isascii(mbc)
     returns true if mbc is plain ASCII.

   mb_isblank (mbc)
     returns true if mbc is a blank.

   mb_iscntrl (mbc)
     returns true if mbc is a control character.

   mb_isdigit (mbc)
     returns true if mbc is a decimal digit.

   mb_isgraph (mbc)
     returns true if mbc is a graphic character.

   mb_islower (mbc)
     returns true if mbc is lowercase.

   mb_isprint (mbc)
     returns true if mbc is a printable character.

   mb_ispunct (mbc)
     returns true if mbc is a punctuation character.

   mb_isspace (mbc)
     returns true if mbc is a space character.

   mb_isupper (mbc)
     returns true if mbc is uppercase.

   mb_isxdigit (mbc)
     returns true if mbc is a hexadecimal digit.

   mb_width (mbc)
     returns the number of columns on the output device occupied by mbc.
     Always >= 0.

   mb_putc (mbc, stream)
     outputs mbc on stream, a byte oriented FILE stream opened for output.

   mb_setascii (&mbc, sc)
     assigns the standard ASCII character sc to mbc.

   mb_copy (&destmbc, &srcmbc)
     copies srcmbc to destmbc.

   Here are the function prototypes of the macros.

   extern const char *	mb_ptr (const mbchar_t mbc);
   extern size_t	mb_len (const mbchar_t mbc);
   extern bool		mb_iseq (const mbchar_t mbc, char sc);
   extern bool		mb_isnul (const mbchar_t mbc);
   extern int		mb_cmp (const mbchar_t mbc1, const mbchar_t mbc2);
   extern int		mb_casecmp (const mbchar_t mbc1, const mbchar_t mbc2);
   extern bool		mb_equal (const mbchar_t mbc1, const mbchar_t mbc2);
   extern bool		mb_caseequal (const mbchar_t mbc1, const mbchar_t mbc2);
   extern bool		mb_isalnum (const mbchar_t mbc);
   extern bool		mb_isalpha (const mbchar_t mbc);
   extern bool		mb_isascii (const mbchar_t mbc);
   extern bool		mb_isblank (const mbchar_t mbc);
   extern bool		mb_iscntrl (const mbchar_t mbc);
   extern bool		mb_isdigit (const mbchar_t mbc);
   extern bool		mb_isgraph (const mbchar_t mbc);
   extern bool		mb_islower (const mbchar_t mbc);
   extern bool		mb_isprint (const mbchar_t mbc);
   extern bool		mb_ispunct (const mbchar_t mbc);
   extern bool		mb_isspace (const mbchar_t mbc);
   extern bool		mb_isupper (const mbchar_t mbc);
   extern bool		mb_isxdigit (const mbchar_t mbc);
   extern int		mb_width (const mbchar_t mbc);
   extern void		mb_putc (const mbchar_t mbc, FILE *stream);
   extern void          mb_setascii (mbchar_t *new, char sc);
   extern void		mb_copy (mbchar_t *new, const mbchar_t *old);
 */

#ifndef _MBCHAR_H
#define _MBCHAR_H 1

#include <stdbool.h>
#include <string.h>

/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.1 has a bug: <stdio.h> and <time.h> must be included before
   <wchar.h>.  */
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#include <wctype.h>

#include "wcwidth.h"

#define MBCHAR_BUF_SIZE 24

struct mbchar
{
  const char *ptr;	/* pointer to current character */
  size_t bytes;		/* number of bytes of current character, > 0 */
  bool wc_valid;	/* true if wc is a valid wide character */
  wchar_t wc;		/* if wc_valid: the current character */
  char buf[MBCHAR_BUF_SIZE]; /* room for the bytes, used for file input only */
};

/* EOF (not a real character) is represented with bytes = 0 and
   wc_valid = false.  */

typedef struct mbchar mbchar_t;

/* Access the current character.  */
#define mb_ptr(mbc) ((mbc).ptr)
#define mb_len(mbc) ((mbc).bytes)

/* Comparison of characters.  */
#define mb_iseq(mbc, sc) ((mbc).wc_valid && (mbc).wc == (sc))
#define mb_isnul(mbc) ((mbc).wc_valid && (mbc).wc == 0)
#define mb_cmp(mbc1, mbc2) \
  ((mbc1).wc_valid							\
   ? ((mbc2).wc_valid							\
      ? (int) (mbc1).wc - (int) (mbc2).wc				\
      : -1)								\
   : ((mbc2).wc_valid							\
      ? 1								\
      : (mbc1).bytes == (mbc2).bytes					\
        ? memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes)			\
        : (mbc1).bytes < (mbc2).bytes					\
          ? (memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes) > 0 ? 1 : -1) \
          : (memcmp ((mbc1).ptr, (mbc2).ptr, (mbc2).bytes) >= 0 ? 1 : -1)))
#define mb_casecmp(mbc1, mbc2) \
  ((mbc1).wc_valid							\
   ? ((mbc2).wc_valid							\
      ? (int) towlower ((mbc1).wc) - (int) towlower ((mbc2).wc)		\
      : -1)								\
   : ((mbc2).wc_valid							\
      ? 1								\
      : (mbc1).bytes == (mbc2).bytes					\
        ? memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes)			\
        : (mbc1).bytes < (mbc2).bytes					\
          ? (memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes) > 0 ? 1 : -1) \
          : (memcmp ((mbc1).ptr, (mbc2).ptr, (mbc2).bytes) >= 0 ? 1 : -1)))
#define mb_equal(mbc1, mbc2) \
  ((mbc1).wc_valid && (mbc2).wc_valid					\
   ? (mbc1).wc == (mbc2).wc						\
   : (mbc1).bytes == (mbc2).bytes					\
     && memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes) == 0)
#define mb_caseequal(mbc1, mbc2) \
  ((mbc1).wc_valid && (mbc2).wc_valid					\
   ? towlower ((mbc1).wc) == towlower ((mbc2).wc)			\
   : (mbc1).bytes == (mbc2).bytes					\
     && memcmp ((mbc1).ptr, (mbc2).ptr, (mbc1).bytes) == 0)

/* <ctype.h>, <wctype.h> classification.  */
#define mb_isascii(mbc) \
  ((mbc).wc_valid && (mbc).wc >= 0 && (mbc).wc <= 127)
#define mb_isalnum(mbc) ((mbc).wc_valid && iswalnum ((mbc).wc))
#define mb_isalpha(mbc) ((mbc).wc_valid && iswalpha ((mbc).wc))
#define mb_isblank(mbc) ((mbc).wc_valid && iswblank ((mbc).wc))
#define mb_iscntrl(mbc) ((mbc).wc_valid && iswcntrl ((mbc).wc))
#define mb_isdigit(mbc) ((mbc).wc_valid && iswdigit ((mbc).wc))
#define mb_isgraph(mbc) ((mbc).wc_valid && iswgraph ((mbc).wc))
#define mb_islower(mbc) ((mbc).wc_valid && iswlower ((mbc).wc))
#define mb_isprint(mbc) ((mbc).wc_valid && iswprint ((mbc).wc))
#define mb_ispunct(mbc) ((mbc).wc_valid && iswpunct ((mbc).wc))
#define mb_isspace(mbc) ((mbc).wc_valid && iswspace ((mbc).wc))
#define mb_isupper(mbc) ((mbc).wc_valid && iswupper ((mbc).wc))
#define mb_isxdigit(mbc) ((mbc).wc_valid && iswxdigit ((mbc).wc))

/* Extra <wchar.h> function.  */

/* Unprintable characters appear as a small box of width 1.  */
#define MB_UNPRINTABLE_WIDTH 1

static inline int
mb_width_aux (wint_t wc)
{
  int w = wcwidth (wc);
  /* For unprintable characters, arbitrarily return 0 for control characters
     and MB_UNPRINTABLE_WIDTH otherwise.  */
  return (w >= 0 ? w : iswcntrl (wc) ? 0 : MB_UNPRINTABLE_WIDTH);
}

#define mb_width(mbc) \
  ((mbc).wc_valid ? mb_width_aux ((mbc).wc) : MB_UNPRINTABLE_WIDTH)

/* Output.  */
#define mb_putc(mbc, stream)  fwrite ((mbc).ptr, 1, (mbc).bytes, (stream))

/* Assignment.  */
#define mb_setascii(mbc, sc) \
  ((mbc)->ptr = (mbc)->buf, (mbc)->bytes = 1, (mbc)->wc_valid = 1, \
   (mbc)->wc = (mbc)->buf[0] = (sc))

/* Copying a character.  */
static inline void
mb_copy (mbchar_t *new, const mbchar_t *old)
{
  if (old->ptr == &old->buf[0])
    {
      memcpy (&new->buf[0], &old->buf[0], old->bytes);
      new->ptr = &new->buf[0];
    }
  else
    new->ptr = old->ptr;
  new->bytes = old->bytes;
  if ((new->wc_valid = old->wc_valid))
    new->wc = old->wc;
}


/* is_basic(c) tests whether the single-byte character c is in the
   ISO C "basic character set".
   This is a convenience function, and is in this file only to share code
   between mbiter_multi.h and mbfile_multi.h.  */
#if (' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
    && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
    && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
    && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
    && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
    && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
    && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
    && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
    && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
    && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
    && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
    && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
    && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
    && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
    && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
    && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
    && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
    && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
    && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
    && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
    && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
    && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
    && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126)
/* The character set is ISO-646, not EBCDIC. */
# define IS_BASIC_ASCII 1

extern unsigned int is_basic_table[];

static inline bool
is_basic (char c)
{
  return (is_basic_table [(unsigned char) c >> 5] >> ((unsigned char) c & 31))
	 & 1;
}

#else

static inline bool
is_basic (char c)
{
  switch (c)
    {
    case '\t': case '\v': case '\f':
    case ' ': case '!': case '"': case '#': case '%':
    case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case ':': case ';': case '<': case '=': case '>':
    case '?':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':
    case '[': case '\\': case ']': case '^': case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~':
      return 1;
    default:
      return 0;
    }
}

#endif

#endif /* _MBCHAR_H */
