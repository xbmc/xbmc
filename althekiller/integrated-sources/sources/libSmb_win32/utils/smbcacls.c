/* 
   Unix SMB/CIFS implementation.
   ACL get/set utility
   
   Copyright (C) Andrew Tridgell 2000
   Copyright (C) Tim Potter      2000
   Copyright (C) Jeremy Allison  2000
   Copyright (C) Jelmer Vernooij 2003
   
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

static pstring owner_username;
static fstring server;
static int test_args = False;
static TALLOC_CTX *ctx;

#define CREATE_ACCESS_READ READ_CONTROL_ACCESS

/* numeric is set when the user wants numeric SIDs and ACEs rather
   than going via LSA calls to resolve them */
static BOOL numeric = False;

enum acl_mode {SMB_ACL_SET, SMB_ACL_DELETE, SMB_ACL_MODIFY, SMB_ACL_ADD };
enum chown_mode {REQUEST_NONE, REQUEST_CHOWN, REQUEST_CHGRP};
enum exit_values {EXIT_OK, EXIT_FAILED, EXIT_PARSE_ERROR};

struct perm_value {
	const char *perm;
	uint32 mask;
};

/* These values discovered by inspection */

static const struct perm_value special_values[] = {
	{ "R", 0x00120089 },
	{ "W", 0x00120116 },
	{ "X", 0x001200a0 },
	{ "D", 0x00010000 },
	{ "P", 0x00040000 },
	{ "O", 0x00080000 },
	{ NULL, 0 },
};

static const struct perm_value standard_values[] = {
	{ "READ",   0x001200a9 },
	{ "CHANGE", 0x001301bf },
	{ "FULL",   0x001f01ff },
	{ NULL, 0 },
};

static struct cli_state *global_hack_cli;
static struct rpc_pipe_client *global_pipe_hnd;
static POLICY_HND pol;
static BOOL got_policy_hnd;

static struct cli_state *connect_one(const char *share);

/* Open cli connection and policy handle */

static BOOL cacls_open_policy_hnd(void)
{
	/* Initialise cli LSA connection */

	if (!global_hack_cli) {
		NTSTATUS ret;
		global_hack_cli = connect_one("IPC$");
		global_pipe_hnd = cli_rpc_pipe_open_noauth(global_hack_cli, PI_LSARPC, &ret);
		if (!global_pipe_hnd) {
				return False;
		}
	}
	
	/* Open policy handle */

	if (!got_policy_hnd) {

		/* Some systems don't support SEC_RIGHTS_MAXIMUM_ALLOWED,
		   but NT sends 0x2000000 so we might as well do it too. */

		if (!NT_STATUS_IS_OK(rpccli_lsa_open_policy(global_pipe_hnd, global_hack_cli->mem_ctx, True, 
							 GENERIC_EXECUTE_ACCESS, &pol))) {
			return False;
		}

		got_policy_hnd = True;
	}
	
	return True;
}

/* convert a SID to a string, either numeric or username/group */
static void SidToString(fstring str, DOM_SID *sid)
{
	char **domains = NULL;
	char **names = NULL;
	uint32 *types = NULL;

	sid_to_string(str, sid);

	if (numeric) return;

	/* Ask LSA to convert the sid to a name */

	if (!cacls_open_policy_hnd() ||
	    !NT_STATUS_IS_OK(rpccli_lsa_lookup_sids(global_pipe_hnd, global_hack_cli->mem_ctx,  
						 &pol, 1, sid, &domains, 
						 &names, &types)) ||
	    !domains || !domains[0] || !names || !names[0]) {
		return;
	}

	/* Converted OK */

	slprintf(str, sizeof(fstring) - 1, "%s%s%s",
		 domains[0], lp_winbind_separator(),
		 names[0]);
	
}

