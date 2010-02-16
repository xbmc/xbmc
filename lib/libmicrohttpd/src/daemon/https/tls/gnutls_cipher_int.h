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

#ifndef GNUTLS_CIPHER_INT
# define GNUTLS_CIPHER_INT

#define cipher_hd_t MHD_gc_cipher_handle
#define GNUTLS_CIPHER_FAILED NULL

// TODO MHD_gc_cipher_handle -> void * x3
void *MHD_gtls_cipher_init (enum MHD_GNUTLS_CipherAlgorithm cipher,
                            const MHD_gnutls_datum_t * key,
                            const MHD_gnutls_datum_t * iv);

int MHD_gtls_cipher_encrypt (void *handle, void *text, int textlen);

int MHD_gtls_cipher_decrypt (void *handle,
                             void *ciphertext, int ciphertextlen);

void MHD_gnutls_cipher_deinit (void *handle);

#endif /* GNUTLS_CIPHER_INT */
