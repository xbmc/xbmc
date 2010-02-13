/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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

#ifndef GNUTLS_HASH_INT_H
# define GNUTLS_HASH_INT_H

#include <gnutls_int.h>

/* for message digests */

typedef struct
{
  MHD_gc_hash_handle handle;
  enum MHD_GNUTLS_HashAlgorithm algorithm;
  const void *key;
  int keysize;
} mac_hd_st;
typedef mac_hd_st *mac_hd_t;
typedef mac_hd_t GNUTLS_HASH_HANDLE;

#define GNUTLS_HASH_FAILED NULL
#define GNUTLS_MAC_FAILED NULL

mac_hd_t MHD_gtls_MHD_hmac_init (enum MHD_GNUTLS_HashAlgorithm algorithm,
                                 const void *key, int keylen);

void MHD_gnutls_MHD_hmac_deinit (mac_hd_t handle, void *digest);

mac_hd_t MHD_gnutls_mac_init_ssl3 (enum MHD_GNUTLS_HashAlgorithm algorithm,
                                   void *key, int keylen);
void MHD_gnutls_mac_deinit_ssl3 (mac_hd_t handle, void *digest);

GNUTLS_HASH_HANDLE MHD_gtls_hash_init (enum MHD_GNUTLS_HashAlgorithm
                                       algorithm);
int MHD_gnutls_hash_get_algo_len (enum MHD_GNUTLS_HashAlgorithm algorithm);
int MHD_gnutls_hash (GNUTLS_HASH_HANDLE handle, const void *text,
                     size_t textlen);
void MHD_gnutls_hash_deinit (GNUTLS_HASH_HANDLE handle, void *digest);

int MHD_gnutls_ssl3_generate_random (void *secret, int secret_len,
                                     void *rnd, int random_len, int bytes,
                                     opaque * ret);
int MHD_gnutls_ssl3_hash_md5 (void *first, int first_len, void *second,
                              int second_len, int ret_len, opaque * ret);

void MHD_gnutls_mac_deinit_ssl3_handshake (mac_hd_t handle, void *digest,
                                           opaque * key, uint32_t key_size);

GNUTLS_HASH_HANDLE MHD_gnutls_hash_copy (GNUTLS_HASH_HANDLE handle);

#endif /* GNUTLS_HASH_INT_H */
