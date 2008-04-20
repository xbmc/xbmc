/* 
   Unix SMB/CIFS implementation.
   struct samu access routines
   Copyright (C) Jeremy Allison 		1996-2001
   Copyright (C) Luke Kenneth Casson Leighton 	1996-1998
   Copyright (C) Gerald (Jerry) Carter		2000-2001
   Copyright (C) Andrew Bartlett		2001-2002
   Copyright (C) Stefan (metze) Metzmacher	2002
      
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_PASSDB

uint32 pdb_get_user_rid (const struct samu *sampass)
{
	uint32 u_rid;

	if (sampass)
		if (sid_peek_check_rid(get_global_sam_sid(), pdb_get_user_sid(sampass),&u_rid))
			return u_rid;
	
	return (0);
}

uint32 pdb_get_group_rid (struct samu *sampass)
{
	uint32 g_rid;

	if (sampass)
		if (sid_peek_check_rid(get_global_sam_sid(), pdb_get_group_sid(sampass),&g_rid))
			return g_rid;
	return (0);
}

BOOL pdb_set_user_sid_from_rid (struct samu *sampass, uint32 rid, enum pdb_value_state flag)
{
	DOM_SID u_sid;
	const DOM_SID *global_sam_sid;
	
	if (!sampass)
		return False;

	if (!(global_sam_sid = get_global_sam_sid())) {
		DEBUG(1, ("pdb_set_user_sid_from_rid: Could not read global sam sid!\n"));
		return False;
	}

	sid_copy(&u_sid, global_sam_sid);

	if (!sid_append_rid(&u_sid, rid))
		return False;

	if (!pdb_set_user_sid(sampass, &u_sid, flag))
		return False;

	DEBUG(10, ("pdb_set_user_sid_from_rid:\n\tsetting user sid %s from rid %d\n", 
		    sid_string_static(&u_sid),rid));

	return True;
}

BOOL pdb_set_group_sid_from_rid (struct samu *sampass, uint32 grid, enum pdb_value_state flag)
{
	DOM_SID g_sid;
	const DOM_SID *global_sam_sid;

	if (!sampass)
		return False;
	
	if (!(global_sam_sid = get_global_sam_sid())) {
		DEBUG(1, ("pdb_set_user_sid_from_rid: Could not read global sam sid!\n"));
		return False;
	}

	sid_copy(&g_sid, global_sam_sid);
	
	if (!sid_append_rid(&g_sid, grid))
		return False;

	if (!pdb_set_group_sid(sampass, &g_sid, flag))
		return False;

	DEBUG(10, ("pdb_set_group_sid_from_rid:\n\tsetting group sid %s from rid %d\n", 
		    sid_string_static(&g_sid), grid));

	return True;
}

