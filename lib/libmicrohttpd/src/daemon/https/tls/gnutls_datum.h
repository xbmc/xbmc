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
#include "gnutls_int.h"

void MHD_gtls_write_datum16 (opaque * dest, MHD_gnutls_datum_t dat);
void MHD_gtls_write_datum24 (opaque * dest, MHD_gnutls_datum_t dat);

int MHD_gtls_set_datum_m (MHD_gnutls_datum_t * dat, const void *data,
                          size_t data_size, MHD_gnutls_alloc_function);
#define MHD__gnutls_set_datum( x, y, z) MHD_gtls_set_datum_m(x,y,z, MHD_gnutls_malloc)
#define MHD__gnutls_sset_datum( x, y, z) MHD_gtls_set_datum_m(x,y,z, MHD_gnutls_secure_malloc)

#define MHD__gnutls_datum_append(x,y,z) MHD_gtls_datum_append_m(x,y,z, MHD_gnutls_realloc)

void MHD_gtls_free_datum_m (MHD_gnutls_datum_t * dat,
                            MHD_gnutls_free_function);
#define MHD__gnutls_free_datum(x) MHD_gtls_free_datum_m(x, MHD_gnutls_free)
