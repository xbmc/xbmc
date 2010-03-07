/* gc.h --- Header file for implementation agnostic crypto wrapper API.
 * Copyright (C) 2002, 2003, 2004, 2005, 2007  Simon Josefsson
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

#ifndef GC_H
#define GC_H

/* Get size_t. */
# include <stddef.h>

enum Gc_rc
{
  GC_OK = 0,
  GC_MALLOC_ERROR,
  GC_INIT_ERROR,
  GC_RANDOM_ERROR,
  GC_INVALID_CIPHER,
  GC_INVALID_HASH,
  GC_PKCS5_INVALID_ITERATION_COUNT,
  GC_PKCS5_INVALID_DERIVED_KEY_LENGTH,
  GC_PKCS5_DERIVED_KEY_TOO_LONG
};
typedef enum Gc_rc Gc_rc;

/* Hash types. */
enum Gc_hash
{
  GC_MD4,
  GC_MD5,
  GC_SHA1,
  GC_MD2,
  GC_RMD160,
  GC_SHA256,
  GC_SHA384,
  GC_SHA512
};
typedef enum Gc_hash Gc_hash;

enum Gc_hash_mode
{
  GC_HMAC = 1
};
typedef enum Gc_hash_mode Gc_hash_mode;

typedef void *MHD_gc_hash_handle;

#define GC_MD2_DIGEST_SIZE 16
#define GC_MD4_DIGEST_SIZE 16
#define GC_MD5_DIGEST_SIZE 16
#define GC_RMD160_DIGEST_SIZE 20
#define GC_SHA1_DIGEST_SIZE 20
#define GC_SHA256_DIGEST_SIZE 32
#define GC_SHA384_DIGEST_SIZE 48
#define GC_SHA512_DIGEST_SIZE 64

/* Cipher types. */
enum Gc_cipher
{
  GC_AES128,
  GC_AES192,
  GC_AES256,
  GC_3DES,
  GC_DES,
  GC_ARCFOUR128,
  GC_ARCFOUR40,
  GC_ARCTWO40,
  GC_CAMELLIA128,
  GC_CAMELLIA256
};
typedef enum Gc_cipher Gc_cipher;

enum Gc_cipher_mode
{
  GC_ECB,
  GC_CBC,
  GC_STREAM
};
typedef enum Gc_cipher_mode Gc_cipher_mode;

typedef void *MHD_gc_cipher_handle;

/* Call before respectively after any other functions. */
Gc_rc MHD_gc_init (void);
void MHD_gc_done (void);

/* Memory allocation (avoid). */
typedef void *(*MHD_gc_malloc_t) (size_t n);
typedef int (*MHD_gc_secure_check_t) (const void *);
typedef void *(*MHD_gc_realloc_t) (void *p, size_t n);
typedef void (*MHD_gc_free_t) (void *);
/* Randomness. */
Gc_rc MHD_gc_nonce (char *data, size_t datalen);
Gc_rc MHD_gc_pseudo_random (char *data, size_t datalen);

/* Ciphers. */
Gc_rc MHD_gc_cipher_open (Gc_cipher cipher,
                          Gc_cipher_mode mode,
                          MHD_gc_cipher_handle * outhandle);
Gc_rc MHD_gc_cipher_setkey (MHD_gc_cipher_handle handle, size_t keylen,
                            const char *key);
Gc_rc MHD_gc_cipher_setiv (MHD_gc_cipher_handle handle, size_t ivlen,
                           const char *iv);
Gc_rc MHD_gc_cipher_encrypt_inline (MHD_gc_cipher_handle handle, size_t len,
                                    char *data);
Gc_rc MHD_gc_cipher_decrypt_inline (MHD_gc_cipher_handle handle, size_t len,
                                    char *data);
Gc_rc MHD_gc_cipher_close (MHD_gc_cipher_handle handle);

/* Hashes. */

Gc_rc MHD_gc_hash_open (Gc_hash hash,
                        Gc_hash_mode mode, MHD_gc_hash_handle * outhandle);
Gc_rc MHD_gc_hash_clone (MHD_gc_hash_handle handle,
                         MHD_gc_hash_handle * outhandle);
size_t MHD_gc_hash_digest_length (Gc_hash hash);
void MHD_gc_hash_MHD_hmac_setkey (MHD_gc_hash_handle handle, size_t len,
                                  const char *key);
void MHD_gc_hash_write (MHD_gc_hash_handle handle, size_t len,
                        const char *data);
const char *MHD_gc_hash_read (MHD_gc_hash_handle handle);
void MHD_gc_hash_close (MHD_gc_hash_handle handle);

/* Compute a hash value over buffer IN of INLEN bytes size using the
 algorithm HASH, placing the result in the pre-allocated buffer OUT.
 The required size of OUT depends on HASH, and is generally
 GC_<HASH>_DIGEST_SIZE.  For example, for GC_MD5 the output buffer
 must be 16 bytes.  The return value is 0 (GC_OK) on success, or
 another Gc_rc error code. */
Gc_rc MHD_gc_hash_buffer (Gc_hash hash, const void *in, size_t inlen,
                          char *out);

/* One-call interface. */
Gc_rc MHD_gc_md2 (const void *in, size_t inlen, void *resbuf);
Gc_rc MHD_gc_md4 (const void *in, size_t inlen, void *resbuf);
Gc_rc MHD_gc_md5 (const void *in, size_t inlen, void *resbuf);
Gc_rc MHD_gc_sha1 (const void *in, size_t inlen, void *resbuf);
Gc_rc MHD_gc_MHD_hmac_md5 (const void *key,
                           size_t keylen, const void *in, size_t inlen,
                           char *resbuf);
Gc_rc MHD_gc_MHD_hmac_sha1 (const void *key, size_t keylen, const void *in,
                            size_t inlen, char *resbuf);

/* Derive cryptographic keys from a password P of length PLEN, with
 salt S of length SLEN, placing the result in pre-allocated buffer
 DK of length DKLEN.  An iteration count is specified in C, where a
 larger value means this function take more time (typical iteration
 counts are 1000-20000).  This function "stretches" the key to be
 exactly dkLen bytes long.  GC_OK is returned on success, otherwise
 an Gc_rc error code is returned.  */
Gc_rc MHD_gc_pbkdf2_sha1 (const char *P,
                          size_t Plen,
                          const char *S,
                          size_t Slen, unsigned int c, char *DK,
                          size_t dkLen);

#endif /* GC_H */
