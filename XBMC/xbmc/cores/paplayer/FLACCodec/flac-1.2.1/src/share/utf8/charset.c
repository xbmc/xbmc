/*
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
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
 */

/*
 * See the corresponding header file for a description of the functions
 * that this file provides.
 *
 * This was first written for Ogg Vorbis but could be of general use.
 *
 * The only deliberate assumption about data sizes is that a short has
 * at least 16 bits, but this code has only been tested on systems with
 * 8-bit char, 16-bit short and 32-bit int.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef HAVE_ICONV /* should be ifdef USE_CHARSET_CONVERT */

#include <stdlib.h>

#include "share/alloc.h"
#include "charset.h"

#include "charmaps.h"

/*
 * This is like the standard strcasecmp, but it does not depend
 * on the locale. Locale-dependent functions can be dangerous:
 * we once had a bug involving strcasecmp("iso", "ISO") in a
 * Turkish locale!
 *
 * (I'm not really sure what the official standard says
 * about the sign of strcasecmp("Z", "["), but usually
 * we're only interested in whether it's zero.)
 */

static int ascii_strcasecmp(const char *s1, const char *s2)
{
  char c1, c2;

  for (;; s1++, s2++) {
    if (!*s1 || !*s1)
      break;
    if (*s1 == *s2)
      continue;
    c1 = *s1;
    if ('a' <= c1 && c1 <= 'z')
      c1 += 'A' - 'a';
    c2 = *s2;
    if ('a' <= c2 && c2 <= 'z')
      c2 += 'A' - 'a';
    if (c1 != c2)
      break;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

/*
 * UTF-8 equivalents of the C library's wctomb() and mbtowc().
 */

int utf8_mbtowc(int *pwc, const char *s, size_t n)
{
  unsigned char c;
  int wc, i, k;

  if (!n || !s)
    return 0;

  c = *s;
  if (c < 0x80) {
    if (pwc)
      *pwc = c;
    return c ? 1 : 0;
  }
  else if (c < 0xc2)
    return -1;
  else if (c < 0xe0) {
    if (n >= 2 && (s[1] & 0xc0) == 0x80) {
      if (pwc)
	*pwc = ((c & 0x1f) << 6) | (s[1] & 0x3f);
      return 2;
    }
    else
      return -1;
  }
  else if (c < 0xf0)
    k = 3;
  else if (c < 0xf8)
    k = 4;
  else if (c < 0xfc)
    k = 5;
  else if (c < 0xfe)
    k = 6;
  else
    return -1;

  if (n < (size_t)k)
    return -1;
  wc = *s++ & ((1 << (7 - k)) - 1);
  for (i = 1; i < k; i++) {
    if ((*s & 0xc0) != 0x80)
      return -1;
    wc = (wc << 6) | (*s++ & 0x3f);
  }
  if (wc < (1 << (5 * k - 4)))
    return -1;
  if (pwc)
    *pwc = wc;
  return k;
}

int utf8_wctomb(char *s, int wc1)
{
  unsigned int wc = wc1;

  if (!s)
    return 0;
  if (wc < (1u << 7)) {
    *s++ = wc;
    return 1;
  }
  else if (wc < (1u << 11)) {
    *s++ = 0xc0 | (wc >> 6);
    *s++ = 0x80 | (wc & 0x3f);
    return 2;
  }
  else if (wc < (1u << 16)) {
    *s++ = 0xe0 | (wc >> 12);
    *s++ = 0x80 | ((wc >> 6) & 0x3f);
    *s++ = 0x80 | (wc & 0x3f);
    return 3;
  }
  else if (wc < (1u << 21)) {
    *s++ = 0xf0 | (wc >> 18);
    *s++ = 0x80 | ((wc >> 12) & 0x3f);
    *s++ = 0x80 | ((wc >> 6) & 0x3f);
    *s++ = 0x80 | (wc & 0x3f);
    return 4;
  }
  else if (wc < (1u << 26)) {
    *s++ = 0xf8 | (wc >> 24);
    *s++ = 0x80 | ((wc >> 18) & 0x3f);
    *s++ = 0x80 | ((wc >> 12) & 0x3f);
    *s++ = 0x80 | ((wc >> 6) & 0x3f);
    *s++ = 0x80 | (wc & 0x3f);
    return 5;
  }
  else if (wc < (1u << 31)) {
    *s++ = 0xfc | (wc >> 30);
    *s++ = 0x80 | ((wc >> 24) & 0x3f);
    *s++ = 0x80 | ((wc >> 18) & 0x3f);
    *s++ = 0x80 | ((wc >> 12) & 0x3f);
    *s++ = 0x80 | ((wc >> 6) & 0x3f);
    *s++ = 0x80 | (wc & 0x3f);
    return 6;
  }
  else
    return -1;
}

/*
 * The charset "object" and methods.
 */

struct charset {
  int max;
  int (*mbtowc)(void *table, int *pwc, const char *s, size_t n);
  int (*wctomb)(void *table, char *s, int wc);
  void *map;
};

int charset_mbtowc(struct charset *charset, int *pwc, const char *s, size_t n)
{
  return (*charset->mbtowc)(charset->map, pwc, s, n);
}

int charset_wctomb(struct charset *charset, char *s, int wc)
{
  return (*charset->wctomb)(charset->map, s, wc);
}

int charset_max(struct charset *charset)
{
  return charset->max;
}

/*
 * Implementation of UTF-8.
 */

static int mbtowc_utf8(void *map, int *pwc, const char *s, size_t n)
{
  (void)map;
  return utf8_mbtowc(pwc, s, n);
}

static int wctomb_utf8(void *map, char *s, int wc)
{
  (void)map;
  return utf8_wctomb(s, wc);
}

/*
 * Implementation of US-ASCII.
 * Probably on most architectures this compiles to less than 256 bytes
 * of code, so we can save space by not having a table for this one.
 */

static int mbtowc_ascii(void *map, int *pwc, const char *s, size_t n)
{
  int wc;

  (void)map;
  if (!n || !s)
    return 0;
  wc = (unsigned char)*s;
  if (wc & ~0x7f)
    return -1;
  if (pwc)
    *pwc = wc;
  return wc ? 1 : 0;
}

static int wctomb_ascii(void *map, char *s, int wc)
{
  (void)map;
  if (!s)
    return 0;
  if (wc & ~0x7f)
    return -1;
  *s = wc;
  return 1;
}

/*
 * Implementation of ISO-8859-1.
 * Probably on most architectures this compiles to less than 256 bytes
 * of code, so we can save space by not having a table for this one.
 */

static int mbtowc_iso1(void *map, int *pwc, const char *s, size_t n)
{
  int wc;

  (void)map;
  if (!n || !s)
    return 0;
  wc = (unsigned char)*s;
  if (wc & ~0xff)
    return -1;
  if (pwc)
    *pwc = wc;
  return wc ? 1 : 0;
}

static int wctomb_iso1(void *map, char *s, int wc)
{
  (void)map;
  if (!s)
    return 0;
  if (wc & ~0xff)
    return -1;
  *s = wc;
  return 1;
}

/*
 * Implementation of any 8-bit charset.
 */

struct map {
  const unsigned short *from;
  struct inverse_map *to;
};

static int mbtowc_8bit(void *map1, int *pwc, const char *s, size_t n)
{
  struct map *map = map1;
  unsigned short wc;

  if (!n || !s)
    return 0;
  wc = map->from[(unsigned char)*s];
  if (wc == 0xffff)
    return -1;
  if (pwc)
    *pwc = (int)wc;
  return wc ? 1 : 0;
}

/*
 * For the inverse map we use a hash table, which has the advantages
 * of small constant memory requirement and simple memory allocation,
 * but the disadvantage of slow conversion in the worst case.
 * If you need real-time performance while letting a potentially
 * malicious user define their own map, then the method used in
 * linux/drivers/char/consolemap.c would be more appropriate.
 */

struct inverse_map {
  unsigned char first[256];
  unsigned char next[256];
};

/*
 * The simple hash is good enough for this application.
 * Use the alternative trivial hashes for testing.
 */
#define HASH(i) ((i) & 0xff)
/* #define HASH(i) 0 */
/* #define HASH(i) 99 */

static struct inverse_map *make_inverse_map(const unsigned short *from)
{
  struct inverse_map *to;
  char used[256];
  int i, j, k;

  to = (struct inverse_map *)malloc(sizeof(struct inverse_map));
  if (!to)
    return 0;
  for (i = 0; i < 256; i++)
    to->first[i] = to->next[i] = used[i] = 0;
  for (i = 255; i >= 0; i--)
    if (from[i] != 0xffff) {
      k = HASH(from[i]);
      to->next[i] = to->first[k];
      to->first[k] = i;
      used[k] = 1;
    }

  /* Point the empty buckets at an empty list. */
  for (i = 0; i < 256; i++)
    if (!to->next[i])
      break;
  if (i < 256)
    for (j = 0; j < 256; j++)
      if (!used[j])
	to->first[j] = i;

  return to;
}

int wctomb_8bit(void *map1, char *s, int wc1)
{
  struct map *map = map1;
  unsigned short wc = wc1;
  int i;

  if (!s)
    return 0;

  if (wc1 & ~0xffff)
    return -1;

  if (1) /* Change 1 to 0 to test the case where malloc fails. */
    if (!map->to)
      map->to = make_inverse_map(map->from);

  if (map->to) {
    /* Use the inverse map. */
    i = map->to->first[HASH(wc)];
    for (;;) {
      if (map->from[i] == wc) {
	*s = i;
	return 1;
      }
      if (!(i = map->to->next[i]))
	break;
    }
  }
  else {
    /* We don't have an inverse map, so do a linear search. */
    for (i = 0; i < 256; i++)
      if (map->from[i] == wc) {
	*s = i;
	return 1;
      }
  }

  return -1;
}

/*
 * The "constructor" charset_find().
 */

struct charset charset_utf8 = {
  6,
  &mbtowc_utf8,
  &wctomb_utf8,
  0
};

struct charset charset_iso1 = {
  1,
  &mbtowc_iso1,
  &wctomb_iso1,
  0
};

struct charset charset_ascii = {
  1,
  &mbtowc_ascii,
  &wctomb_ascii,
  0
};

struct charset *charset_find(const char *code)
{
  int i;

  /* Find good (MIME) name. */
  for (i = 0; names[i].bad; i++)
    if (!ascii_strcasecmp(code, names[i].bad)) {
      code = names[i].good;
      break;
    }

  /* Recognise some charsets for which we avoid using a table. */
  if (!ascii_strcasecmp(code, "UTF-8"))
    return &charset_utf8;
  if (!ascii_strcasecmp(code, "US-ASCII"))
    return &charset_ascii;
  if (!ascii_strcasecmp(code, "ISO-8859-1"))
    return &charset_iso1;

  /* Look for a mapping for a simple 8-bit encoding. */
  for (i = 0; maps[i].name; i++)
    if (!ascii_strcasecmp(code, maps[i].name)) {
      if (!maps[i].charset) {
	maps[i].charset = (struct charset *)malloc(sizeof(struct charset));
	if (maps[i].charset) {
	  struct map *map = (struct map *)malloc(sizeof(struct map));
	  if (!map) {
	    free(maps[i].charset);
	    maps[i].charset = 0;
	  }
	  else {
	    maps[i].charset->max = 1;
	    maps[i].charset->mbtowc = &mbtowc_8bit;
	    maps[i].charset->wctomb = &wctomb_8bit;
	    maps[i].charset->map = map;
	    map->from = maps[i].map;
	    map->to = 0; /* inverse mapping is created when required */
	  }
	}
      }
      return maps[i].charset;
    }

  return 0;
}

/*
 * Function to convert a buffer from one encoding to another.
 * Invalid bytes are replaced by '#', and characters that are
 * not available in the target encoding are replaced by '?'.
 * Each of TO and TOLEN may be zero, if the result is not needed.
 * The output buffer is null-terminated, so it is all right to
 * use charset_convert(fromcode, tocode, s, strlen(s), &t, 0).
 */

int charset_convert(const char *fromcode, const char *tocode,
		    const char *from, size_t fromlen,
		    char **to, size_t *tolen)
{
  int ret = 0;
  struct charset *charset1, *charset2;
  char *tobuf, *p, *newbuf;
  int i, j, wc;

  charset1 = charset_find(fromcode);
  charset2 = charset_find(tocode);
  if (!charset1 || !charset2 )
    return -1;

  tobuf = (char *)safe_malloc_mul2add_(fromlen, /*times*/charset2->max, /*+*/1);
  if (!tobuf)
    return -2;

  for (p = tobuf; fromlen; from += i, fromlen -= i, p += j) {
    i = charset_mbtowc(charset1, &wc, from, fromlen);
    if (!i)
      i = 1;
    else if (i == -1) {
      i  = 1;
      wc = '#';
      ret = 2;
    }
    j = charset_wctomb(charset2, p, wc);
    if (j == -1) {
      if (!ret)
	ret = 1;
      j = charset_wctomb(charset2, p, '?');
      if (j == -1)
	j = 0;
    }
  }

  if (tolen)
    *tolen = p - tobuf;
  *p++ = '\0';
  if (to) {
    newbuf = realloc(tobuf, p - tobuf);
    *to = newbuf ? newbuf : tobuf;
  }
  else
    free(tobuf);

  return ret;
}

#endif /* USE_CHARSET_ICONV */
