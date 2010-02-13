/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006  Free Software Foundation
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

/* Functions that are supposed to run after the handshake procedure is
 * finished. These functions activate the established security parameters.
 */

#include "MHD_config.h"
#include <gnutls_int.h>
#include <gnutls_constate.h>
#include <gnutls_errors.h>
#include <gnutls_kx.h>
#include <gnutls_algorithms.h>
#include <gnutls_num.h>
#include <gnutls_datum.h>
#include <gnutls_state.h>

static const char keyexp[] = "key expansion";
static const int keyexp_length = sizeof (keyexp) - 1;

static const char ivblock[] = "IV block";
static const int ivblock_length = sizeof (ivblock) - 1;

static const char cliwrite[] = "client write key";
static const int cliwrite_length = sizeof (cliwrite) - 1;

static const char servwrite[] = "server write key";
static const int servwrite_length = sizeof (servwrite) - 1;

#define EXPORT_FINAL_KEY_SIZE 16

/* This function is to be called after handshake, when master_secret,
 *  client_random and server_random have been initialized.
 * This function creates the keys and stores them into pending session.
 * (session->cipher_specs)
 */
int
MHD__gnutls_set_keys (MHD_gtls_session_t session, int hash_size, int IV_size,
                      int key_size, int export_flag)
{
  opaque *key_block;
  opaque rnd[2 * TLS_RANDOM_SIZE];
  opaque rrnd[2 * TLS_RANDOM_SIZE];
  int pos, ret;
  int block_size;
  char buf[65];

  if (session->cipher_specs.generated_keys != 0)
    {
      /* keys have already been generated.
       * reset generated_keys and exit normally.
       */
      session->cipher_specs.generated_keys = 0;
      return 0;
    }

  block_size = 2 * hash_size + 2 * key_size;
  if (export_flag == 0)
    block_size += 2 * IV_size;

  key_block = MHD_gnutls_secure_malloc (block_size);
  if (key_block == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  memcpy (rnd, session->security_parameters.server_random, TLS_RANDOM_SIZE);
  memcpy (&rnd[TLS_RANDOM_SIZE],
          session->security_parameters.client_random, TLS_RANDOM_SIZE);

  memcpy (rrnd, session->security_parameters.client_random, TLS_RANDOM_SIZE);
  memcpy (&rrnd[TLS_RANDOM_SIZE],
          session->security_parameters.server_random, TLS_RANDOM_SIZE);

  if (session->security_parameters.version == MHD_GNUTLS_PROTOCOL_SSL3)
    {                           /* SSL 3 */
      ret =
        MHD_gnutls_ssl3_generate_random
        (session->security_parameters.master_secret, TLS_MASTER_SIZE, rnd,
         2 * TLS_RANDOM_SIZE, block_size, key_block);
    }
  else
    {                           /* TLS 1.0 */
      ret =
        MHD_gtls_PRF (session,
                      (const unsigned char *) session->
                      security_parameters.master_secret, TLS_MASTER_SIZE,
                      keyexp, keyexp_length, rnd, 2 * TLS_RANDOM_SIZE,
                      block_size, key_block);
    }

  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (key_block);
      return ret;
    }

  MHD__gnutls_hard_log ("INT: KEY BLOCK[%d]: %s\n", block_size,
                        MHD_gtls_bin2hex (key_block, block_size, buf,
                                          sizeof (buf)));

  pos = 0;
  if (hash_size > 0)
    {
      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.client_write_mac_secret,
           &key_block[pos], hash_size) < 0)
        {
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }
      pos += hash_size;

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.server_write_mac_secret,
           &key_block[pos], hash_size) < 0)
        {
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }
      pos += hash_size;
    }

  if (key_size > 0)
    {
      opaque *client_write_key, *server_write_key;
      int client_write_key_size, server_write_key_size;
      int free_keys = 0;

      if (export_flag == 0)
        {
          client_write_key = &key_block[pos];
          client_write_key_size = key_size;

          pos += key_size;

          server_write_key = &key_block[pos];
          server_write_key_size = key_size;

          pos += key_size;

        }
      else
        {                       /* export */
          free_keys = 1;

          client_write_key = MHD_gnutls_secure_malloc (EXPORT_FINAL_KEY_SIZE);
          if (client_write_key == NULL)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (key_block);
              return GNUTLS_E_MEMORY_ERROR;
            }

          server_write_key = MHD_gnutls_secure_malloc (EXPORT_FINAL_KEY_SIZE);
          if (server_write_key == NULL)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (key_block);
              MHD_gnutls_free (client_write_key);
              return GNUTLS_E_MEMORY_ERROR;
            }

          /* generate the final keys */

          if (session->security_parameters.version ==
              MHD_GNUTLS_PROTOCOL_SSL3)
            {                   /* SSL 3 */
              ret =
                MHD_gnutls_ssl3_hash_md5 (&key_block[pos],
                                          key_size, rrnd,
                                          2 * TLS_RANDOM_SIZE,
                                          EXPORT_FINAL_KEY_SIZE,
                                          client_write_key);

            }
          else
            {                   /* TLS 1.0 */
              ret =
                MHD_gtls_PRF (session, &key_block[pos], key_size,
                              cliwrite, cliwrite_length,
                              rrnd,
                              2 * TLS_RANDOM_SIZE,
                              EXPORT_FINAL_KEY_SIZE, client_write_key);
            }

          if (ret < 0)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (key_block);
              MHD_gnutls_free (server_write_key);
              MHD_gnutls_free (client_write_key);
              return ret;
            }

          client_write_key_size = EXPORT_FINAL_KEY_SIZE;
          pos += key_size;

          if (session->security_parameters.version ==
              MHD_GNUTLS_PROTOCOL_SSL3)
            {                   /* SSL 3 */
              ret =
                MHD_gnutls_ssl3_hash_md5 (&key_block[pos], key_size,
                                          rnd, 2 * TLS_RANDOM_SIZE,
                                          EXPORT_FINAL_KEY_SIZE,
                                          server_write_key);
            }
          else
            {                   /* TLS 1.0 */
              ret =
                MHD_gtls_PRF (session, &key_block[pos], key_size,
                              servwrite, servwrite_length,
                              rrnd, 2 * TLS_RANDOM_SIZE,
                              EXPORT_FINAL_KEY_SIZE, server_write_key);
            }

          if (ret < 0)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (key_block);
              MHD_gnutls_free (server_write_key);
              MHD_gnutls_free (client_write_key);
              return ret;
            }

          server_write_key_size = EXPORT_FINAL_KEY_SIZE;
          pos += key_size;
        }

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.client_write_key,
           client_write_key, client_write_key_size) < 0)
        {
          MHD_gnutls_free (key_block);
          MHD_gnutls_free (server_write_key);
          MHD_gnutls_free (client_write_key);
          return GNUTLS_E_MEMORY_ERROR;
        }
      MHD__gnutls_hard_log ("INT: CLIENT WRITE KEY [%d]: %s\n",
                            client_write_key_size,
                            MHD_gtls_bin2hex (client_write_key,
                                              client_write_key_size, buf,
                                              sizeof (buf)));

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.server_write_key,
           server_write_key, server_write_key_size) < 0)
        {
          MHD_gnutls_free (key_block);
          MHD_gnutls_free (server_write_key);
          MHD_gnutls_free (client_write_key);
          return GNUTLS_E_MEMORY_ERROR;
        }

      MHD__gnutls_hard_log ("INT: SERVER WRITE KEY [%d]: %s\n",
                            server_write_key_size,
                            MHD_gtls_bin2hex (server_write_key,
                                              server_write_key_size, buf,
                                              sizeof (buf)));

      if (free_keys != 0)
        {
          MHD_gnutls_free (server_write_key);
          MHD_gnutls_free (client_write_key);
        }
    }


  /* IV generation in export and non export ciphers.
   */
  if (IV_size > 0 && export_flag == 0)
    {
      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.client_write_IV, &key_block[pos],
           IV_size) < 0)
        {
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }
      pos += IV_size;

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.server_write_IV, &key_block[pos],
           IV_size) < 0)
        {
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }
      pos += IV_size;

    }
  else if (IV_size > 0 && export_flag != 0)
    {
      opaque *iv_block = MHD_gnutls_alloca (IV_size * 2);
      if (iv_block == NULL)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }

      if (session->security_parameters.version == MHD_GNUTLS_PROTOCOL_SSL3)
        {                       /* SSL 3 */
          ret = MHD_gnutls_ssl3_hash_md5 ("", 0,
                                          rrnd, TLS_RANDOM_SIZE * 2,
                                          IV_size, iv_block);

          if (ret < 0)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (key_block);
              MHD_gnutls_afree (iv_block);
              return ret;
            }

          ret = MHD_gnutls_ssl3_hash_md5 ("", 0, rnd,
                                          TLS_RANDOM_SIZE * 2,
                                          IV_size, &iv_block[IV_size]);

        }
      else
        {                       /* TLS 1.0 */
          ret = MHD_gtls_PRF (session, (const unsigned char *) "", 0,
                              ivblock, ivblock_length, rrnd,
                              2 * TLS_RANDOM_SIZE, IV_size * 2, iv_block);
        }

      if (ret < 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_afree (iv_block);
          MHD_gnutls_free (key_block);
          return ret;
        }

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.client_write_IV, iv_block, IV_size) < 0)
        {
          MHD_gnutls_afree (iv_block);
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }

      if (MHD__gnutls_sset_datum
          (&session->cipher_specs.server_write_IV,
           &iv_block[IV_size], IV_size) < 0)
        {
          MHD_gnutls_afree (iv_block);
          MHD_gnutls_free (key_block);
          return GNUTLS_E_MEMORY_ERROR;
        }

      MHD_gnutls_afree (iv_block);
    }

  MHD_gnutls_free (key_block);

  session->cipher_specs.generated_keys = 1;

  return 0;
}

