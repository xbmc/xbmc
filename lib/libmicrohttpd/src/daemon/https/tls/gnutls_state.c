/*
 * Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 Free Software Foundation
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

/* Functions to manipulate the session (MHD_gnutls_int.h), and some other stuff
 * are included here. The file's name is traditionally MHD_gnutls_state even if the
 * state has been renamed to session.
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <gnutls_auth_int.h>
#include <gnutls_num.h>
#include <gnutls_datum.h>
#include <gnutls_record.h>
#include <gnutls_handshake.h>
#include <gnutls_dh.h>
#include <gnutls_buffers.h>
#include <gnutls_state.h>
#include <auth_cert.h>
#include <gnutls_algorithms.h>
#include <gnutls_rsa_export.h>

void
MHD__gnutls_session_cert_type_set (MHD_gtls_session_t session,
                                   enum MHD_GNUTLS_CertificateType ct)
{
  session->security_parameters.cert_type = ct;
}

/**
 * MHD_gnutls_cipher_get - Returns the currently used cipher.
 * @session: is a #MHD_gtls_session_t structure.
 *
 * Returns: the currently used cipher.
 **/
enum MHD_GNUTLS_CipherAlgorithm
MHD_gnutls_cipher_get (MHD_gtls_session_t session)
{
  return session->security_parameters.read_bulk_cipher_algorithm;
}

/**
 * MHD_gnutls_certificate_type_get - Returns the currently used certificate type.
 * @session: is a #MHD_gtls_session_t structure.
 *
 * The certificate type is by default X.509, unless it is negotiated
 * as a TLS extension.
 *
 * Returns: the currently used %enum MHD_GNUTLS_CertificateType certificate
 *   type.
 **/
enum MHD_GNUTLS_CertificateType
MHD_gnutls_certificate_type_get (MHD_gtls_session_t session)
{
  return session->security_parameters.cert_type;
}

/**
 * MHD_gnutls_kx_get - Returns the key exchange algorithm.
 * @session: is a #MHD_gtls_session_t structure.
 *
 * Returns: the key exchange algorithm used in the last handshake.
 **/
enum MHD_GNUTLS_KeyExchangeAlgorithm
MHD_gnutls_kx_get (MHD_gtls_session_t session)
{
  return session->security_parameters.kx_algorithm;
}

/* Check if the given certificate type is supported.
 * This means that it is enabled by the priority functions,
 * and a matching certificate exists.
 */
int
MHD_gtls_session_cert_type_supported (MHD_gtls_session_t session,
                                      enum MHD_GNUTLS_CertificateType
                                      cert_type)
{
  unsigned i;
  unsigned cert_found = 0;
  MHD_gtls_cert_credentials_t cred;

  if (session->security_parameters.entity == GNUTLS_SERVER)
    {
      cred
        = (MHD_gtls_cert_credentials_t) MHD_gtls_get_cred (session->key,
                                                           MHD_GNUTLS_CRD_CERTIFICATE,
                                                           NULL);

      if (cred == NULL)
        return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;

      if (cred->server_get_cert_callback == NULL)
        {
          for (i = 0; i < cred->ncerts; i++)
            {
              if (cred->cert_list[i][0].cert_type == cert_type)
                {
                  cert_found = 1;
                  break;
                }
            }

          if (cert_found == 0)
            /* no certificate is of that type.
             */
            return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
        }
    }

  if (session->internals.priorities.cert_type.num_algorithms == 0 && cert_type
      == DEFAULT_CERT_TYPE)
    return 0;

  for (i = 0; i < session->internals.priorities.cert_type.num_algorithms; i++)
    {
      if (session->internals.priorities.cert_type.priority[i] == cert_type)
        {
          return 0;             /* ok */
        }
    }

  return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
}

/* this function deinitializes all the internal parameters stored
 * in a session struct.
 */