/* convert a string to a SID, either numeric or username/group */
static BOOL StringToSid(DOM_SID *sid, const char *str)
{
	uint32 *types = NULL;
	DOM_SID *sids = NULL;
	BOOL result = True;

	if (strncmp(str, "S-", 2) == 0) {
		return string_to_sid(sid, str);
	}

	if (!cacls_open_policy_hnd() ||
	    !NT_STATUS_IS_OK(rpccli_lsa_lookup_names(global_pipe_hnd, global_hack_cli->mem_ctx, 
						  &pol, 1, &str, NULL, &sids, 
						  &types))) {
		result = False;
		goto done;
	}

	sid_copy(sid, &sids[0]);
 done:

	return result;
}


/* print an ACE on a FILE, using either numeric or ascii representation */
static void print_ace(FILE *f, SEC_ACE *ace)
{
	const struct perm_value *v;
	fstring sidstr;
	int do_print = 0;
	uint32 got_mask;

	SidToString(sidstr, &ace->trustee);

	fprintf(f, "%s:", sidstr);

	if (numeric) {
		fprintf(f, "%d/%d/0x%08x", 
			ace->type, ace->flags, ace->info.mask);
		return;
	}

	/* Ace type */

	if (ace->type == SEC_ACE_TYPE_ACCESS_ALLOWED) {
		fprintf(f, "ALLOWED");
	} else if (ace->type == SEC_ACE_TYPE_ACCESS_DENIED) {
		fprintf(f, "DENIED");
	} else {
		fprintf(f, "%d", ace->type);
	}

	/* Not sure what flags can be set in a file ACL */

	fprintf(f, "/%d/", ace->flags);

	/* Standard permissions */

	for (v = standard_values; v->perm; v++) {
		if (ace->info.mask == v->mask) {
			fprintf(f, "%s", v->perm);
			return;
		}
	}

	/* Special permissions.  Print out a hex value if we have
	   leftover bits in the mask. */

	got_mask = ace->info.mask;

 again:
	for (v = special_values; v->perm; v++) {
		if ((ace->info.mask & v->mask) == v->mask) {
			if (do_print) {
				fprintf(f, "%s", v->perm);
			}
			got_mask &= ~v->mask;
		}
	}

	if (!do_print) {
		if (got_mask != 0) {
			fprintf(f, "0x%08x", ace->info.mask);
		} else {
			do_print = 1;
			goto again;
		}
	}
}


