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

#ifndef GNUTLS_GLOBAL_H
#define GNUTLS_GLOBAL_H

#include "MHD_config.h"
#include <libtasn1.h>
#undef HAVE_CONFIG_H
#include <pthread.h>
#define HAVE_CONFIG_H 1

/* this mutex is used to synchronize threads attempting to call MHD__gnutls_global_init / MHD__gnutls_global_deinit */
extern pthread_mutex_t MHD_gnutls_init_mutex;

int MHD_gnutls_is_secure_memory (const void *mem);

extern ASN1_TYPE MHD__gnutls_pkix1_asn;
extern ASN1_TYPE MHD__gnutlsMHD__gnutls_asn;

#if !HAVE_MEMMEM
extern void *memmem (void const *__haystack, size_t __haystack_len,
                     void const *__needle, size_t __needle_len);
#endif

/* removed const from node_asn* to
 * prevent warnings, since libtasn1 doesn't
 * use the const keywork in its functions.
 */
#define MHD__gnutls_getMHD__gnutls_asn() ((node_asn*) MHD__gnutlsMHD__gnutls_asn)
#define MHD__gnutls_get_pkix() ((node_asn*) MHD__gnutls_pkix1_asn)

#endif
