/*
 * Copyright (C) 2002 John Todd Larason <jtl@molehill.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 */

#ifndef CRYPT_H
#define CRYPT_H

#include "rtv.h"

int rtv_decrypt(const char * cyphertext, u32 cypherlength,
                      char * plainbuf,   u32 plainbuflength,
                u32 * time, u32 * plainlen,
                int checksum_num);
int rtv_encrypt(const char * plaintext,  u32 plaintext_len,
                      char * cyphertext, u32 buffer_len, u32 * cyphertext_len,
                int checksum_num);
#endif