int
MHD__gnutls_set_read_keys (MHD_gtls_session_t session)
{
  int hash_size;
  int IV_size;
  int key_size, export_flag;
  enum MHD_GNUTLS_CipherAlgorithm algo;
  enum MHD_GNUTLS_HashAlgorithm mac_algo;

  mac_algo = session->security_parameters.read_mac_algorithm;
  algo = session->security_parameters.read_bulk_cipher_algorithm;

  hash_size = MHD_gnutls_hash_get_algo_len (mac_algo);
  IV_size = MHD_gtls_cipher_get_iv_size (algo);
  key_size = MHD__gnutls_cipher_get_key_size (algo);
  export_flag = MHD_gtls_cipher_get_export_flag (algo);

  return MHD__gnutls_set_keys (session, hash_size, IV_size, key_size,
                               export_flag);
}

int
MHD__gnutls_set_write_keys (MHD_gtls_session_t session)
{
  int hash_size;
  int IV_size;
  int key_size, export_flag;
  enum MHD_GNUTLS_CipherAlgorithm algo;
  enum MHD_GNUTLS_HashAlgorithm mac_algo;

  mac_algo = session->security_parameters.write_mac_algorithm;
  algo = session->security_parameters.write_bulk_cipher_algorithm;

  hash_size = MHD_gnutls_hash_get_algo_len (mac_algo);
  IV_size = MHD_gtls_cipher_get_iv_size (algo);
  key_size = MHD__gnutls_cipher_get_key_size (algo);
  export_flag = MHD_gtls_cipher_get_export_flag (algo);

  return MHD__gnutls_set_keys (session, hash_size, IV_size, key_size,
                               export_flag);
}

