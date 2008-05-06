/*
  @(#) $Id: lang_hr.c,v 1.7 2005/12/01 10:08:53 yeti Exp $
  encoding data and routines dependent on language; croatian

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
#include "data/croatian/croatian.h"

/* Local prototypes. */
static int hook(EncaAnalyserState *analyser);
static int eol_hook(EncaAnalyserState *analyser);
static int hook_iso1250(EncaAnalyserState *analyser);
static int hook_isowin(EncaAnalyserState *analyser);

/**
 * ENCA_LANGUAGE_HR:
 *
 * Croatian language.
 *
 * Everything the world out there needs to know about this language.
 **/
const EncaLanguageInfo ENCA_LANGUAGE_HR = {
  "hr",
  "croatian",
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
 * Launches language specific hooks for language "hr".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook(EncaAnalyserState *analyser)
{
  return hook_iso1250(analyser);
}

/**
 * eol_hook:
 * @analyser: Analyser state whose charset ratings are to be modified.
 *
 * Launches language specific EOL hooks for language "hr".
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
 * Decides between iso8859-2 and cp1250 charsets for language "hr".
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
 * Decides between iso8859-2 and cp1250 charsets for language "hr".
 *
 * Returns: Nonzero if charset ratigns have been actually modified, zero
 * otherwise.
 **/
static int
hook_iso1250(EncaAnalyserState *analyser)
{
  static const unsigned char list_iso88592[] = {
    0xb9, 0xbe, 0xa9, 0xae
  };
  static const unsigned char list_cp1250[] = {
    0x9a, 0x9e, 0x8a, 0x8e
  };
  static EncaLanguageHookData1CS hookdata[] = {
    MAKE_HOOK_LINE(iso88592),
    MAKE_HOOK_LINE(cp1250),
  };

  return enca_language_hook_ncs(analyser, ELEMENTS(hookdata), hookdata);
}

/* vim: ts=2
 */
