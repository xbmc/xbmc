/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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

#include "gnutls_int.h"
#include "gnutls_errors.h"
#include "gnutls_auth.h"
#include "gnutls_auth_int.h"
#include "gnutls_algorithms.h"
#include "auth_cert.h"
#include <gnutls_datum.h>

/* The functions here are used in order for authentication algorithms
 * to be able to retrieve the needed credentials eg public and private
 * key etc.
 */

/**
  * MHD__gnutls_credentials_clear - Clears all the credentials previously set
  * @session: is a #MHD_gtls_session_t structure.
  *
  * Clears all the credentials previously set in this session.
  *
  **/
void
MHD__gnutls_credentials_clear (MHD_gtls_session_t session)
{
  if (session->key && session->key->cred)
    {                           /* beginning of the list */
      auth_cred_st *ccred, *ncred;
      ccred = session->key->cred;
      while (ccred != NULL)
        {
          ncred = ccred->next;
          MHD_gnutls_free (ccred);
          ccred = ncred;
        }
      session->key->cred = NULL;
    }
}

/*
 * This creates a linked list of the form:
 * { algorithm, credentials, pointer to next }
 */
/**
  * MHD__gnutls_credentials_set - Sets the needed credentials for the specified authentication algorithm.
  * @session: is a #MHD_gtls_session_t structure.
  * @type: is the type of the credentials
  * @cred: is a pointer to a structure.
  *
  * Sets the needed credentials for the specified type.
  * Eg username, password - or public and private keys etc.
  * The (void* cred) parameter is a structure that depends on the
  * specified type and on the current session (client or server).
  * [ In order to minimize memory usage, and share credentials between
  * several threads gnutls keeps a pointer to cred, and not the whole cred
  * structure. Thus you will have to keep the structure allocated until
  * you call MHD__gnutls_deinit(). ]
  *
  * For GNUTLS_CRD_SRP cred should be MHD_gnutls_srp_client_credentials_t
  * in case of a client, and MHD_gnutls_srp_server_credentials_t, in case
  * of a server.
  *
  * For GNUTLS_CRD_CERTIFICATE cred should be MHD_gtls_cert_credentials_t.
  *
  **/
int
MHD__gnutls_credentials_set (MHD_gtls_session_t session,
                             enum MHD_GNUTLS_CredentialsType type, void *cred)
{
  auth_cred_st *ccred = NULL, *pcred = NULL;
  int exists = 0;

  if (session->key->cred == NULL)
    {                           /* beginning of the list */

      session->key->cred = MHD_gnutls_malloc (sizeof (auth_cred_st));
      if (session->key->cred == NULL)
        return GNUTLS_E_MEMORY_ERROR;

      /* copy credentials locally */
      session->key->cred->credentials = cred;

      session->key->cred->next = NULL;
      session->key->cred->algorithm = type;
    }
  else
    {
      ccred = session->key->cred;
      while (ccred != NULL)
        {
          if (ccred->algorithm == type)
            {
              exists = 1;
              break;
            }
          pcred = ccred;
          ccred = ccred->next;
        }
      /* After this, pcred is not null.
       */

      if (exists == 0)
        {                       /* new entry */
          pcred->next = MHD_gnutls_malloc (sizeof (auth_cred_st));
          if (pcred->next == NULL)
            return GNUTLS_E_MEMORY_ERROR;

          ccred = pcred->next;

          /* copy credentials locally */
          ccred->credentials = cred;

          ccred->next = NULL;
          ccred->algorithm = type;
        }
      else
        {                       /* modify existing entry */
          MHD_gnutls_free (ccred->credentials);
          ccred->credentials = cred;
        }
    }

  return 0;
}

/**
  * MHD_gtls_auth_get_type - Returns the type of credentials for the current authentication schema.
  * @session: is a #MHD_gtls_session_t structure.
  *
  * Returns type of credentials for the current authentication schema.
  * The returned information is to be used to distinguish the function used
  * to access authentication data.
  *
  * Eg. for CERTIFICATE ciphersuites (key exchange algorithms: KX_RSA, KX_DHE_RSA),
  * the same function are to be used to access the authentication data.
  **/
enum MHD_GNUTLS_CredentialsType
MHD_gtls_auth_get_type (MHD_gtls_session_t session)
{
/* This is not the credentials we must set, but the authentication data
 * we get by the peer, so it should be reversed.
 */
  int server = session->security_parameters.entity == GNUTLS_SERVER ? 0 : 1;

  return
    MHD_gtls_map_kx_get_cred (MHD_gtls_cipher_suite_get_kx_algo
                              (&session->security_parameters.
                               current_cipher_suite), server);
}

/*
 * This returns a pointer to the linked list. Don't
 * free that!!!
 */
const void *
MHD_gtls_get_kx_cred (MHD_gtls_session_t session,
                      enum MHD_GNUTLS_KeyExchangeAlgorithm algo, int *err)
{
  int server = session->security_parameters.entity == GNUTLS_SERVER ? 1 : 0;

