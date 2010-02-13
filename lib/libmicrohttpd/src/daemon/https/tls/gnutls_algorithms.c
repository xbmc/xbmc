/*
 * Copyright (C) 2000, 2002, 2003, 2004, 2005, 2006, 2007 Free Software Foundation
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

#include "gnutls_int.h"
#include "gnutls_algorithms.h"
#include "gnutls_errors.h"
#include "gnutls_cert.h"
/* x509 */
#include "common.h"

/* Cred type mappings to KX algorithms
 * FIXME: The mappings are not 1-1. Some KX such as SRP_RSA require
 * more than one credentials type.
 */
typedef struct
{
  enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm;
  enum MHD_GNUTLS_CredentialsType client_type;
  enum MHD_GNUTLS_CredentialsType server_type;  /* The type of credentials a server
                                                 * needs to set */
} MHD_gnutls_cred_map;

static const MHD_gnutls_cred_map MHD_gtls_cred_mappings[] = {
  {MHD_GNUTLS_KX_RSA,
   MHD_GNUTLS_CRD_CERTIFICATE,
   MHD_GNUTLS_CRD_CERTIFICATE},
  {MHD_GNUTLS_KX_RSA_EXPORT,
   MHD_GNUTLS_CRD_CERTIFICATE,
   MHD_GNUTLS_CRD_CERTIFICATE},
  {0,
   0,
   0}
};

#define GNUTLS_KX_MAP_LOOP(b) \
        const MHD_gnutls_cred_map *p; \
                for(p = MHD_gtls_cred_mappings; p->algorithm != 0; p++) { b ; }

#define GNUTLS_KX_MAP_ALG_LOOP_SERVER(a) \
                        GNUTLS_KX_MAP_LOOP( if(p->server_type == type) { a; break; })

#define GNUTLS_KX_MAP_ALG_LOOP_CLIENT(a) \
                        GNUTLS_KX_MAP_LOOP( if(p->client_type == type) { a; break; })

/* KX mappings to PK algorithms */
typedef struct
{
  enum MHD_GNUTLS_KeyExchangeAlgorithm kx_algorithm;
  enum MHD_GNUTLS_PublicKeyAlgorithm pk_algorithm;
  enum encipher_type encipher_type;     /* CIPHER_ENCRYPT if this algorithm is to be used
                                         * for encryption, CIPHER_SIGN if signature only,
                                         * CIPHER_IGN if this does not apply at all.
                                         *
                                         * This is useful to certificate cipher suites, which check
                                         * against the certificate key usage bits.
                                         */
} MHD_gnutls_pk_map;

/* This table maps the Key exchange algorithms to
 * the certificate algorithms. Eg. if we have
 * RSA algorithm in the certificate then we can
 * use GNUTLS_KX_RSA or GNUTLS_KX_DHE_RSA.
 */
static const MHD_gnutls_pk_map MHD_gtls_pk_mappings[] = {
  {MHD_GNUTLS_KX_RSA,
   MHD_GNUTLS_PK_RSA,
   CIPHER_ENCRYPT},
  {MHD_GNUTLS_KX_RSA_EXPORT,
   MHD_GNUTLS_PK_RSA,
   CIPHER_SIGN},
  {0,
   0,
   0}
};

#define GNUTLS_PK_MAP_LOOP(b) \
        const MHD_gnutls_pk_map *p; \
                for(p = MHD_gtls_pk_mappings; p->kx_algorithm != 0; p++) { b }

#define GNUTLS_PK_MAP_ALG_LOOP(a) \
                        GNUTLS_PK_MAP_LOOP( if(p->kx_algorithm == kx_algorithm) { a; break; })

/* TLS Versions */

typedef struct
{
  const char *name;
  enum MHD_GNUTLS_Protocol id;  /* gnutls internal version number */
  int major;                    /* defined by the protocol */
  int minor;                    /* defined by the protocol */
  int supported;                /* 0 not supported, > 0 is supported */
} MHD_gnutls_version_entry;

static const MHD_gnutls_version_entry MHD_gtls_sup_versions[] = {
  {"SSL3.0",
   MHD_GNUTLS_PROTOCOL_SSL3,
   3,
   0,
   1},
  {"TLS1.0",
   MHD_GNUTLS_PROTOCOL_TLS1_0,
   3,
   1,
   1},
  {"TLS1.1",
   MHD_GNUTLS_PROTOCOL_TLS1_1,
   3,
   2,
   1},
  {"TLS1.2",
   MHD_GNUTLS_PROTOCOL_TLS1_2,
   3,
   3,
   1},
  {0,
   0,
   0,
   0,
   0}
};

/* Keep the contents of this struct the same as the previous one. */
static const enum MHD_GNUTLS_Protocol MHD_gtls_supported_protocols[] =
{ MHD_GNUTLS_PROTOCOL_SSL3,
  MHD_GNUTLS_PROTOCOL_TLS1_0,
  MHD_GNUTLS_PROTOCOL_TLS1_1,
  MHD_GNUTLS_PROTOCOL_TLS1_2,
  0
};

#define GNUTLS_VERSION_LOOP(b) \
        const MHD_gnutls_version_entry *p; \
                for(p = MHD_gtls_sup_versions; p->name != NULL; p++) { b ; }

#define GNUTLS_VERSION_ALG_LOOP(a) \
                        GNUTLS_VERSION_LOOP( if(p->id == version) { a; break; })

struct MHD_gnutls_cipher_entry
{
  const char *name;
  enum MHD_GNUTLS_CipherAlgorithm id;
  uint16_t blocksize;
  uint16_t keysize;
  cipher_type_t block;
  uint16_t iv;
  int export_flag;              /* 0 non export */
};
typedef struct MHD_gnutls_cipher_entry MHD_gnutls_cipher_entry;

