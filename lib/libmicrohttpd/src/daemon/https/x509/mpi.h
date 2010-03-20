/*
 * Copyright (C) 2003, 2004, 2005 Free Software Foundation
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

#include <gnutls_int.h>
#include "x509.h"

int MHD__gnutls_x509_crt_get_mpis (MHD_gnutls_x509_crt_t cert,
                                   mpi_t * params, int *params_size);
int MHD__gnutls_x509_read_rsa_params (opaque * der, int dersize,
                                      mpi_t * params);
int MHD__gnutls_x509_write_rsa_params (mpi_t * params, int params_size,
                                       MHD_gnutls_datum_t * der);
int MHD__gnutls_x509_read_int (ASN1_TYPE node, const char *value,
                               mpi_t * ret_mpi);
int MHD__gnutls_x509_write_int (ASN1_TYPE node, const char *value, mpi_t mpi,
                                int lz);
