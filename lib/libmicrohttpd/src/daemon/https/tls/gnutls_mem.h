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

#ifndef GNUTLS_MEM_H
# define GNUTLS_MEM_H

#ifdef USE_DMALLOC
# include <dmalloc.h>
#endif

typedef void svoid;             /* for functions that allocate using MHD_gnutls_secure_malloc */

#ifdef HAVE_ALLOCA
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# endif
# ifndef MHD_gnutls_alloca
#  define MHD_gnutls_alloca alloca
#  define MHD_gnutls_afree(x)
# endif
#else
# ifndef MHD_gnutls_alloca
#  define MHD_gnutls_alloca MHD_gnutls_malloc
#  define MHD_gnutls_afree MHD_gnutls_free
# endif
#endif /* HAVE_ALLOCA */

extern int (*MHD__gnutls_is_secure_memory) (const void *);

/* this realloc function will return ptr if size==0, and
 * will free the ptr if the new allocation failed.
 */
void *MHD_gtls_realloc_fast (void *ptr, size_t size);

svoid *MHD_gtls_secure_calloc (size_t nmemb, size_t size);

void *MHD_gtls_calloc (size_t nmemb, size_t size);
char *MHD_gtls_strdup (const char *);

#endif /* GNUTLS_MEM_H */
