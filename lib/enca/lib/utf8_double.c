/*
  @(#) $Id: utf8_double.c,v 1.5 2005/11/24 10:09:03 yeti Exp $
  checks for doubly-encoded utf-8

  Copyright (C) 2000-2002 David Necas (Yeti) <yeti@physics.muni.cz>

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

#include <stdlib.h>
#include <math.h>

#include "enca.h"
#include "internal.h"

/* Local prototypes. */
static void compute_double_utf8_weights (EncaAnalyserState *analyser);
static void create_ucs2_weight_table    (EncaUTFCheckData *amap,
                                         size_t size,
                                         int *wbuf);
static void mark_scratch_buffer         (EncaAnalyserState *analyser);

/**
 * enca_double_utf8_init:
 * @analyser: Analyzer state to be initialized.
 *
 * Initializes double-UTF-8 check.
 *
 * In fact it initializes the fields to #NULL's, they are actually initialized
 * only when needed.
 **/
void
enca_double_utf8_init(EncaAnalyserState *analyser)
{
  analyser->utfch = NULL;
  analyser->utfbuf = NULL;
}

/**
 * enca_double_utf8_destroy:
 * @analyser: Analyzer state whose double-UTF-8 check part should be destroyed.
 *
 * Destroys the double-UTF-8 check part of analyser state @analyser.
 **/
void
enca_double_utf8_destroy(EncaAnalyserState *analyser)
{
  size_t i;

  if (analyser->utfch == NULL)
    return;

  enca_free(analyser->utfbuf);

  for (i = 0; i < analyser->ncharsets; i++) {
    enca_free(analyser->utfch[i].ucs2);
    enca_free(analyser->utfch[i].weights);
  }
  enca_free(analyser->utfch);
}

/**
 * enca_double_utf8_check:
 * @analyser: Analyzer state determinig the language for double-UTF-8 check.
 * @buffer: The buffer to be checked [@size].
 * @size: The size of @buffer.
 *
 * Checks buffer for double-UTF-8 encoding.
 *
 * Double-UTF-8 encoding is the result of [errorneous] conversion of UTF-8 text
 * to UTF-8 again, as if it was in some 8bit charset.  This is quite hard to
 * recover from.
 *
 * The analayser mostly only determines what language will be assumed,
 * the rest of this test is independent on the main guessing routines.
 * When @buffer doesn't containing UTF-8 text, the result is undefined
 * (namely, false positives are possible).
 *
 * Calling this function when language is `none' has currently no effect.
 *
 * Returns: Nonzero, when @buffer probably contains doubly-UTF-8 encoded text.
 *          More precisely, it returns the number of charsets which are
 *          possible candidates for source charset.  You can then use
 *          enca_double_utf8_get_candidates() to retrieve the charsets.
 **/