/* Note that all algorithms are in CBC or STREAM modes.
 * Do not add any algorithms in other modes (avoid modified algorithms).
 * View first: "The order of encryption and authentication for
 * protecting communications" by Hugo Krawczyk - CRYPTO 2001
 */
static const MHD_gnutls_cipher_entry MHD_gtls_algorithms[] = {
  {"AES-256-CBC",
   MHD_GNUTLS_CIPHER_AES_256_CBC,
   16,
   32,
   CIPHER_BLOCK,
   16,
   0},
  {"AES-128-CBC",
   MHD_GNUTLS_CIPHER_AES_128_CBC,
   16,
   16,
   CIPHER_BLOCK,
   16,
   0},
  {"3DES-CBC",
   MHD_GNUTLS_CIPHER_3DES_CBC,
   8,
   24,
   CIPHER_BLOCK,
   8,
   0},
  {"ARCFOUR-128",
   MHD_GNUTLS_CIPHER_ARCFOUR_128,
   1,
   16,
   CIPHER_STREAM,
   0,
   0},
  {"NULL",
   MHD_GNUTLS_CIPHER_NULL,
   1,
   0,
   CIPHER_STREAM,
   0,
   0},
  {0,
   0,
   0,
   0,
   0,
   0,
   0}
};

/* Keep the contents of this struct the same as the previous one. */
static const enum MHD_GNUTLS_CipherAlgorithm MHD_gtls_supported_ciphers[] =
{ MHD_GNUTLS_CIPHER_AES_256_CBC,
  MHD_GNUTLS_CIPHER_AES_128_CBC,
  MHD_GNUTLS_CIPHER_3DES_CBC,
  MHD_GNUTLS_CIPHER_ARCFOUR_128,
  MHD_GNUTLS_CIPHER_NULL,
  0
};

