/*
  @(#) $Id: guess.c,v 1.19 2005/12/01 10:08:53 yeti Exp $
  encoding-guesing engine

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

#include <math.h>
#include <string.h>

#include "enca.h"
#include "internal.h"

/* Number of text character needed to switch to text mode in binary filter. */
#define BIN_TEXT_CHAR_MIN 5

/**
 * FILL_CHARACTER:
 *
 * Replacement character for binary, box-drawing, etc.filters.
 * Note: enca_isspace(#FILL_CHARACTER) must be true.
 **/
#define FILL_CHARACTER ' '

static const EncaAnalyserOptions DEFAULTS = {
  1,      /* const_buffer (always set per call) */
  10,     /* min_chars */
  1.4142, /* threshold */
  1,      /* multibyte_enabled */
  1,      /* interpreted_surfaces */
  0,      /* ambiguous_mode */
  1,      /* filtering */
  1,      /* test_garbageness */
  1,      /* termination_strictness */
};

static const EncaEncoding ENCODING_UNKNOWN = { ENCA_CS_UNKNOWN, 0 };

/* local prototypes  */
static EncaEncoding analyse           (EncaAnalyserState *analyser,
                                       unsigned char *buffer,
                                       size_t size);
static EncaErrno    make_guess        (EncaAnalyserState *analyser);
static int          try_test_list     (EncaAnalyserState *analyser,
                                       EncaGuessFunc *tests);
static size_t       filter_binary     (unsigned char *buffer,
                                       size_t size,
                                       unsigned char fill_char);
static int          looks_like_qp     (EncaAnalyserState *analyser);
static EncaErrno    ambiguous_hook    (EncaAnalyserState *analyser);
static void         count_characters  (EncaAnalyserState *analyser);
static int          test_garbage      (EncaAnalyserState *analyser);
static size_t       check_significant (EncaAnalyserState *analyser);

/**
 * enca_guess_init:
 * @analyser: Analyser to initialize.
 *
 * Allocates and initializes analyser state, sets options to defaults.
 *
 * Assumes @analyser is unitinialized, calling with an initialized @analyser
 * leads to memory leak, but @analyser->lang must be already initialized.
 **/
void
enca_guess_init(EncaAnalyserState *analyser)
{
  assert(analyser->lang != NULL);

  analyser->counts = NEW(size_t, 0x100);
  if (analyser->ncharsets == 0) {
    analyser->ratings = NULL;
    analyser->order = NULL;
  }
  else {
    analyser->ratings = NEW(double, analyser->ncharsets);
    analyser->order = NEW(size_t, analyser->ncharsets);
  }

  analyser->options = DEFAULTS;
  analyser->gerrno = 0;
}

/**
 * enca_guess_destroy:
 * @analyser: Analyser to destroy.
 *
 * Frees memory owned by analyser state.
 **/
void
enca_guess_destroy(EncaAnalyserState *analyser)
{
  enca_free(analyser->counts);
  enca_free(analyser->ratings);
  enca_free(analyser->order);
}

/**
 * enca_analyse:
 * @analyser: An analyser initialized for some language.
 * @buffer: Buffer to be analysed.
 * @size: Size of @buffer.
 *
 * Analyses @buffer and finds its encoding.
 *
 * The @buffer is checked for 8bit encodings of language for which @analyser
 * was initialized and for multibyte encodings, mostly independent on language
 * (unless disabled with enca_set_multibyte()).
 *
 * The contents of @buffer may be (and probably will be) modified during the
 * analyse.  Use enca_analyse_const() instead if this discomforts you.
 *
 * Returns: Encoding of @buffer.  When charset part of return value is
 *          #ENCA_CS_UNKNOWN, encoding was not determined.  Check
 *          enca_errno() for reason.
 **/
EncaEncoding
enca_analyse(EncaAnalyser analyser,
             unsigned char *buffer,
             size_t size)
{
  assert(analyser != NULL);
  analyser->options.const_buffer = 0;
  return analyse(analyser, buffer, size);
}

/**
 * enca_analyse_const:
 * @analyser: An analyser initialized for some language.
 * @buffer: Buffer to be analysed.
 * @size: Size of @buffer.
 *
 * Analyses @buffer and finds its encoding.
 *
 * The @buffer is checked for 8bit encodings of language for which @analyser
 * was initialized and for multibyte encodings, mostly independent on language
 * (unless disabled with enca_set_multibyte()).
 *
 * This function never modifies @buffer (can be even used with string literal
 * @buffer) at the expense it's generally slower than enca_analyse().
 *
 * Returns: Encoding of @buffer.  When charset part of return value is
 *          #ENCA_CS_UNKNOWN, encoding was not determined.  Check
 *          enca_errno() for reason.
 **/
