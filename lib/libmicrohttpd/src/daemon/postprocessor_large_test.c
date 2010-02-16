/*
     This file is part of libmicrohttpd
     (C) 2008 Christian Grothoff

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
 * @file postprocessor_large_test.c
 * @brief  Testcase with very large input for postprocessor
 * @author Christian Grothoff
 */

#include "platform.h"
#include "microhttpd.h"
#include "internal.h"

#ifndef WINDOWS
#include <unistd.h>
#endif

static int
value_checker (void *cls,
               enum MHD_ValueKind kind,
               const char *key,
               const char *filename,
               const char *content_type,
               const char *transfer_encoding,
               const char *data, uint64_t off, size_t size)
{
  unsigned int *pos = cls;
#if 0
  fprintf (stderr,
           "VC: %llu %u `%s' `%s' `%s' `%s' `%.*s'\n",
           off, size,
           key, filename, content_type, transfer_encoding, size, data);
#endif
  if (size == 0)
    return MHD_YES;
  *pos += size;
  return MHD_YES;

}


static int
test_simple_large ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  int i;
  int delta;
  size_t size;
  char data[102400];
  unsigned int pos;

  pos = 0;
  memset (data, 'A', sizeof (data));
  memcpy (data, "key=", 4);
  data[sizeof (data) - 1] = '\0';
  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = MHD_HTTP_POST_ENCODING_FORM_URLENCODED;
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection, 1024, &value_checker, &pos);
  i = 0;
  size = strlen (data);
  while (i < size)
    {
      delta = 1 + RANDOM () % (size - i);
      MHD_post_process (pp, &data[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (pos != sizeof (data) - 5) /* minus 0-termination and 'key=' */
    return 1;
  return 0;
}

int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  errorCount += test_simple_large ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  return errorCount != 0;       /* 0 == pass */
}
