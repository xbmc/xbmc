/*
  @(#) $Id: lang_ru.c,v 1.11 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; russian

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

#include "enca.h"
#include "internal.h"
#include "data/russian/russian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int eol_hook(EncaAnalyserState *analyser);
static int hook_mac1251(EncaAnalyserState *analyser);
static int hook_macwin(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_RU:
 *
 * Russian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_RU = {
  "ru",
  "russian",
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
 * Launches language specific hooks for language "ru".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  return hook_mac1251(analyser);
}

/**
 * eol_hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific EOL hooks for language "ru".
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
 * Decides between maccyr and cp1251 charsets for language "ru".
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
 * Decides between maccyr and cp1251 charsets for language "ru".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_mac1251(EncaAnalyserState *analyser)
{
  static const unsigned char list_maccyr[] = {
    0xdf, 0x82, 0x8f, 0x8d, 0x80, 0x91, 0x88
  };
  static const unsigned char list_cp1251[] = {
    0xff, 0xc2, 0xcf, 0xcd, 0xc0, 0xd1, 0xc8
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(maccyr),
    MAKE_HOOK_LINE(cp1251),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */
