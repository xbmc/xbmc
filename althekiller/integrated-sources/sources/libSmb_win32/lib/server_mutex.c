/* 
   Unix SMB/CIFS implementation.
   Authenticate against a remote domain
   Copyright (C) Andrew Tridgell 1992-2002
   Copyright (C) Andrew Bartlett 2002
   
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

/* For reasons known only to MS, many of their NT/Win2k versions
   need serialised access only.  Two connections at the same time
   may (in certain situations) cause connections to be reset,
   or access to be denied.

   This locking allows smbd's mutlithread architecture to look
   like the single-connection that NT makes. */

static char *mutex_server_name;

BOOL grab_server_mutex(const char *name)
{
	mutex_server_name = SMB_STRDUP(name);
	if (!mutex_server_name) {
		DEBUG(0,("grab_server_mutex: malloc failed for %s\n", name));
		return False;
	}
	if (!secrets_named_mutex(mutex_server_name, 10)) {
		DEBUG(10,("grab_server_mutex: failed for %s\n", name));
		SAFE_FREE(mutex_server_name);
		return False;
	}

	return True;
}

void release_server_mutex(void)
{
	if (mutex_server_name) {
		secrets_named_mutex_release(mutex_server_name);
		SAFE_FREE(mutex_server_name);
	}
}
