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

const void *MHD_gtls_get_cred (MHD_gtls_key_st key,
                               enum MHD_GNUTLS_CredentialsType kx, int *err);
const void *MHD_gtls_get_kx_cred (MHD_gtls_session_t session,
                                  enum MHD_GNUTLS_KeyExchangeAlgorithm algo,
                                  int *err);
void *MHD_gtls_get_auth_info (MHD_gtls_session_t session);
int MHD_gtls_auth_info_set (MHD_gtls_session_t session,
                            enum MHD_GNUTLS_CredentialsType type, int size,
                            int allow_change);
