/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

/* Functions that relate to the TLS handshake procedure.
 */

#include "MHD_config.h"
#include "gnutls_int.h"
#include "gnutls_errors.h"
#include "gnutls_dh.h"
#include "debug.h"
#include "gnutls_algorithms.h"
#include "gnutls_cipher.h"
#include "gnutls_buffers.h"
#include "gnutls_kx.h"
#include "gnutls_handshake.h"
#include "gnutls_num.h"
#include "gnutls_hash_int.h"
#include "gnutls_extensions.h"
#include "gnutls_supplemental.h"
#include "gnutls_auth_int.h"
#include "auth_cert.h"
#include "gnutls_cert.h"
#include "gnutls_constate.h"
#include "gnutls_record.h"
#include "gnutls_state.h"
#include "gnutls_rsa_export.h"  /* for MHD_gnutls_get_rsa_params() */
#include "gc.h"

#ifdef HANDSHAKE_DEBUG
#define ERR(x, y) MHD__gnutls_handshake_log( "HSK[%x]: %s (%d)\n", session, x,y)
#else
#define ERR(x, y)
#endif

#define TRUE 1
#define FALSE 0


/* This should be sufficient by now. It should hold all the extensions
 * plus the headers in a hello message.
 */
#define MAX_EXT_DATA_LENGTH 1024


static int MHD_gtls_remove_unwanted_ciphersuites (MHD_gtls_session_t session,
                                                  cipher_suite_st **
                                                  cipherSuites,
                                                  int numCipherSuites,
                                                  enum
                                                  MHD_GNUTLS_PublicKeyAlgorithm);
static int MHD_gtls_server_select_suite (MHD_gtls_session_t session,
                                         opaque * data, int datalen);

static int MHD_gtls_generate_session_id (opaque * session_id, uint8_t * len);

static int MHD_gtls_handshake_common (MHD_gtls_session_t session);

static int MHD_gtls_handshake_server (MHD_gtls_session_t session);

#if MHD_DEBUG_TLS
static int MHD_gtls_handshake_client (MHD_gtls_session_t session);
#endif


static int MHD__gnutls_server_select_comp_method (MHD_gtls_session_t session,
                                                  opaque * data, int datalen);


/* Clears the handshake hash buffers and handles.
 */
static void
MHD__gnutls_handshake_hash_buffers_clear (MHD_gtls_session_t session)
{
  MHD_gnutls_hash_deinit (session->internals.handshake_mac_handle_md5, NULL);
  MHD_gnutls_hash_deinit (session->internals.handshake_mac_handle_sha, NULL);
  session->internals.handshake_mac_handle_md5 = NULL;
  session->internals.handshake_mac_handle_sha = NULL;
  MHD_gtls_handshake_buffer_clear (session);
}

/**
  * gnutls_handshake_set_max_packet_length - This function will set the maximum length of a handshake message
  * @session: is a #gnutls_session_t structure.
  * @max: is the maximum number.
  *
  * This function will set the maximum size of a handshake message.
  * Handshake messages over this size are rejected.
  * The default value is 16kb which is large enough. Set this to 0 if you do not want
  * to set an upper limit.
  *
  **/
void
MHD__gnutls_handshake_set_max_packet_length (MHD_gtls_session_t session,
                                             size_t max)
{
  session->internals.max_handshake_data_buffer_size = max;
}


static void
MHD_gtls_set_server_random (MHD_gtls_session_t session, uint8_t * rnd)
{
  memcpy (session->security_parameters.server_random, rnd, TLS_RANDOM_SIZE);
}

static void
MHD_gtls_set_client_random (MHD_gtls_session_t session, uint8_t * rnd)
{
  memcpy (session->security_parameters.client_random, rnd, TLS_RANDOM_SIZE);
}

/* Calculate The SSL3 Finished message */
#define SSL3_CLIENT_MSG "CLNT"
#define SSL3_SERVER_MSG "SRVR"
#define SSL_MSG_LEN 4
static int
MHD__gnutls_ssl3_finished (MHD_gtls_session_t session, int type, opaque * ret)
{
  const int siz = SSL_MSG_LEN;
  mac_hd_t td_md5;
  mac_hd_t td_sha;
  const char *mesg;

  td_md5 = MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_md5);
  if (td_md5 == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_HASH_FAILED;
    }

  td_sha = MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_sha);
  if (td_sha == NULL)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_hash_deinit (td_md5, NULL);
      return GNUTLS_E_HASH_FAILED;
    }

  if (type == GNUTLS_SERVER)
    {
      mesg = SSL3_SERVER_MSG;
    }
  else
    {
      mesg = SSL3_CLIENT_MSG;
    }

  MHD_gnutls_hash (td_md5, mesg, siz);
  MHD_gnutls_hash (td_sha, mesg, siz);

  MHD_gnutls_mac_deinit_ssl3_handshake (td_md5, ret,
                                        session->security_parameters.
                                        master_secret, TLS_MASTER_SIZE);
  MHD_gnutls_mac_deinit_ssl3_handshake (td_sha, &ret[16],
                                        session->security_parameters.
                                        master_secret, TLS_MASTER_SIZE);

  return 0;
}

/* Hash the handshake messages as required by TLS 1.0 */
#define SERVER_MSG "server finished"
#define CLIENT_MSG "client finished"
#define TLS_MSG_LEN 15
static int
MHD__gnutls_finished (MHD_gtls_session_t session, int type, void *ret)
{
  const int siz = TLS_MSG_LEN;
  opaque concat[36];
  size_t len;
  const char *mesg;
  mac_hd_t td_md5 = NULL;
  mac_hd_t td_sha;
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

  if (ver < MHD_GNUTLS_PROTOCOL_TLS1_2)
    {
      td_md5 =
        MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_md5);
      if (td_md5 == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_HASH_FAILED;
        }
    }

  td_sha = MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_sha);
  if (td_sha == NULL)
    {
      MHD_gnutls_assert ();
      if (td_md5 != NULL)
        MHD_gnutls_hash_deinit (td_md5, NULL);
      return GNUTLS_E_HASH_FAILED;
    }

  if (ver < MHD_GNUTLS_PROTOCOL_TLS1_2)
    {
      MHD_gnutls_hash_deinit (td_md5, concat);
      MHD_gnutls_hash_deinit (td_sha, &concat[16]);
      len = 20 + 16;
    }
  else
    {
      MHD_gnutls_hash_deinit (td_sha, concat);
      len = 20;
    }

  if (type == GNUTLS_SERVER)
    {
      mesg = SERVER_MSG;
    }
  else
    {
      mesg = CLIENT_MSG;
    }

  return MHD_gtls_PRF (session, session->security_parameters.master_secret,
                       TLS_MASTER_SIZE, mesg, siz, concat, len, 12, ret);
}

/* this function will produce TLS_RANDOM_SIZE==32 bytes of random data
 * and put it to dst.
 */
static int
MHD_gtls_tls_create_random (opaque * dst)
{
  uint32_t tim;

  /* Use weak random numbers for the most of the
   * buffer except for the first 4 that are the
   * system's time.
   */

  tim = time (NULL);
  /* generate server random value */
  MHD_gtls_write_uint32 (tim, dst);

  if (MHD_gc_nonce ((char *) &dst[4], TLS_RANDOM_SIZE - 4) != GC_OK)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_RANDOM_FAILED;
    }

  return 0;
}

/* returns the 0 on success or a negative value.
 */
static int
MHD_gtls_negotiate_version (MHD_gtls_session_t session,
                            enum MHD_GNUTLS_Protocol adv_version)
{
  int ret;

  /* if we do not support that version  */
  if (MHD_gtls_version_is_supported (session, adv_version) == 0)
    {
      /* If he requested something we do not support
       * then we send him the highest we support.
       */
      ret = MHD_gtls_version_max (session);
    }
  else
    {
      ret = adv_version;
    }
  MHD_gtls_set_current_version (session, ret);

  return ret;
}

/* Read a client hello packet.
 * A client hello must be a known version client hello
 * or version 2.0 client hello (only for compatibility
 * since SSL version 2.0 is not supported).
 */
