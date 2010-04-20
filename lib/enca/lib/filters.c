/*
  @(#) $Id: filters.c,v 1.13 2004/05/11 16:14:02 yeti Exp $
  filters and hooks that various languages can use

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

/**
 * EncaBoxDraw:
 * @csname: Charset name.
 * @isvbox: All other box drawing characters.
 * @h1: Horizontal line character (light).
 * @h2: Horizontal line character (heavy).
 *
 * Information about box-drawing characters for a charset.
 **/
struct _EncaBoxDraw {
  const char *csname;
  const unsigned char *isvbox;
  unsigned char h1;
  unsigned char h2;
};

typedef struct _EncaBoxDraw EncaBoxDraw;

/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_IBM852[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1,
  1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* These are identical */
#define BOXVERT_IBM775 BOXVERT_IBM852

/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_KEYBCS2[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* These are identical */
#define BOXVERT_IBM866 BOXVERT_KEYBCS2
#define BOXVERT_CP1125 BOXVERT_KEYBCS2

/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_KOI8R[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

#if 0
/* UNUSED */
/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_KOI8RU[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1,
  1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
#endif

/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_KOI8U[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1,
  1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
static const unsigned char BOXVERT_KOI8UNI[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const EncaBoxDraw BOXDRAW[] = {
  { "cp1125", BOXVERT_CP1125, 196, 205 },
  { "ibm775", BOXVERT_IBM775, 196, 205 },
  { "ibm852", BOXVERT_IBM852, 196, 205 },
  { "ibm866", BOXVERT_IBM866, 196, 205 },
  { "keybcs2", BOXVERT_KEYBCS2, 196, 205 },
  { "koi8r", BOXVERT_KOI8R, 128, 160 },
  { "koi8u", BOXVERT_KOI8U, 128, 160 },
  { "koi8uni", BOXVERT_KOI8UNI, 128, 128 },  /* there's only one */
#if 0
  { "koi8ru", BOXVERT_KOI8RU, 128, 160 },
#endif
};

/* Local prototypes. */
static size_t filter_boxdraw_out(int charset,
                                 unsigned char *buffer,
                                 size_t size,
                                 unsigned char fill_char);

/**
 * enca_filter_boxdraw:
 * @analyser: Analyser whose charsets should be considered for filtration.
 * @fill_char: Replacement character for filtered bytes.
 *
 * Runs boxdrawing characters filter on @buffer for each charset in @language.
 *
 * Returns: Number of characters filtered out.
 **/
size_t
enca_filter_boxdraw(EncaAnalyserState *analyser,
                    unsigned char fill_char)
{
  size_t i;
  size_t filtered = 0;

  for (i = 0; i < analyser->ncharsets; i++) {
    filtered += filter_boxdraw_out(analyser->charsets[i],
                                   analyser->buffer, analyser->size,
                                   fill_char);
  }

  return filtered;
}

/**
 * filter_boxdraw_out:
 * @charset: Charset whose associated filter should be applied.
 * @buffer: Buffer to be filtered.
 * @size: Size of @buffer.
 * @fill_char: Replacement character for filtered bytes.
 *
 * Replaces box-drawing characters in @buffer with @fill_char.
 *
 * Not all possibly box-drawing characters are replaced, only those meeting
 * certain conditions to reduce false filtering.  It's assumed
 * isspace(@fill_char) is true (it aborts when it isn't).
 *
 * It's OK to call with @charset which has no filter associated, it just
 * returns zero then.
 *
 * Returns: The number of characters filtered.
 **/
static size_t
filter_boxdraw_out(int charset,
                   unsigned char *buffer,
                   size_t size,
                   unsigned char fill_char)
{
  static int charset_id[ELEMENTS(BOXDRAW)];
  static int charset_id_initialized = 0;
  const EncaBoxDraw *bd;
  size_t i, n, xout;

  assert(enca_isspace(fill_char));

  if (!charset_id_initialized) {
    for (i = 0; i < ELEMENTS(BOXDRAW); i++) {
      charset_id[i] = enca_name_to_charset(BOXDRAW[i].csname);
      assert(charset_id[i] != ENCA_CS_UNKNOWN);
    }
    charset_id_initialized = 1;
  }

  /* Find whether we have any filter associated with this charset. */
  bd = NULL;
  for (i = 0; i < ELEMENTS(BOXDRAW); i++) {
    if (charset_id[i] == charset) {
      bd = BOXDRAW + i;
      break;
    }
  }
  if (bd == NULL)
    return 0;

  xout = 0;
  /* First stage:
   * Horizontal lines, they must occur at least two in a row. */
  i = 0;
  while (i < size-1) {
    if (buffer[i] == bd->h1 || buffer[i] == bd->h2) {
      for (n = i+1; buffer[n] == buffer[i] && n < size; n++)
        ;

      if (n > i+1) {
        memset(buffer + i, fill_char, n - i);
        xout += n - i;
      }
      i = n;
    }
    else i++;
  }

  /* Second stage:
   * Vertical/mixed, they must occur separated by whitespace.
   * We assume isspace(fill_char) is true. */
  if (size > 1
      && bd->isvbox[buffer[0]]
      && enca_isspace(buffer[1])) {
    buffer[0] = fill_char;
    xout++;
  }

  for (i = 1; i < size-1; i++) {
    if (bd->isvbox[buffer[i]]
        && enca_isspace(buffer[i-1])
        && enca_isspace(buffer[i+1])) {
      buffer[i] = fill_char;
      xout++;
    }
  }

  if (size > 1
      && bd->isvbox[buffer[size-1]]
      && enca_isspace(buffer[size-2])) {
    buffer[size-1] = fill_char;
    xout++;
  }

  return xout;
}

/**
 * enca_language_hook_ncs:
 * @analyser: Analyser whose charset ratings are to be modified.
 * @ncs: The number of charsets.
 * @hookdata: What characters of which charsets should be given the extra
 *            weight.
 *
 * Decide between two charsets differing only in a few characters.
 *
 * If the two most probable charsets correspond to @hookdata charsets,
 * give the characters they differ half the weight of all other characters
 * together, thus allowing to decide between the two very similar charsets.
 *
 * It also recomputes @order when something changes.
 *
 * Returns: Nonzero when @ratings were actually modified, nonzero otherwise.
 **/
int
enca_language_hook_ncs(EncaAnalyserState *analyser,
                       size_t ncs,
                       EncaLanguageHookData1CS *hookdata)
{
  const int *const ids = analyser->charsets;
  const size_t ncharsets = analyser->ncharsets;
  const size_t *counts = analyser->counts;
  const size_t *const order = analyser->order;
  double *const ratings = analyser->ratings;
  size_t maxcnt, j, k, m;
  double q;

  assert(ncharsets > 0);
  assert(ncs <= ncharsets);
  if (ncs < 2)
    return 0;

  /*
  for (j = 0; j < ncharsets; j++) {
    fprintf(stderr, "%s:\t%g\n", enca_csname(ids[order[j]]), ratings[order[j]]);
  }
  */

  /* Find id's and check whether they are the first */
  for (j = 0; j < ncs; j++) {
    EncaLanguageHookData1CS *h = hookdata + j;

    /* Find charset if unknown */
    if (h->cs == (size_t)-1) {
      int id;

      id = enca_name_to_charset(h->name);
      assert(id != ENCA_CS_UNKNOWN);
      k = 0;
      while (k < ncharsets && id != ids[k])
        k++;
      assert(k < ncharsets);
      h->cs = k;
    }

    /* If any charset is not between the first ncs ones, do nothing. */
    k = 0;
    while (k < ncs && order[k] != h->cs)
      k++;
    if (k == ncs)
      return 0;
  }

  /* Sum the extra-important characters and find maximum. */
  maxcnt = 0;
  for (j = 0; j < ncs; j++) {
    EncaLanguageHookData1CS const *h = hookdata + j;

    for (m = k = 0; k < h->size; k++)
      m += counts[h->list[k]];
    if (m > maxcnt)
      maxcnt = m;
  }
  if (maxcnt == 0)
    return 0;

  /* Substract something from charsets that have less than maximum. */
  q = 0.5 * ratings[order[0]]/(maxcnt + EPSILON);
  for (j = 0; j < ncs; j++) {
    EncaLanguageHookData1CS const *h = hookdata + j;

    m = maxcnt;
    for (k = 0; k < h->size; k++)
      m -= counts[h->list[k]];
    ratings[h->cs] -= q*m;
  }

  enca_find_max_sec(analyser);

  return 1;
}

/**
 * enca_language_hook_eol:
 * @analyser: Analyser whose charset ratings are to be modified.
 * @ncs: The number of charsets.
 * @hookdata: What characters of which charsets should be decided with based
 *            on the EOL type.
 *
 * Decide between two charsets differing only in EOL type or other surface.
 *
 * The (surface mask, charset) pairs are scanned in order. If a matching
 * surface is found, ratings of all other charsets in the list are zeroed.
 * So you can place a surface mask of all 1s at the end to match when nothing
 * else matches.
 *
 * All the charsets have to have the same rating, or nothing happens.
 *
 * It also recomputes @order when something changes.
 *
 * Returns: Nonzero when @ratings were actually modified, nonzero otherwise.
 **/
int
enca_language_hook_eol(EncaAnalyserState *analyser,
                       size_t ncs,
                       EncaLanguageHookDataEOL *hookdata)
{
  const int *const ids = analyser->charsets;
  const size_t ncharsets = analyser->ncharsets;
  const size_t *const order = analyser->order;
  double *const ratings = analyser->ratings;
  size_t j, k;

  assert(ncharsets > 0);
  assert(ncs <= ncharsets);
  if (ncs < 2)
    return 0;

  /* Rating equality check. */
  for (j = 1; j < ncs; j++) {
    if (fabs(ratings[order[j-1]] - ratings[order[j]]) > EPSILON)
      return 0;
  }

  /* Find id's and check whether they are the first */
  for (j = 0; j < ncs; j++) {
    EncaLanguageHookDataEOL *h = hookdata + j;

    /* Find charset if unknown */
    if (h->cs == (size_t)-1) {
      int id;

      id = enca_name_to_charset(h->name);
      assert(id != ENCA_CS_UNKNOWN);
      k = 0;
      while (k < ncharsets && id != ids[k])
        k++;
      assert(k < ncharsets);
      h->cs = k;
    }

    /* If any charset is not between the first ncs ones, do nothing. */
    k = 0;
    while (k < ncs && order[k] != h->cs)
      k++;
    if (k == ncs)
      return 0;
  }

  /* Find first matching EOL type. */
  for (j = 0; j < ncs; j++) {
    EncaLanguageHookDataEOL const *h = hookdata + j;

    if (h->eol & analyser->result.surface) {
      int chg = 0;

      for (k = 0; k < ncs; k++) {
        h = hookdata + k;

        if (k != j && ratings[h->cs] > 0.0) {
          ratings[h->cs] = 0.0;
          chg = 1;
        }
      }
      if (chg)
        enca_find_max_sec(analyser);

      return chg;
    }
  }

  return 0;
}

/* vim: ts=2
 */

