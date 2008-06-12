/*
 *  Unix SMB/CIFS implementation.
 *  Local SAM access routines
 *  Copyright (C) Volker Lendecke 2006
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

/*
 * Set a user's data
 */

static int net_sam_userset(int argc, const char **argv, const char *field,
			   BOOL (*fn)(struct samu *, const char *,
				      enum pdb_value_state))
{
	struct samu *sam_acct = NULL;
	DOM_SID sid;
	enum SID_NAME_USE type;
	const char *dom, *name;
	NTSTATUS status;

	if (argc != 2) {
		d_fprintf(stderr, "usage: net sam set %s <user> <value>\n",
			  field);
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &dom, &name, &sid, &type)) {
		d_fprintf(stderr, "Could not find name %s\n", argv[0]);
		return -1;
	}

	if (type != SID_NAME_USER) {
		d_fprintf(stderr, "%s is a %s, not a user\n", argv[0],
			  sid_type_lookup(type));
		return -1;
	}

	if ( !(sam_acct = samu_new( NULL )) ) {
		d_fprintf(stderr, "Internal error\n");
		return -1;
	}

	if (!pdb_getsampwsid(sam_acct, &sid)) {
		d_fprintf(stderr, "Loading user %s failed\n", argv[0]);
		return -1;
	}

	if (!fn(sam_acct, argv[1], PDB_CHANGED)) {
		d_fprintf(stderr, "Internal error\n");
		return -1;
	}

	status = pdb_update_sam_account(sam_acct);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Updating sam account %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	TALLOC_FREE(sam_acct);

	d_printf("Updated %s for %s\\%s to %s\n", field, dom, name, argv[1]);
	return 0;
}

static int net_sam_set_fullname(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "fullname",
			       pdb_set_fullname);
}

static int net_sam_set_logonscript(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "logonscript",
			       pdb_set_logon_script);
}

static int net_sam_set_profilepath(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "profilepath",
			       pdb_set_profile_path);
}

static int net_sam_set_homedrive(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "homedrive",
			       pdb_set_dir_drive);
}

static int net_sam_set_homedir(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "homedir",
			       pdb_set_homedir);
}

static int net_sam_set_workstations(int argc, const char **argv)
{
	return net_sam_userset(argc, argv, "workstations",
			       pdb_set_workstations);
}

/*
 * Set account flags
 */

static int net_sam_set_userflag(int argc, const char **argv, const char *field,
				uint16 flag)
{
	struct samu *sam_acct = NULL;
	DOM_SID sid;
	enum SID_NAME_USE type;
	const char *dom, *name;
	NTSTATUS status;
	uint16 acct_flags;

	if ((argc != 2) || (!strequal(argv[1], "yes") &&
			    !strequal(argv[1], "no"))) {
		d_fprintf(stderr, "usage: net sam set %s <user> [yes|no]\n",
			  field);
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &dom, &name, &sid, &type)) {
		d_fprintf(stderr, "Could not find name %s\n", argv[0]);
		return -1;
	}

	if (type != SID_NAME_USER) {
		d_fprintf(stderr, "%s is a %s, not a user\n", argv[0],
			  sid_type_lookup(type));
		return -1;
	}

	if ( !(sam_acct = samu_new( NULL )) ) {
		d_fprintf(stderr, "Internal error\n");
		return -1;
	}

	if (!pdb_getsampwsid(sam_acct, &sid)) {
		d_fprintf(stderr, "Loading user %s failed\n", argv[0]);
		return -1;
	}

	acct_flags = pdb_get_acct_ctrl(sam_acct);

	if (strequal(argv[1], "yes")) {
		acct_flags |= flag;
	} else {
		acct_flags &= ~flag;
	}

	pdb_set_acct_ctrl(sam_acct, acct_flags, PDB_CHANGED);

	status = pdb_update_sam_account(sam_acct);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Updating sam account %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	TALLOC_FREE(sam_acct);

	d_fprintf(stderr, "Updated flag %s for %s\\%s to %s\n", field, dom,
		  name, argv[1]);
	return 0;
}

static int net_sam_set_disabled(int argc, const char **argv)
{
	return net_sam_set_userflag(argc, argv, "disabled", ACB_DISABLED);
}

static int net_sam_set_pwnotreq(int argc, const char **argv)
{
	return net_sam_set_userflag(argc, argv, "pwnotreq", ACB_PWNOTREQ);
}

static int net_sam_set_autolock(int argc, const char **argv)
{
	return net_sam_set_userflag(argc, argv, "autolock", ACB_AUTOLOCK);
}

static int net_sam_set_pwnoexp(int argc, const char **argv)
{
	return net_sam_set_userflag(argc, argv, "pwnoexp", ACB_PWNOEXP);
}

/*
 * Set a user's time field
 */

static int net_sam_set_time(int argc, const char **argv, const char *field,
			    BOOL (*fn)(struct samu *, time_t,
				       enum pdb_value_state))
{
	struct samu *sam_acct = NULL;
	DOM_SID sid;
	enum SID_NAME_USE type;
	const char *dom, *name;
	NTSTATUS status;
	time_t new_time;

	if (argc != 2) {
		d_fprintf(stderr, "usage: net sam set %s <user> "
			  "[now|YYYY-MM-DD HH:MM]\n", field);
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &dom, &name, &sid, &type)) {
		d_fprintf(stderr, "Could not find name %s\n", argv[0]);
		return -1;
	}

	if (type != SID_NAME_USER) {
		d_fprintf(stderr, "%s is a %s, not a user\n", argv[0],
			  sid_type_lookup(type));
		return -1;
	}

	if (strequal(argv[1], "now")) {
		new_time = time(NULL);
	} else {
		struct tm tm;
		char *end;
		ZERO_STRUCT(tm);
		end = strptime(argv[1], "%Y-%m-%d %H:%M", &tm);
		new_time = mktime(&tm);
		if ((end == NULL) || (*end != '\0') || (new_time == -1)) {
			d_fprintf(stderr, "Could not parse time string %s\n",
				  argv[1]);
			return -1;
		}
	}


	if ( !(sam_acct = samu_new( NULL )) ) {
		d_fprintf(stderr, "Internal error\n");
		return -1;
	}

	if (!pdb_getsampwsid(sam_acct, &sid)) {
		d_fprintf(stderr, "Loading user %s failed\n", argv[0]);
		return -1;
	}

	if (!fn(sam_acct, new_time, PDB_CHANGED)) {
		d_fprintf(stderr, "Internal error\n");
		return -1;
	}

	status = pdb_update_sam_account(sam_acct);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Updating sam account %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	TALLOC_FREE(sam_acct);

	d_printf("Updated %s for %s\\%s to %s\n", field, dom, name, argv[1]);
	return 0;
}

static int net_sam_set_pwdmustchange(int argc, const char **argv)
{
	return net_sam_set_time(argc, argv, "pwdmustchange",
				pdb_set_pass_must_change_time);
}

static int net_sam_set_pwdcanchange(int argc, const char **argv)
{
	return net_sam_set_time(argc, argv, "pwdcanchange",
				pdb_set_pass_can_change_time);
}

/*
 * Set a user's or a group's comment
 */

static int net_sam_set_comment(int argc, const char **argv)
{
	GROUP_MAP map;
	DOM_SID sid;
	enum SID_NAME_USE type;
	const char *dom, *name;
	NTSTATUS status;

	if (argc != 2) {
		d_fprintf(stderr, "usage: net sam set comment <name> "
			  "<comment>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &dom, &name, &sid, &type)) {
		d_fprintf(stderr, "Could not find name %s\n", argv[0]);
		return -1;
	}

	if (type == SID_NAME_USER) {
		return net_sam_userset(argc, argv, "comment",
				       pdb_set_acct_desc);
	}

	if ((type != SID_NAME_DOM_GRP) && (type != SID_NAME_ALIAS) &&
	    (type != SID_NAME_WKN_GRP)) {
		d_fprintf(stderr, "%s is a %s, not a group\n", argv[0],
			  sid_type_lookup(type));
		return -1;
	}

	if (!pdb_getgrsid(&map, sid)) {
		d_fprintf(stderr, "Could not load group %s\n", argv[0]);
		return -1;
	}

	fstrcpy(map.comment, argv[1]);

	status = pdb_update_group_mapping_entry(&map);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Updating group mapping entry failed with "
			  "%s\n", nt_errstr(status));
		return -1;
	}

	d_printf("Updated comment of group %s\\%s to %s\n", dom, name,
		 argv[1]);

	return 0;
}