static int
MHD__gnutls_read_client_hello (MHD_gtls_session_t session, opaque * data,
                               int datalen)
{
  uint8_t session_id_len;
  int pos = 0, ret = 0;
  uint16_t suite_size, comp_size;
  enum MHD_GNUTLS_Protocol adv_version;
  int neg_version;
  int len = datalen;
  opaque rnd[TLS_RANDOM_SIZE], *suite_ptr, *comp_ptr;

  DECR_LEN (len, 2);

  MHD__gnutls_handshake_log ("HSK[%x]: Client's version: %d.%d\n", session,
                             data[pos], data[pos + 1]);

  adv_version = MHD_gtls_version_get (data[pos], data[pos + 1]);
  set_adv_version (session, data[pos], data[pos + 1]);
  pos += 2;

  neg_version = MHD_gtls_negotiate_version (session, adv_version);
  if (neg_version < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* Read client random value.
   */
  DECR_LEN (len, TLS_RANDOM_SIZE);
  MHD_gtls_set_client_random (session, &data[pos]);
  pos += TLS_RANDOM_SIZE;

  MHD_gtls_tls_create_random (rnd);
  MHD_gtls_set_server_random (session, rnd);

  session->security_parameters.timestamp = time (NULL);

  DECR_LEN (len, 1);
  session_id_len = data[pos++];

  /* RESUME SESSION */
  if (session_id_len > TLS_MAX_SESSION_ID_SIZE)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }
  DECR_LEN (len, session_id_len);

  pos += session_id_len;

  MHD_gtls_generate_session_id (session->security_parameters.session_id,
                                &session->security_parameters.
                                session_id_size);

  session->internals.resumed = RESUME_FALSE;
  /* Remember ciphersuites for later
   */
  DECR_LEN (len, 2);
  suite_size = MHD_gtls_read_uint16 (&data[pos]);
  pos += 2;

  DECR_LEN (len, suite_size);
  suite_ptr = &data[pos];
  pos += suite_size;

  /* Point to the compression methods
   */
  DECR_LEN (len, 1);
  comp_size = data[pos++];      /* z is the number of compression methods */

  DECR_LEN (len, comp_size);
  comp_ptr = &data[pos];
  pos += comp_size;

  /* Parse the extensions (if any)
   */
  if (neg_version >= MHD_GNUTLS_PROTOCOL_TLS1_0)
    {
      ret = MHD_gtls_parse_extensions (session, EXTENSION_APPLICATION, &data[pos], len);        /* len is the rest of the parsed length */
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }

  if (neg_version >= MHD_GNUTLS_PROTOCOL_TLS1_0)
    {
      ret = MHD_gtls_parse_extensions (session, EXTENSION_TLS, &data[pos], len);        /* len is the rest of the parsed length */
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }

  /* select an appropriate cipher suite
   */
  ret = MHD_gtls_server_select_suite (session, suite_ptr, suite_size);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* select appropriate compression method */
  ret = MHD__gnutls_server_select_comp_method (session, comp_ptr, comp_size);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return 0;
}

/* here we hash all pending data.
 */
