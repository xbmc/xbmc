/* -*- c -*-
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007 Free Software Foundation
 *
 * Author: Nikos Mavroyanopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 *
 */

/* This file contains the types and prototypes for all the
 * high level functionality of gnutls main library. For the
 * extra functionality (which is under the GNU GPL license) check
 * the gnutls/extra.h header. The openssl compatibility layer is
 * in gnutls/openssl.h.
 *
 * The low level cipher functionality is in libgcrypt. Check
 * gcrypt.h
 */

#ifndef GNUTLS_H
#define GNUTLS_H

#ifdef __cplusplus
extern "C"
{
#if 0                           /* keep Emacsens' auto-indent happy */
}
#endif
#endif


/**
 * List of key exchange algorithms.
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_KeyExchangeAlgorithm
{
  MHD_GNUTLS_KX_UNKNOWN = 0,
  MHD_GNUTLS_KX_RSA = 1,
};

/**
 * Server credentials type (note that not all types
 * maybe supported by all MHD builds).
 */
enum MHD_GNUTLS_CredentialsType
{
  /**
   * We have a x.509 certificate.
   */
  MHD_GNUTLS_CRD_CERTIFICATE = 1,

};

/**
 * Enumeration of possible cryptographic
 * hash functions (for MAC and Digest operations).
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_HashAlgorithm
{
  MHD_GNUTLS_MAC_UNKNOWN = 0,
  MHD_GNUTLS_MAC_NULL = 1,
  MHD_GNUTLS_MAC_MD5,
  MHD_GNUTLS_MAC_SHA1,
  MHD_GNUTLS_MAC_SHA256
};

/**
 * List of compression methods.
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_CompressionMethod
{
  MHD_GNUTLS_COMP_UNKNOWN = 0,

  /**
   * No compression.
   */
  MHD_GNUTLS_COMP_NULL = 1,

};
/**
 * Types of certificates.
 */
enum MHD_GNUTLS_CertificateType
{
  MHD_GNUTLS_CRT_UNKNOWN = 0,
  MHD_GNUTLS_CRT_X509 = 1
};

/**
 * List of public key algorithms.
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_PublicKeyAlgorithm
{
  MHD_GNUTLS_PK_UNKNOWN = 0,
  MHD_GNUTLS_PK_RSA = 1,
  MHD_GNUTLS_KX_RSA_EXPORT
};



#define LIBGNUTLS_VERSION "2.2.3"

/* Get size_t. */
#include <stddef.h>

#define GNUTLS_CIPHER_RIJNDAEL_128_CBC GNUTLS_CIPHER_AES_128_CBC
#define GNUTLS_CIPHER_RIJNDAEL_256_CBC GNUTLS_CIPHER_AES_256_CBC
#define GNUTLS_CIPHER_RIJNDAEL_CBC GNUTLS_CIPHER_AES_128_CBC
#define GNUTLS_CIPHER_ARCFOUR GNUTLS_CIPHER_ARCFOUR_128

#define GNUTLS_MAX_SESSION_ID 32
#define TLS_MASTER_SIZE 48
#define TLS_RANDOM_SIZE 32

#include "platform.h"
#include "microhttpd.h"

typedef enum
{
  GNUTLS_PARAMS_RSA_EXPORT = 1,
  GNUTLS_PARAMS_DH
} MHD_gnutls_params_type_t;

  /* exported for other gnutls headers. This is the maximum number of
   * algorithms (ciphers, kx or macs).
   */
#define GNUTLS_MAX_ALGORITHM_NUM 16
#define GNUTLS_COMP_ZLIB GNUTLS_COMP_DEFLATE

typedef enum
{
  GNUTLS_SERVER = 1,
  GNUTLS_CLIENT
} MHD_gnutls_connection_end_t;

typedef enum
{
  GNUTLS_AL_WARNING = 1,
  GNUTLS_AL_FATAL
} MHD_gnutls_alert_level_t;

typedef enum
{
  GNUTLS_A_CLOSE_NOTIFY,
  GNUTLS_A_UNEXPECTED_MESSAGE = 10,
  GNUTLS_A_BAD_RECORD_MAC = 20,
  GNUTLS_A_DECRYPTION_FAILED,
  GNUTLS_A_RECORD_OVERFLOW,
  GNUTLS_A_DECOMPRESSION_FAILURE = 30,
  GNUTLS_A_HANDSHAKE_FAILURE = 40,
  GNUTLS_A_SSL3_NO_CERTIFICATE = 41,
  GNUTLS_A_BAD_CERTIFICATE = 42,
  GNUTLS_A_UNSUPPORTED_CERTIFICATE,
  GNUTLS_A_CERTIFICATE_REVOKED,
  GNUTLS_A_CERTIFICATE_EXPIRED,
  GNUTLS_A_CERTIFICATE_UNKNOWN,
  GNUTLS_A_ILLEGAL_PARAMETER,
  GNUTLS_A_UNKNOWN_CA,
  GNUTLS_A_ACCESS_DENIED,
  GNUTLS_A_DECODE_ERROR = 50,
  GNUTLS_A_DECRYPT_ERROR,
  GNUTLS_A_EXPORT_RESTRICTION = 60,
  GNUTLS_A_PROTOCOL_VERSION = 70,
  GNUTLS_A_INSUFFICIENT_SECURITY,
  GNUTLS_A_INTERNAL_ERROR = 80,
  GNUTLS_A_USER_CANCELED = 90,
  GNUTLS_A_NO_RENEGOTIATION = 100,
  GNUTLS_A_UNSUPPORTED_EXTENSION = 110,
  GNUTLS_A_CERTIFICATE_UNOBTAINABLE = 111,
  GNUTLS_A_UNRECOGNIZED_NAME = 112,
  GNUTLS_A_UNKNOWN_PSK_IDENTITY = 115,
} MHD_gnutls_alert_description_t;

typedef enum
{ GNUTLS_HANDSHAKE_HELLO_REQUEST = 0,
  GNUTLS_HANDSHAKE_CLIENT_HELLO = 1,
  GNUTLS_HANDSHAKE_SERVER_HELLO = 2,
  GNUTLS_HANDSHAKE_CERTIFICATE_PKT = 11,
  GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE = 12,
  GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST = 13,
  GNUTLS_HANDSHAKE_SERVER_HELLO_DONE = 14,
  GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY = 15,
  GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE = 16,
  GNUTLS_HANDSHAKE_FINISHED = 20,
  GNUTLS_HANDSHAKE_SUPPLEMENTAL = 23
} MHD_gnutls_handshake_description_t;

typedef enum
{
  GNUTLS_CERT_INVALID = 2,      /* will be set if the certificate
                                 * was not verified.
                                 */
  GNUTLS_CERT_REVOKED = 32,     /* in X.509 this will be set only if CRLs are checked
                                 */

  /* Those are extra information about the verification
   * process. Will be set only if the certificate was
   * not verified.
   */
  GNUTLS_CERT_SIGNER_NOT_FOUND = 64,
  GNUTLS_CERT_SIGNER_NOT_CA = 128,
  GNUTLS_CERT_INSECURE_ALGORITHM = 256
} MHD_gnutls_certificate_status_t;

typedef enum
{
  GNUTLS_CERT_IGNORE,
  GNUTLS_CERT_REQUEST = 1,
  GNUTLS_CERT_REQUIRE
} MHD_gnutls_certificate_request_t;

typedef enum
{
  GNUTLS_SHUT_RDWR = 0,
  GNUTLS_SHUT_WR = 1
} MHD_gnutls_close_request_t;

typedef enum
{
  GNUTLS_X509_FMT_DER,
  GNUTLS_X509_FMT_PEM
} MHD_gnutls_x509_crt_fmt_t;

typedef enum
{
  GNUTLS_SIGN_UNKNOWN = 0,
  GNUTLS_SIGN_RSA_SHA1 = 1,
  GNUTLS_SIGN_DSA_SHA1,
  GNUTLS_SIGN_RSA_MD5,
  GNUTLS_SIGN_RSA_MD2,
  GNUTLS_SIGN_RSA_RMD160,
  GNUTLS_SIGN_RSA_SHA256,
  GNUTLS_SIGN_RSA_SHA384,
  GNUTLS_SIGN_RSA_SHA512
} MHD_gnutls_sign_algorithm_t;

/* If you want to change this, then also change the define in
 * MHD_gnutls_int.h, and recompile.
 */
typedef void *MHD_gnutls_transport_ptr_t;

struct MHD_gtls_session_int;
typedef struct MHD_gtls_session_int *MHD_gtls_session_t;

struct MHD_gtls_dh_params_int;
typedef struct MHD_gtls_dh_params_int *MHD_gtls_dh_params_t;

struct MHD_gtls_x509_privkey_int;       /* XXX ugly. */
typedef struct MHD_gtls_x509_privkey_int *MHD_gtls_rsa_params_t;        /* XXX ugly. */

struct MHD_gtls_priority_st;
typedef struct MHD_gtls_priority_st *MHD_gnutls_priority_t;

typedef struct
{
  unsigned char *data;
  unsigned int size;
} MHD_gnutls_datum_t;


typedef struct MHD_gnutls_params_st
{
  MHD_gnutls_params_type_t type;
  union params
  {
    MHD_gtls_dh_params_t dh;
    MHD_gtls_rsa_params_t rsa_export;
  } params;
  int deinit;
} MHD_gnutls_params_st;

typedef int MHD_gnutls_params_function (MHD_gtls_session_t,
                                        MHD_gnutls_params_type_t,
                                        MHD_gnutls_params_st *);

/* internal functions */
int MHD__gnutls_global_init (void);
void MHD__gnutls_global_deinit (void);

int MHD__gnutls_init (MHD_gtls_session_t * session,
                      MHD_gnutls_connection_end_t con_end);
void MHD__gnutls_deinit (MHD_gtls_session_t session);

int MHD__gnutls_bye (MHD_gtls_session_t session,
                     MHD_gnutls_close_request_t how);
int MHD__gnutls_handshake (MHD_gtls_session_t session);
int MHD__gnutls_rehandshake (MHD_gtls_session_t session);

MHD_gnutls_alert_description_t MHD_gnutls_alert_get (MHD_gtls_session_t
                                                     session);
int MHD__gnutls_alert_send (MHD_gtls_session_t session,
                            MHD_gnutls_alert_level_t level,
                            MHD_gnutls_alert_description_t desc);
int MHD__gnutls_alert_send_appropriate (MHD_gtls_session_t session, int err);
const char *MHD__gnutls_alert_get_name (MHD_gnutls_alert_description_t alert);

size_t MHD__gnutls_cipher_get_key_size (enum MHD_GNUTLS_CipherAlgorithm
                                        algorithm);

  /* error functions */
int MHD_gtls_error_is_fatal (int error);
int MHD_gtls_error_to_alert (int err, int *level);
void MHD_gtls_perror (int error);
const char *MHD_gtls_strerror (int error);

/*
 * Record layer functions.
 */
ssize_t MHD__gnutls_record_send (MHD_gtls_session_t session,
                                 const void *data, size_t sizeofdata);
ssize_t MHD__gnutls_record_recv (MHD_gtls_session_t session, void *data,
                                 size_t sizeofdata);

  /* provides extra compatibility */
int MHD__gnutls_record_get_direction (MHD_gtls_session_t session);

/*
 * TLS Extensions
 */
typedef enum
{
  GNUTLS_NAME_DNS = 1
} MHD_gnutls_server_name_type_t;

  /* Supplemental data, RFC 4680. */
typedef enum
{
  GNUTLS_SUPPLEMENTAL_USER_MAPPING_DATA = 0
} MHD_gnutls_supplemental_data_format_type_t;


int MHD_tls_set_default_priority (MHD_gnutls_priority_t *,
                                  const char *priority, const char **err_pos);
void MHD__gnutls_priority_deinit (MHD_gnutls_priority_t);

int MHD__gnutls_priority_set (MHD_gtls_session_t session,
                              MHD_gnutls_priority_t);
int MHD__gnutls_priority_set_direct (MHD_gtls_session_t session,
                                     const char *priority,
                                     const char **err_pos);

/* get the currently used protocol version */
enum MHD_GNUTLS_Protocol
MHD__gnutls_protocol_get_version (MHD_gtls_session_t session);

