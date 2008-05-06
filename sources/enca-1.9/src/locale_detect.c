/*
  @(#) $Id: locale_detect.c,v 1.24 2004/07/20 20:15:09 yeti Exp $
  try to guess user's native language from locale

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
#include "common.h"

#ifdef HAVE_SETLOCALE
#  ifdef HAVE_LOCALE_H
#    include <locale.h>
#  else /* HAVE_LOCALE_H */
char* setlocale(int category, const char *locale);
#  endif /* HAVE_LOCALE_H */
#endif /* HAVE_SETLOCALE */

#ifdef HAVE_NL_LANGINFO
#  ifdef HAVE_LANGINFO_H
#    include <langinfo.h>
#  else /* HAVE_LANGINFO_H */
char *nl_langinfo(nl_item *item);
#  endif /* HAVE_LANGINFO_H */
#endif /* HAVE_NL_LANGINFO */

static char *codeset = NULL;

/* Local prototypes. */
static char* locale_alias_convert(const char *locname);
static char* strip_locale_name(const char *locname);
static char* static_iso639_alias_convert(const char *locname);
#ifdef HAVE_SETLOCALE
static char* detect_target_charset(const char *locname);
static char* detect_user_language(void);
#endif /* HAVE_SETLOCALE */
static void  codeset_free(void);

/*
 * when lang is not NULL converts it to two-character language code
 * othwerise, tries to guess what language user wants from locale settings.
 * returns string of length 2 containig language code (to be freed by caller)
 * or NULL if not detected or unable to convert.
 */
char*
detect_lang(const char *lang)
{
  char *locname, *result, *cvt;

  atexit(codeset_free);
#ifdef HAVE_SETLOCALE
  /* No lang, detect locale, then CODESET, then try to transform it */
  if (!lang) {
    locname = detect_user_language();
    /* HERE: locname is (a) newly allocated (b) NULL */
    codeset = detect_target_charset(locname);
    /* HERE: codeset is (a) newly allocated, different from locname (b) NULL */
    cvt = locale_alias_convert(locname);
    result = strip_locale_name(cvt);
    enca_free(cvt);
    enca_free(locname);
    return result;
  }

  /* We have lang, try it first untransformed, then transformed for CODESET */
  codeset = detect_target_charset(lang);
  locname = locale_alias_convert(lang);
  if (!codeset)
    codeset = detect_target_charset(locname);
  result = strip_locale_name(locname);
  enca_free(locname);
  return result;

#else /* HAVE_SETLOCALE */
  cvt = locale_alias_convert(lang);
  result = strip_locale_name(cvt);
  enca_free(cvt);
  return result;
#endif /* HAVE_SETLOCALE */
}

#ifdef HAVE_SETLOCALE
/**
 * detect_target_charset:
 *
 * Detect target charset from LC_CTYPE for user's language.
 *
 * Returns: A string (to be freed) with charset name or NULL on failure.
 **/
static char*
detect_target_charset(const char *locname)
{
  char *s = NULL;

#ifdef HAVE_NL_LANGINFO
  if (!locname)
    return NULL;

  if ((s = setlocale(LC_CTYPE, locname)) == NULL)
    return NULL;

  s = enca_strdup(nl_langinfo(CODESET));

  if (setlocale(LC_CTYPE, "C") == NULL) {
    fprintf(stderr, "%s: Cannot set LC_CTYPE to the portable \"C\" locale\n",
                    program_name);
    exit(EXIT_TROUBLE);
  }
  if (options.verbosity_level > 2)
    fprintf(stderr, "Detected locale native charset: %s\n", s);
#endif /* HAVE_NL_LANGINFO */

  return s;
}

/**
 * detect_user_language:
 *
 * Detect user's locale by querying several LC categories.
 *
 * NB: this is conceptually wrong, the string returned by setlocale should
 * be taken as opaque -- but then we would be in deep shit^Wtrouble.  This
 * seems to actually happen on Win32.
 *
 * Returns: A string (to be freed) with locale name or NULL on failure.
 **/
static char*
detect_user_language(void)
{
  static const int test_categories[] = {
    LC_CTYPE, LC_COLLATE,
#if HAVE_LC_MESSAGES
    LC_MESSAGES,
#endif
  };
  char *s = NULL;
  size_t i;

  for (i = 0; i < ELEMENTS(test_categories); i++) {
    enca_free(s);
    if ((s = setlocale(test_categories[i], "")) == NULL)
      continue;
    s = enca_strdup(s);
    if (setlocale(test_categories[i], "C") == NULL) {
      fprintf(stderr, "%s: Cannot set locale to the portable \"C\" locale\n",
                      program_name);
      exit(EXIT_TROUBLE);
    }

    if (strcmp(s, "") == 0
        || strcmp(s, "C") == 0
        || strcmp(s, "POSIX") == 0
        || (strncmp(s, "en", 2) == 0 && !isalpha(s[2])))
      continue;

    if (options.verbosity_level > 2)
      fprintf(stderr, "Locale inherited from environment: %s\n", s);

    return s;
  }

  return NULL;
}
#endif /* HAVE_SETLOCALE */

/* convert locale alias to canonical name using LOCALE_ALIAS_FILE (presumably
   /usr/share/locale/locale.alias) and return it

   Returned string should be freed by caller.

   FIXME: this function can get easily confused by lines longer than BUFSIZE
   (but the worst thing that can happen is we return wrong locale name)
   the locale.alias format is nowhere described, so we assume every line
   consists of alias (row 1), some whitespace and canonical name */
static char*
locale_alias_convert(const char *locname)
{
#ifdef HAVE_LOCALE_ALIAS
  File *fla; /* locale.alias file */
  Buffer *buf;
  char *s,*p,*q;
  size_t n;
#endif /* HAVE_LOCALE_ALIAS */

  if (!locname)
    return NULL;

  /* Catch the special language name `none' */
  if (strcmp(locname, "none") == 0)
    return enca_strdup("__");

#ifdef HAVE_LOCALE_ALIAS
  /* try to read locale.alias */
  buf = buffer_new(0);
  fla = file_new(LOCALE_ALIAS_PATH, buf);
  if (file_open(fla, "r") != 0) {
    if (options.verbosity_level) {
      fprintf(stderr, "Cannot find locale.alias file.\n"
                      "This build of enca probably has been configured for "
                      "quite a different system\n");
    }
    file_free(fla);
    buffer_free(buf);
    return enca_strdup(locname);
  }

  /* scan locale.alias
     somewhat crude now */
  n = strlen(locname);
  p = NULL;
  s = (char*)buf->data; /* alias */
  while (file_getline(fla) != NULL) {
    if (strncmp(s, locname, n) == 0 &&
        (isspace(s[n]) || (s[n] == ':' && isspace(s[n+1])))) {
      p = s + n;
      /* skip any amount of whitespace */
      while (isspace(*p)) p++;
      q = p;
      /* anything up to next whitespace is the canonical locale name */
      while (*q != '\0' && !isspace(*q)) q++;
      *q = '\0';
      p = enca_strdup(p);
      break;
    }
  }
  file_close(fla);
  file_free(fla);

  buffer_free(buf);
  return p != NULL ? p : static_iso639_alias_convert(locname);
#else /* HAVE_LOCALE_ALIAS */
  return static_iso639_alias_convert(locname);
#endif /* HAVE_LOCALE_ALIAS */
}

/**
 * get_lang_codeset:
 *
 * Returns locale native charset.
 *
 * MUST be called after detect_lang.
 *
 * Returns: the codeset name.
 **/
const char*
get_lang_codeset(void)
{
  if (!codeset)
    codeset = enca_strdup("");

  return codeset;
}

/**
 * Returns `language' component of locale name locname (if successfully
 * parsed), NULL otherwise
 *
 * Returned string should be freed by caller.
 **/
static char*
strip_locale_name(const char *locname)
{
  /* Some supported languages can also appear as dialects of some other
   * language */
  struct {
    const char *dialect;
    const char *iso639;
  }
  const DIALECTS[] = {
    { "cs_SK", "sk" },
    { "ru_UA", "uk" },
  };

  size_t n;
  char *s;

  if (!locname)
    return NULL;

  s = enca_strdup(locname);
  n = strlen(s);
  /* Just language: en, de, fr, cs, sk, ru, etc. */
  if (n == 2)
    return s;

  /* Some long specification (either X/Open or CEN). */
  if (n >= 5 && s[2] == '_'
      && (s[5] == '\0' || s[5] == '.' || s[5] == '+')) {
    size_t i;

    /* Convert dialects. */
    for (i = 0; i < ELEMENTS(DIALECTS); i++) {
      if (strncmp(DIALECTS[i].dialect, s, 5) == 0) {
        s[0] = DIALECTS[i].iso639[0];
        s[1] = DIALECTS[i].iso639[1];
        break;
      }
    }

    s[2] = '\0';
  }
  else {
    /* Just garbage or some unresolved locale alias. */
    enca_free(s);
  }

  return s;
}

/**
 * Tries to translate language names statically.
 *
 * Also catches some bad and alternative spellings.
 *
 * Returned string should be freed by caller.
 **/
static char*
static_iso639_alias_convert(const char *locname)
{
  struct {
    const char *alias;
    const char *iso639;
  }
  const ALIASES[] = {
    { "byelarussian", "be" },
    { "belarussian", "be" },
    { "byelorussian", "be" },
    { "belorussian", "be" },
    { "bulgarian", "bg" },
    { "croatian", "hr" },
    { "czech", "cs" },
    { "estonian", "et" },
    { "hungarian", "hu" },
    { "magyar", "hu" },
    { "lativan", "lt" },
    { "lettic", "lv" },
    { "lettish", "lv" },
    { "lithuanian", "lt" },
    { "polish", "pl" },
    { "russian", "ru" },
    { "slovak", "sk" },
    { "slovene", "sl" },
    { "ukrainian", "uk" }
  };

  size_t i;

  if (!locname)
    return NULL;

  for (i = 0; i < ELEMENTS(ALIASES); i++) {
    if (strcmp(ALIASES[i].alias, locname) == 0) {
      if (options.verbosity_level > 2)
        fprintf(stderr, "Decrypted locale alias using built-in table: %s\n",
                        ALIASES[i].iso639);

      return enca_strdup(ALIASES[i].iso639);
    }
  }

  return enca_strdup(locname);
}

static void
codeset_free(void)
{
  enca_free(codeset);
}

/* vim: ts=2
 */