#define GNUTLS_LOOP(b) \
        const MHD_gnutls_cipher_entry *p; \
                for(p = MHD_gtls_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_ALG_LOOP(a) \
                        GNUTLS_LOOP( if(p->id == algorithm) { a; break; } )

struct MHD_gnutls_hash_entry
{
  const char *name;
  const char *oid;
  enum MHD_GNUTLS_HashAlgorithm id;
  size_t key_size;              /* in case of mac */
};
typedef struct MHD_gnutls_hash_entry MHD_gnutls_hash_entry;

static const MHD_gnutls_hash_entry MHD_gtls_hash_algorithms[] = {
  {"SHA1",
   HASH_OID_SHA1,
   MHD_GNUTLS_MAC_SHA1,
   20},
  {"MD5",
   HASH_OID_MD5,
   MHD_GNUTLS_MAC_MD5,
   16},
  {"SHA256",
   HASH_OID_SHA256,
   MHD_GNUTLS_MAC_SHA256,
   32},
  {"NULL",
   NULL,
   MHD_GNUTLS_MAC_NULL,
   0},
  {0,
   0,
   0,
   0}
};

/* Keep the contents of this struct the same as the previous one. */
static const enum MHD_GNUTLS_HashAlgorithm MHD_gtls_supported_macs[] =
{ MHD_GNUTLS_MAC_SHA1,
  MHD_GNUTLS_MAC_MD5,
  MHD_GNUTLS_MAC_SHA256,
  MHD_GNUTLS_MAC_NULL,
  0
};

#define GNUTLS_HASH_LOOP(b) \
        const MHD_gnutls_hash_entry *p; \
                for(p = MHD_gtls_hash_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_HASH_ALG_LOOP(a) \
                        GNUTLS_HASH_LOOP( if(p->id == algorithm) { a; break; } )

/* Compression Section */
#define GNUTLS_COMPRESSION_ENTRY(name, id, wb, ml, cl) \
	{ #name, name, id, wb, ml, cl}

#define MAX_COMP_METHODS 5
const int MHD__gnutls_comp_algorithms_size = MAX_COMP_METHODS;

/* the compression entry is defined in MHD_gnutls_algorithms.h */

MHD_gnutls_compression_entry
  MHD__gnutls_compression_algorithms[MAX_COMP_METHODS] =
{
  GNUTLS_COMPRESSION_ENTRY (MHD_GNUTLS_COMP_NULL, 0x00, 0, 0, 0),
  {
  0, 0, 0, 0, 0, 0}
};

static const enum MHD_GNUTLS_CompressionMethod
  MHD_gtls_supported_compressions[] =
{
  MHD_GNUTLS_COMP_NULL,
  0
};

#define GNUTLS_COMPRESSION_LOOP(b) \
        const MHD_gnutls_compression_entry *p; \
                for(p = MHD__gnutls_compression_algorithms; p->name != NULL; p++) { b ; }
#define GNUTLS_COMPRESSION_ALG_LOOP(a) \
                        GNUTLS_COMPRESSION_LOOP( if(p->id == algorithm) { a; break; } )
#define GNUTLS_COMPRESSION_ALG_LOOP_NUM(a) \
                        GNUTLS_COMPRESSION_LOOP( if(p->num == num) { a; break; } )

/* Key Exchange Section */
extern MHD_gtls_mod_auth_st MHD_gtls_rsa_auth_struct;
extern MHD_gtls_mod_auth_st MHD_rsa_export_auth_struct;
extern MHD_gtls_mod_auth_st MHD_gtls_dhe_rsa_auth_struct;
extern MHD_gtls_mod_auth_st MHD_gtls_dhe_dss_auth_struct;
extern MHD_gtls_mod_auth_st srp_auth_struct;
extern MHD_gtls_mod_auth_st psk_auth_struct;
extern MHD_gtls_mod_auth_st dhe_psk_auth_struct;
extern MHD_gtls_mod_auth_st srp_rsa_auth_struct;
extern MHD_gtls_mod_auth_st srp_dss_auth_struct;

typedef struct MHD_gtls_kx_algo_entry
{
  const char *name;
  enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm;
  MHD_gtls_mod_auth_st *auth_struct;
  int needs_dh_params;
  int needs_rsa_params;
} MHD_gtls_kx_algo_entry_t;

static const MHD_gtls_kx_algo_entry_t MHD_gtls_kx_algorithms[] = {
  {"RSA",
   MHD_GNUTLS_KX_RSA,
   &MHD_gtls_rsa_auth_struct,
   0,
   0},
  {"RSA-EXPORT",
   MHD_GNUTLS_KX_RSA_EXPORT,
   &MHD_rsa_export_auth_struct,
   0,
   1 /* needs RSA params */ },
  {0,
   0,
   0,
   0,
   0}
};

/* Keep the contents of this struct the same as the previous one. */
static const enum MHD_GNUTLS_KeyExchangeAlgorithm MHD_gtls_supported_kxs[] =
{
  MHD_GNUTLS_KX_RSA,
  MHD_GNUTLS_KX_RSA_EXPORT,
  0
};

#define GNUTLS_KX_LOOP(b) \
        const MHD_gtls_kx_algo_entry_t *p; \
                for(p = MHD_gtls_kx_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_KX_ALG_LOOP(a) \
                        GNUTLS_KX_LOOP( if(p->algorithm == algorithm) { a; break; } )

/* Cipher SUITES */
#define GNUTLS_CIPHER_SUITE_ENTRY( name, block_algorithm, kx_algorithm, mac_algorithm, version ) \
	{ #name, {name}, block_algorithm, kx_algorithm, mac_algorithm, version }

typedef struct
{
  const char *name;
  cipher_suite_st id;
  enum MHD_GNUTLS_CipherAlgorithm block_algorithm;
  enum MHD_GNUTLS_KeyExchangeAlgorithm kx_algorithm;
  enum MHD_GNUTLS_HashAlgorithm mac_algorithm;
  enum MHD_GNUTLS_Protocol version;     /* this cipher suite is supported
                                         * from 'version' and above;
                                         */
} MHD_gtls_cipher_suite_entry;

/* RSA with NULL cipher and MD5 MAC
 * for test purposes.
 */
#define GNUTLS_RSA_NULL_MD5 { 0x00, 0x01 }

/* PSK (not in TLS 1.0)
 * draft-ietf-tls-psk:
 */
#define GNUTLS_PSK_SHA_ARCFOUR_SHA1 { 0x00, 0x8A }
#define GNUTLS_PSK_SHA_3DES_EDE_CBC_SHA1 { 0x00, 0x8B }
#define GNUTLS_PSK_SHA_AES_128_CBC_SHA1 { 0x00, 0x8C }
#define GNUTLS_PSK_SHA_AES_256_CBC_SHA1 { 0x00, 0x8D }

#define GNUTLS_DHE_PSK_SHA_ARCFOUR_SHA1 { 0x00, 0x8E }
#define GNUTLS_DHE_PSK_SHA_3DES_EDE_CBC_SHA1 { 0x00, 0x8F }
#define GNUTLS_DHE_PSK_SHA_AES_128_CBC_SHA1 { 0x00, 0x90 }
#define GNUTLS_DHE_PSK_SHA_AES_256_CBC_SHA1 { 0x00, 0x91 }

/* SRP (rfc5054)
 */
#define GNUTLS_SRP_SHA_3DES_EDE_CBC_SHA1 { 0xC0, 0x1A }
#define GNUTLS_SRP_SHA_RSA_3DES_EDE_CBC_SHA1 { 0xC0, 0x1B }
#define GNUTLS_SRP_SHA_DSS_3DES_EDE_CBC_SHA1 { 0xC0, 0x1C }

#define GNUTLS_SRP_SHA_AES_128_CBC_SHA1 { 0xC0, 0x1D }
#define GNUTLS_SRP_SHA_RSA_AES_128_CBC_SHA1 { 0xC0, 0x1E }
#define GNUTLS_SRP_SHA_DSS_AES_128_CBC_SHA1 { 0xC0, 0x1F }

#define GNUTLS_SRP_SHA_AES_256_CBC_SHA1 { 0xC0, 0x20 }
#define GNUTLS_SRP_SHA_RSA_AES_256_CBC_SHA1 { 0xC0, 0x21 }
#define GNUTLS_SRP_SHA_DSS_AES_256_CBC_SHA1 { 0xC0, 0x22 }

/* RSA
 */
#define GNUTLS_RSA_ARCFOUR_SHA1 { 0x00, 0x05 }
#define GNUTLS_RSA_ARCFOUR_MD5 { 0x00, 0x04 }
#define GNUTLS_RSA_3DES_EDE_CBC_SHA1 { 0x00, 0x0A }

/* rfc3268:
 */
#define GNUTLS_RSA_AES_128_CBC_SHA1 { 0x00, 0x2F }
#define GNUTLS_RSA_AES_256_CBC_SHA1 { 0x00, 0x35 }

/* rfc4132 */
#define GNUTLS_RSA_CAMELLIA_128_CBC_SHA1 { 0x00,0x41 }
#define GNUTLS_RSA_CAMELLIA_256_CBC_SHA1 { 0x00,0x84 }

/* DHE DSS
 */

#define GNUTLS_DHE_DSS_3DES_EDE_CBC_SHA1 { 0x00, 0x13 }

/* draft-ietf-tls-56-bit-ciphersuites-01:
 */
#define GNUTLS_DHE_DSS_ARCFOUR_SHA1 { 0x00, 0x66 }

/* rfc3268:
 */
#define GNUTLS_DHE_DSS_AES_256_CBC_SHA1 { 0x00, 0x38 }
#define GNUTLS_DHE_DSS_AES_128_CBC_SHA1 { 0x00, 0x32 }

/* rfc4132 */
#define GNUTLS_DHE_DSS_CAMELLIA_128_CBC_SHA1 { 0x00,0x44 }
#define GNUTLS_DHE_DSS_CAMELLIA_256_CBC_SHA1 { 0x00,0x87 }

/* DHE RSA
 */
#define GNUTLS_DHE_RSA_3DES_EDE_CBC_SHA1 { 0x00, 0x16 }

/* rfc3268:
 */
#define GNUTLS_DHE_RSA_AES_128_CBC_SHA1 { 0x00, 0x33 }
#define GNUTLS_DHE_RSA_AES_256_CBC_SHA1 { 0x00, 0x39 }

/* rfc4132 */
#define GNUTLS_DHE_RSA_CAMELLIA_128_CBC_SHA1 { 0x00,0x45 }
#define GNUTLS_DHE_RSA_CAMELLIA_256_CBC_SHA1 { 0x00,0x88 }

#define CIPHER_SUITES_COUNT sizeof(MHD_gtls_cs_algorithms)/sizeof(MHD_gtls_cipher_suite_entry)-1

static const MHD_gtls_cipher_suite_entry MHD_gtls_cs_algorithms[] = {
  /* RSA */
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_NULL_MD5,
                             MHD_GNUTLS_CIPHER_NULL,
                             MHD_GNUTLS_KX_RSA, MHD_GNUTLS_MAC_MD5,
                             MHD_GNUTLS_PROTOCOL_SSL3),
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_ARCFOUR_SHA1,
                             MHD_GNUTLS_CIPHER_ARCFOUR_128,
                             MHD_GNUTLS_KX_RSA, MHD_GNUTLS_MAC_SHA1,
                             MHD_GNUTLS_PROTOCOL_SSL3),
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_ARCFOUR_MD5,
                             MHD_GNUTLS_CIPHER_ARCFOUR_128,
                             MHD_GNUTLS_KX_RSA, MHD_GNUTLS_MAC_MD5,
                             MHD_GNUTLS_PROTOCOL_SSL3),
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_3DES_EDE_CBC_SHA1,
                             MHD_GNUTLS_CIPHER_3DES_CBC,
                             MHD_GNUTLS_KX_RSA, MHD_GNUTLS_MAC_SHA1,
                             MHD_GNUTLS_PROTOCOL_SSL3),
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_AES_128_CBC_SHA1,
                             MHD_GNUTLS_CIPHER_AES_128_CBC, MHD_GNUTLS_KX_RSA,
                             MHD_GNUTLS_MAC_SHA1, MHD_GNUTLS_PROTOCOL_SSL3),
  GNUTLS_CIPHER_SUITE_ENTRY (GNUTLS_RSA_AES_256_CBC_SHA1,
                             MHD_GNUTLS_CIPHER_AES_256_CBC, MHD_GNUTLS_KX_RSA,
                             MHD_GNUTLS_MAC_SHA1, MHD_GNUTLS_PROTOCOL_SSL3),
  {0,
   {
    {0,
     0}},
   0,
   0,
   0,
   0}
};