typedef
  int (*MHD_gnutls_handshake_post_client_hello_func) (MHD_gtls_session_t);
void MHD__gnutls_handshake_set_max_packet_length (MHD_gtls_session_t
                                                  session, size_t max);

/*
 * Functions for setting/clearing credentials
 */
void MHD__gnutls_credentials_clear (MHD_gtls_session_t session);

/*
 * cred is a structure defined by the kx algorithm
 */
int MHD__gnutls_credentials_set (MHD_gtls_session_t session,
                                 enum MHD_GNUTLS_CredentialsType type,
                                 void *cred);

/* Credential structures - used in MHD__gnutls_credentials_set(); */
struct MHD_gtls_certificate_credentials_st;
typedef struct MHD_gtls_certificate_credentials_st
  *MHD_gtls_cert_credentials_t;
typedef MHD_gtls_cert_credentials_t MHD_gtls_cert_server_credentials;
typedef MHD_gtls_cert_credentials_t MHD_gtls_cert_client_credentials;

void MHD__gnutls_certificate_free_credentials (MHD_gtls_cert_credentials_t
                                               sc);
int
MHD__gnutls_certificate_allocate_credentials (MHD_gtls_cert_credentials_t
                                              * res);

void MHD__gnutls_certificate_free_keys (MHD_gtls_cert_credentials_t sc);
void MHD__gnutls_certificate_free_cas (MHD_gtls_cert_credentials_t sc);
void MHD__gnutls_certificate_free_ca_names (MHD_gtls_cert_credentials_t sc);

int MHD__gnutls_certificate_set_x509_key_mem (MHD_gtls_cert_credentials_t
                                              res,
                                              const MHD_gnutls_datum_t *
                                              CERT,
                                              const MHD_gnutls_datum_t *
                                              KEY,
                                              MHD_gnutls_x509_crt_fmt_t type);

void MHD__gnutls_certificate_send_x509_rdn_sequence (MHD_gtls_session_t
                                                     session, int status);

/*
 * New functions to allow setting already parsed X.509 stuff.
 */
struct MHD_gtls_x509_privkey_int;
typedef struct MHD_gtls_x509_privkey_int *MHD_gnutls_x509_privkey_t;

struct MHD_gnutls_x509_crl_int;
typedef struct MHD_gnutls_x509_crl_int *MHD_gnutls_x509_crl_t;

struct MHD_gnutls_x509_crt_int;
typedef struct MHD_gnutls_x509_crt_int *MHD_gnutls_x509_crt_t;

/* global state functions
 */


typedef void *(*MHD_gnutls_alloc_function) (size_t);
typedef int (*MHD_gnutls_is_secure_function) (const void *);
typedef void *(*MHD_gnutls_calloc_function) (size_t, size_t);
typedef void (*MHD_gnutls_free_function) (void *);
typedef void *(*MHD_gnutls_realloc_function) (void *, size_t);

/* For use in callbacks */
extern MHD_gnutls_alloc_function MHD_gnutls_malloc;
extern MHD_gnutls_alloc_function MHD_gnutls_secure_malloc;
extern MHD_gnutls_realloc_function MHD_gnutls_realloc;
extern MHD_gnutls_calloc_function MHD_gnutls_calloc;
extern MHD_gnutls_free_function MHD_gnutls_free;

typedef void (*MHD_gnutls_log_func) (int, const char *);
void MHD_gtls_global_set_log_function (MHD_gnutls_log_func log_func);
void MHD_gtls_global_set_log_level (int level);

/*
 * Diffie Hellman parameter handling.
 */
int MHD__gnutls_dh_params_init (MHD_gtls_dh_params_t * dh_params);
void MHD__gnutls_dh_params_deinit (MHD_gtls_dh_params_t dh_params);


/* RSA params */
int MHD__gnutls_rsa_params_init (MHD_gtls_rsa_params_t * rsa_params);
void MHD__gnutls_rsa_params_deinit (MHD_gtls_rsa_params_t rsa_params);
int MHD__gnutls_rsa_params_generate2 (MHD_gtls_rsa_params_t params,
                                      unsigned int bits);


/*
 * Session stuff
 */