/* parse an ACE in the same format as print_ace() */
static BOOL parse_ace(SEC_ACE *ace, const char *orig_str)
{
	char *p;
	const char *cp;
	fstring tok;
	unsigned int atype = 0;
	unsigned int aflags = 0;
	unsigned int amask = 0;
	DOM_SID sid;
	SEC_ACCESS mask;
	const struct perm_value *v;
	char *str = SMB_STRDUP(orig_str);

	if (!str) {
		return False;
	}

	ZERO_STRUCTP(ace);
	p = strchr_m(str,':');
	if (!p) {
		printf("ACE '%s': missing ':'.\n", orig_str);
		SAFE_FREE(str);
		return False;
	}
	*p = '\0';
	p++;
	/* Try to parse numeric form */

	if (sscanf(p, "%i/%i/%i", &atype, &aflags, &amask) == 3 &&
	    StringToSid(&sid, str)) {
		goto done;
	}

	/* Try to parse text form */

	if (!StringToSid(&sid, str)) {
		printf("ACE '%s': failed to convert '%s' to SID\n",
			orig_str, str);
		SAFE_FREE(str);
		return False;
	}

	cp = p;
	if (!next_token(&cp, tok, "/", sizeof(fstring))) {
		printf("ACE '%s': failed to find '/' character.\n",
			orig_str);
		SAFE_FREE(str);
		return False;
	}

	if (strncmp(tok, "ALLOWED", strlen("ALLOWED")) == 0) {
		atype = SEC_ACE_TYPE_ACCESS_ALLOWED;
	} else if (strncmp(tok, "DENIED", strlen("DENIED")) == 0) {
		atype = SEC_ACE_TYPE_ACCESS_DENIED;
	} else {
		printf("ACE '%s': missing 'ALLOWED' or 'DENIED' entry at '%s'\n",
			orig_str, tok);
		SAFE_FREE(str);
		return False;
	}

	/* Only numeric form accepted for flags at present */

	if (!(next_token(&cp, tok, "/", sizeof(fstring)) &&
	      sscanf(tok, "%i", &aflags))) {
		printf("ACE '%s': bad integer flags entry at '%s'\n",
			orig_str, tok);
		SAFE_FREE(str);
		return False;
	}

	if (!next_token(&cp, tok, "/", sizeof(fstring))) {
		printf("ACE '%s': missing / at '%s'\n",
			orig_str, tok);
		SAFE_FREE(str);
		return False;
	}

	if (strncmp(tok, "0x", 2) == 0) {
		if (sscanf(tok, "%i", &amask) != 1) {
			printf("ACE '%s': bad hex number at '%s'\n",
				orig_str, tok);
			SAFE_FREE(str);
			return False;
		}
		goto done;
	}

	for (v = standard_values; v->perm; v++) {
		if (strcmp(tok, v->perm) == 0) {
			amask = v->mask;
			goto done;
		}
	}

	p = tok;

	while(*p) {
		BOOL found = False;

		for (v = special_values; v->perm; v++) {
			if (v->perm[0] == *p) {
				amask |= v->mask;
				found = True;
			}
		}

		if (!found) {
			printf("ACE '%s': bad permission value at '%s'\n",
				orig_str, p);
			SAFE_FREE(str);
		 	return False;
		}
		p++;
	}

	if (*p) {
		SAFE_FREE(str);
		return False;
	}

 done:
	mask.mask = amask;
	init_sec_ace(ace, &sid, atype, mask, aflags);
	SAFE_FREE(str);
	return True;
}

/* add an ACE to a list of ACEs in a SEC_ACL */
static BOOL add_ace(SEC_ACL **the_acl, SEC_ACE *ace)
{
	SEC_ACL *new_ace;
	SEC_ACE *aces;
	if (! *the_acl) {
		return (((*the_acl) = make_sec_acl(ctx, 3, 1, ace)) != NULL);
	}

	if (!(aces = SMB_CALLOC_ARRAY(SEC_ACE, 1+(*the_acl)->num_aces))) {
		return False;
	}
	memcpy(aces, (*the_acl)->ace, (*the_acl)->num_aces * sizeof(SEC_ACE));
	memcpy(aces+(*the_acl)->num_aces, ace, sizeof(SEC_ACE));
	new_ace = make_sec_acl(ctx,(*the_acl)->revision,1+(*the_acl)->num_aces, aces);
	SAFE_FREE(aces);
	(*the_acl) = new_ace;
	return True;
}

/* parse a ascii version of a security descriptor */
static SEC_DESC *sec_desc_parse(char *str)
{
	const char *p = str;
	fstring tok;
	SEC_DESC *ret = NULL;
	size_t sd_size;
	DOM_SID *grp_sid=NULL, *owner_sid=NULL;
	SEC_ACL *dacl=NULL;
	int revision=1;

	while (next_token(&p, tok, "\t,\r\n", sizeof(tok))) {

		if (strncmp(tok,"REVISION:", 9) == 0) {
			revision = strtol(tok+9, NULL, 16);
			continue;
		}

		if (strncmp(tok,"OWNER:", 6) == 0) {
			if (owner_sid) {
				printf("Only specify owner once\n");
				goto done;
			}
			owner_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!owner_sid ||
			    !StringToSid(owner_sid, tok+6)) {
				printf("Failed to parse owner sid\n");
				goto done;
			}
			continue;
		}

		if (strncmp(tok,"GROUP:", 6) == 0) {
			if (grp_sid) {
				printf("Only specify group once\n");
				goto done;
			}
			grp_sid = SMB_CALLOC_ARRAY(DOM_SID, 1);
			if (!grp_sid ||
			    !StringToSid(grp_sid, tok+6)) {
				printf("Failed to parse group sid\n");
				goto done;
			}
			continue;
		}

		if (strncmp(tok,"ACL:", 4) == 0) {
			SEC_ACE ace;
			if (!parse_ace(&ace, tok+4)) {
				goto done;
			}
			if(!add_ace(&dacl, &ace)) {
				printf("Failed to add ACL %s\n", tok);
				goto done;
			}
			continue;
		}

		printf("Failed to parse token '%s' in security descriptor,\n", tok);
		goto done;
	}

	ret = make_sec_desc(ctx,revision, SEC_DESC_SELF_RELATIVE, owner_sid, grp_sid, 
			    NULL, dacl, &sd_size);

  done:
	SAFE_FREE(grp_sid);
	SAFE_FREE(owner_sid);

	return ret;
}