static int
MHD__gnutls_handshake_hash_pending (MHD_gtls_session_t session)
{
  size_t siz;
  int ret;
  opaque *data;

  if (session->internals.handshake_mac_handle_sha == NULL ||
      session->internals.handshake_mac_handle_md5 == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  /* We check if there are pending data to hash.
   */
  if ((ret = MHD_gtls_handshake_buffer_get_ptr (session, &data, &siz)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  if (siz > 0)
    {
      MHD_gnutls_hash (session->internals.handshake_mac_handle_sha, data,
                       siz);
      MHD_gnutls_hash (session->internals.handshake_mac_handle_md5, data,
                       siz);
    }

  MHD_gtls_handshake_buffer_empty (session);

  return 0;
}


/* This is to be called after sending CHANGE CIPHER SPEC packet
 * and initializing encryption. This is the first encrypted message
 * we send.
 */
static int
MHD__gnutls_send_finished (MHD_gtls_session_t session, int again)
{
  uint8_t data[36];
  int ret;
  int data_size = 0;


  if (again == 0)
    {

      /* This is needed in order to hash all the required
       * messages.
       */
      if ((ret = MHD__gnutls_handshake_hash_pending (session)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      if (MHD__gnutls_protocol_get_version (session) ==
          MHD_GNUTLS_PROTOCOL_SSL3)
        {
          ret =
            MHD__gnutls_ssl3_finished (session,
                                       session->security_parameters.entity,
                                       data);
          data_size = 36;
        }
      else
        {                       /* TLS 1.0 */
          ret =
            MHD__gnutls_finished (session,
                                  session->security_parameters.entity, data);
          data_size = 12;
        }

      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

    }

  ret =
    MHD_gtls_send_handshake (session, data, data_size,
                             GNUTLS_HANDSHAKE_FINISHED);

  return ret;
}

/* This is to be called after sending our finished message. If everything
 * went fine we have negotiated a secure connection
 */
static int
MHD__gnutls_recv_finished (MHD_gtls_session_t session)
{
  uint8_t data[36], *vrfy;
  int data_size;
  int ret;
  int vrfysize;

  ret =
    MHD_gtls_recv_handshake (session, &vrfy, &vrfysize,
                             GNUTLS_HANDSHAKE_FINISHED, MANDATORY_PACKET);
  if (ret < 0)
    {
      ERR ("recv finished int", ret);
      MHD_gnutls_assert ();
      return ret;
    }


  if (MHD__gnutls_protocol_get_version (session) == MHD_GNUTLS_PROTOCOL_SSL3)
    {
      data_size = 36;
    }
  else
    {
      data_size = 12;
    }

  if (vrfysize != data_size)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (vrfy);
      return GNUTLS_E_ERROR_IN_FINISHED_PACKET;
    }

  if (MHD__gnutls_protocol_get_version (session) == MHD_GNUTLS_PROTOCOL_SSL3)
    {
      ret =
        MHD__gnutls_ssl3_finished (session,
                                   (session->security_parameters.entity +
                                    1) % 2, data);
    }
  else
    {                           /* TLS 1.0 */
      ret =
        MHD__gnutls_finished (session,
                              (session->security_parameters.entity +
                               1) % 2, data);
    }

  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (vrfy);
      return ret;
    }

  if (memcmp (vrfy, data, data_size) != 0)
    {
      MHD_gnutls_assert ();
      ret = GNUTLS_E_ERROR_IN_FINISHED_PACKET;
    }
  MHD_gnutls_free (vrfy);

  return ret;
}

/* returns PK_RSA if the given cipher suite list only supports,
 * RSA algorithms, PK_DSA if DSS, and PK_ANY for both or PK_NONE for none.
 */
static int
MHD__gnutls_server_find_pk_algos_in_ciphersuites (const opaque *
                                                  data, int datalen)
{
  int j;
  enum MHD_GNUTLS_PublicKeyAlgorithm algo = GNUTLS_PK_NONE, prev_algo = 0;
  enum MHD_GNUTLS_KeyExchangeAlgorithm kx;
  cipher_suite_st cs;

  if (datalen % 2 != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }

  for (j = 0; j < datalen; j += 2)
    {
      memcpy (&cs.suite, &data[j], 2);
      kx = MHD_gtls_cipher_suite_get_kx_algo (&cs);

      if (MHD_gtls_map_kx_get_cred (kx, 1) == MHD_GNUTLS_CRD_CERTIFICATE)
        {
          algo = MHD_gtls_map_pk_get_pk (kx);

          if (algo != prev_algo && prev_algo != 0)
            return GNUTLS_PK_ANY;
          prev_algo = algo;
        }
    }

  return algo;
}


/* This selects the best supported ciphersuite from the given ones. Then
 * it adds the suite to the session and performs some checks.
 */
static int
MHD_gtls_server_select_suite (MHD_gtls_session_t session, opaque * data,
                              int datalen)
{
  int x, i, j;
  cipher_suite_st *ciphers, cs;
  int retval, err;
  enum MHD_GNUTLS_PublicKeyAlgorithm pk_algo;   /* will hold the pk algorithms
                                                 * supported by the peer.
                                                 */

  pk_algo = MHD__gnutls_server_find_pk_algos_in_ciphersuites (data, datalen);

  x = MHD_gtls_supported_ciphersuites (session, &ciphers);
  if (x < 0)
    {                           /* the case x==0 is handled within the function. */
      MHD_gnutls_assert ();
      return x;
    }

  /* Here we remove any ciphersuite that does not conform
   * the certificate requested, or to the
   * authentication requested (e.g. SRP).
   */
  x = MHD_gtls_remove_unwanted_ciphersuites (session, &ciphers, x, pk_algo);
  if (x <= 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (ciphers);
      if (x < 0)
        return x;
      else
        return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
    }

  /* Data length should be zero mod 2 since
   * every ciphersuite is 2 bytes. (this check is needed
   * see below).
   */
  if (datalen % 2 != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }
  memset (session->security_parameters.current_cipher_suite.suite, '\0', 2);

  retval = GNUTLS_E_UNKNOWN_CIPHER_SUITE;

  for (j = 0; j < datalen; j += 2)
    {
      for (i = 0; i < x; i++)
        {
          if (memcmp (ciphers[i].suite, &data[j], 2) == 0)
            {
              memcpy (&cs.suite, &data[j], 2);

              MHD__gnutls_handshake_log
                ("HSK[%x]: Selected cipher suite: %s\n", session,
                 MHD_gtls_cipher_suite_get_name (&cs));
              memcpy (session->security_parameters.current_cipher_suite.suite,
                      ciphers[i].suite, 2);
              retval = 0;
              goto finish;
            }
        }
    }

finish:
  MHD_gnutls_free (ciphers);

  if (retval != 0)
    {
      MHD_gnutls_assert ();
      return retval;
    }

  /* check if the credentials (username, public key etc.) are ok
   */
  if (MHD_gtls_get_kx_cred
      (session,
       MHD_gtls_cipher_suite_get_kx_algo (&session->security_parameters.
                                          current_cipher_suite), &err) == NULL
      && err != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }


  /* set the MHD_gtls_mod_auth_st to the appropriate struct
   * according to the KX algorithm. This is needed since all the
   * handshake functions are read from there;
   */
  session->internals.auth_struct =
    MHD_gtls_kx_auth_struct (MHD_gtls_cipher_suite_get_kx_algo
                             (&session->security_parameters.
                              current_cipher_suite));
  if (session->internals.auth_struct == NULL)
    {

      MHD__gnutls_handshake_log
        ("HSK[%x]: Cannot find the appropriate handler for the KX algorithm\n",
         session);
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  return 0;

}


/* This selects the best supported compression method from the ones provided
 */
static int
MHD__gnutls_server_select_comp_method (MHD_gtls_session_t session,
                                       opaque * data, int datalen)
{
  int x, i, j;
  uint8_t *comps;

  x = MHD_gtls_supported_compression_methods (session, &comps);
  if (x < 0)
    {
      MHD_gnutls_assert ();
      return x;
    }

  memset (&session->internals.compression_method, 0,
          sizeof (enum MHD_GNUTLS_CompressionMethod));

  for (j = 0; j < datalen; j++)
    {
      for (i = 0; i < x; i++)
        {
          if (comps[i] == data[j])
            {
              enum MHD_GNUTLS_CompressionMethod method =
                MHD_gtls_compression_get_id_from_int (comps[i]);

              session->internals.compression_method = method;
              MHD_gnutls_free (comps);
              return 0;
            }
        }
    }

  /* we were not able to find a compatible compression
   * algorithm
   */
  MHD_gnutls_free (comps);
  MHD_gnutls_assert ();
  return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;

}

/* This function sends an empty handshake packet. (like hello request).
 * If the previous MHD__gnutls_send_empty_handshake() returned
 * GNUTLS_E_AGAIN or GNUTLS_E_INTERRUPTED, then it must be called again
 * (until it returns ok), with NULL parameters.
 */
static int
MHD__gnutls_send_empty_handshake (MHD_gtls_session_t session,
                                  MHD_gnutls_handshake_description_t type,
                                  int again)
{
  opaque data = 0;
  opaque *ptr;

  if (again == 0)
    ptr = &data;
  else
    ptr = NULL;

  return MHD_gtls_send_handshake (session, ptr, 0, type);
}


/* This function will hash the handshake message we sent. */
static int
MHD__gnutls_handshake_hash_add_sent (MHD_gtls_session_t session,
                                     MHD_gnutls_handshake_description_t type,
                                     opaque * dataptr, uint32_t datalen)
{
  int ret;

  if ((ret = MHD__gnutls_handshake_hash_pending (session)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  if (type != GNUTLS_HANDSHAKE_HELLO_REQUEST)
    {
      MHD_gnutls_hash (session->internals.handshake_mac_handle_sha, dataptr,
                       datalen);
      MHD_gnutls_hash (session->internals.handshake_mac_handle_md5, dataptr,
                       datalen);
    }

  return 0;
}


/* This function sends a handshake message of type 'type' containing the
 * data specified here. If the previous MHD_gtls_send_handshake() returned
 * GNUTLS_E_AGAIN or GNUTLS_E_INTERRUPTED, then it must be called again
 * (until it returns ok), with NULL parameters.
 */
int
MHD_gtls_send_handshake (MHD_gtls_session_t session, void *i_data,
                         uint32_t i_datasize,
                         MHD_gnutls_handshake_description_t type)
{
  int ret;
  uint8_t *data;
  uint32_t datasize;
  int pos = 0;

  if (i_data == NULL && i_datasize == 0)
    {
      /* we are resuming a previously interrupted
       * send.
       */
      ret = MHD_gtls_handshake_io_write_flush (session);
      return ret;

    }

  if (i_data == NULL && i_datasize > 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  /* first run */
  datasize = i_datasize + HANDSHAKE_HEADER_SIZE;
  data = MHD_gnutls_alloca (datasize);
  if (data == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  data[pos++] = (uint8_t) type;
  MHD_gtls_write_uint24 (i_datasize, &data[pos]);
  pos += 3;

  if (i_datasize > 0)
    memcpy (&data[pos], i_data, i_datasize);

  /* Here we keep the handshake messages in order to hash them...
   */
  if (type != GNUTLS_HANDSHAKE_HELLO_REQUEST)
    if ((ret =
         MHD__gnutls_handshake_hash_add_sent (session, type, data,
                                              datasize)) < 0)
      {
        MHD_gnutls_assert ();
        MHD_gnutls_afree (data);
        return ret;
      }

  session->internals.last_handshake_out = type;

  ret =
    MHD_gtls_handshake_io_send_int (session, GNUTLS_HANDSHAKE, type,
                                    data, datasize);

  MHD__gnutls_handshake_log ("HSK[%x]: %s was sent [%ld bytes]\n",
                             session, MHD__gnutls_handshake2str (type),
                             (long) datasize);

  MHD_gnutls_afree (data);

  return ret;
}

/* This function will read the handshake header and return it to the caller. If the
 * received handshake packet is not the one expected then it buffers the header, and
 * returns UNEXPECTED_HANDSHAKE_PACKET.
 *
 * FIXME: This function is complex.
 */
#define SSL2_HEADERS 1
static int
MHD__gnutls_recv_handshake_header (MHD_gtls_session_t session,
                                   MHD_gnutls_handshake_description_t type,
                                   MHD_gnutls_handshake_description_t *
                                   recv_type)
{
  int ret;
  uint32_t length32 = 0;
  uint8_t *dataptr = NULL;      /* for realloc */
  size_t handshake_header_size = HANDSHAKE_HEADER_SIZE;

  /* if we have data into the buffer then return them, do not read the next packet.
   * In order to return we need a full TLS handshake header, or in case of a version 2
   * packet, then we return the first byte.
   */
  if (session->internals.handshake_header_buffer.header_size ==
      handshake_header_size || (session->internals.v2_hello != 0
                                && type == GNUTLS_HANDSHAKE_CLIENT_HELLO
                                && session->internals.
                                handshake_header_buffer.packet_length > 0))
    {

      *recv_type = session->internals.handshake_header_buffer.recv_type;

      return session->internals.handshake_header_buffer.packet_length;
    }

  /* Note: SSL2_HEADERS == 1 */

  dataptr = session->internals.handshake_header_buffer.header;

  /* If we haven't already read the handshake headers.
   */
  if (session->internals.handshake_header_buffer.header_size < SSL2_HEADERS)
    {
      ret =
        MHD_gtls_handshake_io_recv_int (session, GNUTLS_HANDSHAKE,
                                        type, dataptr, SSL2_HEADERS);

      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      /* The case ret==0 is caught here.
       */
      if (ret != SSL2_HEADERS)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
        }
      session->internals.handshake_header_buffer.header_size = SSL2_HEADERS;
    }

  if (session->internals.v2_hello == 0
      || type != GNUTLS_HANDSHAKE_CLIENT_HELLO)
    {
      ret =
        MHD_gtls_handshake_io_recv_int (session, GNUTLS_HANDSHAKE,
                                        type,
                                        &dataptr
                                        [session->internals.
                                         handshake_header_buffer.header_size],
                                        HANDSHAKE_HEADER_SIZE -
                                        session->internals.
                                        handshake_header_buffer.header_size);
      if (ret <= 0)
        {
          MHD_gnutls_assert ();
          return (ret < 0) ? ret : GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
        }
      if ((size_t) ret !=
          HANDSHAKE_HEADER_SIZE -
          session->internals.handshake_header_buffer.header_size)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
        }
      *recv_type = dataptr[0];

      /* we do not use DECR_LEN because we know
       * that the packet has enough data.
       */
      length32 = MHD_gtls_read_uint24 (&dataptr[1]);
      handshake_header_size = HANDSHAKE_HEADER_SIZE;

      MHD__gnutls_handshake_log ("HSK[%x]: %s was received [%ld bytes]\n",
                                 session,
                                 MHD__gnutls_handshake2str (dataptr[0]),
                                 length32 + HANDSHAKE_HEADER_SIZE);

    }
  else
    {                           /* v2 hello */
      length32 = session->internals.v2_hello - SSL2_HEADERS;    /* we've read the first byte */

      handshake_header_size = SSL2_HEADERS;     /* we've already read one byte */

      *recv_type = dataptr[0];

      MHD__gnutls_handshake_log ("HSK[%x]: %s(v2) was received [%ld bytes]\n",
                                 session,
                                 MHD__gnutls_handshake2str (*recv_type),
                                 length32 + handshake_header_size);

      if (*recv_type != GNUTLS_HANDSHAKE_CLIENT_HELLO)
        {                       /* it should be one or nothing */
          MHD_gnutls_assert ();
          return GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
        }
    }

  /* put the packet into the buffer */
  session->internals.handshake_header_buffer.header_size =
    handshake_header_size;
  session->internals.handshake_header_buffer.packet_length = length32;
  session->internals.handshake_header_buffer.recv_type = *recv_type;

  if (*recv_type != type)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
    }

  return length32;
}

#define MHD__gnutls_handshake_header_buffer_clear( session) session->internals.handshake_header_buffer.header_size = 0

/* This function will hash the handshake headers and the
 * handshake data.
 */
static int
MHD__gnutls_handshake_hash_add_recvd (MHD_gtls_session_t session,
                                      MHD_gnutls_handshake_description_t
                                      recv_type, opaque * header,
                                      uint16_t header_size, opaque * dataptr,
                                      uint32_t datalen)
{
  int ret;

  /* The idea here is to hash the previous message we received,
   * and add the one we just received into the handshake_hash_buffer.
   */

  if ((ret = MHD__gnutls_handshake_hash_pending (session)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* here we buffer the handshake messages - needed at Finished message */
  if (recv_type != GNUTLS_HANDSHAKE_HELLO_REQUEST)
    {

      if ((ret =
           MHD_gtls_handshake_buffer_put (session, header, header_size)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      if (datalen > 0)
        {
          if ((ret =
               MHD_gtls_handshake_buffer_put (session, dataptr, datalen)) < 0)
            {
              MHD_gnutls_assert ();
              return ret;
            }
        }
    }

  return 0;
}

/* This function will receive handshake messages of the given types,
 * and will pass the message to the right place in order to be processed.
 * E.g. for the SERVER_HELLO message (if it is expected), it will be
 * passed to MHD_gtls_recv_hello().
 */
int
MHD_gtls_recv_handshake (MHD_gtls_session_t session, uint8_t ** data,
                         int *datalen,
                         MHD_gnutls_handshake_description_t type,
                         Optional optional)
{
  int ret;
  uint32_t length32 = 0;
  opaque *dataptr = NULL;
  MHD_gnutls_handshake_description_t recv_type;

  ret = MHD__gnutls_recv_handshake_header (session, type, &recv_type);
  if (ret < 0)
    {

      if (ret == GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET
          && optional == OPTIONAL_PACKET)
        {
          if (datalen != NULL)
            *datalen = 0;
          if (data != NULL)
            *data = NULL;
          return 0;             /* ok just ignore the packet */
        }

      return ret;
    }

  session->internals.last_handshake_in = recv_type;

  length32 = ret;

  if (length32 > 0)
    dataptr = MHD_gnutls_malloc (length32);
  else if (recv_type != GNUTLS_HANDSHAKE_SERVER_HELLO_DONE)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }

  if (dataptr == NULL && length32 > 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  if (datalen != NULL)
    *datalen = length32;

  if (length32 > 0)
    {
      ret =
        MHD_gtls_handshake_io_recv_int (session, GNUTLS_HANDSHAKE,
                                        type, dataptr, length32);
      if (ret <= 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (dataptr);
          return (ret == 0) ? GNUTLS_E_UNEXPECTED_PACKET_LENGTH : ret;
        }
    }

  if (data != NULL && length32 > 0)
    *data = dataptr;


  ret = MHD__gnutls_handshake_hash_add_recvd (session, recv_type,
                                              session->internals.
                                              handshake_header_buffer.header,
                                              session->internals.
                                              handshake_header_buffer.header_size,
                                              dataptr, length32);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_handshake_header_buffer_clear (session);
      return ret;
    }

  /* If we fail before this then we will reuse the handshake header
   * have have received above. if we get here the we clear the handshake
   * header we received.
   */
  MHD__gnutls_handshake_header_buffer_clear (session);

  switch (recv_type)
    {
    case GNUTLS_HANDSHAKE_CLIENT_HELLO:
    case GNUTLS_HANDSHAKE_SERVER_HELLO:
      ret = MHD_gtls_recv_hello (session, dataptr, length32);
      /* dataptr is freed because the caller does not
       * need it */
      MHD_gnutls_free (dataptr);
      if (data != NULL)
        *data = NULL;
      break;
    case GNUTLS_HANDSHAKE_SERVER_HELLO_DONE:
      if (length32 == 0)
        ret = 0;
      else
        ret = GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
      break;
    case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:
    case GNUTLS_HANDSHAKE_FINISHED:
    case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
    case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
    case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
    case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
    case GNUTLS_HANDSHAKE_SUPPLEMENTAL:
      ret = length32;
      break;
    default:
      MHD_gnutls_assert ();
      MHD_gnutls_free (dataptr);
      if (data != NULL)
        *data = NULL;
      ret = GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET;
    }

  return ret;
}

#if MHD_DEBUG_TLS
/* This function checks if the given cipher suite is supported, and sets it
 * to the session;
 */
static int
MHD__gnutls_client_set_ciphersuite (MHD_gtls_session_t session,
                                    opaque suite[2])
{
  uint8_t z;
  cipher_suite_st *cipher_suites;
  int cipher_suite_num;
  int i, err;

  z = 1;
  cipher_suite_num =
    MHD_gtls_supported_ciphersuites (session, &cipher_suites);
  if (cipher_suite_num < 0)
    {
      MHD_gnutls_assert ();
      return cipher_suite_num;
    }

  for (i = 0; i < cipher_suite_num; i++)
    {
      if (memcmp (&cipher_suites[i], suite, 2) == 0)
        {
          z = 0;
          break;
        }
    }

  MHD_gnutls_free (cipher_suites);

  if (z != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_CIPHER_SUITE;
    }

  memcpy (session->security_parameters.current_cipher_suite.suite, suite, 2);

  MHD__gnutls_handshake_log ("HSK[%x]: Selected cipher suite: %s\n", session,
                             MHD_gtls_cipher_suite_get_name
                             (&session->security_parameters.
                              current_cipher_suite));


  /* check if the credentials (username, public key etc.) are ok.
   * Actually checks if they exist.
   */
  if (MHD_gtls_get_kx_cred
      (session,
       MHD_gtls_cipher_suite_get_kx_algo
       (&session->security_parameters.current_cipher_suite), &err) == NULL
      && err != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }


  /* set the MHD_gtls_mod_auth_st to the appropriate struct
   * according to the KX algorithm. This is needed since all the
   * handshake functions are read from there;
   */
  session->internals.auth_struct =
    MHD_gtls_kx_auth_struct (MHD_gtls_cipher_suite_get_kx_algo
                             (&session->security_parameters.
                              current_cipher_suite));

  if (session->internals.auth_struct == NULL)
    {

      MHD__gnutls_handshake_log
        ("HSK[%x]: Cannot find the appropriate handler for the KX algorithm\n",
         session);
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }


  return 0;
}


/* This function sets the given comp method to the session.
 */
static int
MHD__gnutls_client_set_comp_method (MHD_gtls_session_t session,
                                    opaque comp_method)
{
  int comp_methods_num;
  uint8_t *compression_methods;
  int i;

  comp_methods_num = MHD_gtls_supported_compression_methods (session,
                                                             &compression_methods);
  if (comp_methods_num < 0)
    {
      MHD_gnutls_assert ();
      return comp_methods_num;
    }

  for (i = 0; i < comp_methods_num; i++)
    {
      if (compression_methods[i] == comp_method)
        {
          comp_methods_num = 0;
          break;
        }
    }

  MHD_gnutls_free (compression_methods);

  if (comp_methods_num != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }

  session->internals.compression_method =
    MHD_gtls_compression_get_id_from_int (comp_method);


  return 0;
}

/* This function returns 0 if we are resuming a session or -1 otherwise.
 * This also sets the variables in the session. Used only while reading a server
 * hello.
 */
static int
MHD__gnutls_client_check_if_resuming (MHD_gtls_session_t session,
                                      opaque * session_id, int session_id_len)
{
  opaque buf[2 * TLS_MAX_SESSION_ID_SIZE + 1];

  MHD__gnutls_handshake_log ("HSK[%x]: SessionID length: %d\n", session,
                             session_id_len);
  MHD__gnutls_handshake_log ("HSK[%x]: SessionID: %s\n", session,
                             MHD_gtls_bin2hex (session_id, session_id_len,
                                               (char *) buf, sizeof (buf)));

  if (session_id_len > 0 &&
      session->internals.resumed_security_parameters.session_id_size ==
      session_id_len
      && memcmp (session_id,
                 session->internals.resumed_security_parameters.session_id,
                 session_id_len) == 0)
    {
      /* resume session */
      memcpy (session->internals.resumed_security_parameters.server_random,
              session->security_parameters.server_random, TLS_RANDOM_SIZE);
      memcpy (session->internals.resumed_security_parameters.client_random,
              session->security_parameters.client_random, TLS_RANDOM_SIZE);
      session->internals.resumed = RESUME_TRUE; /* we are resuming */

      return 0;
    }
  else
    {
      /* keep the new session id */
      session->internals.resumed = RESUME_FALSE;        /* we are not resuming */
      session->security_parameters.session_id_size = session_id_len;
      memcpy (session->security_parameters.session_id,
              session_id, session_id_len);

      return -1;
    }
}

/* This function reads and parses the server hello handshake message.
 * This function also restores resumed parameters if we are resuming a
 * session.
 */
static int
MHD__gnutls_read_server_hello (MHD_gtls_session_t session,
                               opaque * data, int datalen)
{
  uint8_t session_id_len = 0;
  int pos = 0;
  int ret = 0;
  enum MHD_GNUTLS_Protocol version;
  int len = datalen;

  if (datalen < 38)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }

  MHD__gnutls_handshake_log ("HSK[%x]: Server's version: %d.%d\n",
                             session, data[pos], data[pos + 1]);

  DECR_LEN (len, 2);
  version = MHD_gtls_version_get (data[pos], data[pos + 1]);
  if (MHD_gtls_version_is_supported (session, version) == 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
    }
  else
    {
      MHD_gtls_set_current_version (session, version);
    }

  pos += 2;

  DECR_LEN (len, TLS_RANDOM_SIZE);
  MHD_gtls_set_server_random (session, &data[pos]);
  pos += TLS_RANDOM_SIZE;


  /* Read session ID
   */
  DECR_LEN (len, 1);
  session_id_len = data[pos++];

  if (len < session_id_len)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNSUPPORTED_VERSION_PACKET;
    }
  DECR_LEN (len, session_id_len);


  /* check if we are resuming and set the appropriate
   * values;
   */
  if (MHD__gnutls_client_check_if_resuming
      (session, &data[pos], session_id_len) == 0)
    return 0;
  pos += session_id_len;


  /* Check if the given cipher suite is supported and copy
   * it to the session.
   */

  DECR_LEN (len, 2);
  ret = MHD__gnutls_client_set_ciphersuite (session, &data[pos]);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }
  pos += 2;



  /* move to compression   */
  DECR_LEN (len, 1);

  ret = MHD__gnutls_client_set_comp_method (session, data[pos++]);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }

  /* Parse extensions.
   */
  if (version >= MHD_GNUTLS_PROTOCOL_TLS1_0)
    {
      ret = MHD_gtls_parse_extensions (session, EXTENSION_ANY, &data[pos], len);        /* len is the rest of the parsed length */
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }
  return ret;
}


/* This function copies the appropriate ciphersuites to a locally allocated buffer
 * Needed in client hello messages. Returns the new data length.
 */
int
MHD__gnutls_copy_ciphersuites (MHD_gtls_session_t session,
                               opaque * ret_data, size_t ret_data_size)
{
  int ret, i;
  cipher_suite_st *cipher_suites;
  uint16_t cipher_num;
  int datalen, pos;

  ret = MHD_gtls_supported_ciphersuites_sorted (session, &cipher_suites);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* Here we remove any ciphersuite that does not conform
   * the certificate requested, or to the
   * authentication requested (eg SRP).
   */
  ret =
    MHD_gtls_remove_unwanted_ciphersuites (session, &cipher_suites, ret, -1);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (cipher_suites);
      return ret;
    }

  /* If no cipher suites were enabled.
   */
  if (ret == 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (cipher_suites);
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }

  cipher_num = ret;

  cipher_num *= sizeof (uint16_t);      /* in order to get bytes */

  datalen = pos = 0;

  datalen += sizeof (uint16_t) + cipher_num;

  if ((size_t) datalen > ret_data_size)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  MHD_gtls_write_uint16 (cipher_num, ret_data);
  pos += 2;

  for (i = 0; i < (cipher_num / 2); i++)
    {
      memcpy (&ret_data[pos], cipher_suites[i].suite, 2);
      pos += 2;
    }
  MHD_gnutls_free (cipher_suites);

  return datalen;
}

/* This function copies the appropriate compression methods, to a locally allocated buffer
 * Needed in hello messages. Returns the new data length.
 */
static int
MHD__gnutls_copy_comp_methods (MHD_gtls_session_t session,
                               opaque * ret_data, size_t ret_data_size)
{
  int ret, i;
  uint8_t *compression_methods, comp_num;
  int datalen, pos;

  ret =
    MHD_gtls_supported_compression_methods (session, &compression_methods);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  comp_num = ret;

  datalen = pos = 0;
  datalen += comp_num + 1;

  if ((size_t) datalen > ret_data_size)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  ret_data[pos++] = comp_num;   /* put the number of compression methods */

  for (i = 0; i < comp_num; i++)
    {
      ret_data[pos++] = compression_methods[i];
    }

  MHD_gnutls_free (compression_methods);

  return datalen;
}

static void
MHD_gtls_set_adv_version (MHD_gtls_session_t session,
                          enum MHD_GNUTLS_Protocol ver)
{
  set_adv_version (session, MHD_gtls_version_get_major (ver),
                   MHD_gtls_version_get_minor (ver));
}

/* This function sends the client hello handshake message.
 */
static int
MHD__gnutls_send_client_hello (MHD_gtls_session_t session, int again)
{
  opaque *data = NULL;
  int extdatalen;
  int pos = 0;
  int datalen = 0, ret = 0;
  opaque rnd[TLS_RANDOM_SIZE];
  enum MHD_GNUTLS_Protocol hver;
  opaque extdata[MAX_EXT_DATA_LENGTH];

  opaque *SessionID =
    session->internals.resumed_security_parameters.session_id;
  uint8_t session_id_len =
    session->internals.resumed_security_parameters.session_id_size;

  if (SessionID == NULL)
    session_id_len = 0;
  else if (session_id_len == 0)
    SessionID = NULL;

  if (again == 0)
    {

      datalen = 2 + (session_id_len + 1) + TLS_RANDOM_SIZE;
      /* 2 for version, (4 for unix time + 28 for random bytes==TLS_RANDOM_SIZE)
       */

      data = MHD_gnutls_malloc (datalen);
      if (data == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }

      /* if we are resuming a session then we set the
       * version number to the previously established.
       */
      if (SessionID == NULL)
        hver = MHD_gtls_version_max (session);
      else
        {                       /* we are resuming a session */
          hver = session->internals.resumed_security_parameters.version;
        }

      if (hver == MHD_GNUTLS_PROTOCOL_VERSION_UNKNOWN || hver == 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (data);
          return GNUTLS_E_INTERNAL_ERROR;
        }

      data[pos++] = MHD_gtls_version_get_major (hver);
      data[pos++] = MHD_gtls_version_get_minor (hver);

      /* Set the version we advertized as maximum
       * (RSA uses it).
       */
      MHD_gtls_set_adv_version (session, hver);

      /* Some old implementations do not interoperate if we send a
       * different version in the record layer.
       * It seems they prefer to read the record's version
       * as the one we actually requested.
       * The proper behaviour is to use the one in the client hello
       * handshake packet and ignore the one in the packet's record
       * header.
       */
      MHD_gtls_set_current_version (session, hver);

      /* In order to know when this session was initiated.
       */
      session->security_parameters.timestamp = time (NULL);

      /* Generate random data
       */
      MHD_gtls_tls_create_random (rnd);
      MHD_gtls_set_client_random (session, rnd);

      memcpy (&data[pos], rnd, TLS_RANDOM_SIZE);
      pos += TLS_RANDOM_SIZE;

      /* Copy the Session ID       */
      data[pos++] = session_id_len;

      if (session_id_len > 0)
        {
          memcpy (&data[pos], SessionID, session_id_len);
          pos += session_id_len;
        }


      /* Copy the ciphersuites.
       */
      extdatalen =
        MHD__gnutls_copy_ciphersuites (session, extdata, sizeof (extdata));
      if (extdatalen > 0)
        {
          datalen += extdatalen;
          data = MHD_gtls_realloc_fast (data, datalen);
          if (data == NULL)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }

          memcpy (&data[pos], extdata, extdatalen);
          pos += extdatalen;

        }
      else
        {
          if (extdatalen == 0)
            extdatalen = GNUTLS_E_INTERNAL_ERROR;
          MHD_gnutls_free (data);
          MHD_gnutls_assert ();
          return extdatalen;
        }


      /* Copy the compression methods.
       */
      extdatalen =
        MHD__gnutls_copy_comp_methods (session, extdata, sizeof (extdata));
      if (extdatalen > 0)
        {
          datalen += extdatalen;
          data = MHD_gtls_realloc_fast (data, datalen);
          if (data == NULL)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }

          memcpy (&data[pos], extdata, extdatalen);
          pos += extdatalen;

        }
      else
        {
          if (extdatalen == 0)
            extdatalen = GNUTLS_E_INTERNAL_ERROR;
          MHD_gnutls_free (data);
          MHD_gnutls_assert ();
          return extdatalen;
        }

      /* Generate and copy TLS extensions.
       */
      if (hver >= MHD_GNUTLS_PROTOCOL_TLS1_0)
        {
          extdatalen =
            MHD_gtls_gen_extensions (session, extdata, sizeof (extdata));

          if (extdatalen > 0)
            {
              datalen += extdatalen;
              data = MHD_gtls_realloc_fast (data, datalen);
              if (data == NULL)
                {
                  MHD_gnutls_assert ();
                  return GNUTLS_E_MEMORY_ERROR;
                }

              memcpy (&data[pos], extdata, extdatalen);
            }
          else if (extdatalen < 0)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (data);
              return extdatalen;
            }
        }
    }

  ret =
    MHD_gtls_send_handshake (session, data, datalen,
                             GNUTLS_HANDSHAKE_CLIENT_HELLO);
  MHD_gnutls_free (data);

  return ret;
}
#endif

