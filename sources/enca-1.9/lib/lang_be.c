/*
  @(#) $Id: lang_be.c,v 1.10 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; belarussian

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
#include "data/belarussian/belarussian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int hook_macwin(EncaAnalyserState *analyser);
static int hook_isokoi(EncaAnalyserState *analyser);
static int hook_855866(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_BE:
 *
 * Belarussian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_BE = {
  "be",
  "belarussian",
  NCHARSETS,
  CHARSET_NAMES,
  CHARSET_WEIGHTS,
  SIGNIFICANT,
  CHARSET_LETTERS,
  CHARSET_PAIRS,
  WEIGHT_SUM,
  &hook,
  NULL,
  NULL,
  NULL,
};

/**
 * hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific hooks for language "be".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  return hook_macwin(analyser)
         || hook_isokoi(analyser)
         || hook_855866(analyser);
}

/**
 * hook_macwin:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between maccyr and cp1251 charsets for language "be".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_macwin(EncaAnalyserState *analyser)
{
  static const unsigned char list_maccyr[] = {
    0xb4, 0xdf, 0xd9, 0xde, 0x80, 0x8d, 0x91, 0x8f, 0x81
  };
  static const unsigned char list_cp1251[] = {
    0xb3, 0xff, 0xa2, 0xb8, 0xc0, 0xcd, 0xd1, 0xcf, 0xc1
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
 * Decides between iso8859-5 and koi8u charsets for language "be".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_isokoi(EncaAnalyserState *analyser)
{
  static const unsigned char list_iso88595[] = {
    0xdd, 0xd0, 0xe0, 0xf6, 0xeb, 0xef, 0xe3, 0xe2, 0xe1, 0xdf
  };
  static const unsigned char list_koi8u[] = {
    0xc1, 0xce, 0xc5, 0xcc, 0xcb, 0xcf, 0xa6, 0xc4, 0xcd, 0xae
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(iso88595),
    MAKE_HOOK_LINE(koi8u),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_855866:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between cp866 and ibm855 charsets for language "be".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_855866(EncaAnalyserState *analyser)
{
  static const unsigned char list_ibm855[] = {
    0xd4, 0xd0, 0xd6, 0x8a, 0xc6, 0xde, 0xd2, 0xf3, 0xd8, 0x98, 0xf1
  };
  static const unsigned char list_ibm866[] = {
    0xad, 0xe0, 0xa5, 0xab, 0xae, 0xaa, 0xe2, 0xef, 0xa7, 0xaf, 0xe6
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(ibm855),
    MAKE_HOOK_LINE(ibm866),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */

