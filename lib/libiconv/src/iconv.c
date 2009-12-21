/* Copyright (C) 2000-2009 Free Software Foundation, Inc.
   This file is part of the GNU LIBICONV Library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#ifndef ICONV_CONST
# define ICONV_CONST
#endif

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <locale.h>
#include <fcntl.h>

/* Ensure that iconv_no_i18n does not depend on libintl.  */
#ifdef NO_I18N
# undef ENABLE_NLS
# undef ENABLE_RELOCATABLE
#endif

#include "binary-io.h"
#include "progname.h"
#include "relocatable.h"
#include "xalloc.h"
#include "uniwidth.h"
#include "uniwidth/cjk.h"

/* Ensure that iconv_no_i18n does not depend on libintl.  */
#ifdef NO_I18N
#include <stdarg.h>
static void
error (int status, int errnum, const char *message, ...)
{
  va_list args;

  fflush(stdout);
  fprintf(stderr,"%s: ",program_name);
  va_start(args,message);
  vfprintf(stderr,message,args);
  va_end(args);
  if (errnum) {
    const char *s = strerror(errnum);
    if (s == NULL)
      s = "Unknown system error";
  }
  putc('\n',stderr);
  fflush(stderr);
  if (status)
    exit(status);
}
#else
# include "error.h"
#endif

#include "gettext.h"

#define _(str) gettext(str)

/* Ensure that iconv_no_i18n does not depend on libintl.  */
#ifdef NO_I18N
# define xmalloc malloc
# define xalloc_die abort
#endif

/* Locale independent test for a decimal digit.
   Argument can be  'char' or 'unsigned char'.  (Whereas the argument of
   <ctype.h> isdigit must be an 'unsigned char'.)  */
#undef isdigit
#define isdigit(c) ((unsigned int) ((c) - '0') < 10)

/* Locale independent test for a printable character.
   Argument can be  'char' or 'unsigned char'.  (Whereas the argument of
   <ctype.h> isdigit must be an 'unsigned char'.)  */
#define c_isprint(c) ((c) >= ' ' && (c) <= '~')

/* ========================================================================= */

static int discard_unconvertible = 0;
static int silent = 0;

static void usage (int exitcode)
{
  if (exitcode != 0) {
    const char* helpstring1 =
      /* TRANSLATORS: The first line of the short usage message.  */
      _("Usage: iconv [-c] [-s] [-f fromcode] [-t tocode] [file ...]");
    const char* helpstring2 =
      /* TRANSLATORS: The second line of the short usage message.
         Align it correctly against the first line.  */
      _("or:    iconv -l");
    fprintf(stderr, "%s\n%s\n", helpstring1, helpstring2);
    fprintf(stderr, _("Try `%s --help' for more information.\n"), program_name);
  } else {
    /* xgettext: no-wrap */
    /* TRANSLATORS: The first line of the long usage message.
       The %s placeholder expands to the program name.  */
    printf(_("\
Usage: %s [OPTION...] [-f ENCODING] [-t ENCODING] [INPUTFILE...]\n"),
           program_name);
    /* xgettext: no-wrap */
    /* TRANSLATORS: The second line of the long usage message.
       Align it correctly against the first line.
       The %s placeholder expands to the program name.  */
    printf(_("\
or:    %s -l\n"),
           program_name);
    printf("\n");
    /* xgettext: no-wrap */
    /* TRANSLATORS: Description of the iconv program.  */
    printf(_("\
Converts text from one encoding to another encoding.\n"));
    printf("\n");
    /* xgettext: no-wrap */
    printf(_("\
Options controlling the input and output format:\n"));
    /* xgettext: no-wrap */
    printf(_("\
  -f ENCODING, --from-code=ENCODING\n\
                              the encoding of the input\n"));
    /* xgettext: no-wrap */
    printf(_("\
  -t ENCODING, --to-code=ENCODING\n\
                              the encoding of the output\n"));
    printf("\n");
    /* xgettext: no-wrap */
    printf(_("\
Options controlling conversion problems:\n"));
    /* xgettext: no-wrap */
    printf(_("\
  -c                          discard unconvertible characters\n"));
    /* xgettext: no-wrap */
    printf(_("\
  --unicode-subst=FORMATSTRING\n\
                              substitution for unconvertible Unicode characters\n"));
    /* xgettext: no-wrap */
    printf(_("\
  --byte-subst=FORMATSTRING   substitution for unconvertible bytes\n"));
    /* xgettext: no-wrap */
    printf(_("\
  --widechar-subst=FORMATSTRING\n\
                              substitution for unconvertible wide characters\n"));
    printf("\n");
    /* xgettext: no-wrap */
    printf(_("\
Options controlling error output:\n"));
    /* xgettext: no-wrap */
    printf(_("\
  -s, --silent                suppress error messages about conversion problems\n"));
    printf("\n");
    /* xgettext: no-wrap */
    printf(_("\
Informative output:\n"));
    /* xgettext: no-wrap */
    printf(_("\
  -l, --list                  list the supported encodings\n"));
    /* xgettext: no-wrap */
    printf(_("\
  --help                      display this help and exit\n"));
    /* xgettext: no-wrap */
    printf(_("\
  --version                   output version information and exit\n"));
    printf("\n");
    /* TRANSLATORS: The placeholder indicates the bug-reporting address
       for this package.  Please add _another line_ saying
       "Report translation bugs to <...>\n" with the address for translation
       bugs (typically your translation team's web or email address).  */
    fputs(_("Report bugs to <bug-gnu-libiconv@gnu.org>.\n"),stdout);
  }
  exit(exitcode);
}

