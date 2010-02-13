/*
     This file is part of libmicrohttpd
     (C) 2007 Christian Grothoff

     libmicrohttpd is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 2, or (at your
     option) any later version.

     libmicrohttpd is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libmicrohttpd; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330,
     Boston, MA 02111-1307, USA.
*/

/**
 * @file postprocessor_test.c
 * @brief  Testcase for postprocessor
 * @author Christian Grothoff
 */

#include "platform.h"
#include "microhttpd.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

/**
 * Array of values that the value checker "wants".
 * Each series of checks should be terminated by
 * five NULL-entries.
 */
const char *want[] = {
#define URL_DATA "abc=def&x=5"
#define URL_START 0
  "abc", NULL, NULL, NULL, "def",
  "x", NULL, NULL, NULL, "5",
#define URL_END (URL_START + 10)
  NULL, NULL, NULL, NULL, NULL,
#define FORM_DATA "--AaB03x\r\ncontent-disposition: form-data; name=\"field1\"\r\n\r\nJoe Blow\r\n--AaB03x\r\ncontent-disposition: form-data; name=\"pics\"; filename=\"file1.txt\"\r\nContent-Type: text/plain\r\nContent-Transfer-Encoding: binary\r\n\r\nfiledata\r\n--AaB03x--\r\n"
#define FORM_START (URL_END + 5)
  "field1", NULL, NULL, NULL, "Joe Blow",
  "pics", "file1.txt", "text/plain", "binary", "filedata",
#define FORM_END (FORM_START + 10)
  NULL, NULL, NULL, NULL, NULL,
#define FORM_NESTED_DATA "--AaB03x\r\ncontent-disposition: form-data; name=\"field1\"\r\n\r\nJane Blow\r\n--AaB03x\r\ncontent-disposition: form-data; name=\"pics\"\r\nContent-type: multipart/mixed, boundary=BbC04y\r\n\r\n--BbC04y\r\nContent-disposition: attachment; filename=\"file1.txt\"\r\nContent-Type: text/plain\r\n\r\nfiledata1\r\n--BbC04y\r\nContent-disposition: attachment; filename=\"file2.gif\"\r\nContent-type: image/gif\r\nContent-Transfer-Encoding: binary\r\n\r\nfiledata2\r\n--BbC04y--\r\n--AaB03x--"
#define FORM_NESTED_START (FORM_END + 5)
  "field1", NULL, NULL, NULL, "Jane Blow",
  "pics", "file1.txt", "text/plain", NULL, "filedata1",
  "pics", "file2.gif", "image/gif", "binary", "filedata2",
#define FORM_NESTED_END (FORM_NESTED_START + 15)
  NULL, NULL, NULL, NULL, NULL,
};

static int
mismatch (const char *a, const char *b)
{
  if (a == b)
    return 0;
  if ((a == NULL) || (b == NULL))
    return 1;
  return 0 != strcmp (a, b);
}

static int
value_checker (void *cls,
               enum MHD_ValueKind kind,
               const char *key,
               const char *filename,
               const char *content_type,
               const char *transfer_encoding,
               const char *data, uint64_t off, size_t size)
{
  int *want_off = cls;
  int idx = *want_off;

#if 0
  fprintf (stderr,
           "VC: `%s' `%s' `%s' `%s' `%.*s'\n",
           key, filename, content_type, transfer_encoding, size, data);
#endif
  if (size == 0)
    return MHD_YES;
  if ((idx < 0) ||
      (want[idx] == NULL) ||
      (0 != strcmp (key, want[idx])) ||
      (mismatch (filename, want[idx + 1])) ||
      (mismatch (content_type, want[idx + 2])) ||
      (mismatch (transfer_encoding, want[idx + 3])) ||
      (0 != memcmp (data, &want[idx + 4][off], size)))
    {
      *want_off = -1;
      return MHD_NO;
    }
  if (off + size == strlen (want[idx + 4]))
    *want_off = idx + 5;
  return MHD_YES;

}


static int
test_urlencoding ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = URL_START;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = MHD_HTTP_POST_ENCODING_FORM_URLENCODED;
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (URL_DATA);
  while (i < size)
    {
      delta = 1 + RANDOM () % (size - i);
      MHD_post_process (pp, &URL_DATA[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (want_off != URL_END)
    return 1;
  return 0;
}


static int
test_multipart ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = FORM_START;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value =
    MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA ", boundary=AaB03x";
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (FORM_DATA);
  while (i < size)
    {
      delta = 1 + RANDOM () % (size - i);
      MHD_post_process (pp, &FORM_DATA[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (want_off != FORM_END)
    return 2;
  return 0;
}


static int
test_nested_multipart ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = FORM_NESTED_START;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value =
    MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA ", boundary=AaB03x";
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (FORM_NESTED_DATA);
  while (i < size)
    {
      delta = 1 + RANDOM () % (size - i);
      MHD_post_process (pp, &FORM_NESTED_DATA[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (want_off != FORM_NESTED_END)
    return 4;
  return 0;
}

int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  errorCount += test_urlencoding ();
  errorCount += test_multipart ();
  errorCount += test_nested_multipart ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  return errorCount != 0;       /* 0 == pass */
}
