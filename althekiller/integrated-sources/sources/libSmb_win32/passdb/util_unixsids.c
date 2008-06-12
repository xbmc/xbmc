/* 
   Unix SMB/CIFS implementation.
   Translate unix-defined names to SIDs and vice versa
   Copyright (C) Volker Lendecke 2005
      
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

BOOL sid_check_is_unix_users(const DOM_SID *sid)
{
	return sid_equal(sid, &global_sid_Unix_Users);
}

BOOL sid_check_is_in_unix_users(const DOM_SID *sid)
{
	DOM_SID dom_sid;
	uint32 rid;

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);
	
	return sid_check_is_unix_users(&dom_sid);
}

BOOL uid_to_unix_users_sid(uid_t uid, DOM_SID *sid)
{
	sid_copy(sid, &global_sid_Unix_Users);
	return sid_append_rid(sid, uid);
}

BOOL gid_to_unix_groups_sid(gid_t gid, DOM_SID *sid)
{
	sid_copy(sid, &global_sid_Unix_Groups);
	return sid_append_rid(sid, gid);
}

const char *unix_users_domain_name(void)
{
	return "Unix User";
}

BOOL lookup_unix_user_name(const char *name, DOM_SID *sid)
{
	struct passwd *pwd;

	pwd = getpwnam_alloc(NULL, name);
	if (pwd == NULL) {
		return False;
	}

	sid_copy(sid, &global_sid_Unix_Users);
	sid_append_rid(sid, pwd->pw_uid); /* For 64-bit uid's we have enough
					  * space ... */
	TALLOC_FREE(pwd);
	return True;
}

BOOL sid_check_is_unix_groups(const DOM_SID *sid)
{
	return sid_equal(sid, &global_sid_Unix_Groups);
}

BOOL sid_check_is_in_unix_groups(const DOM_SID *sid)
{
	DOM_SID dom_sid;
	uint32 rid;

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);
	
	return sid_check_is_unix_groups(&dom_sid);
}

const char *unix_groups_domain_name(void)
{
	return "Unix Group";
}

BOOL lookup_unix_group_name(const char *name, DOM_SID *sid)
{
	struct group *grp;

	grp = getgrnam(name);
	if (grp == NULL) {
		return False;
	}

	sid_copy(sid, &global_sid_Unix_Groups);
	sid_append_rid(sid, grp->gr_gid); /* For 64-bit uid's we have enough
					   * space ... */
	return True;
}