EncaEncoding
enca_analyse_const(EncaAnalyserState *analyser,
                   const unsigned char *buffer,
                   size_t size)
{
  assert(analyser != NULL);
  analyser->options.const_buffer = 1;
  return analyse(analyser, (unsigned char *)buffer, size);
}

/**
 * analyse:
 * @analyser: An analyser initialized for some language.
 * @buffer: Buffer to be analysed.
 * @size: Size of @buffer.
 *
 * Analyses @buffer and finds its encoding.
 *
 * Returns: Encoding of @buffer.
 **/
static EncaEncoding
analyse(EncaAnalyserState *analyser,
        unsigned char *buffer,
        size_t size)
{
  analyser->result = ENCODING_UNKNOWN;

  /* Empty buffer? */
  if (size == 0) {
    analyser->gerrno = ENCA_EEMPTY;
    return analyser->result;
  }
  assert(buffer != NULL);

  /* Initialize stuff. */
  analyser->gerrno = 0;

  analyser->buffer = buffer;
  analyser->size = size;

  analyser->buffer2 = NULL;
  analyser->size2 = 0;

  analyser->gerrno = make_guess(analyser);
  if (analyser->gerrno)
    analyser->result = ENCODING_UNKNOWN;

  /* When buffer2 is not NULL, then it holds the original buffer, so we must
   * free the copy (i.e. buffer, not buffer2!). */
  if (analyser->buffer2 != NULL)
    enca_free(analyser->buffer);

  return analyser->result;
}

/**
 * make_guess:
 * @analyser: An analyser whose buffer is to be analysed.
 *
 * Finds encoding of @buffer and stores it in @analyser->result.
 *
 * Returns: Zero on success, nonzero error code when the encoding was not
 * determined.
 **/
