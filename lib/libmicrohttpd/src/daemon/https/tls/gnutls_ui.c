/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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

/* This file contains certificate authentication functions to be exported in the
 * API and did not fit elsewhere.
 */

#include <gnutls_int.h>
#include <auth_cert.h>
#include <gnutls_errors.h>
#include <gnutls_auth_int.h>
#include <gnutls_state.h>
#include <gnutls_datum.h>

/* ANON & DHE */

/**
 * MHD__gnutls_dh_set_prime_bits - Used to set the bits for a DH ciphersuite
 * @session: is a #MHD_gtls_session_t structure.
 * @bits: is the number of bits
 *
 * This function sets the number of bits, for use in an
 * Diffie Hellman key exchange. This is used both in DH ephemeral and
 * DH anonymous cipher suites. This will set the
 * minimum size of the prime that will be used for the handshake.
 *
 * In the client side it sets the minimum accepted number of bits.
 * If a server sends a prime with less bits than that
 * GNUTLS_E_DH_PRIME_UNACCEPTABLE will be returned by the
 * handshake.
 *
 **/
void
MHD__gnutls_dh_set_prime_bits (MHD_gtls_session_t session, unsigned int bits)
{
  session->internals.dh_prime_bits = bits;
}
