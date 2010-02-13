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
#include <gnutls_errors.h>
#include <gnutls_global.h>
#include <libtasn1.h>
#include <gnutls_datum.h>
#include "common.h"
#include "x509.h"
#include <gnutls_num.h>
#include "mpi.h"

/*
 * some x509 certificate parsing functions that relate to MPI parameter
 * extraction. This reads the BIT STRING subjectPublicKey.
 * Returns 2 parameters (m,e).
 */
int
MHD__gnutls_x509_read_rsa_params (opaque * der, int dersize, mpi_t * params)
{
  int result;
  ASN1_TYPE spk = ASN1_TYPE_EMPTY;

  if ((result =
       MHD__asn1_create_element (MHD__gnutls_getMHD__gnutls_asn (),
                                 "GNUTLS.RSAPublicKey",
                                 &spk)) != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  result = MHD__asn1_der_decoding (&spk, der, dersize, NULL);

  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      MHD__asn1_delete_structure (&spk);
      return MHD_gtls_asn2err (result);
    }

  if ((result = MHD__gnutls_x509_read_int (spk, "modulus", &params[0])) < 0)
    {
      MHD_gnutls_assert ();
      MHD__asn1_delete_structure (&spk);
      return GNUTLS_E_ASN1_GENERIC_ERROR;
    }

  if ((result =
       MHD__gnutls_x509_read_int (spk, "publicExponent", &params[1])) < 0)
    {
      MHD_gnutls_assert ();
      MHD_gtls_mpi_release (&params[0]);
      MHD__asn1_delete_structure (&spk);
      return GNUTLS_E_ASN1_GENERIC_ERROR;
    }

  MHD__asn1_delete_structure (&spk);

  return 0;

}


/* Extracts DSA and RSA parameters from a certificate.
 */
int
MHD__gnutls_x509_crt_get_mpis (MHD_gnutls_x509_crt_t cert,
                               mpi_t * params, int *params_size)
{
  int result;
  int pk_algorithm;
  MHD_gnutls_datum_t tmp = { NULL, 0 };

  /* Read the algorithm's OID
   */
  pk_algorithm = MHD_gnutls_x509_crt_get_pk_algorithm (cert, NULL);

  /* Read the algorithm's parameters
   */
  result
    = MHD__gnutls_x509_read_value (cert->cert,
                                   "tbsCertificate.subjectPublicKeyInfo.subjectPublicKey",
                                   &tmp, 2);

  if (result < 0)
    {
      MHD_gnutls_assert ();
      return result;
    }

  switch (pk_algorithm)
    {
    case MHD_GNUTLS_PK_RSA:
      /* params[0] is the modulus,
       * params[1] is the exponent
       */
      if (*params_size < RSA_PUBLIC_PARAMS)
        {
          MHD_gnutls_assert ();
          /* internal error. Increase the mpi_ts in params */
          result = GNUTLS_E_INTERNAL_ERROR;
          goto error;
        }

      if ((result =
           MHD__gnutls_x509_read_rsa_params (tmp.data, tmp.size, params)) < 0)
        {
          MHD_gnutls_assert ();
          goto error;
        }
      *params_size = RSA_PUBLIC_PARAMS;

      break;
    default:
      /* other types like DH
       * currently not supported
       */
      MHD_gnutls_assert ();
      result = GNUTLS_E_X509_CERTIFICATE_ERROR;
      goto error;
    }

  result = 0;

error:MHD__gnutls_free_datum (&tmp);
  return result;
}

/*
 * some x509 certificate functions that relate to MPI parameter
 * setting. This writes the BIT STRING subjectPublicKey.
 * Needs 2 parameters (m,e).
 *
 * Allocates the space used to store the DER data.
 */
int
MHD__gnutls_x509_write_rsa_params (mpi_t * params,
                                   int params_size, MHD_gnutls_datum_t * der)
{
  int result;
  ASN1_TYPE spk = ASN1_TYPE_EMPTY;

  der->data = NULL;
  der->size = 0;

  if (params_size < 2)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_INVALID_REQUEST;
      goto cleanup;
    }

  if ((result =
       MHD__asn1_create_element (MHD__gnutls_getMHD__gnutls_asn (),
                                 "GNUTLS.RSAPublicKey",
                                 &spk)) != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  result = MHD__gnutls_x509_write_int (spk, "modulus", params[0], 0);
  if (result < 0)
    {
      MHD_gnutls_assert ();
      goto cleanup;
    }

  result = MHD__gnutls_x509_write_int (spk, "publicExponent", params[1], 0);
  if (result < 0)
    {
      MHD_gnutls_assert ();
      goto cleanup;
    }

  result = MHD__gnutls_x509_der_encode (spk, "", der, 0);
  if (result < 0)
    {
      MHD_gnutls_assert ();
      goto cleanup;
    }

  MHD__asn1_delete_structure (&spk);
  return 0;

cleanup:MHD__asn1_delete_structure (&spk);

  return result;
}
