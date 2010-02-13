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

ssize_t MHD_gtls_send_int (MHD_gtls_session_t session, content_type_t type,
                           MHD_gnutls_handshake_description_t htype,
                           const void *data, size_t sizeofdata);
ssize_t MHD_gtls_recv_int (MHD_gtls_session_t session, content_type_t type,
                           MHD_gnutls_handshake_description_t, opaque * data,
                           size_t sizeofdata);
ssize_t MHD_gtls_send_change_cipher_spec (MHD_gtls_session_t session,
                                          int again);
void MHD__gnutls_transport_set_lowat (MHD_gtls_session_t session, int num);
