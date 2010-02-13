/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007 Free Software Foundation
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

/* contains functions that make it easier to
 * write vectors of <size|data>. The destination size
 * should be preallocated (datum.size+(bits/8))
 */

#include <gnutls_int.h>
#include <gnutls_num.h>
#include <gnutls_datum.h>
#include <gnutls_errors.h>


void
MHD_gtls_write_datum16 (opaque * dest, MHD_gnutls_datum_t dat)
{
  MHD_gtls_write_uint16 (dat.size, dest);
  if (dat.data != NULL)
    memcpy (&dest[2], dat.data, dat.size);
}

void
MHD_gtls_write_datum24 (opaque * dest, MHD_gnutls_datum_t dat)
{
  MHD_gtls_write_uint24 (dat.size, dest);
  if (dat.data != NULL)
    memcpy (&dest[3], dat.data, dat.size);
}

int
MHD_gtls_set_datum_m (MHD_gnutls_datum_t * dat, const void *data,
                      size_t data_size, MHD_gnutls_alloc_function galloc_func)
{
  if (data_size == 0 || data == NULL)
    {
      dat->data = NULL;
      dat->size = 0;
      return 0;
    }

  dat->data = galloc_func (data_size);
  if (dat->data == NULL)
    return GNUTLS_E_MEMORY_ERROR;

  dat->size = data_size;
  memcpy (dat->data, data, data_size);

  return 0;
}

void
MHD_gtls_free_datum_m (MHD_gnutls_datum_t * dat,
                       MHD_gnutls_free_function gfree_func)
{
  if (dat->data != NULL)
    gfree_func (dat->data);

  dat->data = NULL;
  dat->size = 0;
}
