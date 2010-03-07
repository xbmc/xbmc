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

#ifndef GNUTLS_MPI_H
#define GNUTLS_MPI_H

# include <gnutls_int.h>
# include <gcrypt.h>
# include "gc.h"

typedef gcry_mpi_t mpi_t;

#define MHD__gnutls_mpi_cmp gcry_mpi_cmp
#define MHD__gnutls_mpi_cmp_ui gcry_mpi_cmp_ui
#define MHD__gnutls_mpi_new gcry_mpi_new
#define MHD__gnutls_mpi_snew gcry_mpi_snew
#define MHD__gnutls_mpi_copy gcry_mpi_copy
#define MHD__gnutls_mpi_randomize gcry_mpi_randomize
#define MHD__gnutls_mpi_get_nbits gcry_mpi_get_nbits
#define MHD__gnutls_mpi_powm gcry_mpi_powm
#define MHD__gnutls_mpi_invm gcry_mpi_invm
#define MHD__gnutls_mpi_alloc_like(x) MHD__gnutls_mpi_new(MHD__gnutls_mpi_get_nbits(x))

void MHD_gtls_mpi_release (mpi_t * x);

int MHD_gtls_mpi_scan_nz (mpi_t * ret_mpi, const opaque * buffer,
                          size_t * nbytes);
int MHD_gtls_mpi_scan (mpi_t * ret_mpi, const opaque * buffer,
                       size_t * nbytes);
int MHD_gtls_mpi_print (void *buffer, size_t * nbytes, const mpi_t a);
int MHD_gtls_mpi_dprint_lz (MHD_gnutls_datum_t * dest, const mpi_t a);
int MHD_gtls_mpi_dprint (MHD_gnutls_datum_t * dest, const mpi_t a);

#endif