static int
MHD__gnutls_send_server_hello (MHD_gtls_session_t session, int again)
{
  opaque *data = NULL;
  opaque extdata[MAX_EXT_DATA_LENGTH];
  int extdatalen;
  int pos = 0;
  int datalen, ret = 0;
  uint8_t comp;
  opaque *SessionID = session->security_parameters.session_id;
  uint8_t session_id_len = session->security_parameters.session_id_size;
  opaque buf[2 * TLS_MAX_SESSION_ID_SIZE + 1];

  if (SessionID == NULL)
    session_id_len = 0;

  datalen = 0;

  if (again == 0)
    {
      datalen = 2 + session_id_len + 1 + TLS_RANDOM_SIZE + 3;
      extdatalen =
        MHD_gtls_gen_extensions (session, extdata, sizeof (extdata));

      if (extdatalen < 0)
        {
          MHD_gnutls_assert ();
          return extdatalen;
        }

      data = MHD_gnutls_alloca (datalen + extdatalen);
      if (data == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }

      data[pos++] =
        MHD_gtls_version_get_major (session->security_parameters.version);
      data[pos++] =
        MHD_gtls_version_get_minor (session->security_parameters.version);

      memcpy (&data[pos],
              session->security_parameters.server_random, TLS_RANDOM_SIZE);
      pos += TLS_RANDOM_SIZE;

      data[pos++] = session_id_len;
      if (session_id_len > 0)
        {
          memcpy (&data[pos], SessionID, session_id_len);
        }
      pos += session_id_len;

      MHD__gnutls_handshake_log ("HSK[%x]: SessionID: %s\n", session,
                                 MHD_gtls_bin2hex (SessionID, session_id_len,
                                                   (char *) buf,
                                                   sizeof (buf)));

      memcpy (&data[pos],
              session->security_parameters.current_cipher_suite.suite, 2);
      pos += 2;

      comp =
        (uint8_t) MHD_gtls_compression_get_num (session->
                                                internals.compression_method);
      data[pos++] = comp;


      if (extdatalen > 0)
        {
          datalen += extdatalen;

          memcpy (&data[pos], extdata, extdatalen);
        }
    }

  ret =
    MHD_gtls_send_handshake (session, data, datalen,
                             GNUTLS_HANDSHAKE_SERVER_HELLO);
  MHD_gnutls_afree (data);

  return ret;
}

