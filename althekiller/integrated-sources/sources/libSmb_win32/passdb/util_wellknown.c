/* 
   Unix SMB/CIFS implementation.
   Lookup routines for well-known SIDs
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Luke Kenneth Caseson Leighton 1998-1999
   Copyright (C) Jeremy Allison  1999
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

struct sid_name_map_info
{
	const DOM_SID *sid;
	const char *name;
	const struct rid_name_map *known_users;
};

static const struct rid_name_map everyone_users[] = {
	{ 0, "Everyone" },
	{ 0, NULL}};

static const struct rid_name_map creator_owner_users[] = {
	{ 0, "Creator Owner" },
	{ 1, "Creator Group" },
	{ 0, NULL}};

static const struct rid_name_map nt_authority_users[] = {
	{  1, "Dialup" },
	{  2, "Network"},
	{  3, "Batch"},
	{  4, "Interactive"},
	{  6, "Service"},
	{  7, "AnonymousLogon"},
	{  8, "Proxy"},
	{  9, "ServerLogon"},
	{ 10, "Self"},
	{ 11, "Authenticated Users"},
	{ 12, "Restricted"},
	{ 13, "Terminal Server User"},
	{ 14, "Remote Interactive Logon"},
	{ 15, "This Organization"},
	{ 18, "SYSTEM"},
	{ 19, "Local Service"},
	{ 20, "Network Service"},
	{  0,  NULL}};

static struct sid_name_map_info special_domains[] = {
	{ &global_sid_World_Domain, "", everyone_users },
	{ &global_sid_Creator_Owner_Domain, "", creator_owner_users },
	{ &global_sid_NT_Authority, "NT Authority", nt_authority_users },
	{ NULL, NULL, NULL }};

BOOL sid_check_is_wellknown_domain(const DOM_SID *sid, const char **name)
{
	int i;

	for (i=0; special_domains[i].sid != NULL; i++) {
		if (sid_equal(sid, special_domains[i].sid)) {
			if (name != NULL) {
				*name = special_domains[i].name;
			}
			return True;
		}
	}
	return False;
}

BOOL sid_check_is_in_wellknown_domain(const DOM_SID *sid)
{
	DOM_SID dom_sid;
	uint32 rid;

	sid_copy(&dom_sid, sid);
	sid_split_rid(&dom_sid, &rid);
	
	return sid_check_is_wellknown_domain(&dom_sid, NULL);
}

/**************************************************************************
 Looks up a known username from one of the known domains.
***************************************************************************/

BOOL lookup_wellknown_sid(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
			  const char **domain, const char **name)
{
	int i;
	DOM_SID dom_sid;
	uint32 rid;
	const struct rid_name_map *users = NULL;

	sid_copy(&dom_sid, sid);
	if (!sid_split_rid(&dom_sid, &rid)) {
		DEBUG(2, ("Could not split rid from SID\n"));
		return False;
	}

	for (i=0; special_domains[i].sid != NULL; i++) {
		if (sid_equal(&dom_sid, special_domains[i].sid)) {
			*domain = talloc_strdup(mem_ctx,
						special_domains[i].name);
			users = special_domains[i].known_users;
			break;
		}
	}

	if (users == NULL) {
		DEBUG(10, ("SID %s is no special sid\n",
			   sid_string_static(sid)));
		return False;
	}

	for (i=0; users[i].name != NULL; i++) {
		if (rid == users[i].rid) {
			*name = talloc_strdup(mem_ctx, users[i].name);
			return True;
		}
	}

	DEBUG(10, ("RID of special SID %s not found\n",
		   sid_string_static(sid)));

	return False;
}

/**************************************************************************
 Try and map a name to one of the well known SIDs.
***************************************************************************/

BOOL lookup_wellknown_name(TALLOC_CTX *mem_ctx, const char *name,
			   DOM_SID *sid, const char **domain)
{
	int i, j;

	DEBUG(10,("map_name_to_wellknown_sid: looking up %s\n", name));

	for (i=0; special_domains[i].sid != NULL; i++) {
		const struct rid_name_map *users =
			special_domains[i].known_users;

		if (users == NULL)
			continue;

		for (j=0; users[j].name != NULL; j++) {
			if ( strequal(users[j].name, name) ) {
				sid_copy(sid, special_domains[i].sid);
				sid_append_rid(sid, users[j].rid);
				*domain = talloc_strdup(
					mem_ctx, special_domains[i].name);
				return True;
			}
		}
	}

	return False;
}
