/*
  @(#) $Id: options.c,v 1.31 2005/12/18 12:07:46 yeti Exp $
  command line option processing

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

#ifdef HAVE_GETOPT_H
# include <getopt.h>
#else /* HAVE_GETOPT_H */
# include "getopt.h"
#endif /* HAVE_GETOPT_H */

#ifdef HAVE_WORDEXP
# ifdef HAVE_WORDEXP_H
#  include <wordexp.h>
# else /* HAVE_WORDEXP_H */
/* don't declare all the stuff from wordexp.h when not present, it would
   probably not work anyway---just don't use it */
#  undef HAVE_WORDEXP
# endif /* HAVE_WORDEXP_H */
#endif /* HAVE_WORDEXP */

typedef void (* ReportFunc)(void);

/* Program behaviour: enca or enconv. */
typedef enum {
  BEHAVE_ENCA,
  BEHAVE_ENCONV
} ProgramBehaviour;

/* Settings. */
char *program_name = NULL;
static ProgramBehaviour behaviour = BEHAVE_ENCA;
Options options;

/* Environment variable containing default options. */
static const char *ENCA_ENV_VAR = "ENCAOPT";

/* Environment variable containing default target charset (think recode). */
static const char *RECODE_CHARSET_VAR = "DEFAULT_CHARSET";

/* Default option values. */
static const Options DEFAULTS = {
  0, /* verbosity_level */
  NULL, /* language */
  OTYPE_HUMAN, /* output_type */
  { ENCA_CS_UNKNOWN, 0 }, /* target_enc */
  NULL, /* target_enc_str */
  -1, /* prefix_filename */
};

extern const char *const COPYING_text[];
extern const char *const    HELP_text[];

/* Version/copyright text. */
static const char *VERSION_TEXT = /* {{{ */
"Features: "
#ifdef HAVE_LIBRECODE
"+"
#else /* HAVE_LIBRECODE */
"-"
#endif /* HAVE_LIBRECODE */
"librecode-interface "

#ifdef HAVE_GOOD_ICONV
"+"
#else /* HAVE_GOOD_ICONV */
"-"
#endif /* HAVE_GOOD_ICONV */
"iconv-interface "

#ifdef ENABLE_EXTERNAL
"+"
#else /* ENABLE_EXTERNAL */
"-"
#endif /* ENABLE_EXTERNAL */
"external-converter "

#ifdef HAVE_SETLOCALE
"+"
#else /* HAVE_SETLOCALE */
"-"
#endif /* HAVE_SETLOCALE */
"language-detection "

#ifdef HAVE_LOCALE_ALIAS
"+"
#else /* HAVE_LOCALE_ALIAS */
"-"
#endif /* HAVE_LOCALE_ALIAS */
"locale-alias "

#ifdef HAVE_NL_LANGINFO
"+"
#else /* HAVE_NL_LANGINFO */
"-"
#endif /* HAVE_NL_LANGINFO */
"target-charset-auto "

#ifdef HAVE_WORDEXP
"+"
#else /* HAVE_WORDEXP */
"-"
#endif /* HAVE_WORDEXP */
"ENCAOPT "

"\n\n"
"Copyright (C) 2000-2005 David Necas (Yeti) (<yeti@physics.muni.cz>),\n"
"              2005 Zuxy Meng (<zuxy.meng@gmail.com>).\n"
"\n"
PACKAGE_NAME
" is free software; it can be copied and/or modified under the terms of\n"
"version 2 of GNU General Public License, run `enca --license' to see the full\n"
"license text.  There is NO WARRANTY; not even for MERCHANTABILITY or FITNESS\n"
"FOR A PARTICULAR PURPOSE.";
/* }}} */

/* Local prototypes. */
static char**     interpret_opt          (int argc,
                                          char *argv[],
                                          int cmdl_argc);
static int        prepend_env            (int argc,
                                          char *argv[],
                                          int *newargc,
                                          char *(*newargv[]));
static OutputType optchar_to_otype       (const char c);
static void       set_otype_from_name    (const char *otname);
static void       set_program_behaviour  (void);
static int        parse_arg_x            (const char *s);
static int        add_parsed_converters  (const char *list);
static void       print_some_list        (const char *listname);
static char**     make_filelist          (const int n,
                                          char *argvrest[]);