static int net_sam_set(int argc, const char **argv)
{
	struct functable2 func[] = {
		{ "homedir", net_sam_set_homedir,
		  "Change a user's home directory" },
		{ "profilepath", net_sam_set_profilepath,
		  "Change a user's profile path" },
		{ "comment", net_sam_set_comment,
		  "Change a users or groups description" },
		{ "fullname", net_sam_set_fullname,
		  "Change a user's full name" },
		{ "logonscript", net_sam_set_logonscript,
		  "Change a user's logon script" },
		{ "homedrive", net_sam_set_homedrive,
		  "Change a user's home drive" },
		{ "workstations", net_sam_set_workstations,
		  "Change a user's allowed workstations" },
		{ "disabled", net_sam_set_disabled,
		  "Disable/Enable a user" },
		{ "pwnotreq", net_sam_set_pwnotreq,
		  "Disable/Enable the password not required flag" },
		{ "autolock", net_sam_set_autolock,
		  "Disable/Enable a user's lockout flag" },
		{ "pwnoexp", net_sam_set_pwnoexp,
		  "Disable/Enable whether a user's pw does not expire" },
		{ "pwdmustchange", net_sam_set_pwdmustchange,
		  "Set a users password must change time" },
		{ "pwdcanchange", net_sam_set_pwdcanchange,
		  "Set a users password can change time" },
		{NULL, NULL}
	};

	return net_run_function2(argc, argv, "net sam set", func);
}

/*
 * Map a unix group to a domain group
 */

static int net_sam_mapunixgroup(int argc, const char **argv)
{
	NTSTATUS status;
	GROUP_MAP map;
	struct group *grp;

	if (argc != 1) {
		d_fprintf(stderr, "usage: net sam mapunixgroup <name>\n");
		return -1;
	}

	grp = getgrnam(argv[0]);
	if (grp == NULL) {
		d_fprintf(stderr, "Could not find group %s\n", argv[0]);
		return -1;
	}

	status = map_unix_group(grp, &map);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Mapping group %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	d_printf("Mapped unix group %s to SID %s\n", argv[0],
		 sid_string_static(&map.sid));

	return 0;
}

/*
 * Create a local group
 */

static int net_sam_createlocalgroup(int argc, const char **argv)
{
	NTSTATUS status;
	uint32 rid;

	if (argc != 1) {
		d_fprintf(stderr, "usage: net sam createlocalgroup <name>\n");
		return -1;
	}

	if (!winbind_ping()) {
		d_fprintf(stderr, "winbind seems not to run. createlocalgroup "
			  "only works when winbind runs.\n");
		return -1;
	}

	status = pdb_create_alias(argv[0], &rid);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Creating %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	d_printf("Created local group %s with RID %d\n", argv[0], rid);

	return 0;
}

/*
 * Create a local group
 */

static int net_sam_createbuiltingroup(int argc, const char **argv)
{
	NTSTATUS status;
	uint32 rid;
	enum SID_NAME_USE type;
	fstring groupname;
	DOM_SID sid;

	if (argc != 1) {
		d_fprintf(stderr, "usage: net sam createbuiltingroup <name>\n");
		return -1;
	}

	if (!winbind_ping()) {
		d_fprintf(stderr, "winbind seems not to run. createlocalgroup "
			  "only works when winbind runs.\n");
		return -1;
	}

	/* validate the name and get the group */
	
	fstrcpy( groupname, "BUILTIN\\" );
	fstrcat( groupname, argv[0] );
	
	if ( !lookup_name(tmp_talloc_ctx(), groupname, LOOKUP_NAME_ALL, NULL,
			  NULL, &sid, &type)) {
		d_fprintf(stderr, "%s is not a BUILTIN group\n", argv[0]);
		return -1;
	}
	
	if ( !sid_peek_rid( &sid, &rid ) ) {
		d_fprintf(stderr, "Failed to get RID for %s\n", argv[0]);
		return -1;
	}

	status = pdb_create_builtin_alias( rid );

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "Creating %s failed with %s\n",
			  argv[0], nt_errstr(status));
		return -1;
	}

	d_printf("Created BUILTIN group %s with RID %d\n", argv[0], rid);

	return 0;
}

/*
 * Add a group member
 */

