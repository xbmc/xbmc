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
 * @file mhds_session_info_test.c
 * @brief  Testcase for libmicrohttpd HTTPS connection querying operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include <curl/curl.h>

#include "tls_test_common.h"

extern int curl_check_version (const char *req_version, ...);
extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

struct MHD_Daemon *d;

/*
 * HTTP access handler call back
 * used to query negotiated security parameters
 */
static int
query_session_ahc (void *cls, struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *upload_data, const char *version,
                   size_t *upload_data_size, void **ptr)
{
  struct MHD_Response *response;
  int ret;

  /* assert actual connection cipher is the one negotiated */
  if (MHD_get_connection_info
      (connection,
       MHD_CONNECTION_INFO_CIPHER_ALGO)->cipher_algorithm !=
      MHD_GNUTLS_CIPHER_AES_256_CBC)
    {
      fprintf (stderr, "Error: requested cipher mismatch. %s\n",
               strerror (errno));
      return -1;
    }

  if (MHD_get_connection_info
      (connection,
       MHD_CONNECTION_INFO_PROTOCOL)->protocol != MHD_GNUTLS_PROTOCOL_SSL3)
    {
      fprintf (stderr, "Error: requested compression mismatch. %s\n",
               strerror (errno));
      return -1;
    }

  response = MHD_create_response_from_data (strlen (EMPTY_PAGE),
                                            (void *) EMPTY_PAGE,
                                            MHD_NO, MHD_NO);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}

/*
 * negotiate a secure connection with server & query negotiated security parameters
 */
static int
test_query_session ()
{
  CURL *c;
  struct CBC cbc;
  CURLcode errornum;
  char url[256];

  if (NULL == (cbc.buf = malloc (sizeof (char) * 255)))
    return 16;
  cbc.size = 255;
  cbc.pos = 0;

  gen_test_file_url (url, DEAMON_TEST_PORT);

  /* setup test */
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, DEAMON_TEST_PORT,
                        NULL, NULL, &query_session_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    return 2;

  c = curl_easy_init ();
#if DEBUG_HTTPS_TEST
  curl_easy_setopt (c, CURLOPT_VERBOSE, 1);
#endif
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_FILE, &cbc);
  /* TLS options */
  curl_easy_setopt (c, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
  curl_easy_setopt (c, CURLOPT_SSL_CIPHER_LIST, "AES256-SHA");
  /* currently skip any peer authentication */
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYHOST, 0);

  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);

  // NOTE: use of CONNECTTIMEOUT without also
  //   setting NOSIGNAL results in really weird
  //   crashes on my system!
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr, "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));

      MHD_stop_daemon (d);
      curl_easy_cleanup (c);
      free (cbc.buf);
      return -1;
    }

  MHD_stop_daemon (d);
  curl_easy_cleanup (c);
  free (cbc.buf);
  return 0;
}

int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  if (curl_check_version (MHD_REQ_CURL_VERSION))
    {
      return -1;
    }

  if (0 != curl_global_init (CURL_GLOBAL_ALL))
    {
      fprintf (stderr, "Error (code: %u)\n", errorCount);
      return -1;
    }

  errorCount += test_query_session ();

  print_test_result (errorCount, argv[0]);

  curl_global_cleanup ();

  return errorCount != 0;
}