static int        prefix_filename        (int pfx);
#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
# define program_invocation_short_name strip_path(argv[0])
static char*      strip_path             (const char *fullpath);
#endif /* not HAVE_PROGRAM_INVOCATION_SHORT_NAME */
static void       print_version          (void);
static void       print_all_charsets     (void);
static void       print_builtin_charsets (void);
static void       print_surfaces         (void);
static void       print_languages        (void);
static void       print_lists            (void);
static void       print_names            (void);
static void       print_charsets         (int only_builtin);
static void       print_text_and_exit    (const char *const *text,
                                          int exitcode);

/* merge all sources of options (ENCAOPT and command line arguments) and
   process them
   returns list of file to process (or NULL for stdin) */
char**
process_opt(const int argc, char *argv[])
{
  int newargc;
  char **newargv;
  char **flist;

  /* Assign defaults. */
  options = DEFAULTS;

  program_name = program_invocation_short_name;
  set_program_behaviour();

#ifdef ENABLE_EXTERNAL
  set_external_converter(DEFAULT_EXTERNAL_CONVERTER);
#endif /* ENABLE_EXTERNAL */

  /* Prepend options in $ENCAOPT. */
  prepend_env(argc, argv, &newargc, &newargv);

  /* Interpret them. */
  flist = interpret_opt(newargc, newargv, argc);

  /* prefix result with file name iff we are about to process stdin or the
     file list contains only one file and we don't print result */
  if (prefix_filename(-1) == -1) {
    if ((flist == NULL || flist[1] == NULL)
        && options.output_type != OTYPE_DETAILS)
      prefix_filename(0);
    else
      prefix_filename(1);
  }

  return flist;
}

/* process options, return file list (i.e. all remaining arguments)

   FIXME: this function is infinitely ugly */
static char**
interpret_opt(int argc, char *argv[], int cmdl_argc)
{
  /* Short command line options. */
  static const char *short_options =
    "cC:deE:fgGhil:L:mn:pPrsvVx:";

  /* Long `GNU style' command line options {{{. */
  static const struct option long_options[] = {
    { "auto-convert", no_argument, NULL, 'c' },
    { "convert-to", required_argument, NULL, 'x' },
    { "cstocs-name", no_argument, NULL, 's' },
    { "details", no_argument, NULL, 'd' },
    { "enca-name", no_argument, NULL, 'e' },
    { "external-converter-program", required_argument, NULL, 'E' },
    { "guess", no_argument, NULL, 'g' },
    { "help", no_argument, NULL, 'h' },
    { "human-readable", no_argument, NULL, 'f' },
    { "iconv-name", no_argument, NULL, 'i' },
    { "language", required_argument, NULL, 'L' },
    { "license", no_argument, NULL, 'G' },
    { "list", required_argument, NULL, 'l' },
    { "mime-name", no_argument, NULL, 'm' },
    { "name", required_argument, NULL, 'n' },
    { "no-filename", no_argument, NULL, 'P' },
    { "rfc1345-name", no_argument, NULL, 'r' },
    { "try-converters", required_argument, NULL, 'C' },
    { "verbose", no_argument, NULL, 'V' },
    { "version", no_argument, NULL, 'v' },
    { "with-filename", no_argument, NULL, 'p' },
    { NULL, 0, NULL, 0 }
  };
  /* }}} */

  int c;
  char **filelist;
  int otype_set = 0; /* Whether output type was explicitely set. */

  /* Process options. */
  opterr = 0; /* Getopt() shouldn't print errors, we do it ourself. */
  while ((c = getopt_long(argc, argv, short_options,
                          long_options, NULL)) != -1) {
    switch (c) {
      case '?': /* Unknown option. */
      fprintf(stderr, "%s: Unknown option -%c%s.\n"
                      "Run `%s --help' to get brief help.\n",
                      program_name, optopt,
                      optopt == '\0' ? " or misspelt/ambiguous long option"
                                     : "",
                      program_name);
      exit(EXIT_TROUBLE);
      break;

      case ':': /* Missing paramter. */
      fprintf(stderr, "%s: Option -%c requires an argument.\n"
                      "Run `%s --help' to get brief help.\n",
                      program_name, optopt, program_name);
      exit(EXIT_TROUBLE);
      break;

      case 'h': /* Help (and exit). */
      print_text_and_exit(HELP_text, EXIT_SUCCESS);
      break;

      case 'v': /* Version (and exit). */
      print_version();
      exit(EXIT_SUCCESS);
      break;

      case 'G': /* License (and exit). */
      print_text_and_exit(COPYING_text, EXIT_SUCCESS);
      break;

      case 'l': /* Print required list (and exit). */
      print_some_list(optarg);
      exit(EXIT_SUCCESS);
      break;

      case 'd': /* Detailed output. */
      case 'e': /* Canonical name. */
      case 'f': /* Full (descriptive) output. */
      case 'i': /* Iconv name. */
      case 'm': /* MIME name. */
      case 'r': /* RFC 1345 name as output. */
      case 's': /* Cstocs name as output. */
      options.output_type = optchar_to_otype(c);
      otype_set = 1;
      break;

      case 'n': /* Output type by name. */
      set_otype_from_name(optarg);
      otype_set = 1;
      break;

      case 'p': /* Prefix filename on. */
      case 'P': /* Prefix filename off. */
      prefix_filename(islower(c));
      break;

      case 'g': /* Behave enca. */
      behaviour = BEHAVE_ENCA;
      break;

      case 'c': /* Behave enconv. */
      behaviour = BEHAVE_ENCONV;
      break;

      case 'V': /* Increase verbosity level. */
      options.verbosity_level++;
      break;

      case 'x': /* Convert to. */
      options.output_type = OTYPE_CONVERT;
      parse_arg_x(optarg);
      otype_set = 1;
      break;

      case 'L': /* Language. */
      options.language = optarg;
      break;

      case 'C': /* Add converters to converter list. */
      add_parsed_converters(optarg);
      break;

      case 'E': /* Converter name. */
#ifdef ENABLE_EXTERNAL
      set_external_converter(optarg);
#else /* ENABLE_EXTERNAL */
      fprintf(stderr, "%s: Cannot set external converter.\n"
                      "Enca was built without support "
                      "for external converters.\n",
                      program_name);
#endif /* ENABLE_EXTERNAL */
      break;

      default:
      abort();
      break;
    }
  }

  /* Set and initialize language. */
  options.language = detect_lang(options.language);
  if (options.language == NULL) {
    fprintf(stderr, "%s: Cannot determine (or understand) "
                    "your language preferences.\n"
                    "Please use `-L language', or `-L none' if your language is not supported\n"
                    "(only a few multibyte encodings can be recognized then).\n"
                    "Run `%s --list languages' to get a list of supported languages.\n",
                    program_name, program_name);
    exit(EXIT_TROUBLE);
  }

  /* Behaviour. */
  /* With an explicit output type doesn't matter how we were called. */
  if (otype_set) {
    behaviour = BEHAVE_ENCA;
    if (options.output_type == OTYPE_CONVERT
        && options.verbosity_level > 2)
      fprintf(stderr, "Explicitly specified target charset: %s\n",
                      options.target_enc_str);
  }

  switch (behaviour) {
    case BEHAVE_ENCA:
    /* Nothing special here. */
    break;

    case BEHAVE_ENCONV:
    {
      const char *charset;

      /* Try recode's default target charset. */
      charset = getenv(RECODE_CHARSET_VAR);
      if (charset != NULL) {
        if (options.verbosity_level > 2)
          fprintf(stderr, "Inherited recode's %s target charset: %s\n",
                          RECODE_CHARSET_VAR, charset);
      }
      else {
        /* Then locale native charset. */
        charset = get_lang_codeset();
        assert(charset != NULL);
      }

      parse_arg_x(charset);
    }
    if (options.target_enc_str[0] == '\0') {
      fprintf(stderr, "%s: Cannot detect native charset for locale %s.\n"
                      "You have to use the `-x' option "
                      "or the %s environment variable "
                      "to set the target encoding manually.\n",
                      program_name,
                      options.language,
                      RECODE_CHARSET_VAR);
      exit(EXIT_TROUBLE);
    }
    options.output_type = OTYPE_CONVERT;
    break;

    default:
    abort();
    break;
  }

  /* Set up default list of converters. */
  if (add_parsed_converters(NULL) == 0)
    add_parsed_converters(DEFAULT_CONVERTER_LIST);

  /* Create file list from remaining options. */
  filelist = make_filelist(argc-optind, argv+optind);
  /* When run without any arguments and input is a tty, print help. */
  if (filelist == NULL && enca_isatty(STDIN_FILENO) && cmdl_argc == 1)
    print_text_and_exit(HELP_text, EXIT_SUCCESS);

#ifdef ENABLE_EXTERNAL
  if (options.output_type == OTYPE_CONVERT
      && external_converter_listed()
      && !check_external_converter())
    exit(EXIT_TROUBLE);
#endif

  return filelist;
}

/* prepend parsed contents of environment variable containing default options
   (ENCAOPT) before command line arguments (but after argv[0]) and return the
   new list of arguments in newargv (its length is newargc) */
static int
prepend_env(int argc,
            char *argv[],
            int *newargc,
            char *(*newargv[]))
#ifdef HAVE_WORDEXP
{
  char *msg;
  char *encaenv;
  wordexp_t encaenv_parsed;
  size_t i;

  *newargc = argc;
  *newargv = argv;
  /* Fetch value of ENCA_ENV_VAR, if set. */
  encaenv = getenv(ENCA_ENV_VAR);
  if (encaenv == NULL)
    return 0;

  /* Parse encaenv. */
  if ((i = wordexp(encaenv, &encaenv_parsed, WRDE_NOCMD)) != 0) {
    switch (i) {
      case WRDE_NOSPACE:
      wordfree(&encaenv_parsed);
      fprintf(stderr, "%s: Cannot allocate memory.\n",
                      program_name);
      exit(EXIT_TROUBLE);
      break;

      case WRDE_BADCHAR:
      msg = "invalid characters";
      break;

      case WRDE_CMDSUB:
      msg = "command substitution is disabled";
      break;

      case WRDE_SYNTAX:
      msg = "syntax error";
      break;

      default:
      msg = NULL;
      break;
    }
    fprintf(stderr, "%s: Cannot parse value of %s (",
                    program_name, ENCA_ENV_VAR);
    if (msg == NULL)
      fprintf(stderr, "error %zd", i);
    else
      fprintf(stderr, "%s", msg);

    fputs("), ignoring it\n", stderr);

    return 1;
  }

  /* create newargv starting from argv[0], then encaenv_parsed, and last rest
     of argv; note we copy addresses, not strings themselves from argv */
  *newargc = argc + encaenv_parsed.we_wordc;
  *newargv = (char**)enca_malloc((*newargc)*sizeof(char*));
  (*newargv)[0] = argv[0];

  for (i = 0; i < encaenv_parsed.we_wordc; i++)
    (*newargv)[i+1] = enca_strdup(encaenv_parsed.we_wordv[i]);

  for (i = 1; i < (size_t)argc; i++)
    (*newargv)[i + encaenv_parsed.we_wordc] = argv[i];

  /* Free memory. */
  wordfree(&encaenv_parsed);

  return 0;
}
#else /* HAVE_WORDEXP */
{
  char *encaenv;
  size_t nitems;
  size_t i, state;
  const char *p;

  *newargc = argc;
  *newargv = argv;
  /* Fetch value of ENCA_ENV_VAR, if set. */
  encaenv = getenv(ENCA_ENV_VAR);
  if (encaenv == NULL)
    return 0;

  /* Count the number of tokens in ENCA_ENV_VAR. */
  encaenv = enca_strdup(encaenv);
  nitems = 0;
  state = 0;
  for (i = 0; encaenv[i] != '\0'; i++) {
    if (state == 0) {
      if (!isspace(encaenv[i]))
        nitems += ++state;
    }
    else {
      if (isspace(encaenv[i])) {
        encaenv[i] = '\0';
        state = 0;
      }
    }
  }

  /* Extend argv[].  (see above) */
  *newargc = argc + nitems;
  *newargv = (char**)enca_malloc((*newargc)*sizeof(char*));
  (*newargv)[0] = argv[0];

  p = encaenv;
  for (i = 0; i < nitems; i++) {
    while (isspace(*p))
      p++;

    (*newargv)[i+1] = enca_strdup(p);

    while (*p != '\0')
      p++;
    p++;
  }
  enca_free(encaenv);

  for (i = 1; i < argc; i++)
    (*newargv)[i + nitems] = argv[i];

  return 0;
}
#endif /* HAVE_WORDEXP */

/* Return output type appropriate for given option character. */
static OutputType
optchar_to_otype(const char c)
{
  switch (c) {
    case 'd': return OTYPE_DETAILS; /* Detailed output. */
    case 'e': return OTYPE_CANON;   /* Enca's name. */
    case 'f': return OTYPE_HUMAN;   /* Full (descriptive) output. */
    case 'i': return OTYPE_ICONV;   /* Iconv name. */
    case 'r': return OTYPE_RFC1345; /* RFC 1345 name as output */
    case 's': return OTYPE_CS2CS;   /* Cstocs name as output. */
    case 'm': return OTYPE_MIME;    /* Preferred MIME name as output. */
  }

  abort();
  return 0;
}

/* if otname represents a valid output type name, assign it to *otype,
   otherwise do nothing
   when gets NULL as the name, prints list of valid names instead */
static void
set_otype_from_name(const char *otname)
{
  /* Abbreviations table stores pointers, we need something to point to. */
  static const OutputType OTS[] = {
    OTYPE_DETAILS,
    OTYPE_CANON,
    OTYPE_HUMAN,
    OTYPE_RFC1345,
    OTYPE_ICONV,
    OTYPE_CS2CS,
    OTYPE_MIME,
    OTYPE_ALIASES
  };

  /* Output type names. */
  static const Abbreviation OTNAMES[] =
  {
    { "aliases", OTS+7 },
    { "cstocs", OTS+4 },
    { "details", OTS },
    { "enca", OTS+1 },
    { "human-readable", OTS+2 },
    { "iconv", OTS+5 },
    { "mime", OTS+6 },
    { "rfc1345", OTS+3 },
  };

  const Abbreviation *p;

  p = expand_abbreviation(otname, OTNAMES,
                          sizeof(OTNAMES)/sizeof(Abbreviation),
                          "output type");
  if (p != NULL)
    options.output_type = *(OutputType*)p->data;
}

/* parse -x argument, assign output encoding */
static int
parse_arg_x(const char *s)
{
  /* Encoding separator for -x argument. */
  static const char XENC_SEPARATOR[] = "..";
  static const size_t XENC_SEPARATOR_LEN = sizeof(XENC_SEPARATOR);

  /* Strip leading `..' if present. */
  if (strncmp(s, XENC_SEPARATOR, XENC_SEPARATOR_LEN) == 0)
    s += XENC_SEPARATOR_LEN;

  /* Assign target encoding. */
  enca_free(options.target_enc_str);
  options.target_enc_str = enca_strdup(s);

  /* We have to check for `..CHARSET/SURFACE..CHARSET2/SURFACE2' which would
   * enca_parse_encoding_name() split as
   * charset = CHARSET
   * surfaces = SURFACE..CHARSET2, SURFACE2
   * which is aboviously not what we want. */
  if (enca_strstr(s, XENC_SEPARATOR) == NULL)
    options.target_enc = enca_parse_encoding_name(s);
  else {
    options.target_enc.charset = ENCA_CS_UNKNOWN;
    options.target_enc.surface = 0;
  }

  return 0;
}

/* add comma separated list of converters to list of converters
   returns zero on success, nonzero otherwise
   when list is NULL return number of successfully added converters instead */
static int
add_parsed_converters(const char *list)
{
  /* Converter list separator for -E argument. */
  static const char CONVERTER_SEPARATOR = ',';

  char *s;
  char *p_c,*p_c1;
  static int nc = 0;

  if (list == NULL)
    return nc;

  s = enca_strdup(list);
  /* Add converter names one by one. */
  p_c = s;
  while ((p_c1 = strchr(p_c, CONVERTER_SEPARATOR)) != NULL) {
    *p_c1++ = '\0';
    if (add_converter(p_c) == 0) nc++;
    p_c = p_c1;
  }
  if (add_converter(p_c) == 0) nc++;
  enca_free(s);

  return 0;
}

/* create NULL-terminated file list from remaining fields in argv[]
   and return it */
static char**
make_filelist(const int n, char *argvrest[])
{
  int i;
  char **flist = NULL;

  /* Accept `-' as stdin. */
  if (n == 0
      || (n == 1 && strcmp(argvrest[0], "-") == 0))
    return NULL;

  flist = (char**)enca_malloc((n+1)*sizeof(char*));
  for (i = 0; i < n; i++) flist[i] = enca_strdup(argvrest[i]);
  flist[n] = NULL;

  return flist;
}

static int
prefix_filename(int pfx) {
  if (pfx != -1)
    options.prefix_filename = pfx;
  return options.prefix_filename;
}

/* prints some list user asked for (--list) -- just calls appropriate
 * functions from appropriate module (with funny abbreviation expansion)
 * when listname is NULL prints list of available lists instead */
static void
print_some_list(const char *listname)
{
  /* ISO C forbids initialization between function pointers and void*
     so we use one more level of indirection to comply (and hope gracious
     complier will forgive us our sins, amen) */
  static const ReportFunc printer_bics = print_builtin_charsets;
  static const ReportFunc printer_conv = print_converter_list;
  static const ReportFunc printer_char = print_all_charsets;
  static const ReportFunc printer_lang = print_languages;
  static const ReportFunc printer_list = print_lists;
  static const ReportFunc printer_name = print_names;
  static const ReportFunc printer_surf = print_surfaces;

  /* List names and pointers to pointers to list-printers. */
  static const Abbreviation LISTS[] = {
    { "built-in-charsets", &printer_bics },
    { "converters", &printer_conv },
    { "charsets", &printer_char },
    { "languages", &printer_lang },
    { "lists", &printer_list },
    { "names", &printer_name },
    { "surfaces", &printer_surf },
  };

  const Abbreviation *p = NULL;
  ReportFunc list_printer; /* Pointer to list printing functions. */

  /* Get the abbreviation data. */
  p = expand_abbreviation(listname, LISTS,
                          sizeof(LISTS)/sizeof(Abbreviation),
                          "list");

  /* p can be NULL in weird situations, e.g. when print_some_list() was
   * called recursively by itself through print_lists.
   * In all cases, return. */
  if (p == NULL)
    return;

  list_printer = *(ReportFunc*)p->data;
  list_printer();
}

#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
/* Create and return string containing only last component of path fullpath. */
static char*
strip_path(const char *fullpath)
{
  char *p;

  p = strrchr(fullpath, '/');
  if (p == NULL)
    p = (char*)fullpath;
  else
    p++;

  return enca_strdup(p);
}
#endif

/* Print version information. */
static void
print_version(void)
{
  printf("%s %s\n\n%s\n", PACKAGE_TARNAME, PACKAGE_VERSION, VERSION_TEXT);
}

/**
 * Prints builtin charsets.
 * Must be of type ReportFunc.
 **/
static void
print_builtin_charsets(void)
{
  print_charsets(1);
}

/**
 * Prints all charsets.
 * Must be of type ReportFunc.
 **/
static void
print_all_charsets(void)
{
  print_charsets(0);
}

/**
 * Prints list of charsets using name style from options.output_type.
 *
 * It prints all charsets, except:
 * - charsets without given name, and
 * - charsets without UCS-2 map when only_builtin is set.
 **/
static void
print_charsets(int only_builtin)
{
  size_t ncharsets, i;

  ncharsets = enca_number_of_charsets();
  for (i = 0; i < ncharsets; i++) {
    if (only_builtin && !enca_charset_has_ucs2_map(i))
      continue;

    switch (options.output_type) {
      case OTYPE_ALIASES:
      print_aliases(i);
      break;

      case OTYPE_CANON:
      case OTYPE_CONVERT:
      puts(enca_charset_name(i, ENCA_NAME_STYLE_ENCA));
      break;

      case OTYPE_HUMAN:
      case OTYPE_DETAILS:
      puts(enca_charset_name(i, ENCA_NAME_STYLE_HUMAN));
      break;

      case OTYPE_RFC1345:
      puts(enca_charset_name(i, ENCA_NAME_STYLE_RFC1345));
      break;

      case OTYPE_CS2CS:
      if (enca_charset_name(i, ENCA_NAME_STYLE_CSTOCS) != NULL)
        puts(enca_charset_name(i, ENCA_NAME_STYLE_CSTOCS));
      break;

      case OTYPE_ICONV:
      if (enca_charset_name(i, ENCA_NAME_STYLE_ICONV) != NULL)
        puts(enca_charset_name(i, ENCA_NAME_STYLE_ICONV));
      break;

      case OTYPE_MIME:
      if (enca_charset_name(i, ENCA_NAME_STYLE_MIME) != NULL)
        puts(enca_charset_name(i, ENCA_NAME_STYLE_MIME));
      break;

      default:
      abort();
      break;
    }
  }
}

