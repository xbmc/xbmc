/*
     This file is part of libmicrohttpd
     (C) 2008 Christian Grothoff (and other contributing authors)

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
 * @file authorization_example.c
 * @brief example for how to use libmicrohttpd with HTTP authentication
 * @author Christian Grothoff
 */

#include "platform.h"
#include <microhttpd.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"

#define DENIED "<html><head><title>Access denied</title></head><body>Access denied</body></html>"



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
  int code;
  const char *auth;

  if (0 != strcmp (method, "GET"))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */
  auth = MHD_lookup_connection_value (connection,
                                      MHD_HEADER_KIND,
                                      MHD_HTTP_HEADER_AUTHORIZATION);
  if ((auth == NULL) ||
      (0 != strcmp (auth, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==")))
    {
      /* require: "Aladdin" with password "open sesame" */
      response = MHD_create_response_from_data (strlen (DENIED),
                                                (void *) DENIED, MHD_NO,
                                                MHD_NO);
      MHD_add_response_header (response, MHD_HTTP_HEADER_WWW_AUTHENTICATE,
                               "Basic realm=\"TestRealm\"");
      code = MHD_HTTP_UNAUTHORIZED;
    }
  else
    {
      response = MHD_create_response_from_data (strlen (me),
                                                (void *) me, MHD_NO, MHD_NO);
      code = MHD_HTTP_OK;
    }
  ret = MHD_queue_response (connection, code, response);
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
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  if (d == NULL)
    return 1;
  sleep (atoi (argv[2]));
  MHD_stop_daemon (d);
  return 0;
}
