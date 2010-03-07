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

int MHD_gtls_encrypt (MHD_gtls_session_t session, const opaque * headers,
                      size_t headers_size, const opaque * data,
                      size_t data_size, opaque * ciphertext,
                      size_t ciphertext_size, content_type_t type,
                      int random_pad);

int MHD_gtls_decrypt (MHD_gtls_session_t session, opaque * ciphertext,
                      size_t ciphertext_size, uint8_t * data,
                      size_t data_size, content_type_t type);
int MHD_gtls_compressed2ciphertext (MHD_gtls_session_t session,
                                    opaque * cipher_data, int cipher_size,
                                    MHD_gnutls_datum_t compressed,
                                    content_type_t _type, int random_pad);
int MHD_gtls_ciphertext2compressed (MHD_gtls_session_t session,
                                    opaque * compress_data, int compress_size,
                                    MHD_gnutls_datum_t ciphertext,
                                    uint8_t type);
