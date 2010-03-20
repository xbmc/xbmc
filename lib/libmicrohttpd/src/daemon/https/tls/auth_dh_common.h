/*
 * Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation
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

#ifndef AUTH_DH_COMMON
# define AUTH_DH_COMMON

typedef struct
{
  int secret_bits;

  MHD_gnutls_datum_t prime;
  MHD_gnutls_datum_t generator;
  MHD_gnutls_datum_t public_key;
} MHD_gtls_dh_info_st;

void MHD_gtls_free_dh_info (MHD_gtls_dh_info_st * dh);
int MHD_gtls_gen_dh_common_client_kx (MHD_gtls_session_t, opaque **);
int MHD_gtls_proc_dh_common_client_kx (MHD_gtls_session_t session,
                                       opaque * data, size_t _data_size,
                                       mpi_t p, mpi_t g);
int MHD_gtls_dh_common_print_server_kx (MHD_gtls_session_t, mpi_t g, mpi_t p,
                                        opaque ** data, int psk);
int MHD_gtls_proc_dh_common_server_kx (MHD_gtls_session_t session,
                                       opaque * data, size_t _data_size,
                                       int psk);

#endif