int
MHD_gtls_send_hello (MHD_gtls_session_t session, int again)
{
  int ret;
#if MHD_DEBUG_TLS
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {
      ret = MHD__gnutls_send_client_hello (session, again);

    }
  else
#endif
    {                           /* SERVER */
      ret = MHD__gnutls_send_server_hello (session, again);
    }

  return ret;
}

/* RECEIVE A HELLO MESSAGE. This should be called from MHD_gnutls_recv_handshake_int only if a
 * hello message is expected. It uses the security_parameters.current_cipher_suite
 * and internals.compression_method.
 */
int
MHD_gtls_recv_hello (MHD_gtls_session_t session, opaque * data, int datalen)
{
  int ret;
#if MHD_DEBUG_TLS
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {
      ret = MHD__gnutls_read_server_hello (session, data, datalen);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }
  else
#endif
    {                           /* Server side reading a client hello */

      ret = MHD__gnutls_read_client_hello (session, data, datalen);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }

  return ret;
}

/* The packets in MHD__gnutls_handshake (it's more broad than original TLS handshake)
 *
 *     Client                                               Server
 *
 *     ClientHello                  -------->
 *                                  <--------         ServerHello
 *
 *                                                    Certificate*
 *                                              ServerKeyExchange*
 *                                  <--------   CertificateRequest*
 *
 *                                  <--------      ServerHelloDone
 *     Certificate*
 *     ClientKeyExchange
 *     CertificateVerify*
 *     [ChangeCipherSpec]
 *     Finished                     -------->
 *                                              [ChangeCipherSpec]
 *                                  <--------             Finished
 *
 * (*): means optional packet.
 */

