/* 
   Unix SMB/CIFS implementation.

   An implementation of arc4.

   Copyright (C) Jeremy Allison 2005.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

/*****************************************************************
 Initialize state for an arc4 crypt/decrpyt.
 arc4 state is 258 bytes - last 2 bytes are the index bytes.
*****************************************************************/

void smb_arc4_init(unsigned char arc4_state_out[258], const unsigned char *key, size_t keylen)
{
	size_t ind;
	unsigned char j = 0;

	for (ind = 0; ind < 256; ind++) {
		arc4_state_out[ind] = (unsigned char)ind;
	}

	for( ind = 0; ind < 256; ind++) {
		unsigned char tc;

		j += (arc4_state_out[ind] + key[ind%keylen]);

		tc = arc4_state_out[ind];
		arc4_state_out[ind] = arc4_state_out[j];
		arc4_state_out[j] = tc;
	}
	arc4_state_out[256] = 0;
	arc4_state_out[257] = 0;
}

/*****************************************************************
 Do the arc4 crypt/decrpyt.
 arc4 state is 258 bytes - last 2 bytes are the index bytes.
*****************************************************************/

void smb_arc4_crypt(unsigned char arc4_state_inout[258], unsigned char *data, size_t len)
{
	unsigned char index_i = arc4_state_inout[256];
	unsigned char index_j = arc4_state_inout[257];
        size_t ind;

	for( ind = 0; ind < len; ind++) {
		unsigned char tc;
		unsigned char t;

		index_i++;
		index_j += arc4_state_inout[index_i];

		tc = arc4_state_inout[index_i];
		arc4_state_inout[index_i] = arc4_state_inout[index_j];
		arc4_state_inout[index_j] = tc;

		t = arc4_state_inout[index_i] + arc4_state_inout[index_j];
		data[ind] = data[ind] ^ arc4_state_inout[t];
	}

	arc4_state_inout[256] = index_i;
	arc4_state_inout[257] = index_j;
}