static EncaErrno
make_guess(EncaAnalyserState *analyser)
{
  const unsigned short int *const *const weights = analyser->lang->weights;
  const unsigned short int *const significant = analyser->lang->significant;
  size_t *const counts = analyser->counts;
  size_t *const order = analyser->order;
  double *const ratings = analyser->ratings;
  const EncaAnalyserOptions *const options = &(analyser->options);
  unsigned char *buffer = analyser->buffer;
  size_t size = analyser->size;

  static int ascii = ENCA_CS_UNKNOWN; /* ASCII charset id */

  size_t fchars; /* characters filtered out */
  size_t i, cs;

  /* Initialize when we are called the first time. */
  if (ascii == ENCA_CS_UNKNOWN) {
    ascii = enca_name_to_charset("ascii");
    assert(ascii != ENCA_CS_UNKNOWN);
  }

  /* Count characters. */
  count_characters(analyser);

  /* Pure ascii file (but may be qp-encoded!). */
  if (!analyser->bin && !analyser->up) {
    if (options->multibyte_enabled) {
      if (try_test_list(analyser, ENCA_MULTIBYTE_TESTS_ASCII))
        return 0;
    }

    if (options->interpreted_surfaces && looks_like_qp(analyser)) {
      /* Quoted printables => recompute aliases and recount characters. */
      buffer = analyser->buffer;
      size = analyser->size;
      count_characters(analyser);
    }

    if (!analyser->bin && !analyser->up) {
      /* Plain ascii. */
      analyser->result.charset = ascii;
      analyser->result.surface |= enca_eol_surface(buffer, size,
                                                   analyser->counts);
      return 0;
    }
  }

  /* Binary encodings (binary noise is handled later). */
  if (analyser->bin && options->multibyte_enabled) {
    if (try_test_list(analyser, ENCA_MULTIBYTE_TESTS_BINARY))
      return 0;
  }
  /* When interpreted surfaces are not allowed and sample contains binary data,
   * we can give it up right here. */
  if (!options->interpreted_surfaces && analyser->bin)
    return ENCA_EGARBAGE;

  /* Multibyte 8bit sample (utf-8), this has to be tested before
   * filtering too -- no language independent multibyte encoding can be
   * assumed to survive it. */
  if (!analyser->bin && analyser->up && options->multibyte_enabled) {
    if (try_test_list(analyser, ENCA_MULTIBYTE_TESTS_8BIT))
      return 0;
  }

  /* Now it can still be a regular 8bit charset (w/ or w/o noise), language
   * dependent MBCS (w/ or w/o noise), ascii w/ noise or just garbage. */

  /* When the buffer must be treated as const and filters are enabled
   * (and we didn't created a copy earlier), create a copy and store
   * original into buffer2 */
  if (options->const_buffer
      && options->filtering
      && analyser->buffer2 == NULL) {
    analyser->buffer2 = buffer;
    analyser->size2 = size;
    analyser->buffer = memcpy(enca_malloc(size), buffer, size);
    buffer = analyser->buffer;
  }

  /* Filter out blocks of binary data and box-drawing characters. */
  fchars = 0;
  if (options->filtering) {
    if (analyser->bin) {
      fchars = filter_binary(buffer, size, FILL_CHARACTER);
      if (fchars)
        analyser->result.surface |= ENCA_SURFACE_EOL_BIN;
    }
    fchars += enca_filter_boxdraw(analyser, FILL_CHARACTER);
  }

  /* At least something should remain after filtering. */
  if (size - fchars < sqrt((double)size))
    return ENCA_EFILTERED;

  /* Detect surface. */
  analyser->result.surface |= enca_eol_surface(buffer, size, counts);

  /* When sample has been damaged by filters, recount characters. */
  if (fchars) {
    count_characters(analyser);

    if (!analyser->up) {
      analyser->result.charset = ascii;
      /* FIXME: What if it's ASCII + box-drawing characters? */
      analyser->result.surface |= ENCA_SURFACE_EOL_BIN;
      return 0;
    }
  }

  /* Check multibyte 8bit sample (utf-8) again.
   * Chances are filtering helped it, even if it most probably destroyed it. */
  if (analyser->up && options->multibyte_enabled) {
    if (try_test_list(analyser, ENCA_MULTIBYTE_TESTS_8BIT_TOLERANT))
      return 0;
  }

  /* When no regular charsets are present (i.e. language is `none')
   * nothing of the following procedure has sense so just quit. */
  if (analyser->ncharsets == 0)
    return ENCA_ENOCS8;

  /* How many significant characters we caught? */
  if (!check_significant(analyser))
      return ENCA_ESIGNIF;

  /* Try pair analysis first. */
  if (enca_pair_analyse(analyser))
    return 0;

  /* Regular, language dependent 8bit charsets.
   *
   * When w_rs is relative occurence of character s in charset r we multiply
   * count[s] with factor (the sum in denominator is so-called significancy)
   *
   *          w
   *           rs
   *   ----------------
   *           ___
   *          \
   *    eps +  >   w
   *          /___  rs
   *            r
   */
  if (weights) {
    for (cs = 0; cs < analyser->ncharsets; cs++) {
      ratings[cs] = 0.0;
      for (i = 0; i < 0x100; i++) {
        ratings[cs] += weights[cs][i]/(significant[i] + EPSILON)*counts[i];
      }
    }
  } else {
    assert(analyser->lang->ratinghook);
    analyser->lang->ratinghook(analyser);
  }

  /* Find winner and second best. */
  enca_find_max_sec(analyser);

  /* Run langauge specific hooks. */
  if (analyser->ncharsets > 1 && analyser->lang->hook)
    analyser->lang->hook(analyser);

  /* Now we have found charset with the best relative ratings
     but we need an absolute test to detect total garbage. */
  if (options->test_garbageness && weights
      && test_garbage(analyser))
      return ENCA_EGARBAGE;

  /* Do we have a winner? */
  if (analyser->ncharsets == 1) {
    analyser->result.charset = analyser->charsets[order[0]];
    return 0;
  }

  if (ratings[order[0]]/(ratings[order[1]] + EPSILON)
      < options->threshold + EPSILON) {
    /* Unfortunately no, but in ambiguous mode have the last chance. */
    if (options->ambiguous_mode && weights)
      return ambiguous_hook(analyser);

    return ENCA_EWINNER;
  }
  analyser->result.charset = analyser->charsets[order[0]];

  return 0;
}

/**
 * filter_binary:
 * @buffer: Buffer to be filtered.
 * @size: Size of @buffer.
 * @fill_char: Replacement character.
 *
 * Replace blocks of binary characters in @buffer with @fill_char.
 *
 * Returns: Number of characters filtered out.
 **/
