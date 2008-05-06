/*
  @(#) $Id: lang_pl.c,v 1.11 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; polish

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
#include "data/polish/polish.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int eol_hook(EncaAnalyserState *analyser);
static int hook_iso1250(EncaAnalyserState *analyser);
static int hook_balt13(EncaAnalyserState *analyser);
static int hook_isowin(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_PL:
 *
 * Polish language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_PL = {
  "pl",
  "polish",
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
 * Launches language specific hooks for language "pl".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  return hook_iso1250(analyser)
         || hook_balt13(analyser);
}

/**
 * eol_hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific EOL hooks for language "pl".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
eol_hook(EncaAnalyserState *analyser)
{
  return hook_isowin(analyser);
}

/**
 * hook_isowin:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-2 and cp1250 charsets for language "pl".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_isowin(EncaAnalyserState *analyser)
{
  static EncaLanguageHookDataEOL hookdata[] = {
    { "cp1250", ENCA_SURFACE_EOL_CRLF, (size_t)-1 },
    { "iso88592", ENCA_SURFACE_MASK_EOL, (size_t)-1 },
  };

  return enca_language_hook_eol(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_iso1250:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between iso8859-2 and cp1250 charsets for language "pl".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_iso1250(EncaAnalyserState *analyser)
{
  static const unsigned char list_iso88592[] = {
    0xb1, 0xb6, 0xbc, 0xa6
  };
  static const unsigned char list_cp1250[] = {
    0xb9, 0x9c, 0x9f, 0x8,
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(iso88592),
    MAKE_HOOK_LINE(cp1250),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/**
 * hook_balt13:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Decides between baltic and iso8859-13 charsets for language "pl".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_balt13(EncaAnalyserState *analyser)
{
  static const unsigned char list_baltic[] = {
    0xf0, 0xeb, 0xf2, 0xfe
  };
  static const unsigned char list_iso885913[] = {
    0xf9, 0xe0, 0xf1, 0xea
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(baltic),
    MAKE_HOOK_LINE(iso885913),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */
