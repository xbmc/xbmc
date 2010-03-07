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
 * @brief: daemon TLS cipher change message test-case
 *
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include "internal.h"
#include "gnutls_int.h"
#include "gnutls_datum.h"
#include "gnutls_record.h"

#include "tls_test_common.h"
extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

char *http_get_req = "GET / HTTP/1.1\r\n\r\n";

/* HTTP access handler call back */
static int
rehandshake_ahc (void *cls, struct MHD_Connection *connection,
                 const char *url, const char *method, const char *upload_data,
                 const char *version, size_t *upload_data_size,
                 void **ptr)
{
  int ret;
  /* server side re-handshake request */
  ret = MHD__gnutls_rehandshake (connection->tls_session);

  if (ret < 0)
    {
      fprintf (stderr, "Error: %s. f: %s, l: %d\n",
               "server failed to send Hello Request", __FUNCTION__, __LINE__);
    }

  return 0;
}

/*
 * Cipher change message should only occur while negotiating
 * the SSL/TLS handshake.
 * Test server disconnects upon receiving an out of context
 * message.
 *
 * @param session: initiallized TLS session
 */
static int
test_out_of_context_cipher_change (MHD_gtls_session_t session)
{
  int sd, ret;
  struct sockaddr_in sa;

  sd = socket (AF_INET, SOCK_STREAM, 0);
  if (sd == -1)
    {
      fprintf (stderr, "Failed to create socket: %s\n", strerror (errno));
      return -1;
    }

  memset (&sa, '\0', sizeof (struct sockaddr_in));
  sa.sin_family = AF_INET;
  sa.sin_port = htons (DEAMON_TEST_PORT);
  inet_pton (AF_INET, "127.0.0.1", &sa.sin_addr);

  MHD__gnutls_transport_set_ptr (session, (MHD_gnutls_transport_ptr_t) (long) sd);

  ret = connect (sd, &sa, sizeof (struct sockaddr_in));

  if (ret < 0)
    {
      fprintf (stderr, "%s\n", MHD_E_FAILED_TO_CONNECT);
      return -1;
    }

  ret = MHD__gnutls_handshake (session);
  if (ret < 0)
    {
      return -1;
    }

  /* send an out of context cipher change spec */
  MHD_gtls_send_change_cipher_spec (session, 0);


  /* assert server has closed connection */
  /* TODO better RST trigger */
  if (send (sd, "", 1, 0) == 0)
    {
      return -1;
    }

  close (sd);
  return 0;
}

int
main (int argc, char *const *argv)
{
  int errorCount = 0;;
  struct MHD_Daemon *d;
  MHD_gtls_session_t session;
  MHD_gnutls_datum_t key;
  MHD_gnutls_datum_t cert;
  MHD_gtls_cert_credentials_t xcred;

  MHD__gnutls_global_init ();
  MHD_gtls_global_set_log_level (11);

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_SSL |
                        MHD_USE_DEBUG, DEAMON_TEST_PORT,
                        NULL, NULL, &rehandshake_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, "%s\n", MHD_E_SERVER_INIT);
      return -1;
    }

  setup_session (&session, &key, &cert, &xcred);
  errorCount += test_out_of_context_cipher_change (session);
  teardown_session (session, &key, &cert, xcred);

  print_test_result (errorCount, argv[0]);

  MHD_stop_daemon (d);
  MHD__gnutls_global_deinit ();

  return errorCount != 0;
}
