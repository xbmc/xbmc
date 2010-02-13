/*
     This file is part of libmicrohttpd
     (C) 2007, 2008 Christian Grothoff

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

#include "socat.c"

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
testInternalGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  int i;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY /* | MHD_USE_DEBUG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 1;
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://localhost:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      curl_easy_perform (c);
      curl_easy_cleanup (c);
    }
  fprintf (stderr, "\n");
  zzuf_socat_stop ();
  MHD_stop_daemon (d);
  return 0;
}

static int
testMultithreadedGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  int i;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION /* | MHD_USE_DEBUG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 16;
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://localhost:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      curl_easy_perform (c);
      curl_easy_cleanup (c);
    }
  fprintf (stderr, "\n");
  zzuf_socat_stop ();
  MHD_stop_daemon (d);
  return 0;
}


static int
testExternalGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLM *multi;
  CURLMcode mret;
  fd_set rs;
  fd_set ws;
  fd_set es;
  int max;
  int running;
  time_t start;
  struct timeval tv;
  int i;

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_NO_FLAG /* | MHD_USE_DEBUG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 256;
  multi = curl_multi_init ();
  if (multi == NULL)
    {
      MHD_stop_daemon (d);
      return 512;
    }
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://localhost:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      mret = curl_multi_add_handle (multi, c);
      if (mret != CURLM_OK)
        {
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          zzuf_socat_stop ();
          MHD_stop_daemon (d);
          return 1024;
        }
      start = time (NULL);
      while ((time (NULL) - start < 5) && (c != NULL))
        {
          max = 0;
          FD_ZERO (&rs);
          FD_ZERO (&ws);
          FD_ZERO (&es);
          curl_multi_perform (multi, &running);
          mret = curl_multi_fdset (multi, &rs, &ws, &es, &max);
          if (mret != CURLM_OK)
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              zzuf_socat_stop ();
              MHD_stop_daemon (d);
              return 2048;
            }
          if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              zzuf_socat_stop ();
              MHD_stop_daemon (d);
              return 4096;
            }
          tv.tv_sec = 0;
          tv.tv_usec = 1000;
          select (max + 1, &rs, &ws, &es, &tv);
          curl_multi_perform (multi, &running);
          if (running == 0)
            {
              curl_multi_info_read (multi, &running);
              curl_multi_remove_handle (multi, c);
              curl_easy_cleanup (c);
              c = NULL;
            }
          MHD_run (d);
        }
      if (c != NULL)
        {
          curl_multi_remove_handle (multi, c);
          curl_easy_cleanup (c);
        }
    }
  fprintf (stderr, "\n");
  curl_multi_cleanup (multi);
  zzuf_socat_stop ();
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
  errorCount += testInternalGet ();
  errorCount += testMultithreadedGet ();
  errorCount += testExternalGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
