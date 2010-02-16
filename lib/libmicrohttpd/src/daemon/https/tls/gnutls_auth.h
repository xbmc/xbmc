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

#ifndef GNUTLS_AUTH_H
#define GNUTLS_AUTH_H

typedef struct MHD_gtls_mod_auth_st_int
{
  const char *name;             /* null terminated */
  int (*MHD_gtls_gen_server_certificate) (MHD_gtls_session_t, opaque **);
  int (*MHD_gtls_gen_client_certificate) (MHD_gtls_session_t, opaque **);
  int (*MHD_gtls_gen_server_kx) (MHD_gtls_session_t, opaque **);
  int (*MHD_gtls_gen_client_kx) (MHD_gtls_session_t, opaque **);        /* used in SRP */
  int (*MHD_gtls_gen_client_cert_vrfy) (MHD_gtls_session_t, opaque **);
  int (*MHD_gtls_gen_server_certificate_request) (MHD_gtls_session_t,
                                                  opaque **);

  int (*MHD_gtls_process_server_certificate) (MHD_gtls_session_t, opaque *,
                                              size_t);
  int (*MHD_gtls_process_client_certificate) (MHD_gtls_session_t, opaque *,
                                              size_t);
  int (*MHD_gtls_process_server_kx) (MHD_gtls_session_t, opaque *, size_t);
  int (*MHD_gtls_process_client_kx) (MHD_gtls_session_t, opaque *, size_t);
  int (*MHD_gtls_process_client_cert_vrfy) (MHD_gtls_session_t, opaque *,
                                            size_t);
  int (*MHD_gtls_process_server_certificate_request) (MHD_gtls_session_t,
                                                      opaque *, size_t);
} MHD_gtls_mod_auth_st;

#endif
