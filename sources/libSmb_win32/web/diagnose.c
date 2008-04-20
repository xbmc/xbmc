/* 
   Unix SMB/CIFS implementation.
   diagnosis tools for web admin
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
#include "web/swat_proto.h"

extern struct in_addr loopback_ip;

#ifdef WITH_WINBIND

/* check to see if winbind is running by pinging it */

BOOL winbindd_running(void)
{
	return winbind_ping();
}	
#endif

/* check to see if nmbd is running on localhost by looking for a __SAMBA__
   response */
BOOL nmbd_running(void)
{
	int fd, count, flags;
	struct in_addr *ip_list;

	if ((fd = open_socket_in(SOCK_DGRAM, 0, 3,
				 interpret_addr("127.0.0.1"), True)) != -1) {
		if ((ip_list = name_query(fd, "__SAMBA__", 0, 
					  True, True, loopback_ip,
					  &count, &flags, NULL)) != NULL) {
			SAFE_FREE(ip_list);
#ifdef _XBOX
			closesocket(fd);
#elif	
			close(fd);
#endif			
			return True;
		}
		close (fd);
	}

	return False;
}


/* check to see if smbd is running on localhost by trying to open a connection
   then closing it */
BOOL smbd_running(void)
{
	static struct cli_state cli;

	if (!cli_initialise(&cli))
		return False;

	if (!cli_connect(&cli, global_myname(), &loopback_ip)) {
		cli_shutdown(&cli);
		return False;
	}

	cli_shutdown(&cli);
	return True;
}
