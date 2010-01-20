/*
  @(#) $Id: convert.c,v 1.28 2005/12/01 10:08:53 yeti Exp $
  conversion to other encodings

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

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#else
pid_t waitpid(pid_t pid, int *status, int options);
#endif

/* We can't go on w/o this, defining struct stat manually is braindamaged. */
#include <sys/types.h>
#include <sys/stat.h>

/* converter flags */
#define CONV_EXTERN   0x0001

/* converter-type (filename, input encoding, output encoding) */
typedef int (* ConverterFunc)(File*, EncaEncoding);

/* struct converter data */
typedef struct _ConverterData ConverterData;

struct _ConverterData {
  unsigned long int flags; /* flags */
  ConverterFunc convfunc; /* pointer to converter function */
};

/* struct converter list */
typedef struct _Converter Converter;

struct _Converter {
  const Abbreviation *conv; /* the converter (an abbreviation table entry) */
  Converter *next; /* next in the list */
};

/* converter list */
static Converter *converters = NULL;

/* data for xtable */
static struct {
  size_t ncharsets; /* number of charsets */
  int *charsets; /* charset id's for active language [ncharsets] */
  byte *tables; /* tables from charsets to target_charset [ncharsets * 0x100] */
  int *have_table; /* whether particular table is already cached [ncharsets] */
  unsigned int *ucs2_map; /* temporary space for map computation [0x10000] */
  unsigned int target_map[0x100];
}
xdata = { 0, NULL, NULL, NULL, NULL, { 0 } };

/* Local prototypes. */
static int   convert_builtin (File *file,
                              EncaEncoding from_enc);
static const byte* xtable    (int from_charset);
static void  xdata_free      (void);

static const ConverterData cdata_builtin = { 0, &convert_builtin };
#ifdef HAVE_LIBRECODE
static const ConverterData cdata_librecode = { 0, &convert_recode };
#endif /* HAVE_LIBRECODE */
#ifdef HAVE_GOOD_ICONV
static const ConverterData cdata_iconv = { 0, &convert_iconv };
#endif /* HAVE_GOOD_ICONV */
#ifdef ENABLE_EXTERNAL
static const ConverterData cdata_extern = { CONV_EXTERN, &convert_external };
#endif /* ENABLE_EXTERNAL */

static const Abbreviation CONVERTERS[] = {
  { "built-in", &cdata_builtin },
#ifdef HAVE_LIBRECODE
  { "librecode", &cdata_librecode },
#endif /* HAVE_LIBRECODE */
#ifdef HAVE_GOOD_ICONV
  { "iconv", &cdata_iconv },
#endif /* HAVE_GOOD_ICONV */
#ifdef ENABLE_EXTERNAL
  { "extern", &cdata_extern }
#endif /* ENABLE_EXTERNAL */
};

/* decide which converter should be run and do common checks
   from_enc, to_enc are current and requested encoding
   returns error code

   it doesn't open the file (guess() did it) and doesn't close it (caller does
   it) */
int
convert(File *file,
        EncaEncoding from_enc)
{
  Converter *conv;
  int extern_failed = 0;
  int err;

  if (options.verbosity_level) {
    fprintf(stderr, "%s: converting `%s': %s\n",
                    program_name, ffname_r(file->name),
                    format_request_string(from_enc, options.target_enc, 0));
  }

  /* do nothing when requested encoding is current encoding
     (`nothing' may include copying stdin to stdout) */
  if (from_enc.charset == options.target_enc.charset
      && from_enc.surface == options.target_enc.surface) {
    if (file->name != NULL)
      return ERR_OK;
    else
      return copy_and_convert(file, file, NULL);
  }

  /* try sequentially all allowed converters until we find some that can
     perform the conversion or exahust the list */
  conv = converters;
  while (conv != NULL) {
    if (options.verbosity_level > 1) {
      fprintf(stderr, "    trying to convert `%s' using %s\n",
                      ffname_r(file->name), conv->conv->name);
    }
    err = ((ConverterData *)conv->conv->data)->convfunc(file, from_enc);
    if (err == ERR_OK)
      return ERR_OK;

    if ((((ConverterData *)conv->conv->data)->flags & CONV_EXTERN) != 0) {
      fprintf(stderr, "%s: external converter failed on `%s', "
                      "probably destroying it\n",
                      program_name, ffname_w(file->name));
      extern_failed = 1;
    }
    /* don't tempt fate in case of i/o or other serious problems */
    if (err != ERR_CANNOT)
      return ERR_IOFAIL;

    conv = conv->next;
  }

  /* no converter able/allowed to perform given conversion, that's bad */
  fprintf(stderr, "%s: no converter is able/allowed to perform "
                  "conversion %s on file `%s'\n",
                  program_name,
                  format_request_string(from_enc, options.target_enc, 0),
                  ffname_r(file->name));

  /* nevertheless stdin should be copied to stdout anyway it cannot make
     more mess */
  if (file->name == NULL)
    copy_and_convert(file, file, NULL);

  return ERR_CANNOT;
}

/* built-in converter
   performs conversion by in place modification of file named fname
   or by calling copy_and_convert() for stdin -> stdout conversion
   returns zero on success, error code otherwise */
static int
convert_builtin(File *file,
                EncaEncoding from_enc)
{
  static int ascii = ENCA_CS_UNKNOWN;

  Buffer *buf; /* file->buffer alias */
  const byte *xlat; /* conversion table */

  if (!enca_charset_is_known(ascii)) {
    ascii = enca_name_to_charset("ascii");
    assert(enca_charset_is_known(ascii));
  }

  /* surfaces can cause fail iff user specificaly requested some
   * or when they are other type than EOLs */
  {
    EncaSurface srf = options.target_enc.surface ^ from_enc.surface;

    if ((options.target_enc.surface
         && from_enc.surface != options.target_enc.surface)
         || srf != (srf & ENCA_SURFACE_MASK_EOL)) {
      if (options.verbosity_level > 2)
        fprintf(stderr, "%s: built-in: cannot convert between "
                "different surfaces\n",
                program_name);
      return ERR_CANNOT;
    }
  }

  /* catch trivial conversions */
  {
    int identity = 0;

    if (from_enc.charset == options.target_enc.charset)
      identity = 1;

    if (from_enc.charset == ascii
        && enca_charset_is_8bit(options.target_enc.charset)
        && !enca_charset_is_binary(options.target_enc.charset))
      identity = 1;

    if (identity) {
      if (file->name == NULL)
        return copy_and_convert(file, file, NULL);
      else
        return ERR_OK;
    }
  }

  xlat = xtable(from_enc.charset);
  if (xlat == NULL)
    return ERR_CANNOT;

  if (file->name == NULL)
    return copy_and_convert(file, file, xlat);

  /* read buffer_size bytes, convert, write back, etc. to death (or eof,
     whichever come first) */
  buf = file->buffer;
  buf->pos = 0;
  file_seek(file, 0, SEEK_SET);

  do {
    if (file_read(file) == -1)
      return ERR_IOFAIL;

    if (buf->pos == 0)
      break;

    {
      size_t len = buf->pos;
      byte *p = buf->data;
      do {
        *p = xlat[*p];
        p++;
      } while (--len);
    }

    if (file_seek(file, -(buf->pos), SEEK_CUR) == -1)
      return ERR_IOFAIL;

    if (file_write(file) == -1)
      return ERR_IOFAIL;

    /* XXX: apparent no-op
       but ISO C requires fseek() or ftell() between subsequent fwrite() and
       fread(), or else the latter _may_ read nonsense -- and it actually does
       read nonsense with glibc-2.2 (at least); see fopen(3) */
    if (file_seek(file, 0, SEEK_CUR) == -1)
      return ERR_IOFAIL;

  } while (1);

  return ERR_OK;
}

/* copy file file_from to file file_to, optionally performing xlat conversion
   (if not NULL)
   file_from has to be already opened for reading,
   file_to has to be already opened for writing
   they have to share common buffer
   returns 0 on success, nonzero on failure */
int
copy_and_convert(File *file_from, File *file_to, const byte *xlat)
{
  Buffer *buf; /* file_from->buffer alias */

  if (xlat == NULL && options.verbosity_level > 3)
    fprintf(stderr, "    copying `%s' to `%s'\n",
                    ffname_r(file_from->name),
                    ffname_w(file_to->name));

  assert(file_from->buffer == file_to->buffer);
  buf = file_from->buffer;
  /* If there's something in the buffer, process it first. */
  if (file_from->buffer->pos != 0) {
    if (xlat != NULL) {
      size_t len = buf->pos;
      byte *p = buf->data;
      do {
        *p = xlat[*p];
        p++;
      } while (--len);
    }
    if (file_write(file_to) == -1)
      return ERR_IOFAIL;
  }
  /* Then copy the rest. */
  do {
    if (file_read(file_from) == -1)
      return ERR_IOFAIL;

    if (buf->pos == 0)
      break;

    if (xlat != NULL) {
      size_t len = buf->pos;
      byte *p = buf->data;
      do {
        *p = xlat[*p];
        p++;
      } while (--len);
    }

    if (file_write(file_to) == -1)
      return ERR_IOFAIL;
  } while (1);
  fflush(file_to->stream);

  return ERR_OK;
}

/* add converter to list of converters
   (note `none' adds nothing and causes removing of all converters instead)
   returns zero if everything went ok, nonzero otherwise */
int
add_converter(const char *cname)
{
  /* no converters symbolic name */
  static const char *CONVERTER_NAME_NONE = "none";

  const Abbreviation *data;
  Converter *conv = NULL, *conv1;

  /* remove everything when we got `none' */
  if (strcmp(CONVERTER_NAME_NONE, cname) == 0) {
    if (options.verbosity_level > 3)
      fprintf(stderr, "Removing all converters\n");
    while (converters != NULL) {
      conv = converters->next;
      enca_free(converters);
      converters = conv;
    }
    return 0;
  }

  /* find converter data */
  data = expand_abbreviation(cname, CONVERTERS, ELEMENTS(CONVERTERS),
                             "converter");
  if (data == NULL)
    return 1;

  /* add it to the end of converter list */
  if (options.verbosity_level > 3)
    fprintf(stderr, "Adding converter `%s'\n", data->name);
  if (converters == NULL)
    converters = conv = NEW(Converter, 1);
  else {
    for (conv1 = converters; conv1 != NULL; conv1 = conv1->next) {
      /* reject duplicities */
      if (data == conv1->conv->data) {
        fprintf(stderr, "%s: converter %s specified more than once\n",
                       program_name,
                       conv1->conv->name);
        return 1;
      }
      conv = conv1;
    }

    conv->next = NEW(Converter, 1);
    conv = conv->next;
  }
  conv->next = NULL;
  conv->conv = data;

  return 0;
}

/* return nonzero if the list contains external converter */
int
external_converter_listed(void)
{
  Converter *conv;

  for (conv = converters; conv; conv = conv->next) {
    if (((ConverterData*)conv->conv->data)->flags & CONV_EXTERN)
      return 1;
  }

  return 0;
}

/* print white separated list of all valid converter names */
void
print_converter_list(void)
{
  size_t i;

  for (i = 0; i < sizeof(CONVERTERS)/sizeof(Abbreviation); i++)
    printf("%s\n", CONVERTERS[i].name);
}

/* create and return request string for conversion from e1 to e2
   filters out natrual surfaces || mask
   is NOT thread-safe
   returned string must NOT be freed and must be cosidered volatile */
const char*
format_request_string(EncaEncoding e1,
                      EncaEncoding e2,
                      EncaSurface mask)
{
  static char *s = NULL;
  char *p, *q;
  const char *e2_name, *e1_name;

  enca_free(s);
  /* build s sequentially since value returned by surface_name() is lost
     by the second call */
  e1_name = enca_charset_name(e1.charset, ENCA_NAME_STYLE_ENCA);
  p = enca_get_surface_name(e1.surface
                            & ~(enca_charset_natural_surface(e1.charset)
                                | mask),
                            ENCA_NAME_STYLE_ENCA);
  if (!enca_charset_is_known(e2.charset)) {
    q = enca_strdup("");
    e2_name = options.target_enc_str;
  }
  else {
    q = enca_get_surface_name(e2.surface
                              & ~(enca_charset_natural_surface(e2.charset)
                                  | mask),
                              ENCA_NAME_STYLE_ENCA);
    e2_name = enca_charset_name(e2.charset, ENCA_NAME_STYLE_ENCA);
  }

  s = enca_strconcat(e1_name, p, "..", e2_name, q, NULL);

  enca_free(p);
  enca_free(q);

  return s;
}

