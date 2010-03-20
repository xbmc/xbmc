/*
 * Copyright (C) 2000, 2004, 2005 Free Software Foundation
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
#include <gnutls_cipher_int.h>
#include <gnutls_datum.h>

cipher_hd_t
MHD_gtls_cipher_init (enum MHD_GNUTLS_CipherAlgorithm cipher,
                      const MHD_gnutls_datum_t * key,
                      const MHD_gnutls_datum_t * iv)
{
  cipher_hd_t ret = NULL;
  int err = GC_INVALID_CIPHER;  /* doesn't matter */

  switch (cipher)
    {
    case MHD_GNUTLS_CIPHER_AES_128_CBC:
      err = MHD_gc_cipher_open (GC_AES128, GC_CBC, &ret);
      break;
    case MHD_GNUTLS_CIPHER_AES_256_CBC:
      err = MHD_gc_cipher_open (GC_AES256, GC_CBC, &ret);
      break;
    case MHD_GNUTLS_CIPHER_3DES_CBC:
      err = MHD_gc_cipher_open (GC_3DES, GC_CBC, &ret);
      break;
    case MHD_GNUTLS_CIPHER_ARCFOUR_128:
      err = MHD_gc_cipher_open (GC_ARCFOUR128, GC_STREAM, &ret);
      break;
    default:
      return NULL;
    }

  if (err == 0)
    {
      MHD_gc_cipher_setkey (ret, key->size, (const char *) key->data);
      if (iv->data != NULL && iv->size > 0)
        MHD_gc_cipher_setiv (ret, iv->size, (const char *) iv->data);
    }
  else if (cipher != MHD_GNUTLS_CIPHER_NULL)
    {
      MHD_gnutls_assert ();
      MHD__gnutls_x509_log ("Crypto cipher[%d] error: %d\n", cipher, err);
      /* FIXME: MHD_gc_strerror */
    }

  return ret;
}

int
MHD_gtls_cipher_encrypt (cipher_hd_t handle, void *text, int textlen)
{
  if (handle != GNUTLS_CIPHER_FAILED)
    {
      if (MHD_gc_cipher_encrypt_inline (handle, textlen, text) != 0)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }
    }
  return 0;
}

int
MHD_gtls_cipher_decrypt (cipher_hd_t handle, void *ciphertext,
                         int ciphertextlen)
{
  if (handle != GNUTLS_CIPHER_FAILED)
    {
      if (MHD_gc_cipher_decrypt_inline (handle, ciphertextlen, ciphertext) !=
          0)
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_INTERNAL_ERROR;
        }
    }
  return 0;
}

void
MHD_gnutls_cipher_deinit (cipher_hd_t handle)
{
  if (handle != GNUTLS_CIPHER_FAILED)
    {
      MHD_gc_cipher_close (handle);
    }
}
