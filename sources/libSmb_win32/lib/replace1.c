/* 
   Unix SMB/CIFS implementation.
   replacement routines for broken systems
   Copyright (C) Andrew Tridgell 1992-1998
   
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

 void replace1_dummy(void);
 void replace1_dummy(void) {}

#ifndef HAVE_SETENV
 int setenv(const char *name, const char *value, int overwrite) 
{
	char *p = NULL;
	int ret = -1;

	asprintf(&p, "%s=%s", name, value);

	if (overwrite || getenv(name)) {
		if (p) ret = putenv(p);
	} else {
		ret = 0;
	}

	return ret;	
}
#endif