/**
  * MHD__gnutls_rehandshake - This function will renegotiate security parameters
  * @session: is a #MHD_gtls_session_t structure.
  *
  * This function will renegotiate security parameters with the
  * client.  This should only be called in case of a server.
  *
  * This message informs the peer that we want to renegotiate
  * parameters (perform a handshake).
  *
  * If this function succeeds (returns 0), you must call the
  * MHD__gnutls_handshake() function in order to negotiate the new
  * parameters.
  *
  * If the client does not wish to renegotiate parameters he will
  * should with an alert message, thus the return code will be
  * %GNUTLS_E_WARNING_ALERT_RECEIVED and the alert will be
  * %GNUTLS_A_NO_RENEGOTIATION.  A client may also choose to ignore
  * this message.
  *
  * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
  *
  **/
int
MHD__gnutls_rehandshake (MHD_gtls_session_t session)
{
  int ret;

  ret =
    MHD__gnutls_send_empty_handshake (session, GNUTLS_HANDSHAKE_HELLO_REQUEST,
                                      AGAIN (STATE50));
  STATE = STATE50;

  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }
  STATE = STATE0;

  return 0;
}

inline static int
MHD__gnutls_abort_handshake (MHD_gtls_session_t session, int ret)
{
  if (((ret == GNUTLS_E_WARNING_ALERT_RECEIVED) &&
       (MHD_gnutls_alert_get (session) == GNUTLS_A_NO_RENEGOTIATION))
      || ret == GNUTLS_E_GOT_APPLICATION_DATA)
    return 0;

  /* this doesn't matter */
  return GNUTLS_E_INTERNAL_ERROR;
}

/* This function initialized the handshake hash session.
 * required for finished messages.
 */
inline static int
MHD__gnutls_handshake_hash_init (MHD_gtls_session_t session)
{

  if (session->internals.handshake_mac_handle_md5 == NULL)
    {
      session->internals.handshake_mac_handle_md5 =
        MHD_gtls_hash_init (MHD_GNUTLS_MAC_MD5);

      if (session->internals.handshake_mac_handle_md5 == GNUTLS_HASH_FAILED)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }
    }

  if (session->internals.handshake_mac_handle_sha == NULL)
    {
      session->internals.handshake_mac_handle_sha =
        MHD_gtls_hash_init (MHD_GNUTLS_MAC_SHA1);
      if (session->internals.handshake_mac_handle_sha == GNUTLS_HASH_FAILED)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }
    }

  return 0;
}

static int
MHD__gnutls_send_supplemental (MHD_gtls_session_t session, int again)
{
  int ret = 0;

  MHD__gnutls_debug_log ("EXT[%x]: Sending supplemental data\n", session);

  if (again)
    ret = MHD_gtls_send_handshake (session, NULL, 0,
                                   GNUTLS_HANDSHAKE_SUPPLEMENTAL);
  else
    {
      MHD_gtls_buffer buf;
      MHD_gtls_buffer_init (&buf);

      ret = MHD__gnutls_gen_supplemental (session, &buf);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      ret = MHD_gtls_send_handshake (session, buf.data, buf.length,
                                     GNUTLS_HANDSHAKE_SUPPLEMENTAL);
      MHD_gtls_buffer_clear (&buf);
    }

  return ret;
}

static int
MHD__gnutls_recv_supplemental (MHD_gtls_session_t session)
{
  uint8_t *data = NULL;
  int datalen = 0;
  int ret;

  MHD__gnutls_debug_log ("EXT[%x]: Expecting supplemental data\n", session);

  ret = MHD_gtls_recv_handshake (session, &data, &datalen,
                                 GNUTLS_HANDSHAKE_SUPPLEMENTAL,
                                 OPTIONAL_PACKET);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  ret = MHD__gnutls_parse_supplemental (session, data, datalen);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  MHD_gnutls_free (data);

  return ret;
}

/**
  * MHD__gnutls_handshake - This is the main function in the handshake protocol.
  * @session: is a #MHD_gtls_session_t structure.
  *
  * This function does the handshake of the TLS/SSL protocol, and
  * initializes the TLS connection.
  *
  * This function will fail if any problem is encountered, and will
  * return a negative error code. In case of a client, if the client
  * has asked to resume a session, but the server couldn't, then a
  * full handshake will be performed.
  *
  * The non-fatal errors such as %GNUTLS_E_AGAIN and
  * %GNUTLS_E_INTERRUPTED interrupt the handshake procedure, which
  * should be later be resumed.  Call this function again, until it
  * returns 0; cf.  MHD__gnutls_record_get_direction() and
  * MHD_gtls_error_is_fatal().
  *
  * If this function is called by a server after a rehandshake request
  * then %GNUTLS_E_GOT_APPLICATION_DATA or
  * %GNUTLS_E_WARNING_ALERT_RECEIVED may be returned.  Note that these
  * are non fatal errors, only in the specific case of a rehandshake.
  * Their meaning is that the client rejected the rehandshake request.
  *
  * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
  *
  **/