#define CPY_COMMON dst->entity = src->entity; \
	dst->kx_algorithm = src->kx_algorithm; \
	memcpy( &dst->current_cipher_suite, &src->current_cipher_suite, sizeof(cipher_suite_st)); \
	memcpy( dst->master_secret, src->master_secret, TLS_MASTER_SIZE); \
	memcpy( dst->client_random, src->client_random, TLS_RANDOM_SIZE); \
	memcpy( dst->server_random, src->server_random, TLS_RANDOM_SIZE); \
	memcpy( dst->session_id, src->session_id, TLS_MAX_SESSION_ID_SIZE); \
	dst->session_id_size = src->session_id_size; \
	dst->cert_type = src->cert_type; \
	dst->timestamp = src->timestamp; \
	dst->max_record_recv_size = src->max_record_recv_size; \
	dst->max_record_send_size = src->max_record_send_size; \
	dst->version = src->version; \
	memcpy( &dst->extensions, &src->extensions, sizeof(MHD_gtls_ext_st)); \
	memcpy( &dst->inner_secret, &src->inner_secret, TLS_MASTER_SIZE);

static void
MHD__gnutls_cpy_read_security_parameters (MHD_gtls_security_param_st *
                                          dst,
                                          MHD_gtls_security_param_st * src)
{
  CPY_COMMON;

  dst->read_bulk_cipher_algorithm = src->read_bulk_cipher_algorithm;
  dst->read_mac_algorithm = src->read_mac_algorithm;
  dst->read_compression_algorithm = src->read_compression_algorithm;
}