static void print_version (void)
{
  printf("iconv (GNU libiconv %d.%d)\n",
         _libiconv_version >> 8, _libiconv_version & 0xff);
  printf("Copyright (C) %s Free Software Foundation, Inc.\n", "2000-2009");
  /* xgettext: no-wrap */
  fputs (_("\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),stdout);
  /* TRANSLATORS: The %s placeholder expands to an author's name.  */
  printf(_("Written by %s.\n"),"Bruno Haible");
  exit(EXIT_SUCCESS);
}

static int print_one (unsigned int namescount, const char * const * names,
                      void* data)
{
  unsigned int i;
  (void)data;
  for (i = 0; i < namescount; i++) {
    if (i > 0)
      putc(' ',stdout);
    fputs(names[i],stdout);
  }
  putc('\n',stdout);
  return 0;
}

/* ========================================================================= */

/* Line number and column position. */
static unsigned int line;
static unsigned int column;
static const char* cjkcode;
/* Update the line number and column position after a character was
   successfully converted. */
static void update_line_column (unsigned int uc, void* data)
{
  if (uc == 0x000A) {
    line++;
    column = 0;
  } else {
    int width = uc_width(uc, cjkcode);
    if (width >= 0)
      column += width;
    else if (uc == 0x0009)
      column += 8 - (column % 8);
  }
}

/* ========================================================================= */

/* Production of placeholder strings as fallback for unconvertible
   characters. */

/* Check that the argument is a format string taking either no argument
   or exactly one unsigned integer argument. Returns the maximum output
   size of the format string. */
static size_t check_subst_formatstring (const char *format, const char *param_name)
{
  /* C format strings are described in POSIX (IEEE P1003.1 2001), section
     XSH 3 fprintf().  See also Linux fprintf(3) manual page.
     For simplicity, we don't accept
       - the '%m$' reordering syntax,
       - the 'I' flag,
       - width specifications referring to an argument,
       - precision specifications referring to an argument,
       - size specifiers,
       - format specifiers other than 'o', 'u', 'x', 'X'.
     What remains?
     A directive
       - starts with '%',
       - is optionally followed by any of the characters '#', '0', '-', ' ',
         '+', "'", each of which acts as a flag,
       - is optionally followed by a width specification: a nonempty digit
         sequence,
       - is optionally followed by '.' and a precision specification: a
         nonempty digit sequence,
       - is finished by a specifier
         - '%', that needs no argument,
         - 'o', 'u', 'x', 'X', that need an unsigned integer argument.
   */
  size_t maxsize = 0;
  unsigned int unnumbered_arg_count = 0;

  for (; *format != '\0';) {
    if (*format++ == '%') {
      /* A directive. */
      unsigned int width = 0;
      unsigned int precision = 0;
      unsigned int length;
      /* Parse flags. */
      for (;;) {
        if (*format == ' ' || *format == '+' || *format == '-'
            || *format == '#' || *format == '0' || *format == '\'')
          format++;
        else
          break;
      }
      /* Parse width. */
      if (*format == '*')
        error(EXIT_FAILURE,0,
              /* TRANSLATORS: An error message.
                 The %s placeholder expands to a command-line option.  */
              _("%s argument: A format directive with a variable width is not allowed here."),
              param_name);
      if (isdigit (*format)) {
        do {
          width = 10*width + (*format - '0');
          format++;
        } while (isdigit (*format));
      }
      /* Parse precision. */
      if (*format == '.') {
        format++;
        if (*format == '*')
          error(EXIT_FAILURE,0,
                /* TRANSLATORS: An error message.
                   The %s placeholder expands to a command-line option.  */
                _("%s argument: A format directive with a variable precision is not allowed here."),
                param_name);
        if (isdigit (*format)) {
          do {
            precision = 10*precision + (*format - '0');
            format++;
          } while (isdigit (*format));
        }
      }
      /* Parse size. */
      switch (*format) {
        case 'h': case 'l': case 'L': case 'q':
        case 'j': case 'z': case 'Z': case 't':
          error(EXIT_FAILURE,0,
                /* TRANSLATORS: An error message.
                   The %s placeholder expands to a command-line option.  */
                _("%s argument: A format directive with a size is not allowed here."),
                param_name);
      }
      /* Parse end of directive. */
      switch (*format) {
        case '%':
          length = 1;
          break;
        case 'u': case 'o': case 'x': case 'X':
          if (*format == 'u') {
            length = (unsigned int) (sizeof (unsigned int) * CHAR_BIT
                                     * 0.30103 /* binary -> decimal */
                                    )
                     + 1; /* turn floor into ceil */
            if (length < precision)
              length = precision;
            length *= 2; /* estimate for FLAG_GROUP */
            length += 1; /* account for leading sign */
          } else if (*format == 'o') {
            length = (unsigned int) (sizeof (unsigned int) * CHAR_BIT
                                     * 0.333334 /* binary -> octal */
                                    )
                     + 1; /* turn floor into ceil */
            if (length < precision)
              length = precision;
            length += 1; /* account for leading sign */
          } else { /* 'x', 'X' */
            length = (unsigned int) (sizeof (unsigned int) * CHAR_BIT
                                     * 0.25 /* binary -> hexadecimal */
                                    )
                     + 1; /* turn floor into ceil */
            if (length < precision)
              length = precision;
            length += 2; /* account for leading sign or alternate form */
          }
          unnumbered_arg_count++;
          break;
        default:
          if (*format == '\0')
            error(EXIT_FAILURE,0,
                  /* TRANSLATORS: An error message.
                     The %s placeholder expands to a command-line option.  */
                  _("%s argument: The string ends in the middle of a directive."),
                  param_name);
          else if (c_isprint(*format))
            error(EXIT_FAILURE,0,
                  /* TRANSLATORS: An error message.
                     The %s placeholder expands to a command-line option.
                     The %c placeholder expands to an unknown format directive.  */
                  _("%s argument: The character '%c' is not a valid conversion specifier."),
                  param_name,*format);
          else
            error(EXIT_FAILURE,0,
                  /* TRANSLATORS: An error message.
                     The %s placeholder expands to a command-line option.  */
                  _("%s argument: The character that terminates the format directive is not a valid conversion specifier."),
                  param_name);
          abort(); /*NOTREACHED*/
      }
      format++;
      if (length < width)
        length = width;
      maxsize += length;
    } else
      maxsize++;
  }
  if (unnumbered_arg_count > 1)
    error(EXIT_FAILURE,0,
          /* TRANSLATORS: An error message.
             The %s placeholder expands to a command-line option.
             The %u placeholder expands to the number of arguments consumed by the format string.  */
          ngettext("%s argument: The format string consumes more than one argument: %u argument.",
                   "%s argument: The format string consumes more than one argument: %u arguments.",
                   unnumbered_arg_count),
          param_name,unnumbered_arg_count);
  return maxsize;
}

/* Format strings. */
static const char* ilseq_byte_subst;
static const char* ilseq_wchar_subst;
static const char* ilseq_unicode_subst;

/* Maximum result size for each format string. */
static size_t ilseq_byte_subst_size;
static size_t ilseq_wchar_subst_size;
static size_t ilseq_unicode_subst_size;

/* Buffer of size ilseq_byte_subst_size+1. */
static char* ilseq_byte_subst_buffer;
#if HAVE_WCHAR_T
/* Buffer of size ilseq_wchar_subst_size+1. */
static char* ilseq_wchar_subst_buffer;
#endif
/* Buffer of size ilseq_unicode_subst_size+1. */
static char* ilseq_unicode_subst_buffer;

/* Auxiliary variables for subst_mb_to_uc_fallback. */
/* Converter from locale encoding to UCS-4. */
static iconv_t subst_mb_to_uc_cd;
/* Buffer of size ilseq_byte_subst_size. */
static unsigned int* subst_mb_to_uc_temp_buffer;

static void subst_mb_to_uc_fallback
            (const char* inbuf, size_t inbufsize,
             void (*write_replacement) (const unsigned int *buf, size_t buflen,
                                        void* callback_arg),
             void* callback_arg,
             void* data)
{
  for (; inbufsize > 0; inbuf++, inbufsize--) {
    const char* inptr;
    size_t inbytesleft;
    char* outptr;
    size_t outbytesleft;
    sprintf(ilseq_byte_subst_buffer,
            ilseq_byte_subst, (unsigned int)(unsigned char)*inbuf);
    inptr = ilseq_byte_subst_buffer;
    inbytesleft = strlen(ilseq_byte_subst_buffer);
    outptr = (char*)subst_mb_to_uc_temp_buffer;
    outbytesleft = ilseq_byte_subst_size*sizeof(unsigned int);
    iconv(subst_mb_to_uc_cd,NULL,NULL,NULL,NULL);
    if (iconv(subst_mb_to_uc_cd, (ICONV_CONST char**)&inptr,&inbytesleft, &outptr,&outbytesleft)
        == (size_t)(-1)
        || iconv(subst_mb_to_uc_cd, NULL,NULL, &outptr,&outbytesleft)
           == (size_t)(-1))
      error(EXIT_FAILURE,0,
            /* TRANSLATORS: An error message.
               The %s placeholder expands to a piece of text, specified through --byte-subst.  */
            _("cannot convert byte substitution to Unicode: %s"),
            ilseq_byte_subst_buffer);
    if (!(outbytesleft%sizeof(unsigned int) == 0))
      abort();
    write_replacement(subst_mb_to_uc_temp_buffer,
                      ilseq_byte_subst_size-(outbytesleft/sizeof(unsigned int)),
                      callback_arg);
  }
}

/* Auxiliary variables for subst_uc_to_mb_fallback. */
/* Converter from locale encoding to target encoding. */
static iconv_t subst_uc_to_mb_cd;
/* Buffer of size ilseq_unicode_subst_size*4. */
static char* subst_uc_to_mb_temp_buffer;

static void subst_uc_to_mb_fallback
            (unsigned int code,
             void (*write_replacement) (const char *buf, size_t buflen,
                                        void* callback_arg),
             void* callback_arg,
             void* data)
{
  const char* inptr;
  size_t inbytesleft;
  char* outptr;
  size_t outbytesleft;
  sprintf(ilseq_unicode_subst_buffer, ilseq_unicode_subst, code);
  inptr = ilseq_unicode_subst_buffer;
  inbytesleft = strlen(ilseq_unicode_subst_buffer);
  outptr = subst_uc_to_mb_temp_buffer;
  outbytesleft = ilseq_unicode_subst_size*4;
  iconv(subst_uc_to_mb_cd,NULL,NULL,NULL,NULL);
  if (iconv(subst_uc_to_mb_cd, (ICONV_CONST char**)&inptr,&inbytesleft, &outptr,&outbytesleft)
      == (size_t)(-1)
      || iconv(subst_uc_to_mb_cd, NULL,NULL, &outptr,&outbytesleft)
         == (size_t)(-1))
    error(EXIT_FAILURE,0,
          /* TRANSLATORS: An error message.
             The %s placeholder expands to a piece of text, specified through --unicode-subst.  */
          _("cannot convert unicode substitution to target encoding: %s"),
          ilseq_unicode_subst_buffer);
  write_replacement(subst_uc_to_mb_temp_buffer,
                    ilseq_unicode_subst_size*4-outbytesleft,
                    callback_arg);
}

#if HAVE_WCHAR_T

/* Auxiliary variables for subst_mb_to_wc_fallback. */
/* Converter from locale encoding to wchar_t. */
static iconv_t subst_mb_to_wc_cd;
/* Buffer of size ilseq_byte_subst_size. */
static wchar_t* subst_mb_to_wc_temp_buffer;

static void subst_mb_to_wc_fallback
            (const char* inbuf, size_t inbufsize,
             void (*write_replacement) (const wchar_t *buf, size_t buflen,
                                        void* callback_arg),
             void* callback_arg,
             void* data)
{
  for (; inbufsize > 0; inbuf++, inbufsize--) {
    const char* inptr;
    size_t inbytesleft;
    char* outptr;
    size_t outbytesleft;
    sprintf(ilseq_byte_subst_buffer,
            ilseq_byte_subst, (unsigned int)(unsigned char)*inbuf);
    inptr = ilseq_byte_subst_buffer;
    inbytesleft = strlen(ilseq_byte_subst_buffer);
    outptr = (char*)subst_mb_to_wc_temp_buffer;
    outbytesleft = ilseq_byte_subst_size*sizeof(wchar_t);
    iconv(subst_mb_to_wc_cd,NULL,NULL,NULL,NULL);
    if (iconv(subst_mb_to_wc_cd, (ICONV_CONST char**)&inptr,&inbytesleft, &outptr,&outbytesleft)
        == (size_t)(-1)
        || iconv(subst_mb_to_wc_cd, NULL,NULL, &outptr,&outbytesleft)
           == (size_t)(-1))
      error(EXIT_FAILURE,0,
            /* TRANSLATORS: An error message.
               The %s placeholder expands to a piece of text, specified through --byte-subst.  */
            _("cannot convert byte substitution to wide string: %s"),
            ilseq_byte_subst_buffer);
    if (!(outbytesleft%sizeof(wchar_t) == 0))
      abort();
    write_replacement(subst_mb_to_wc_temp_buffer,
                      ilseq_byte_subst_size-(outbytesleft/sizeof(wchar_t)),
                      callback_arg);
  }
}

/* Auxiliary variables for subst_wc_to_mb_fallback. */
/* Converter from locale encoding to target encoding. */
static iconv_t subst_wc_to_mb_cd;
/* Buffer of size ilseq_wchar_subst_size*4.
   Hardcode factor 4, because MB_LEN_MAX is not reliable on some platforms. */
static char* subst_wc_to_mb_temp_buffer;

static void subst_wc_to_mb_fallback
            (wchar_t code,
             void (*write_replacement) (const char *buf, size_t buflen,
                                        void* callback_arg),
             void* callback_arg,
             void* data)
{
  const char* inptr;
  size_t inbytesleft;
  char* outptr;
  size_t outbytesleft;
  sprintf(ilseq_wchar_subst_buffer, ilseq_wchar_subst, (unsigned int) code);
  inptr = ilseq_wchar_subst_buffer;
  inbytesleft = strlen(ilseq_wchar_subst_buffer);
  outptr = subst_wc_to_mb_temp_buffer;
  outbytesleft = ilseq_wchar_subst_size*4;
  iconv(subst_wc_to_mb_cd,NULL,NULL,NULL,NULL);
  if (iconv(subst_wc_to_mb_cd, (ICONV_CONST char**)&inptr,&inbytesleft, &outptr,&outbytesleft)
      == (size_t)(-1)
      || iconv(subst_wc_to_mb_cd, NULL,NULL, &outptr,&outbytesleft)
         == (size_t)(-1))
    error(EXIT_FAILURE,0,
          /* TRANSLATORS: An error message.
             The %s placeholder expands to a piece of text, specified through --widechar-subst.  */
          _("cannot convert widechar substitution to target encoding: %s"),
          ilseq_wchar_subst_buffer);
  write_replacement(subst_wc_to_mb_temp_buffer,
                    ilseq_wchar_subst_size*4-outbytesleft,
                    callback_arg);
}

#else

#define subst_mb_to_wc_fallback NULL
#define subst_wc_to_mb_fallback NULL

#endif

/* Auxiliary variables for subst_mb_to_mb_fallback. */
/* Converter from locale encoding to target encoding. */
static iconv_t subst_mb_to_mb_cd;
/* Buffer of size ilseq_byte_subst_size*4. */
static char* subst_mb_to_mb_temp_buffer;

static void subst_mb_to_mb_fallback (const char* inbuf, size_t inbufsize)
{
  for (; inbufsize > 0; inbuf++, inbufsize--) {
    const char* inptr;
    size_t inbytesleft;
    char* outptr;
    size_t outbytesleft;
    sprintf(ilseq_byte_subst_buffer,
            ilseq_byte_subst, (unsigned int)(unsigned char)*inbuf);
    inptr = ilseq_byte_subst_buffer;
    inbytesleft = strlen(ilseq_byte_subst_buffer);
    outptr = subst_mb_to_mb_temp_buffer;
    outbytesleft = ilseq_byte_subst_size*4;
    iconv(subst_mb_to_mb_cd,NULL,NULL,NULL,NULL);
    if (iconv(subst_mb_to_mb_cd, (ICONV_CONST char**)&inptr,&inbytesleft, &outptr,&outbytesleft)
        == (size_t)(-1)
        || iconv(subst_mb_to_mb_cd, NULL,NULL, &outptr,&outbytesleft)
           == (size_t)(-1))
      error(EXIT_FAILURE,0,
            /* TRANSLATORS: An error message.
               The %s placeholder expands to a piece of text, specified through --byte-subst.  */
            _("cannot convert byte substitution to target encoding: %s"),
            ilseq_byte_subst_buffer);
    fwrite(subst_mb_to_mb_temp_buffer,1,ilseq_byte_subst_size*4-outbytesleft,
           stdout);
  }
}

/* ========================================================================= */

/* Error messages during conversion.  */

static void conversion_error_EILSEQ (const char* infilename)
{
  fflush(stdout);
  if (column > 0)
    putc('\n',stderr);
  error(0,0,
        /* TRANSLATORS: An error message.
           The placeholders expand to the input file name, a line number, and a column number.  */
        _("%s:%u:%u: cannot convert"),
        infilename,line,column);
}

static void conversion_error_EINVAL (const char* infilename)
{
  fflush(stdout);
  if (column > 0)
    putc('\n',stderr);
  error(0,0,
        /* TRANSLATORS: An error message.
           The placeholders expand to the input file name, a line number, and a column number.
           A "shift sequence" is a sequence of bytes that changes the state of the converter;
           this concept exists only for "stateful" encodings like ISO-2022-JP.  */
        _("%s:%u:%u: incomplete character or shift sequence"),
        infilename,line,column);
}

static void conversion_error_other (int errnum, const char* infilename)
{
  fflush(stdout);
  if (column > 0)
    putc('\n',stderr);
  error(0,errnum,
        /* TRANSLATORS: The first part of an error message.
           It is followed by a colon and a detail message.
           The placeholders expand to the input file name, a line number, and a column number.  */
        _("%s:%u:%u"),
        infilename,line,column);
}

/* Convert the input given in infile.  */

static int convert (iconv_t cd, FILE* infile, const char* infilename)
{
  char inbuf[4096+4096];
  size_t inbufrest = 0;
  char initial_outbuf[4096];
  char *outbuf = initial_outbuf;
  size_t outbufsize = sizeof(initial_outbuf);
  int status = 0;

#if O_BINARY
  SET_BINARY(fileno(infile));
#endif
  line = 1; column = 0;
  iconv(cd,NULL,NULL,NULL,NULL);
  for (;;) {
    size_t inbufsize = fread(inbuf+4096,1,4096,infile);
    if (inbufsize == 0) {
      if (inbufrest == 0)
        break;
      else {
        if (ilseq_byte_subst != NULL)
          subst_mb_to_mb_fallback(inbuf+4096-inbufrest, inbufrest);
        if (!silent)
          conversion_error_EINVAL(infilename);
        status = 1;
        goto done;
      }
    } else {
      const char* inptr = inbuf+4096-inbufrest;
      size_t insize = inbufrest+inbufsize;
      inbufrest = 0;
      while (insize > 0) {
        char* outptr = outbuf;
        size_t outsize = outbufsize;
        size_t res = iconv(cd,(ICONV_CONST char**)&inptr,&insize,&outptr,&outsize);
        if (outptr != outbuf) {
          int saved_errno = errno;
          if (fwrite(outbuf,1,outptr-outbuf,stdout) < outptr-outbuf) {
            status = 1;
            goto done;
          }
          errno = saved_errno;
        }
        if (res == (size_t)(-1)) {
          if (errno == EILSEQ) {
            if (discard_unconvertible == 1) {
              int one = 1;
              iconvctl(cd,ICONV_SET_DISCARD_ILSEQ,&one);
              discard_unconvertible = 2;
              status = 1;
            } else {
              if (!silent)
                conversion_error_EILSEQ(infilename);
              status = 1;
              goto done;
            }
          } else if (errno == EINVAL) {
            if (inbufsize == 0 || insize > 4096) {
              if (!silent)
                conversion_error_EINVAL(infilename);
              status = 1;
              goto done;
            } else {
              inbufrest = insize;
              if (insize > 0) {
                /* Like memcpy(inbuf+4096-insize,inptr,insize), except that
                   we cannot use memcpy here, because source and destination
                   regions may overlap. */
                char* restptr = inbuf+4096-insize;
                do { *restptr++ = *inptr++; } while (--insize > 0);
              }
              break;
            }
          } else if (errno == E2BIG) {
            if (outptr==outbuf) {
              /* outbuf is too small. Double its size. */
              if (outbuf != initial_outbuf)
                free(outbuf);
              outbufsize = 2*outbufsize;
              if (outbufsize==0) /* integer overflow? */
                xalloc_die();
              outbuf = (char*)xmalloc(outbufsize);
            }
          } else {
            if (!silent)
              conversion_error_other(errno,infilename);
            status = 1;
            goto done;
          }
        }
      }
    }
  }
  for (;;) {
    char* outptr = outbuf;
    size_t outsize = outbufsize;
    size_t res = iconv(cd,NULL,NULL,&outptr,&outsize);
    if (outptr != outbuf) {
      int saved_errno = errno;
      if (fwrite(outbuf,1,outptr-outbuf,stdout) < outptr-outbuf) {
        status = 1;
        goto done;
      }
      errno = saved_errno;
    }
    if (res == (size_t)(-1)) {
      if (errno == EILSEQ) {
        if (discard_unconvertible == 1) {
          int one = 1;
          iconvctl(cd,ICONV_SET_DISCARD_ILSEQ,&one);
          discard_unconvertible = 2;
          status = 1;
        } else {
          if (!silent)
            conversion_error_EILSEQ(infilename);
          status = 1;
          goto done;
        }
      } else if (errno == EINVAL) {
        if (!silent)
          conversion_error_EINVAL(infilename);
        status = 1;
        goto done;
      } else if (errno == E2BIG) {
        if (outptr==outbuf) {
          /* outbuf is too small. Double its size. */
          if (outbuf != initial_outbuf)
            free(outbuf);
          outbufsize = 2*outbufsize;
          if (outbufsize==0) /* integer overflow? */
            xalloc_die();
          outbuf = (char*)xmalloc(outbufsize);
        }
      } else {
        if (!silent)
          conversion_error_other(errno,infilename);
        status = 1;
        goto done;
      }
    } else
      break;
  }
  if (ferror(infile)) {
    fflush(stdout);
    if (column > 0)
      putc('\n',stderr);
    error(0,0,
          /* TRANSLATORS: An error message.
             The placeholder expands to the input file name.  */
          _("%s: I/O error"),
          infilename);
    status = 1;
    goto done;
  }
 done:
  if (outbuf != initial_outbuf)
    free(outbuf);
  return status;
}

/* ========================================================================= */

int main (int argc, char* argv[])
{
  const char* fromcode = NULL;
  const char* tocode = NULL;
  int do_list = 0;
  iconv_t cd;
  struct iconv_fallbacks fallbacks;
  struct iconv_hooks hooks;
  int i;
  int status;

  set_program_name (argv[0]);
#if HAVE_SETLOCALE
  /* Needed for the locale dependent encodings, "char" and "wchar_t",
     and for gettext. */
  setlocale(LC_CTYPE,"");
#if ENABLE_NLS
  /* Needed for gettext. */
  setlocale(LC_MESSAGES,"");
#endif
#endif
#if ENABLE_NLS
  bindtextdomain("libiconv",relocate(LOCALEDIR));
#endif
  textdomain("libiconv");
  for (i = 1; i < argc;) {
    size_t len = strlen(argv[i]);
    if (!strcmp(argv[i],"--")) {
      i++;
      break;
    }
    if (!strcmp(argv[i],"-f")
        /* --f ... --from-code */
        || (len >= 3 && len <= 11 && !strncmp(argv[i],"--from-code",len))
        /* --from-code=... */
        || (len >= 12 && !strncmp(argv[i],"--from-code=",12))) {
      if (len < 12)
        if (i == argc-1) usage(1);
      if (fromcode != NULL) usage(1);
      if (len < 12) {
        fromcode = argv[i+1];
        i += 2;
      } else {
        fromcode = argv[i]+12;
        i++;
      }
      continue;
    }
    if (!strcmp(argv[i],"-t")
        /* --t ... --to-code */
        || (len >= 3 && len <= 9 && !strncmp(argv[i],"--to-code",len))
        /* --from-code=... */
        || (len >= 10 && !strncmp(argv[i],"--to-code=",10))) {
      if (len < 10)
        if (i == argc-1) usage(1);
      if (tocode != NULL) usage(1);
      if (len < 10) {
        tocode = argv[i+1];
        i += 2;
      } else {
        tocode = argv[i]+10;
        i++;
      }
      continue;
    }
    if (!strcmp(argv[i],"-l")
        /* --l ... --list */
        || (len >= 3 && len <= 6 && !strncmp(argv[i],"--list",len))) {
      do_list = 1;
      i++;
      continue;
    }
    if (/* --by ... --byte-subst */
        (len >= 4 && len <= 12 && !strncmp(argv[i],"--byte-subst",len))
        /* --byte-subst=... */
        || (len >= 13 && !strncmp(argv[i],"--byte-subst=",13))) {
      if (len < 13) {
        if (i == argc-1) usage(1);
        ilseq_byte_subst = argv[i+1];
        i += 2;
      } else {
        ilseq_byte_subst = argv[i]+13;
        i++;
      }
      ilseq_byte_subst_size =
        check_subst_formatstring(ilseq_byte_subst, "--byte-subst");
      continue;
    }
    if (/* --w ... --widechar-subst */
        (len >= 3 && len <= 16 && !strncmp(argv[i],"--widechar-subst",len))
        /* --widechar-subst=... */
        || (len >= 17 && !strncmp(argv[i],"--widechar-subst=",17))) {
      if (len < 17) {
        if (i == argc-1) usage(1);
        ilseq_wchar_subst = argv[i+1];
        i += 2;
      } else {
        ilseq_wchar_subst = argv[i]+17;
        i++;
      }
      ilseq_wchar_subst_size =
        check_subst_formatstring(ilseq_wchar_subst, "--widechar-subst");
      continue;
    }
    if (/* --u ... --unicode-subst */
        (len >= 3 && len <= 15 && !strncmp(argv[i],"--unicode-subst",len))
        /* --unicode-subst=... */
        || (len >= 16 && !strncmp(argv[i],"--unicode-subst=",16))) {
      if (len < 16) {
        if (i == argc-1) usage(1);
        ilseq_unicode_subst = argv[i+1];
        i += 2;
      } else {
        ilseq_unicode_subst = argv[i]+16;
        i++;
      }
      ilseq_unicode_subst_size =
        check_subst_formatstring(ilseq_unicode_subst, "--unicode-subst");
      continue;
    }
    if /* --s ... --silent */
       (len >= 3 && len <= 8 && !strncmp(argv[i],"--silent",len)) {
      silent = 1;
      continue;
    }
    if /* --h ... --help */
       (len >= 3 && len <= 6 && !strncmp(argv[i],"--help",len)) {
      usage(0);
    }
    if /* --v ... --version */
       (len >= 3 && len <= 9 && !strncmp(argv[i],"--version",len)) {
      print_version();
    }
#if O_BINARY
    /* Backward compatibility with iconv <= 1.9.1. */
    if /* --bi ... --binary */
       (len >= 4 && len <= 8 && !strncmp(argv[i],"--binary",len)) {
      i++;
      continue;
    }
#endif
    if (argv[i][0] == '-') {
      const char *option = argv[i] + 1;
      if (*option == '\0')
        usage(1);
      for (; *option; option++)
        switch (*option) {
          case 'c': discard_unconvertible = 1; break;
          case 's': silent = 1; break;
          default: usage(1);
        }
      i++;
      continue;
    }
    break;
  }
  if (do_list) {
    if (i != 2 || i != argc)
      usage(1);
    iconvlist(print_one,NULL);
    status = 0;
  } else {
#if O_BINARY
    SET_BINARY(fileno(stdout));
#endif
    if (fromcode == NULL)
      fromcode = "char";
    if (tocode == NULL)
      tocode = "char";
    cd = iconv_open(tocode,fromcode);
    if (cd == (iconv_t)(-1)) {
      if (iconv_open("UCS-4",fromcode) == (iconv_t)(-1))
        error(0,0,
              /* TRANSLATORS: An error message.
                 The placeholder expands to the encoding name, specified through --from-code.  */
              _("conversion from %s unsupported"),
              fromcode);
      else if (iconv_open(tocode,"UCS-4") == (iconv_t)(-1))
        error(0,0,
              /* TRANSLATORS: An error message.
                 The placeholder expands to the encoding name, specified through --to-code.  */
              _("conversion to %s unsupported"),
              tocode);
      else
        error(0,0,
              /* TRANSLATORS: An error message.
                 The placeholders expand to the encoding names, specified through --from-code and --to-code, respectively.  */
              _("conversion from %s to %s unsupported"),
              fromcode,tocode);
      error(EXIT_FAILURE,0,
            /* TRANSLATORS: Additional advice after an error message.
               The %s placeholder expands to the program name.  */
            _("try '%s -l' to get the list of supported encodings"),
            program_name);
    }
    /* Look at fromcode and tocode, to determine whether character widths
       should be determined according to legacy CJK conventions. */
    cjkcode = iconv_canonicalize(tocode);
    if (!is_cjk_encoding(cjkcode))
      cjkcode = iconv_canonicalize(fromcode);
    /* Set up fallback routines for handling impossible conversions. */
    if (ilseq_byte_subst != NULL)
      ilseq_byte_subst_buffer = (char*)xmalloc((ilseq_byte_subst_size+1)*sizeof(char));
    if (!discard_unconvertible) {
      #if HAVE_WCHAR_T
      if (ilseq_wchar_subst != NULL)
        ilseq_wchar_subst_buffer = (char*)xmalloc((ilseq_wchar_subst_size+1)*sizeof(char));
      #endif
      if (ilseq_unicode_subst != NULL)
        ilseq_unicode_subst_buffer = (char*)xmalloc((ilseq_unicode_subst_size+1)*sizeof(char));
      if (ilseq_byte_subst != NULL) {
        subst_mb_to_uc_cd = iconv_open("UCS-4-INTERNAL","char");
        subst_mb_to_uc_temp_buffer = (unsigned int*)xmalloc(ilseq_byte_subst_size*sizeof(unsigned int));
        #if HAVE_WCHAR_T
        subst_mb_to_wc_cd = iconv_open("wchar_t","char");
        subst_mb_to_wc_temp_buffer = (wchar_t*)xmalloc(ilseq_byte_subst_size*sizeof(wchar_t));
        #endif
        subst_mb_to_mb_cd = iconv_open(tocode,"char");
        subst_mb_to_mb_temp_buffer = (char*)xmalloc(ilseq_byte_subst_size*4);
      }
      #if HAVE_WCHAR_T
      if (ilseq_wchar_subst != NULL) {
        subst_wc_to_mb_cd = iconv_open(tocode,"char");
        subst_wc_to_mb_temp_buffer = (char*)xmalloc(ilseq_wchar_subst_size*4);
      }
      #endif
      if (ilseq_unicode_subst != NULL) {
        subst_uc_to_mb_cd = iconv_open(tocode,"char");
        subst_uc_to_mb_temp_buffer = (char*)xmalloc(ilseq_unicode_subst_size*4);
      }
      fallbacks.mb_to_uc_fallback =
        (ilseq_byte_subst != NULL ? subst_mb_to_uc_fallback : NULL);
      fallbacks.uc_to_mb_fallback =
        (ilseq_unicode_subst != NULL ? subst_uc_to_mb_fallback : NULL);
      fallbacks.mb_to_wc_fallback =
        (ilseq_byte_subst != NULL ? subst_mb_to_wc_fallback : NULL);
      fallbacks.wc_to_mb_fallback =
        (ilseq_wchar_subst != NULL ? subst_wc_to_mb_fallback : NULL);
      fallbacks.data = NULL;
      iconvctl(cd, ICONV_SET_FALLBACKS, &fallbacks);
    }
    /* Set up hooks for updating the line and column position. */
    hooks.uc_hook = update_line_column;
    hooks.wc_hook = NULL;
    hooks.data = NULL;
    iconvctl(cd, ICONV_SET_HOOKS, &hooks);
    if (i == argc)
      status = convert(cd,stdin,
                       /* TRANSLATORS: A filename substitute denoting standard input.  */
                       _("(stdin)"));
    else {
      status = 0;
      for (; i < argc; i++) {
        const char* infilename = argv[i];
        FILE* infile = fopen(infilename,"r");
        if (infile == NULL) {
          int saved_errno = errno;
          error(0,saved_errno,
                /* TRANSLATORS: The first part of an error message.
                   It is followed by a colon and a detail message.
                   The %s placeholder expands to the input file name.  */
                _("%s"),
                infilename);
          status = 1;
        } else {
          status |= convert(cd,infile,infilename);
          fclose(infile);
        }
      }
    }
    iconv_close(cd);
  }
  if (ferror(stdout) || fclose(stdout)) {
    error(0,0,
          /* TRANSLATORS: An error message.  */
          _("I/O error"));
    status = 1;
  }
  exit(status);
}
