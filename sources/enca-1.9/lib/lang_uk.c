/*
  @(#) $Id: lang_uk.c,v 1.10 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; ukrainian

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

#include "enca.h"
#include "internal.h"
#include "data/ukrainian/ukrainian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int eol_hook(EncaAnalyserState *analyser);
static int hook_mac1251(EncaAnalyserState *analyser);
static int hook_isokoi(EncaAnalyserState *analyser);
static int hook_ibm1125(EncaAnalyserState *analyser);
static int hook_macwin(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_UK:
 *
 * Ukrainian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_UK = {
  "uk",
  "ukrainian",
  NCHARSETS,
  CHARSET_NAMES,
  CHARSET_WEIGHTS,
  SIGNIFICANT,
  CHARSET_LETTERS,
  CHARSET_PAIRS,
  WEIGHT_SUM,
  &hook,
  &eol_hook,
  NULL,
  NULL
};

/**
 * hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific hooks for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  return hook_mac1251(analyser)
         || hook_isokoi(analyser)
         || hook_ibm1125(analyser);
}

/**
 * eol_hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific EOL hooks for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
eol_hook(EncaAnalyserState *analyser)
{
  return hook_macwin(analyser);
}

/**
 * hook_macwin:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between maccyr and cp1251 charsets for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_macwin(EncaAnalyserState *analyser)
{
  static EncaLanguageHookDataEOL hookdata[] = {
    { "maccyr", ENCA_SURFACE_EOL_CR, (size_t)-1 },
    { "cp1251", ENCA_SURFACE_MASK_EOL, (size_t)-1 },
  };

  return enca_language_hook_eol(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_mac1251:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between maccyr and cp1251 charsets for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_mac1251(EncaAnalyserState *analyser)
{
  static const unsigned char list_maccyr[] = {
    0xb4, 0xdf, 0xbb, 0x82, 0x8f
  };
  static const unsigned char list_cp1251[] = {
    0xb3, 0xff, 0xbf, 0xc2, 0xcf
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(maccyr),
    MAKE_HOOK_LINE(cp1251),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_isokoi:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-5 and koi8u charsets for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_isokoi(EncaAnalyserState *analyser)
{
  static const unsigned char list_iso88595[] = {
    0xdd, 0xe2, 0xf6, 0xe0, 0xde, 0xe1, 0xe3, 0xdc, 0xdf, 0xef, 0xec
  };
  static const unsigned char list_koi8u[] = {
    0xcf, 0xc1, 0xce, 0xc9, 0xa6, 0xc5, 0xcb, 0xcc, 0xc4, 0xcd, 0xc7
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(iso88595),
    MAKE_HOOK_LINE(koi8u),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_ibm1125:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between cp1125 and ibm855 charsets for language "uk".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_ibm1125(EncaAnalyserState *analyser)
{
  static const unsigned char list_cp1125[] = {
    0xae, 0xad, 0xe2, 0xf7, 0xe0, 0xa5, 0xab, 0xaf, 0xef, 0xaa, 0xa7
  };
  static const unsigned char list_ibm855[] = {
    0xd6, 0xd4, 0xb7, 0x8a, 0xeb, 0xc6, 0xd0, 0xd2, 0xd8, 0xde, 0xf3
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(cp1125),
    MAKE_HOOK_LINE(ibm855),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */

