/* 
   Unix SMB/CIFS implementation.
   Translate BUILTIN names to SIDs and vice versa
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

struct rid_name_map {
	uint32 rid;
	const char *name;
};

static const struct rid_name_map builtin_aliases[] = {
	{ BUILTIN_ALIAS_RID_ADMINS,		"Administrators" },
	{ BUILTIN_ALIAS_RID_USERS,		"Users" },
	{ BUILTIN_ALIAS_RID_GUESTS,		"Guests" },
	{ BUILTIN_ALIAS_RID_POWER_USERS,	"Power Users" },
	{ BUILTIN_ALIAS_RID_ACCOUNT_OPS,	"Account Operators" },
	{ BUILTIN_ALIAS_RID_SYSTEM_OPS,		"Server Operators" },
	{ BUILTIN_ALIAS_RID_PRINT_OPS,		"Print Operators" },
	{ BUILTIN_ALIAS_RID_BACKUP_OPS,		"Backup Operators" },
	{ BUILTIN_ALIAS_RID_REPLICATOR,		"Replicator" },
	{ BUILTIN_ALIAS_RID_RAS_SERVERS,	"RAS Servers" },
	{ BUILTIN_ALIAS_RID_PRE_2K_ACCESS,	"Pre-Windows 2000 Compatible Access" },
	{  0, NULL}};

/*******************************************************************
 Look up a rid in the BUILTIN domain
 ********************************************************************/
BOOL lookup_builtin_rid(TALLOC_CTX *mem_ctx, uint32 rid, const char **name)
{
	const struct rid_name_map *aliases = builtin_aliases;

	while (aliases->name != NULL) {
		if (rid == aliases->rid) {
			*name = talloc_strdup(mem_ctx, aliases->name);
			return True;
		}
		aliases++;
	}

	return False;
}

/*******************************************************************
 Look up a name in the BUILTIN domain
 ********************************************************************/
BOOL lookup_builtin_name(const char *name, uint32 *rid)
{
	const struct rid_name_map *aliases = builtin_aliases;

	while (aliases->name != NULL) {
		if (strequal(name, aliases->name)) {
			*rid = aliases->rid;
			return True;
		}
		aliases++;
	}

	return False;
}

/*****************************************************************
 Return the name of the BUILTIN domain
*****************************************************************/  

const char *builtin_domain_name(void)
{
	return "BUILTIN";
}

/*****************************************************************
 Check if the SID is the builtin SID (S-1-5-32).
*****************************************************************/  

BOOL sid_check_is_builtin(const DOM_SID *sid)
{
	return sid_equal(sid, &global_sid_Builtin);
}

/*****************************************************************
 Check if the SID is one of the builtin SIDs (S-1-5-32-a).
*****************************************************************/  

BOOL sid_check_is_in_builtin(const DOM_SID *sid)
{
	DOM_SID dom_sid;
	uint32 rid;

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);
	
	return sid_check_is_builtin(&dom_sid);
}