/* print a ascii version of a security descriptor on a FILE handle */
static void sec_desc_print(FILE *f, SEC_DESC *sd)
{
	fstring sidstr;
	uint32 i;

	fprintf(f, "REVISION:%d\n", sd->revision);

	/* Print owner and group sid */

	if (sd->owner_sid) {
		SidToString(sidstr, sd->owner_sid);
	} else {
		fstrcpy(sidstr, "");
	}

	fprintf(f, "OWNER:%s\n", sidstr);

	if (sd->grp_sid) {
		SidToString(sidstr, sd->grp_sid);
	} else {
		fstrcpy(sidstr, "");
	}

	fprintf(f, "GROUP:%s\n", sidstr);

	/* Print aces */
	for (i = 0; sd->dacl && i < sd->dacl->num_aces; i++) {
		SEC_ACE *ace = &sd->dacl->ace[i];
		fprintf(f, "ACL:");
		print_ace(f, ace);
		fprintf(f, "\n");
	}

}

/***************************************************** 
dump the acls for a file
*******************************************************/
static int cacl_dump(struct cli_state *cli, char *filename)
{
	int result = EXIT_FAILED;
	int fnum = -1;
	SEC_DESC *sd;

	if (test_args) 
		return EXIT_OK;

	fnum = cli_nt_create(cli, filename, CREATE_ACCESS_READ);

	if (fnum == -1) {
		printf("Failed to open %s: %s\n", filename, cli_errstr(cli));
		goto done;
	}

	sd = cli_query_secdesc(cli, fnum, ctx);

	if (!sd) {
		printf("ERROR: secdesc query failed: %s\n", cli_errstr(cli));
		goto done;
	}

	sec_desc_print(stdout, sd);

	result = EXIT_OK;

done:
	if (fnum != -1)
		cli_close(cli, fnum);

	return result;
}

/***************************************************** 
Change the ownership or group ownership of a file. Just
because the NT docs say this can't be done :-). JRA.
*******************************************************/

static int owner_set(struct cli_state *cli, enum chown_mode change_mode, 
		     char *filename, char *new_username)
{
	int fnum;
	DOM_SID sid;
	SEC_DESC *sd, *old;
	size_t sd_size;

	fnum = cli_nt_create(cli, filename, CREATE_ACCESS_READ);

	if (fnum == -1) {
		printf("Failed to open %s: %s\n", filename, cli_errstr(cli));
		return EXIT_FAILED;
	}

	if (!StringToSid(&sid, new_username))
		return EXIT_PARSE_ERROR;

	old = cli_query_secdesc(cli, fnum, ctx);

	cli_close(cli, fnum);

	if (!old) {
		printf("owner_set: Failed to query old descriptor\n");
		return EXIT_FAILED;
	}

	sd = make_sec_desc(ctx,old->revision, old->type,
				(change_mode == REQUEST_CHOWN) ? &sid : NULL,
				(change_mode == REQUEST_CHGRP) ? &sid : NULL,
			   NULL, NULL, &sd_size);

	fnum = cli_nt_create(cli, filename, WRITE_OWNER_ACCESS);

	if (fnum == -1) {
		printf("Failed to open %s: %s\n", filename, cli_errstr(cli));
		return EXIT_FAILED;
	}

	if (!cli_set_secdesc(cli, fnum, sd)) {
		printf("ERROR: secdesc set failed: %s\n", cli_errstr(cli));
	}

	cli_close(cli, fnum);

	return EXIT_OK;
}


/* The MSDN is contradictory over the ordering of ACE entries in an ACL.
   However NT4 gives a "The information may have been modified by a
   computer running Windows NT 5.0" if denied ACEs do not appear before
   allowed ACEs. */

static int ace_compare(SEC_ACE *ace1, SEC_ACE *ace2)
{
	if (sec_ace_equal(ace1, ace2)) 
		return 0;

	if (ace1->type != ace2->type) 
		return ace2->type - ace1->type;

	if (sid_compare(&ace1->trustee, &ace2->trustee)) 
		return sid_compare(&ace1->trustee, &ace2->trustee);

	if (ace1->flags != ace2->flags) 
		return ace1->flags - ace2->flags;

	if (ace1->info.mask != ace2->info.mask) 
		return ace1->info.mask - ace2->info.mask;

	if (ace1->size != ace2->size) 
		return ace1->size - ace2->size;

	return memcmp(ace1, ace2, sizeof(SEC_ACE));
}

static void sort_acl(SEC_ACL *the_acl)
{
	uint32 i;
	if (!the_acl) return;

	qsort(the_acl->ace, the_acl->num_aces, sizeof(the_acl->ace[0]), QSORT_CAST ace_compare);

	for (i=1;i<the_acl->num_aces;) {
		if (sec_ace_equal(&the_acl->ace[i-1], &the_acl->ace[i])) {
			int j;
			for (j=i; j<the_acl->num_aces-1; j++) {
				the_acl->ace[j] = the_acl->ace[j+1];
			}
			the_acl->num_aces--;
		} else {
			i++;
		}
	}
}

/***************************************************** 
set the ACLs on a file given an ascii description
*******************************************************/
static int cacl_set(struct cli_state *cli, char *filename, 
		    char *the_acl, enum acl_mode mode)
{
	int fnum;
	SEC_DESC *sd, *old;
	uint32 i, j;
	size_t sd_size;
	int result = EXIT_OK;

	sd = sec_desc_parse(the_acl);

	if (!sd) return EXIT_PARSE_ERROR;
	if (test_args) return EXIT_OK;

	/* The desired access below is the only one I could find that works
	   with NT4, W2KP and Samba */

	fnum = cli_nt_create(cli, filename, CREATE_ACCESS_READ);

	if (fnum == -1) {
		printf("cacl_set failed to open %s: %s\n", filename, cli_errstr(cli));
		return EXIT_FAILED;
	}

	old = cli_query_secdesc(cli, fnum, ctx);

	if (!old) {
		printf("calc_set: Failed to query old descriptor\n");
		return EXIT_FAILED;
	}

	cli_close(cli, fnum);

	/* the logic here is rather more complex than I would like */
	switch (mode) {
	case SMB_ACL_DELETE:
		for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
			BOOL found = False;

			for (j=0;old->dacl && j<old->dacl->num_aces;j++) {
				if (sec_ace_equal(&sd->dacl->ace[i],
						  &old->dacl->ace[j])) {
					uint32 k;
					for (k=j; k<old->dacl->num_aces-1;k++) {
						old->dacl->ace[k] = old->dacl->ace[k+1];
					}
					old->dacl->num_aces--;
					found = True;
					break;
				}
			}

			if (!found) {
				printf("ACL for ACE:"); 
				print_ace(stdout, &sd->dacl->ace[i]);
				printf(" not found\n");
			}
		}
		break;

	case SMB_ACL_MODIFY:
		for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
			BOOL found = False;

			for (j=0;old->dacl && j<old->dacl->num_aces;j++) {
				if (sid_equal(&sd->dacl->ace[i].trustee,
					      &old->dacl->ace[j].trustee)) {
					old->dacl->ace[j] = sd->dacl->ace[i];
					found = True;
				}
			}

			if (!found) {
				fstring str;

				SidToString(str, &sd->dacl->ace[i].trustee);
				printf("ACL for SID %s not found\n", str);
			}
		}

		if (sd->owner_sid) {
			old->owner_sid = sd->owner_sid;
		}

		if (sd->grp_sid) { 
			old->grp_sid = sd->grp_sid;
		}

		break;

	case SMB_ACL_ADD:
		for (i=0;sd->dacl && i<sd->dacl->num_aces;i++) {
			add_ace(&old->dacl, &sd->dacl->ace[i]);
		}
		break;

	case SMB_ACL_SET:
 		old = sd;
		break;
	}

	/* Denied ACE entries must come before allowed ones */
	sort_acl(old->dacl);

	/* Create new security descriptor and set it */