typedef ssize_t (*MHD_gtls_pull_func) (MHD_gnutls_transport_ptr_t, void *,
                                       size_t);
typedef ssize_t (*MHD_gtls_push_func) (MHD_gnutls_transport_ptr_t,
                                       const void *, size_t);
void MHD__gnutls_transport_set_ptr (MHD_gtls_session_t session,
                                    MHD_gnutls_transport_ptr_t ptr);
void MHD__gnutls_transport_set_lowat (MHD_gtls_session_t session, int num);


void MHD__gnutls_transport_set_push_function (MHD_gtls_session_t session,
                                              MHD_gtls_push_func push_func);
void MHD__gnutls_transport_set_pull_function (MHD_gtls_session_t session,
                                              MHD_gtls_pull_func pull_func);

typedef enum MHD_gnutls_x509_subject_alt_name_t
{
  GNUTLS_SAN_DNSNAME = 1,
  GNUTLS_SAN_RFC822NAME,
  GNUTLS_SAN_URI,
  GNUTLS_SAN_IPADDRESS,
  GNUTLS_SAN_OTHERNAME,
  GNUTLS_SAN_DN,
  /* The following are "virtual" subject alternative name types, in
     that they are represented by an otherName value and an OID.
     Used by MHD_gnutls_x509_crt_get_subject_alt_othername_oid().  */
  GNUTLS_SAN_OTHERNAME_XMPP = 1000
} MHD_gnutls_x509_subject_alt_name_t;

typedef struct MHD_gnutls_retr_st
{
  enum MHD_GNUTLS_CertificateType type;
  union cert
  {
    MHD_gnutls_x509_crt_t *x509;
  } cert;
  unsigned int ncerts;

  union key
  {
    MHD_gnutls_x509_privkey_t x509;
  } key;

  unsigned int deinit_all;      /* if non zero all keys will be deinited */
} MHD_gnutls_retr_st;

typedef int
MHD_gnutls_certificate_client_retrieve_function (MHD_gtls_session_t,
                                                 const MHD_gnutls_datum_t
                                                 * req_ca_rdn, int nreqs,
                                                 const enum
                                                 MHD_GNUTLS_PublicKeyAlgorithm
                                                 *pk_algos,
                                                 int pk_algos_length,
                                                 MHD_gnutls_retr_st *);

typedef int
MHD_gnutls_certificate_server_retrieve_function (MHD_gtls_session_t,
                                                 MHD_gnutls_retr_st *);

  /*
   * Functions that allow auth_info_t structures handling
   */
enum MHD_GNUTLS_CredentialsType MHD_gtls_auth_get_type (MHD_gtls_session_t
                                                        session);
  /*
   * DH
   */
void MHD__gnutls_dh_set_prime_bits (MHD_gtls_session_t session,
                                    unsigned int bits);

  /* External signing callback.  Experimental. */
typedef int (*MHD_gnutls_sign_func) (MHD_gtls_session_t session,
                                     void *userdata,
                                     enum MHD_GNUTLS_CertificateType
                                     cert_type,
                                     const MHD_gnutls_datum_t * cert,
                                     const MHD_gnutls_datum_t * hash,
                                     MHD_gnutls_datum_t * signature);

  /* key_usage will be an OR of the following values: */
  /* when the key is to be used for signing: */
#define GNUTLS_KEY_DIGITAL_SIGNATURE	128
#define GNUTLS_KEY_NON_REPUDIATION	64
  /* when the key is to be used for encryption: */
#define GNUTLS_KEY_KEY_ENCIPHERMENT	32
#define GNUTLS_KEY_DATA_ENCIPHERMENT	16
#define GNUTLS_KEY_KEY_AGREEMENT	8
#define GNUTLS_KEY_KEY_CERT_SIGN	4
#define GNUTLS_KEY_CRL_SIGN		2
#define GNUTLS_KEY_ENCIPHER_ONLY	1
#define GNUTLS_KEY_DECIPHER_ONLY	32768

  /*
   * Error codes. TLS alert mapping shown in comments.
   */
