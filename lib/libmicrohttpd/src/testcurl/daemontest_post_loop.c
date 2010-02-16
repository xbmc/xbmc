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
 * @file daemontest_post_loop.c
 * @brief  Testcase for libmicrohttpd POST operations using URL-encoding
 * @author Christian Grothoff (inspired by bug report #1296)
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

#define POST_DATA "<?xml version='1.0' ?>\n<xml>\n<data-id>1</data-id>\n</xml>\n"

#define LOOPCOUNT 10

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
          void **mptr)
{
  static int marker;
  struct MHD_Response *response;
  int ret;

  if (0 != strcmp ("POST", method))
    {
      printf ("METHOD: %s\n", method);
      return MHD_NO;            /* unexpected method */
    }
  if ((*mptr != NULL) && (0 == *upload_data_size))
    {
      if (*mptr != &marker)
        abort ();
      response = MHD_create_response_from_data (2, "OK", MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
      *mptr = NULL;
      return ret;
    }
  if (strlen (POST_DATA) != *upload_data_size)
    return MHD_YES;
  *upload_data_size = 0;
  *mptr = &marker;
  return MHD_YES;
}


static int
testInternalPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                        1080, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_END);
  if (d == NULL)
    return 1;
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      sprintf (url, "http://localhost:1080/hw%d", i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      if (CURLE_OK != (errornum = curl_easy_perform (c)))
        {
          fprintf (stderr,
                   "curl_easy_perform failed: `%s'\n",
                   curl_easy_strerror (errornum));
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 2;
        }
      curl_easy_cleanup (c);
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 4;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testMultithreadedPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
                        1081, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_END);
  if (d == NULL)
    return 16;
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      sprintf (url, "http://localhost:1081/hw%d", i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      if (CURLE_OK != (errornum = curl_easy_perform (c)))
        {
          fprintf (stderr,
                   "curl_easy_perform failed: `%s'\n",
                   curl_easy_strerror (errornum));
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 32;
        }
      curl_easy_cleanup (c);
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 64;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testMultithreadedPoolPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                        1081, NULL, NULL, &ahc_echo, NULL,
                        MHD_OPTION_THREAD_POOL_SIZE, 4, MHD_OPTION_END);
  if (d == NULL)
    return 16;
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      sprintf (url, "http://localhost:1081/hw%d", i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      if (CURLE_OK != (errornum = curl_easy_perform (c)))
        {
          fprintf (stderr,
                   "curl_easy_perform failed: `%s'\n",
                   curl_easy_strerror (errornum));
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 32;
        }
      curl_easy_cleanup (c);
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 64;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testExternalPost ()
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
  struct CURLMsg *msg;
  time_t start;
  struct timeval tv;
  int i;
  unsigned long long timeout;
  long ctimeout;
  char url[1024];

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_DEBUG,
                        1082, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_END);
  if (d == NULL)
    return 256;
  multi = curl_multi_init ();
  if (multi == NULL)
    {
      MHD_stop_daemon (d);
      return 512;
    }
  for (i = 0; i < LOOPCOUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      sprintf (url, "http://localhost:1082/hw%d", i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      if (oneone)
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
        curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 15L);
      // NOTE: use of CONNECTTIMEOUT without also
      //   setting NOSIGNAL results in really weird
      //   crashes on my system!
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
      mret = curl_multi_add_handle (multi, c);
      if (mret != CURLM_OK)
        {
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 1024;
        }
      start = time (NULL);
      while ((time (NULL) - start < 5) && (multi != NULL))
        {
          max = 0;
          FD_ZERO (&rs);
          FD_ZERO (&ws);
          FD_ZERO (&es);
          while (CURLM_CALL_MULTI_PERFORM ==
                 curl_multi_perform (multi, &running));
          mret = curl_multi_fdset (multi, &rs, &ws, &es, &max);
          if (mret != CURLM_OK)
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              MHD_stop_daemon (d);
              return 2048;
            }
          if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              MHD_stop_daemon (d);
              return 4096;
            }
          if (MHD_NO == MHD_get_timeout (d, &timeout))
            timeout = 100;      /* 100ms == INFTY -- CURL bug... */
          if ((CURLM_OK == curl_multi_timeout (multi, &ctimeout)) &&
              (ctimeout < timeout) && (ctimeout >= 0))
            timeout = ctimeout;
          tv.tv_sec = timeout / 1000;
          tv.tv_usec = (timeout % 1000) * 1000;
          select (max + 1, &rs, &ws, &es, &tv);
          while (CURLM_CALL_MULTI_PERFORM ==
                 curl_multi_perform (multi, &running));
          if (running == 0)
            {
              msg = curl_multi_info_read (multi, &running);
              if (msg == NULL)
                break;
              if (msg->msg == CURLMSG_DONE)
                {
                  if (msg->data.result != CURLE_OK)
                    printf ("%s failed at %s:%d: `%s'\n",
                            "curl_multi_perform",
                            __FILE__,
                            __LINE__, curl_easy_strerror (msg->data.result));
                  curl_multi_remove_handle (multi, c);
                  curl_easy_cleanup (c);
                  c = NULL;
                }
            }
          MHD_run (d);
        }
      if (c != NULL)
        {
          curl_multi_remove_handle (multi, c);
          curl_easy_cleanup (c);
        }
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          curl_multi_cleanup (multi);
          MHD_stop_daemon (d);
          return 8192;
        }
    }
  curl_multi_cleanup (multi);
  MHD_stop_daemon (d);
  fprintf (stderr, "\n");
  return 0;
}



int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  oneone = NULL != strstr (argv[0], "11");
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  errorCount += testInternalPost ();
  errorCount += testMultithreadedPost ();
  errorCount += testMultithreadedPoolPost ();
  errorCount += testExternalPost ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
