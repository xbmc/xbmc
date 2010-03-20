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

#ifndef GNUTLS_SIG_H
# define GNUTLS_SIG_H

int MHD_gtls_tls_sign_hdata (MHD_gtls_session_t session,
                             MHD_gnutls_cert * cert,
                             MHD_gnutls_privkey * pkey,
                             MHD_gnutls_datum_t * signature);

int MHD_gtls_tls_sign_params (MHD_gtls_session_t session,
                              MHD_gnutls_cert * cert,
                              MHD_gnutls_privkey * pkey,
                              MHD_gnutls_datum_t * params,
                              MHD_gnutls_datum_t * signature);

int MHD_gtls_verify_sig_hdata (MHD_gtls_session_t session,
                               MHD_gnutls_cert * cert,
                               MHD_gnutls_datum_t * signature);

int MHD_gtls_verify_sig_params (MHD_gtls_session_t session,
                                MHD_gnutls_cert * cert,
                                const MHD_gnutls_datum_t * params,
                                MHD_gnutls_datum_t * signature);


#endif