static size_t
filter_binary(unsigned char *buffer,
              size_t size,
              unsigned char fill_char)
{
  int mode; /* Mode 0 == text; mode > 0 binary, contains number of character
               remaining to switch to text mode. */
  size_t i, xout;
  unsigned char old[BIN_TEXT_CHAR_MIN - 1];  /* saved maybe-text characters */

  mode = 0;
  xout = 0;
  for (i = 0; i < size; i++) {
    if (enca_isbinary(buffer[i]))
      mode = BIN_TEXT_CHAR_MIN;
    else {
      if (mode > 0) {
        if (enca_istext(buffer[i])) {
          mode--;
          if (mode == 0) {
            /* Restore saved characters. */
            unsigned char *b = buffer + i+1 - BIN_TEXT_CHAR_MIN;
            size_t j;

            for (j = 0; j < BIN_TEXT_CHAR_MIN-1; j++)
              b[j] = old[j];
            xout -= BIN_TEXT_CHAR_MIN-1;
          }
          else
            /* Save this text character. */
            old[BIN_TEXT_CHAR_MIN - mode - 1] = buffer[i];
        }
        else mode = BIN_TEXT_CHAR_MIN;
      }
    }
    /* Fill binary characters with FILL_CHARACTER */
    if (mode > 0) {
      buffer[i] = fill_char;
      xout++;
    }
  }

  /* Return number of characters filled with fill_char. */
  return xout;
}

/**
 * ambiguous_hook:
 * @analyser: An analyser.
 *
 * Checks whether we can determine the charset, allowing the result to be
 * ambiguous.
 *
 * I.e. checks wheter meaning of all present characters is the same in all
 * charsets under threshold, and if so, set @analyser->result accordingly.
 *
 * Returns: Zero on success, #ENCA_EWINNER when result cannot be determined.
 **/
static EncaErrno
ambiguous_hook(EncaAnalyserState *analyser)
{
  const double *const ratings = analyser->ratings;
  const size_t max = analyser->order[0];
  const int csmax = analyser->charsets[max];
  const double t = analyser->options.threshold;
  size_t i;

  for (i = 0; i < analyser->ncharsets; i++) {
    if (i != max
        && ratings[max]/(ratings[i] + EPSILON) < t + EPSILON) {
      if (!enca_charsets_subset_identical(csmax, analyser->charsets[i],
                                          analyser->counts))
        return ENCA_EWINNER;
    }
  }

  if (analyser->lang->eolhook)
    analyser->lang->eolhook(analyser);

  analyser->result.charset = analyser->charsets[analyser->order[0]];
  return 0;
}

/**
 * try_test_list:
 * @analyser: An analyser.
 * @tests: List of test functions, NULL-terminated.
 *
 * Sequentially try @tests until some succeed or the list is exhausted.
 *
 * Returns: Nonzero when some test succeeded (@analyser->result is then set
 *          accordingly), zero otherwise.
 **/
static int
try_test_list(EncaAnalyserState *analyser,
              EncaGuessFunc *tests)
{
  int i;

  for (i = 0; tests[i] != NULL; i++) {
    if (tests[i](analyser))
      return 1;
  }

  return 0;
}

/**
 * looks_like_qp:
 * @analyser: An analyser.
 *
 * Checks whether @analyser buffer is QP-encoded.
 *
 * If so, it sets ENCA_SURFACE_QP in result and removes this surface, saving
 * original buffer in @analyser->buffer2 (if not saved yet).
 *
 * Must be called with buffer containing only 7bit characters!
 *
 * Returns: Nonzero when @analyser buffer is QP-encoded.
 **/
