/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation
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
#include <libtasn1.h>
#include <gnutls_datum.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <gnutls_str.h>
#include <gnutls_x509.h>
#include <gnutls_num.h>
#include <x509_b64.h>
#include <common.h>
#include <mpi.h>
#include <time.h>

/* A generic export function. Will export the given ASN.1 encoded data
 * to PEM or DER raw data.
 */
int
MHD__gnutls_x509_export_int (ASN1_TYPE MHD__asn1_data,
                             MHD_gnutls_x509_crt_fmt_t format,
                             char *pem_header,
                             unsigned char *output_data,
                             size_t * output_data_size)
{
  int result, len;

  if (format == GNUTLS_X509_FMT_DER)
    {

      if (output_data == NULL)
        *output_data_size = 0;

      len = *output_data_size;

      if ((result =
           MHD__asn1_der_coding (MHD__asn1_data, "", output_data, &len,
                                 NULL)) != ASN1_SUCCESS)
        {
          *output_data_size = len;
          if (result == ASN1_MEM_ERROR)
            {
              return GNUTLS_E_SHORT_MEMORY_BUFFER;
            }
          MHD_gnutls_assert ();
          return MHD_gtls_asn2err (result);
        }

      *output_data_size = len;

    }
  else
    {                           /* PEM */
      opaque *out;
      MHD_gnutls_datum_t tmp;

      result = MHD__gnutls_x509_der_encode (MHD__asn1_data, "", &tmp, 0);
      if (result < 0)
        {
          MHD_gnutls_assert ();
          return result;
        }

      result =
        MHD__gnutls_fbase64_encode (pem_header, tmp.data, tmp.size, &out);

      MHD__gnutls_free_datum (&tmp);

      if (result < 0)
        {
          MHD_gnutls_assert ();
          return result;
        }

      if (result == 0)
        {                       /* oooops */
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }

      if ((unsigned) result > *output_data_size)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (out);
          *output_data_size = result;
          return GNUTLS_E_SHORT_MEMORY_BUFFER;
        }

      *output_data_size = result;

      if (output_data)
        {
          memcpy (output_data, out, result);

          /* do not include the null character into output size.
           */
          *output_data_size = result - 1;
        }
      MHD_gnutls_free (out);

    }

  return 0;
}

/* Decodes an octet string. Leave string_type null for a normal
 * octet string. Otherwise put something like BMPString, PrintableString
 * etc.
 */
static int
MHD__gnutls_x509_decode_octet_string (const char *string_type,
                                      const opaque * der,
                                      size_t der_size,
                                      opaque * output, size_t * output_size)
{
  ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
  int result, tmp_output_size;
  char strname[64];

  if (string_type == NULL)
    MHD_gtls_str_cpy (strname, sizeof (strname), "PKIX1.pkcs-7-Data");
  else
    {
      MHD_gtls_str_cpy (strname, sizeof (strname), "PKIX1.");
      MHD_gtls_str_cat (strname, sizeof (strname), string_type);
    }

  if ((result =
       MHD__asn1_create_element (MHD__gnutls_get_pkix (), strname,
                                 &c2)) != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  result = MHD__asn1_der_decoding (&c2, der, der_size, NULL);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  tmp_output_size = *output_size;
  result = MHD__asn1_read_value (c2, "", output, &tmp_output_size);
  *output_size = tmp_output_size;

  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  return 0;

cleanup:if (c2)
    MHD__asn1_delete_structure (&c2);

  return result;
}

/* Reads a value from an ASN1 tree, and puts the output
 * in an allocated variable in the given datum.
 * flags == 0 do nothing  with the DER output
 * flags == 1 parse the DER output as OCTET STRING
 * flags == 2 the value is a BIT STRING
 */
int
MHD__gnutls_x509_read_value (ASN1_TYPE c,
                             const char *root, MHD_gnutls_datum_t * ret,
                             int flags)
{
  int len = 0, result;
  size_t slen;
  opaque *tmp = NULL;

  result = MHD__asn1_read_value (c, root, NULL, &len);
  if (result != ASN1_MEM_ERROR)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      return result;
    }

  if (flags == 2)
    len /= 8;

  tmp = MHD_gnutls_malloc (len);
  if (tmp == NULL)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_MEMORY_ERROR;
      goto cleanup;
    }

  result = MHD__asn1_read_value (c, root, tmp, &len);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  if (flags == 2)
    len /= 8;

  /* Extract the OCTET STRING.
   */

  if (flags == 1)
    {
      slen = len;
      result =
        MHD__gnutls_x509_decode_octet_string (NULL, tmp, slen, tmp, &slen);
      if (result < 0)
        {
          MHD_gnutls_assert ();
          goto cleanup;
        }
      len = slen;
    }

  ret->data = tmp;
  ret->size = len;

  return 0;