static void
MHD__gnutls_cpy_write_security_parameters (MHD_gtls_security_param_st *
                                           dst,
                                           MHD_gtls_security_param_st * src)
{
  CPY_COMMON;

  dst->write_bulk_cipher_algorithm = src->write_bulk_cipher_algorithm;
  dst->write_mac_algorithm = src->write_mac_algorithm;
  dst->write_compression_algorithm = src->write_compression_algorithm;
}

/* Sets the current connection session to conform with the
 * Security parameters(pending session), and initializes encryption.
 * Actually it initializes and starts encryption ( so it needs
 * secrets and random numbers to have been negotiated)
 * This is to be called after sending the Change Cipher Spec packet.
 */
int
MHD_gtls_connection_state_init (MHD_gtls_session_t session)
{
  int ret;

/* Setup the master secret
 */
  if ((ret = MHD_gtls_generate_master (session, 0), 0) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }


  return 0;
}


/* Initializes the read connection session
 * (read encrypted data)
 */
int
MHD_gtls_read_connection_state_init (MHD_gtls_session_t session)
{
  int mac_size;
  int rc;

  MHD__gnutls_uint64zero (session->connection_state.read_sequence_number);

/* Update internals from CipherSuite selected.
 * If we are resuming just copy the connection session
 */
  if (session->internals.resumed == RESUME_FALSE)
    {
      rc = MHD_gtls_set_read_cipher (session,
                                     MHD_gtls_cipher_suite_get_cipher_algo
                                     (&session->security_parameters.
                                      current_cipher_suite));
      if (rc < 0)
        return rc;
      rc = MHD_gtls_set_read_mac (session,
                                  MHD_gtls_cipher_suite_get_mac_algo
                                  (&session->security_parameters.
                                   current_cipher_suite));
      if (rc < 0)
        return rc;

      rc = MHD_gtls_set_kx (session,
                            MHD_gtls_cipher_suite_get_kx_algo
                            (&session->security_parameters.
                             current_cipher_suite));
      if (rc < 0)
        return rc;

      rc = MHD_gtls_set_read_compression (session,
                                          session->internals.
                                          compression_method);
      if (rc < 0)
        return rc;
    }
  else
    {                           /* RESUME_TRUE */
      MHD__gnutls_cpy_read_security_parameters (&session->security_parameters,
                                                &session->internals.
                                                resumed_security_parameters);
    }


  rc = MHD__gnutls_set_read_keys (session);
  if (rc < 0)
    return rc;

  MHD__gnutls_handshake_log ("HSK[%x]: Cipher Suite: %s\n",
                             session,
                             MHD_gtls_cipher_suite_get_name
                             (&session->security_parameters.
                              current_cipher_suite));

  if (MHD_gtls_compression_is_ok
      (session->security_parameters.read_compression_algorithm) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }

  if (MHD_gnutls_mac_is_ok
      (session->security_parameters.read_mac_algorithm) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  /* Free all the previous keys/ sessions etc.
   */
  if (session->connection_state.read_mac_secret.data != NULL)
    MHD__gnutls_free_datum (&session->connection_state.read_mac_secret);

  if (session->connection_state.read_cipher_state != NULL)
    MHD_gnutls_cipher_deinit (session->connection_state.read_cipher_state);

  mac_size =
    MHD_gnutls_hash_get_algo_len (session->security_parameters.
                                  read_mac_algorithm);

  MHD__gnutls_handshake_log
    ("HSK[%x]: Initializing internal [read] cipher sessions\n", session);

  switch (session->security_parameters.entity)
    {
    case GNUTLS_SERVER:
      /* initialize cipher session
       */
      session->connection_state.read_cipher_state =
        MHD_gtls_cipher_init (session->security_parameters.
                              read_bulk_cipher_algorithm,
                              &session->cipher_specs.client_write_key,
                              &session->cipher_specs.client_write_IV);
      if (session->connection_state.read_cipher_state == GNUTLS_CIPHER_FAILED
          && session->security_parameters.read_bulk_cipher_algorithm !=
          MHD_GNUTLS_CIPHER_NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }

      /* copy mac secrets from cipherspecs, to connection
       * session.
       */
      if (mac_size > 0)
        {
          if (MHD__gnutls_sset_datum
              (&session->connection_state.read_mac_secret,
               session->cipher_specs.client_write_mac_secret.data,
               session->cipher_specs.client_write_mac_secret.size) < 0)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }

        }

      break;
#if MHD_DEBUG_TLS
    case GNUTLS_CLIENT:
      session->connection_state.read_cipher_state =
        MHD_gtls_cipher_init (session->security_parameters.
                              read_bulk_cipher_algorithm,
                              &session->cipher_specs.server_write_key,
                              &session->cipher_specs.server_write_IV);

      if (session->connection_state.read_cipher_state ==
          GNUTLS_CIPHER_FAILED
          && session->security_parameters.read_bulk_cipher_algorithm !=
          MHD_GNUTLS_CIPHER_NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }


      /* copy mac secret to connection session
       */
      if (mac_size > 0)
        {
          if (MHD__gnutls_sset_datum
              (&session->connection_state.read_mac_secret,
               session->cipher_specs.server_write_mac_secret.data,
               session->cipher_specs.server_write_mac_secret.size) < 0)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }
        }

      break;
#endif
    default:                   /* this check is useless */
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  return 0;
}