static int
looks_like_qp(EncaAnalyserState *analyser)
{
  /* QP escape character. */
  static const unsigned char QP_ESCAPE = '=';

  /* THIS IS A GENERATED TABLE, see tools/expand_table.pl */
  static const short int HEXDIGITS[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    1,  2,  3,  4,  5,  6,  7,  8,  9, 10,  0,  0,  0,  0,  0,  0, 
    0, 11, 12, 13, 14, 15, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
  };

  unsigned char *buffer = analyser->buffer;
  size_t size = analyser->size;

  size_t qpcount = 0; /* number of qp encoded characters */
  size_t reduce = 0; /* how shorter will be the buffer after removing /qp */
  unsigned char *buffer2;
  unsigned char *p, *p2, *p0;

  /* When the file doesn't contain enough escape characters,
     don't waste time scanning it. */
  if (analyser->counts[(int)QP_ESCAPE] < analyser->options.min_chars)
    return 0;

  /* Count qp encoded characters. */
  p = memchr(buffer, QP_ESCAPE, size);
  while (p != NULL && (size_t)(p-buffer) + 2 < size) {
    if (*p == QP_ESCAPE) {
      reduce++;
      p++;
      if (*p == CR || *p == LF) {
        while ((size_t)(p-buffer) < size
               && (*p == CR || *p == LF)) {
          reduce++;
          p++;
        }
        continue;
      }

      if (HEXDIGITS[*p] && HEXDIGITS[*(p+1)]) {
        qpcount++;
        reduce++;
      }
      else
        return 0;

      p += 2;
      continue;
    }
    p = memchr(p, QP_ESCAPE, size - (p - buffer));
  }

  /* When we decide it is qp encoded text, extract the 8bit characters right
     now. */
  if (qpcount < analyser->options.min_chars)
    return 0;

  analyser->result.surface |= ENCA_SURFACE_QP;
  /* When the buffer must be treated as const (and we didn't created a copy
   * earlier), create a copy and store original into buffer2 */
  analyser->size -= reduce;
  if (analyser->options.const_buffer
      && analyser->buffer2 == NULL) {
    analyser->buffer2 = buffer;
    analyser->size2 = size;
    analyser->buffer = enca_malloc(analyser->size);
    buffer = analyser->buffer;
    buffer2 = analyser->buffer2;
  }
  else
    buffer2 = analyser->buffer;

  /* Don't mess with too much queer stuff, OTOH try to create something
     strict UTF-8 parser can accept. */
  p2 = buffer2;
  p0 = buffer;
  p = memchr(buffer2, QP_ESCAPE, size);
  while (p != NULL && (size_t)(p-buffer2) + 2 < size) {
    memmove(p0, p2, p-p2);
    p0 += p - p2;
    p++;
    if (*p == CR || *p == LF) {
      while ((size_t)(p-buffer2) < size
             && (*p == CR || *p == LF))
        p++;
    }
    else {
      *p0++ = ((HEXDIGITS[*p]-1) << 4) + HEXDIGITS[*(p+1)]-1;
      p += 2;
    }
    p2 = p;
    p = memchr(p, QP_ESCAPE, size - (p - buffer2));
  }
  memmove(p0, p2, size - (p2 - buffer2));

  return 1;
}

/**
 * enca_eol_surface:
 * @buffer: A buffer whose EOL type is to be detected.
 * @size: Size of @buffer.
 * @counts: Character counts.
 *
 * Find EOL type of sample in @buffer.
 *
 * Returns: The EOL surface flags.
 **/
EncaSurface
enca_eol_surface(const unsigned char *buffer,
                 size_t size,
                 const size_t *counts)
{
  unsigned char *p;
  size_t i;

  /* Return BINARY when the sample contains some strange characters.
   * Note this can happen only with filtering off. */
  for (i = 0; i < 0x20; i++) {
     if (counts[i] && enca_isbinary(i))
       return ENCA_SURFACE_EOL_BIN;
  }
  /* Return LF (Unix) also when the sample doesn't contain any EOLs at all. */
  if (counts[CR] == 0)
    return ENCA_SURFACE_EOL_LF;
  /* But when it contain no LF's then it's Mac. */
  if (counts[LF] == 0)
    return ENCA_SURFACE_EOL_CR;
  /* Otherwise it's MS-DOG or garbage. */
  if (counts[CR] != counts[LF])
    return ENCA_SURFACE_EOL_MIX;

  /* MS-DOS */
  p = memchr(buffer+1, LF, size-1);
  while (p != NULL) {
    if (*(p-1) != CR)
      return ENCA_SURFACE_EOL_MIX;
    p++;
    p = memchr(p, LF, size - (p - buffer));
  }

  return ENCA_SURFACE_EOL_CRLF;
}

/**
 * enca_find_max_sec:
 * @analyser: An analyser.
 *
 * Updates @analyser->order according to charset @ratings.
 *
 * XXX: This should be stable sort.  The ordering is defined by
 * data/<lang>/<lang>.h header file which is in turn defined by odering in
 * the appropriate script (doit.sh).  Silly.
 *
 * Must not be called with @analyser with no regular charsets.
 **/