int
MHD__gnutls_handshake (MHD_gtls_session_t session)
{
  int ret;

  if ((ret = MHD__gnutls_handshake_hash_init (session)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }
#if MHD_DEBUG_TLS
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {
      ret = MHD_gtls_handshake_client (session);
    }
  else
#endif
    {
      ret = MHD_gtls_handshake_server (session);
    }

  if (ret < 0)
    {
      /* In the case of a rehandshake abort
       * we should reset the handshake's internal state.
       */
      if (MHD__gnutls_abort_handshake (session, ret) == 0)
        STATE = STATE0;

      return ret;
    }

  ret = MHD_gtls_handshake_common (session);

  if (ret < 0)
    {
      if (MHD__gnutls_abort_handshake (session, ret) == 0)
        STATE = STATE0;

      return ret;
    }

  STATE = STATE0;

  MHD__gnutls_handshake_io_buffer_clear (session);
  MHD_gtls_handshake_internal_state_clear (session);

  return 0;
}

#define IMED_RET( str, ret) do { \
	if (ret < 0) { \
		if (MHD_gtls_error_is_fatal(ret)==0) return ret; \
		MHD_gnutls_assert(); \
		ERR( str, ret); \
		MHD__gnutls_handshake_hash_buffers_clear(session); \
		return ret; \
	} } while (0)


#if MHD_DEBUG_TLS
/*
 * MHD_gtls_handshake_client
 * This function performs the client side of the handshake of the TLS/SSL protocol.
 */
static int
MHD_gtls_handshake_client (MHD_gtls_session_t session)
{
  int ret = 0;

  switch (STATE)
    {
    case STATE0:
    case STATE1:
      ret = MHD_gtls_send_hello (session, AGAIN (STATE1));
      STATE = STATE1;
      IMED_RET ("send hello", ret);

    case STATE2:
      /* receive the server hello */
      ret =
        MHD_gtls_recv_handshake (session, NULL, NULL,
                                 GNUTLS_HANDSHAKE_SERVER_HELLO,
                                 MANDATORY_PACKET);
      STATE = STATE2;
      IMED_RET ("recv hello", ret);

    case STATE70:
      if (session->security_parameters.extensions.do_recv_supplemental)
        {
          ret = MHD__gnutls_recv_supplemental (session);
          STATE = STATE70;
          IMED_RET ("recv supplemental", ret);
        }

    case STATE3:
      /* RECV CERTIFICATE */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_server_certificate (session);
      STATE = STATE3;
      IMED_RET ("recv server certificate", ret);

    case STATE4:
      /* receive the server key exchange */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_server_kx_message (session);
      STATE = STATE4;
      IMED_RET ("recv server kx message", ret);

    case STATE5:
      /* receive the server certificate request - if any
       */

      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_server_certificate_request (session);
      STATE = STATE5;
      IMED_RET ("recv server certificate request message", ret);

    case STATE6:
      /* receive the server hello done */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret =
          MHD_gtls_recv_handshake (session, NULL, NULL,
                                   GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
                                   MANDATORY_PACKET);
      STATE = STATE6;
      IMED_RET ("recv server hello done", ret);

    case STATE71:
      if (session->security_parameters.extensions.do_send_supplemental)
        {
          ret = MHD__gnutls_send_supplemental (session, AGAIN (STATE71));
          STATE = STATE71;
          IMED_RET ("send supplemental", ret);
        }

    case STATE7:
      /* send our certificate - if any and if requested
       */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_send_client_certificate (session, AGAIN (STATE7));
      STATE = STATE7;
      IMED_RET ("send client certificate", ret);

    case STATE8:
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_send_client_kx_message (session, AGAIN (STATE8));
      STATE = STATE8;
      IMED_RET ("send client kx", ret);

    case STATE9:
      /* send client certificate verify */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret =
          MHD_gtls_send_client_certificate_verify (session, AGAIN (STATE9));
      STATE = STATE9;
      IMED_RET ("send client certificate verify", ret);

      STATE = STATE0;
    default:
      break;
    }


  return 0;
}
#endif

/* This function sends the final handshake packets and initializes connection
 */
static int
MHD__gnutls_send_handshake_final (MHD_gtls_session_t session, int init)
{
  int ret = 0;

  /* Send the CHANGE CIPHER SPEC PACKET */

  switch (STATE)
    {
    case STATE0:
    case STATE20:
      ret = MHD_gtls_send_change_cipher_spec (session, AGAIN (STATE20));
      STATE = STATE20;
      if (ret < 0)
        {
          ERR ("send ChangeCipherSpec", ret);
          MHD_gnutls_assert ();
          return ret;
        }

      /* Initialize the connection session (start encryption) - in case of client
       */
      if (init == TRUE)
        {
          ret = MHD_gtls_connection_state_init (session);
          if (ret < 0)
            {
              MHD_gnutls_assert ();
              return ret;
            }
        }

      ret = MHD_gtls_write_connection_state_init (session);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

    case STATE21:
      /* send the finished message */
      ret = MHD__gnutls_send_finished (session, AGAIN (STATE21));
      STATE = STATE21;
      if (ret < 0)
        {
          ERR ("send Finished", ret);
          MHD_gnutls_assert ();
          return ret;
        }

      STATE = STATE0;
    default:
      break;
    }

  return 0;
}

/* This function receives the final handshake packets
 * And executes the appropriate function to initialize the
 * read session.
 */
static int
MHD__gnutls_recv_handshake_final (MHD_gtls_session_t session, int init)
{
  int ret = 0;
  uint8_t ch;

  switch (STATE)
    {
    case STATE0:
    case STATE30:
      ret =
        MHD_gtls_recv_int (session, GNUTLS_CHANGE_CIPHER_SPEC, -1, &ch, 1);
      STATE = STATE30;
      if (ret <= 0)
        {
          ERR ("recv ChangeCipherSpec", ret);
          MHD_gnutls_assert ();
          return (ret < 0) ? ret : GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
        }

      /* Initialize the connection session (start encryption) - in case of server */
      if (init == TRUE)
        {
          ret = MHD_gtls_connection_state_init (session);
          if (ret < 0)
            {
              MHD_gnutls_assert ();
              return ret;
            }
        }

      ret = MHD_gtls_read_connection_state_init (session);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

    case STATE31:
      ret = MHD__gnutls_recv_finished (session);
      STATE = STATE31;
      if (ret < 0)
        {
          ERR ("recv finished", ret);
          MHD_gnutls_assert ();
          return ret;
        }
      STATE = STATE0;
    default:
      break;
    }


  return 0;
}

 /*
  * MHD_gtls_handshake_server
  * This function does the server stuff of the handshake protocol.
  */

static int
MHD_gtls_handshake_server (MHD_gtls_session_t session)
{
  int ret = 0;

  switch (STATE)
    {
    case STATE0:
    case STATE1:
      ret =
        MHD_gtls_recv_handshake (session, NULL, NULL,
                                 GNUTLS_HANDSHAKE_CLIENT_HELLO,
                                 MANDATORY_PACKET);
      STATE = STATE1;
      IMED_RET ("recv hello", ret);

    case STATE2:
      ret = MHD_gtls_send_hello (session, AGAIN (STATE2));
      STATE = STATE2;
      IMED_RET ("send hello", ret);

    case STATE70:
      if (session->security_parameters.extensions.do_send_supplemental)
        {
          ret = MHD__gnutls_send_supplemental (session, AGAIN (STATE70));
          STATE = STATE70;
          IMED_RET ("send supplemental data", ret);
        }

      /* SEND CERTIFICATE + KEYEXCHANGE + CERTIFICATE_REQUEST */
    case STATE3:
      /* NOTE: these should not be send if we are resuming */

      if (session->internals.resumed == RESUME_FALSE)
        ret = MHD_gtls_send_server_certificate (session, AGAIN (STATE3));
      STATE = STATE3;
      IMED_RET ("send server certificate", ret);

    case STATE4:
      /* send server key exchange (A) */
      if (session->internals.resumed == RESUME_FALSE)
        ret = MHD_gtls_send_server_kx_message (session, AGAIN (STATE4));
      STATE = STATE4;
      IMED_RET ("send server kx", ret);

    case STATE5:
      /* Send certificate request - if requested to */
      if (session->internals.resumed == RESUME_FALSE)
        ret =
          MHD_gtls_send_server_certificate_request (session, AGAIN (STATE5));
      STATE = STATE5;
      IMED_RET ("send server cert request", ret);

    case STATE6:
      /* send the server hello done */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret =
          MHD__gnutls_send_empty_handshake (session,
                                            GNUTLS_HANDSHAKE_SERVER_HELLO_DONE,
                                            AGAIN (STATE6));
      STATE = STATE6;
      IMED_RET ("send server hello done", ret);

    case STATE71:
      if (session->security_parameters.extensions.do_recv_supplemental)
        {
          ret = MHD__gnutls_recv_supplemental (session);
          STATE = STATE71;
          IMED_RET ("recv client supplemental", ret);
        }

      /* RECV CERTIFICATE + KEYEXCHANGE + CERTIFICATE_VERIFY */
    case STATE7:
      /* receive the client certificate message */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_client_certificate (session);
      STATE = STATE7;
      IMED_RET ("recv client certificate", ret);

    case STATE8:
      /* receive the client key exchange message */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_client_kx_message (session);
      STATE = STATE8;
      IMED_RET ("recv client kx", ret);

    case STATE9:
      /* receive the client certificate verify message */
      if (session->internals.resumed == RESUME_FALSE)   /* if we are not resuming */
        ret = MHD_gtls_recv_client_certificate_verify_message (session);
      STATE = STATE9;
      IMED_RET ("recv client certificate verify", ret);

      STATE = STATE0;           /* finished thus clear session */
    default:
      break;
    }

  return 0;
}

