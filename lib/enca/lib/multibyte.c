/*
  @(#) $Id: multibyte.c,v 1.13 2005/12/01 10:08:53 yeti Exp $
  multibyte character set checks

  Copyright (C) 2000-2003 David Necas (Yeti) <yeti@physics.muni.cz>

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as published
  by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif /* HAVE_CONFIG_H */

#include <math.h>
#include <string.h>

#include "enca.h"
#include "internal.h"

/*
 * See http://www.unicode.org/unicode/faq/utf_bom.html#25 for BOMs:
 * 00 00 FE FF      UTF-32, big-endian
 * FF FE 00 00      UTF-32, little-endian
 * FE FF            UTF-16, big-endian
 * FF FE            UTF-16, little-endian
 * EF BB BF         UTF-8
 */

/* Local prototypes. */
static int    is_valid_utf8       (EncaAnalyserState *analyser);
static int    looks_like_TeX      (EncaAnalyserState *analyser);
static int    is_valid_utf7       (EncaAnalyserState *analyser);
static int    looks_like_hz       (EncaAnalyserState *analyser);
static int    looks_like_ucs2     (EncaAnalyserState *analyser);
static int    looks_like_ucs4     (EncaAnalyserState *analyser);
static int    looks_like_utf8     (EncaAnalyserState *analyser);
static size_t what_if_it_was_ucs4 (const unsigned char *buffer,
                                   size_t size,
                                   size_t min_chars,
                                   EncaSurface *crlf_surf);
static void   shuffle_byte_order  (unsigned char *buffer,
                                   size_t size,
                                   EncaSurface permutation);

/* Multibyte test lists.
 * These arrays must be NULL-terminated. */
EncaGuessFunc ENCA_MULTIBYTE_TESTS_ASCII[] = {
  &is_valid_utf7,
  &looks_like_TeX,
  &looks_like_hz,
  NULL
};

EncaGuessFunc ENCA_MULTIBYTE_TESTS_8BIT[] = {
  &is_valid_utf8,
  NULL
};

EncaGuessFunc ENCA_MULTIBYTE_TESTS_BINARY[] = {
  &looks_like_ucs4,
  &looks_like_ucs2,
  NULL
};

EncaGuessFunc ENCA_MULTIBYTE_TESTS_8BIT_TOLERANT[] = {
  &looks_like_utf8,
  NULL
};

/**
 * is_valid_utf8:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer contains valid UTF-8.
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
is_valid_utf8(EncaAnalyserState *analyser)
{
  static int utf8 = ENCA_CS_UNKNOWN; /* UTF-8 charset */
  size_t size = analyser->size;
  const unsigned char *buffer = analyser->buffer;
  const size_t *const counts = analyser->counts;

  /* Bonus added when we catch a byte order marker. */
  size_t bom_bonus;

  int remains_10xxxxxx = 0;       /* how many next bytes have to be 10xxxxxx */
  int utf8count = 0;              /* number of UTF-8 encoded characters */
  size_t i;
  unsigned char b;

  /* Bytes 0xfe and 0xff just cannot appear in utf-8 in any case. */
  if (counts[0xfe] || counts[0xff])
    return 0;

  /* Initialize when we are called the first time. */
  if (utf8 == ENCA_CS_UNKNOWN) {
    utf8 = enca_name_to_charset("utf-8");
    assert(utf8 != ENCA_CS_UNKNOWN);
  }

  /* Check BOM */
  bom_bonus = (size_t)(sqrt((double)size) + size/10.0);
  if (size >= 3
      && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
    utf8count += bom_bonus;
    buffer += 3;
    size -= 3;
  }

  /* Parse. */
  for (i = 0; i < size; i++) {
    b = buffer[i];
    if (!remains_10xxxxxx) {
      if ((b & 0x80) == 0) /* 7bit characters */
        continue;
      if ((b & 0xe0) == 0xc0) { /* 110xxxxx 10xxxxxx sequence */
        remains_10xxxxxx = 1;
        utf8count++;
        continue;
      }
      if ((b & 0xf0) == 0xe0) { /* 1110xxxx 2 x 10xxxxxx sequence */
        remains_10xxxxxx = 2;
        utf8count++;
        continue;
      }
      /* Following are valid 32-bit UCS characters, but not 16-bit Unicode,
         they are very rare, nevertheless we accept them. */
      if ((b & 0xf8) == 0xf0) { /* 1110xxxx 3 x 10xxxxxx sequence */
        remains_10xxxxxx = 3;
        utf8count++;
        continue;
      }
      if ((b & 0xfc) == 0xf8) { /* 1110xxxx 4 x 10xxxxxx sequence */
        remains_10xxxxxx = 4;
        utf8count++;
        continue;
      }
      if ((b & 0xfe) == 0xfc) { /* 1110xxxx 5 x 10xxxxxx sequence */
        remains_10xxxxxx = 5;
        utf8count++;
        continue;
      }
      /* We can get here only when input is invalid: (b & 0xc0) == 0x80. */
      return 0;
    }
    else {
      /* Broken 10xxxxxx sequence? */
      if ((b & 0xc0) != 0x80) {
        return 0;
      }
      remains_10xxxxxx--;
    }
  }

  /* Unfinished 10xxxxxx sequence. */
  if (remains_10xxxxxx != 0 && analyser->options.termination_strictness > 0)
    return 0;

  if (utf8count < (int)analyser->options.min_chars)
    return 0;

  analyser->result.charset = utf8;
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);
  return 1;
}