#if 0
	/* We used to just have "WRITE_DAC_ACCESS" without WRITE_OWNER.
	   But if we're sending an owner, even if it's the same as the one
	   that already exists then W2K3 insists we open with WRITE_OWNER access.
	   I need to check that setting a SD with no owner set works against WNT
	   and W2K. JRA.
	*/

	sd = make_sec_desc(ctx,old->revision, old->type, old->owner_sid, old->grp_sid,
			   NULL, old->dacl, &sd_size);

	fnum = cli_nt_create(cli, filename, WRITE_DAC_ACCESS|WRITE_OWNER_ACCESS);
#else
	sd = make_sec_desc(ctx,old->revision, old->type, NULL, NULL,
			   NULL, old->dacl, &sd_size);

	fnum = cli_nt_create(cli, filename, WRITE_DAC_ACCESS);
#endif
	if (fnum == -1) {
		printf("cacl_set failed to open %s: %s\n", filename, cli_errstr(cli));
		return EXIT_FAILED;
	}

	if (!cli_set_secdesc(cli, fnum, sd)) {
		printf("ERROR: secdesc set failed: %s\n", cli_errstr(cli));
		result = EXIT_FAILED;
	}

	/* Clean up */

	cli_close(cli, fnum);

	return result;
}


/***************************************************** 
return a connection to a server
*******************************************************/
static struct cli_state *connect_one(const char *share)
{
	struct cli_state *c;
	struct in_addr ip;
	NTSTATUS nt_status;
	zero_ip(&ip);
	
	if (!cmdline_auth_info.got_pass) {
		char *pass = getpass("Password: ");
		if (pass) {
			pstrcpy(cmdline_auth_info.password, pass);
			cmdline_auth_info.got_pass = True;
		}
	}

	if (NT_STATUS_IS_OK(nt_status = cli_full_connection(&c, global_myname(), server, 
							    &ip, 0,
							    share, "?????",  
							    cmdline_auth_info.username, lp_workgroup(),
							    cmdline_auth_info.password, 0,
							    cmdline_auth_info.signing_state, NULL))) {
		return c;
	} else {
		DEBUG(0,("cli_full_connection failed! (%s)\n", nt_errstr(nt_status)));
		return NULL;
	}
}

