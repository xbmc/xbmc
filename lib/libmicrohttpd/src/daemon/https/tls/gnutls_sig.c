/*
 * Copyright (C) 2001, 2004, 2005, 2006, 2007 Free Software Foundation
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

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <x509_b64.h>
#include <auth_cert.h>
#include <gnutls_cert.h>
#include <gnutls_datum.h>
#include <gnutls_mpi.h>
#include <gnutls_global.h>
#include <gnutls_pk.h>
#include <debug.h>
#include <gnutls_buffers.h>
#include <gnutls_sig.h>
#include <gnutls_kx.h>

static int MHD__gnutls_tls_sign (MHD_gtls_session_t session,
                                 MHD_gnutls_cert * cert,
                                 MHD_gnutls_privkey * pkey,
                                 const MHD_gnutls_datum_t * hash_concat,
                                 MHD_gnutls_datum_t * signature);

/* Generates a signature of all the previous sent packets in the
 * handshake procedure. (20040227: now it works for SSL 3.0 as well)
 */
int
MHD_gtls_tls_sign_hdata (MHD_gtls_session_t session,
                         MHD_gnutls_cert * cert,
                         MHD_gnutls_privkey * pkey,
                         MHD_gnutls_datum_t * signature)
{
  MHD_gnutls_datum_t dconcat;
  int ret;
  opaque concat[36];
  mac_hd_t td_md5;
  mac_hd_t td_sha;
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

  td_sha = MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_sha);
  if (td_sha == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_HASH_FAILED;
    }

  if (ver == MHD_GNUTLS_PROTOCOL_SSL3)
    {
      ret = MHD_gtls_generate_master (session, 1);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      MHD_gnutls_mac_deinit_ssl3_handshake (td_sha, &concat[16],
                                            session->security_parameters.
                                            master_secret, TLS_MASTER_SIZE);
    }
  else
    MHD_gnutls_hash_deinit (td_sha, &concat[16]);

  switch (cert->subject_pk_algorithm)
    {
    case MHD_GNUTLS_PK_RSA:
      td_md5 =
        MHD_gnutls_hash_copy (session->internals.handshake_mac_handle_md5);
      if (td_md5 == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_HASH_FAILED;
        }

      if (ver == MHD_GNUTLS_PROTOCOL_SSL3)
        MHD_gnutls_mac_deinit_ssl3_handshake (td_md5, concat,
                                              session->security_parameters.
                                              master_secret, TLS_MASTER_SIZE);
      else
        MHD_gnutls_hash_deinit (td_md5, concat);

      dconcat.data = concat;
      dconcat.size = 36;
      break;
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }
  ret = MHD__gnutls_tls_sign (session, cert, pkey, &dconcat, signature);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
    }

  return ret;
}

/* Generates a signature of all the random data and the parameters.
 * Used in DHE_* ciphersuites.
 */
int
MHD_gtls_tls_sign_params (MHD_gtls_session_t session,
                          MHD_gnutls_cert * cert,
                          MHD_gnutls_privkey * pkey,
                          MHD_gnutls_datum_t * params,
                          MHD_gnutls_datum_t * signature)
{
  MHD_gnutls_datum_t dconcat;
  int ret;
  mac_hd_t td_sha;
  opaque concat[36];
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

  td_sha = MHD_gtls_hash_init (MHD_GNUTLS_MAC_SHA1);
  if (td_sha == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_HASH_FAILED;
    }

  MHD_gnutls_hash (td_sha, session->security_parameters.client_random,
                   TLS_RANDOM_SIZE);
  MHD_gnutls_hash (td_sha, session->security_parameters.server_random,
                   TLS_RANDOM_SIZE);
  MHD_gnutls_hash (td_sha, params->data, params->size);

  switch (cert->subject_pk_algorithm)
    {
    case MHD_GNUTLS_PK_RSA:
      if (ver < MHD_GNUTLS_PROTOCOL_TLS1_2)
        {
          mac_hd_t td_md5 = MHD_gtls_hash_init (MHD_GNUTLS_MAC_MD5);
          if (td_md5 == NULL)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_HASH_FAILED;
            }

          MHD_gnutls_hash (td_md5, session->security_parameters.client_random,
                           TLS_RANDOM_SIZE);
          MHD_gnutls_hash (td_md5, session->security_parameters.server_random,
                           TLS_RANDOM_SIZE);
          MHD_gnutls_hash (td_md5, params->data, params->size);

          MHD_gnutls_hash_deinit (td_md5, concat);
          MHD_gnutls_hash_deinit (td_sha, &concat[16]);

          dconcat.size = 36;
        }
      else
        {
#if 1
          /* Use NULL parameters. */
          memcpy (concat,
                  "\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14",
                  15);
          MHD_gnutls_hash_deinit (td_sha, &concat[15]);
          dconcat.size = 35;
#else
          /* No parameters field. */
          memcpy (concat,
                  "\x30\x1f\x30\x07\x06\x05\x2b\x0e\x03\x02\x1a\x04\x14", 13);
          MHD_gnutls_hash_deinit (td_sha, &concat[13]);
          dconcat.size = 33;
#endif
        }
      dconcat.data = concat;
      break;
    default:
      MHD_gnutls_assert ();
      MHD_gnutls_hash_deinit (td_sha, NULL);
      return GNUTLS_E_INTERNAL_ERROR;
    }
  ret = MHD__gnutls_tls_sign (session, cert, pkey, &dconcat, signature);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
    }

  return ret;

}

