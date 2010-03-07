/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006 Free Software Foundation
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

#ifndef GNUTLS_STATE_H
# define GNUTLS_STATE_H

#include <gnutls_int.h>

void MHD__gnutls_session_cert_type_set (MHD_gtls_session_t session,
                                        enum MHD_GNUTLS_CertificateType);
enum MHD_GNUTLS_KeyExchangeAlgorithm MHD_gnutls_kx_get (MHD_gtls_session_t
                                                        session);
enum MHD_GNUTLS_CipherAlgorithm MHD_gnutls_cipher_get (MHD_gtls_session_t
                                                       session);
enum MHD_GNUTLS_CertificateType
MHD_gnutls_certificate_type_get (MHD_gtls_session_t);

#include <gnutls_auth_int.h>

#define CHECK_AUTH(auth, ret) if (MHD_gtls_auth_get_type(session) != auth) { \
	MHD_gnutls_assert(); \
	return ret; \
	}

#endif

int MHD_gtls_session_cert_type_supported (MHD_gtls_session_t,
                                          enum MHD_GNUTLS_CertificateType);

int MHD_gtls_dh_set_secret_bits (MHD_gtls_session_t session, unsigned bits);

int MHD_gtls_dh_set_peer_public (MHD_gtls_session_t session, mpi_t public);
int MHD_gtls_dh_set_group (MHD_gtls_session_t session, mpi_t gen,
                           mpi_t prime);

int MHD_gtls_dh_get_allowed_prime_bits (MHD_gtls_session_t session);
void MHD_gtls_handshake_internal_state_clear (MHD_gtls_session_t);

int MHD_gtls_rsa_export_set_pubkey (MHD_gtls_session_t session,
                                    mpi_t exponent, mpi_t modulus);

int MHD_gtls_session_is_resumable (MHD_gtls_session_t session);
int MHD_gtls_session_is_export (MHD_gtls_session_t session);

int MHD_gtls_openpgp_send_fingerprint (MHD_gtls_session_t session);

int MHD_gtls_PRF (MHD_gtls_session_t session,
                  const opaque * secret, int secret_size,
                  const char *label, int label_size,
                  const opaque * seed, int seed_size,
                  int total_bytes, void *ret);

int MHD__gnutls_init (MHD_gtls_session_t * session,
                      MHD_gnutls_connection_end_t con_end);

#define DEFAULT_CERT_TYPE MHD_GNUTLS_CRT_X509
