/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Guenther Deschner 2006

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

static void smb_nscd_flush_cache(const char *service)
{
#ifdef HAVE_NSCD_FLUSH_CACHE
	if (!nscd_flush_cache(service)) {
		DEBUG(10,("failed to flush nscd cache for '%s' service: %s. "
			  "Is nscd running?\n",
			  service, strerror(errno)));
	}
#endif
}

void smb_nscd_flush_user_cache(void)
{
	smb_nscd_flush_cache("passwd");
}

void smb_nscd_flush_group_cache(void)
{
	smb_nscd_flush_cache("group");
}
