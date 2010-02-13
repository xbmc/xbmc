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

int MHD__gnutls_fbase64_encode (const char *msg, const uint8_t * data,
                                int data_size, uint8_t ** result);
int MHD__gnutls_base64_decode (const uint8_t * data, size_t data_size,
                               uint8_t ** result);
int MHD__gnutls_fbase64_decode (const char *header, const uint8_t * data,
                                size_t data_size, uint8_t ** result);

#define B64SIZE( data_size) ((data_size%3==0)?((data_size*4)/3):(4+((data_size/3)*4)))

/* The size for B64 encoding + newlines plus header
 */

#define HEADSIZE( hsize) \
	sizeof("-----BEGIN ")-1+sizeof("-----")-1+ \
	sizeof("\n-----END ")-1+sizeof("-----\n")-1+hsize+hsize

#define B64FSIZE( hsize, dsize) \
	(B64SIZE(dsize) + HEADSIZE(hsize) + /*newlines*/ \
	B64SIZE(dsize)/64 + (((B64SIZE(dsize) % 64) > 0) ? 1 : 0))
