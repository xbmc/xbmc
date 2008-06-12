/* 
   Unix SMB/CIFS implementation.
   session handling for recording currently vailid vuids
   Copyright (C) tridge@samba.org 2001
   Copyright (C) Andew Bartlett <abartlet@samba.org> 2001
   
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

/* a "session" is claimed when we do a SessionSetupX operation
   and is yielded when the corresponding vuid is destroyed.

   sessions are used to populate utmp and PAM session structures
*/

struct sessionid {
	uid_t uid;
	gid_t gid;
	fstring username;
	fstring hostname;
	fstring netbios_name;
	fstring remote_machine;
	fstring id_str;
	uint32  id_num;
	uint32  pid;
	fstring ip_addr;
};

