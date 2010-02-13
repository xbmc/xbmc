/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation
 * Author: Nikos Mavrogiannopoulos, Simon Josefsson, Howard Chu
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

/* Functions on X.509 Certificate parsing
 */

#include <gnutls_int.h>
#include <gnutls_datum.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <common.h>
#include <gnutls_x509.h>
#include <x509_b64.h>
#include <x509.h>
#include <extensions.h>
#include <libtasn1.h>
#include <mpi.h>
#include <privkey.h>

/**
 * MHD_gnutls_x509_crt_init - This function initializes a MHD_gnutls_x509_crt_t structure
 * @cert: The structure to be initialized
 *
 * This function will initialize an X.509 certificate structure.
 *
 * Returns 0 on success.
 *
 **/
int
MHD_gnutls_x509_crt_init (MHD_gnutls_x509_crt_t * cert)
{
  MHD_gnutls_x509_crt_t tmp =
    MHD_gnutls_calloc (1, sizeof (MHD_gnutls_x509_crt_int));
  int result;

  if (!tmp)
    return GNUTLS_E_MEMORY_ERROR;

  result = MHD__asn1_create_element (MHD__gnutls_get_pkix (),
                                     "PKIX1.Certificate", &tmp->cert);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (tmp);
      return MHD_gtls_asn2err (result);
    }

  *cert = tmp;

  return 0;                     /* success */
}

/**
 * MHD_gnutls_x509_crt_deinit - This function deinitializes memory used by a MHD_gnutls_x509_crt_t structure
 * @cert: The structure to be initialized
 *
 * This function will deinitialize a CRL structure.
 *
 **/
void
MHD_gnutls_x509_crt_deinit (MHD_gnutls_x509_crt_t cert)
{
  if (!cert)
    return;

  if (cert->cert)
    MHD__asn1_delete_structure (&cert->cert);

  MHD_gnutls_free (cert);
}

/**
 * MHD_gnutls_x509_crt_import - This function will import a DER or PEM encoded Certificate
 * @cert: The structure to store the parsed certificate.
 * @data: The DER or PEM encoded certificate.
 * @format: One of DER or PEM
 *
 * This function will convert the given DER or PEM encoded Certificate
 * to the native MHD_gnutls_x509_crt_t format. The output will be stored in @cert.
 *
 * If the Certificate is PEM encoded it should have a header of "X509 CERTIFICATE", or
 * "CERTIFICATE".
 *
 * Returns 0 on success.
 *
 **/
int
MHD_gnutls_x509_crt_import (MHD_gnutls_x509_crt_t cert,
                            const MHD_gnutls_datum_t * data,
                            MHD_gnutls_x509_crt_fmt_t format)
{
  int result = 0, need_free = 0;
  MHD_gnutls_datum_t _data;
  opaque *signature = NULL;

  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  _data.data = data->data;
  _data.size = data->size;

  /* If the Certificate is in PEM format then decode it
   */
  if (format == GNUTLS_X509_FMT_PEM)
    {
      opaque *out;

      /* Try the first header */
      result =
        MHD__gnutls_fbase64_decode (PEM_X509_CERT2, data->data, data->size,
                                    &out);

      if (result <= 0)
        {
          /* try for the second header */
          result = MHD__gnutls_fbase64_decode (PEM_X509_CERT, data->data,
                                               data->size, &out);

          if (result <= 0)
            {
              if (result == 0)
                result = GNUTLS_E_INTERNAL_ERROR;
              MHD_gnutls_assert ();
              return result;
            }
        }

      _data.data = out;
      _data.size = result;

      need_free = 1;
    }

  result = MHD__asn1_der_decoding (&cert->cert, _data.data, _data.size, NULL);
  if (result != ASN1_SUCCESS)
    {
      result = MHD_gtls_asn2err (result);
      MHD_gnutls_assert ();
      goto cleanup;
    }

  /* Since we do not want to disable any extension
   */
  cert->use_extensions = 1;
  if (need_free)
    MHD__gnutls_free_datum (&_data);

  return 0;

cleanup:MHD_gnutls_free (signature);
  if (need_free)
    MHD__gnutls_free_datum (&_data);
  return result;
}

/**
 * MHD_gnutls_x509_crt_get_version - This function returns the Certificate's version number
 * @cert: should contain a MHD_gnutls_x509_crt_t structure
 *
 * This function will return the version of the specified Certificate.
 *
 * Returns a negative value on error.
 *
 **/