inline static void
deinit_internal_params (MHD_gtls_session_t session)
{
  if (session->internals.params.free_dh_params)
    MHD__gnutls_dh_params_deinit (session->internals.params.dh_params);

  if (session->internals.params.free_rsa_params)
    MHD__gnutls_rsa_params_deinit (session->internals.params.rsa_params);

  memset (&session->internals.params, 0, sizeof (session->internals.params));
}

/* This function will clear all the variables in internals
 * structure within the session, which depend on the current handshake.
 * This is used to allow further handshakes.
 */
void
MHD_gtls_handshake_internal_state_clear (MHD_gtls_session_t session)
{
  session->internals.extensions_sent_size = 0;

  /* by default no selected certificate */
  session->internals.proposed_record_size = DEFAULT_MAX_RECORD_SIZE;
  session->internals.adv_version_major = 0;
  session->internals.adv_version_minor = 0;
  session->internals.v2_hello = 0;
  memset (&session->internals.handshake_header_buffer, 0,
          sizeof (MHD_gtls_handshake_header_buffer_st));
  session->internals.adv_version_minor = 0;
  session->internals.adv_version_minor = 0;
  session->internals.direction = 0;

  /* use out of band data for the last
   * handshake messages received.
   */
  session->internals.last_handshake_in = -1;
  session->internals.last_handshake_out = -1;

  session->internals.resumable = RESUME_TRUE;
  MHD__gnutls_free_datum (&session->internals.recv_buffer);

  deinit_internal_params (session);

}

