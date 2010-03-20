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

#include <gnutls_int.h>
#include "gnutls_auth_int.h"
#include "gnutls_errors.h"
#include <gnutls_cert.h>
#include <auth_cert.h>
#include "gnutls_dh.h"
#include "gnutls_num.h"
#include "gnutls_datum.h"
#include <gnutls_pk.h>
#include <gnutls_algorithms.h>
#include <gnutls_global.h>
#include <gnutls_record.h>
#include <gnutls_sig.h>
#include <gnutls_state.h>
#include <gnutls_pk.h>
#include <gnutls_str.h>
#include <debug.h>
#include <x509_b64.h>
#include <gnutls_x509.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* x509 */
#include "common.h"
#include "x509.h"
#include "mpi.h"
#include "privkey.h"


/*
 * some x509 certificate parsing functions.
 */

/* Check if the number of bits of the key in the certificate
 * is unacceptable.
  */
inline static int
check_bits (MHD_gnutls_x509_crt_t crt, unsigned int max_bits)
{
  int ret;
  unsigned int bits;

  ret = MHD_gnutls_x509_crt_get_pk_algorithm (crt, &bits);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  if (bits > max_bits && max_bits > 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_CONSTRAINT_ERROR;
    }

  return 0;
}


#define CLEAR_CERTS for(x=0;x<peer_certificate_list_size;x++) { \
	if (peer_certificate_list[x]) \
		MHD_gnutls_x509_crt_deinit(peer_certificate_list[x]); \
	} \
	MHD_gnutls_free( peer_certificate_list)

/*
 * Read certificates and private keys, from memory etc.
 */

/* returns error if the certificate has different algorithm than
 * the given key parameters.
 */
static int
MHD__gnutls_check_key_cert_match (MHD_gtls_cert_credentials_t res)
{
  MHD_gnutls_datum_t cid;
  MHD_gnutls_datum_t kid;
  unsigned pk = res->cert_list[res->ncerts - 1][0].subject_pk_algorithm;

  if (res->pkey[res->ncerts - 1].pk_algorithm != pk)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
    }

  MHD__gnutls_x509_write_rsa_params (res->pkey[res->ncerts - 1].params,
                                     res->pkey[res->ncerts -
                                               1].params_size, &kid);


  MHD__gnutls_x509_write_rsa_params (res->
                                     cert_list[res->ncerts - 1][0].params,
                                     res->cert_list[res->ncerts -
                                                    1][0].params_size, &cid);

  if (cid.size != kid.size)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_free_datum (&kid);
      MHD__gnutls_free_datum (&cid);
      return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
    }

  if (memcmp (kid.data, cid.data, kid.size) != 0)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_free_datum (&kid);
      MHD__gnutls_free_datum (&cid);
      return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
    }

  MHD__gnutls_free_datum (&kid);
  MHD__gnutls_free_datum (&cid);
  return 0;
}

/* Reads a DER encoded certificate list from memory and stores it to
 * a MHD_gnutls_cert structure.
 * Returns the number of certificates parsed.
 */
static int
parse_crt_mem (MHD_gnutls_cert ** cert_list, unsigned *ncerts,
               MHD_gnutls_x509_crt_t cert)
{
  int i;
  int ret;

  i = *ncerts + 1;

  *cert_list =
    (MHD_gnutls_cert *) MHD_gtls_realloc_fast (*cert_list,
                                               i * sizeof (MHD_gnutls_cert));

  if (*cert_list == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  ret = MHD_gtls_x509_crt_to_gcert (&cert_list[0][i - 1], cert, 0);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  *ncerts = i;

  return 1;                     /* one certificate parsed */
}

/* Reads a DER encoded certificate list from memory and stores it to
 * a MHD_gnutls_cert structure.
 * Returns the number of certificates parsed.
 */
static int
parse_der_cert_mem (MHD_gnutls_cert ** cert_list, unsigned *ncerts,
                    const void *input_cert, int input_cert_size)
{
  MHD_gnutls_datum_t tmp;
  MHD_gnutls_x509_crt_t cert;
  int ret;

  ret = MHD_gnutls_x509_crt_init (&cert);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  tmp.data = (opaque *) input_cert;
  tmp.size = input_cert_size;

  ret = MHD_gnutls_x509_crt_import (cert, &tmp, GNUTLS_X509_FMT_DER);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_x509_crt_deinit (cert);
      return ret;
    }

  ret = parse_crt_mem (cert_list, ncerts, cert);
  MHD_gnutls_x509_crt_deinit (cert);

  return ret;
}

/* Reads a base64 encoded certificate list from memory and stores it to
 * a MHD_gnutls_cert structure. Returns the number of certificate parsed.
 */
