/*
     This file is part of libmicrohttpd
     (C) 2009 Christian Grothoff

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
 * @file daemontest_termination.c
 * @brief  Testcase for libmicrohttpd tolerating client not closing immediately
 * @author hollosig
 */
#define PORT	12345

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <unistd.h>
#include <curl/curl.h>


static int
connection_handler (void *cls,
                    struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data, size_t * upload_data_size,
                    void **ptr)
{
  static int i;

  if (*ptr == NULL)
    {
      *ptr = &i;
      return MHD_YES;
    }

  if (*upload_data_size != 0)
    {
      (*upload_data_size) = 0;
      return MHD_YES;
    }

  struct MHD_Response *response =
    MHD_create_response_from_data (strlen ("Response"), "Response", 0, 0);
  int ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}

static size_t
write_data (void *ptr, size_t size, size_t nmemb, void *stream)
{
  return size * nmemb;
}

int
main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
                             PORT,
                             NULL,
                             NULL, connection_handler, NULL, MHD_OPTION_END);

  if (daemon == NULL)
    {
      fprintf (stderr, "Daemon cannot be started!");
      exit (1);
    }

  CURL *curl = curl_easy_init ();
  //curl_easy_setopt(curl, CURLOPT_POST, 1L);
  char url[255];
  sprintf (url, "http://localhost:%d", PORT);
  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_data);

  CURLcode success = curl_easy_perform (curl);
  if (success != 0)
    {
      fprintf (stderr, "CURL Error");
      exit (1);
    }
  /* CPU used to go crazy here */
  sleep (1);

  curl_easy_cleanup (curl);
  MHD_stop_daemon (daemon);

  return 0;
}
