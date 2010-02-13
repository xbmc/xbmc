/* gc-libgcrypt.c --- Crypto wrappers around Libgcrypt for GC.
 * Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007  Simon Josefsson
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/* Note: This file is only built if GC uses Libgcrypt. */

#include "MHD_config.h"

/* Get prototype. */
#include "gc.h"

#include <stdlib.h>
#include <string.h>

/* Get libgcrypt API. */
#include <gcrypt.h>

#include <assert.h>

/* Initialization. */

Gc_rc
MHD_gc_init (void)
{
  gcry_error_t err;

  err = gcry_control (GCRYCTL_ANY_INITIALIZATION_P);
  if (err == GPG_ERR_NO_ERROR)
    {
      if (gcry_check_version (GCRYPT_VERSION) == NULL)
        return GC_INIT_ERROR;

      err = gcry_control (GCRYCTL_INITIALIZATION_FINISHED, NULL, 0);
      if (err != GPG_ERR_NO_ERROR)
        return GC_INIT_ERROR;
    }
  return GC_OK;
}

void
MHD_gc_done (void)
{
  return;
}

#ifdef GNULIB_GC_RANDOM

/* Randomness. */

Gc_rc
MHD_gc_nonce (char *data, size_t datalen)
{
  gcry_create_nonce ((unsigned char *) data, datalen);
  return GC_OK;
}

Gc_rc
MHD_gc_pseudo_random (char *data, size_t datalen)
{
  gcry_randomize ((unsigned char *) data, datalen, GCRY_STRONG_RANDOM);
  return GC_OK;
}

#endif

/* Memory allocation. */

/* Ciphers. */

