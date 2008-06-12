/*
  @(#) $Id: enca.c,v 1.9 2003/12/22 22:24:33 yeti Exp $
  encoding-guessing libary; the high-level analyser interface

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

/**
 * enca_analyser_alloc:
 * @langname: Language for which the analyser should be initialized.
 *
 * Allocates an analyser and initializes it for language @language.
 *
 * The analyser, once crerated, can be used only for language for which it
 * was initialized.  If you need to detect encodings of texts in more than one
 * language, you must allocate an analyser for each one.  Note however, an
 * analyser may occupy a considerable amount of memory (a few hundreds of kB),
 * so it's generally not a good idea to have several hundreds of them floating
 * around.
 *
 * @langname is two-letter ISO 639:1989 language code.  Locale names in form
 * language_territory and ISO-639 English language names also may be accepted
 * in the future. To be on the safe side, use only names returned by
 * enca_get_languages().
 *
 * Returns: The newly created #EncaAnalyser on success, #NULL on failure
 *          (namely when @langname is unknown or otherwise invalid).
 **/
EncaAnalyser
enca_analyser_alloc(const char *langname)
{
  EncaAnalyserState *analyser;

  if (langname == NULL)
    return NULL;

  analyser = NEW(EncaAnalyserState, 1);
  if (!enca_language_init(analyser, langname)) {
    enca_free(analyser);
    return NULL;
  }

  enca_guess_init(analyser);
  enca_double_utf8_init(analyser);
  enca_pair_init(analyser);

  return analyser;
}

/**
 * enca_analyser_free:
 * @analyser: An analyser to be destroyed.
 *
 * Frees memory used by #EncaAnalyser @analyser.
 **/
void
enca_analyser_free(EncaAnalyser analyser)
{
  assert(analyser != NULL);

  enca_pair_destroy(analyser);
  enca_double_utf8_destroy(analyser);
  enca_guess_destroy(analyser);
  enca_language_destroy(analyser);
  enca_free(analyser);
}

/**
 * enca_errno:
 * @analyser: An analyser.
 *
 * Returns analyser error code.
 *
 * The error code is not modified.  However, any other analyser call i.e.
 * call to a function taking @analyser as parameter can change the error code.
 *
 * Returns: Error code of reason why last analyser call failed.
 **/
int
enca_errno(EncaAnalyser analyser)
{
  assert(analyser != NULL);

  return analyser->gerrno;
}

/**
 * enca_strerror:
 * @analyser: An analyser.
 * @errnum: An analyser error code.
 *
 * Returns string describing the error code.
 *
 * The returned string must be considered constant and must NOT be freed.
 * It is however gauranteed not to be modified on invalidated by subsequent
 * calls to any libenca functions, including enca_strerror().
 *
 * The analyser error code is not changed for a successful call, and it set
 * to #ENCA_EINVALUE upon error.
 *
 * Returns: String describing the error code.
 **/
const char*
enca_strerror(EncaAnalyser analyser,
              int errnum)
{
  static const char *const DESCRIPTION_LIST[] = {
    "OK",
    "Invalid value",
    "Sample is empty",
    "After filtering, (almost) nothing remained",
    "Multibyte tests failed, language contains no 8bit charsets",
    "Not enough significant characters",
    "No clear winner",
    "Sample is just garbage"
  };

  if ((size_t)errnum >= ELEMENTS(DESCRIPTION_LIST)) {
    analyser->gerrno = ENCA_EINVALUE;
    return "Unknown error! (FIXME!)";
  }

  return DESCRIPTION_LIST[errnum];
}

/***** Documentation *********************************************************/

/**
 * EncaErrno:
 * @ENCA_EOK: OK.
 * @ENCA_EINVALUE: Invalid value (usually of an option).
 * @ENCA_EEMPTY: Sample is empty.
 * @ENCA_EFILTERED: After filtering, (almost) nothing remained.
 * @ENCA_ENOCS8: Mulitibyte tests failed and language contains no 8bit charsets.
 * @ENCA_ESIGNIF: Too few significant characters.
 * @ENCA_EWINNER: No clear winner.
 * @ENCA_EGARBAGE: Sample is garbage.
 *
 * Error codes.
 **/

/* vim: ts=2 sw=2 et
 */

