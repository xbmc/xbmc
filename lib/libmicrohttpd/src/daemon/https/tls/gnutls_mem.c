/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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
#include <gnutls_errors.h>
#include <gnutls_num.h>

MHD_gnutls_alloc_function MHD_gnutls_secure_malloc = malloc;
MHD_gnutls_alloc_function MHD_gnutls_malloc = malloc;
MHD_gnutls_free_function MHD_gnutls_free = free;
MHD_gnutls_realloc_function MHD_gnutls_realloc = realloc;

void *(*MHD_gnutls_calloc) (size_t, size_t) = calloc;

int
MHD__gnutls_is_secure_mem_null (const void *ign)
{
  return 0;
}

int (*MHD__gnutls_is_secure_memory) (const void *) =
  MHD__gnutls_is_secure_mem_null;



/* This realloc will free ptr in case realloc
 * fails.
 */
void *
MHD_gtls_realloc_fast (void *ptr, size_t size)
{
  void *ret;

  if (size == 0)
    return ptr;

  ret = MHD_gnutls_realloc (ptr, size);
  if (ret == NULL)
    {
      MHD_gnutls_free (ptr);
    }

  return ret;
}
