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

#include <defines.h>

#define GNUTLS_E_INT_RET_0 -1251

#ifdef __FILE__
# ifdef __LINE__
#  define MHD_gnutls_assert() MHD__gnutls_debug_log( "ASSERT: %s:%d\n", __FILE__,__LINE__);
# else
#  define MHD_gnutls_assert()
# endif
#else /* __FILE__ not defined */
# define MHD_gnutls_assert()
#endif

int MHD_gtls_asn2err (int asn_err);
void MHD_gtls_log (int, const char *fmt, ...);

extern int MHD__gnutls_log_level;

#ifdef C99_MACROS
#define LEVEL(l, ...) if (MHD__gnutls_log_level >= l || MHD__gnutls_log_level > 9) \
	MHD_gtls_log( l, __VA_ARGS__)

#define LEVEL_EQ(l, ...) if (MHD__gnutls_log_level == l || MHD__gnutls_log_level > 9) \
	MHD_gtls_log( l, __VA_ARGS__)

# define MHD__gnutls_debug_log(...) LEVEL(2, __VA_ARGS__)
# define MHD__gnutls_handshake_log(...) LEVEL(3, __VA_ARGS__)
# define MHD__gnutls_buffers_log(...) LEVEL_EQ(6, __VA_ARGS__)
# define MHD__gnutls_hard_log(...) LEVEL(9, __VA_ARGS__)
# define MHD__gnutls_record_log(...) LEVEL(4, __VA_ARGS__)
# define MHD__gnutls_read_log(...) LEVEL_EQ(7, __VA_ARGS__)
# define MHD__gnutls_write_log(...) LEVEL_EQ(7, __VA_ARGS__)
# define MHD__gnutls_x509_log(...) LEVEL(1, __VA_ARGS__)
#else
# define MHD__gnutls_debug_log MHD__gnutls_null_log
# define MHD__gnutls_handshake_log MHD__gnutls_null_log
# define MHD__gnutls_buffers_log MHD__gnutls_null_log
# define MHD__gnutls_hard_log MHD__gnutls_null_log
# define MHD__gnutls_record_log MHD__gnutls_null_log
# define MHD__gnutls_read_log MHD__gnutls_null_log
# define MHD__gnutls_write_log MHD__gnutls_null_log
# define MHD__gnutls_x509_log MHD__gnutls_null_log

void MHD__gnutls_null_log (void *, ...);

#endif /* C99_MACROS */