/**
 * looks_like_TeX:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer contains TeX-encoded 8bit characters.
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
looks_like_TeX(EncaAnalyserState *analyser)
{
  /* TeX escape character, skip-characters, punctuation and alpha accents */

  /* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
  static const unsigned char TEX_ACCPUNCT[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  };

  /* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
  static const unsigned char TEX_ACCALPHA[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 
    0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  };

  static const unsigned char TEX_ESCAPE = '\\';
  static const unsigned char TEX_BRACE = '{';

  static int TeX = ENCA_CS_UNKNOWN; /* TeX charset */

  const unsigned char *const buffer = analyser->buffer;
  const size_t size = analyser->size;
  const size_t *const counts = analyser->counts;

  size_t TeXaccents = 0; /* number of TeX accents */
  const unsigned char *p;

  /* When the file doesn't contain enough escape characters,
     don't waste time scanning it. */
  if (counts[TEX_ESCAPE] < analyser->options.min_chars)
    return 0;

  /* Initialize when we are called the first time. */
  if (TeX == ENCA_CS_UNKNOWN) {
    TeX = enca_name_to_charset("TeX");
    assert(TeX != ENCA_CS_UNKNOWN);
  }

  /* [roughly] count TeX accents */
  p = memchr(buffer, TEX_ESCAPE, size);
  while (p != NULL && (size_t)(p-buffer) + 2 < size) {
    if (*p == TEX_ESCAPE) {
      p++;
      if (*p == TEX_ESCAPE)
        p++; /* catch \\ */
      if (TEX_ACCPUNCT[*p]
          || (TEX_ACCALPHA[*p]
              && (*++p == TEX_BRACE || enca_isspace((char)*p)))) {
        while ((size_t)(p-buffer) + 1 < size
               && (*++p == TEX_BRACE || enca_isspace((char)*p)))
          ;
        if (enca_isalpha(*p)) TeXaccents++;
      }
      continue;
    }
    p = memchr(p, TEX_ESCAPE, size - (p - buffer));
  }

  if (TeXaccents < analyser->options.min_chars)
    return 0;

  analyser->result.charset = TeX;
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);
  return 1;
}