#define MIN_DH_BITS 727
/**
 * MHD__gnutls_init - This function initializes the session to null (null encryption etc...).
 * @con_end: indicate if this session is to be used for server or client.
 * @session: is a pointer to a #MHD_gtls_session_t structure.
 *
 * This function initializes the current session to null. Every
 * session must be initialized before use, so internal structures can
 * be allocated.  This function allocates structures which can only
 * be free'd by calling MHD__gnutls_deinit().  Returns zero on success.
 *
 * @con_end can be one of %GNUTLS_CLIENT and %GNUTLS_SERVER.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/

/* TODO rm redundent pointer ref */
int
MHD__gnutls_init (MHD_gtls_session_t * session,
                  MHD_gnutls_connection_end_t con_end)
{
  *session = MHD_gnutls_calloc (1, sizeof (struct MHD_gtls_session_int));
  if (*session == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  (*session)->security_parameters.entity = con_end;

  /* the default certificate type for TLS */
  (*session)->security_parameters.cert_type = DEFAULT_CERT_TYPE;

  /* Set the defaults for initial handshake */
  (*session)->security_parameters.read_bulk_cipher_algorithm =
    (*session)->security_parameters.write_bulk_cipher_algorithm =
    MHD_GNUTLS_CIPHER_NULL;

  (*session)->security_parameters.read_mac_algorithm =
    (*session)->security_parameters.write_mac_algorithm = MHD_GNUTLS_MAC_NULL;

  /* Initialize buffers */
  MHD_gtls_buffer_init (&(*session)->internals.application_data_buffer);
  MHD_gtls_buffer_init (&(*session)->internals.handshake_data_buffer);
  MHD_gtls_buffer_init (&(*session)->internals.handshake_hash_buffer);
  MHD_gtls_buffer_init (&(*session)->internals.ia_data_buffer);

  MHD_gtls_buffer_init (&(*session)->internals.record_send_buffer);
  MHD_gtls_buffer_init (&(*session)->internals.record_recv_buffer);

  MHD_gtls_buffer_init (&(*session)->internals.handshake_send_buffer);
  MHD_gtls_buffer_init (&(*session)->internals.handshake_recv_buffer);

  (*session)->key = MHD_gnutls_calloc (1, sizeof (struct MHD_gtls_key));
  if ((*session)->key == NULL)
    {
    cleanup_session:MHD_gnutls_free (*session);
      *session = NULL;
      return GNUTLS_E_MEMORY_ERROR;
    }

  (*session)->internals.expire_time = DEFAULT_EXPIRE_TIME;      /* one hour default */

  MHD__gnutls_dh_set_prime_bits ((*session), MIN_DH_BITS);

  MHD__gnutls_transport_set_lowat ((*session), DEFAULT_LOWAT);  /* the default for tcp */

  MHD__gnutls_handshake_set_max_packet_length ((*session),
                                               MAX_HANDSHAKE_PACKET_SIZE);

  /* Allocate a minimum size for recv_data
   * This is allocated in order to avoid small messages, making
   * the receive procedure slow.
   */
  (*session)->internals.record_recv_buffer.data
    = MHD_gnutls_malloc (INITIAL_RECV_BUFFER_SIZE);
  if ((*session)->internals.record_recv_buffer.data == NULL)
    {
      MHD_gnutls_free ((*session)->key);
      goto cleanup_session;
    }

  /* set the socket pointers to -1; */
  (*session)->internals.transport_recv_ptr = (MHD_gnutls_transport_ptr_t) - 1;
  (*session)->internals.transport_send_ptr = (MHD_gnutls_transport_ptr_t) - 1;

  /* set the default maximum record size for TLS
   */
  (*session)->security_parameters.max_record_recv_size
    = DEFAULT_MAX_RECORD_SIZE;
  (*session)->security_parameters.max_record_send_size
    = DEFAULT_MAX_RECORD_SIZE;

  /* everything else not initialized here is initialized
   * as NULL or 0. This is why calloc is used.
   */

  MHD_gtls_handshake_internal_state_clear (*session);

  return 0;
}


/**
 * MHD__gnutls_deinit - This function clears all buffers associated with a session
 * @session: is a #MHD_gtls_session_t structure.
 *
 * This function clears all buffers associated with the @session.
 * This function will also remove session data from the session
 * database if the session was terminated abnormally.
 **/
void
MHD__gnutls_deinit (MHD_gtls_session_t session)
{

  if (session == NULL)
    return;

  /* remove auth info firstly */
  MHD_gtls_free_auth_info (session);

  MHD_gtls_handshake_internal_state_clear (session);
  MHD__gnutls_handshake_io_buffer_clear (session);

  MHD__gnutls_free_datum (&session->connection_state.read_mac_secret);
  MHD__gnutls_free_datum (&session->connection_state.write_mac_secret);

  MHD_gtls_buffer_clear (&session->internals.ia_data_buffer);
  MHD_gtls_buffer_clear (&session->internals.handshake_hash_buffer);
  MHD_gtls_buffer_clear (&session->internals.handshake_data_buffer);
  MHD_gtls_buffer_clear (&session->internals.application_data_buffer);
  MHD_gtls_buffer_clear (&session->internals.record_recv_buffer);
  MHD_gtls_buffer_clear (&session->internals.record_send_buffer);

  MHD__gnutls_credentials_clear (session);
  MHD_gtls_selected_certs_deinit (session);

  if (session->connection_state.read_cipher_state != NULL)
    MHD_gnutls_cipher_deinit (session->connection_state.read_cipher_state);
  if (session->connection_state.write_cipher_state != NULL)
    MHD_gnutls_cipher_deinit (session->connection_state.write_cipher_state);

  MHD__gnutls_free_datum (&session->cipher_specs.server_write_mac_secret);
  MHD__gnutls_free_datum (&session->cipher_specs.client_write_mac_secret);
  MHD__gnutls_free_datum (&session->cipher_specs.server_write_IV);
  MHD__gnutls_free_datum (&session->cipher_specs.client_write_IV);
  MHD__gnutls_free_datum (&session->cipher_specs.server_write_key);
  MHD__gnutls_free_datum (&session->cipher_specs.client_write_key);

  if (session->key != NULL)
    {
      MHD_gtls_mpi_release (&session->key->KEY);
      MHD_gtls_mpi_release (&session->key->client_Y);
      MHD_gtls_mpi_release (&session->key->client_p);
      MHD_gtls_mpi_release (&session->key->client_g);

      MHD_gtls_mpi_release (&session->key->u);
      MHD_gtls_mpi_release (&session->key->a);
      MHD_gtls_mpi_release (&session->key->x);
      MHD_gtls_mpi_release (&session->key->A);
      MHD_gtls_mpi_release (&session->key->B);
      MHD_gtls_mpi_release (&session->key->b);

      /* RSA */
      MHD_gtls_mpi_release (&session->key->rsa[0]);
      MHD_gtls_mpi_release (&session->key->rsa[1]);

      MHD_gtls_mpi_release (&session->key->dh_secret);
      MHD_gnutls_free (session->key);

      session->key = NULL;
    }

  memset (session, 0, sizeof (struct MHD_gtls_session_int));
  MHD_gnutls_free (session);
}

/* Returns the minimum prime bits that are acceptable.
 */
int
MHD_gtls_dh_get_allowed_prime_bits (MHD_gtls_session_t session)
{
  return session->internals.dh_prime_bits;
}

int
MHD_gtls_dh_set_peer_public (MHD_gtls_session_t session, mpi_t public)
{
  MHD_gtls_dh_info_st *dh;
  int ret;

  switch (MHD_gtls_auth_get_type (session))
    {
    case MHD_GNUTLS_CRD_CERTIFICATE:
      {
        cert_auth_info_t info;

        info = MHD_gtls_get_auth_info (session);
        if (info == NULL)
          return GNUTLS_E_INTERNAL_ERROR;

        dh = &info->dh;
        break;
      }
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  ret = MHD_gtls_mpi_dprint_lz (&dh->public_key, public);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return 0;
}

int
MHD_gtls_dh_set_secret_bits (MHD_gtls_session_t session, unsigned bits)
{
  switch (MHD_gtls_auth_get_type (session))
    {
    case MHD_GNUTLS_CRD_CERTIFICATE:
      {
        cert_auth_info_t info;

        info = MHD_gtls_get_auth_info (session);
        if (info == NULL)
          return GNUTLS_E_INTERNAL_ERROR;

        info->dh.secret_bits = bits;
        break;
    default:
        MHD_gnutls_assert ();
        return GNUTLS_E_INTERNAL_ERROR;
      }
    }

  return 0;
}

/* This function will set in the auth info structure the
 * RSA exponent and the modulus.
 */
int
MHD_gtls_rsa_export_set_pubkey (MHD_gtls_session_t session,
                                mpi_t exponent, mpi_t modulus)
{
  cert_auth_info_t info;
  int ret;

  info = MHD_gtls_get_auth_info (session);
  if (info == NULL)
    return GNUTLS_E_INTERNAL_ERROR;

  ret = MHD_gtls_mpi_dprint_lz (&info->rsa_export.modulus, modulus);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  ret = MHD_gtls_mpi_dprint_lz (&info->rsa_export.exponent, exponent);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_free_datum (&info->rsa_export.modulus);
      return ret;
    }

  return 0;
}

/* Sets the prime and the generator in the auth info structure.
 */
int
MHD_gtls_dh_set_group (MHD_gtls_session_t session, mpi_t gen, mpi_t prime)
{
  MHD_gtls_dh_info_st *dh;
  int ret;

  switch (MHD_gtls_auth_get_type (session))
    {
    case MHD_GNUTLS_CRD_CERTIFICATE:
      {
        cert_auth_info_t info;

        info = MHD_gtls_get_auth_info (session);
        if (info == NULL)
          return GNUTLS_E_INTERNAL_ERROR;

        dh = &info->dh;
        break;
      }
    default:
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  /* prime
   */
  ret = MHD_gtls_mpi_dprint_lz (&dh->prime, prime);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* generator
   */
  ret = MHD_gtls_mpi_dprint_lz (&dh->generator, gen);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_free_datum (&dh->prime);
      return ret;
    }

  return 0;
}

/**
 * MHD__gnutls_certificate_send_x509_rdn_sequence - This function will order gnutls to send or not the x.509 rdn sequence
 * @session: is a pointer to a #MHD_gtls_session_t structure.
 * @status: is 0 or 1
 *
 * If status is non zero, this function will order gnutls not to send
 * the rdnSequence in the certificate request message. That is the
 * server will not advertize it's trusted CAs to the peer. If status
 * is zero then the default behaviour will take effect, which is to
 * advertize the server's trusted CAs.
 *
 * This function has no effect in clients, and in authentication
 * methods other than certificate with X.509 certificates.
 **/
void
MHD__gnutls_certificate_send_x509_rdn_sequence (MHD_gtls_session_t session,
                                                int status)
{
  session->internals.ignore_rdn_sequence = status;
}

/*-
 * MHD__gnutls_record_set_default_version - Used to set the default version for the first record packet
 * @session: is a #MHD_gtls_session_t structure.
 * @major: is a tls major version
 * @minor: is a tls minor version
 *
 * This function sets the default version that we will use in the first
 * record packet (client hello). This function is only useful to people
 * that know TLS internals and want to debug other implementations.
 *
 -*/
void
MHD__gnutls_record_set_default_version (MHD_gtls_session_t session,
                                        unsigned char major,
                                        unsigned char minor)
{
  session->internals.default_record_version[0] = major;
  session->internals.default_record_version[1] = minor;
}

inline static int
MHD__gnutls_cal_PRF_A (enum MHD_GNUTLS_HashAlgorithm algorithm,
                       const void *secret,
                       int secret_size,
                       const void *seed, int seed_size, void *result)
{
  mac_hd_t td1;

  td1 = MHD_gtls_MHD_hmac_init (algorithm, secret, secret_size);
  if (td1 == GNUTLS_MAC_FAILED)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  MHD_gnutls_hash (td1, seed, seed_size);
  MHD_gnutls_MHD_hmac_deinit (td1, result);

  return 0;
}

#define MAX_SEED_SIZE 200

/* Produces "total_bytes" bytes using the hash algorithm specified.
 * (used in the PRF function)
 */
static int
MHD__gnutls_P_hash (enum MHD_GNUTLS_HashAlgorithm algorithm,
                    const opaque * secret,
                    int secret_size,
                    const opaque * seed,
                    int seed_size, int total_bytes, opaque * ret)
{

  mac_hd_t td2;
  int i, times, how, blocksize, A_size;
  opaque final[20], Atmp[MAX_SEED_SIZE];
  int output_bytes, result;

  if (seed_size > MAX_SEED_SIZE || total_bytes <= 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  blocksize = MHD_gnutls_hash_get_algo_len (algorithm);

  output_bytes = 0;
  do
    {
      output_bytes += blocksize;
    }
  while (output_bytes < total_bytes);

  /* calculate A(0) */

  memcpy (Atmp, seed, seed_size);
  A_size = seed_size;

  times = output_bytes / blocksize;

  for (i = 0; i < times; i++)
    {
      td2 = MHD_gtls_MHD_hmac_init (algorithm, secret, secret_size);
      if (td2 == GNUTLS_MAC_FAILED)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }

      /* here we calculate A(i+1) */
      if ((result =
           MHD__gnutls_cal_PRF_A (algorithm, secret, secret_size, Atmp,
                                  A_size, Atmp)) < 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_MHD_hmac_deinit (td2, final);
          return result;
        }

      A_size = blocksize;

      MHD_gnutls_hash (td2, Atmp, A_size);
      MHD_gnutls_hash (td2, seed, seed_size);
      MHD_gnutls_MHD_hmac_deinit (td2, final);

      if ((1 + i) * blocksize < total_bytes)
        {
          how = blocksize;
        }
      else
        {
          how = total_bytes - (i) * blocksize;
        }

      if (how > 0)
        {
          memcpy (&ret[i * blocksize], final, how);
        }
    }

  return 0;
}

/* Xor's two buffers and puts the output in the first one.
 */
inline static void
MHD__gnutls_xor (opaque * o1, opaque * o2, int length)
{
  int i;
  for (i = 0; i < length; i++)
    {
      o1[i] ^= o2[i];
    }
}

#define MAX_PRF_BYTES 200

/* The PRF function expands a given secret
 * needed by the TLS specification. ret must have a least total_bytes
 * available.
 */
int
MHD_gtls_PRF (MHD_gtls_session_t session,
              const opaque * secret,
              int secret_size,
              const char *label,
              int label_size,
              const opaque * seed, int seed_size, int total_bytes, void *ret)
{
  int l_s, s_seed_size;
  const opaque *s1, *s2;
  opaque s_seed[MAX_SEED_SIZE];
  opaque o1[MAX_PRF_BYTES], o2[MAX_PRF_BYTES];
  int result;
  enum MHD_GNUTLS_Protocol ver = MHD__gnutls_protocol_get_version (session);

  if (total_bytes > MAX_PRF_BYTES)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }
  /* label+seed = s_seed */
  s_seed_size = seed_size + label_size;

  if (s_seed_size > MAX_SEED_SIZE)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  memcpy (s_seed, label, label_size);
  memcpy (&s_seed[label_size], seed, seed_size);

  if (ver >= MHD_GNUTLS_PROTOCOL_TLS1_2)
    {
      result =
        MHD__gnutls_P_hash (MHD_GNUTLS_MAC_SHA1, secret, secret_size, s_seed,
                            s_seed_size, total_bytes, ret);
      if (result < 0)
        {
          MHD_gnutls_assert ();
          return result;
        }
    }
  else
    {
      l_s = secret_size / 2;

      s1 = &secret[0];
      s2 = &secret[l_s];

      if (secret_size % 2 != 0)
        {
          l_s++;
        }

      result =
        MHD__gnutls_P_hash (MHD_GNUTLS_MAC_MD5, s1, l_s, s_seed, s_seed_size,
                            total_bytes, o1);
      if (result < 0)
        {
          MHD_gnutls_assert ();
          return result;
        }

      result =
        MHD__gnutls_P_hash (MHD_GNUTLS_MAC_SHA1, s2, l_s, s_seed, s_seed_size,
                            total_bytes, o2);
      if (result < 0)
        {
          MHD_gnutls_assert ();
          return result;
        }

      MHD__gnutls_xor (o1, o2, total_bytes);

      memcpy (ret, o1, total_bytes);
    }

  return 0;                     /* ok */

}