/****************************************************************************
  main program
****************************************************************************/
 int main(int argc, const char *argv[])
{
	char *share;
	int opt;
	enum acl_mode mode = SMB_ACL_SET;
	static char *the_acl = NULL;
	enum chown_mode change_mode = REQUEST_NONE;
	int result;
	fstring path;
	pstring filename;
	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "delete", 'D', POPT_ARG_STRING, NULL, 'D', "Delete an acl", "ACL" },
		{ "modify", 'M', POPT_ARG_STRING, NULL, 'M', "Modify an acl", "ACL" },
		{ "add", 'a', POPT_ARG_STRING, NULL, 'a', "Add an acl", "ACL" },
		{ "set", 'S', POPT_ARG_STRING, NULL, 'S', "Set acls", "ACLS" },
		{ "chown", 'C', POPT_ARG_STRING, NULL, 'C', "Change ownership of a file", "USERNAME" },
		{ "chgrp", 'G', POPT_ARG_STRING, NULL, 'G', "Change group ownership of a file", "GROUPNAME" },
		{ "numeric", 0, POPT_ARG_NONE, &numeric, True, "Don't resolve sids or masks to names" },
		{ "test-args", 't', POPT_ARG_NONE, &test_args, True, "Test arguments"},
		POPT_COMMON_SAMBA
		POPT_COMMON_CREDENTIALS
		{ NULL }
	};

	struct cli_state *cli;

	load_case_tables();

	ctx=talloc_init("main");

	/* set default debug level to 1 regardless of what smb.conf sets */
	setup_logging( "smbcacls", True );
	DEBUGLEVEL_CLASS[DBGC_ALL] = 1;
	dbf = x_stderr;
	x_setbuf( x_stderr, NULL );

	setlinebuf(stdout);

	lp_load(dyn_CONFIGFILE,True,False,False,True);
	load_interfaces();

	pc = poptGetContext("smbcacls", argc, argv, long_options, 0);
	
	poptSetOtherOptionHelp(pc, "//server1/share1 filename\nACLs look like: "
		"'ACL:user:[ALLOWED|DENIED]/flags/permissions'");

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case 'S':
			the_acl = smb_xstrdup(poptGetOptArg(pc));
			mode = SMB_ACL_SET;
			break;

		case 'D':
			the_acl = smb_xstrdup(poptGetOptArg(pc));
			mode = SMB_ACL_DELETE;
			break;

		case 'M':
			the_acl = smb_xstrdup(poptGetOptArg(pc));
			mode = SMB_ACL_MODIFY;
			break;

		case 'a':
			the_acl = smb_xstrdup(poptGetOptArg(pc));
			mode = SMB_ACL_ADD;
			break;

		case 'C':
			pstrcpy(owner_username,poptGetOptArg(pc));
			change_mode = REQUEST_CHOWN;
			break;

		case 'G':
			pstrcpy(owner_username,poptGetOptArg(pc));
			change_mode = REQUEST_CHGRP;
			break;
		}
	}

	/* Make connection to server */
	if(!poptPeekArg(pc)) { 
		poptPrintUsage(pc, stderr, 0);
		return -1;
	}
	
	fstrcpy(path, poptGetArg(pc));
	
	if(!poptPeekArg(pc)) { 
		poptPrintUsage(pc, stderr, 0);	
		return -1;
	}
	
	pstrcpy(filename, poptGetArg(pc));

	all_string_sub(path,"/","\\",0);

	fstrcpy(server,path+2);
	share = strchr_m(server,'\\');
	if (!share) {
		share = strchr_m(server,'/');
		if (!share) {
			printf("Invalid argument: %s\n", share);
			return -1;
		}
	}

	*share = 0;
	share++;

	if (!test_args) {
		cli = connect_one(share);
		if (!cli) {
			talloc_destroy(ctx);
			exit(EXIT_FAILED);
		}
	} else {
		exit(0);
	}

	all_string_sub(filename, "/", "\\", 0);
	if (filename[0] != '\\') {
		pstring s;
		s[0] = '\\';
		safe_strcpy(&s[1], filename, sizeof(pstring)-2);
		pstrcpy(filename, s);
	}

	/* Perform requested action */

	if (change_mode != REQUEST_NONE) {
		result = owner_set(cli, change_mode, filename, owner_username);
	} else if (the_acl) {
		result = cacl_set(cli, filename, the_acl, mode);
	} else {
		result = cacl_dump(cli, filename);
	}

	talloc_destroy(ctx);

	return result;
}
