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
 * @file tls_authentication_test.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include <curl/curl.h>
#include <limits.h>
#include <sys/stat.h>

#include "tls_test_common.h"

extern int curl_check_version (const char *req_version, ...);
extern const char test_file_data[];

extern const char ca_key_pem[];
extern const char ca_cert_pem[];
extern const char srv_signed_cert_pem[];
extern const char srv_signed_key_pem[];

const char *ca_cert_file_name = "tmp_ca_cert.pem";

/*
 * test HTTPS transfer
 * @param test_fd: file to attempt transfering
 */
static int
test_daemon_get (FILE * test_fd, char *cipher_suite, int proto_version)
{
  CURL *c;
  struct CBC cbc;
  CURLcode errornum;
  char url[255];
  struct stat statb;

  stat (TEST_FILE_NAME, &statb);

  int len = statb.st_size;

  /* used to memcmp local copy & deamon supplied copy */
  unsigned char *mem_test_file_local;

  if (NULL == (mem_test_file_local = malloc (len)))
    {
      fprintf (stderr, MHD_E_MEM);
      return -1;
    }

  fseek (test_fd, 0, SEEK_SET);
  if (fread (mem_test_file_local, sizeof (char), len, test_fd) != len)
    {
      fprintf (stderr, "Error: failed to read test file. %s\n",
               strerror (errno));
      free (mem_test_file_local);
      return -1;
    }

  if (NULL == (cbc.buf = malloc (sizeof (char) * len)))
    {
      fprintf (stderr, MHD_E_MEM);
      free (mem_test_file_local);
      return -1;
    }
  cbc.size = len;
  cbc.pos = 0;

  /* construct url - this might use doc_path */
  gen_test_file_url (url, DEAMON_TEST_PORT);

  c = curl_easy_init ();
#if DEBUG_HTTPS_TEST
  curl_easy_setopt (c, CURLOPT_VERBOSE, 1);
#endif
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_FILE, &cbc);

  /* TLS options */
  curl_easy_setopt (c, CURLOPT_SSLVERSION, proto_version);
  curl_easy_setopt (c, CURLOPT_SSL_CIPHER_LIST, cipher_suite);

  /* perform peer authentication */
  /* TODO merge into send_curl_req */
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYPEER, 1);
  curl_easy_setopt (c, CURLOPT_CAINFO, ca_cert_file_name);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYHOST, 0);

  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);

  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);

  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr, "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      free (mem_test_file_local);
      free (cbc.buf);
      return errornum;
    }

  curl_easy_cleanup (c);

  if (memcmp (cbc.buf, mem_test_file_local, len) != 0)
    {
      fprintf (stderr, "Error: local file & received file differ.\n");
      free (cbc.buf);
      free (mem_test_file_local);
      return -1;
    }

  free (mem_test_file_local);
  free (cbc.buf);
  return 0;
}

/* perform a HTTP GET request via SSL/TLS */
static int
test_secure_get (FILE * test_fd, char *cipher_suite, int proto_version)
{
  int ret;
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, DEAMON_TEST_PORT,
                        NULL, NULL, &http_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_signed_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, MHD_E_SERVER_INIT);
      return -1;
    }

  ret = test_daemon_get (test_fd, cipher_suite, proto_version);

  MHD_stop_daemon (d);
  return ret;
}

static FILE *
setup_ca_cert ()
{
  FILE *cert_fd;

  if (NULL == (cert_fd = fopen (ca_cert_file_name, "wb+")))
    {
      fprintf (stderr, "Error: failed to open `%s': %s\n",
               ca_cert_file_name, strerror (errno));
      return NULL;
    }
  if (fwrite (ca_cert_pem, sizeof (char), strlen (ca_cert_pem), cert_fd)
      != strlen (ca_cert_pem))
    {
      fprintf (stderr, "Error: failed to write `%s. %s'\n",
               ca_cert_file_name, strerror (errno));
      fclose (cert_fd);
      return NULL;
    }
  if (fflush (cert_fd))
    {
      fprintf (stderr, "Error: failed to flush ca cert file stream. %s\n",
               strerror (errno));
      fclose (cert_fd);
      return NULL;
    }

  return cert_fd;
}

int
main (int argc, char *const *argv)
{
  FILE *test_fd;
  unsigned int errorCount = 0;

  if (curl_check_version (MHD_REQ_CURL_VERSION))
    {
      return -1;
    }

  if ((test_fd = setup_test_file ()) == NULL || setup_ca_cert () == NULL)
    {
      fprintf (stderr, MHD_E_TEST_FILE_CREAT);
      return -1;
    }

  if (0 != curl_global_init (CURL_GLOBAL_ALL))
    {
      fprintf (stderr, "Error (code: %u)\n", errorCount);
      fclose (test_fd);
      return -1;
    }

  errorCount +=
    test_secure_get (test_fd, "AES256-SHA", CURL_SSLVERSION_TLSv1);

  print_test_result (errorCount, argv[0]);

  curl_global_cleanup ();
  fclose (test_fd);

  remove (TEST_FILE_NAME);
  remove (ca_cert_file_name);
  return errorCount != 0;
}