int
enca_double_utf8_check(EncaAnalyser analyser,
                       const unsigned char *buffer,
                       size_t size)
{
  long int ucs4char = 0;
  int remains_10xxxxxx = 0;
  size_t i;

  if (analyser->ncharsets == 0 || analyser->lang->weights == 0)
    return 0;

  /* Compute weights when we are called the first time. */
  if (analyser->utfch == NULL)
    compute_double_utf8_weights(analyser);

  mark_scratch_buffer(analyser);

  /* Parse. */
  for (i = 0; i < size; i++) {
    unsigned char b = buffer[i];

    if (!remains_10xxxxxx) {
      if ((b & 0x80) == 0) /* 7bit characters */
        continue;
      if ((b & 0xe0) == 0xc0) { /* 110xxxxx 10xxxxxx sequence */
        ucs4char = b & 0x1f;
        remains_10xxxxxx = 1;
        continue;
      }
      if ((b & 0xf0) == 0xe0) { /* 1110xxxx 2 x 10xxxxxx sequence */
        ucs4char = b & 0x0f;
        remains_10xxxxxx = 2;
        continue;
      }
      /* Following are valid 32-bit UCS characters, but not 16-bit Unicode,
         nevertheless we accept them. */
      if ((b & 0xf8) == 0xf0) { /* 1110xxxx 3 x 10xxxxxx sequence */
        ucs4char = b & 0x07;
        remains_10xxxxxx = 3;
        continue;
      }
      if ((b & 0xfc) == 0xf8) { /* 1110xxxx 4 x 10xxxxxx sequence */
        ucs4char = b & 0x03;
        remains_10xxxxxx = 4;
        continue;
      }
      if ((b & 0xfe) == 0xfc) { /* 1110xxxx 5 x 10xxxxxx sequence */
        ucs4char = b & 0x01;
        remains_10xxxxxx = 5;
        continue;
      }
      /* We can get here only when input is invalid: (b & 0xc0) == 0x80. */
      remains_10xxxxxx = 0;
    }
    else {
      /* Broken 10xxxxxx sequence? */
      if ((b & 0xc0) != 0x80) {
        remains_10xxxxxx = 0;
      }
      else {
        /* Good 10xxxxxx continuation. */
        ucs4char <<= 6;
        ucs4char |= b & 0x3f;
        remains_10xxxxxx--;

        /* Do we have a whole character?
         * (We must not touch positions in utfbuf containing zeroes.) */
        if (remains_10xxxxxx == 0
            && ucs4char < 0x10000
            && analyser->utfbuf[ucs4char] != 0) {
          if (analyser->utfbuf[ucs4char] < 0)
            analyser->utfbuf[ucs4char] = 1;
          else
            analyser->utfbuf[ucs4char]++;
        }
      }
    }
  }

  /* Compute the ratings. */
  for (i = 0; i < analyser->ncharsets; i++) {
    EncaUTFCheckData *amap = analyser->utfch + i;
    size_t j;

    amap->rating = 0.0;
    amap->result = 0;
    for (j = 0; j < amap->size; j++)
      amap->rating += analyser->utfbuf[amap->ucs2[j]] * amap->weights[j];
  }

  /* Now check whether we've found some negative ratings. */
  {
    size_t min = 0;
    size_t max = 0;

    for (i = 1; i < analyser->ncharsets; i++) {
      if (analyser->utfch[i].rating < analyser->utfch[min].rating)
        min = i;
      if (analyser->utfch[i].rating > analyser->utfch[max].rating)
        max = i;
    }

    if (analyser->utfch[min].rating < 0.0
        && -analyser->utfch[min].rating > 0.5*analyser->utfch[max].rating) {
      size_t total = 0;
      double q = analyser->utfch[min].rating
                 * (1.0 - 45.0*exp(-4.5*analyser->options.threshold));

      for (i = 0; i < analyser->ncharsets; i++) {
        if (analyser->utfch[i].rating < q) {
          analyser->utfch[i].result = 1;
          total++;
        }
      }
      return total;
    }
  }

  return 0;
}

/**
 * enca_double_utf8_get_candidates:
 * @analyser: Analyzer state for which double-UTF-8 candidates are to be
 *            returned.
 *
 * Returns array of double-UTF-8 source charset candidates from the last check.
 *
 * The returned array should be freed by caller then no longer needed. Its
 * is the return value of the preceding enca_double_utf8_check() call.
 *
 * When called before any double-UTF-8 test has been performed yet or after
 * and unsuccessfull double-UTF-8 test, it returns NULL, but the result after
 * an unsuccessfull check should be considered undefined.
 *
 * Returns: An array containing charset id's of possible source charsets from
 *          which the sample was doubly-UTF-8 encoded.  The array may contain
 *          only one value, but usually enca is not able to decide between
 *          e.g. ISO-8859-2 and Win1250, thus more candidates are returned.
 **/
int*
enca_double_utf8_get_candidates(EncaAnalyser analyser)
{
  size_t j = 0;
  size_t i;
  int *candidates;

  assert(analyser);
  if (analyser->utfch == NULL)
    return NULL;

  for (i = 0; i < analyser->ncharsets; i++) {
    if (analyser->utfch[i].result)
      j++;
  }

  if (j == 0)
    return NULL;

  candidates = NEW(int, j);
  j = 0;
  for (i = 0; i < analyser->ncharsets; i++) {
    if (analyser->utfch[i].result) {
      candidates[j] = analyser->charsets[i];
      j++;
    }
  }

  return candidates;
}

/**
 * compute_double_utf8_weights:
 * @analyser: Analyzer state whose double-UTF-8 check weigths should be
 *            computed.
 *
 * Computes UCS-2 character weights used in double-UTF-8 check.  Must be
 * called at most once for a given analyser.  It also allocates the scratch
 * buffer analyser->utfbuf and leaves it filled with zeroes.
 **/