void
enca_find_max_sec(EncaAnalyserState *analyser)
{
  double *const ratings = analyser->ratings;
  size_t *const order = analyser->order;
  size_t i, j;

  assert(analyser->ncharsets >= 1);

  /* Always start from 0, 1, 2, 3, ... */
  for (i = 0; i < analyser->ncharsets; i++)
    order[i] = i;

  /* Sort it with stable sort */
  for (i = 0; i+1 < analyser->ncharsets; i++) {
    double m = ratings[order[i]];

    for (j = i+1; j < analyser->ncharsets; j++) {
      if (m < ratings[order[j]]) {
        size_t z = order[j];

        order[j] = order[i];
        order[i] = z;
        m = ratings[z];
      }
    }
  }
}

/**
 * count_characters:
 * @analyser: An analyser.
 *
 * Recomputes occurence tables in @analyser, updating @analyser->bin and
 * @analyser->up counts too.
 **/
static void
count_characters(EncaAnalyserState *analyser)
{
  const size_t size = analyser->size;
  const unsigned char *const buffer = analyser->buffer;
  size_t *const counts = analyser->counts;
  size_t i;

  analyser->bin = 0;
  analyser->up = 0;

  for (i = 0; i < 0x100; i++)
    counts[i] = 0;

  for (i = 0; i < size; i++)
    counts[buffer[i]]++;

  for (i = 0; i < 0x100; i++) {
    if (enca_isbinary(i))
      analyser->bin += counts[i];
  }

  for (i = 0x080; i < 0x100; i++)
    analyser->up += counts[i];
}

/**
 * check_significant:
 * @analyser: An analyser.
 *
 * Counts significant characters in sample.
 * Currenly disabled for language depedent multibyte charsets.
 *
 * Returns: Nonzero when there are at least options.min_chars significant
 *          characters, zero otherwise.
 **/
static size_t
check_significant(EncaAnalyserState *analyser)
{
  const unsigned short int *const significant = analyser->lang->significant;
  size_t *const counts = analyser->counts;
  size_t i;
  size_t sgnf = 0;

  if (!significant)
    return 1;

  for (i = 0; i < 0x100; i++) {
    if (significant[i])
      sgnf += counts[i];
  }

  return sgnf >= analyser->options.min_chars;
}

/**
 * test_garbage:
 * @analyser: An analyser.
 *
 * Checks whether sample is just garbage (near white noise) or not.
 *
 * The theory behind this test is shomewhat speculative.
 *
 * Returns: Nonzero when sample is garbage, zero otherwise.
 **/
static int
test_garbage(EncaAnalyserState *analyser)
{
  const unsigned short int *const *const weights = analyser->lang->weights;
  const unsigned short int *const w = weights[analyser->order[0]];
  size_t *const counts = analyser->counts;
  double garbage, r;
  size_t i;

  /* 8bit white noise. */
  r = analyser->lang->weight_sum/128.0*analyser->options.threshold;
  garbage = 0.0;
  for (i = 0x080; i < 0x100; i++)
    garbage += (r - w[i])*counts[i];

  garbage /= analyser->lang->weight_sum;
  return garbage > 0.0;
}

/**
 * enca_set_multibyte:
 * @analyser: An analyser.
 * @multibyte: Whether multibyte encoding tests should be enabled (nonzero to
 *             enable, zero to disable).
 *
 * Enables or disables multibyte encoding tests for @analyser.
 *
 * This option is enabled by default.
 *
 * When multibyte encodings are disabled, only 8bit charsets are checked.
 * Disabling them for language with no 8bit charsets leaves only one thing
 * enca_analyse() could test: whether the sample is purely 7bit ASCII or not
 * (the latter leading to analyser failure, of course).
 *
 * Multibyte encoding detection is also affected by
 * enca_set_termination_strictness().
 **/
void
enca_set_multibyte(EncaAnalyser analyser,
                   int multibyte)
{
  assert(analyser != NULL);
  analyser->options.multibyte_enabled = (multibyte != 0);
}

