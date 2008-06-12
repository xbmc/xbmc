/*
  @(#) $Id: lang_lv.c,v 1.13 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; latvian

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
#include "data/latvian/latvian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int hook_iso13win(EncaAnalyserState *analyser);
static int hook_baltwin(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_LV:
 *
 * Latvian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_LV = {
  "lv",
  "latvian",
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
 * Launches language specific hooks for language "lv".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  int chg = 0;

  /* we may want to run both, and in this order */
  chg += hook_baltwin(analyser);
  chg += hook_iso13win(analyser);
  return chg;
}

/**
 * hook_baltwin:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between cp1257, iso8859-13 and baltic charsets for language "lv".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_baltwin(EncaAnalyserState *analyser)
{
  static const unsigned char list_cp1257[] = {
    0xe7, 0xf0, 0xf2, 0xfe, 0xd0, 0xde
  };
  static const unsigned char list_iso885913[] = {
    0xe7, 0xf0, 0xf2, 0xfe, 0xd0, 0xde
  };
  static const unsigned char list_baltic[] = {
    0xec, 0xf9, 0xf1, 0xea, 0xd9, 0xcc
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(cp1257),
    MAKE_HOOK_LINE(iso885913),
    MAKE_HOOK_LINE(baltic),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_iso13win:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-13 and cp1257 charsets for language "lv".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_iso13win(EncaAnalyserState *analyser)
{
  static EncaLanguageHookDataEOL hookdata[] = {
    { "iso885913", ENCA_SURFACE_EOL_LF, (size_t)-1 },
    { "cp1257", ENCA_SURFACE_MASK_EOL, (size_t)-1 },
  };

  return enca_language_hook_eol(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */
