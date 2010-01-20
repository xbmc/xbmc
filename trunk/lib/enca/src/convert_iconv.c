/*
  @(#) $Id: convert_iconv.c,v 1.19 2005/02/27 12:40:50 yeti Exp $
  interface to UNIX98 iconv conversion functions

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
#ifdef HAVE_GOOD_ICONV

#include <iconv.h>
#include "iconvenc.h"

/* Local prototypes. */
static int  iconv_one_step            (File *file_from,
                                       File *file_to,
                                       iconv_t icd);
static int  do_iconv_open             (EncaEncoding from_enc,
                                       EncaEncoding to_enc,
                                       iconv_t *icd);
static void do_iconv_close            (iconv_t icd);
static int  acceptable_surface        (EncaEncoding enc);

/* Second buffer needed for iconv(). */
static Buffer *buffer_iconv = NULL;

/* convert file using UNIX98 iconv functions
   returns 0 on success, nonzero error code otherwise
   when iconv implementation is not transitive (ICONV_TRANSITIVE is not
   defined), it may help to perform conversion via Unicode, so we try it too
   (probably UCS-2/ISO-10646, but maybe UTF-8---whatever has been detected
   at configure time) */
int
convert_iconv(File *file,
              EncaEncoding from_enc)
{
  static int ascii = ENCA_CS_UNKNOWN;
  File *tempfile = NULL;
  int err;
  iconv_t icd;

  if (!enca_charset_is_known(ascii)) {
    ascii = enca_name_to_charset("ascii");
    assert(enca_charset_is_known(ascii));
  }

  /* When iconv doesn't know the encodings, it can't convert between them.
   * We also don't try conversion to ASCII, it can only damage the files and
   * upset users, nothing else.
   * And fail early on really silly surfaces. */
  if (!enca_charset_name(from_enc.charset, ENCA_NAME_STYLE_ICONV)
      || (enca_charset_is_known(options.target_enc.charset)
          && !enca_charset_name(options.target_enc.charset,
                                ENCA_NAME_STYLE_ICONV))
      || options.target_enc.charset == ascii
      || !acceptable_surface(from_enc)
      || !acceptable_surface(options.target_enc))
    return ERR_CANNOT;

  /* Is the conversion possible? */
  if (do_iconv_open(from_enc, options.target_enc, &icd) != 0)
    return ERR_CANNOT;

  /* Since iconv doesn't recode files in place, we make a temporary file
     and copy contents of file fname to it.
     save the current content first, then copy the rest.
     When the file is stdin, fake-reopen it to stdout. */
  err = ERR_IOFAIL;
  if ((tempfile = file_temporary(file->buffer, 1))
      && file_write(tempfile) != -1
      && copy_and_convert(file, tempfile, NULL) == 0
      && (!file->name || file_seek(file, 0, SEEK_SET) == 0)
      && file_seek(tempfile, 0, SEEK_SET) == 0
      && (!file->name || file_truncate(file, 0) == 0)
      && (file->name || (file_close(file) == 0
                         && file_open(file, "wb") == 0))) {
    /* Create the second buffer when we don't have any yet
      but don't make it unnecessarily large, system default suffices */
    if (!buffer_iconv)
      buffer_iconv = buffer_new(0);
    tempfile->buffer = buffer_iconv;

    err = iconv_one_step(tempfile, file, icd);
  }

  file_free(tempfile);
  do_iconv_close(icd);
  return err;
}

/* perform one conversion step using conversion descriptor icd
   reading for file_from and putting result to file_to */