/* This will create a PKCS1 or DSA signature, using the given parameters, and the
 * given data. The output will be allocated and be put in signature.
 */
static int
MHD_gtls_sign (enum MHD_GNUTLS_PublicKeyAlgorithm algo,
               mpi_t * params,
               int params_size,
               const MHD_gnutls_datum_t * data,
               MHD_gnutls_datum_t * signature)
{
  int ret;

  switch (algo)
    {
    case MHD_GNUTLS_PK_RSA:
      /* encrypt */
      if ((ret =
           MHD_gtls_pkcs1_rsa_encrypt (signature, data, params, params_size,
                                       1)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      break;
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
      break;
    }

  return 0;
}

/* This will create a PKCS1 or DSA signature, as defined in the TLS protocol.
 * Cert is the certificate of the corresponding private key. It is only checked if
 * it supports signing.
 */
static int
MHD__gnutls_tls_sign (MHD_gtls_session_t session,
                      MHD_gnutls_cert * cert,
                      MHD_gnutls_privkey * pkey,
                      const MHD_gnutls_datum_t * hash_concat,
                      MHD_gnutls_datum_t * signature)
{

  /* If our certificate supports signing
   */

  if (cert != NULL)
    if (cert->key_usage != 0)
      if (!(cert->key_usage & KEY_DIGITAL_SIGNATURE))
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_KEY_USAGE_VIOLATION;
        }

  /* External signing. */
  if (!pkey || pkey->params_size == 0)
    {
      if (!session->internals.sign_func)
        return GNUTLS_E_INSUFFICIENT_CREDENTIALS;

      return (*session->internals.sign_func) (session,
                                              session->internals.
                                              sign_func_userdata,
                                              cert->cert_type, &cert->raw,
                                              hash_concat, signature);
    }

  return MHD_gtls_sign (pkey->pk_algorithm, pkey->params, pkey->params_size,
                        hash_concat, signature);
}

static int
MHD__gnutls_verify_sig (MHD_gnutls_cert * cert,
                        const MHD_gnutls_datum_t * hash_concat,
                        MHD_gnutls_datum_t * signature, size_t sha1pos)
{
  int ret;
  MHD_gnutls_datum_t vdata;

  if ((cert == NULL) || (cert->version == 0))
    {                           /* this is the only way to check
                                 * if it is initialized
                                 */
      MHD_gnutls_assert ();
      return GNUTLS_E_CERTIFICATE_ERROR;
    }

  /* If the certificate supports signing continue.
   */
  if (cert != NULL)
    if (cert->key_usage != 0)
      if (!(cert->key_usage & KEY_DIGITAL_SIGNATURE))
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_KEY_USAGE_VIOLATION;
        }

  switch (cert->subject_pk_algorithm)
    {
    case MHD_GNUTLS_PK_RSA:

      vdata.data = hash_concat->data;
      vdata.size = hash_concat->size;

      /* verify signature */
      if ((ret = MHD_gtls_rsa_verify (&vdata, signature, cert->params,
                                      cert->params_size, 1)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      break;
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  return 0;
}

/* Verifies a TLS signature (like the one in the client certificate
 * verify message).
 */
int
MHD_gtls_verify_sig_hdata (MHD_gtls_session_t session,
                           MHD_gnutls_cert * cert,
                           MHD_gnutls_datum_t * signature)
{
  int ret;
  opaque concat[36];
  mac_hd_t td_md5;
  mac_hd_t td_sha;
  MHD_gnutls_datum_t dconcat;
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

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

  if (ver == MHD_GNUTLS_PROTOCOL_SSL3)
    {
      ret = MHD_gtls_generate_master (session, 1);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      MHD_gnutls_mac_deinit_ssl3_handshake (td_md5, concat,
                                            session->security_parameters.
                                            master_secret, TLS_MASTER_SIZE);
      MHD_gnutls_mac_deinit_ssl3_handshake (td_sha, &concat[16],
                                            session->security_parameters.
                                            master_secret, TLS_MASTER_SIZE);
    }
  else
    {
      MHD_gnutls_hash_deinit (td_md5, concat);
      MHD_gnutls_hash_deinit (td_sha, &concat[16]);
    }

  dconcat.data = concat;
  dconcat.size = 20 + 16;       /* md5+ sha */

  ret = MHD__gnutls_verify_sig (cert, &dconcat, signature, 16);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return ret;

}

/* Generates a signature of all the random data and the parameters.
 * Used in DHE_* ciphersuites.
 */
int
MHD_gtls_verify_sig_params (MHD_gtls_session_t session,
                            MHD_gnutls_cert * cert,
                            const MHD_gnutls_datum_t * params,
                            MHD_gnutls_datum_t * signature)
{
  MHD_gnutls_datum_t dconcat;
  int ret;
  mac_hd_t td_md5 = NULL;
  mac_hd_t td_sha;
  opaque concat[36];
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

  if (ver < MHD_GNUTLS_PROTOCOL_TLS1_2)
    {
      td_md5 = MHD_gtls_hash_init (MHD_GNUTLS_MAC_MD5);
      if (td_md5 == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_HASH_FAILED;
        }

      MHD_gnutls_hash (td_md5, session->security_parameters.client_random,
                       TLS_RANDOM_SIZE);
      MHD_gnutls_hash (td_md5, session->security_parameters.server_random,
                       TLS_RANDOM_SIZE);
      MHD_gnutls_hash (td_md5, params->data, params->size);
    }

  td_sha = MHD_gtls_hash_init (MHD_GNUTLS_MAC_SHA1);
  if (td_sha == NULL)
    {
      MHD_gnutls_assert ();
      if (td_md5)
        MHD_gnutls_hash_deinit (td_md5, NULL);
      return GNUTLS_E_HASH_FAILED;
    }

  MHD_gnutls_hash (td_sha, session->security_parameters.client_random,
                   TLS_RANDOM_SIZE);
  MHD_gnutls_hash (td_sha, session->security_parameters.server_random,
                   TLS_RANDOM_SIZE);
  MHD_gnutls_hash (td_sha, params->data, params->size);

  if (ver < MHD_GNUTLS_PROTOCOL_TLS1_2)
    {
      MHD_gnutls_hash_deinit (td_md5, concat);
      MHD_gnutls_hash_deinit (td_sha, &concat[16]);
      dconcat.size = 36;
    }
  else
    {
#if 1
      /* Use NULL parameters. */
      memcpy (concat,
              "\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14",
              15);
      MHD_gnutls_hash_deinit (td_sha, &concat[15]);
      dconcat.size = 35;
#else
      /* No parameters field. */
      memcpy (concat,
              "\x30\x1f\x30\x07\x06\x05\x2b\x0e\x03\x02\x1a\x04\x14", 13);
      MHD_gnutls_hash_deinit (td_sha, &concat[13]);
      dconcat.size = 33;
#endif
    }

  dconcat.data = concat;

  ret = MHD__gnutls_verify_sig (cert, &dconcat, signature, dconcat.size - 20);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return ret;

}