static int net_sam_addmem(int argc, const char **argv)
{
	const char *groupdomain, *groupname, *memberdomain, *membername;
	DOM_SID group, member;
	enum SID_NAME_USE grouptype, membertype;
	NTSTATUS status;

	if (argc != 2) {
		d_fprintf(stderr, "usage: net sam addmem <group> <member>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &groupdomain, &groupname, &group, &grouptype)) {
		d_fprintf(stderr, "Could not find group %s\n", argv[0]);
		return -1;
	}

	/* check to see if the member to be added is a name or a SID */

	if (!lookup_name(tmp_talloc_ctx(), argv[1], LOOKUP_NAME_ISOLATED,
			 &memberdomain, &membername, &member, &membertype))
	{
		/* try it as a SID */

		if ( !string_to_sid( &member, argv[1] ) ) {
			d_fprintf(stderr, "Could not find member %s\n", argv[1]);
			return -1;
		}

		if ( !lookup_sid(tmp_talloc_ctx(), &member, &memberdomain, 
			&membername, &membertype) ) 
		{
			d_fprintf(stderr, "Could not resolve SID %s\n", argv[1]);
			return -1;
		}
	}

	if ((grouptype == SID_NAME_ALIAS) || (grouptype == SID_NAME_WKN_GRP)) {
		if ((membertype != SID_NAME_USER) &&
		    (membertype != SID_NAME_DOM_GRP)) {
			d_fprintf(stderr, "%s is a local group, only users "
				  "and domain groups can be added.\n"
				  "%s is a %s\n", argv[0], argv[1],
				  sid_type_lookup(membertype));
			return -1;
		}
		status = pdb_add_aliasmem(&group, &member);

		if (!NT_STATUS_IS_OK(status)) {
			d_fprintf(stderr, "Adding local group member failed "
				  "with %s\n", nt_errstr(status));
			return -1;
		}
	} else {
		d_fprintf(stderr, "Can only add members to local groups so "
			  "far, %s is a %s\n", argv[0],
			  sid_type_lookup(grouptype));
		return -1;
	}

	d_printf("Added %s\\%s to %s\\%s\n", memberdomain, membername, 
		groupdomain, groupname);

	return 0;
}

/*
 * Delete a group member
 */

static int net_sam_delmem(int argc, const char **argv)
{
	const char *groupdomain, *groupname;
	const char *memberdomain = NULL;
	const char *membername = NULL;
	DOM_SID group, member;
	enum SID_NAME_USE grouptype;
	NTSTATUS status;

	if (argc != 2) {
		d_fprintf(stderr, "usage: net sam delmem <group> <member>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &groupdomain, &groupname, &group, &grouptype)) {
		d_fprintf(stderr, "Could not find group %s\n", argv[0]);
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[1], LOOKUP_NAME_ISOLATED,
			 &memberdomain, &membername, &member, NULL)) {
		if (!string_to_sid(&member, argv[1])) {
			d_fprintf(stderr, "Could not find member %s\n",
				  argv[1]);
			return -1;
		}
	}

	if ((grouptype == SID_NAME_ALIAS) ||
	    (grouptype == SID_NAME_WKN_GRP)) {
		status = pdb_del_aliasmem(&group, &member);

		if (!NT_STATUS_IS_OK(status)) {
			d_fprintf(stderr, "Deleting local group member failed "
				  "with %s\n", nt_errstr(status));
			return -1;
		}
	} else {
		d_fprintf(stderr, "Can only delete members from local groups "
			  "so far, %s is a %s\n", argv[0],
			  sid_type_lookup(grouptype));
		return -1;
	}

	if (membername != NULL) {
		d_printf("Deleted %s\\%s from %s\\%s\n",
			 memberdomain, membername, groupdomain, groupname);
	} else {
		d_printf("Deleted %s from %s\\%s\n",
			 sid_string_static(&member), groupdomain, groupname);
	}

	return 0;
}

/*
 * List group members
 */

static int net_sam_listmem(int argc, const char **argv)
{
	const char *groupdomain, *groupname;
	DOM_SID group;
	enum SID_NAME_USE grouptype;
	NTSTATUS status;

	if (argc != 1) {
		d_fprintf(stderr, "usage: net sam listmem <group>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &groupdomain, &groupname, &group, &grouptype)) {
		d_fprintf(stderr, "Could not find group %s\n", argv[0]);
		return -1;
	}

	if ((grouptype == SID_NAME_ALIAS) ||
	    (grouptype == SID_NAME_WKN_GRP)) {
		DOM_SID *members = NULL;
		size_t i, num_members = 0;
		
		status = pdb_enum_aliasmem(&group, &members, &num_members);

		if (!NT_STATUS_IS_OK(status)) {
			d_fprintf(stderr, "Listing group members failed with "
				  "%s\n", nt_errstr(status));
			return -1;
		}

		d_printf("%s\\%s has %u members\n", groupdomain, groupname,
			 (unsigned int)num_members);
		for (i=0; i<num_members; i++) {
			const char *dom, *name;
			if (lookup_sid(tmp_talloc_ctx(), &members[i],
				       &dom, &name, NULL)) {
				d_printf(" %s\\%s\n", dom, name);
			} else {
				d_printf(" %s\n",
					 sid_string_static(&members[i]));
			}
		}
	} else {
		d_fprintf(stderr, "Can only list local group members so far.\n"
			  "%s is a %s\n", argv[0], sid_type_lookup(grouptype));
		return -1;
	}

	return 0;
}

/*
 * Do the listing
 */
static int net_sam_do_list(int argc, const char **argv,
			   struct pdb_search *search, const char *what)
{
	BOOL verbose = (argc == 1);

	if ((argc > 1) ||
	    ((argc == 1) && !strequal(argv[0], "verbose"))) {
		d_fprintf(stderr, "usage: net sam list %s [verbose]\n", what);
		return -1;
	}

	if (search == NULL) {
		d_fprintf(stderr, "Could not start search\n");
		return -1;
	}

	while (True) {
		struct samr_displayentry entry;
		if (!search->next_entry(search, &entry)) {
			break;
		}
		if (verbose) {
			d_printf("%s:%d:%s\n",
				 entry.account_name,
				 entry.rid,
				 entry.description);
		} else {
			d_printf("%s\n", entry.account_name);
		}
	}

	search->search_end(search);
	return 0;
}

static int net_sam_list_users(int argc, const char **argv)
{
	return net_sam_do_list(argc, argv, pdb_search_users(ACB_NORMAL),
			       "users");
}

static int net_sam_list_groups(int argc, const char **argv)
{
	return net_sam_do_list(argc, argv, pdb_search_groups(), "groups");
}

static int net_sam_list_localgroups(int argc, const char **argv)
{
	return net_sam_do_list(argc, argv,
			       pdb_search_aliases(get_global_sam_sid()),
			       "localgroups");
}

static int net_sam_list_builtin(int argc, const char **argv)
{
	return net_sam_do_list(argc, argv,
			       pdb_search_aliases(&global_sid_Builtin),
			       "builtin");
}

static int net_sam_list_workstations(int argc, const char **argv)
{
	return net_sam_do_list(argc, argv,
			       pdb_search_users(ACB_WSTRUST),
			       "workstations");
}

/*
 * List stuff
 */

static int net_sam_list(int argc, const char **argv)
{
	struct functable2 func[] = {
		{ "users", net_sam_list_users,
		  "List SAM users" },
		{ "groups", net_sam_list_groups,
		  "List SAM groups" },
		{ "localgroups", net_sam_list_localgroups,
		  "List SAM local groups" },
		{ "builtin", net_sam_list_builtin,
		  "List builtin groups" },
		{ "workstations", net_sam_list_workstations,
		  "List domain member workstations" },
		{NULL, NULL}
	};

	return net_run_function2(argc, argv, "net sam list", func);
}

/*
 * Show details of SAM entries
 */

static int net_sam_show(int argc, const char **argv)
{
	DOM_SID sid;
	enum SID_NAME_USE type;
	const char *dom, *name;

	if (argc != 1) {
		d_fprintf(stderr, "usage: net sam show <name>\n");
		return -1;
	}

	if (!lookup_name(tmp_talloc_ctx(), argv[0], LOOKUP_NAME_ISOLATED,
			 &dom, &name, &sid, &type)) {
		d_fprintf(stderr, "Could not find name %s\n", argv[0]);
		return -1;
	}

	d_printf("%s\\%s is a %s with SID %s\n", dom, name,
		 sid_type_lookup(type), sid_string_static(&sid));

	return 0;
}

#ifdef HAVE_LDAP

/*
 * Init an LDAP tree with default users and Groups
 * if ldapsam:editposix is enabled
 */

static int net_sam_provision(int argc, const char **argv)
{
	TALLOC_CTX *tc;
	char *ldap_bk;
	char *ldap_uri = NULL;
	char *p;
	struct smbldap_state *ls;
	GROUP_MAP gmap;
	DOM_SID gsid;
	gid_t domusers_gid = -1;
	gid_t domadmins_gid = -1;
	struct samu *samuser;
	struct passwd *pwd;

	tc = talloc_new(NULL);
	if (!tc) {
		d_fprintf(stderr, "Out of Memory!\n");
		return -1;
	}

	if ((ldap_bk = talloc_strdup(tc, lp_passdb_backend())) == NULL) {
		d_fprintf(stderr, "talloc failed\n");
		talloc_free(tc);
		return -1;
	}
	p = strchr(ldap_bk, ':');
	if (p) {
		*p = 0;
		ldap_uri = talloc_strdup(tc, p+1);
		trim_char(ldap_uri, ' ', ' ');
	}

	trim_char(ldap_bk, ' ', ' ');
	        
	if (strcmp(ldap_bk, "ldapsam") != 0) {
		d_fprintf(stderr, "Provisioning works only with ldapsam backend\n");
		goto failed;
	}
	
	if (!lp_parm_bool(-1, "ldapsam", "trusted", False) ||
	    !lp_parm_bool(-1, "ldapsam", "editposix", False)) {

		d_fprintf(stderr, "Provisioning works only if ldapsam:trusted"
				  " and ldapsam:editposix are enabled.\n");
		goto failed;
	}

	if (!winbind_ping()) {
		d_fprintf(stderr, "winbind seems not to run. Provisioning "
			  "LDAP only works when winbind runs.\n");
		goto failed;
	}

	if (!NT_STATUS_IS_OK(smbldap_init(tc, ldap_uri, &ls))) {
		d_fprintf(stderr, "Unable to connect to the LDAP server.\n");
		goto failed;
	}

	d_printf("Checking for Domain Users group.\n");

	sid_compose(&gsid, get_global_sam_sid(), DOMAIN_GROUP_RID_USERS);

	if (!pdb_getgrsid(&gmap, gsid)) {
		LDAPMod **mods = NULL;
		char *dn;
		char *uname;
		char *wname;
		char *gidstr;
		char *gtype;
		int rc;

		d_printf("Adding the Domain Users group.\n");

		/* lets allocate a new groupid for this group */
		if (!winbind_allocate_gid(&domusers_gid)) {
			d_fprintf(stderr, "Unable to allocate a new gid to create Domain Users group!\n");
			goto domu_done;
		}

		uname = talloc_strdup(tc, "domusers");
		wname = talloc_strdup(tc, "Domain Users");
		dn = talloc_asprintf(tc, "cn=%s,%s", "domusers", lp_ldap_group_suffix());
		gidstr = talloc_asprintf(tc, "%d", domusers_gid);
		gtype = talloc_asprintf(tc, "%d", SID_NAME_DOM_GRP);

		if (!uname || !wname || !dn || !gidstr || !gtype) {
			d_fprintf(stderr, "Out of Memory!\n");
			goto failed;
		}

		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectclass", LDAP_OBJ_POSIXGROUP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_GROUPMAP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "cn", uname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "displayName", wname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "gidNumber", gidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaSid", sid_string_static(&gsid));
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaGroupType", gtype);

		talloc_autofree_ldapmod(tc, mods);

		rc = smbldap_add(ls, dn, mods);

		if (rc != LDAP_SUCCESS) {
			d_fprintf(stderr, "Failed to add Domain Users group to ldap directory\n");
		}
	} else {
		d_printf("found!\n");
	}	

domu_done:

	d_printf("Checking for Domain Admins group.\n");

	sid_compose(&gsid, get_global_sam_sid(), DOMAIN_GROUP_RID_ADMINS);

	if (!pdb_getgrsid(&gmap, gsid)) {
		LDAPMod **mods = NULL;
		char *dn;
		char *uname;
		char *wname;
		char *gidstr;
		char *gtype;
		int rc;

		d_printf("Adding the Domain Admins group.\n");

		/* lets allocate a new groupid for this group */
		if (!winbind_allocate_gid(&domadmins_gid)) {
			d_fprintf(stderr, "Unable to allocate a new gid to create Domain Admins group!\n");
			goto doma_done;
		}

		uname = talloc_strdup(tc, "domadmins");
		wname = talloc_strdup(tc, "Domain Admins");
		dn = talloc_asprintf(tc, "cn=%s,%s", "domadmins", lp_ldap_group_suffix());
		gidstr = talloc_asprintf(tc, "%d", domadmins_gid);
		gtype = talloc_asprintf(tc, "%d", SID_NAME_DOM_GRP);

		if (!uname || !wname || !dn || !gidstr || !gtype) {
			d_fprintf(stderr, "Out of Memory!\n");
			goto failed;
		}

		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectclass", LDAP_OBJ_POSIXGROUP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_GROUPMAP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "cn", uname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "displayName", wname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "gidNumber", gidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaSid", sid_string_static(&gsid));
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaGroupType", gtype);

		talloc_autofree_ldapmod(tc, mods);

		rc = smbldap_add(ls, dn, mods);

		if (rc != LDAP_SUCCESS) {
			d_fprintf(stderr, "Failed to add Domain Admins group to ldap directory\n");
		}
	} else {
		d_printf("found!\n");
	}

doma_done:

	d_printf("Check for Administrator account.\n");

	samuser = samu_new(tc);
	if (!samuser) {
		d_fprintf(stderr, "Out of Memory!\n");
		goto failed;
	}

	if (!pdb_getsampwnam(samuser, "Administrator")) {
		LDAPMod **mods = NULL;
		DOM_SID sid;
		char *dn;
		char *name;
		char *uidstr;
		char *gidstr;
		char *shell;
		char *dir;
		uid_t uid;
		int rc;
		
		d_printf("Adding the Administrator user.\n");

		if (domadmins_gid == -1) {
			d_fprintf(stderr, "Can't create Administrtor user, Domain Admins group not available!\n");
			goto done;
		}
		if (!winbind_allocate_uid(&uid)) {
			d_fprintf(stderr, "Unable to allocate a new uid to create the Administrator user!\n");
			goto done;
		}
		name = talloc_strdup(tc, "Administrator");
		dn = talloc_asprintf(tc, "uid=Administrator,%s", lp_ldap_user_suffix());
		uidstr = talloc_asprintf(tc, "%d", uid);
		gidstr = talloc_asprintf(tc, "%d", domadmins_gid);
		dir = talloc_sub_specified(tc, lp_template_homedir(),
						"Administrator",
						get_global_sam_name(),
						uid, domadmins_gid);
		shell = talloc_sub_specified(tc, lp_template_shell(),
						"Administrator",
						get_global_sam_name(),
						uid, domadmins_gid);

		if (!name || !dn || !uidstr || !gidstr || !dir || !shell) {
			d_fprintf(stderr, "Out of Memory!\n");
			goto failed;
		}

		sid_compose(&sid, get_global_sam_sid(), DOMAIN_USER_RID_ADMIN);

		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_ACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_POSIXACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_SAMBASAMACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "uid", name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "cn", name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "displayName", name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "uidNumber", uidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "gidNumber", gidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "homeDirectory", dir);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "loginShell", shell);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaSID", sid_string_static(&sid));
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaAcctFlags",
				pdb_encode_acct_ctrl(ACB_NORMAL|ACB_DISABLED,
				NEW_PW_FORMAT_SPACE_PADDED_LEN));

		talloc_autofree_ldapmod(tc, mods);

		rc = smbldap_add(ls, dn, mods);

		if (rc != LDAP_SUCCESS) {
			d_fprintf(stderr, "Failed to add Administrator user to ldap directory\n");
		}
	} else {
		d_printf("found!\n");
	}

	d_printf("Checking for Guest user.\n");

	samuser = samu_new(tc);
	if (!samuser) {
		d_fprintf(stderr, "Out of Memory!\n");
		goto failed;
	}

	if (!pdb_getsampwnam(samuser, lp_guestaccount())) {
		LDAPMod **mods = NULL;
		DOM_SID sid;
		char *dn;
		char *uidstr;
		char *gidstr;
		int rc;
		
		d_printf("Adding the Guest user.\n");

		pwd = getpwnam_alloc(tc, lp_guestaccount());

		if (!pwd) {
			if (domusers_gid == -1) {
				d_fprintf(stderr, "Can't create Guest user, Domain Users group not available!\n");
				goto done;
			}
			if ((pwd = talloc(tc, struct passwd)) == NULL) {
				d_fprintf(stderr, "talloc failed\n");
				goto done;
			}
			pwd->pw_name = talloc_strdup(pwd, lp_guestaccount());
			if (!winbind_allocate_uid(&(pwd->pw_uid))) {
				d_fprintf(stderr, "Unable to allocate a new uid to create the Guest user!\n");
				goto done;
			}
			pwd->pw_gid = domusers_gid;
			pwd->pw_dir = talloc_strdup(tc, "/");
			pwd->pw_shell = talloc_strdup(tc, "/bin/false");
			if (!pwd->pw_dir || !pwd->pw_shell) {
				d_fprintf(stderr, "Out of Memory!\n");
				goto failed;
			}
		}

		sid_compose(&sid, get_global_sam_sid(), DOMAIN_USER_RID_GUEST);

		dn = talloc_asprintf(tc, "uid=%s,%s", pwd->pw_name, lp_ldap_user_suffix ());
		uidstr = talloc_asprintf(tc, "%d", pwd->pw_uid);
		gidstr = talloc_asprintf(tc, "%d", pwd->pw_gid);
		if (!dn || !uidstr || !gidstr) {
			d_fprintf(stderr, "Out of Memory!\n");
			goto failed;
		}

		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_ACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_POSIXACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_SAMBASAMACCOUNT);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "uid", pwd->pw_name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "cn", pwd->pw_name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "displayName", pwd->pw_name);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "uidNumber", uidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "gidNumber", gidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "homeDirectory", pwd->pw_dir);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "loginShell", pwd->pw_shell);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaSID", sid_string_static(&sid));
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaAcctFlags",
				pdb_encode_acct_ctrl(ACB_NORMAL|ACB_DISABLED,
				NEW_PW_FORMAT_SPACE_PADDED_LEN));

		talloc_autofree_ldapmod(tc, mods);

		rc = smbldap_add(ls, dn, mods);

		if (rc != LDAP_SUCCESS) {
			d_fprintf(stderr, "Failed to add Guest user to ldap directory\n");
		}
	} else {
		d_printf("found!\n");
	}

	d_printf("Checking Guest's group.\n");

	pwd = getpwnam_alloc(NULL, lp_guestaccount());
	if (!pwd) {
		d_fprintf(stderr, "Failed to find just created Guest account!\n"
				  "   Is nssswitch properly configured?!\n");
		goto failed;
	}

	if (pwd->pw_gid == domusers_gid) {
		d_printf("found!\n");
		goto done;
	}

	if (!pdb_getgrgid(&gmap, pwd->pw_gid)) {
		LDAPMod **mods = NULL;
		char *dn;
		char *uname;
		char *wname;
		char *gidstr;
		char *gtype;
		int rc;

		d_printf("Adding the Domain Guests group.\n");

		uname = talloc_strdup(tc, "domguests");
		wname = talloc_strdup(tc, "Domain Guests");
		dn = talloc_asprintf(tc, "cn=%s,%s", "domguests", lp_ldap_group_suffix());
		gidstr = talloc_asprintf(tc, "%d", pwd->pw_gid);
		gtype = talloc_asprintf(tc, "%d", SID_NAME_DOM_GRP);

		if (!uname || !wname || !dn || !gidstr || !gtype) {
			d_fprintf(stderr, "Out of Memory!\n");
			goto failed;
		}

		sid_compose(&gsid, get_global_sam_sid(), DOMAIN_GROUP_RID_GUESTS);

		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectclass", LDAP_OBJ_POSIXGROUP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "objectClass", LDAP_OBJ_GROUPMAP);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "cn", uname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "displayName", wname);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "gidNumber", gidstr);
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaSid", sid_string_static(&gsid));
		smbldap_set_mod(&mods, LDAP_MOD_ADD, "sambaGroupType", gtype);

		talloc_autofree_ldapmod(tc, mods);

		rc = smbldap_add(ls, dn, mods);

		if (rc != LDAP_SUCCESS) {
			d_fprintf(stderr, "Failed to add Domain Guests group to ldap directory\n");
		}
	} else {
		d_printf("found!\n");
	}


