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
 * @file tls_extension_test.c
 * @brief: test daemon response to TLS client hello requests containing extensions
 *
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include "gnutls_int.h"
#include "gnutls_handshake.h"   // MHD_gtls_send_handshake
#include "gnutls_num.h"         // MHD_gtls_write_x
#include "common.h"             // MHD_gtls_version_x


#include "tls_test_common.h"
#define MAX_EXT_DATA_LENGTH 256

extern int
MHD__gnutls_copy_ciphersuites (MHD_gtls_session_t session,
                               opaque * ret_data, size_t ret_data_size);

extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

/**
 * Test daemon response to TLS client hello requests containing extensions
 *
 * @param session
 * @param exten_t - the type of extension being appended to client hello request
 * @param ext_count - the number of consecutive extension replicas inserted into request
 * @param ext_length - the length of each appended extension
 * @return 0 on successful test completion, -1 otherwise
 */
static int
test_hello_extension (MHD_gtls_session_t session, extensions_t exten_t,
                      int ext_count, int ext_length)
{
  int i, sd, ret = 0, pos = 0;
  int exten_data_len, ciphersuite_len, datalen;
  struct sockaddr_in sa;
  char url[255];
  opaque *data = NULL;
  uint8_t session_id_len = 0;
  opaque rnd[TLS_RANDOM_SIZE];
  opaque extdata[MAX_EXT_DATA_LENGTH];

  /* single, null compression */
  unsigned char comp[] = { 0x01, 0x00 };
  struct CBC cbc;

  sd = -1;
  memset (&cbc, 0, sizeof (struct CBC));
  if (NULL == (cbc.buf = malloc (sizeof (char) * 256)))
    {
      fprintf (stderr, MHD_E_MEM);
      ret = -1;
      goto cleanup;
    }
  cbc.size = 256;

  sd = socket (AF_INET, SOCK_STREAM, 0);
  if (sd == -1)
    {
      fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
      free (cbc.buf);
      return -1;
    }
  memset (&sa, '\0', sizeof (struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_port = htons (DEAMON_TEST_PORT);
  inet_pton (AF_INET, "127.0.0.1", &sa.sin_addr);

  enum MHD_GNUTLS_Protocol hver;

  /* init hash functions */
  session->internals.handshake_mac_handle_md5 =
    MHD_gtls_hash_init (MHD_GNUTLS_MAC_MD5);
  session->internals.handshake_mac_handle_sha =
    MHD_gtls_hash_init (MHD_GNUTLS_MAC_SHA1);

  /* version = 2 , random = [4 for unix time + 28 for random bytes] */
  datalen = 2 /* version */ + TLS_RANDOM_SIZE + (session_id_len + 1);

  data = MHD_gnutls_malloc (datalen);

  hver = MHD_gtls_version_max (session);
  data[pos++] = MHD_gtls_version_get_major (hver);
  data[pos++] = MHD_gtls_version_get_minor (hver);

  /* Set the version we advertise as maximum (RSA uses it). */
  set_adv_version (session, MHD_gtls_version_get_major (hver),
                   MHD_gtls_version_get_minor (hver));

  session->security_parameters.version = hver;
  session->security_parameters.timestamp = time (NULL);

  /* generate session client random */
  memset (session->security_parameters.client_random, 0, TLS_RANDOM_SIZE);
  MHD_gtls_write_uint32 (time (NULL), rnd);
  if (GC_OK != MHD_gc_nonce ((char *) &rnd[4], TLS_RANDOM_SIZE - 4)) abort ();
  memcpy (session->security_parameters.client_random, rnd, TLS_RANDOM_SIZE);
  memcpy (&data[pos], rnd, TLS_RANDOM_SIZE);
  pos += TLS_RANDOM_SIZE;

  /* Copy the Session ID       */
  data[pos++] = session_id_len;

  /*
   * len = ciphersuite data + 2 bytes ciphersuite length \
   *       1 byte compression length + 1 byte compression data + \
   * 2 bytes extension length, extensions data
   */
  ciphersuite_len = MHD__gnutls_copy_ciphersuites (session, extdata,
                                                   sizeof (extdata));
  exten_data_len = ext_count * (2 + 2 + ext_length);
  datalen += ciphersuite_len + 2 + 2 + exten_data_len;
  data = MHD_gtls_realloc_fast (data, datalen);
  memcpy (&data[pos], extdata, sizeof (ciphersuite_len));
  pos += ciphersuite_len;

  /* set compression */
  memcpy (&data[pos], comp, sizeof (comp));
  pos += 2;

  /* set extensions length = 2 type bytes + 2 length bytes + extension length */
  MHD_gtls_write_uint16 (exten_data_len, &data[pos]);
  pos += 2;
  for (i = 0; i < ext_count; ++i)
    {
      /* write extension type */
      MHD_gtls_write_uint16 (exten_t, &data[pos]);
      pos += 2;
      MHD_gtls_write_uint16 (ext_length, &data[pos]);
      pos += 2;
      /* we might want to generate random data here */
      memset (&data[pos], 0, ext_length);
      pos += ext_length;
    }

  if (connect (sd, &sa, sizeof (struct sockaddr_in)) < 0)
    {
      fprintf (stderr, "%s\n", MHD_E_FAILED_TO_CONNECT);
      ret = -1;
      goto cleanup;
    }

  MHD__gnutls_transport_set_ptr (session, (MHD_gnutls_transport_ptr_t) (long) sd);

  if (gen_test_file_url (url, DEAMON_TEST_PORT))
    {
      ret = -1;
      goto cleanup;
    }

  /* this should crash the server */
  ret = MHD_gtls_send_handshake (session, data, datalen,
                                 GNUTLS_HANDSHAKE_CLIENT_HELLO);

  /* advance to STATE2 */
  session->internals.handshake_state = STATE2;
  ret = MHD__gnutls_handshake (session);
  ret = MHD__gnutls_bye (session, GNUTLS_SHUT_WR);

  MHD_gnutls_free (data);

  /* make sure daemon is still functioning */
  if (CURLE_OK != send_curl_req (url, &cbc, "AES128-SHA",
                                 MHD_GNUTLS_PROTOCOL_TLS1_2))
    {
      ret = -1;
      goto cleanup;
    }

cleanup:
  if (sd != -1)
    close (sd);
  MHD_gnutls_free (cbc.buf);
  return ret;
}

int
main (int argc, char *const *argv)
{
  int i, errorCount = 0;
  FILE *test_fd;
  struct MHD_Daemon *d;
  MHD_gtls_session_t session;
  MHD_gnutls_datum_t key;
  MHD_gnutls_datum_t cert;
  MHD_gtls_cert_credentials_t xcred;

  int ext_arr[] = { GNUTLS_EXTENSION_SERVER_NAME,
    -1
  };

  MHD_gtls_global_set_log_level (11);

  if ((test_fd = setup_test_file ()) == NULL)
    {
      fprintf (stderr, MHD_E_TEST_FILE_CREAT);
      return -1;
    }

  if (0 != curl_global_init (CURL_GLOBAL_ALL))
    {
      fprintf (stderr, "Error: %s\n", strerror (errno));
      return -1;
    }

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, DEAMON_TEST_PORT,
                        NULL, NULL, &http_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, "%s\n", MHD_E_SERVER_INIT);
      return -1;
    }

  i = 0;
  setup_session (&session, &key, &cert, &xcred);
  errorCount += test_hello_extension (session, ext_arr[i], 1, 16);
  teardown_session (session, &key, &cert, xcred);
#if 1
  i = 0;
  while (ext_arr[i] != -1)
    {
      setup_session (&session, &key, &cert, &xcred);
      errorCount += test_hello_extension (session, ext_arr[i], 1, 16);
      teardown_session (session, &key, &cert, xcred);

      setup_session (&session, &key, &cert, &xcred);
      errorCount += test_hello_extension (session, ext_arr[i], 3, 8);
      teardown_session (session, &key, &cert, xcred);

      /* this test specifically tests the issue raised in CVE-2008-1948 */
      setup_session (&session, &key, &cert, &xcred);
      errorCount += test_hello_extension (session, ext_arr[i], 6, 0);
      teardown_session (session, &key, &cert, xcred);
      i++;
    }
#endif

  print_test_result (errorCount, argv[0]);

  MHD_stop_daemon (d);

  curl_global_cleanup ();
  fclose (test_fd);

  return errorCount;
}
