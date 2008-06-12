/*
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Gerald (Jerry) Carter          2004.

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

/* Stupid dummy functions required due to the horrible dependency mess
   in Samba. */

#include "includes.h"

void decrement_smbd_process_count( void )
{
	return;
}

int find_service(fstring service)
{
	return -1;
}

BOOL conn_snum_used(int snum)
{
	return False;
}
