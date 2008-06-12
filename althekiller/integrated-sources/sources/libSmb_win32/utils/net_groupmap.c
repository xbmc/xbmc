/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-2000,
 *  Copyright (C) Jean François Micouleau      1998-2001.
 *  Copyright (C) Gerald Carter                2003,
 *  Copyright (C) Volker Lendecke              2004
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "includes.h"
#include "utils/net.h"


/*********************************************************
 utility function to parse an integer parameter from 
 "parameter = value"
**********************************************************/
static uint32 get_int_param( const char* param )
{
	char *p;
	
	p = strchr( param, '=' );
	if ( !p )
		return 0;
		
	return atoi(p+1);
}

/*********************************************************
 utility function to parse an integer parameter from 
 "parameter = value"
**********************************************************/
static char* get_string_param( const char* param )
{
	char *p;
	
	p = strchr( param, '=' );
	if ( !p )
		return NULL;
		
	return (p+1);
}

/*********************************************************
 Figure out if the input was an NT group or a SID string.
 Return the SID.
**********************************************************/
static BOOL get_sid_from_input(DOM_SID *sid, char *input)
{
	GROUP_MAP map;

	if (StrnCaseCmp( input, "S-", 2)) {
		/* Perhaps its the NT group name? */
		if (!pdb_getgrnam(&map, input)) {
			printf("NT Group %s doesn't exist in mapping DB\n", input);
			return False;
		} else {
			*sid = map.sid;
		}
	} else {
		if (!string_to_sid(sid, input)) {
			printf("converting sid %s from a string failed!\n", input);
			return False;
		}
	}
	return True;
}

/*********************************************************
 Dump a GROUP_MAP entry to stdout (long or short listing)
**********************************************************/

