/*
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Simo Sorce 2001

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

#ifndef _UTIL_GETENT_H
#define _UTIL_GETENT_H

/* Element for a single linked list of group entries */
/* Replace the use of struct group in some cases */
/* Used by getgrent_list() */

struct sys_grent {
	char *gr_name;
	char *gr_passwd;
	gid_t gr_gid;
	char **gr_mem;
	struct sys_grent *next;
};

/* Element for a single linked list of passwd entries */
/* Replace the use of struct passwd in some cases */
/* Used by getpwent_list() */

struct sys_pwent {
	char *pw_name;
	char *pw_passwd;
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_gecos;
	char *pw_dir;
	char *pw_shell;
	struct sys_pwent *next;
};

/* Element for a single linked list of user names in a group. */
/* Used to return group lists that may span multiple lines in 
   /etc/group file. */
/* Used by get_users_in_group() */

struct sys_userlist {
	struct sys_userlist *next, *prev;
	char *unix_name;
};

#endif /* _UTIL_GETENT_H */