#define GNUTLS_CIPHER_SUITE_LOOP(b) \
        const MHD_gtls_cipher_suite_entry *p; \
                for(p = MHD_gtls_cs_algorithms; p->name != NULL; p++) { b ; }

#define GNUTLS_CIPHER_SUITE_ALG_LOOP(a) \
                        GNUTLS_CIPHER_SUITE_LOOP( if( (p->id.suite[0] == suite->suite[0]) && (p->id.suite[1] == suite->suite[1])) { a; break; } )

/* Generic Functions */

int
MHD_gtls_mac_priority (MHD_gtls_session_t session,
                       enum MHD_GNUTLS_HashAlgorithm algorithm)
{                               /* actually returns the priority */
  unsigned int i;
  for (i = 0; i < session->internals.priorities.mac.num_algorithms; i++)
    {
      if (session->internals.priorities.mac.priority[i] == algorithm)
        return i;
    }
  return -1;
}


int
MHD_gnutls_mac_is_ok (enum MHD_GNUTLS_HashAlgorithm algorithm)
{
  ssize_t ret = -1;
  GNUTLS_HASH_ALG_LOOP (ret = p->id);
  if (ret >= 0)
    ret = 0;
  else
    ret = 1;
  return ret;
}


/**
 * MHD__gnutls_compression_get_name - Returns a string with the name of the specified compression algorithm
 * @algorithm: is a Compression algorithm
 *
 * Returns: a pointer to a string that contains the name of the
 * specified compression algorithm, or %NULL.
 **/
const char *
MHD_gtls_compression_get_name (enum MHD_GNUTLS_CompressionMethod algorithm)
{
  const char *ret = NULL;

  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->name + sizeof ("GNUTLS_COMP_") - 1);

  return ret;
}

/**
 * MHD_gtls_compression_get_id - Returns the gnutls id of the specified in string algorithm
 * @algorithm: is a compression method name
 *
 * The names are compared in a case insensitive way.
 *
 * Returns: an id of the specified in a string compression method, or
 * %GNUTLS_COMP_UNKNOWN on error.
 *
 **/