static int
parse_pem_cert_mem (MHD_gnutls_cert ** cert_list, unsigned *ncerts,
                    const char *input_cert, int input_cert_size)
{
  int size, siz2, i;
  const char *ptr;
  opaque *ptr2;
  MHD_gnutls_datum_t tmp;
  int ret, count;

  /* move to the certificate
   */
  ptr = memmem (input_cert, input_cert_size,
                PEM_CERT_SEP, sizeof (PEM_CERT_SEP) - 1);
  if (ptr == NULL)
    ptr = memmem (input_cert, input_cert_size,
                  PEM_CERT_SEP2, sizeof (PEM_CERT_SEP2) - 1);

  if (ptr == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_BASE64_DECODING_ERROR;
    }
  size = input_cert_size - (ptr - input_cert);

  i = *ncerts + 1;
  count = 0;

  do
    {

      siz2 =
        MHD__gnutls_fbase64_decode (NULL, (const unsigned char *) ptr, size,
                                    &ptr2);

      if (siz2 < 0)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_BASE64_DECODING_ERROR;
        }

      *cert_list =
        (MHD_gnutls_cert *) MHD_gtls_realloc_fast (*cert_list,
                                                   i *
                                                   sizeof (MHD_gnutls_cert));

      if (*cert_list == NULL)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_MEMORY_ERROR;
        }

      tmp.data = ptr2;
      tmp.size = siz2;

      ret = MHD_gtls_x509_raw_cert_to_gcert (&cert_list[0][i - 1], &tmp, 0);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
      MHD__gnutls_free_datum (&tmp);    /* free ptr2 */

      /* now we move ptr after the pem header
       */
      ptr++;
      /* find the next certificate (if any)
       */
      size = input_cert_size - (ptr - input_cert);

      if (size > 0)
        {
          char *ptr3;

          ptr3 = memmem (ptr, size, PEM_CERT_SEP, sizeof (PEM_CERT_SEP) - 1);
          if (ptr3 == NULL)
            ptr3 = memmem (ptr, size, PEM_CERT_SEP2,
                           sizeof (PEM_CERT_SEP2) - 1);

          ptr = ptr3;
        }
      else
        ptr = NULL;

      i++;
      count++;

    }
  while (ptr != NULL);

  *ncerts = i - 1;

  return count;
}



/* Reads a DER or PEM certificate from memory
 */
static int
read_cert_mem (MHD_gtls_cert_credentials_t res, const void *cert,
               int cert_size, MHD_gnutls_x509_crt_fmt_t type)
{
  int ret;

  /* allocate space for the certificate to add
   */
  res->cert_list = MHD_gtls_realloc_fast (res->cert_list,
                                          (1 +
                                           res->ncerts) *
                                          sizeof (MHD_gnutls_cert *));
  if (res->cert_list == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  res->cert_list_length = MHD_gtls_realloc_fast (res->cert_list_length,
                                                 (1 +
                                                  res->ncerts) *
                                                 sizeof (int));
  if (res->cert_list_length == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  res->cert_list[res->ncerts] = NULL;   /* for realloc */
  res->cert_list_length[res->ncerts] = 0;

  if (type == GNUTLS_X509_FMT_DER)
    ret = parse_der_cert_mem (&res->cert_list[res->ncerts],
                              &res->cert_list_length[res->ncerts],
                              cert, cert_size);
  else
    ret =
      parse_pem_cert_mem (&res->cert_list[res->ncerts],
                          &res->cert_list_length[res->ncerts], cert,
                          cert_size);

  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return ret;
}


int
MHD__gnutls_x509_privkey_to_gkey (MHD_gnutls_privkey * dest,
                                  MHD_gnutls_x509_privkey_t src)
{
  int i, ret;

  memset (dest, 0, sizeof (MHD_gnutls_privkey));

  for (i = 0; i < src->params_size; i++)
    {
      dest->params[i] = MHD__gnutls_mpi_copy (src->params[i]);
      if (dest->params[i] == NULL)
        {
          MHD_gnutls_assert ();
          ret = GNUTLS_E_MEMORY_ERROR;
          goto cleanup;
        }
    }

  dest->pk_algorithm = src->pk_algorithm;
  dest->params_size = src->params_size;

  return 0;

cleanup:

  for (i = 0; i < src->params_size; i++)
    {
      MHD_gtls_mpi_release (&dest->params[i]);
    }
  return ret;
}

void
MHD_gtls_gkey_deinit (MHD_gnutls_privkey * key)
{
  int i;
  if (key == NULL)
    return;

  for (i = 0; i < key->params_size; i++)
    {
      MHD_gtls_mpi_release (&key->params[i]);
    }
}

int
MHD__gnutls_x509_raw_privkey_to_gkey (MHD_gnutls_privkey * privkey,
                                      const MHD_gnutls_datum_t * raw_key,
                                      MHD_gnutls_x509_crt_fmt_t type)
{
  MHD_gnutls_x509_privkey_t tmpkey;
  int ret;

  ret = MHD_gnutls_x509_privkey_init (&tmpkey);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  ret = MHD_gnutls_x509_privkey_import (tmpkey, raw_key, type);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_x509_privkey_deinit (tmpkey);
      return ret;
    }

  ret = MHD__gnutls_x509_privkey_to_gkey (privkey, tmpkey);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_x509_privkey_deinit (tmpkey);
      return ret;
    }

  MHD_gnutls_x509_privkey_deinit (tmpkey);

  return 0;
}