int
MHD_gnutls_x509_crt_get_version (MHD_gnutls_x509_crt_t cert)
{
  opaque version[5];
  int len, result;

  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  len = sizeof (version);
  if ((result =
       MHD__asn1_read_value (cert->cert, "tbsCertificate.version", version,
                             &len)) != ASN1_SUCCESS)
    {

      if (result == ASN1_ELEMENT_NOT_FOUND)
        return 1;               /* the DEFAULT version */
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  return (int) version[0] + 1;
}

/**
 * MHD_gnutls_x509_crt_get_pk_algorithm - This function returns the certificate's PublicKey algorithm
 * @cert: should contain a MHD_gnutls_x509_crt_t structure
 * @bits: if bits is non null it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of an X.509
 * certificate.
 *
 * If bits is non null, it should have enough size to hold the parameters
 * size in bits. For RSA the bits returned is the modulus.
 * For DSA the bits returned are of the public
 * exponent.
 *
 * Returns a member of the enum MHD_GNUTLS_PublicKeyAlgorithm enumeration on success,
 * or a negative value on error.
 *
 **/
int
MHD_gnutls_x509_crt_get_pk_algorithm (MHD_gnutls_x509_crt_t cert,
                                      unsigned int *bits)
{
  int result;

  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  result = MHD__gnutls_x509_get_pk_algorithm (cert->cert,
                                              "tbsCertificate.subjectPublicKeyInfo",
                                              bits);

  if (result < 0)
    {
      MHD_gnutls_assert ();
      return result;
    }

  return result;

}

inline static int
is_type_printable (int type)
{
  if (type == GNUTLS_SAN_DNSNAME || type == GNUTLS_SAN_RFC822NAME || type
      == GNUTLS_SAN_URI)
    return 1;
  else
    return 0;
}

/**
 * MHD_gnutls_x509_crt_get_key_usage - This function returns the certificate's key usage
 * @cert: should contain a MHD_gnutls_x509_crt_t structure
 * @key_usage: where the key usage bits will be stored
 * @critical: will be non zero if the extension is marked as critical
 *
 * This function will return certificate's key usage, by reading the
 * keyUsage X.509 extension (2.5.29.15). The key usage value will ORed values of the:
 * GNUTLS_KEY_DIGITAL_SIGNATURE, GNUTLS_KEY_NON_REPUDIATION,
 * GNUTLS_KEY_KEY_ENCIPHERMENT, GNUTLS_KEY_DATA_ENCIPHERMENT,
 * GNUTLS_KEY_KEY_AGREEMENT, GNUTLS_KEY_KEY_CERT_SIGN,
 * GNUTLS_KEY_CRL_SIGN, GNUTLS_KEY_ENCIPHER_ONLY,
 * GNUTLS_KEY_DECIPHER_ONLY.
 *
 * A negative value may be returned in case of parsing error.
 * If the certificate does not contain the keyUsage extension
 * GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will be returned.
 *
 **/
int
MHD_gnutls_x509_crt_get_key_usage (MHD_gnutls_x509_crt_t cert,
                                   unsigned int *key_usage,
                                   unsigned int *critical)
{
  int result;
  MHD_gnutls_datum_t keyUsage;
  uint16_t _usage;

  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if ((result =
       MHD__gnutls_x509_crt_get_extension (cert, "2.5.29.15", 0, &keyUsage,
                                           critical)) < 0)
    {
      return result;
    }

  if (keyUsage.size == 0 || keyUsage.data == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
    }

  result = MHD__gnutls_x509_ext_extract_keyUsage (&_usage, keyUsage.data,
                                                  keyUsage.size);
  MHD__gnutls_free_datum (&keyUsage);

  *key_usage = _usage;

  if (result < 0)
    {
      MHD_gnutls_assert ();
      return result;
    }

  return 0;
}


/**
 * MHD_gnutls_x509_crt_export - This function will export the certificate
 * @cert: Holds the certificate
 * @format: the format of output params. One of PEM or DER.
 * @output_data: will contain a certificate PEM or DER encoded
 * @output_data_size: holds the size of output_data (and will be
 *   replaced by the actual size of parameters)
 *
 * This function will export the certificate to DER or PEM format.
 *
 * If the buffer provided is not long enough to hold the output, then
 * *output_data_size is updated and GNUTLS_E_SHORT_MEMORY_BUFFER will
 * be returned.
 *
 * If the structure is PEM encoded, it will have a header
 * of "BEGIN CERTIFICATE".
 *
 * Return value: In case of failure a negative value will be
 *   returned, and 0 on success.
 **/
int
MHD_gnutls_x509_crt_export (MHD_gnutls_x509_crt_t cert,
                            MHD_gnutls_x509_crt_fmt_t format,
                            void *output_data, size_t * output_data_size)
{
  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  return MHD__gnutls_x509_export_int (cert->cert, format, "CERTIFICATE",
                                      output_data, output_data_size);
}