/**
 * is_valid_utf7:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer contains valid UTF-7
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
is_valid_utf7(EncaAnalyserState *analyser)
{
  /* UTF-7 special characters. */
  static const unsigned char UTF7_ESCAPE = '+';
  /* This is not a bug. `+-' is `+' in UTF-7. */
  static const unsigned char UTF7_PLUS = '-';

  /* Base64 base (or so-called set B), see RFC1521, RFC1642 */
  /* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
  static const short int BASE64[] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 63,  0,  0,  0, 64,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62,  0,  0,  0,  0,  0,  0,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,  0,  0,  0,  0,  0,
     0, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  };

  static int utf7 = ENCA_CS_UNKNOWN; /* UTF-7 charset */

  const unsigned char *const buffer = analyser->buffer;
  const size_t size = analyser->size;
  const size_t *const counts = analyser->counts;

  size_t utf7count = 0; /* number of >7bit characters */
  unsigned char *p,*q;

  /* When the file doesn't contain enough UTF-7 shift characters,
     don't waste time scanning it. */
  if (counts[UTF7_ESCAPE] < analyser->options.min_chars)
    return 0;

  /* Initialize when we are called the first time. */
  if (utf7 == ENCA_CS_UNKNOWN) {
    utf7 = enca_name_to_charset("utf-7");
    assert(utf7 != ENCA_CS_UNKNOWN);
  }

  p = memchr(buffer, UTF7_ESCAPE, size);
  while (p != NULL && (size_t)(p-buffer) + 1 < size) {
    p++;
    if (*p == UTF7_PLUS) { /* +- */
      /* Don't count +- since it's often used for 0x00b1 in plain 7bit ascii. */
      /* utf7count++; */
    } else {
      for (q = p; (size_t)(q-buffer) < size && BASE64[*q]; q++)
        ;
      if ((size_t)(q-buffer) == size) {
        p = q;
        break;
      }
      /* check whether all padding bits are 0's (don't try to understand how) */
      if (q-p == 0
          || ((BASE64[*(q-1)]-1) & 0x3f>>(6 - 6*(q - p)%8)))
        return 0;

      utf7count += 6*(q - p)/16;
      p = q;
    }
    p = memchr(p, UTF7_ESCAPE, size - (p - buffer));
  }

  /* p != NULL means unsinished sequence here. */ 
  if (p != NULL && analyser->options.termination_strictness > 0)
    return 0;

  if (utf7count < analyser->options.min_chars)
    return 0;

  analyser->result.charset = utf7;
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);
  return 1;
}