/**
 * xtable:
 * @from_charset: Charset id for which the conversion table should be returned.
 *
 * Returns translation table from charset @from to (global) target charset.
 *
 * The returned table must be considered constant and must NOT be freed.
 *
 * Only conversion between charsets of one language is supported.  We assume
 * a language contains all known charsets usable for represenation of texts,
 * so other charsets are taken as incompatible.
 *
 * Globals used: options.target_enc.charset, options.language.
 *
 * Returns: The conversion table [0x100]; #NULL on failure.
 **/
static const byte*
xtable(int from_charset)
{
  static int xtable_initialized = 0;

  unsigned int from_map[0x100];
  size_t i;
  ssize_t fidx;

  if (!enca_charset_has_ucs2_map(options.target_enc.charset)
      || !enca_charset_has_ucs2_map(from_charset))
    return NULL;

  /* Initialize when we are called the first time. */
  if (!xtable_initialized) {
    /* Allocate various tables.  Never freed. */
    xdata.charsets = enca_get_language_charsets(options.language,
                                                &xdata.ncharsets);
    assert(xdata.ncharsets > 1);
    xdata.have_table = NEW(int, xdata.ncharsets);
    xdata.tables = NEW(byte, 0x100*xdata.ncharsets);
    xdata.ucs2_map = NEW(unsigned int, 0x10000);

    for (i = 0; i < xdata.ncharsets; i++)
      xdata.have_table[i] = 0;

    /* Initialize tables to identity */
    for (i = 0; i < 0x100; i++)
      xdata.tables[i] = (byte)i;
    for (i = 1; i < xdata.ncharsets; i++)
      memcpy(xdata.tables + 0x100*i, xdata.tables, 0x100);

    /* Check whether target_charset belongs to given language */
    fidx = -1;
    for (i = 0; i < xdata.ncharsets; i++) {
      if (xdata.charsets[i] == options.target_enc.charset) {
        fidx = i;
        break;
      }
    }
    if (fidx < 0)
      return NULL;

    {
      int map_created;
      map_created = enca_charset_ucs2_map(options.target_enc.charset,
                                          xdata.target_map);
      assert(map_created);
    }
    atexit(xdata_free);
  }

  /* Check whether from_charset belongs to given language */
  fidx = -1;
  for (i = 0; i < xdata.ncharsets; i++) {
    if (xdata.charsets[i] == from_charset) {
      fidx = i;
      break;
    }
  }
  if (fidx < 0)
    return NULL;

  /* Return table if cached. */
  if (xdata.have_table[fidx])
    return xdata.tables + 0x100*fidx;

  /* Otherwise it must be generated */
  {
    int map_created;
    map_created = enca_charset_ucs2_map(from_charset, from_map);
    assert(map_created);
  }

  for (i = 0; i < 0x10000; i++)
    xdata.ucs2_map[i] = ENCA_NOT_A_CHAR;

  for (i = 0; i < 0x100; i++) {
    size_t j = 0xff - i;

    if (xdata.target_map[j] != ENCA_NOT_A_CHAR)
      xdata.ucs2_map[xdata.target_map[j]] = (unsigned int)j;
  }

  /* XXX XXX XXX XXX XXX Warning: Extreme brain damage! XXX XXX XXX XXX XXX
   * When converting to ibm866 we have to replace Belarussian/Ukrainian i/I
   * with Latin versions.  I've been told everybody expect this. */
  if (options.target_enc.charset == enca_name_to_charset("ibm866")) {
    xdata.ucs2_map[0x0406] = (byte)'I';
    xdata.ucs2_map[0x0456] = (byte)'i';
  }

  for (i = 0; i < 0x100; i++) {
    size_t j = 0xff - i;

    if (from_map[j] != ENCA_NOT_A_CHAR
        && xdata.ucs2_map[from_map[j]] != ENCA_NOT_A_CHAR)
      xdata.tables[0x100*fidx + j] = (byte)xdata.ucs2_map[from_map[j]];
  }

  return xdata.tables + 0x100*fidx;
}

static void
xdata_free(void)
{
  enca_free(xdata.charsets);
  enca_free(xdata.tables);
  enca_free(xdata.have_table);
  enca_free(xdata.ucs2_map);
}

/* vim: ts=2
 */
