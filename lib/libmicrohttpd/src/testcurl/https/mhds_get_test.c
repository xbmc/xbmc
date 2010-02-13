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
 * @file mhds_get_test.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"

#include <limits.h>
#include <sys/stat.h>

#include "gnutls.h"
#include <curl/curl.h>

#include "tls_test_common.h"

int curl_check_version (const char *req_version, ...);
extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];
extern const char srv_signed_cert_pem[];
extern const char srv_signed_key_pem[];

static int
test_cipher_option (FILE * test_fd, char *cipher_suite, int proto_version)
{

  int ret;
  int ciper[] = { MHD_GNUTLS_CIPHER_3DES_CBC, 0 };
  struct MHD_Daemon *d;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, 42433,
                        NULL, NULL, &http_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                        MHD_OPTION_CIPHER_ALGORITHM, ciper, MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, MHD_E_SERVER_INIT);
      return -1;
    }

  ret = test_https_transfer (test_fd, cipher_suite, proto_version);

  MHD_stop_daemon (d);
  return ret;
}

/* perform a HTTP GET request via SSL/TLS */
int
test_secure_get (FILE * test_fd, char *cipher_suite, int proto_version)
{
  int ret;
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, 42433,
                        NULL, NULL, &http_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_signed_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, MHD_E_SERVER_INIT);
      return -1;
    }

  ret = test_https_transfer (test_fd, cipher_suite, proto_version);

  MHD_stop_daemon (d);
  return ret;
}

int
main (int argc, char *const *argv)
{
  FILE *test_fd;
  unsigned int errorCount = 0;

  /* gnutls_global_set_log_level(11); */

  if (curl_check_version (MHD_REQ_CURL_VERSION, MHD_REQ_CURL_OPENSSL_VERSION))
    {
      return -1;
    }

  if ((test_fd = setup_test_file ()) == NULL)
    {
      fprintf (stderr, MHD_E_TEST_FILE_CREAT);
      return -1;
    }

  if (0 != curl_global_init (CURL_GLOBAL_ALL))
    {
      fprintf (stderr, "Error: %s\n", strerror (errno));
      fclose (test_fd);
      return -1;
    }

  errorCount +=
    test_secure_get (test_fd, "AES256-SHA", CURL_SSLVERSION_TLSv1);
  errorCount +=
    test_secure_get (test_fd, "AES256-SHA", CURL_SSLVERSION_SSLv3);
  errorCount +=
    test_cipher_option (test_fd, "DES-CBC3-SHA", CURL_SSLVERSION_TLSv1);

  print_test_result (errorCount, argv[0]);

  curl_global_cleanup ();
  fclose (test_fd);
  remove (TEST_FILE_NAME);

  return errorCount != 0;
}