/**
 * enca_get_multibyte:
 * @analyser: An analyser.
 *
 * Returns whether @analyser can return multibyte encodings.
 *
 * See enca_set_multibyte() for more detailed description of multibyte
 * encoding checking.
 *
 * Returns: Nonzero when multibyte encoding are possible, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_multibyte(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.multibyte_enabled;
}

/**
 * enca_set_interpreted_surfaces:
 * @analyser: An analyser.
 * @interpreted_surfaces: Whether interpreted surfaces tests should be enabled
 *                        (nonzero to allow, zero to disallow).
 *
 * Enables or disables interpeted surfaces tests for @analyser.
 *
 * This option is enabled by default.
 *
 * To allow simple applications which care about charset only and don't want
 * to wrestle with surfaces, neglecting surface information should not have
 * serious consequences.  While ignoring EOL type surface is feasible, and
 * ignoring UCS byteorders may be acceptable in endian-homogenous environment;
 * ignoring the fact file is Quoted-Printable encoded can have disasterous
 * consequences.  By disabling this option you can disable surfaces requiring
 * fundamental reinterpretation of the content, namely %ENCA_SURFACE_QP
 * and %ENCA_SURFACE_EOL_BIN (thus probably making enca_analyse() to fail on
 * such samples).
 **/
void
enca_set_interpreted_surfaces(EncaAnalyser analyser,
                              int interpreted_surfaces)
{
  assert(analyser != NULL);
  analyser->options.interpreted_surfaces = (interpreted_surfaces != 0);
}

/**
 * enca_get_interpreted_surfaces:
 * @analyser: An analyser.
 *
 * Returns whether @analyser allows interpreted surfaces.
 *
 * See enca_set_interpreted_surfaces() for more detailed description of
 * interpreted surfaces.
 *
 * Returns: Nonzero when interpreted surfaces are possible, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_interpreted_surfaces(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.interpreted_surfaces;
}

/**
 * enca_set_ambiguity:
 * @analyser: An analyser.
 * @ambiguity: Whether result can be ambiguous (nonzero to allow, zero to
 *             disallow).
 *
 * Enables or disables ambiguous mode for @analyser.
 *
 * This option is disabled by default.
 *
 * In ambiguous mode some result is returned even when the charset cannot be
 * determined uniquely, provided that sample contains only characters which
 * have the same meaning in all charsets under consideration.
 **/
void
enca_set_ambiguity(EncaAnalyser analyser,
                   int ambiguity)
{
  assert(analyser != NULL);
  analyser->options.ambiguous_mode = (ambiguity != 0);
}

/**
 * enca_get_ambiguity:
 * @analyser: An analyser.
 *
 * Returns whether @analyser can return ambiguous results.
 *
 * See enca_set_ambiguity() for more detailed description of ambiguous results.
 *
 * Returns: Nonzero when ambiguous results are allowed, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_ambiguity(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.ambiguous_mode;
}

/**
 * enca_set_filtering:
 * @analyser: An analyser.
 * @filtering: Whether filters should be enabled (nonzero to enable, zero to
 *             disable).
 *
 * Enables or disables filters for @analyser.
 *
 * This option is enabled by default.
 *
 * Various filters are used to filter out block of binary noise and box-drawing
 * characters that could otherwise confuse enca.  In cases this is unwanted,
 * you can disable them by setting this option to zero.
 **/
void
enca_set_filtering(EncaAnalyser analyser,
                   int filtering)
{
  assert(analyser != NULL);
  analyser->options.filtering = (filtering != 0);
}

/**
 * enca_get_filtering:
 * @analyser: An analyser.
 *
 * Returns whether @analyser has filtering enabled.
 *
 * See enca_set_filtering() for more detailed description of filtering.
 *
 * Returns: Nonzero when filtering is enabled, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_filtering(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.filtering;
}

/**
 * enca_set_garbage_test:
 * @analyser: An analyser.
 * @garabage_test: Whether garbage test should be allowed (nonzero to enable,
 *                 nzero to disable).
 *
 * Enables or disables garbage test for @analyser.
 *
 * This option is enabled by default.
 *
 * To prevent white noise (and almost-white noise) from being accidentally
 * detected as some charset, a garbage test is used.  In cases this is
 * unwanted, you can disable is by setting this option to zero.
 **/
void
enca_set_garbage_test(EncaAnalyser analyser,
                      int garabage_test)
{
  assert(analyser != NULL);
  analyser->options.test_garbageness = (garabage_test != 0);
}

/**
 * enca_get_garbage_test:
 * @analyser: An analyser.
 *
 * Returns whether @analyser has garbage test enabled.
 *
 * See enca_set_garbage_test() for more detailed description of garbage test.
 *
 * Returns: Nonzero when garbage test is enabled, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_garbage_test(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.test_garbageness;
}

/**
 * enca_set_termination_strictness:
 * @analyser: An analyser.
 * @termination_strictness: Whether multibyte sequences are required to be
 *                          terminated correctly at the end of sample
 *                          (nonzero to require, zero to relax).
 *
 * Enables or disables requiring multibyte sequences to be terminated correctly
 * at the end of sample.
 *
 * This option is enabled by default.
 *
 * The sample given to enca_analyse() generally may not be a complete text
 * (e.g. for efficiency reasons).  As a result, it may end in the middle of a
 * multibyte sequence.  In this case, you should disable this option to
 * prevent rejecting some charset just because the sample don't terminate
 * correctly.  On the other hand, when given sample contains whole text, you
 * should always enable this option to assure correctness of the result.
 *
 * Note this option does NOT affect fixed character size encodings, like UCS-2
 * and UCS-4; sample is never assumed to contain UCS-2 text when its size is
 * not even (and similarly for UCS-4).
 **/
void
enca_set_termination_strictness(EncaAnalyser analyser,
                                int termination_strictness)
{
  assert(analyser != NULL);
  analyser->options.termination_strictness = (termination_strictness != 0);
}

/**
 * enca_get_termination_strictness:
 * @analyser: An analyser.
 *
 * Returns whether @analyser requires strict termination.
 *
 * See enca_set_termination_strictness() for more detailed description of
 * termination strictness.
 *
 * Returns: Nonzero when strict termination is required, zero otherwise.
 *
 * Since: 1.3.
 **/
int
enca_get_termination_strictness(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.termination_strictness;
}

/**
 * enca_set_significant:
 * @analyser: An analyser.
 * @significant: Minimal number of required significant characters.
 *
 * Sets the minimal number of required significant characters.
 *
 * The default value of this option is 10.
 *
 * enca_analyse() refuses to make a decision unles at least this number
 * of significant characters is found in sample.  You may want to lower this
 * number for very short texts.
 *
 * Returns: Zero on success, nonzero on failure, i.e. when you passed zero
 *          as @significant.  It sets analyser errno to ENCA_EINVALUE on
 *          failure.
 **/
int
enca_set_significant(EncaAnalyser analyser,
                     size_t significant)
{
  assert(analyser != NULL);
  if (significant == 0)
    return analyser->gerrno = ENCA_EINVALUE;

  analyser->options.min_chars = significant;
  return 0;
}

/**
 * enca_get_significant:
 * @analyser: An analyser.
 *
 * Returns the minimum number of significant characters required by @analyser.
 *
 * See enca_set_significant() for more detailed description of required
 * significant characters.
 *
 * Returns: The minimum number of significant characters.
 *
 * Since: 1.3.
 **/
size_t
enca_get_significant(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.min_chars;
}

/**
 * enca_set_threshold:
 * @analyser: An analyser.
 * @threshold: Minimal ratio between winner and second best guess.
 *
 * Sets the minimal ratio between the most probable and the second most
 * probable charsets.
 *
 * The default value of this option is 1.4142.
 *
 * enca_analyse() consideres the result known only when there's a clear gap
 * between the most probable and the second most probable charset
 * proababilities.  Lower @threshold values mean larger probability of a
 * mistake and smaller probability of not recognizing a charset; higher
 * @threshold values the contrary.  Threshold value of 2 is almost infinity.
 *
 * Returns: Zero on success, nonzero on failure, i.e. when you passed value
 *          smaller than 1.0 as @threshold.  It sets analyser errno to
 *          ENCA_EINVALUE on failure.
 **/
int
enca_set_threshold(EncaAnalyser analyser,
                   double threshold)
{
  assert(analyser != NULL);
  if (threshold < 1.0)
    return analyser->gerrno = ENCA_EINVALUE;

  analyser->options.threshold = threshold;
  return 0;
}

/**
 * enca_get_threshold:
 * @analyser: An analyser.
 *
 * Returns the threshold value used by @analyser.
 *
 * See enca_set_threshold() for more detailed threshold description.
 *
 * Returns: The threshold value.
 *
 * Since: 1.3.
 **/
double
enca_get_threshold(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->options.threshold;
}

/* vim: ts=2 sw=2 et
 */