Gc_rc
MHD_gc_cipher_open (Gc_cipher alg,
                    Gc_cipher_mode mode, MHD_gc_cipher_handle * outhandle)
{
  int gcryalg, gcrymode;
  gcry_error_t err;

  switch (alg)
    {
    case GC_AES128:
      gcryalg = GCRY_CIPHER_RIJNDAEL;
      break;

    case GC_AES192:
      gcryalg = GCRY_CIPHER_RIJNDAEL;
      break;

    case GC_AES256:
      gcryalg = GCRY_CIPHER_RIJNDAEL256;
      break;

    case GC_3DES:
      gcryalg = GCRY_CIPHER_3DES;
      break;

    case GC_DES:
      gcryalg = GCRY_CIPHER_DES;
      break;

    case GC_ARCFOUR128:
    case GC_ARCFOUR40:
      gcryalg = GCRY_CIPHER_ARCFOUR;
      break;

    case GC_ARCTWO40:
      gcryalg = GCRY_CIPHER_RFC2268_40;
      break;

    default:
      return GC_INVALID_CIPHER;
    }

  switch (mode)
    {
    case GC_ECB:
      gcrymode = GCRY_CIPHER_MODE_ECB;
      break;

    case GC_CBC:
      gcrymode = GCRY_CIPHER_MODE_CBC;
      break;

    case GC_STREAM:
      gcrymode = GCRY_CIPHER_MODE_STREAM;
      break;

    default:
      return GC_INVALID_CIPHER;
    }

  err =
    gcry_cipher_open ((gcry_cipher_hd_t *) outhandle, gcryalg, gcrymode, 0);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
MHD_gc_cipher_setkey (MHD_gc_cipher_handle handle, size_t keylen,
                      const char *key)
{
  gcry_error_t err;

  err = gcry_cipher_setkey ((gcry_cipher_hd_t) handle, key, keylen);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
MHD_gc_cipher_setiv (MHD_gc_cipher_handle handle, size_t ivlen,
                     const char *iv)
{
  gcry_error_t err;

  err = gcry_cipher_setiv ((gcry_cipher_hd_t) handle, iv, ivlen);
  if (gcry_err_code (err))
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
MHD_gc_cipher_encrypt_inline (MHD_gc_cipher_handle handle, size_t len,
                              char *data)
{
  if (gcry_cipher_encrypt ((gcry_cipher_hd_t) handle, data, len, NULL, len) !=
      0)
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
MHD_gc_cipher_decrypt_inline (MHD_gc_cipher_handle handle, size_t len,
                              char *data)
{
  if (gcry_cipher_decrypt ((gcry_cipher_hd_t) handle, data, len, NULL, len) !=
      0)
    return GC_INVALID_CIPHER;

  return GC_OK;
}

Gc_rc
MHD_gc_cipher_close (MHD_gc_cipher_handle handle)
{
  gcry_cipher_close (handle);

  return GC_OK;
}

/* Hashes. */

typedef struct _MHD_gc_hash_ctx
{
  Gc_hash alg;
  Gc_hash_mode mode;
  gcry_md_hd_t gch;
} _MHD_gc_hash_ctx;

Gc_rc
MHD_gc_hash_open (Gc_hash hash, Gc_hash_mode mode,
                  MHD_gc_hash_handle * outhandle)
{
  _MHD_gc_hash_ctx *ctx;
  int gcryalg = 0, gcrymode = 0;
  gcry_error_t err;
  Gc_rc rc = GC_OK;

  ctx = calloc (sizeof (*ctx), 1);
  if (!ctx)
    return GC_MALLOC_ERROR;

  ctx->alg = hash;
  ctx->mode = mode;

  switch (hash)
    {
    case GC_MD2:
      gcryalg = GCRY_MD_NONE;
      break;

    case GC_MD4:
      gcryalg = GCRY_MD_MD4;
      break;

    case GC_MD5:
      gcryalg = GCRY_MD_MD5;
      break;

    case GC_SHA1:
      gcryalg = GCRY_MD_SHA1;
      break;

    case GC_SHA256:
      gcryalg = GCRY_MD_SHA256;
      break;

    case GC_SHA384:
      gcryalg = GCRY_MD_SHA384;
      break;

    case GC_SHA512:
      gcryalg = GCRY_MD_SHA512;
      break;

    case GC_RMD160:
      gcryalg = GCRY_MD_RMD160;
      break;

    default:
      rc = GC_INVALID_HASH;
    }

  switch (mode)
    {
    case 0:
      gcrymode = 0;
      break;

    case GC_HMAC:
      gcrymode = GCRY_MD_FLAG_HMAC;
      break;

    default:
      rc = GC_INVALID_HASH;
    }

  if (rc == GC_OK && gcryalg != GCRY_MD_NONE)
    {
      err = gcry_md_open (&ctx->gch, gcryalg, gcrymode);
      if (gcry_err_code (err))
        rc = GC_INVALID_HASH;
    }

  if (rc == GC_OK)
    *outhandle = ctx;
  else
    free (ctx);

  return rc;
}

Gc_rc
MHD_gc_hash_clone (MHD_gc_hash_handle handle, MHD_gc_hash_handle * outhandle)
{
  _MHD_gc_hash_ctx *in = handle;
  _MHD_gc_hash_ctx *out;
  int err;

  *outhandle = out = calloc (sizeof (*out), 1);
  if (!out)
    return GC_MALLOC_ERROR;

  memcpy (out, in, sizeof (*out));

  err = gcry_md_copy (&out->gch, in->gch);
  if (err)
    {
      free (out);
      return GC_INVALID_HASH;
    }

  return GC_OK;
}

size_t
MHD_gc_hash_digest_length (Gc_hash hash)
{
  size_t len;

  switch (hash)
    {
    case GC_MD2:
      len = GC_MD2_DIGEST_SIZE;
      break;

    case GC_MD4:
      len = GC_MD4_DIGEST_SIZE;
      break;

    case GC_MD5:
      len = GC_MD5_DIGEST_SIZE;
      break;

    case GC_RMD160:
      len = GC_RMD160_DIGEST_SIZE;
      break;

    case GC_SHA1:
      len = GC_SHA1_DIGEST_SIZE;
      break;

    case GC_SHA256:
      len = GC_SHA256_DIGEST_SIZE;
      break;

    case GC_SHA384:
      len = GC_SHA384_DIGEST_SIZE;
      break;

    case GC_SHA512:
      len = GC_SHA512_DIGEST_SIZE;
      break;

    default:
      return 0;
    }

  return len;
}

void
MHD_gc_hash_MHD_hmac_setkey (MHD_gc_hash_handle handle, size_t len,
                             const char *key)
{
  _MHD_gc_hash_ctx *ctx = handle;
  gcry_md_setkey (ctx->gch, key, len);
}

void
MHD_gc_hash_write (MHD_gc_hash_handle handle, size_t len, const char *data)
{
  _MHD_gc_hash_ctx *ctx = handle;
  gcry_md_write (ctx->gch, data, len);
}

const char *
MHD_gc_hash_read (MHD_gc_hash_handle handle)
{
  _MHD_gc_hash_ctx *ctx = handle;
  const char *digest;
  {
    gcry_md_final (ctx->gch);
    digest = (const char *) gcry_md_read (ctx->gch, 0);
  }

  return digest;
}

void
MHD_gc_hash_close (MHD_gc_hash_handle handle)
{
  _MHD_gc_hash_ctx *ctx = handle;

  gcry_md_close (ctx->gch);

  free (ctx);
}