done:
	talloc_free(tc);
	return 0;

failed:
	talloc_free(tc);
	return -1;
}

#endif

/***********************************************************
 migrated functionality from smbgroupedit
 **********************************************************/
int net_sam(int argc, const char **argv)
{
	struct functable2 func[] = {
		{ "createbuiltingroup", net_sam_createbuiltingroup,
		  "Create a new BUILTIN group" },
		{ "createlocalgroup", net_sam_createlocalgroup,
		  "Create a new local group" },
		{ "mapunixgroup", net_sam_mapunixgroup,
		  "Map a unix group to a domain group" },
		{ "addmem", net_sam_addmem,
		  "Add a member to a group" },
		{ "delmem", net_sam_delmem,
		  "Delete a member from a group" },
		{ "listmem", net_sam_listmem,
		  "List group members" },
		{ "list", net_sam_list,
		  "List users, groups and local groups" },
		{ "show", net_sam_show,
		  "Show details of a SAM entry" },
		{ "set", net_sam_set,
		  "Set details of a SAM account" },
#ifdef HAVE_LDAP
		{ "provision", net_sam_provision,
		  "Provision a clean User Database" },
#endif
		{ NULL, NULL, NULL }
	};

	/* we shouldn't have silly checks like this */
	if (getuid() != 0) {
		d_fprintf(stderr, "You must be root to edit the SAM "
			  "directly.\n");
		return -1;
	}
	
	return net_run_function2(argc, argv, "net sam", func);
}

