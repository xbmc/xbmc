/*
  language info: chinese

  Copyright (C) 2005 Meng Jie (Zuxy) <zuxy.meng@gmail.com>

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
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "enca.h"
#include "internal.h"
#include "data/chinese/chinese.h"

static int hook(EncaAnalyserState *analyser);
static int calc_rating(EncaAnalyserState *analyser);
/* Not 8-bit clean, can't be a HZ here */
static int is_hz(const unsigned char* str) { return 0; }

static const char *const CHARSET_NAMES[] = {
  "gbk",
  "big5",
  "hz"
};

static ValidityFunc* validity_check_table[] = {
  is_gbk,
  is_big5,
  is_hz
};

static RateFunc* rate_calc_table[] = {
  in_gbk,
  in_big5,
  NULL
};

#define NCHARSETS (sizeof(CHARSET_NAMES)/sizeof(const char* const))

/**
 * ENCA_LANGUAGE_ZH:
 *
 * Chinese language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_ZH = {
  "zh",
  "chinese",
  NCHARSETS,
  CHARSET_NAMES,
  0,
  0,
  0,
  0,
  0,
  &hook,
  NULL,
  NULL,
  &calc_rating
};

/**
 * hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Adjust ratings for language "zh", see calc_rating below.
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  const size_t* order = analyser->order;
  double* rating_first = &analyser->ratings[order[0]];
  double* rating_second = &analyser->ratings[order[1]];

  if (*rating_second < 0) {
    *rating_second = 0.;

    if (*rating_first < 0)
      *rating_first = 0.;
    else
      *rating_first = 1.;  /* Make sure that the first won */

    return 1;
  }

  return 0;
}

/**
 * calc_rating:
 * @analyser: An analyser.
 *
 * Calculating ratings for GBK and Big5, respectively, and
 * ratings may be set to negative values when invalid a character
 * for a charset was encoutered. This should not affect the result of
 * enca_find_max_sec, but must be adjust to positive by hook for
 * the final comparison.
 *
 * Returns: Always return 1
 **/

static int calc_rating(EncaAnalyserState *analyser)
{
  int islowbyte = 0;
  unsigned int i, j;
  unsigned char low;
  const size_t size = analyser->size;
  const unsigned char *buffer = analyser->buffer;
  double *ratings = analyser->ratings;
  int continue_check[NCHARSETS];
  const struct zh_weight* pweight;

  assert(analyser->ncharsets == NCHARSETS
         && sizeof(rate_calc_table)/sizeof(RateFunc*) == NCHARSETS
         && sizeof(validity_check_table)/sizeof(ValidityFunc*) == NCHARSETS);

  for (i = 0; i < NCHARSETS; i++) {
    continue_check[i] = 1;
    ratings[i] = 0.;
  }

  for (i = 0; i < size; i++) {
    low = buffer[i];

    /* low byte */
    if (islowbyte) {
      const unsigned char* hanzi = buffer + i - 1;

      assert(i);
      for (j = 0; j < NCHARSETS; j++) {
        if (continue_check[j]) {
          continue_check[j] = validity_check_table[j](hanzi);
          if (!continue_check[j])
            ratings[j] = -1.;
          else {
            pweight = rate_calc_table[j](hanzi);
            if (pweight)
              ratings[j] += pweight->freq;
          }
        }
      }

      islowbyte = 0;
      continue;
    }

    if (low & 0x80)
      islowbyte = 1;
  }
#ifdef DEBUG
  printf("GBK: %f, BIG5: %f\n", ratings[0], ratings[1]);
#endif

  /* Unfinished DBCS. */
  if (islowbyte && analyser->options.termination_strictness > 0)
  {
    for (i = 0; i < NCHARSETS; i++)
      ratings[i] = 0.;
  }

  return 1;
}

/* vim: ts=2
 */