/**
 * looks_like_ucs2:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer contains UCS-2 encoded characters.
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
looks_like_ucs2(EncaAnalyserState *analyser)
{
  static int ucs2 = ENCA_CS_UNKNOWN; /* UCS-2 charset id */

  const unsigned char *const buffer = analyser->buffer;
  const size_t size = analyser->size;
  const size_t *const counts = analyser->counts;

  /* Bonus added when we catch a byte order marker. */
  size_t bom_bonus;

  size_t ucs2count = 0; /* something like number of good ucs-2 characters */
  unsigned int byte_order = 0; /* default byte order is little endian
                                * (msb first) */
  unsigned int byte_order_changes = 0; /* how many times byte_order changed */
  size_t cr = 0; /* number of CR's */
  size_t lf = 0; /* number of LF's */
  int crlf_ok = 1; /* are all LF's preceeded by CR's? */
  unsigned char b1, b2;
  double r;
  size_t i;

  /* The number of bytes must be of course even */
  if (size%2 != 0)
    return 0;

  bom_bonus = (size_t)(sqrt((double)size) + size/10.0);

  /* When the file doesn't contain enough zeros,
     don't waste time scanning it. */
  r = (2.0*(counts[0] + counts[1] + counts[2] + counts[3] + counts[4])
       + bom_bonus)/size;
  if (r < log(analyser->options.threshold + EPSILON))
    return 0;

  /* Initialize when we are called the first time. */
  if (ucs2 == ENCA_CS_UNKNOWN) {
    ucs2 = enca_name_to_charset("ucs-2");
    assert(ucs2 != ENCA_CS_UNKNOWN);
  }

  /* Try to catch lsb even when it doesn't start with endian marker. */
  if (buffer[1] == 0 && enca_isprint(buffer[0]))
    byte_order = 1;

  /* Scan buffer. */
  for (i = 0; i < size; i += 2) {
    b1 = buffer[i + byte_order];
    b2 = buffer[i+1 - byte_order];
    /* Byte order marker detection. */
    if (b1 == 0xfe && b2 == 0xff) {
      if (i == 0)
        ucs2count += bom_bonus;
      else
        byte_order_changes++;
      continue;
    }
    if (b1 == 0xff && b2 == 0xfe) {
      byte_order = 1-byte_order;
      if (i == 0)
        ucs2count += bom_bonus;
      else
        byte_order_changes++;
      continue;
    }
    /* Black magic.
     * When almost any word can be UCS-2 character, we have to assume some
     * are far more probable. */
    if (b1 == 0) {
      ucs2count += (enca_isprint(b2) || enca_isspace(b2)) ? 2 : 0;
      /* check EOLs */
      if (b2 == CR)
        cr++;
      if (b2 == LF) {
        lf++;
        if (i > 0
            && (buffer[i-1-byte_order] != CR
                || buffer[i-2+byte_order] != 0))
          crlf_ok = 0;
      }
    }
    else {
      if (b1 <= 4)
        ucs2count += 2;
    }
  }

  /* Now we have to decide what we tell to the caller. */
  r = (double)ucs2count/size;
  if (r < log(analyser->options.threshold + EPSILON)
      || ucs2count/2 < analyser->options.min_chars)
    return 0;

  analyser->result.charset = ucs2;

  /* Byte order surface. */
  if (byte_order_changes)
    analyser->result.surface |= ENCA_SURFACE_PERM_MIX;
  else
    analyser->result.surface |= byte_order ? ENCA_SURFACE_PERM_21: 0;

  /* EOL surface. */
  if (cr == 0)
    analyser->result.surface |= ENCA_SURFACE_EOL_LF;
  else {
    if (lf == 0)
      analyser->result.surface |= ENCA_SURFACE_EOL_CR;
    else {
      analyser->result.surface |= crlf_ok
                                  ? ENCA_SURFACE_EOL_CRLF
                                  : ENCA_SURFACE_EOL_MIX;
    }
  }

  return 1;
}

/**
 * looks_like_ucs4:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer contains UCS-4 encoded characters.
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
looks_like_ucs4(EncaAnalyserState *analyser)
{
  static const EncaSurface PERMS[] = {
    ENCA_SURFACE_PERM_4321,
    ENCA_SURFACE_PERM_21
  };

  static int ucs4 = ENCA_CS_UNKNOWN; /* UCS-4 charset id */

  unsigned char *buffer = analyser->buffer;
  const size_t size = analyser->size;
  const size_t *const counts = analyser->counts;

  ssize_t ucs4count = 0; /* ucs-4-icity */
  size_t count_perm[4]; /* counts for various byteorders */
  EncaSurface eol[4]; /* EOL types for various byteorders */
  double r; /* rating */
  size_t i, max;

  /* The number of bytes must be of course multiple of 4. */
  if (size%4 != 0)
    return 0;

  /* When the file doesn't contain enough zeros (and other small bytes),
     don't waste time scanning it. */
  r = (4.0*(counts[0] + counts[1] + counts[2] + counts[3] + counts[4])/3.0)
      /size;
  if (r < log(analyser->options.threshold + EPSILON))
    return 0;

  /* Initialize when we are called the first time. */
  if (ucs4 == ENCA_CS_UNKNOWN) {
    ucs4 = enca_name_to_charset("ucs-4");
    assert(ucs4 != ENCA_CS_UNKNOWN);
  }

  /* Try all sensible unsigned charorders and find maximum.
     At the end the buffer has the same byteorder as it had, but when
     the buffer have to be considered const, work on copy. */
  if (analyser->options.const_buffer) {
    buffer = memcpy(enca_malloc(size), buffer, size);
  }

  max = 0;
  for (i = 0; i < 4; i++) {
    count_perm[i] = what_if_it_was_ucs4(buffer, size,
                                        analyser->options.min_chars,
                                        eol + i);
    if (count_perm[i] > count_perm[max])
      max = i;
    shuffle_byte_order(buffer, size, PERMS[i%2]);
  }

  if (analyser->options.const_buffer)
    enca_free(buffer);

  /* Use quite a cruel selection to restrain other byteorders. */
  ucs4count = 2*count_perm[max];
  for (i = 0; i < 4; i++)
    ucs4count -= count_perm[i];

  /* Now we have to decide what we tell to the caller. */
  r = (double)ucs4count/size;
  if (r < log(analyser->options.threshold + EPSILON)
      || ucs4count/4 < (int)analyser->options.min_chars)
    return 0;

  analyser->result.charset = ucs4;
  /* Compute what permutation corresponds to max. */
  for (i = 0; i < max; i++)
    analyser->result.surface ^= PERMS[i%2];
  analyser->result.surface |= eol[max];

  return 1;
}

