/*
  @(#) $Id: lang_hu.c,v 1.13 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; hungarian

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
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "enca.h"
#include "internal.h"
#include "data/hungarian/hungarian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int hook_iso1250(EncaAnalyserState *analyser);
static int hook_isocork(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_HU:
 *
 * Hungarian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_HU = {
  "hu",
  "hungarian",
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
  NULL
};

/**
 * hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific hooks for language "hu".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  int chg = 0;

  /* we may want to run both, and in this order */
  chg += hook_isocork(analyser);
  chg += hook_iso1250(analyser);
  return chg;
}

/**
 * hook_iso1250:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-2 and cp1250 charsets for language "hu".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_iso1250(EncaAnalyserState *analyser)
{
  static EncaLanguageHookDataEOL hookdata[] = {
    { "cp1250", ENCA_SURFACE_EOL_CRLF, (size_t)-1 },
    { "iso88592", ENCA_SURFACE_MASK_EOL, (size_t)-1 },
  };

  return enca_language_hook_eol(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_isocork:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-2, cp1250 and cork charsets for language "hu".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_isocork(EncaAnalyserState *analyser)
{
  static const unsigned char list_iso88592[] = {
    0xf5, 0xfb, 0xd5
  };
  static const unsigned char list_cp1250[] = {
    0xf5, 0xfb, 0xd5
  };
  static const unsigned char list_cork[] = {
    0xae, 0xb6, 0x8e
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(iso88592),
    MAKE_HOOK_LINE(cp1250),
    MAKE_HOOK_LINE(cork),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */
