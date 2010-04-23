/*
  @(#) $Id: lang.c,v 1.18 2005/12/01 10:08:53 yeti Exp $
  uniform interface to particular languages

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

#include <string.h>

#include "enca.h"
#include "internal.h"

/**
 * Language `none'.
 *
 * This language has no regular charsets, so only multibyte encodings are
 * tested
 **/
static const EncaLanguageInfo ENCA_LANGUAGE___ = {
  "__", /* name */
  "none", /* human name */
  0,    /* number of charsets */
  NULL, /* their names */
  NULL, /* character weights */
  NULL, /* significancy data */
  NULL, /* letter data */
  NULL, /* pair data */
  0,    /* sum of weights */
  NULL, /* hook function */
  NULL, /* eolhook function */
  NULL, /* lcuchook function */
  NULL, /* ratinghook function */
};

/* All languages. */
static const EncaLanguageInfo *const LANGUAGE_LIST[] = {
  &ENCA_LANGUAGE_BE, /* Belarussian. */
  &ENCA_LANGUAGE_BG, /* Bulgarian. */
  &ENCA_LANGUAGE_CS, /* Czech. */
  &ENCA_LANGUAGE_ET, /* Estonian. */
  &ENCA_LANGUAGE_HR, /* Croatian. */
  &ENCA_LANGUAGE_HU, /* Hungarian. */
  &ENCA_LANGUAGE_LT, /* Latvian. */
  &ENCA_LANGUAGE_LV, /* Lithuanian. */
  &ENCA_LANGUAGE_PL, /* Polish. */
  &ENCA_LANGUAGE_RU, /* Russian. */
  &ENCA_LANGUAGE_SK, /* Slovak. */
  &ENCA_LANGUAGE_SL, /* Slovene. */
  &ENCA_LANGUAGE_UK, /* Ukrainian. */
  &ENCA_LANGUAGE_ZH, /* Chinese. */
  &ENCA_LANGUAGE___, /* None. */
};

#define NLANGUAGES (ELEMENTS(LANGUAGE_LIST))

/* Local prototypes. */
static int* language_charsets_ids(const EncaLanguageInfo *lang);
static const EncaLanguageInfo* find_language(const char *langname);

/**
 * enca_language_init:
 * @analyser: Analyzer state to be initialized for this language.
 * @langname: Two-letter ISO-639 language code.
 *
 * Initializes analyser for language @langname.
 *
 * Assumes @analyser is unitinialized, calling with an initialized @analyser
 * leads to memory leak.
 *
 * Returns: Nonzero on success, zero otherwise.
 **/
int
enca_language_init(EncaAnalyserState *analyser,
                   const char *langname)
{
  const EncaLanguageInfo *lang;

  assert(langname != NULL);

  analyser->lang = NULL;
  analyser->ncharsets = 0;
  analyser->charsets = NULL;
  analyser->lcbits = NULL;
  analyser->ucbits = NULL;

  lang = find_language(langname);
  if (lang == NULL)
    return 0;

  analyser->lang = lang;
  if (lang->ncharsets == 0)
    return 1;

  analyser->ncharsets = lang->ncharsets;
  analyser->charsets = language_charsets_ids(lang);

  return 1;
}

/**
 * enca_language_destroy:
 * @analyser: Analyzer state whose language part should be destroyed.
 *
 * Destroys the language part of analyser state @analyser.
 **/
void
enca_language_destroy(EncaAnalyserState *analyser)
{
  enca_free(analyser->charsets);
  enca_free(analyser->lcbits);
  enca_free(analyser->ucbits);
  analyser->ncharsets = 0;
  analyser->lang = NULL;
}

/**
 * enca_get_languages:
 * @n: The number of languages will be stored here.
 *
 * Returns list of known languages.
 *
 * The returned strings are two-letter ISO-639 language codes, the same as
 * enca_analyser_alloc() accepts.
 *
 * The list of languages has to be freed by caller; the strings themselves
 * must be considered constant and must NOT be freed.
 *
 * Returns: The list of languages, storing their number into *@n.
 **/
const char**
enca_get_languages(size_t *n)
{
  const char **languages;
  size_t i;

  languages = NEW(const char*, NLANGUAGES);
  for (i = 0; i < NLANGUAGES; i++)
    languages[i] = LANGUAGE_LIST[i]->name;

  *n = NLANGUAGES;
  return languages;
}

/**
 * enca_analyser_language:
 * @analyser: An analyser.
 *
 * Returns name of language which was @analyser initialized for.
 *
 * The returned string must be considered constant and must NOT be freed.
 *
 * Returns: The language name.
 **/
const char*
enca_analyser_language(EncaAnalyser analyser)
{
  assert(analyser != NULL);
  return analyser->lang->name;
}

/**
 * enca_language_english_name:
 * @lang: A two-letter language code, such as obtained from
 *        enca_analyser_language() or enca_get_languages().
 *
 * Returns an English name of a language given its ISO-639 code.
 *
 * The returned string must be considered constant and must NOT be freed.
 *
 * Returns: The English language name.
 **/
const char*
enca_language_english_name(const char *lang)
{
  const EncaLanguageInfo *linfo;

  linfo = find_language(lang);
  if (!linfo)
    return NULL;

  return linfo->humanname;
}

/**
 * enca_get_language_charsets:
 * @langname: Two-letter ISO-639 language code.
 * @n: The number of charsets will be stored here.
 *
 * Returns list of identifiers of charsets supported for language @language.
 *
 * The list of charset identifiers has to be freed by caller.
 *
 * Returns: The list of charsets, storing their number into *@n.  When language
 *          contains no charsets or @langname is invalid, #NULL is returned
 *          and zero stored into *@n.
 **/
int*
enca_get_language_charsets(const char *langname,
                           size_t *n)
{
  const EncaLanguageInfo *lang;

  assert(langname != NULL);

  lang = find_language(langname);
  if (lang == NULL) {
    *n = 0;
    return NULL;
  }

  *n = lang->ncharsets;
  return language_charsets_ids(lang);
}

/**
 * language_charsets_ids:
 * @lang: A language.
 *
 * Creates and fills table of charset identifiers of charsets supported for
 * language @lang.
 *
 * The size of the table is determined by @lang->ncharsets.
 *
 * Returns: The charsets id table; #NULL when @lang has no charsets.
 **/
static int*
language_charsets_ids(const EncaLanguageInfo *lang)
{
  int *charsets;
  size_t i;

  assert(lang != NULL);

  if (lang->ncharsets == 0)
    return NULL;

  charsets = NEW(int, lang->ncharsets);
  for (i = 0; i < lang->ncharsets; i++) {
    charsets[i] = enca_name_to_charset(lang->csnames[i]);
    assert(charsets[i] != ENCA_CS_UNKNOWN);
  }

  return charsets;
}

/**
 * find_language:
 * @langname: Language (i.e. locale) name.
 *
 * Finds language @langname.
 *
 * Returns: Pointer to its language information data; #NULL if not found.
 **/
static const EncaLanguageInfo*
find_language(const char *langname)
{
  const EncaLanguageInfo *lang = NULL;
  size_t i;

  if (langname == NULL)
    return NULL;

  for (i = 0; i < NLANGUAGES; i++) {
    if (strcmp(langname, LANGUAGE_LIST[i]->name) == 0) {
      lang = LANGUAGE_LIST[i];
      break;
    }
  }

  return lang;
}

/**
 * enca_get_charset_similarity_matrix:
 * @lang: A language.
 *
 * Computes character weight similarity matrix for language @lang.
 *
 * sim[i,j] is normalized to sim[i,i] thus:
 * - a row i contains ,probabilities` different languages will look like the
 *   i-th one
 * - a column i contains ,probabilities` the i-th language will look like
 *   the other languages.
 *
 * For all practical applications, the higher one of sim[i,j] and sim[j,i]
 * is important.
 *
 * Note: this is not used anywhere, only by simtable.
 *
 * Returns: The matrix, its size is determined by @lang->ncharsets; #NULL
 *          for language with no charsets.
 **/
double*
enca_get_charset_similarity_matrix(const EncaLanguageInfo *lang)
{
  const size_t n = lang->ncharsets;
  const unsigned short int *const *w = lang->weights;
  const unsigned short int *s = lang->significant;

  double *smat;
  size_t i, j, c;

  assert(lang != NULL);

  if (n == 0)
    return NULL;

  /* Below diagonal. */
  smat = NEW(double, n*n);
  for (i = 0; i < n; i++) {
    for (j = 0; j <= i; j++) {
      smat[i*n + j] = 0.0;
      for (c = 0; c < 0x100; c++)
        smat[i*n + j] += (double)w[i][c] * (double)w[j][c] / (s[c] + EPSILON);
    }
  }

  /* Above diagonal. */
  for (i = 0; i < n; i++) {
    for (j = i+1; j < n; j++)
      smat[i*n + j] = smat[j*n + i];
  }

  /* Normalize. */
  for (i = 0; i < n; i++) {
    double wmax = smat[i*n + i];

    for (j = 0; j < n; j++) {
      smat[i*n + j] /= wmax;
    }
  }

  return smat;
}
/* vim: ts=2
 */

