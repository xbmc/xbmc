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
 * @file daemontest_get.c
 * @brief  Testcase for libmicrohttpd GET operations
 *         TODO: test parsing of query
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

static int oneone;

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};

static size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
  struct CBC *cbc = ctx;

  if (cbc->pos + size * nmemb > cbc->size)
    return 0;                   /* overflow */
  memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
  cbc->pos += size * nmemb;
  return size * nmemb;
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  static int ptr;
  const char *me = cls;
  struct MHD_Response *response;
  int ret;

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  response = MHD_create_response_from_data (strlen (url),
                                            (void *) url, MHD_NO, MHD_YES);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}

static int
testMultithreadedGet ()
{
  struct MHD_Daemon *d;
  char buf[2048];
  int k;

  /* Test only valid for HTTP/1.1 (uses persistent connections) */
  if (!oneone)
    return 0;

  d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                        1081, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;

  for (k = 0; k < 3; ++k)
    {
      struct CBC cbc[3];
      CURL *cenv[3];
      int i;

      for (i = 0; i < 3; ++i)
        {
          CURL *c;
          CURLcode errornum;
 
          cenv[i] = c = curl_easy_init ();
          cbc[i].buf = buf;
          cbc[i].size = 2048;
          cbc[i].pos = 0;

          curl_easy_setopt (c, CURLOPT_URL, "http://localhost:1081/hello_world");
          curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
          curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc[i]);
          curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
          curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
          curl_easy_setopt (c, CURLOPT_FORBID_REUSE, 0L);
          curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
          curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
          // NOTE: use of CONNECTTIMEOUT without also
          //   setting NOSIGNAL results in really weird
          //   crashes on my system!
          curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);

          errornum = curl_easy_perform (c);
          if ( ( (CURLE_OK != errornum) && (i <  2) ) ||
	       ( (CURLE_OK == errornum) && (i == 2) ) )
            {
              int j;

              /* First 2 should succeed */
              if (i < 2)
                fprintf (stderr,
                         "curl_easy_perform failed: `%s'\n",
                         curl_easy_strerror (errornum));

              /* Last request should have failed */
              else
                fprintf (stderr,
                         "No error on IP address over limit\n");

              for (j = 0; j < i; ++j)
                curl_easy_cleanup (cenv[j]);
              MHD_stop_daemon (d);
              return 32;
            }
        }

      /* Cleanup the environments */
      for (i = 0; i < 3; ++i)
        curl_easy_cleanup (cenv[i]);

      sleep(2);

      for (i = 0; i < 2; ++i)
        {
          if (cbc[i].pos != strlen ("/hello_world"))
            {
              MHD_stop_daemon (d);
              return 64;
            }
          if (0 != strncmp ("/hello_world", cbc[i].buf, strlen ("/hello_world")))
            {
              MHD_stop_daemon (d);
              return 128;
            }
        }


    }
  MHD_stop_daemon (d);
  return 0;
}

static int
testMultithreadedPoolGet ()
{
  struct MHD_Daemon *d;
  char buf[2048];
  int k;

  /* Test only valid for HTTP/1.1 (uses persistent connections) */
  if (!oneone)
    return 0;

  d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                        1081, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                        MHD_OPTION_THREAD_POOL_SIZE, 4,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;

  for (k = 0; k < 3; ++k)
    {
      struct CBC cbc[3];
      CURL *cenv[3];
      int i;

      for (i = 0; i < 3; ++i)
        {
          CURL *c;
          CURLcode errornum;
 
          cenv[i] = c = curl_easy_init ();
          cbc[i].buf = buf;
          cbc[i].size = 2048;
          cbc[i].pos = 0;

          curl_easy_setopt (c, CURLOPT_URL, "http://localhost:1081/hello_world");
          curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
          curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc[i]);
          curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
          curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
          curl_easy_setopt (c, CURLOPT_FORBID_REUSE, 0L);
          curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
          curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
          // NOTE: use of CONNECTTIMEOUT without also
          //   setting NOSIGNAL results in really weird
          //   crashes on my system!
          curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);

          errornum = curl_easy_perform (c);
          if ( ( (CURLE_OK != errornum) && (i <  2) ) ||
	       ( (CURLE_OK == errornum) && (i == 2) ) )
            {
              int j;

              /* First 2 should succeed */
              if (i < 2)
                fprintf (stderr,
                         "curl_easy_perform failed: `%s'\n",
                         curl_easy_strerror (errornum));

              /* Last request should have failed */
              else
                fprintf (stderr,
                         "No error on IP address over limit\n");

              for (j = 0; j < i; ++j)
                curl_easy_cleanup (cenv[j]);
              MHD_stop_daemon (d);
              return 32;
            }
        }

      /* Cleanup the environments */
      for (i = 0; i < 3; ++i)
        curl_easy_cleanup (cenv[i]);

      sleep(2);

      for (i = 0; i < 2; ++i)
        {
          if (cbc[i].pos != strlen ("/hello_world"))
            {
              MHD_stop_daemon (d);
              return 64;
            }
          if (0 != strncmp ("/hello_world", cbc[i].buf, strlen ("/hello_world")))
            {
              MHD_stop_daemon (d);
              return 128;
            }
        }


    }
  MHD_stop_daemon (d);
  return 0;
}

int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  oneone = NULL != strstr (argv[0], "11");
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  errorCount += testMultithreadedGet ();
  errorCount += testMultithreadedPoolGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