static void print_map_entry ( GROUP_MAP map, BOOL long_list )
{
	if (!long_list)
		d_printf("%s (%s) -> %s\n", map.nt_name,
			 sid_string_static(&map.sid), gidtoname(map.gid));
	else {
		d_printf("%s\n", map.nt_name);
		d_printf("\tSID       : %s\n", sid_string_static(&map.sid));
		d_printf("\tUnix gid  : %d\n", map.gid);
		d_printf("\tUnix group: %s\n", gidtoname(map.gid));
		d_printf("\tGroup type: %s\n",
			 sid_type_lookup(map.sid_name_use));
		d_printf("\tComment   : %s\n", map.comment);
	}

}
/*********************************************************
 List the groups.
**********************************************************/
static int net_groupmap_list(int argc, const char **argv)
{
	size_t entries;
	BOOL long_list = False;
	size_t i;
	fstring ntgroup = "";
	fstring sid_string = "";

	if (opt_verbose || opt_long_list_entries)
		long_list = True;
	
	/* get the options */
	for ( i=0; i<argc; i++ ) {
		if ( !StrCaseCmp(argv[i], "verbose")) {
			long_list = True;
		}
		else if ( !StrnCaseCmp(argv[i], "ntgroup", strlen("ntgroup")) ) {
			fstrcpy( ntgroup, get_string_param( argv[i] ) );
			if ( !ntgroup[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "sid", strlen("sid")) ) {
			fstrcpy( sid_string, get_string_param( argv[i] ) );
			if ( !sid_string[0] ) {
				d_fprintf(stderr, "must supply a SID\n");
				return -1;
			}		
		}
		else {
			d_fprintf(stderr, "Bad option: %s\n", argv[i]);
			return -1;
		}
	}

	/* list a single group is given a name */
	if ( ntgroup[0] || sid_string[0] ) {
		DOM_SID sid;
		GROUP_MAP map;
		
		if ( sid_string[0] )
			fstrcpy( ntgroup, sid_string);
			
		if (!get_sid_from_input(&sid, ntgroup)) {
			return -1;
		}

		/* Get the current mapping from the database */
		if(!pdb_getgrsid(&map, sid)) {
			d_fprintf(stderr, "Failure to local group SID in the database\n");
			return -1;
		}
	
		print_map_entry( map, long_list );
	}
	else {
		GROUP_MAP *map=NULL;
		/* enumerate all group mappings */
		if (!pdb_enum_group_mapping(NULL, SID_NAME_UNKNOWN, &map, &entries, ENUM_ALL_MAPPED))
			return -1;
	
		for (i=0; i<entries; i++) {
			print_map_entry( map[i], long_list );
		}

		SAFE_FREE(map);
	}

	return 0;
}

/*********************************************************
 Add a new group mapping entry
**********************************************************/

static int net_groupmap_add(int argc, const char **argv)
{
	DOM_SID sid;
	fstring ntgroup = "";
	fstring unixgrp = "";
	fstring string_sid = "";
	fstring type = "";
	fstring ntcomment = "";
	enum SID_NAME_USE sid_type = SID_NAME_DOM_GRP;
	uint32 rid = 0;	
	gid_t gid;
	int i;
	const char *name_type = "domain group";

	/* get the options */
	for ( i=0; i<argc; i++ ) {
		if ( !StrnCaseCmp(argv[i], "rid", strlen("rid")) ) {
			rid = get_int_param(argv[i]);
			if ( rid < DOMAIN_GROUP_RID_ADMINS ) {
				d_fprintf(stderr, "RID must be greater than %d\n", (uint32)DOMAIN_GROUP_RID_ADMINS-1);
				return -1;
			}
		}
		else if ( !StrnCaseCmp(argv[i], "unixgroup", strlen("unixgroup")) ) {
			fstrcpy( unixgrp, get_string_param( argv[i] ) );
			if ( !unixgrp[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "ntgroup", strlen("ntgroup")) ) {
			fstrcpy( ntgroup, get_string_param( argv[i] ) );
			if ( !ntgroup[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "sid", strlen("sid")) ) {
			fstrcpy( string_sid, get_string_param( argv[i] ) );
			if ( !string_sid[0] ) {
				d_fprintf(stderr, "must supply a SID\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "comment", strlen("comment")) ) {
			fstrcpy( ntcomment, get_string_param( argv[i] ) );
			if ( !ntcomment[0] ) {
				d_fprintf(stderr, "must supply a comment string\n");
				return -1;
			}				
		}
		else if ( !StrnCaseCmp(argv[i], "type", strlen("type")) )  {
			fstrcpy( type, get_string_param( argv[i] ) );
			switch ( type[0] ) {
				case 'b':
				case 'B':
					sid_type = SID_NAME_WKN_GRP;
					name_type = "wellknown group";
					break;
				case 'd':
				case 'D':
					sid_type = SID_NAME_DOM_GRP;
					name_type = "domain group";
					break;
				case 'l':
				case 'L':
					sid_type = SID_NAME_ALIAS;
					name_type = "alias (local) group";
					break;
				default:
					d_fprintf(stderr, "unknown group type %s\n", type);
					return -1;
			}
		}
		else {
			d_fprintf(stderr, "Bad option: %s\n", argv[i]);
			return -1;
		}
	}

	if ( !unixgrp[0] ) {
		d_printf("Usage: net groupmap add {rid=<int>|sid=<string>} unixgroup=<string> [type=<domain|local|builtin>] [ntgroup=<string>] [comment=<string>]\n");
		return -1;
	}
	
	if ( (gid = nametogid(unixgrp)) == (gid_t)-1 ) {
		d_fprintf(stderr, "Can't lookup UNIX group %s\n", unixgrp);
		return -1;
	}

	{
		GROUP_MAP map;
		if (pdb_getgrgid(&map, gid)) {
			d_printf("Unix group %s already mapped to SID %s\n",
				 unixgrp, sid_string_static(&map.sid));
			return -1;
		}
	}
	
	if ( (rid == 0) && (string_sid[0] == '\0') ) {
		d_printf("No rid or sid specified, choosing a RID\n");
		if (pdb_rid_algorithm()) {
			rid = algorithmic_pdb_gid_to_group_rid(gid);
		} else {
			if (!pdb_new_rid(&rid)) {
				d_printf("Could not get new RID\n");
			}
		}
		d_printf("Got RID %d\n", rid);
	}

	/* append the rid to our own domain/machine SID if we don't have a full SID */
	if ( !string_sid[0] ) {
		sid_copy(&sid, get_global_sam_sid());
		sid_append_rid(&sid, rid);
		sid_to_string(string_sid, &sid);
	}

	if (!ntcomment[0]) {
		switch (sid_type) {
		case SID_NAME_WKN_GRP:
			fstrcpy(ntcomment, "Wellknown Unix group");
			break;
		case SID_NAME_DOM_GRP:
			fstrcpy(ntcomment, "Domain Unix group");
			break;
		case SID_NAME_ALIAS:
			fstrcpy(ntcomment, "Local Unix group");
			break;
		default:
			fstrcpy(ntcomment, "Unix group");
			break;
		}
	}
		
	if (!ntgroup[0] )
		fstrcpy( ntgroup, unixgrp );
		
	
	if (!NT_STATUS_IS_OK(add_initial_entry(gid, string_sid, sid_type, ntgroup, ntcomment))) {
		d_fprintf(stderr, "adding entry for group %s failed!\n", ntgroup);
		return -1;
	}

	d_printf("Successfully added group %s to the mapping db as a %s\n",
		ntgroup, name_type);
	return 0;
}

static int net_groupmap_modify(int argc, const char **argv)
{
	DOM_SID sid;
	GROUP_MAP map;
	fstring ntcomment = "";
	fstring type = "";
	fstring ntgroup = "";
	fstring unixgrp = "";
	fstring sid_string = "";
	enum SID_NAME_USE sid_type = SID_NAME_UNKNOWN;
	int i;
	gid_t gid;

	/* get the options */
	for ( i=0; i<argc; i++ ) {
		if ( !StrnCaseCmp(argv[i], "ntgroup", strlen("ntgroup")) ) {
			fstrcpy( ntgroup, get_string_param( argv[i] ) );
			if ( !ntgroup[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "sid", strlen("sid")) ) {
			fstrcpy( sid_string, get_string_param( argv[i] ) );
			if ( !sid_string[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "comment", strlen("comment")) ) {
			fstrcpy( ntcomment, get_string_param( argv[i] ) );
			if ( !ntcomment[0] ) {
				d_fprintf(stderr, "must supply a comment string\n");
				return -1;
			}				
		}
		else if ( !StrnCaseCmp(argv[i], "unixgroup", strlen("unixgroup")) ) {
			fstrcpy( unixgrp, get_string_param( argv[i] ) );
			if ( !unixgrp[0] ) {
				d_fprintf(stderr, "must supply a group name\n");
				return -1;
			}				
		}
		else if ( !StrnCaseCmp(argv[i], "type", strlen("type")) )  {
			fstrcpy( type, get_string_param( argv[i] ) );
			switch ( type[0] ) {
				case 'd':
				case 'D':
					sid_type = SID_NAME_DOM_GRP;
					break;
				case 'l':
				case 'L':
					sid_type = SID_NAME_ALIAS;
					break;
			}
		}
		else {
			d_fprintf(stderr, "Bad option: %s\n", argv[i]);
			return -1;
		}
	}
	
	if ( !ntgroup[0] && !sid_string[0] ) {
		d_printf("Usage: net groupmap modify {ntgroup=<string>|sid=<SID>} [comment=<string>] [unixgroup=<string>] [type=<domain|local>]\n");
		return -1;
	}

	/* give preference to the SID; if both the ntgroup name and SID
	   are defined, use the SID and assume that the group name could be a 
	   new name */
	   	
	if ( sid_string[0] ) {	
		if (!get_sid_from_input(&sid, sid_string)) {
			return -1;
		}
	}
	else {
		if (!get_sid_from_input(&sid, ntgroup)) {
			return -1;
		}
	}	

	/* Get the current mapping from the database */
	if(!pdb_getgrsid(&map, sid)) {
		d_fprintf(stderr, "Failure to local group SID in the database\n");
		return -1;
	}
	
	/*
	 * Allow changing of group type only between domain and local
	 * We disallow changing Builtin groups !!! (SID problem)
	 */ 
	if (sid_type == SID_NAME_UNKNOWN) {
		d_fprintf(stderr, "Can't map to an unknown group type.\n");
		return -1;
	}

	if (map.sid_name_use == SID_NAME_WKN_GRP) {
		d_fprintf(stderr, "You can only change between domain and local groups.\n");
		return -1;
	}
		
	map.sid_name_use=sid_type;

	/* Change comment if new one */
	if ( ntcomment[0] )
		fstrcpy( map.comment, ntcomment );
		
	if ( ntgroup[0] )
		fstrcpy( map.nt_name, ntgroup );
		
	if ( unixgrp[0] ) {
		gid = nametogid( unixgrp );
		if ( gid == -1 ) {
			d_fprintf(stderr, "Unable to lookup UNIX group %s.  Make sure the group exists.\n",
				unixgrp);
			return -1;
		}
		
		map.gid = gid;
	}

	if ( !NT_STATUS_IS_OK(pdb_update_group_mapping_entry(&map)) ) {
		d_fprintf(stderr, "Could not update group database\n");
		return -1;
	}
	
	d_printf("Updated mapping entry for %s\n", map.nt_name);

	return 0;
}

static int net_groupmap_delete(int argc, const char **argv)
{
	DOM_SID sid;
	fstring ntgroup = "";
	fstring sid_string = "";
	int i;

	/* get the options */
	for ( i=0; i<argc; i++ ) {
		if ( !StrnCaseCmp(argv[i], "ntgroup", strlen("ntgroup")) ) {
			fstrcpy( ntgroup, get_string_param( argv[i] ) );
			if ( !ntgroup[0] ) {
				d_fprintf(stderr, "must supply a name\n");
				return -1;
			}		
		}
		else if ( !StrnCaseCmp(argv[i], "sid", strlen("sid")) ) {
			fstrcpy( sid_string, get_string_param( argv[i] ) );
			if ( !sid_string[0] ) {
				d_fprintf(stderr, "must supply a SID\n");
				return -1;
			}		
		}
		else {
			d_fprintf(stderr, "Bad option: %s\n", argv[i]);
			return -1;
		}
	}
	
	if ( !ntgroup[0] && !sid_string[0]) {
		d_printf("Usage: net groupmap delete {ntgroup=<string>|sid=<SID>}\n");
		return -1;
	}
	
	/* give preference to the SID if we have that */
	
	if ( sid_string[0] )
		fstrcpy( ntgroup, sid_string );
		
	if ( !get_sid_from_input(&sid, ntgroup) ) {
		d_fprintf(stderr, "Unable to resolve group %s to a SID\n", ntgroup);
		return -1;
	}

	if ( !NT_STATUS_IS_OK(pdb_delete_group_mapping_entry(sid)) ) {
		d_fprintf(stderr, "Failed to removing group %s from the mapping db!\n", ntgroup);
		return -1;
	}

	d_printf("Sucessfully removed %s from the mapping db\n", ntgroup);

	return 0;
}

static int net_groupmap_set(int argc, const char **argv)
{
	const char *ntgroup = NULL;
	struct group *grp = NULL;
	GROUP_MAP map;
	BOOL have_map = False;

	if ((argc < 1) || (argc > 2)) {
		d_printf("Usage: net groupmap set \"NT Group\" "
			 "[\"unix group\"] [-C \"comment\"] [-L] [-D]\n");
		return -1;
	}

	if ( opt_localgroup && opt_domaingroup ) {
		d_printf("Can only specify -L or -D, not both\n");
		return -1;
	}

	ntgroup = argv[0];

	if (argc == 2) {
		grp = getgrnam(argv[1]);

		if (grp == NULL) {
			d_fprintf(stderr, "Could not find unix group %s\n", argv[1]);
			return -1;
		}
	}

	have_map = pdb_getgrnam(&map, ntgroup);

	if (!have_map) {
		DOM_SID sid;
		have_map = ( (strncmp(ntgroup, "S-", 2) == 0) &&
			     string_to_sid(&sid, ntgroup) &&
			     pdb_getgrsid(&map, sid) );
	}

	if (!have_map) {

		/* Ok, add it */

		if (grp == NULL) {
			d_fprintf(stderr, "Could not find group mapping for %s\n",
				 ntgroup);
			return -1;
		}

		map.gid = grp->gr_gid;

		if (opt_rid == 0) {
			if ( pdb_rid_algorithm() )
				opt_rid = algorithmic_pdb_gid_to_group_rid(map.gid);
			else {
				if ( !pdb_new_rid((uint32*)&opt_rid) ) {
					d_fprintf( stderr, "Could not allocate new RID\n");
					return -1;
				}
			}
		}

		sid_copy(&map.sid, get_global_sam_sid());
		sid_append_rid(&map.sid, opt_rid);

		map.sid_name_use = SID_NAME_DOM_GRP;
		fstrcpy(map.nt_name, ntgroup);
		fstrcpy(map.comment, "");

		if (!NT_STATUS_IS_OK(pdb_add_group_mapping_entry(&map))) {
			d_fprintf(stderr, "Could not add mapping entry for %s\n",
				 ntgroup);
			return -1;
		}
	}

	/* Now we have a mapping entry, update that stuff */

	if ( opt_localgroup || opt_domaingroup ) {
		if (map.sid_name_use == SID_NAME_WKN_GRP) {
			d_fprintf(stderr, "Can't change type of the BUILTIN group %s\n",
				 map.nt_name);
			return -1;
		}
	}

	if (opt_localgroup)
		map.sid_name_use = SID_NAME_ALIAS;

	if (opt_domaingroup)
		map.sid_name_use = SID_NAME_DOM_GRP;

	/* The case (opt_domaingroup && opt_localgroup) was tested for above */

	if (strlen(opt_comment) > 0)
		fstrcpy(map.comment, opt_comment);

	if (strlen(opt_newntname) > 0)
		fstrcpy(map.nt_name, opt_newntname);

	if (grp != NULL)
		map.gid = grp->gr_gid;

	if (!NT_STATUS_IS_OK(pdb_update_group_mapping_entry(&map))) {
		d_fprintf(stderr, "Could not update group mapping for %s\n", ntgroup);
		return -1;
	}

	return 0;
}

static int net_groupmap_cleanup(int argc, const char **argv)
{
	GROUP_MAP *map = NULL;
	size_t i, entries;

	if (!pdb_enum_group_mapping(NULL, SID_NAME_UNKNOWN, &map, &entries,
				    ENUM_ALL_MAPPED)) {
		d_fprintf(stderr, "Could not list group mappings\n");
		return -1;
	}

	for (i=0; i<entries; i++) {

		if (map[i].gid == -1)
			printf("Group %s is not mapped\n", map[i].nt_name);

		if (!sid_check_is_in_our_domain(&map[i].sid)) {
			printf("Deleting mapping for NT Group %s, sid %s\n",
			       map[i].nt_name,
			       sid_string_static(&map[i].sid));
			pdb_delete_group_mapping_entry(map[i].sid);
		}
	}

	SAFE_FREE(map);

	return 0;
}

static int net_groupmap_addmem(int argc, const char **argv)
{
	DOM_SID alias, member;

	if ( (argc != 2) || 
	     !string_to_sid(&alias, argv[0]) ||
	     !string_to_sid(&member, argv[1]) ) {
		d_printf("Usage: net groupmap addmem alias-sid member-sid\n");
		return -1;
	}

	if (!NT_STATUS_IS_OK(pdb_add_aliasmem(&alias, &member))) {
		d_fprintf(stderr, "Could not add sid %s to alias %s\n",
			 argv[1], argv[0]);
		return -1;
	}

	return 0;
}

static int net_groupmap_delmem(int argc, const char **argv)
{
	DOM_SID alias, member;

	if ( (argc != 2) || 
	     !string_to_sid(&alias, argv[0]) ||
	     !string_to_sid(&member, argv[1]) ) {
		d_printf("Usage: net groupmap delmem alias-sid member-sid\n");
		return -1;
	}

	if (!NT_STATUS_IS_OK(pdb_del_aliasmem(&alias, &member))) {
		d_fprintf(stderr, "Could not delete sid %s from alias %s\n",
			 argv[1], argv[0]);
		return -1;
	}

	return 0;
}

static int net_groupmap_listmem(int argc, const char **argv)
{
	DOM_SID alias;
	DOM_SID *members;
	size_t i, num;

	if ( (argc != 1) || 
	     !string_to_sid(&alias, argv[0]) ) {
		d_printf("Usage: net groupmap listmem alias-sid\n");
		return -1;
	}

	members = NULL;
	num = 0;

	if (!NT_STATUS_IS_OK(pdb_enum_aliasmem(&alias, &members, &num))) {
		d_fprintf(stderr, "Could not list members for sid %s\n", argv[0]);
		return -1;
	}

	for (i = 0; i < num; i++) {
		printf("%s\n", sid_string_static(&(members[i])));
	}

	SAFE_FREE(members);

	return 0;
}

static BOOL print_alias_memberships(TALLOC_CTX *mem_ctx,
				    const DOM_SID *domain_sid,
				    const DOM_SID *member)
{
	uint32 *alias_rids;
	size_t i, num_alias_rids;

	alias_rids = NULL;
	num_alias_rids = 0;

	if (!NT_STATUS_IS_OK(pdb_enum_alias_memberships(
				     mem_ctx, domain_sid, member, 1,
				     &alias_rids, &num_alias_rids))) {
		d_fprintf(stderr, "Could not list memberships for sid %s\n",
			 sid_string_static(member));
		return False;
	}

	for (i = 0; i < num_alias_rids; i++) {
		DOM_SID alias;
		sid_copy(&alias, domain_sid);
		sid_append_rid(&alias, alias_rids[i]);
		printf("%s\n", sid_string_static(&alias));
	}

	return True;
}

static int net_groupmap_memberships(int argc, const char **argv)
{
	TALLOC_CTX *mem_ctx;
	DOM_SID *domain_sid, *builtin_sid, member;

	if ( (argc != 1) || 
	     !string_to_sid(&member, argv[0]) ) {
		d_printf("Usage: net groupmap memberof sid\n");
		return -1;
	}

	mem_ctx = talloc_init("net_groupmap_memberships");
	if (mem_ctx == NULL) {
		d_fprintf(stderr, "talloc_init failed\n");
		return -1;
	}

	domain_sid = get_global_sam_sid();
	builtin_sid = string_sid_talloc(mem_ctx, "S-1-5-32");
	if ((domain_sid == NULL) || (builtin_sid == NULL)) {
		d_fprintf(stderr, "Could not get domain sid\n");
		return -1;
	}

	if (!print_alias_memberships(mem_ctx, domain_sid, &member) ||
	    !print_alias_memberships(mem_ctx, builtin_sid, &member))
		return -1;

	talloc_destroy(mem_ctx);

	return 0;
}

int net_help_groupmap(int argc, const char **argv)
{
	d_printf("net groupmap add"\
		"\n  Create a new group mapping\n");
	d_printf("net groupmap modify"\
		"\n  Update a group mapping\n");
	d_printf("net groupmap delete"\
		"\n  Remove a group mapping\n");
	d_printf("net groupmap addmem"\
		 "\n  Add a foreign alias member\n");
	d_printf("net groupmap delmem"\
		 "\n  Delete a foreign alias member\n");
	d_printf("net groupmap listmem"\
		 "\n  List foreign group members\n");
	d_printf("net groupmap memberships"\
		 "\n  List foreign group memberships\n");
	d_printf("net groupmap list"\
		"\n  List current group map\n");
	d_printf("net groupmap set"\
		"\n  Set group mapping\n");
	d_printf("net groupmap cleanup"\
		"\n  Remove foreign group mapping entries\n");
	
	return -1;
}


/***********************************************************
 migrated functionality from smbgroupedit
 **********************************************************/
int net_groupmap(int argc, const char **argv)
{
	struct functable func[] = {
		{"add", net_groupmap_add},
		{"modify", net_groupmap_modify},
		{"delete", net_groupmap_delete},
		{"set", net_groupmap_set},
		{"cleanup", net_groupmap_cleanup},
 		{"addmem", net_groupmap_addmem},
 		{"delmem", net_groupmap_delmem},
 		{"listmem", net_groupmap_listmem},
 		{"memberships", net_groupmap_memberships},
		{"list", net_groupmap_list},
		{"help", net_help_groupmap},
		{NULL, NULL}
	};

	/* we shouldn't have silly checks like this */
	if (getuid() != 0) {
		d_fprintf(stderr, "You must be root to edit group mappings.\n");
		return -1;
	}
	
	if ( argc )
		return net_run_function(argc, argv, func, net_help_groupmap);

	return net_help_groupmap( argc, argv );
}