/**
 * Prints all aliases of given charset.
 **/
void
print_aliases(size_t cs)
{
  size_t i, na;
  const char **aliases = enca_get_charset_aliases(cs, &na);

  for (i = 0; i < na; i++)
    printf("%s ", aliases[i]);

  putchar('\n');
  enca_free(aliases);
}

/**
 * Prints all [public] surfaces.
 * Must be of type ReportFunc.
 **/
static void
print_surfaces(void)
{
  EncaNameStyle ns;
  char *s;
  unsigned int i;

  /* Only these two know surfaces. */
  if (options.output_type == OTYPE_HUMAN)
    ns = ENCA_NAME_STYLE_HUMAN;
  else
    ns = ENCA_NAME_STYLE_ENCA;

  for (i = 1; i != 0; i <<= 1) {
    s = enca_get_surface_name(i, ns);
    if (s != NULL && s[0] != '\0') {
      fputs(s, stdout);
      if (ns == ENCA_NAME_STYLE_ENCA)
        putchar('\n');
      enca_free(s);
    }
  }
}

/* Magically print the list of lists. */
static void
print_lists(void)
{
  print_some_list(NULL);
}

/**
 * Prints all languages list of charsets of each.
 * Must be of type ReportFunc.
 * Quite illogically affected by options.output_type: it changes *language*
 * name style, instead of charset name style.
 **/
static void
print_languages(void)
{
  size_t nl, nc, i, j, maxlen;
  const char **l;
  int *c;
  int english;

  l = enca_get_languages(&nl);

  english = options.output_type == OTYPE_HUMAN
            || options.output_type == OTYPE_DETAILS;
  /* Find max. language name length for English. */
  maxlen = 0;
  if (english) {
    for (i = 0; i < nl; i++) {
      j = strlen(enca_language_english_name(l[i]));
      if (j > maxlen)
        maxlen = j;
    }
  }

  /* Print the names. */
  for (i = 0; i < nl; i++) {
    if (english)
      printf("%*s:", (int)maxlen, enca_language_english_name(l[i]));
    else
      printf("%s:", l[i]);
    c = enca_get_language_charsets(l[i], &nc);
    for (j = 0; j < nc; j++)
      printf(" %s", enca_charset_name(c[j], ENCA_NAME_STYLE_ENCA));

    putchar('\n');
    enca_free(c);
  }
  enca_free(l);
}

/**
 * Prints list of all encoding name styles.
 * Must be of type ReportFunc.
 **/
static void
print_names(void)
{
  set_otype_from_name(NULL);
}

/**
 * Print some text (help, copying, ...) and exit with given code.
 **/
static void
print_text_and_exit(const char *const *text, int exitcode)
{
  assert(text);

  for (; *text; text++)
    puts(*text);

  exit(exitcode);
}

/**
 * set_program_behaviour:
 *
 * Sets behaviour according to name we were called.
 **/
static void
set_program_behaviour(void)
{
  static const char enca_name[] = "enca";
  static const char enconv_name[] = "enconv";
  static const size_t nenca = sizeof(enca_name) - 1;
  static const size_t nenconv = sizeof(enconv_name) - 1;

  if (strncmp(program_name, enca_name, nenca) == 0
      && !isalpha(program_name[nenca])) {
    behaviour = BEHAVE_ENCA;
    return;
  }

  if (strncmp(program_name, enconv_name, nenconv) == 0
      && !isalpha(program_name[nenconv])) {
    behaviour = BEHAVE_ENCONV;
    return;
  }
}

/* vim: ts=2
 */