/**
 * what_if_it_was_ucs4:
 * @buffer: Buffer to be checked.
 * @size: Size of @buffer.
 * @min_chars: Minimal number of `nice' UCS-4 characters to succeede.
 * @crlf_surf: Where detected EOL surface type should be stored.
 *
 * Checks whether @buffer contains little endian UCS-4 encoded characters.
 *
 * Assumes @buffer contains little endian UCS-4 and returns the number of
 * `good' characters, and in case it's at least @min_chars, finds EOL surface
 * type too.
 *
 * Returns: The number of `good' UCS-4 characters with some bonus for a good
 * BOM.
 **/
static size_t
what_if_it_was_ucs4(const unsigned char *buffer,
                    size_t size,
                    size_t min_chars,
                    EncaSurface *crlf_surf)
{
  /* Bonus added when we catch a byte order marker. */
  size_t bom_bonus;

  size_t count = 0;   /* ucs-4-icity */
  size_t cr = 0;      /* number of CR's */
  size_t lf = 0;      /* number of LF's */
  int crlf_ok = 1;    /* are all LF's preceeded by CR's? */
  size_t i;

  /* check BOM */
  bom_bonus = (size_t)(sqrt((double)size) + size/20.0);
  if (size) {
    if (buffer[0] == 0 && buffer[1] == 0
        && buffer[2] == 0xfe && buffer[3] == 0xff) {
      count += bom_bonus;
      buffer += 4;
      size -= 4;
    }
  }

  for (i = 0; i < size; i += 4) {
    /* Does it look like little endian ucs-4? */
    if (buffer[i] == 0 && buffer[i+1] == 0) {
      if (buffer[i+2] == 0)
        count += enca_isprint(buffer[i+3]) || enca_isspace(buffer[i+3]) ? 4 : 0;
      else {
        if (buffer[i+2] < 5)
          count += 4;
      }
    }
  }

  /* Detect EOL surface
   * To be 100% portable, we do it the ugly way: by testing individual bytes. */
  if (count/4 >= min_chars) {
    for (i = 0; i < size; i += 4) {
      if (buffer[i+3] == CR && buffer[i+2] == 0
          && buffer[i+1] == 0 && buffer[i] == 0)
        cr++;
      if (buffer[i+3] == LF && buffer[i+2] == 0
          && buffer[i+1] == 0 && buffer[i] == 0) {
        lf++;
        if (crlf_ok && i > 0
            && (buffer[i-1] != CR || buffer[i-2] != 0
                || buffer[i-3] != 0 || buffer[i-4] != 0))
          crlf_ok = 0;
      }
    }
    /* EOL surface result */
    if (cr == 0)
      *crlf_surf = ENCA_SURFACE_EOL_LF;
    else {
      if (lf == 0)
        *crlf_surf = ENCA_SURFACE_EOL_CR;
      else
        *crlf_surf = crlf_ok ? ENCA_SURFACE_EOL_CRLF : ENCA_SURFACE_EOL_MIX;
    }
  }

  return count;
}