static int
iconv_one_step(File *file_from,
               File *file_to,
               iconv_t icd)
{
  size_t size_from, size_to, n;
  char *p_from, *p_to;
  int hit_eof;

  /* convert */
  do {
    /* read to io_buffer */
    if (file_read(file_from) == -1)
      return ERR_IOFAIL;

    p_from = (char*)file_from->buffer->data;
    size_from = file_from->buffer->pos;
    hit_eof = (ssize_t)file_from->buffer->size > file_from->buffer->pos;
    /* convert without reading more data until io_buffer is exhausted or some
       error occurs */
    do {
      p_to = (char*)file_to->buffer->data;
      size_to = file_to->buffer->size;
      n = iconv(icd,
                (ICONV_CONST char**)&p_from, &size_from,
                &p_to, &size_to);
      file_to->buffer->pos = file_to->buffer->size - size_to;
      if (n != (size_t)-1 || errno != E2BIG)
        break;

      if (file_write(file_to) == -1)
        return ERR_IOFAIL;

    } while (1);

    if (n == (size_t)-1) {
      /* EINVAL means some multibyte sequence has been splitted---that's ok,
         move it to the begining and go on */
      if (errno == EINVAL) {
        memmove(file_from->buffer->data, p_from, size_from);
        file_from->buffer->pos = size_from;
      }
      else {
        /* but other errors are critical, conversion and try to recover  */
        fprintf(stderr, "%s: Iconv conversion error on `%s': %s\n",
                        program_name,
                        ffname_r(file_from->name),
                        strerror(errno));
        if (file_from->name && file_to->name) {
          Buffer *buf;
          int err;

          /* regular file */
          fprintf(stderr, "Trying to recover... ");
          if (file_seek(file_from, 0, SEEK_SET) != 0
              || file_seek(file_to, 0, SEEK_SET) != 0
              || file_truncate(file_to, file_to->size) != 0) {
            fprintf(stderr, "failed\n");
            return ERR_IOFAIL;
          }
          file_from->buffer->pos = 0;
          buf = file_to->buffer;
          file_to->buffer = file_from->buffer;
          err = copy_and_convert(file_from, file_to, NULL);
          file_to->buffer = buf;

          if (err != 0) {
            fprintf(stderr, "failed\n");
            return ERR_IOFAIL;
          }
          fprintf(stderr, "succeeded.\n");
        }
        else {
          fprintf(stderr, "No way to recover in a pipe.\n");
          return ERR_IOFAIL;
        }

        return ERR_MALFORM;
      }
    }
    else file_from->buffer->pos = 0;

    /* write the remainder */
    if (file_write(file_to) == -1)
      return ERR_IOFAIL;

  } while (!hit_eof);

  /* file might end with an unfinished multibyte sequence */
  if (size_from > 0) {
    fprintf(stderr, "%s: File `%s' seems to be truncated, "
                    "the trailing incomplete multibyte sequence "
                    "has been lost\n",
                    program_name,
                    ffname_r(file_from->name));
    return ERR_MALFORM;
  }

  return ERR_OK;
}

/* try to ask for conversion from from_enc to to_enc
   returns 0 on success, nonzero on failure
   on fatal error simply aborts program */
static int
do_iconv_open(EncaEncoding from_enc,
              EncaEncoding to_enc,
              iconv_t *icd)
{
  const char *to_name, *from_name;

  if (!enca_charset_is_known(to_enc.charset))
    to_name = options.target_enc_str;
  else
    to_name = enca_charset_name(to_enc.charset, ENCA_NAME_STYLE_ICONV);
  from_name = enca_charset_name(from_enc.charset, ENCA_NAME_STYLE_ICONV);
  assert(from_name != NULL);
  assert(to_name != NULL);

  /* Iconv_open() paramters has reverse order than we use. */
  *icd = iconv_open(to_name, from_name);
  if (*icd != (iconv_t)-1)
    return 0;

  /* Failure, EINVAL means this conversion is not possible. */
  if (errno == EINVAL)
    return ERR_CANNOT;

  /* But otherwise we are in deep trouble, we've got out of memory or file
     descriptors. */
  fprintf(stderr, "%s: Aborting: %s\n",
                  program_name,
                  strerror(errno));
  exit(EXIT_TROUBLE);

  return 0;
}

/* close iconv descriptor icd
   abort if it is not possible */
static void
do_iconv_close(iconv_t icd)
{
  if (iconv_close(icd) != 0) {
    fprintf(stderr, "%s: Cannot close iconv descriptor (memory leak): %s\n",
                    program_name,
                    strerror(errno));
    exit(EXIT_TROUBLE);
  }
}

/**
 * Do we think iconv will accept given encoding as a source or target?
 *
 * This is a somewhat less strict condition than natural surface requirement.
 **/
static int
acceptable_surface(EncaEncoding enc)
{
  EncaSurface mask;

  mask = enca_charset_natural_surface(enc.charset) | ENCA_SURFACE_MASK_EOL;

  return (enc.surface & ~mask) == 0;
}

#endif /* HAVE_GOOD_ICONV */

/* vim: ts=2
 */