/* Initializes the write connection session
 * (write encrypted data)
 */
int
MHD_gtls_write_connection_state_init (MHD_gtls_session_t session)
{
  int mac_size;
  int rc;

  MHD__gnutls_uint64zero (session->connection_state.write_sequence_number);

/* Update internals from CipherSuite selected.
 * If we are resuming just copy the connection session
 */
  if (session->internals.resumed == RESUME_FALSE)
    {
      rc = MHD_gtls_set_write_cipher (session,
                                      MHD_gtls_cipher_suite_get_cipher_algo
                                      (&session->security_parameters.
                                       current_cipher_suite));
      if (rc < 0)
        return rc;
      rc = MHD_gtls_set_write_mac (session,
                                   MHD_gtls_cipher_suite_get_mac_algo
                                   (&session->security_parameters.
                                    current_cipher_suite));
      if (rc < 0)
        return rc;

      rc = MHD_gtls_set_kx (session,
                            MHD_gtls_cipher_suite_get_kx_algo
                            (&session->security_parameters.
                             current_cipher_suite));
      if (rc < 0)
        return rc;

      rc = MHD_gtls_set_write_compression (session,
                                           session->internals.
                                           compression_method);
      if (rc < 0)
        return rc;
    }
  else
    {                           /* RESUME_TRUE */
      MHD__gnutls_cpy_write_security_parameters
        (&session->security_parameters,
         &session->internals.resumed_security_parameters);
    }

  rc = MHD__gnutls_set_write_keys (session);
  if (rc < 0)
    return rc;

  MHD__gnutls_handshake_log ("HSK[%x]: Cipher Suite: %s\n", session,
                             MHD_gtls_cipher_suite_get_name
                             (&session->security_parameters.
                              current_cipher_suite));

  if (MHD_gtls_compression_is_ok
      (session->security_parameters.write_compression_algorithm) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }

  if (MHD_gnutls_mac_is_ok
      (session->security_parameters.write_mac_algorithm) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }



  /* Free all the previous keys/ sessions etc.
   */
  if (session->connection_state.write_mac_secret.data != NULL)
    MHD__gnutls_free_datum (&session->connection_state.write_mac_secret);

  if (session->connection_state.write_cipher_state != NULL)
    MHD_gnutls_cipher_deinit (session->connection_state.write_cipher_state);

  mac_size =
    MHD_gnutls_hash_get_algo_len (session->security_parameters.
                                  write_mac_algorithm);

  MHD__gnutls_handshake_log
    ("HSK[%x]: Initializing internal [write] cipher sessions\n", session);

  switch (session->security_parameters.entity)
    {
    case GNUTLS_SERVER:
      /* initialize cipher session
       */
      session->connection_state.write_cipher_state =
        MHD_gtls_cipher_init (session->security_parameters.
                              write_bulk_cipher_algorithm,
                              &session->cipher_specs.server_write_key,
                              &session->cipher_specs.server_write_IV);

      if (session->connection_state.write_cipher_state ==
          GNUTLS_CIPHER_FAILED
          && session->security_parameters.write_bulk_cipher_algorithm !=
          MHD_GNUTLS_CIPHER_NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }


      /* copy mac secrets from cipherspecs, to connection
       * session.
       */
      if (mac_size > 0)
        {
          if (MHD__gnutls_sset_datum
              (&session->connection_state.write_mac_secret,
               session->cipher_specs.server_write_mac_secret.data,
               session->cipher_specs.server_write_mac_secret.size) < 0)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }

        }


      break;
#if MHD_DEBUG_TLS
    case GNUTLS_CLIENT:
      session->connection_state.write_cipher_state =
        MHD_gtls_cipher_init (session->security_parameters.
                              write_bulk_cipher_algorithm,
                              &session->cipher_specs.client_write_key,
                              &session->cipher_specs.client_write_IV);

      if (session->connection_state.write_cipher_state ==
          GNUTLS_CIPHER_FAILED
          && session->security_parameters.write_bulk_cipher_algorithm !=
          MHD_GNUTLS_CIPHER_NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }

      /* copy mac secret to connection session
       */
      if (mac_size > 0)
        {
          if (MHD__gnutls_sset_datum
              (&session->connection_state.write_mac_secret,
               session->cipher_specs.client_write_mac_secret.data,
               session->cipher_specs.client_write_mac_secret.size) < 0)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_MEMORY_ERROR;
            }
        }

      break;