/**
 * shuffle_byte_order:
 * @buffer: Buffer to be shuffled.
 * @size: Size of @buffer.
 * @permutation: Permutation type, possible values mean
 *               0                                                no change
 *               ENCA_SURFACE_PERM_4321                           4321
 *               ENCA_SURFACE_PERM_21                             21 (== 2143)
 *               ENCA_SURFACE_PERM_21|ENCA_SURFACE_PERM_4321      3412
 *
 * Performs given permutation on @buffer.
 **/
static void
shuffle_byte_order(unsigned char *buffer,
                   size_t size,
                   EncaSurface permutation)
{
  size_t i;
  unsigned char b;

  if (permutation & ENCA_SURFACE_PERM_4321) {
    for (i = 0; i < size; i += 4) {
      b = buffer[i];
      buffer[i] = buffer[i+3];
      buffer [i+3] = b;

      b = buffer[i+1];
      buffer[i+1] = buffer[i+2];
      buffer[i+2] = b;
    }
  }

  if (permutation & ENCA_SURFACE_PERM_21) {
    for (i = 0; i < size; i += 2) {
      b = buffer[i];
      buffer[i] = buffer[i+1];
      buffer [i+1] = b;
    }
  }
}

/**
 * looks_like_utf8:
 * @analyser: Analyser whose buffer is to be checked.
 *
 * Checks whether @analyser->buffer may contain UTF-8.
 *
 * This is a fault-tolerant version of is_valid_utf8, intended to be used after
 * filtering, when a few stray 8bit characters may appear in the sample.
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
looks_like_utf8(EncaAnalyserState *analyser)
{
  static int utf8 = ENCA_CS_UNKNOWN; /* UTF-8 charset */
  size_t size = analyser->size;
  const unsigned char *buffer = analyser->buffer;
  const size_t *const counts = analyser->counts;

  /* Bonus added when we catch a byte order marker. */
  size_t bom_bonus;

  int remains_10xxxxxx = 0;       /* how many next bytes have to be 10xxxxxx */
  int utf8count = 0;              /* number of UTF-8 encoded characters */
  int failures = 0;               /* number of invalid sequences encountered */
  size_t i;
  unsigned char b;

  /* Initialize when we are called the first time. */
  if (utf8 == ENCA_CS_UNKNOWN) {
    utf8 = enca_name_to_charset("utf-8");
    assert(utf8 != ENCA_CS_UNKNOWN);
  }

  /* Check BOM */
  bom_bonus = (size_t)(sqrt((double)size) + size/10.0);
  if (size >= 3
      && buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf) {
    utf8count += bom_bonus;
    buffer += 3;
    size -= 3;
  }

  /* Parse. */
  for (i = 0; i < size; i++) {
    b = buffer[i];
    if (!remains_10xxxxxx) {
      if ((b & 0x80) == 0) /* 7bit characters */
        continue;
      if ((b & 0xe0) == 0xc0) { /* 110xxxxx 10xxxxxx sequence */
        remains_10xxxxxx = 1;
        utf8count++;
        continue;
      }
      if ((b & 0xf0) == 0xe0) { /* 1110xxxx 2 x 10xxxxxx sequence */
        remains_10xxxxxx = 2;
        utf8count++;
        continue;
      }
      /* Following are valid 32-bit UCS characters, but not 16-bit Unicode,
         they are very rare, nevertheless we accept them. */
      if ((b & 0xf8) == 0xf0) { /* 1110xxxx 3 x 10xxxxxx sequence */
        remains_10xxxxxx = 3;
        utf8count++;
        continue;
      }
      if ((b & 0xfc) == 0xf8) { /* 1110xxxx 4 x 10xxxxxx sequence */
        remains_10xxxxxx = 4;
        utf8count++;
        continue;
      }
      if ((b & 0xfe) == 0xfc) { /* 1110xxxx 5 x 10xxxxxx sequence */
        remains_10xxxxxx = 5;
        utf8count++;
        continue;
      }
      /* We can get here only when input is invalid: (b & 0xc0) == 0x80. */
      failures++;
      remains_10xxxxxx = 0;
    }
    else {
      /* Broken 10xxxxxx sequence? */
      if ((b & 0xc0) != 0x80) {
        failures++;
        utf8count--;
        remains_10xxxxxx = 0;
      }
      else
        remains_10xxxxxx--;
    }
  }

  /* Unfinished 10xxxxxx sequence. */
  if (remains_10xxxxxx != 0 && analyser->options.termination_strictness > 0)
    failures += 2;

  if (utf8count < (int)analyser->options.min_chars)
    return 0;

  /* Tolerate a small number of failures. */
  if (failures > exp(-7*(analyser->options.threshold - 1.0))*utf8count/2.0)
    return 0;

  analyser->result.charset = utf8;
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);
  if (failures > 0)
    analyser->result.surface |= ENCA_SURFACE_EOL_BIN;
  return 1;
}