static int
MHD_gtls_handshake_common (MHD_gtls_session_t session)
{
  int ret = 0;

  /* send and recv the change cipher spec and finished messages */
  if ((session->internals.resumed == RESUME_TRUE
       && session->security_parameters.entity == GNUTLS_CLIENT)
      || (session->internals.resumed == RESUME_FALSE
          && session->security_parameters.entity == GNUTLS_SERVER))
    {
      /* if we are a client resuming - or we are a server not resuming */

      ret = MHD__gnutls_recv_handshake_final (session, TRUE);
      IMED_RET ("recv handshake final", ret);

      ret = MHD__gnutls_send_handshake_final (session, FALSE);
      IMED_RET ("send handshake final", ret);
    }
  else
    {                           /* if we are a client not resuming - or we are a server resuming */

      ret = MHD__gnutls_send_handshake_final (session, TRUE);
      IMED_RET ("send handshake final 2", ret);

      ret = MHD__gnutls_recv_handshake_final (session, FALSE);
      IMED_RET ("recv handshake final 2", ret);
    }

  /* clear handshake buffer */
  MHD__gnutls_handshake_hash_buffers_clear (session);
  return ret;

}

static int
MHD_gtls_generate_session_id (opaque * session_id, uint8_t * len)
{
  *len = TLS_MAX_SESSION_ID_SIZE;

  if (MHD_gc_nonce ((char *) session_id, *len) != GC_OK)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_RANDOM_FAILED;
    }

  return 0;
}

int
MHD_gtls_recv_hello_request (MHD_gtls_session_t session, void *data,
                             uint32_t data_size)
{
  uint8_t type;

  if (session->security_parameters.entity == GNUTLS_SERVER)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET;
    }
  if (data_size < 1)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
    }
  type = ((uint8_t *) data)[0];
  if (type == GNUTLS_HANDSHAKE_HELLO_REQUEST)
    return GNUTLS_E_REHANDSHAKE;
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNEXPECTED_PACKET;
    }
}

/* Returns 1 if the given KX has not the corresponding parameters
 * (DH or RSA) set up. Otherwise returns 0.
 */
inline static int
check_server_params (MHD_gtls_session_t session,
                     enum MHD_GNUTLS_KeyExchangeAlgorithm kx,
                     enum MHD_GNUTLS_KeyExchangeAlgorithm *alg, int alg_size)
{
  int cred_type;
  MHD_gtls_dh_params_t dh_params = NULL;
  MHD_gtls_rsa_params_t rsa_params = NULL;
  int j;

  cred_type = MHD_gtls_map_kx_get_cred (kx, 1);

  /* Read the Diffie Hellman parameters, if any.
   */
  if (cred_type == MHD_GNUTLS_CRD_CERTIFICATE)
    {
      int delete;
      MHD_gtls_cert_credentials_t x509_cred =
        (MHD_gtls_cert_credentials_t) MHD_gtls_get_cred (session->key,
                                                         cred_type, NULL);

      if (x509_cred != NULL)
        {
          dh_params =
            MHD_gtls_get_dh_params (x509_cred->dh_params,
                                    x509_cred->params_func, session);
          rsa_params =
            MHD_gtls_certificate_get_rsa_params (x509_cred->rsa_params,
                                                 x509_cred->params_func,
                                                 session);
        }

      /* Check also if the certificate supports the
       * KX method.
       */
      delete = 1;
      for (j = 0; j < alg_size; j++)
        {
          if (alg[j] == kx)
            {
              delete = 0;
              break;
            }
        }

      if (delete == 1)
        return 1;

    }
  else
    return 0;                   /* no need for params */


  /* If the key exchange method needs RSA or DH params,
   * but they are not set then remove it.
   */
  if (MHD_gtls_kx_needs_rsa_params (kx) != 0)
    {
      /* needs rsa params. */
      if (MHD__gnutls_rsa_params_to_mpi (rsa_params) == NULL)
        {
          MHD_gnutls_assert ();
          return 1;
        }
    }

  if (MHD_gtls_kx_needs_dh_params (kx) != 0)
    {
      /* needs DH params. */
      if (MHD_gtls_dh_params_to_mpi (dh_params) == NULL)
        {
          MHD_gnutls_assert ();
          return 1;
        }
    }

  return 0;
}

/* This function will remove algorithms that are not supported by
 * the requested authentication method. We remove an algorithm if
 * we have a certificate with keyUsage bits set.
 *
 * This does a more high level check than  MHD_gnutls_supported_ciphersuites(),
 * by checking certificates etc.
 */
static int
MHD_gtls_remove_unwanted_ciphersuites (MHD_gtls_session_t session,
                                       cipher_suite_st ** cipherSuites,
                                       int numCipherSuites,
                                       enum MHD_GNUTLS_PublicKeyAlgorithm
                                       requested_pk_algo)
{

  int ret = 0;
  cipher_suite_st *newSuite, cs;
  int newSuiteSize = 0, i;
  MHD_gtls_cert_credentials_t cert_cred;
  enum MHD_GNUTLS_KeyExchangeAlgorithm kx;
  int server = session->security_parameters.entity == GNUTLS_SERVER ? 1 : 0;
  enum MHD_GNUTLS_KeyExchangeAlgorithm *alg = NULL;
  int alg_size = 0;

  /* if we should use a specific certificate,
   * we should remove all algorithms that are not supported
   * by that certificate and are on the same authentication
   * method (CERTIFICATE).
   */

  cert_cred =
    (MHD_gtls_cert_credentials_t) MHD_gtls_get_cred (session->key,
                                                     MHD_GNUTLS_CRD_CERTIFICATE,
                                                     NULL);

  /* If there are certificate credentials, find an appropriate certificate
   * or disable them;
   */
  if (session->security_parameters.entity == GNUTLS_SERVER
      && cert_cred != NULL)
    {
      ret = MHD_gtls_server_select_cert (session, requested_pk_algo);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          MHD__gnutls_x509_log
            ("Could not find an appropriate certificate: %s\n",
             MHD_gtls_strerror (ret));
          cert_cred = NULL;
        }
    }

  /* get all the key exchange algorithms that are
   * supported by the X509 certificate parameters.
   */
  if ((ret =
       MHD_gtls_selected_cert_supported_kx (session, &alg, &alg_size)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  newSuite = MHD_gnutls_malloc (numCipherSuites * sizeof (cipher_suite_st));
  if (newSuite == NULL)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (alg);
      return GNUTLS_E_MEMORY_ERROR;
    }

  /* now removes ciphersuites based on the KX algorithm
   */
  for (i = 0; i < numCipherSuites; i++)
    {
      int delete = 0;

      /* finds the key exchange algorithm in
       * the ciphersuite
       */
      kx = MHD_gtls_cipher_suite_get_kx_algo (&(*cipherSuites)[i]);

      /* if it is defined but had no credentials
       */
      if (MHD_gtls_get_kx_cred (session, kx, NULL) == NULL)
        {
          delete = 1;
        }
      else
        {
          delete = 0;

          if (server)
            delete = check_server_params (session, kx, alg, alg_size);
        }
      memcpy (&cs.suite, &(*cipherSuites)[i].suite, 2);

      if (delete == 0)
        {

          MHD__gnutls_handshake_log ("HSK[%x]: Keeping ciphersuite: %s\n",
                                     session,
                                     MHD_gtls_cipher_suite_get_name (&cs));

          memcpy (newSuite[newSuiteSize].suite, (*cipherSuites)[i].suite, 2);
          newSuiteSize++;
        }
      else
        {
          MHD__gnutls_handshake_log ("HSK[%x]: Removing ciphersuite: %s\n",
                                     session,
                                     MHD_gtls_cipher_suite_get_name (&cs));

        }
    }

  MHD_gnutls_free (alg);
  MHD_gnutls_free (*cipherSuites);
  *cipherSuites = newSuite;

  ret = newSuiteSize;

  return ret;

}

enum MHD_GNUTLS_Protocol
MHD_gtls_get_adv_version (MHD_gtls_session_t session)
{
  return MHD_gtls_version_get (MHD__gnutls_get_adv_version_major (session),
                               MHD__gnutls_get_adv_version_minor (session));
}