#endif
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }


  return 0;
}

/* Sets the specified cipher into the pending session
 */
int
MHD_gtls_set_read_cipher (MHD_gtls_session_t session,
                          enum MHD_GNUTLS_CipherAlgorithm algo)
{

  if (MHD_gtls_cipher_is_ok (algo) == 0)
    {
      if (MHD_gtls_cipher_priority (session, algo) < 0)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_UNWANTED_ALGORITHM;
        }

      session->security_parameters.read_bulk_cipher_algorithm = algo;

    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  return 0;

}

int
MHD_gtls_set_write_cipher (MHD_gtls_session_t session,
                           enum MHD_GNUTLS_CipherAlgorithm algo)
{

  if (MHD_gtls_cipher_is_ok (algo) == 0)
    {
      if (MHD_gtls_cipher_priority (session, algo) < 0)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_UNWANTED_ALGORITHM;
        }

      session->security_parameters.write_bulk_cipher_algorithm = algo;

    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  return 0;

}


/* Sets the specified algorithm into pending compression session
 */
int
MHD_gtls_set_read_compression (MHD_gtls_session_t session,
                               enum MHD_GNUTLS_CompressionMethod algo)
{

  if (MHD_gtls_compression_is_ok (algo) == 0)
    {
      session->security_parameters.read_compression_algorithm = algo;
    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }
  return 0;

}

int
MHD_gtls_set_write_compression (MHD_gtls_session_t session,
                                enum MHD_GNUTLS_CompressionMethod algo)
{

  if (MHD_gtls_compression_is_ok (algo) == 0)
    {
      session->security_parameters.write_compression_algorithm = algo;
    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM;
    }
  return 0;

}

/* Sets the specified kx algorithm into pending session
 */
int
MHD_gtls_set_kx (MHD_gtls_session_t session,
                 enum MHD_GNUTLS_KeyExchangeAlgorithm algo)
{

  if (MHD_gtls_kx_is_ok (algo) == 0)
    {
      session->security_parameters.kx_algorithm = algo;
    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }
  if (MHD_gtls_kx_priority (session, algo) < 0)
    {
      MHD_gnutls_assert ();
      /* we shouldn't get here */
      return GNUTLS_E_UNWANTED_ALGORITHM;
    }

  return 0;

}

/* Sets the specified mac algorithm into pending session */
int
MHD_gtls_set_read_mac (MHD_gtls_session_t session,
                       enum MHD_GNUTLS_HashAlgorithm algo)
{

  if (MHD_gnutls_mac_is_ok (algo) == 0)
    {
      session->security_parameters.read_mac_algorithm = algo;
    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }
  if (MHD_gtls_mac_priority (session, algo) < 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNWANTED_ALGORITHM;
    }


  return 0;

}

int
MHD_gtls_set_write_mac (MHD_gtls_session_t session,
                        enum MHD_GNUTLS_HashAlgorithm algo)
{

  if (MHD_gnutls_mac_is_ok (algo) == 0)
    {
      session->security_parameters.write_mac_algorithm = algo;
    }
  else
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }
  if (MHD_gtls_mac_priority (session, algo) < 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_UNWANTED_ALGORITHM;
    }


  return 0;

}
