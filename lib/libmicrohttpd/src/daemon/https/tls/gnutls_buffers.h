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

int MHD_gnutls_record_buffer_put (content_type_t type,
                                  MHD_gtls_session_t session, opaque * data,
                                  size_t length);
int MHD_gnutls_record_buffer_get_size (content_type_t type,
                                       MHD_gtls_session_t session);
int MHD_gtls_record_buffer_get (content_type_t type,
                                MHD_gtls_session_t session, opaque * data,
                                size_t length);
ssize_t MHD_gtls_io_read_buffered (MHD_gtls_session_t, opaque ** iptr,
                                   size_t n, content_type_t);
void MHD_gtls_io_clear_read_buffer (MHD_gtls_session_t);
int MHD_gtls_io_clear_peeked_data (MHD_gtls_session_t session);

ssize_t MHD_gtls_io_write_buffered (MHD_gtls_session_t, const void *iptr,
                                    size_t n);
ssize_t MHD_gtls_io_write_buffered2 (MHD_gtls_session_t, const void *iptr,
                                     size_t n, const void *iptr2, size_t n2);

int MHD_gtls_handshake_buffer_put (MHD_gtls_session_t session, opaque * data,
                                   size_t length);
int MHD_gtls_handshake_buffer_clear (MHD_gtls_session_t session);
int MHD_gtls_handshake_buffer_empty (MHD_gtls_session_t session);
int MHD_gtls_handshake_buffer_get_ptr (MHD_gtls_session_t session,
                                       opaque ** data_ptr, size_t * length);

#define MHD__gnutls_handshake_io_buffer_clear( session) \
        MHD_gtls_buffer_clear( &session->internals.handshake_send_buffer); \
        MHD_gtls_buffer_clear( &session->internals.handshake_recv_buffer); \
        session->internals.handshake_send_buffer_prev_size = 0

ssize_t MHD_gtls_handshake_io_recv_int (MHD_gtls_session_t, content_type_t,
                                        MHD_gnutls_handshake_description_t,
                                        void *, size_t);
ssize_t MHD_gtls_handshake_io_send_int (MHD_gtls_session_t, content_type_t,
                                        MHD_gnutls_handshake_description_t,
                                        const void *, size_t);
ssize_t MHD_gtls_io_write_flush (MHD_gtls_session_t session);
ssize_t MHD_gtls_handshake_io_write_flush (MHD_gtls_session_t session);

size_t MHD_gtls_record_check_pending (MHD_gtls_session_t session);