/* Reads a PEM encoded PKCS-1 RSA/DSA private key from memory.  Type
 * indicates the certificate format.  KEY can be NULL, to indicate
 * that GnuTLS doesn't know the private key.
 */
static int
read_key_mem (MHD_gtls_cert_credentials_t res,
              const void *key, int key_size, MHD_gnutls_x509_crt_fmt_t type)
{
  int ret;
  MHD_gnutls_datum_t tmp;

  /* allocate space for the pkey list
   */
  res->pkey =
    MHD_gtls_realloc_fast (res->pkey,
                           (res->ncerts + 1) * sizeof (MHD_gnutls_privkey));
  if (res->pkey == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  if (key)
    {
      tmp.data = (opaque *) key;
      tmp.size = key_size;

      ret =
        MHD__gnutls_x509_raw_privkey_to_gkey (&res->pkey[res->ncerts], &tmp,
                                              type);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }
    }
  else
    memset (&res->pkey[res->ncerts], 0, sizeof (MHD_gnutls_privkey));

  return 0;
}

/**
  * MHD__gnutls_certificate_set_x509_key_mem - Used to set keys in a MHD_gtls_cert_credentials_t structure
  * @res: is an #MHD_gtls_cert_credentials_t structure.
  * @cert: contains a certificate list (path) for the specified private key
  * @key: is the private key, or %NULL
  * @type: is PEM or DER
  *
  * This function sets a certificate/private key pair in the
  * MHD_gtls_cert_credentials_t structure. This function may be called
  * more than once (in case multiple keys/certificates exist for the
  * server).
  *
  * Currently are supported: RSA PKCS-1 encoded private keys,
  * DSA private keys.
  *
  * DSA private keys are encoded the OpenSSL way, which is an ASN.1
  * DER sequence of 6 INTEGERs - version, p, q, g, pub, priv.
  *
  * Note that the keyUsage (2.5.29.15) PKIX extension in X.509 certificates
  * is supported. This means that certificates intended for signing cannot
  * be used for ciphersuites that require encryption.
  *
  * If the certificate and the private key are given in PEM encoding
  * then the strings that hold their values must be null terminated.
  *
  * The @key may be %NULL if you are using a sign callback, see
  * MHD_gtls_sign_callback_set().
  *
  * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
  **/
int
MHD__gnutls_certificate_set_x509_key_mem (MHD_gtls_cert_credentials_t
                                          res,
                                          const MHD_gnutls_datum_t * cert,
                                          const MHD_gnutls_datum_t * key,
                                          MHD_gnutls_x509_crt_fmt_t type)
{
  int ret;

  /* this should be first
   */
  if ((ret = read_key_mem (res, key ? key->data : NULL,
                           key ? key->size : 0, type)) < 0)
    return ret;

  if ((ret = read_cert_mem (res, cert->data, cert->size, type)) < 0)
    return ret;

  res->ncerts++;

  if (key && (ret = MHD__gnutls_check_key_cert_match (res)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return 0;
}

/* Returns 0 if it's ok to use the enum MHD_GNUTLS_KeyExchangeAlgorithm with this
 * certificate (uses the KeyUsage field).
 */
int
MHD__gnutls_check_key_usage (const MHD_gnutls_cert * cert,
                             enum MHD_GNUTLS_KeyExchangeAlgorithm alg)
{
  unsigned int key_usage = 0;
  int encipher_type;

  if (cert == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  if (MHD_gtls_map_kx_get_cred (alg, 1) == MHD_GNUTLS_CRD_CERTIFICATE ||
      MHD_gtls_map_kx_get_cred (alg, 0) == MHD_GNUTLS_CRD_CERTIFICATE)
    {

      key_usage = cert->key_usage;

      encipher_type = MHD_gtls_kx_encipher_type (alg);

      if (key_usage != 0 && encipher_type != CIPHER_IGN)
        {
          /* If key_usage has been set in the certificate
           */

          if (encipher_type == CIPHER_ENCRYPT)
            {
              /* If the key exchange method requires an encipher
               * type algorithm, and key's usage does not permit
               * encipherment, then fail.
               */
              if (!(key_usage & KEY_KEY_ENCIPHERMENT))
                {
                  MHD_gnutls_assert ();
                  return GNUTLS_E_KEY_USAGE_VIOLATION;
                }
            }

          if (encipher_type == CIPHER_SIGN)
            {
              /* The same as above, but for sign only keys
               */
              if (!(key_usage & KEY_DIGITAL_SIGNATURE))
                {
                  MHD_gnutls_assert ();
                  return GNUTLS_E_KEY_USAGE_VIOLATION;
                }
            }
        }
    }
  return 0;
}