/*-
 * MHD_gtls_session_is_export - Used to check whether this session is of export grade
 * @session: is a #MHD_gtls_session_t structure.
 *
 * This function will return non zero if this session is of export grade.
 *
 -*/
int
MHD_gtls_session_is_export (MHD_gtls_session_t session)
{
  enum MHD_GNUTLS_CipherAlgorithm cipher;

  cipher =
    MHD_gtls_cipher_suite_get_cipher_algo (&session->security_parameters.
                                           current_cipher_suite);

  if (MHD_gtls_cipher_get_export_flag (cipher) != 0)
    return 1;

  return 0;
}

/**
 * MHD__gnutls_record_get_direction - This function will return the direction of the last interrupted function call
 * @session: is a #MHD_gtls_session_t structure.
 *
 * This function provides information about the internals of the
 * record protocol and is only useful if a prior gnutls function call
 * (e.g.  MHD__gnutls_handshake()) was interrupted for some reason, that
 * is, if a function returned %GNUTLS_E_INTERRUPTED or
 * %GNUTLS_E_AGAIN.  In such a case, you might want to call select()
 * or poll() before calling the interrupted gnutls function again.
 * To tell you whether a file descriptor should be selected for
 * either reading or writing, MHD__gnutls_record_get_direction() returns 0
 * if the interrupted function was trying to read data, and 1 if it
 * was trying to write data.
 *
 * Returns: 0 if trying to read data, 1 if trying to write data.
 **/
int
MHD__gnutls_record_get_direction (MHD_gtls_session_t session)
{
  return session->internals.direction;
}
