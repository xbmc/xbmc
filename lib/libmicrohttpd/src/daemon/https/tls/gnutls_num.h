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

#include <gnutls_int.h>

#define rotl32(x,n)   (((x) << ((uint16_t)(n))) | ((x) >> (32 - (uint16_t)(n))))
#define rotr32(x,n)   (((x) >> ((uint16_t)(n))) | ((x) << (32 - (uint16_t)(n))))
#define rotl16(x,n)   (((x) << ((uint16_t)(n))) | ((x) >> (16 - (uint16_t)(n))))
#define rotr16(x,n)   (((x) >> ((uint16_t)(n))) | ((x) << (16 - (uint16_t)(n))))

#define byteswap16(x)  ((rotl16(x, 8) & 0x00ff) | (rotr16(x, 8) & 0xff00))
#define byteswap32(x)  ((rotl32(x, 8) & 0x00ff00ffUL) | (rotr32(x, 8) & 0xff00ff00UL))

uint32_t MHD_gtls_uint24touint32 (uint24 num);
uint24 MHD_gtls_uint32touint24 (uint32_t num);
uint32_t MHD_gtls_read_uint32 (const opaque * data);
uint16_t MHD_gtls_read_uint16 (const opaque * data);
uint32_t MHD_gtls_conv_uint32 (uint32_t data);
uint16_t MHD_gtls_conv_uint16 (uint16_t data);
uint32_t MHD_gtls_read_uint24 (const opaque * data);
void MHD_gtls_write_uint24 (uint32_t num, opaque * data);
void MHD_gtls_write_uint32 (uint32_t num, opaque * data);
void MHD_gtls_write_uint16 (uint16_t num, opaque * data);
uint32_t MHD_gtls_uint64touint32 (const uint64 *);

int MHD_gtls_uint64pp (uint64 *);
#define MHD__gnutls_uint64zero(x) x.i[0] = x.i[1] = x.i[2] = x.i[3] = x.i[4] = x.i[5] = x.i[6] = x.i[7] = 0
#define UINT64DATA(x) x.i
