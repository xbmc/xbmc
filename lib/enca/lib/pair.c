/*
  @(#) $Id: pair.c,v 1.6 2003/11/17 12:27:39 yeti Exp $
  pair-frequency based tests (used for 8bit-dense languages)

  Copyright (C) 2003 David Necas (Yeti) <yeti@physics.muni.cz>

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
#include <string.h>

#include "enca.h"
#include "internal.h"

/* Local prototypes. */
static size_t count_all_8bit_pairs(EncaAnalyserState *analyser);
static void compute_pair2bits(EncaAnalyserState *analyser);
static void count_good_pairs(EncaAnalyserState *analyser);

/**
 * enca_pair_init:
 * @analyser: Analyzer state to be initialized.
 *
 * Initializes pair statistics data.
 *
 * In fact it just sets everything to #NULL, to be initialized when needed.
 **/
void
enca_pair_init(EncaAnalyserState *analyser)
{
  analyser->bitcounts = NULL;
  analyser->pairratings = NULL;
  analyser->pair2bits = NULL;
}

/**
 * enca_pair_destroy:
 * @analyser: Analyzer state whose pair statistics part should be destroyed.
 *
 * Destroys the pair statistics part of analyser state @analyser.
 **/
void
enca_pair_destroy(EncaAnalyserState *analyser)
{
  enca_free(analyser->pair2bits);
  enca_free(analyser->bitcounts);
  enca_free(analyser->pairratings);
}

/**
 * enca_pair_analyse:
 * @analyser: Analysed containing the sample for pair frequency analysis.
 *
 * Performs pair-frequency based analysis, provided that the language supports
 * it (does nothing otherwise).
 *
 * Returns: Nonzero when the character set was succesfully determined,
 *          @analyser->@result.@charset is then directly modified.
 **/
int
enca_pair_analyse(EncaAnalyserState *analyser)
{
  const unsigned char *const *letters = analyser->lang->letters;
  const unsigned char **const *pairs = analyser->lang->pairs;
  size_t ncharsets = analyser->ncharsets;
  size_t i, best, all8bitpairs;
  double q;

  if (!letters || !pairs)
    return 0;

  if (!analyser->pairratings)
    analyser->pairratings = NEW(size_t, ncharsets);

  /* count the good pairs and find winner
   * initialize when we are called the first time */
  if (!analyser->pair2bits) {
    compute_pair2bits(analyser);
    analyser->bitcounts = NEW(size_t, 1 << ncharsets);
  }
  memset(analyser->pairratings, 0, ncharsets*sizeof(size_t));
  all8bitpairs = count_all_8bit_pairs(analyser);
  count_good_pairs(analyser);

  best = 0;
  for (i = 1; i < ncharsets; i++) {
    if (analyser->pairratings[i] > analyser->pairratings[best])
      best = i;
  }

  /* Just a Right Value */
  q = 1.0 - exp(3.0*(1.0 - analyser->options.threshold));
  if (analyser->pairratings[best] >= analyser->options.min_chars
      && analyser->pairratings[best] >= q*all8bitpairs) {
    analyser->result.charset = analyser->charsets[best];
    return 1;
  }

  /* I don't like saying it, but the sample seems to be garbage... */
  return 0;
}

/**
 * count_all_8bit_pairs:
 * @analyser: An analyser.
 *
 * Count all pairs containing at least one 8bit characters.
 *
 * Returns: The number of such pairs.
 **/
static size_t
count_all_8bit_pairs(EncaAnalyserState *analyser)
{
  unsigned char *buffer = analyser->buffer;
  size_t size = analyser->size;
  size_t i, c, sum8bits;

  sum8bits = 0;
  c = FILL_NONLETTER;
  for (i = size; i; i--) {
    if ((c | *buffer) & 0x80)
      sum8bits++;
    c = *(buffer++);
  }
  if (size && (c & 0x80))
    sum8bits++;

  return sum8bits;
}

/**
 * count_good_pairs:
 * @analyser: An analyser.
 *
 * Count `good' pairs for each charset.
 *
 * Makes use of @analyser->pair2bits.  See compute_pair2bits() comment for
 * description of how it works.
 **/
static void
count_good_pairs(EncaAnalyserState *analyser)
{
  size_t *ratings = analyser->pairratings;
  unsigned char *pair2bits = analyser->pair2bits;
  size_t *bitcounts = analyser->bitcounts;
  size_t ncharsets = analyser->ncharsets;
  const unsigned char *buffer = analyser->buffer;
  size_t size = analyser->size;
  size_t i, j, c, cs;

  assert(ncharsets <= 8);
  assert(pair2bits);
  assert(bitcounts);
  assert(ratings);

  memset(bitcounts, 0, (1 << ncharsets)*sizeof(size_t));
  c = FILL_NONLETTER << 8;
  for (i = size; i; i--) {
    bitcounts[pair2bits[c | *buffer]]++;
    c = *(buffer++) << 8;
  }
  if (size)
    bitcounts[pair2bits[c | FILL_NONLETTER]]++;

  memset(ratings, 0, ncharsets*sizeof(size_t));
  for (cs = 0; cs < ncharsets; cs++) {
    size_t bit = 1 << cs;
    size_t rating = 0;

    for (i = 0; i < (1U << ncharsets); i += 2*bit) {
      for (j = i+bit; j < i+2*bit; j++)
        rating += bitcounts[j];
    }
    ratings[cs] = rating;
  }
}

/**
 * compute_pair2bits:
 * @analyser: An analyser.
 *
 * Allocate and fill @analyser->@pair2bits.
 *
 * The meaning of pair2bits is following: it's indexed by pairs (0x100*first
 * + second) and each @pair2bits element has set a bit corresponding to a
 * charset when the pair is `good' for the charset.
 *
 * To determine `good' pair counts for all charsets we don't have to count
 * the pairs, we only have to count the bit combinations (@bitcounts) and the
 * per charset pair counts can be easily constructed from them -- by summing
 * those with the particular bit set.
 **/
static void
compute_pair2bits(EncaAnalyserState *analyser)
{
  size_t ncharsets = analyser->ncharsets;
  size_t cs, c;

  assert(analyser->pair2bits == NULL);
  assert(analyser->ncharsets <= 8);

  analyser->pair2bits = NEW(unsigned char, 0x10000);
  memset(analyser->pair2bits, 0, 0x10000);
  for (cs = 0; cs < ncharsets; cs++) {
    const unsigned char *letters = analyser->lang->letters[cs];
    const unsigned char *const *pairs = analyser->lang->pairs[cs];
    size_t bit = 1 << cs;

    for (c = 0; c < 0x100; c++) {
      size_t j = letters[c];

      if (j != 255) {
        const unsigned char *s = pairs[j];
        unsigned char *p = analyser->pair2bits + (c << 8);

        /* set the corresponding bit for all pairs */
        do {
          p[(size_t)*(s++)] |= bit;
        } while (*s);
      }
    }
  }
}