static void
compute_double_utf8_weights(EncaAnalyserState *analyser)
{
  int *buf;
  unsigned int ucs2map[0x100];
  size_t i, j;

  assert(analyser != NULL);
  assert(analyser->lang != NULL);
  assert(analyser->utfch == NULL);
  assert(analyser->utfbuf == NULL);
  if (analyser->ncharsets == 0)
    return;

  analyser->utfch = NEW(EncaUTFCheckData, analyser->ncharsets);
  analyser->utfbuf = NEW(int, 0x10000);
  buf = analyser->utfbuf;

  for (i = 0; i < 0x10000; i++)
    buf[i] = 0;

  /* For all charsets compute UTF-8 prefix byte occurence tables and select
   * those characters having the highest difference between occurences when
   * counted as UTF-8 prefix and when counted as a regular character. */
  for (j = 0; j < analyser->ncharsets; j++) {
    const unsigned short int *const w = analyser->lang->weights[j];
    size_t table_size = 0;

    assert(enca_charset_has_ucs2_map(analyser->charsets[j]));
    enca_charset_ucs2_map(analyser->charsets[j], ucs2map);

    /* Go through all characters, some maps may map even 7bits to something
     * else. Compute required table size meanwhile. */
    for (i = 0; i < 0x100; i++) {
      unsigned int ucs2c = ucs2map[i];
      assert(ucs2c < 0x10000);

      if (w[i] == 0)
        continue;

      /* Count the character weight as positive. */
      if (ucs2c < 0x80 || ucs2c == ENCA_NOT_A_CHAR)
        continue;

      if (buf[ucs2c] == 0)
        table_size++;
      buf[ucs2c] += w[i];

      /* Transform the character and count UTF-8 transformed first byte weight
       * as negative. */
      if (ucs2c < 0x800)
        ucs2c = ucs2map[0xc0 | (ucs2c >> 6)];
      else
        ucs2c = ucs2map[0xe0 | (ucs2c >> 12)];

      if (ucs2c < 0x80 || ucs2c == ENCA_NOT_A_CHAR)
        continue;

      if (buf[ucs2c] == 0)
        table_size++;
      buf[ucs2c] -= w[i];
      if (buf[ucs2c] == 0)
        buf[ucs2c] = 1;
    }

    /* Build the table of significant UCS-2 characters, i.e. characters
     * having nonzero weight. */
    create_ucs2_weight_table(analyser->utfch + j, table_size, buf);
  }
}

/**
 * create_ucs2_weight_table:
 * @amap: A pointer to Double-UTF8-check data to be filled.
 * @size: The number of UCS-2 characters with nonzero weight in @wbuf.
 * @wbuf: UCS-2 character weights [@size].
 *
 * Creates `compressed' UCS-2 weight table.
 **/
static void
create_ucs2_weight_table(EncaUTFCheckData *amap,
                         size_t size,
                         int *wbuf)
{
  unsigned int ucs2c;
  size_t i;

  amap->size = size;
  amap->ucs2 = NEW(int, size);
  amap->weights = NEW(int, size);

  i = 0;
  for (ucs2c = 0; ucs2c < 0x10000; ucs2c++) {
    if (wbuf[ucs2c] != 0) {
      assert(i < size);

      amap->ucs2[i] = ucs2c;
      amap->weights[i] = wbuf[ucs2c];
      wbuf[ucs2c] = 0;  /* Fill the buffer with zeroes. */
      i++;
    }
  }

  assert(i == size);
}

/**
 * mark_scratch_buffer:
 * @analyser: Analyzer whose significant ucs2 characters are to be marked in
 *            @analyser->utfbuf.
 *
 * Marks significant characters in @analyser->utfbuf with -1.
 *
 * The @analyser->utfbuf buffer is magic.  Once we found the significant
 * characters in compute_double_utf8_weights(), we always keep zeroes at
 * positions of nonsiginifant characters.  This way we never have to scan
 * through the whole buffer, not even to fill it wit zeroes -- we put zeroes
 * only where we know we changed it.
 *
 * -1 is used to mark significant characters before counting, because it's not
 * zero.
 **/
static void
mark_scratch_buffer(EncaAnalyserState *analyser)
{
  size_t i, j;

  for (j = 0; j < analyser->ncharsets; j++) {
    EncaUTFCheckData *amap = analyser->utfch + j;

    for (i = 0; i < amap->size; i++)
      analyser->utfbuf[amap->ucs2[i]] = -1;
  }
}