cleanup:MHD_gnutls_free (tmp);
  return result;

}

/* DER Encodes the src ASN1_TYPE and stores it to
 * the given datum. If str is non null then the data are encoded as
 * an OCTET STRING.
 */
int
MHD__gnutls_x509_der_encode (ASN1_TYPE src,
                             const char *src_name, MHD_gnutls_datum_t * res,
                             int str)
{
  int size, result;
  int asize;
  opaque *data = NULL;
  ASN1_TYPE c2 = ASN1_TYPE_EMPTY;

  size = 0;
  result = MHD__asn1_der_coding (src, src_name, NULL, &size, NULL);
  if (result != ASN1_MEM_ERROR)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  /* allocate data for the der
   */

  if (str)
    size += 16;                 /* for later to include the octet tags */
  asize = size;

  data = MHD_gnutls_malloc (size);
  if (data == NULL)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_MEMORY_ERROR;
      goto cleanup;
    }

  result = MHD__asn1_der_coding (src, src_name, data, &size, NULL);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      result = MHD_gtls_asn2err (result);
      goto cleanup;
    }

  if (str)
    {
      if ((result =
           MHD__asn1_create_element (MHD__gnutls_get_pkix (),
                                     "PKIX1.pkcs-7-Data",
                                     &c2)) != ASN1_SUCCESS)
        {
          MHD_gnutls_assert ();
          result = MHD_gtls_asn2err (result);
          goto cleanup;
        }

      result = MHD__asn1_write_value (c2, "", data, size);
      if (result != ASN1_SUCCESS)
        {
          MHD_gnutls_assert ();
          result = MHD_gtls_asn2err (result);
          goto cleanup;
        }

      result = MHD__asn1_der_coding (c2, "", data, &asize, NULL);
      if (result != ASN1_SUCCESS)
        {
          MHD_gnutls_assert ();
          result = MHD_gtls_asn2err (result);
          goto cleanup;
        }

      size = asize;

      MHD__asn1_delete_structure (&c2);
    }

  res->data = data;
  res->size = size;
  return 0;

cleanup:MHD_gnutls_free (data);
  MHD__asn1_delete_structure (&c2);
  return result;

}

/* Reads and returns the PK algorithm of the given certificate-like
 * ASN.1 structure. src_name should be something like "tbsCertificate.subjectPublicKeyInfo".
 */
int
MHD__gnutls_x509_get_pk_algorithm (ASN1_TYPE src,
                                   const char *src_name, unsigned int *bits)
{
  int result;
  opaque *str = NULL;
  int algo;
  char oid[64];
  int len;
  mpi_t params[MAX_PUBLIC_PARAMS_SIZE];
  char name[128];

  MHD_gtls_str_cpy (name, sizeof (name), src_name);
  MHD_gtls_str_cat (name, sizeof (name), ".algorithm.algorithm");

  len = sizeof (oid);
  result = MHD__asn1_read_value (src, name, oid, &len);

  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  algo = MHD_gtls_x509_oid2pk_algorithm (oid);

  if (bits == NULL)
    {
      MHD_gnutls_free (str);
      return algo;
    }

  /* Now read the parameters' bits
   */
  MHD_gtls_str_cpy (name, sizeof (name), src_name);
  MHD_gtls_str_cat (name, sizeof (name), ".subjectPublicKey");

  len = 0;
  result = MHD__asn1_read_value (src, name, NULL, &len);
  if (result != ASN1_MEM_ERROR)
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  if (len % 8 != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_CERTIFICATE_ERROR;
    }

  len /= 8;

  str = MHD_gnutls_malloc (len);
  if (str == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  MHD_gtls_str_cpy (name, sizeof (name), src_name);
  MHD_gtls_str_cat (name, sizeof (name), ".subjectPublicKey");

  result = MHD__asn1_read_value (src, name, str, &len);

  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (str);
      return MHD_gtls_asn2err (result);
    }

  len /= 8;

  switch (algo)
    {
    case MHD_GNUTLS_PK_RSA:
      {
        if ((result =
             MHD__gnutls_x509_read_rsa_params (str, len, params)) < 0)
          {
            MHD_gnutls_assert ();
            return result;
          }

        bits[0] = MHD__gnutls_mpi_get_nbits (params[0]);

        MHD_gtls_mpi_release (&params[0]);
        MHD_gtls_mpi_release (&params[1]);
      }
      break;
    default:
      MHD__gnutls_x509_log
        ("MHD__gnutls_x509_get_pk_algorithm: unhandled algorithm %d\n", algo);
    }

  MHD_gnutls_free (str);
  return algo;
}