#define GNUTLS_E_SUCCESS 0
#define	GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM -3
#define	GNUTLS_E_UNKNOWN_CIPHER_TYPE -6
#define	GNUTLS_E_LARGE_PACKET -7
#define GNUTLS_E_UNSUPPORTED_VERSION_PACKET -8  /* GNUTLS_A_PROTOCOL_VERSION */
#define GNUTLS_E_UNEXPECTED_PACKET_LENGTH -9    /* GNUTLS_A_RECORD_OVERFLOW */
#define GNUTLS_E_INVALID_SESSION -10
#define GNUTLS_E_FATAL_ALERT_RECEIVED -12
#define GNUTLS_E_UNEXPECTED_PACKET -15  /* GNUTLS_A_UNEXPECTED_MESSAGE */
#define GNUTLS_E_WARNING_ALERT_RECEIVED -16
#define GNUTLS_E_ERROR_IN_FINISHED_PACKET -18
#define GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET -19
#define	GNUTLS_E_UNKNOWN_CIPHER_SUITE -21       /* GNUTLS_A_HANDSHAKE_FAILURE */
#define	GNUTLS_E_UNWANTED_ALGORITHM -22
#define	GNUTLS_E_MPI_SCAN_FAILED -23
#define GNUTLS_E_DECRYPTION_FAILED -24  /* GNUTLS_A_DECRYPTION_FAILED, GNUTLS_A_BAD_RECORD_MAC */
#define GNUTLS_E_MEMORY_ERROR -25
#define GNUTLS_E_DECOMPRESSION_FAILED -26       /* GNUTLS_A_DECOMPRESSION_FAILURE */
#define GNUTLS_E_COMPRESSION_FAILED -27
#define GNUTLS_E_AGAIN -28
#define GNUTLS_E_EXPIRED -29
#define GNUTLS_E_DB_ERROR -30
#define GNUTLS_E_SRP_PWD_ERROR -31
#define GNUTLS_E_INSUFFICIENT_CREDENTIALS -32
#define GNUTLS_E_INSUFICIENT_CREDENTIALS GNUTLS_E_INSUFFICIENT_CREDENTIALS      /* for backwards compatibility only */
#define GNUTLS_E_INSUFFICIENT_CRED GNUTLS_E_INSUFFICIENT_CREDENTIALS
#define GNUTLS_E_INSUFICIENT_CRED GNUTLS_E_INSUFFICIENT_CREDENTIALS     /* for backwards compatibility only */

#define GNUTLS_E_HASH_FAILED -33
#define GNUTLS_E_BASE64_DECODING_ERROR -34

#define	GNUTLS_E_MPI_PRINT_FAILED -35
#define GNUTLS_E_REHANDSHAKE -37        /* GNUTLS_A_NO_RENEGOTIATION */
#define GNUTLS_E_GOT_APPLICATION_DATA -38
#define GNUTLS_E_RECORD_LIMIT_REACHED -39
#define GNUTLS_E_ENCRYPTION_FAILED -40

#define GNUTLS_E_PK_ENCRYPTION_FAILED -44
#define GNUTLS_E_PK_DECRYPTION_FAILED -45
#define GNUTLS_E_PK_SIGN_FAILED -46
#define GNUTLS_E_X509_UNSUPPORTED_CRITICAL_EXTENSION -47
#define GNUTLS_E_KEY_USAGE_VIOLATION -48
#define GNUTLS_E_NO_CERTIFICATE_FOUND -49       /* GNUTLS_A_BAD_CERTIFICATE */
#define GNUTLS_E_INVALID_REQUEST -50
#define GNUTLS_E_SHORT_MEMORY_BUFFER -51
#define GNUTLS_E_INTERRUPTED -52
#define GNUTLS_E_PUSH_ERROR -53
#define GNUTLS_E_PULL_ERROR -54
#define GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER -55 /* GNUTLS_A_ILLEGAL_PARAMETER */
#define GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE -56
#define GNUTLS_E_PKCS1_WRONG_PAD -57
#define GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION -58
#define GNUTLS_E_INTERNAL_ERROR -59
#define GNUTLS_E_DH_PRIME_UNACCEPTABLE -63
#define GNUTLS_E_FILE_ERROR -64
#define GNUTLS_E_TOO_MANY_EMPTY_PACKETS -78
#define GNUTLS_E_UNKNOWN_PK_ALGORITHM -80


  /* returned if libextra functionality was requested but
   * MHD_gnutls_global_init_extra() was not called.
   */
