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
 * @file tls_daemon_options_test.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include <sys/stat.h>
#include <limits.h>
#include "gnutls.h"

#include "tls_test_common.h"

extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

int curl_check_version (const char *req_version, ...);

/**
 * test server refuses to negotiate connections with unsupported protocol versions
 *
 */
/* TODO rm test_fd */
static int
test_unmatching_ssl_version (FILE * test_fd, char *cipher_suite,
                             int curl_req_ssl_version)
{
  struct CBC cbc;
  if (NULL == (cbc.buf = malloc (sizeof (char) * 256)))
    {
      fprintf (stderr, "Error: failed to allocate: %s\n",
               strerror (errno));
      return -1;
    }
  cbc.size = 256;
  cbc.pos = 0;

  char url[255];
  if (gen_test_file_url (url, DEAMON_TEST_PORT))
    {
      free (cbc.buf);
      return -1;
    }

  /* assert daemon *rejected* request */
  if (CURLE_OK ==
      send_curl_req (url, &cbc, cipher_suite, curl_req_ssl_version))
    {
      free (cbc.buf);
      return -1;
    }

  free (cbc.buf);
  return 0;
}

/* setup a temporary transfer test file */
int
main (int argc, char *const *argv)
{
  FILE *test_fd;
  unsigned int errorCount = 0;
  unsigned int cpos;
  char test_name[64];

  int daemon_flags =
    MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL | MHD_USE_DEBUG;

  if (curl_check_version (MHD_REQ_CURL_VERSION))
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
      fclose (test_fd);
      remove (TEST_FILE_NAME);
      fprintf (stderr, "Error: %s\n", strerror (errno));
      return -1;
    }

  int p_ssl3[] = { MHD_GNUTLS_PROTOCOL_SSL3, 0 };
  int p_tls[] = { MHD_GNUTLS_PROTOCOL_TLS1_2,
    MHD_GNUTLS_PROTOCOL_TLS1_1,
    MHD_GNUTLS_PROTOCOL_TLS1_0, 0
  };

  struct CipherDef ciphers[] = {
    {{MHD_GNUTLS_CIPHER_AES_128_CBC, 0}, "AES128-SHA"},
    {{MHD_GNUTLS_CIPHER_ARCFOUR_128, 0}, "RC4-SHA"},
    {{MHD_GNUTLS_CIPHER_3DES_CBC, 0}, "3DES-SHA"},
    {{MHD_GNUTLS_CIPHER_AES_256_CBC, 0}, "AES256-SHA"},
    {{0, 0}, NULL}
  };
  fprintf (stderr, "SHA/TLS tests:\n");
  cpos = 0;
  while (ciphers[cpos].curlname != NULL)
    {
      sprintf (test_name, "%s-TLS", ciphers[cpos].curlname);
      errorCount +=
        test_wrap (test_name,
                   &test_https_transfer, test_fd, daemon_flags,
                   ciphers[cpos].curlname,
                   CURL_SSLVERSION_TLSv1,
                   MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                   MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                   MHD_OPTION_PROTOCOL_VERSION, p_tls,
                   MHD_OPTION_CIPHER_ALGORITHM, ciphers[cpos].options,
                   MHD_OPTION_END);
      cpos++;
    }
  fprintf (stderr, "SHA/SSL3 tests:\n");
  cpos = 0;
  while (ciphers[cpos].curlname != NULL)
    {
      sprintf (test_name, "%s-SSL3", ciphers[cpos].curlname);
      errorCount +=
        test_wrap (test_name,
                   &test_https_transfer, test_fd, daemon_flags,
                   ciphers[cpos].curlname,
                   CURL_SSLVERSION_SSLv3,
                   MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                   MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                   MHD_OPTION_PROTOCOL_VERSION, p_ssl3,
                   MHD_OPTION_CIPHER_ALGORITHM, ciphers[cpos].options,
                   MHD_OPTION_END);
      cpos++;
    }
#if 0
  /* manual inspection of the handshake suggests that CURL will
     request TLSv1, we send back "SSL3" and CURL takes it *despite*
     being configured to speak SSL3-only.  Notably, the other way
     round (have curl request SSL3, respond with TLSv1 only)
     is properly refused by CURL.  Either way, this does NOT seem
     to be a bug in MHD/gnuTLS but rather in CURL; hence this
     test is commented out here... */
  errorCount +=
    test_wrap ("unmatching version: SSL3 vs. TLS", &test_unmatching_ssl_version,
               test_fd, daemon_flags, "AES256-SHA", CURL_SSLVERSION_TLSv1,
               MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
               MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
               MHD_OPTION_PROTOCOL_VERSION, p_ssl3, MHD_OPTION_END);
#endif
  errorCount +=
    test_wrap ("unmatching version: TLS vs. SSL3", &test_unmatching_ssl_version,
               test_fd, daemon_flags, "AES256-SHA", CURL_SSLVERSION_SSLv3,
               MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
               MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
               MHD_OPTION_PROTOCOL_VERSION, p_tls, MHD_OPTION_END);
  curl_global_cleanup ();
  fclose (test_fd);
  remove (TEST_FILE_NAME);

  return errorCount != 0;
}
