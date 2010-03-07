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

/* Functions that relate to the X.509 extension parsing.
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <gnutls_global.h>
#include <mpi.h>
#include <libtasn1.h>
#include <common.h>
#include <x509.h>
#include <extensions.h>
#include <gnutls_datum.h>

/* This function will attempt to return the requested extension found in
 * the given X509v3 certificate. The return value is allocated and stored into
 * ret.
 *
 * Critical will be either 0 or 1.
 *
 * If the extension does not exist, GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE will
 * be returned.
 */
int
MHD__gnutls_x509_crt_get_extension (MHD_gnutls_x509_crt_t cert,
                                    const char *extension_id, int indx,
                                    MHD_gnutls_datum_t * ret,
                                    unsigned int *_critical)
{
  int k, result, len;
  char name[MAX_NAME_SIZE], name2[MAX_NAME_SIZE];
  char str[1024];
  char str_critical[10];
  int critical = 0;
  char extnID[128];
  MHD_gnutls_datum_t value;
  int indx_counter = 0;

  ret->data = NULL;
  ret->size = 0;

  k = 0;
  do
    {
      k++;

      snprintf (name, sizeof (name), "tbsCertificate.extensions.?%u", k);

      len = sizeof (str) - 1;
      result = MHD__asn1_read_value (cert->cert, name, str, &len);

      /* move to next
       */

      if (result == ASN1_ELEMENT_NOT_FOUND)
        {
          break;
        }

      do
        {

          MHD_gtls_str_cpy (name2, sizeof (name2), name);
          MHD_gtls_str_cat (name2, sizeof (name2), ".extnID");

          len = sizeof (extnID) - 1;
          result = MHD__asn1_read_value (cert->cert, name2, extnID, &len);

          if (result == ASN1_ELEMENT_NOT_FOUND)
            {
              MHD_gnutls_assert ();
              break;
            }
          else if (result != ASN1_SUCCESS)
            {
              MHD_gnutls_assert ();
              return MHD_gtls_asn2err (result);
            }

          /* Handle Extension
           */
          if (strcmp (extnID, extension_id) == 0 && indx == indx_counter++)
            {
              /* extension was found
               */

              /* read the critical status.
               */
              MHD_gtls_str_cpy (name2, sizeof (name2), name);
              MHD_gtls_str_cat (name2, sizeof (name2), ".critical");

              len = sizeof (str_critical);
              result =
                MHD__asn1_read_value (cert->cert, name2, str_critical, &len);

              if (result == ASN1_ELEMENT_NOT_FOUND)
                {
                  MHD_gnutls_assert ();
                  break;
                }
              else if (result != ASN1_SUCCESS)
                {
                  MHD_gnutls_assert ();
                  return MHD_gtls_asn2err (result);
                }

              if (str_critical[0] == 'T')
                critical = 1;
              else
                critical = 0;

              /* read the value.
               */
              MHD_gtls_str_cpy (name2, sizeof (name2), name);
              MHD_gtls_str_cat (name2, sizeof (name2), ".extnValue");

              result =
                MHD__gnutls_x509_read_value (cert->cert, name2, &value, 0);
              if (result < 0)
                {
                  MHD_gnutls_assert ();
                  return result;
                }

              ret->data = value.data;
              ret->size = value.size;

              if (_critical)
                *_critical = critical;

              return 0;
            }


        }
      while (0);
    }
  while (1);

  if (result == ASN1_ELEMENT_NOT_FOUND)
    {
      return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
    }
  else
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }
}

/* Here we only extract the KeyUsage field, from the DER encoded
 * extension.
 */
int
MHD__gnutls_x509_ext_extract_keyUsage (uint16_t * keyUsage,
                                       opaque * extnValue, int extnValueLen)
{
  ASN1_TYPE ext = ASN1_TYPE_EMPTY;
  int len, result;
  uint8_t str[2];

  str[0] = str[1] = 0;
  *keyUsage = 0;

  if ((result = MHD__asn1_create_element
       (MHD__gnutls_get_pkix (), "PKIX1.KeyUsage", &ext)) != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      return MHD_gtls_asn2err (result);
    }

  result = MHD__asn1_der_decoding (&ext, extnValue, extnValueLen, NULL);

  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      MHD__asn1_delete_structure (&ext);
      return MHD_gtls_asn2err (result);
    }

  len = sizeof (str);
  result = MHD__asn1_read_value (ext, "", str, &len);
  if (result != ASN1_SUCCESS)
    {
      MHD_gnutls_assert ();
      MHD__asn1_delete_structure (&ext);
      return 0;
    }

  *keyUsage = str[0] | (str[1] << 8);

  MHD__asn1_delete_structure (&ext);

  return 0;
}