enum MHD_GNUTLS_CompressionMethod
MHD_gtls_compression_get_id (const char *name)
{
  enum MHD_GNUTLS_CompressionMethod ret = MHD_GNUTLS_COMP_UNKNOWN;

  GNUTLS_COMPRESSION_LOOP (if
                           (strcasecmp
                            (p->name + sizeof ("GNUTLS_COMP_") - 1,
                             name) == 0) ret = p->id)
    ;

  return ret;
}


/* return the tls number of the specified algorithm */
int
MHD_gtls_compression_get_num (enum MHD_GNUTLS_CompressionMethod algorithm)
{
  int ret = -1;

  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->num);

  return ret;
}

int
MHD_gtls_compression_get_wbits (enum MHD_GNUTLS_CompressionMethod algorithm)
{
  int ret = -1;
  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->window_bits);
  return ret;
}

int
MHD_gtls_compression_get_mem_level (enum MHD_GNUTLS_CompressionMethod
                                    algorithm)
{
  int ret = -1;
  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->mem_level);
  return ret;
}

int
MHD_gtls_compression_get_comp_level (enum MHD_GNUTLS_CompressionMethod
                                     algorithm)
{
  int ret = -1;
  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->comp_level);
  return ret;
}

/* returns the gnutls internal ID of the TLS compression
 * method num
 */
enum MHD_GNUTLS_CompressionMethod
MHD_gtls_compression_get_id_from_int (int num)
{
  enum MHD_GNUTLS_CompressionMethod ret = -1;

  /* avoid prefix */
  GNUTLS_COMPRESSION_ALG_LOOP_NUM (ret = p->id);

  return ret;
}

int
MHD_gtls_compression_is_ok (enum MHD_GNUTLS_CompressionMethod algorithm)
{
  ssize_t ret = -1;
  GNUTLS_COMPRESSION_ALG_LOOP (ret = p->id);
  if (ret >= 0)
    ret = 0;
  else
    ret = 1;
  return ret;
}

