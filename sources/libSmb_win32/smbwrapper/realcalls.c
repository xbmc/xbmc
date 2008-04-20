/* 
   Unix SMB/CIFS implementation.
   SMB wrapper functions for calls that syscall() can't do
   Copyright (C) Andrew Tridgell 1998
   
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
#include "realcalls.h"

#ifdef REPLACE_UTIME
int real_utime(const char *name, struct utimbuf *buf)
{
	struct timeval tv[2];
	
	tv[0].tv_sec = buf->actime;
	tv[0].tv_usec = 0;
	tv[1].tv_sec = buf->modtime;
	tv[1].tv_usec = 0;
	
	return real_utimes(name, &tv[0]);
}
#endif

#ifdef REPLACE_UTIMES
int real_utimes(const char *name, struct timeval tv[2])
{
	struct utimbuf buf;

	buf.actime = tv[0].tv_sec;
	buf.modtime = tv[1].tv_sec;
	
	return real_utime(name, &buf);
}
#endif