  return MHD_gtls_get_cred (session->key,
                            MHD_gtls_map_kx_get_cred (algo, server), err);
}

const void *
MHD_gtls_get_cred (MHD_gtls_key_st key, enum MHD_GNUTLS_CredentialsType type,
                   int *err)
{
  const void *retval = NULL;
  int _err = -1;
  auth_cred_st *ccred;

  if (key == NULL)
    goto out;

  ccred = key->cred;
  while (ccred != NULL)
    {
      if (ccred->algorithm == type)
        {
          break;
        }
      ccred = ccred->next;
    }
  if (ccred == NULL)
    goto out;

  _err = 0;
  retval = ccred->credentials;

out:
  if (err != NULL)
    *err = _err;
  return retval;
}

/*-
  * MHD_gtls_get_auth_info - Returns a pointer to authentication information.
  * @session: is a #MHD_gtls_session_t structure.
  *
  * This function must be called after a succesful MHD__gnutls_handshake().
  * Returns a pointer to authentication information. That information
  * is data obtained by the handshake protocol, the key exchange algorithm,
  * and the TLS extensions messages.
  *
  * In case of GNUTLS_CRD_CERTIFICATE returns a type of &cert_auth_info_t;
  * In case of GNUTLS_CRD_SRP returns a type of &srp_(server/client)_auth_info_t;
  -*/
void *
MHD_gtls_get_auth_info (MHD_gtls_session_t session)
{
  return session->key->auth_info;
}

/*-
  * MHD_gtls_free_auth_info - Frees the auth info structure
  * @session: is a #MHD_gtls_session_t structure.
  *
  * This function frees the auth info structure and sets it to
  * null. It must be called since some structures contain malloced
  * elements.
  -*/
void
MHD_gtls_free_auth_info (MHD_gtls_session_t session)
{
  MHD_gtls_dh_info_st *dh_info;
  rsa_info_st *rsa_info;

  if (session == NULL || session->key == NULL)
    {
      MHD_gnutls_assert ();
      return;
    }

  switch (session->key->auth_info_type)
    {
    case MHD_GNUTLS_CRD_CERTIFICATE:
      {
        unsigned int i;
        cert_auth_info_t info = MHD_gtls_get_auth_info (session);

        if (info == NULL)
          break;

        dh_info = &info->dh;
        rsa_info = &info->rsa_export;
        for (i = 0; i < info->ncerts; i++)
          {
            MHD__gnutls_free_datum (&info->raw_certificate_list[i]);
          }

        MHD_gnutls_free (info->raw_certificate_list);
        info->raw_certificate_list = NULL;
        info->ncerts = 0;

        MHD_gtls_free_dh_info (dh_info);
        MHD_gtls_free_rsa_info (rsa_info);
      }


      break;
    default:
      return;

    }

  MHD_gnutls_free (session->key->auth_info);
  session->key->auth_info = NULL;
  session->key->auth_info_size = 0;
  session->key->auth_info_type = 0;

}

/* This function will set the auth info structure in the key
 * structure.
 * If allow change is !=0 then this will allow changing the auth
 * info structure to a different type.
 */
int
MHD_gtls_auth_info_set (MHD_gtls_session_t session,
                        enum MHD_GNUTLS_CredentialsType type, int size,
                        int allow_change)
{
  if (session->key->auth_info == NULL)
    {
      session->key->auth_info = MHD_gnutls_calloc (1, size);
      if (session->key->auth_info == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }
      session->key->auth_info_type = type;
      session->key->auth_info_size = size;
    }
  else
    {
      if (allow_change == 0)
        {
          /* If the credentials for the current authentication scheme,
           * are not the one we want to set, then it's an error.
           * This may happen if a rehandshake is performed an the
           * ciphersuite which is negotiated has different authentication
           * schema.
           */
          if (MHD_gtls_auth_get_type (session) !=
              session->key->auth_info_type)
            {
              MHD_gnutls_assert ();
              return GNUTLS_E_INVALID_REQUEST;
            }
        }
      else
        {
          /* The new behaviour: Here we reallocate the auth info structure
           * in order to be able to negotiate different authentication
           * types. Ie. perform an auth_anon and then authenticate again using a
           * certificate (in order to prevent revealing the certificate's contents,
           * to passive eavesdropers.
           */
          if (MHD_gtls_auth_get_type (session) !=
              session->key->auth_info_type)
            {

              MHD_gtls_free_auth_info (session);

              session->key->auth_info = calloc (1, size);
              if (session->key->auth_info == NULL)
                {
                  MHD_gnutls_assert ();
                  return GNUTLS_E_MEMORY_ERROR;
                }

              session->key->auth_info_type = type;
              session->key->auth_info_size = size;
            }
        }
    }
  return 0;
}
