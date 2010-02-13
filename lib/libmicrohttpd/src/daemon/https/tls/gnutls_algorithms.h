/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007 Free Software Foundation
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

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "gnutls_auth.h"

/* Functions for version handling. */
enum MHD_GNUTLS_Protocol MHD_gtls_version_max (MHD_gtls_session_t session);
int MHD_gtls_version_priority (MHD_gtls_session_t session,
                               enum MHD_GNUTLS_Protocol version);
int MHD_gtls_version_is_supported (MHD_gtls_session_t session,
                                   const enum MHD_GNUTLS_Protocol version);
int MHD_gtls_version_get_major (enum MHD_GNUTLS_Protocol ver);
int MHD_gtls_version_get_minor (enum MHD_GNUTLS_Protocol ver);
enum MHD_GNUTLS_Protocol MHD_gtls_version_get (int major, int minor);

/* Functions for MACs. */
int MHD_gnutls_mac_is_ok (enum MHD_GNUTLS_HashAlgorithm algorithm);
/* Functions for cipher suites. */
int MHD_gtls_supported_ciphersuites (MHD_gtls_session_t session,
                                     cipher_suite_st ** ciphers);
int MHD_gtls_supported_ciphersuites_sorted (MHD_gtls_session_t session,
                                            cipher_suite_st ** ciphers);
int MHD_gtls_supported_compression_methods (MHD_gtls_session_t session,
                                            uint8_t ** comp);
const char *MHD_gtls_cipher_suite_get_name (cipher_suite_st * algorithm);
enum MHD_GNUTLS_CipherAlgorithm MHD_gtls_cipher_suite_get_cipher_algo (const
                                                                       cipher_suite_st
                                                                       *
                                                                       algorithm);
enum MHD_GNUTLS_KeyExchangeAlgorithm MHD_gtls_cipher_suite_get_kx_algo (const
                                                                        cipher_suite_st
                                                                        *
                                                                        algorithm);
enum MHD_GNUTLS_HashAlgorithm MHD_gtls_cipher_suite_get_mac_algo (const
                                                                  cipher_suite_st
                                                                  *
                                                                  algorithm);
enum MHD_GNUTLS_Protocol MHD_gtls_cipher_suite_get_version (const
                                                            cipher_suite_st *
                                                            algorithm);
cipher_suite_st MHD_gtls_cipher_suite_get_suite_name (cipher_suite_st *
                                                      algorithm);

/* Functions for ciphers. */
int MHD_gtls_cipher_get_block_size (enum MHD_GNUTLS_CipherAlgorithm
                                    algorithm);
int MHD_gtls_cipher_is_block (enum MHD_GNUTLS_CipherAlgorithm algorithm);
int MHD_gtls_cipher_is_ok (enum MHD_GNUTLS_CipherAlgorithm algorithm);
int MHD_gtls_cipher_get_iv_size (enum MHD_GNUTLS_CipherAlgorithm algorithm);
int MHD_gtls_cipher_get_export_flag (enum MHD_GNUTLS_CipherAlgorithm
                                     algorithm);

/* Functions for key exchange. */
int MHD_gtls_kx_needs_dh_params (enum MHD_GNUTLS_KeyExchangeAlgorithm
                                 algorithm);
int MHD_gtls_kx_needs_rsa_params (enum MHD_GNUTLS_KeyExchangeAlgorithm
                                  algorithm);
MHD_gtls_mod_auth_st *MHD_gtls_kx_auth_struct (enum
                                               MHD_GNUTLS_KeyExchangeAlgorithm
                                               algorithm);
int MHD_gtls_kx_is_ok (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm);

/* Functions for compression. */
int MHD_gtls_compression_is_ok (enum MHD_GNUTLS_CompressionMethod algorithm);
int MHD_gtls_compression_get_num (enum MHD_GNUTLS_CompressionMethod
                                  algorithm);
enum MHD_GNUTLS_CompressionMethod MHD_gtls_compression_get_id_from_int (int
                                                                        num);
int MHD_gtls_compression_get_mem_level (enum MHD_GNUTLS_CompressionMethod
                                        algorithm);
int MHD_gtls_compression_get_comp_level (enum MHD_GNUTLS_CompressionMethod
                                         algorithm);
int MHD_gtls_compression_get_wbits (enum MHD_GNUTLS_CompressionMethod
                                    algorithm);

/* Type to KX mappings. */
enum MHD_GNUTLS_CredentialsType MHD_gtls_map_kx_get_cred (enum
                                                          MHD_GNUTLS_KeyExchangeAlgorithm
                                                          algorithm,
                                                          int server);

/* KX to PK mapping. */
enum MHD_GNUTLS_PublicKeyAlgorithm MHD_gtls_map_pk_get_pk (enum
                                                           MHD_GNUTLS_KeyExchangeAlgorithm
                                                           kx_algorithm);
enum MHD_GNUTLS_PublicKeyAlgorithm MHD_gtls_x509_oid2pk_algorithm (const char
                                                                   *oid);
enum encipher_type
{ CIPHER_ENCRYPT = 0, CIPHER_SIGN = 1, CIPHER_IGN };

enum encipher_type MHD_gtls_kx_encipher_type (enum
                                              MHD_GNUTLS_KeyExchangeAlgorithm
                                              algorithm);

struct MHD_gtls_compression_entry
{
  const char *name;
  enum MHD_GNUTLS_CompressionMethod id;
  int num;                      /* the number reserved in TLS for the specific compression method */

  /* used in zlib compressor */
  int window_bits;
  int mem_level;
  int comp_level;
};
typedef struct MHD_gtls_compression_entry MHD_gnutls_compression_entry;

/* Functions for sign algorithms. */

int MHD_gtls_mac_priority (MHD_gtls_session_t session,
                           enum MHD_GNUTLS_HashAlgorithm algorithm);
int MHD_gtls_cipher_priority (MHD_gtls_session_t session,
                              enum MHD_GNUTLS_CipherAlgorithm algorithm);
int MHD_gtls_kx_priority (MHD_gtls_session_t session,
                          enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm);

enum MHD_GNUTLS_HashAlgorithm MHD_gtls_mac_get_id (const char *name);
enum MHD_GNUTLS_CipherAlgorithm MHD_gtls_cipher_get_id (const char *name);
enum MHD_GNUTLS_KeyExchangeAlgorithm MHD_gtls_kx_get_id (const char *name);
enum MHD_GNUTLS_Protocol MHD_gtls_protocol_get_id (const char *name);
enum MHD_GNUTLS_CertificateType MHD_gtls_certificate_type_get_id (const char
                                                                  *name);

#endif