/**
 * looks_like_hz:
 * @analyser: An analyser.
 *
 * Checks whether @analyser buffer is HZ-encoded. See RFC 1843
 *
 * Directly modifies @analyser->result on success.
 *
 * Returns: Nonzero when @analyser->result was set, zero othewrise.
 **/
static int
looks_like_hz(EncaAnalyserState *analyser)
{
  unsigned char *buffer = analyser->buffer;
  size_t size = analyser->size;
  static int hz = ENCA_CS_UNKNOWN; /* HZ charset */
  size_t hzcount = 0; /* number of qp encoded characters */
  unsigned char *p = buffer;
  const size_t *const counts = analyser->counts;

  int escaped; /* true when we're in 8-bit mode */
  unsigned int i;

  /* Initialize when we are called the first time. */
  if (hz == ENCA_CS_UNKNOWN) {
    hz = enca_name_to_charset("hz");
    assert(hz != ENCA_CS_UNKNOWN);
  }

  for (i = 0; i < analyser->ncharsets; i++)
   if (analyser->charsets[i] ==  hz)
     goto goahead;
  return 0;

goahead:
  /* When the file doesn't contain escape characters,
     don't waste time scanning it. */
  if (counts['{'] == 0
    || counts['}'] == 0
    || counts['~'] == 0)
    return 0;

  /* Move to first escaped-in */
  /* FIXME: Things will be simpler if we have strnstr()? */
  while ((size_t)(p - buffer) + 2 < size) {
     p = memchr(p, '~', size - (p - buffer));
     if (p == NULL)
       return 0;
     if (p[1] == '{') {
       escaped = 1;
       p += 2;
       break;
     } else if (p[1] == '\n') {
       p += 2;
     } else if (p[1] == '~') {
       p += 2;
     } else
       p += 2;
  }
  
  /* Check if it's valid HZ and count hz encoded characters. */
  while (p < buffer + size) {
    if (*p == '~' && p < buffer + size - 1) {
      switch (p[1]) {
        case '~':
          if (escaped) {
            p++;
            hzcount++;
          } else {
            p += 2;
          }
          break;
        case '{':
          if (!escaped) {
            p += 2;
            escaped = 1;
          } else {
            return 0;
          }
          break;
        case '}':
          if (escaped) {
            escaped = 0;
            p += 2;
          } else {
            return 0;
          }
          break;
        case '\n':
          if (escaped) {
            return 0;
          }
          p += 2;
          break;
        default:
          if (!escaped) {
            return 0;
          }
          p++;
      }
    } else {
      /* Spaces, CR or LF not allowed in escaped block */
      if (escaped) {
        if (*p < ' ') {
          return 0;
        }
        hzcount++;
      }
      p++;
    }
  }

  if (hzcount < analyser->options.min_chars)
    return 0;

  /* Unfinished escaped block here. */ 
  if (escaped && analyser->options.termination_strictness > 0)
    return 0;

  analyser->result.charset = hz;
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);

  return 1;
}
