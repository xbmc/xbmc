/*
 * Copyright (C) 2003, 2004, 2005, 2007 Free Software Foundation
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
#include <gnutls_datum.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <gnutls_rsa_export.h>
#include <gnutls_sig.h>
#include <common.h>
#include <gnutls_x509.h>
#include <x509_b64.h>
#include <x509.h>
#include <mpi.h>
#include <extensions.h>

/* remove this when libgcrypt can handle the PKCS #1 coefficients from
 * rsa keys
 */
#define CALC_COEFF 1

/**
 * MHD_gnutls_x509_privkey_init - This function initializes a MHD_gnutls_crl structure
 * @key: The structure to be initialized
 *
 * This function will initialize an private key structure.
 *
 * Returns 0 on success.
 *
 **/
int
MHD_gnutls_x509_privkey_init (MHD_gnutls_x509_privkey_t * key)
{
  *key = MHD_gnutls_calloc (1, sizeof (MHD_gnutls_x509_privkey_int));

  if (*key)
    {
      (*key)->key = ASN1_TYPE_EMPTY;
      (*key)->pk_algorithm = MHD_GNUTLS_PK_UNKNOWN;
      return 0;                 /* success */
    }

  return GNUTLS_E_MEMORY_ERROR;
}

/**
 * MHD_gnutls_x509_privkey_deinit - This function deinitializes memory used by a MHD_gnutls_x509_privkey_t structure
 * @key: The structure to be initialized
 *
 * This function will deinitialize a private key structure.
 *
 **/
void
MHD_gnutls_x509_privkey_deinit (MHD_gnutls_x509_privkey_t key)
{
  int i;

  if (!key)
    return;

  for (i = 0; i < key->params_size; i++)
    {
      MHD_gtls_mpi_release (&key->params[i]);
    }

  MHD__asn1_delete_structure (&key->key);
  MHD_gnutls_free (key);
}


/* Converts an RSA PKCS#1 key to
 * an internal structure (MHD_gnutls_private_key)
 */
ASN1_TYPE
MHD__gnutls_privkey_decode_pkcs1_rsa_key (const MHD_gnutls_datum_t * raw_key,
                                          MHD_gnutls_x509_privkey_t pkey)
{
  int result;
  ASN1_TYPE pkey_asn;

  if ((result = MHD__asn1_create_element (MHD__gnutls_getMHD__gnutls_asn (),
                                          "GNUTLS.RSAPrivateKey",
                                          &pkey_asn)) != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      return NULL;
    }

  if ((sizeof (pkey->params) / sizeof (mpi_t)) < RSA_PRIVATE_PARAMS)
    {
      MHD_gnutls_assert ();
      /* internal error. Increase the mpi_ts in params */
      return NULL;
    }

  result =
    MHD__asn1_der_decoding (&pkey_asn, raw_key->data, raw_key->size, NULL);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  if ((result =
       MHD__gnutls_x509_read_int (pkey_asn, "modulus", &pkey->params[0])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  if ((result = MHD__gnutls_x509_read_int (pkey_asn, "publicExponent",
                                           &pkey->params[1])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  if ((result = MHD__gnutls_x509_read_int (pkey_asn, "privateExponent",
                                           &pkey->params[2])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  if ((result =
       MHD__gnutls_x509_read_int (pkey_asn, "prime1", &pkey->params[3])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  if ((result =
       MHD__gnutls_x509_read_int (pkey_asn, "prime2", &pkey->params[4])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

#ifdef CALC_COEFF
  /* Calculate the coefficient. This is because the gcrypt
   * library is uses the p,q in the reverse order.
   */
  pkey->params[5] =
    MHD__gnutls_mpi_snew (MHD__gnutls_mpi_get_nbits (pkey->params[0]));

  if (pkey->params[5] == NULL)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  MHD__gnutls_mpi_invm (pkey->params[5], pkey->params[3], pkey->params[4]);
  /* p, q */
#else
  if ((result = MHD__gnutls_x509_read_int (pkey_asn, "coefficient",
                                           &pkey->params[5])) < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }
#endif
  pkey->params_size = 6;

  return pkey_asn;

error:MHD__asn1_delete_structure (&pkey_asn);
  MHD_gtls_mpi_release (&pkey->params[0]);
  MHD_gtls_mpi_release (&pkey->params[1]);
  MHD_gtls_mpi_release (&pkey->params[2]);
  MHD_gtls_mpi_release (&pkey->params[3]);
  MHD_gtls_mpi_release (&pkey->params[4]);
  MHD_gtls_mpi_release (&pkey->params[5]);
  return NULL;

}

#define PEM_KEY_RSA "RSA PRIVATE KEY"

/**
 * MHD_gnutls_x509_privkey_import - This function will import a DER or PEM encoded key
 * @key: The structure to store the parsed key
 * @data: The DER or PEM encoded certificate.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded key
 * to the native MHD_gnutls_x509_privkey_t format. The output will be stored in @key .
 *
 * If the key is PEM encoded it should have a header of "RSA PRIVATE KEY", or
 * "DSA PRIVATE KEY".
 *
 * Returns 0 on success.
 *
 **/
int
MHD_gnutls_x509_privkey_import (MHD_gnutls_x509_privkey_t key,
                                const MHD_gnutls_datum_t * data,
                                MHD_gnutls_x509_crt_fmt_t format)
{
  int result = 0, need_free = 0;
  MHD_gnutls_datum_t _data;

  if (key == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  _data.data = data->data;
  _data.size = data->size;

  key->pk_algorithm = MHD_GNUTLS_PK_UNKNOWN;

  /* If the Certificate is in PEM format then decode it */
  if (format == GNUTLS_X509_FMT_PEM)
    {
      opaque *out;

      /* Try the first header */
      result
        =
        MHD__gnutls_fbase64_decode (PEM_KEY_RSA, data->data, data->size,
                                    &out);
      key->pk_algorithm = MHD_GNUTLS_PK_RSA;

      _data.data = out;
      _data.size = result;

      need_free = 1;
    }

  if (key->pk_algorithm == MHD_GNUTLS_PK_RSA)
    {
      key->key = MHD__gnutls_privkey_decode_pkcs1_rsa_key (&_data, key);
      if (key->key == NULL)
        MHD_gnutls_assert ();
    }
  else
    {
      /* Try decoding with both, and accept the one that succeeds. */
      key->pk_algorithm = MHD_GNUTLS_PK_RSA;
      key->key = MHD__gnutls_privkey_decode_pkcs1_rsa_key (&_data, key);

      // TODO rm
//      if (key->key == NULL)
//        {
//          key->pk_algorithm = GNUTLS_PK_DSA;
//          key->key = decode_dsa_key(&_data, key);
//          if (key->key == NULL)
//            MHD_gnutls_assert();
//        }
    }

  if (key->key == NULL)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_ASN1_DER_ERROR;
      key->pk_algorithm = MHD_GNUTLS_PK_UNKNOWN;
      return result;
    }

  if (need_free)
    MHD__gnutls_free_datum (&_data);

  /* The key has now been decoded.
   */

  return 0;
}
