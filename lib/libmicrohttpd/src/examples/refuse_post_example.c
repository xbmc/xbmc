/*
     This file is part of libmicrohttpd
     (C) 2007, 2008 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file refuse_post_example.c
 * @brief example for how to refuse a POST request properly
 * @author Christian Grothoff and Sebastian Gerhardt
 */
#include "platform.h"
#include <microhttpd.h>

const char *askpage = "<html><body>\n\
                       Upload a file, please!<br>\n\
                       <form action=\"/filepost\" method=\"post\" enctype=\"multipart/form-data\">\n\
                       <input name=\"file\" type=\"file\">\n\
                       <input type=\"submit\" value=\" Send \"></form>\n\
                       </body></html>";

#define BUSYPAGE "<html><head><title>Webserver busy</title></head><body>We are too busy to process POSTs right now.</body></html>"

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  static int aptr;
  const char *me = cls;
  struct MHD_Response *response;
  int ret;

  if ((0 != strcmp (method, "GET")) && (0 != strcmp (method, "POST")))
    return MHD_NO;              /* unexpected method */

  if (&aptr != *ptr)
    {
      *ptr = &aptr;

      /* always to busy for POST requests */
      if (0 == strcmp (method, "POST"))
        {
          response = MHD_create_response_from_data (strlen (BUSYPAGE),
                                                    (void *) BUSYPAGE, MHD_NO,
                                                    MHD_NO);
          ret =
            MHD_queue_response (connection, MHD_HTTP_SERVICE_UNAVAILABLE,
                                response);
          MHD_destroy_response (response);
          return ret;
        }
    }

  *ptr = NULL;                  /* reset when done */
  response = MHD_create_response_from_data (strlen (me),
                                            (void *) me, MHD_NO, MHD_NO);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;

  if (argc != 3)
    {
      printf ("%s PORT SECONDS-TO-RUN\n", argv[0]);
      return 1;
    }
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
                        atoi (argv[1]),
                        NULL, NULL, &ahc_echo, (void *) askpage,
                        MHD_OPTION_END);
  if (d == NULL)
    return 1;
  sleep (atoi (argv[2]));
  MHD_stop_daemon (d);
  return 0;
}

/* end of refuse_post_example.c */