/* CIPHER functions */
int
MHD_gtls_cipher_get_block_size (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{
  size_t ret = 0;
  GNUTLS_ALG_LOOP (ret = p->blocksize);
  return ret;

}

/* returns the priority */
int
MHD_gtls_cipher_priority (MHD_gtls_session_t session,
                          enum MHD_GNUTLS_CipherAlgorithm algorithm)
{
  unsigned int i;
  for (i = 0; i < session->internals.priorities.cipher.num_algorithms; i++)
    {
      if (session->internals.priorities.cipher.priority[i] == algorithm)
        return i;
    }
  return -1;
}

int
MHD_gtls_cipher_is_block (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{
  size_t ret = 0;

  GNUTLS_ALG_LOOP (ret = p->block);
  return ret;

}

/**
 * MHD__gnutls_cipher_get_key_size - Returns the length of the cipher's key size
 * @algorithm: is an encryption algorithm
 *
 * Returns: length (in bytes) of the given cipher's key size, o 0 if
 *   the given cipher is invalid.
 **/
size_t
MHD__gnutls_cipher_get_key_size (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{                               /* In bytes */
  size_t ret = 0;
  GNUTLS_ALG_LOOP (ret = p->keysize);
  return ret;

}

int
MHD_gtls_cipher_get_iv_size (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{                               /* In bytes */
  size_t ret = 0;
  GNUTLS_ALG_LOOP (ret = p->iv);
  return ret;

}

int
MHD_gtls_cipher_get_export_flag (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{                               /* In bytes */
  size_t ret = 0;
  GNUTLS_ALG_LOOP (ret = p->export_flag);
  return ret;

}


int
MHD_gtls_cipher_is_ok (enum MHD_GNUTLS_CipherAlgorithm algorithm)
{
  ssize_t ret = -1;
  GNUTLS_ALG_LOOP (ret = p->id);
  if (ret >= 0)
    ret = 0;
  else
    ret = 1;
  return ret;
}

/* Key EXCHANGE functions */
MHD_gtls_mod_auth_st *
MHD_gtls_kx_auth_struct (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm)
{
  MHD_gtls_mod_auth_st *ret = NULL;
  GNUTLS_KX_ALG_LOOP (ret = p->auth_struct);
  return ret;

}

int
MHD_gtls_kx_priority (MHD_gtls_session_t session,
                      enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm)
{
  unsigned int i;
  for (i = 0; i < session->internals.priorities.kx.num_algorithms; i++)
    {
      if (session->internals.priorities.kx.priority[i] == algorithm)
        return i;
    }
  return -1;
}


int
MHD_gtls_kx_is_ok (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm)
{
  ssize_t ret = -1;
  GNUTLS_KX_ALG_LOOP (ret = p->algorithm);
  if (ret >= 0)
    ret = 0;
  else
    ret = 1;
  return ret;
}

int
MHD_gtls_kx_needs_rsa_params (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm)
{
  ssize_t ret = 0;
  GNUTLS_KX_ALG_LOOP (ret = p->needs_rsa_params);
  return ret;
}

int
MHD_gtls_kx_needs_dh_params (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm)
{
  ssize_t ret = 0;
  GNUTLS_KX_ALG_LOOP (ret = p->needs_dh_params);
  return ret;
}

/* Version */
int
MHD_gtls_version_priority (MHD_gtls_session_t session,
                           enum MHD_GNUTLS_Protocol version)
{                               /* actually returns the priority */
  unsigned int i;

  if (session->internals.priorities.protocol.priority == NULL)
    {
      MHD_gnutls_assert ();
      return -1;
    }

  for (i = 0; i < session->internals.priorities.protocol.num_algorithms; i++)
    {
      if (session->internals.priorities.protocol.priority[i] == version)
        return i;
    }
  return -1;
}


enum MHD_GNUTLS_Protocol
MHD_gtls_version_max (MHD_gtls_session_t session)
{                               /* returns the maximum version supported */
  unsigned int i, max = 0x00;

  if (session->internals.priorities.protocol.priority == NULL)
    {
      return MHD_GNUTLS_PROTOCOL_VERSION_UNKNOWN;
    }
  else
    for (i = 0; i < session->internals.priorities.protocol.num_algorithms;
         i++)
      {
        if (session->internals.priorities.protocol.priority[i] > max)
          max = session->internals.priorities.protocol.priority[i];
      }

  if (max == 0x00)
    return MHD_GNUTLS_PROTOCOL_VERSION_UNKNOWN; /* unknown version */

  return max;
}

int
MHD_gtls_version_get_minor (enum MHD_GNUTLS_Protocol version)
{
  int ret = -1;

  GNUTLS_VERSION_ALG_LOOP (ret = p->minor);
  return ret;
}

enum MHD_GNUTLS_Protocol
MHD_gtls_version_get (int major, int minor)
{
  int ret = -1;

  GNUTLS_VERSION_LOOP (if ((p->major == major) && (p->minor == minor))
                       ret = p->id)
    ;
  return ret;
}

int
MHD_gtls_version_get_major (enum MHD_GNUTLS_Protocol version)
{
  int ret = -1;

  GNUTLS_VERSION_ALG_LOOP (ret = p->major);
  return ret;
}

/* Version Functions */

int
MHD_gtls_version_is_supported (MHD_gtls_session_t session,
                               const enum MHD_GNUTLS_Protocol version)
{
  int ret = 0;

  GNUTLS_VERSION_ALG_LOOP (ret = p->supported);
  if (ret == 0)
    return 0;

  if (MHD_gtls_version_priority (session, version) < 0)
    return 0;                   /* disabled by the user */
  else
    return 1;
}

enum MHD_GNUTLS_CredentialsType
MHD_gtls_map_kx_get_cred (enum MHD_GNUTLS_KeyExchangeAlgorithm algorithm,
                          int server)
{
  enum MHD_GNUTLS_CredentialsType ret = -1;
  if (server)
    {
      GNUTLS_KX_MAP_LOOP (if (p->algorithm == algorithm) ret = p->server_type)
        ;
    }
  else
    {
      GNUTLS_KX_MAP_LOOP (if (p->algorithm == algorithm) ret = p->client_type)
        ;
    }

  return ret;
}

/* Cipher Suite's functions */
enum MHD_GNUTLS_CipherAlgorithm
MHD_gtls_cipher_suite_get_cipher_algo (const cipher_suite_st * suite)
{
  int ret = 0;
  GNUTLS_CIPHER_SUITE_ALG_LOOP (ret = p->block_algorithm);
  return ret;
}

enum MHD_GNUTLS_Protocol
MHD_gtls_cipher_suite_get_version (const cipher_suite_st * suite)
{
  int ret = 0;
  GNUTLS_CIPHER_SUITE_ALG_LOOP (ret = p->version);
  return ret;
}

enum MHD_GNUTLS_KeyExchangeAlgorithm
MHD_gtls_cipher_suite_get_kx_algo (const cipher_suite_st * suite)
{
  int ret = 0;

  GNUTLS_CIPHER_SUITE_ALG_LOOP (ret = p->kx_algorithm);
  return ret;

}

enum MHD_GNUTLS_HashAlgorithm
MHD_gtls_cipher_suite_get_mac_algo (const cipher_suite_st * suite)
{                               /* In bytes */
  int ret = 0;
  GNUTLS_CIPHER_SUITE_ALG_LOOP (ret = p->mac_algorithm);
  return ret;

}

const char *
MHD_gtls_cipher_suite_get_name (cipher_suite_st * suite)
{
  const char *ret = NULL;

  /* avoid prefix */
  GNUTLS_CIPHER_SUITE_ALG_LOOP (ret = p->name + sizeof ("GNUTLS_") - 1);

  return ret;
}

static inline int
MHD__gnutls_cipher_suite_is_ok (cipher_suite_st * suite)
{
  size_t ret;
  const char *name = NULL;

  GNUTLS_CIPHER_SUITE_ALG_LOOP (name = p->name);
  if (name != NULL)
    ret = 0;
  else
    ret = 1;
  return ret;

}

#define SWAP(x, y) memcpy(tmp,x,size); \
		   memcpy(x,y,size); \
		   memcpy(y,tmp,size);

#define MAX_ELEM_SIZE 4
static inline int
MHD__gnutls_partition (MHD_gtls_session_t session,
                       void *_base,
                       size_t nmemb,
                       size_t size,
                       int (*compar) (MHD_gtls_session_t,
                                      const void *, const void *))
{
  uint8_t *base = _base;
  uint8_t tmp[MAX_ELEM_SIZE];
  uint8_t ptmp[MAX_ELEM_SIZE];
  unsigned int pivot;
  unsigned int i, j;
  unsigned int full;

  i = pivot = 0;
  j = full = (nmemb - 1) * size;

  memcpy (ptmp, &base[0], size);        /* set pivot item */

  while (i < j)
    {
      while ((compar (session, &base[i], ptmp) <= 0) && (i < full))
        {
          i += size;
        }
      while ((compar (session, &base[j], ptmp) >= 0) && (j > 0))
        j -= size;

      if (i < j)
        {
          SWAP (&base[j], &base[i]);
        }
    }

  if (j > pivot)
    {
      SWAP (&base[pivot], &base[j]);
      pivot = j;
    }
  else if (i < pivot)
    {
      SWAP (&base[pivot], &base[i]);
      pivot = i;
    }
  return pivot / size;
}

static void
MHD__gnutls_qsort (MHD_gtls_session_t session,
                   void *_base,
                   size_t nmemb,
                   size_t size,
                   int (*compar) (MHD_gtls_session_t, const void *,
                                  const void *))
{
  unsigned int pivot;
  char *base = _base;
  size_t snmemb = nmemb;

  if (snmemb <= 1)
    return;
  pivot = MHD__gnutls_partition (session, _base, nmemb, size, compar);

  MHD__gnutls_qsort (session, base, pivot < nmemb ? pivot + 1
                     : pivot, size, compar);
  MHD__gnutls_qsort (session, &base[(pivot + 1) * size], nmemb - pivot - 1,
                     size, compar);
}

/* a compare function for KX algorithms (using priorities).
 * For use with qsort
 */
static int
MHD__gnutls_compare_algo (MHD_gtls_session_t session,
                          const void *i_A1, const void *i_A2)
{
  enum MHD_GNUTLS_KeyExchangeAlgorithm kA1 =
    MHD_gtls_cipher_suite_get_kx_algo ((const cipher_suite_st *) i_A1);
  enum MHD_GNUTLS_KeyExchangeAlgorithm kA2 =
    MHD_gtls_cipher_suite_get_kx_algo ((const cipher_suite_st *) i_A2);
  enum MHD_GNUTLS_CipherAlgorithm cA1 =
    MHD_gtls_cipher_suite_get_cipher_algo ((const cipher_suite_st *) i_A1);
  enum MHD_GNUTLS_CipherAlgorithm cA2 =
    MHD_gtls_cipher_suite_get_cipher_algo ((const cipher_suite_st *) i_A2);
  enum MHD_GNUTLS_HashAlgorithm mA1 =
    MHD_gtls_cipher_suite_get_mac_algo ((const cipher_suite_st *) i_A1);
  enum MHD_GNUTLS_HashAlgorithm mA2 =
    MHD_gtls_cipher_suite_get_mac_algo ((const cipher_suite_st *) i_A2);

  int p1 = (MHD_gtls_kx_priority (session, kA1) + 1) * 64;
  int p2 = (MHD_gtls_kx_priority (session, kA2) + 1) * 64;
  p1 += (MHD_gtls_cipher_priority (session, cA1) + 1) * 8;
  p2 += (MHD_gtls_cipher_priority (session, cA2) + 1) * 8;
  p1 += MHD_gtls_mac_priority (session, mA1);
  p2 += MHD_gtls_mac_priority (session, mA2);

  if (p1 > p2)
    {
      return 1;
    }
  else
    {
      if (p1 == p2)
        {
          return 0;
        }
      return -1;
    }
}

int
MHD_gtls_supported_ciphersuites_sorted (MHD_gtls_session_t session,
                                        cipher_suite_st ** ciphers)
{

  int count;

  count = MHD_gtls_supported_ciphersuites (session, ciphers);
  if (count <= 0)
    {
      MHD_gnutls_assert ();
      return count;
    }
  MHD__gnutls_qsort (session, *ciphers, count, sizeof (cipher_suite_st),
                     MHD__gnutls_compare_algo);

  return count;
}

int
MHD_gtls_supported_ciphersuites (MHD_gtls_session_t session,
                                 cipher_suite_st ** _ciphers)
{

  unsigned int i, ret_count, j;
  unsigned int count = CIPHER_SUITES_COUNT;
  cipher_suite_st *tmp_ciphers;
  cipher_suite_st *ciphers;
  enum MHD_GNUTLS_Protocol version;

  if (count == 0)
    {
      return 0;
    }

  tmp_ciphers = MHD_gnutls_alloca (count * sizeof (cipher_suite_st));
  if (tmp_ciphers == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  ciphers = MHD_gnutls_malloc (count * sizeof (cipher_suite_st));
  if (ciphers == NULL)
    {
      MHD_gnutls_afree (tmp_ciphers);
      return GNUTLS_E_MEMORY_ERROR;
    }

  version = MHD__gnutls_protocol_get_version (session);

  for (i = 0; i < count; i++)
    {
      memcpy (&tmp_ciphers[i], &MHD_gtls_cs_algorithms[i].id,
              sizeof (cipher_suite_st));
    }

  for (i = j = 0; i < count; i++)
    {
      /* remove private cipher suites, if requested.
       */
      if (tmp_ciphers[i].suite[0] == 0xFF)
        continue;

      /* remove cipher suites which do not support the
       * protocol version used.
       */
      if (MHD_gtls_cipher_suite_get_version (&tmp_ciphers[i]) > version)
        continue;

      if (MHD_gtls_kx_priority (session,
                                MHD_gtls_cipher_suite_get_kx_algo
                                (&tmp_ciphers[i])) < 0)
        continue;
      if (MHD_gtls_mac_priority (session,
                                 MHD_gtls_cipher_suite_get_mac_algo
                                 (&tmp_ciphers[i])) < 0)
        continue;
      if (MHD_gtls_cipher_priority (session,
                                    MHD_gtls_cipher_suite_get_cipher_algo
                                    (&tmp_ciphers[i])) < 0)
        continue;

      memcpy (&ciphers[j], &tmp_ciphers[i], sizeof (cipher_suite_st));
      j++;
    }

  ret_count = j;

  MHD_gnutls_afree (tmp_ciphers);

  /* This function can no longer return 0 cipher suites.
   * It returns an error code instead.
   */
  if (ret_count == 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (ciphers);
      return GNUTLS_E_NO_CIPHER_SUITES;
    }
  *_ciphers = ciphers;
  return ret_count;
}

/* For compression  */

#define MIN_PRIVATE_COMP_ALGO 0xEF

/* returns the TLS numbers of the compression methods we support
 */
#define SUPPORTED_COMPRESSION_METHODS session->internals.priorities.compression.num_algorithms
int
MHD_gtls_supported_compression_methods (MHD_gtls_session_t session,
                                        uint8_t ** comp)
{
  unsigned int i, j;

  *comp =
    MHD_gnutls_malloc (sizeof (uint8_t) * SUPPORTED_COMPRESSION_METHODS);
  if (*comp == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  for (i = j = 0; i < SUPPORTED_COMPRESSION_METHODS; i++)
    {
      int tmp =
        MHD_gtls_compression_get_num (session->internals.priorities.
                                      compression.priority[i]);

      /* remove private compression algorithms, if requested.
       */
      if (tmp == -1 || (tmp >= MIN_PRIVATE_COMP_ALGO))
        {
          MHD_gnutls_assert ();
          continue;
        }

      (*comp)[j] = (uint8_t) tmp;
      j++;
    }

  if (j == 0)
    {
      MHD_gnutls_assert ();
      MHD_gnutls_free (*comp);
      *comp = NULL;
      return GNUTLS_E_NO_COMPRESSION_ALGORITHMS;
    }
  return j;
}

static const enum MHD_GNUTLS_CertificateType
  MHD_gtls_supported_certificate_types[] =
{ MHD_GNUTLS_CRT_X509,
  0
};


/* returns the enum MHD_GNUTLS_PublicKeyAlgorithm which is compatible with
 * the given enum MHD_GNUTLS_KeyExchangeAlgorithm.
 */
enum MHD_GNUTLS_PublicKeyAlgorithm
MHD_gtls_map_pk_get_pk (enum MHD_GNUTLS_KeyExchangeAlgorithm kx_algorithm)
{
  enum MHD_GNUTLS_PublicKeyAlgorithm ret = -1;

  GNUTLS_PK_MAP_ALG_LOOP (ret = p->pk_algorithm) return ret;
}

/* Returns the encipher type for the given key exchange algorithm.
 * That one of CIPHER_ENCRYPT, CIPHER_SIGN, CIPHER_IGN.
 *
 * ex. GNUTLS_KX_RSA requires a certificate able to encrypt... so returns CIPHER_ENCRYPT.
 */
enum encipher_type
MHD_gtls_kx_encipher_type (enum MHD_GNUTLS_KeyExchangeAlgorithm kx_algorithm)
{
  int ret = CIPHER_IGN;
  GNUTLS_PK_MAP_ALG_LOOP (ret = p->encipher_type) return ret;

}

/* signature algorithms;
 */
struct MHD_gnutls_sign_entry
{
  const char *name;
  const char *oid;
  MHD_gnutls_sign_algorithm_t id;
  enum MHD_GNUTLS_PublicKeyAlgorithm pk;
  enum MHD_GNUTLS_HashAlgorithm mac;
};
typedef struct MHD_gnutls_sign_entry MHD_gnutls_sign_entry;

static const MHD_gnutls_sign_entry MHD_gtls_sign_algorithms[] = {
  {"RSA-SHA",
   SIG_RSA_SHA1_OID,
   GNUTLS_SIGN_RSA_SHA1,
   MHD_GNUTLS_PK_RSA,
   MHD_GNUTLS_MAC_SHA1},
  {"RSA-SHA256",
   SIG_RSA_SHA256_OID,
   GNUTLS_SIGN_RSA_SHA256,
   MHD_GNUTLS_PK_RSA,
   MHD_GNUTLS_MAC_SHA256},
  {"RSA-MD5",
   SIG_RSA_MD5_OID,
   GNUTLS_SIGN_RSA_MD5,
   MHD_GNUTLS_PK_RSA,
   MHD_GNUTLS_MAC_MD5},
  {"GOST R 34.10-2001",
   SIG_GOST_R3410_2001_OID,
   0,
   0,
   0},
  {"GOST R 34.10-94",
   SIG_GOST_R3410_94_OID,
   0,
   0,
   0},
  {0,
   0,
   0,
   0,
   0}
};

#define GNUTLS_SIGN_LOOP(b) \
  do {								       \
    const MHD_gnutls_sign_entry *p;					       \
    for(p = MHD_gtls_sign_algorithms; p->name != NULL; p++) { b ; }	       \
  } while (0)

#define GNUTLS_SIGN_ALG_LOOP(a) \
  GNUTLS_SIGN_LOOP( if(p->id && p->id == sign) { a; break; } )

/* pk algorithms;
 */
struct MHD_gnutls_pk_entry
{
  const char *name;
  const char *oid;
  enum MHD_GNUTLS_PublicKeyAlgorithm id;
};
typedef struct MHD_gnutls_pk_entry MHD_gnutls_pk_entry;

static const MHD_gnutls_pk_entry MHD_gtls_pk_algorithms[] = {
  {"RSA",
   PK_PKIX1_RSA_OID,
   MHD_GNUTLS_PK_RSA},
  {"GOST R 34.10-2001",
   PK_GOST_R3410_2001_OID,
   0},
  {"GOST R 34.10-94",
   PK_GOST_R3410_94_OID,
   0},
  {0,
   0,
   0}
};

enum MHD_GNUTLS_PublicKeyAlgorithm
MHD_gtls_x509_oid2pk_algorithm (const char *oid)
{
  enum MHD_GNUTLS_PublicKeyAlgorithm ret = MHD_GNUTLS_PK_UNKNOWN;
  const MHD_gnutls_pk_entry *p;

  for (p = MHD_gtls_pk_algorithms; p->name != NULL; p++)
    if (strcmp (p->oid, oid) == 0)
      {
        ret = p->id;
        break;
      }

  return ret;
}