#define GNUTLS_E_INIT_LIBEXTRA -82
#define GNUTLS_E_LIBRARY_VERSION_MISMATCH -83


  /* returned if you need to generate temporary RSA
   * parameters. These are needed for export cipher suites.
   */
#define GNUTLS_E_NO_TEMPORARY_RSA_PARAMS -84

#define GNUTLS_E_LZO_INIT_FAILED -85
#define GNUTLS_E_NO_COMPRESSION_ALGORITHMS -86
#define GNUTLS_E_NO_CIPHER_SUITES -87

#define GNUTLS_E_PK_SIG_VERIFY_FAILED -89

#define GNUTLS_E_ILLEGAL_SRP_USERNAME -90
#define GNUTLS_E_SRP_PWD_PARSING_ERROR -91
#define GNUTLS_E_NO_TEMPORARY_DH_PARAMS -93

  /* For certificate and key stuff
   */
#define GNUTLS_E_ASN1_ELEMENT_NOT_FOUND -67
#define GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND -68
#define GNUTLS_E_ASN1_DER_ERROR -69
#define GNUTLS_E_ASN1_VALUE_NOT_FOUND -70
#define GNUTLS_E_ASN1_GENERIC_ERROR -71
#define GNUTLS_E_ASN1_VALUE_NOT_VALID -72
#define GNUTLS_E_ASN1_TAG_ERROR -73
#define GNUTLS_E_ASN1_TAG_IMPLICIT -74
#define GNUTLS_E_ASN1_TYPE_ANY_ERROR -75
#define GNUTLS_E_ASN1_SYNTAX_ERROR -76
#define GNUTLS_E_ASN1_DER_OVERFLOW -77
#define GNUTLS_E_CERTIFICATE_ERROR -43
#define GNUTLS_E_X509_CERTIFICATE_ERROR GNUTLS_E_CERTIFICATE_ERROR
#define GNUTLS_E_CERTIFICATE_KEY_MISMATCH -60
#define GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE -61       /* GNUTLS_A_UNSUPPORTED_CERTIFICATE */
#define GNUTLS_E_X509_UNKNOWN_SAN -62
#define GNUTLS_E_X509_UNSUPPORTED_ATTRIBUTE -95
#define GNUTLS_E_UNKNOWN_HASH_ALGORITHM -96
#define GNUTLS_E_UNKNOWN_PKCS_CONTENT_TYPE -97
#define GNUTLS_E_UNKNOWN_PKCS_BAG_TYPE -98
#define GNUTLS_E_INVALID_PASSWORD -99
#define GNUTLS_E_MAC_VERIFY_FAILED -100 /* for PKCS #12 MAC */
#define GNUTLS_E_CONSTRAINT_ERROR -101

#define GNUTLS_E_WARNING_IA_IPHF_RECEIVED -102
#define GNUTLS_E_WARNING_IA_FPHF_RECEIVED -103

#define GNUTLS_E_IA_VERIFY_FAILED -104

#define GNUTLS_E_UNKNOWN_ALGORITHM -105

#define GNUTLS_E_BASE64_ENCODING_ERROR -201
#define GNUTLS_E_INCOMPATIBLE_GCRYPT_LIBRARY -202       /* obsolete */
#define GNUTLS_E_INCOMPATIBLE_CRYPTO_LIBRARY -202
#define GNUTLS_E_INCOMPATIBLE_LIBTASN1_LIBRARY -203

#define GNUTLS_E_X509_UNSUPPORTED_OID -205

#define GNUTLS_E_RANDOM_FAILED -206
#define GNUTLS_E_BASE64_UNEXPECTED_HEADER_ERROR -207


#define GNUTLS_E_UNIMPLEMENTED_FEATURE -1250

#define GNUTLS_E_APPLICATION_ERROR_MAX -65000
#define GNUTLS_E_APPLICATION_ERROR_MIN -65500


#if 0                           /* keep Emacsens' auto-indent happy */
{
#endif
#ifdef __cplusplus
}
#endif

#endif /* GNUTLS_H */
