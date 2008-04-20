/* 
   Unix SMB/CIFS implementation.
   passdb editing frontend
   
   Copyright (C) Simo Sorce      2000
   Copyright (C) Andrew Bartlett 2001   
   Copyright (C) Jelmer Vernooij 2002

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

#define BIT_BACKEND	0x00000004
#define BIT_VERBOSE	0x00000008
#define BIT_SPSTYLE	0x00000010
#define BIT_CAN_CHANGE	0x00000020
#define BIT_MUST_CHANGE	0x00000040
#define BIT_USERSIDS	0x00000080
#define BIT_FULLNAME	0x00000100
#define BIT_HOMEDIR	0x00000200
#define BIT_HDIRDRIVE	0x00000400
#define BIT_LOGSCRIPT	0x00000800
#define BIT_PROFILE	0x00001000
#define BIT_MACHINE	0x00002000
#define BIT_USERDOMAIN	0x00004000
#define BIT_USER	0x00008000
#define BIT_LIST	0x00010000
#define BIT_MODIFY	0x00020000
#define BIT_CREATE	0x00040000
#define BIT_DELETE	0x00080000
#define BIT_ACCPOLICY	0x00100000
#define BIT_ACCPOLVAL	0x00200000
#define BIT_ACCTCTRL	0x00400000
#define BIT_RESERV_7	0x00800000
#define BIT_IMPORT	0x01000000
#define BIT_EXPORT	0x02000000
#define BIT_FIX_INIT    0x04000000
#define BIT_BADPWRESET	0x08000000
#define BIT_LOGONHOURS	0x10000000

#define MASK_ALWAYS_GOOD	0x0000001F
#define MASK_USER_GOOD		0x00405FE0

/*********************************************************
 Add all currently available users to another db
 ********************************************************/

static int export_database (struct pdb_methods *in, 
                            struct pdb_methods *out, 
                            const char *username) 
{
	struct samu *user = NULL;
	NTSTATUS status;

	DEBUG(3, ("export_database: username=\"%s\"\n", username ? username : "(NULL)"));

	status = in->setsampwent(in, 0, 0);
	if ( NT_STATUS_IS_ERR(status) ) {
		fprintf(stderr, "Unable to set account database iterator for %s!\n", 
			in->name);
		return 1;
	}

	if ( ( user = samu_new( NULL ) ) == NULL ) {
		fprintf(stderr, "export_database: Memory allocation failure!\n");
		return 1;
	}

	while ( NT_STATUS_IS_OK(in->getsampwent(in, user)) ) 
	{
		DEBUG(4, ("Processing account %s\n", user->username));

		/* If we don't have a specific user or if we do and 
		   the login name matches */

		if ( !username || (strcmp(username, user->username) == 0)) {
			struct samu *account;

			if ( (account = samu_new( NULL )) == NULL ) {
				fprintf(stderr, "export_database: Memory allocation failure!\n");
				TALLOC_FREE( user );
				in->endsampwent( in );
				return 1;
			}

			printf("Importing accout for %s...", user->username);
			if ( !NT_STATUS_IS_OK(out->getsampwnam( out, account, user->username )) ) {
				status = out->add_sam_account(out, user);
			} else {
				status = out->update_sam_account( out, user );
			}

			if ( NT_STATUS_IS_OK(status) ) {
				printf( "ok\n");
			} else {
				printf( "failed\n");
			}

			TALLOC_FREE( account );
		}

		/* clean up and get ready for another run */

		TALLOC_FREE( user );

		if ( ( user = samu_new( NULL ) ) == NULL ) {
			fprintf(stderr, "export_database: Memory allocation failure!\n");
			return 1;
		}
	}

	TALLOC_FREE( user );

	in->endsampwent(in);

	return 0;
}

/*********************************************************
 Add all currently available group mappings to another db
 ********************************************************/

static int export_groups (struct pdb_methods *in, struct pdb_methods *out) 
{
	GROUP_MAP *maps = NULL;
	size_t i, entries = 0;
	NTSTATUS status;

	status = in->enum_group_mapping(in, get_global_sam_sid(), 
			SID_NAME_DOM_GRP, &maps, &entries, False);

	if ( NT_STATUS_IS_ERR(status) ) {
		fprintf(stderr, "Unable to enumerate group map entries.\n");
		return 1;
	}

	for (i=0; i<entries; i++) {
		out->add_group_mapping_entry(out, &(maps[i]));
	}

	SAFE_FREE( maps );

	return 0;
}

/*********************************************************
 Reset account policies to their default values and remove marker
 ********************************************************/

static int reinit_account_policies (void) 
{
	int i;

	for (i=1; decode_account_policy_name(i) != NULL; i++) {
		uint32 policy_value;
		if (!account_policy_get_default(i, &policy_value)) {
			fprintf(stderr, "Can't get default account policy\n");
			return -1;
		}
		if (!account_policy_set(i, policy_value)) {
			fprintf(stderr, "Can't set account policy in tdb\n");
			return -1;
		}
	}

	if (!remove_account_policy_migrated()) {
		fprintf(stderr, "Can't remove marker from tdb\n");
		return -1;
	}

	return 0;
}


/*********************************************************
 Add all currently available account policy from tdb to one backend
 ********************************************************/

static int export_account_policies (struct pdb_methods *in, struct pdb_methods *out) 
{
	int i;

	if (!account_policy_migrated(True)) {
		fprintf(stderr, "Unable to set account policy marker in tdb\n");
		return -1;
	}

	for ( i=1; decode_account_policy_name(i) != NULL; i++ ) {
		uint32 policy_value;
		NTSTATUS status;

		status = in->get_account_policy(in, i, &policy_value);

		if ( NT_STATUS_IS_ERR(status) ) {
			fprintf(stderr, "Unable to get account policy from %s\n", in->name);
			remove_account_policy_migrated();
			return -1;
		}

		status = out->set_account_policy(out, i, policy_value);

		if ( NT_STATUS_IS_ERR(status) ) {
			fprintf(stderr, "Unable to migrate account policy to %s\n", out->name);
			remove_account_policy_migrated();
			return -1;
		}
	}

	return 0;
}


/*********************************************************
 Print info from sam structure
**********************************************************/

static int print_sam_info (struct samu *sam_pwent, BOOL verbosity, BOOL smbpwdstyle)
{
	uid_t uid;
	time_t tmp;

	/* TODO: chaeck if entry is a user or a workstation */
	if (!sam_pwent) return -1;
	
	if (verbosity) {
		pstring temp;
		const uint8 *hours;
		
		printf ("Unix username:        %s\n", pdb_get_username(sam_pwent));
		printf ("NT username:          %s\n", pdb_get_nt_username(sam_pwent));
		printf ("Account Flags:        %s\n", pdb_encode_acct_ctrl(pdb_get_acct_ctrl(sam_pwent), NEW_PW_FORMAT_SPACE_PADDED_LEN));
		printf ("User SID:             %s\n",
			sid_string_static(pdb_get_user_sid(sam_pwent)));
		printf ("Primary Group SID:    %s\n",
			sid_string_static(pdb_get_group_sid(sam_pwent)));
		printf ("Full Name:            %s\n", pdb_get_fullname(sam_pwent));
		printf ("Home Directory:       %s\n", pdb_get_homedir(sam_pwent));
		printf ("HomeDir Drive:        %s\n", pdb_get_dir_drive(sam_pwent));
		printf ("Logon Script:         %s\n", pdb_get_logon_script(sam_pwent));
		printf ("Profile Path:         %s\n", pdb_get_profile_path(sam_pwent));
		printf ("Domain:               %s\n", pdb_get_domain(sam_pwent));
		printf ("Account desc:         %s\n", pdb_get_acct_desc(sam_pwent));
		printf ("Workstations:         %s\n", pdb_get_workstations(sam_pwent));
		printf ("Munged dial:          %s\n", pdb_get_munged_dial(sam_pwent));
		
		tmp = pdb_get_logon_time(sam_pwent);
		printf ("Logon time:           %s\n", tmp ? http_timestring(tmp) : "0");
		
		tmp = pdb_get_logoff_time(sam_pwent);
		printf ("Logoff time:          %s\n", tmp ? http_timestring(tmp) : "0");
		
		tmp = pdb_get_kickoff_time(sam_pwent);
		printf ("Kickoff time:         %s\n", tmp ? http_timestring(tmp) : "0");
		
		tmp = pdb_get_pass_last_set_time(sam_pwent);
		printf ("Password last set:    %s\n", tmp ? http_timestring(tmp) : "0");
		
		tmp = pdb_get_pass_can_change_time(sam_pwent);
		printf ("Password can change:  %s\n", tmp ? http_timestring(tmp) : "0");
		
		tmp = pdb_get_pass_must_change_time(sam_pwent);
		printf ("Password must change: %s\n", tmp ? http_timestring(tmp) : "0");

		tmp = pdb_get_bad_password_time(sam_pwent);
		printf ("Last bad password   : %s\n", tmp ? http_timestring(tmp) : "0");
		printf ("Bad password count  : %d\n", 
			pdb_get_bad_password_count(sam_pwent));
		
		hours = pdb_get_hours(sam_pwent);
		pdb_sethexhours(temp, hours);
		printf ("Logon hours         : %s\n", temp);
		
	} else if (smbpwdstyle) {
		char lm_passwd[33];
		char nt_passwd[33];

		uid = nametouid(pdb_get_username(sam_pwent));
		pdb_sethexpwd(lm_passwd, pdb_get_lanman_passwd(sam_pwent), pdb_get_acct_ctrl(sam_pwent));
		pdb_sethexpwd(nt_passwd, pdb_get_nt_passwd(sam_pwent), pdb_get_acct_ctrl(sam_pwent));
			
		printf("%s:%lu:%s:%s:%s:LCT-%08X:\n",
		       pdb_get_username(sam_pwent),
		       (unsigned long)uid,
		       lm_passwd,
		       nt_passwd,
		       pdb_encode_acct_ctrl(pdb_get_acct_ctrl(sam_pwent),NEW_PW_FORMAT_SPACE_PADDED_LEN),
		       (uint32)pdb_get_pass_last_set_time(sam_pwent));
	} else {
		uid = nametouid(pdb_get_username(sam_pwent));
		printf ("%s:%lu:%s\n", pdb_get_username(sam_pwent), (unsigned long)uid,	
			pdb_get_fullname(sam_pwent));
	}

	return 0;	
}

/*********************************************************
 Get an Print User Info
**********************************************************/

static int print_user_info (struct pdb_methods *in, const char *username, BOOL verbosity, BOOL smbpwdstyle)
{
	struct samu *sam_pwent=NULL;
	BOOL ret;

	if ( (sam_pwent = samu_new( NULL )) == NULL ) {
		return -1;
	}

	ret = NT_STATUS_IS_OK(in->getsampwnam (in, sam_pwent, username));

	if (ret==False) {
		fprintf (stderr, "Username not found!\n");
		TALLOC_FREE(sam_pwent);
		return -1;
	}

	ret=print_sam_info (sam_pwent, verbosity, smbpwdstyle);
	TALLOC_FREE(sam_pwent);
	
	return ret;
}
	
/*********************************************************
 List Users
**********************************************************/
static int print_users_list (struct pdb_methods *in, BOOL verbosity, BOOL smbpwdstyle)
{
	struct samu *sam_pwent=NULL;
	BOOL check;
	
	check = NT_STATUS_IS_OK(in->setsampwent(in, False, 0));
	if (!check) {
		return 1;
	}

	check = True;
	if ( (sam_pwent = samu_new( NULL )) == NULL ) {
		return 1;
	}

	while (check && NT_STATUS_IS_OK(in->getsampwent (in, sam_pwent))) {
		if (verbosity)
			printf ("---------------\n");
		print_sam_info (sam_pwent, verbosity, smbpwdstyle);
		TALLOC_FREE(sam_pwent);
		
		if ( (sam_pwent = samu_new( NULL )) == NULL ) {
			check = False;
		}
	}
	if (check) 
		TALLOC_FREE(sam_pwent);
	
	in->endsampwent(in);
	return 0;
}

/*********************************************************
 Fix a list of Users for uninitialised passwords
**********************************************************/
static int fix_users_list (struct pdb_methods *in)
{
	struct samu *sam_pwent=NULL;
	BOOL check;
	
	check = NT_STATUS_IS_OK(in->setsampwent(in, False, 0));
	if (!check) {
		return 1;
	}

	check = True;
	if ( (sam_pwent = samu_new( NULL )) == NULL ) {
		return 1;
	}

	while (check && NT_STATUS_IS_OK(in->getsampwent (in, sam_pwent))) {
		printf("Updating record for user %s\n", pdb_get_username(sam_pwent));
	
		if (!NT_STATUS_IS_OK(pdb_update_sam_account(sam_pwent))) {
			printf("Update of user %s failed!\n", pdb_get_username(sam_pwent));
		}
		TALLOC_FREE(sam_pwent);
		if ( (sam_pwent = samu_new( NULL )) == NULL ) {
			check = False;
		}
		if (!check) {
			fprintf(stderr, "Failed to initialise new struct samu structure (out of memory?)\n");
		}
			
	}
	if (check) 
		TALLOC_FREE(sam_pwent);
	
	in->endsampwent(in);
	return 0;
}

/*********************************************************
 Set User Info
**********************************************************/

static int set_user_info (struct pdb_methods *in, const char *username, 
			  const char *fullname, const char *homedir, 
			  const char *acct_desc, 
			  const char *drive, const char *script, 
			  const char *profile, const char *account_control,
			  const char *user_sid, const char *user_domain,
			  const BOOL badpw, const BOOL hours,
			  time_t pwd_can_change, time_t pwd_must_change)
{
	BOOL updated_autolock = False, updated_badpw = False;
	struct samu *sam_pwent=NULL;
	BOOL ret;
	
	if ( (sam_pwent = samu_new( NULL )) == NULL ) {
		return 1;
	}
	
	ret = NT_STATUS_IS_OK(in->getsampwnam (in, sam_pwent, username));
	if (ret==False) {
		fprintf (stderr, "Username not found!\n");
		TALLOC_FREE(sam_pwent);
		return -1;
	}

	if (hours) {
		uint8 hours_array[MAX_HOURS_LEN];
		uint32 hours_len;
		
		hours_len = pdb_get_hours_len(sam_pwent);
		memset(hours_array, 0xff, hours_len);
		
		pdb_set_hours(sam_pwent, hours_array, PDB_CHANGED);
	}

	if (pwd_can_change != -1) {
		pdb_set_pass_can_change_time(sam_pwent, pwd_can_change, PDB_CHANGED);
	}

	if (pwd_must_change != -1) {
		pdb_set_pass_must_change_time(sam_pwent, pwd_must_change, PDB_CHANGED);
	}

	if (!pdb_update_autolock_flag(sam_pwent, &updated_autolock)) {
		DEBUG(2,("pdb_update_autolock_flag failed.\n"));
	}

	if (!pdb_update_bad_password_count(sam_pwent, &updated_badpw)) {
		DEBUG(2,("pdb_update_bad_password_count failed.\n"));
	}

	if (fullname)
		pdb_set_fullname(sam_pwent, fullname, PDB_CHANGED);
	if (acct_desc)
		pdb_set_acct_desc(sam_pwent, acct_desc, PDB_CHANGED);
	if (homedir)
		pdb_set_homedir(sam_pwent, homedir, PDB_CHANGED);
	if (drive)
		pdb_set_dir_drive(sam_pwent,drive, PDB_CHANGED);
	if (script)
		pdb_set_logon_script(sam_pwent, script, PDB_CHANGED);
	if (profile)
		pdb_set_profile_path (sam_pwent, profile, PDB_CHANGED);
	if (user_domain)
		pdb_set_domain(sam_pwent, user_domain, PDB_CHANGED);

	if (account_control) {
		uint32 not_settable = ~(ACB_DISABLED|ACB_HOMDIRREQ|ACB_PWNOTREQ|
					ACB_PWNOEXP|ACB_AUTOLOCK);

		uint32 newflag = pdb_decode_acct_ctrl(account_control);

		if (newflag & not_settable) {
			fprintf(stderr, "Can only set [NDHLX] flags\n");
			TALLOC_FREE(sam_pwent);
			return -1;
		}

		pdb_set_acct_ctrl(sam_pwent,
				  (pdb_get_acct_ctrl(sam_pwent) & not_settable) | newflag,
				  PDB_CHANGED);
	}
	if (user_sid) {
		DOM_SID u_sid;
		if (!string_to_sid(&u_sid, user_sid)) {
			/* not a complete sid, may be a RID, try building a SID */
			int u_rid;
			
			if (sscanf(user_sid, "%d", &u_rid) != 1) {
				fprintf(stderr, "Error passed string is not a complete user SID or RID!\n");
				return -1;
			}
			sid_copy(&u_sid, get_global_sam_sid());
			sid_append_rid(&u_sid, u_rid);
		}
		pdb_set_user_sid (sam_pwent, &u_sid, PDB_CHANGED);
	}

	if (badpw) {
		pdb_set_bad_password_count(sam_pwent, 0, PDB_CHANGED);
		pdb_set_bad_password_time(sam_pwent, 0, PDB_CHANGED);
	}

	if (NT_STATUS_IS_OK(in->update_sam_account (in, sam_pwent)))
		print_user_info (in, username, True, False);
	else {
		fprintf (stderr, "Unable to modify entry!\n");
		TALLOC_FREE(sam_pwent);
		return -1;
	}
	TALLOC_FREE(sam_pwent);
	return 0;
}

/*********************************************************
 Add New User
**********************************************************/
static int new_user (struct pdb_methods *in, const char *username,
			const char *fullname, const char *homedir,
			const char *drive, const char *script,
			const char *profile, char *user_sid, BOOL stdin_get)
{
	struct samu *sam_pwent;
	char *password1, *password2;
	int rc_pwd_cmp;
	struct passwd *pwd;

	get_global_sam_sid();

	if ( !(pwd = getpwnam_alloc( NULL, username )) ) {
		DEBUG(0,("Cannot locate Unix account for %s\n", username));
		return -1;
	}

	if ( (sam_pwent = samu_new( NULL )) == NULL ) {
		DEBUG(0, ("Memory allocation failure!\n"));
		return -1;
	}

	if (!NT_STATUS_IS_OK(samu_alloc_rid_unix(sam_pwent, pwd ))) {
		TALLOC_FREE( sam_pwent );
		TALLOC_FREE( pwd );
		DEBUG(0, ("could not create account to add new user %s\n", username));
		return -1;
	}

	password1 = get_pass( "new password:", stdin_get);
	password2 = get_pass( "retype new password:", stdin_get);
	if ((rc_pwd_cmp = strcmp (password1, password2))) {
		fprintf (stderr, "Passwords do not match!\n");
		TALLOC_FREE(sam_pwent);
	} else {
		pdb_set_plaintext_passwd(sam_pwent, password1);
	}

	memset(password1, 0, strlen(password1));
	SAFE_FREE(password1);
	memset(password2, 0, strlen(password2));
	SAFE_FREE(password2);

	/* pwds do _not_ match? */
	if (rc_pwd_cmp)
		return -1;

	if (fullname)
		pdb_set_fullname(sam_pwent, fullname, PDB_CHANGED);
	if (homedir)
		pdb_set_homedir (sam_pwent, homedir, PDB_CHANGED);
	if (drive)
		pdb_set_dir_drive (sam_pwent, drive, PDB_CHANGED);
	if (script)
		pdb_set_logon_script(sam_pwent, script, PDB_CHANGED);
	if (profile)
		pdb_set_profile_path (sam_pwent, profile, PDB_CHANGED);
	if (user_sid) {
		DOM_SID u_sid;
		if (!string_to_sid(&u_sid, user_sid)) {
			/* not a complete sid, may be a RID, try building a SID */
			int u_rid;
			
			if (sscanf(user_sid, "%d", &u_rid) != 1) {
				fprintf(stderr, "Error passed string is not a complete user SID or RID!\n");
				return -1;
			}
			sid_copy(&u_sid, get_global_sam_sid());
			sid_append_rid(&u_sid, u_rid);
		}
		pdb_set_user_sid (sam_pwent, &u_sid, PDB_CHANGED);
	}
	
	pdb_set_acct_ctrl (sam_pwent, ACB_NORMAL, PDB_CHANGED);
	
	if (NT_STATUS_IS_OK(in->add_sam_account (in, sam_pwent))) { 
		print_user_info (in, username, True, False);
	} else {
		fprintf (stderr, "Unable to add user! (does it already exist?)\n");
		TALLOC_FREE(sam_pwent);
		return -1;
	}
	TALLOC_FREE(sam_pwent);
	return 0;
}

/*********************************************************
 Add New Machine
**********************************************************/

static int new_machine (struct pdb_methods *in, const char *machine_in)
{
	struct samu *sam_pwent=NULL;
	fstring machinename;
	fstring machineaccount;
	struct passwd  *pwd = NULL;
	
	get_global_sam_sid();

	if (strlen(machine_in) == 0) {
		fprintf(stderr, "No machine name given\n");
		return -1;
	}

	fstrcpy(machinename, machine_in); 
	machinename[15]= '\0';

	if (machinename[strlen (machinename) -1] == '$')
		machinename[strlen (machinename) -1] = '\0';
	
	strlower_m(machinename);
	
	fstrcpy(machineaccount, machinename);
	fstrcat(machineaccount, "$");

	if ((pwd = getpwnam_alloc(NULL, machineaccount))) {

		if ( (sam_pwent = samu_new( NULL )) == NULL ) {
			fprintf(stderr, "Memory allocation error!\n");
			TALLOC_FREE(pwd);
			return -1;
		}

		if ( !NT_STATUS_IS_OK(samu_set_unix(sam_pwent, pwd )) ) {
			fprintf(stderr, "Could not init sam from pw\n");
			TALLOC_FREE(pwd);
			return -1;
		}

		TALLOC_FREE(pwd);
	} else {
		if ( (sam_pwent = samu_new( NULL )) == NULL ) {
			fprintf(stderr, "Could not init sam from pw\n");
			return -1;
		}
	}

	pdb_set_plaintext_passwd (sam_pwent, machinename);
	pdb_set_username (sam_pwent, machineaccount, PDB_CHANGED);	
	pdb_set_acct_ctrl (sam_pwent, ACB_WSTRUST, PDB_CHANGED);
	
	if (NT_STATUS_IS_OK(in->add_sam_account (in, sam_pwent))) {
		print_user_info (in, machineaccount, True, False);
	} else {
		fprintf (stderr, "Unable to add machine! (does it already exist?)\n");
		TALLOC_FREE(sam_pwent);
		return -1;
	}
	TALLOC_FREE(sam_pwent);
	return 0;
}

/*********************************************************
 Delete user entry
**********************************************************/

static int delete_user_entry (struct pdb_methods *in, const char *username)
{
	struct samu *samaccount = NULL;

	if ( (samaccount = samu_new( NULL )) == NULL ) {
		return -1;
	}

	if (!NT_STATUS_IS_OK(in->getsampwnam(in, samaccount, username))) {
		fprintf (stderr, "user %s does not exist in the passdb\n", username);
		return -1;
	}

	if (!NT_STATUS_IS_OK(in->delete_sam_account (in, samaccount))) {
		fprintf (stderr, "Unable to delete user %s\n", username);
		return -1;
	}
	return 0;
}

/*********************************************************
 Delete machine entry
**********************************************************/

static int delete_machine_entry (struct pdb_methods *in, const char *machinename)
{
	fstring name;
	struct samu *samaccount = NULL;

	if (strlen(machinename) == 0) {
		fprintf(stderr, "No machine name given\n");
		return -1;
	}
	
	fstrcpy(name, machinename);
	name[15] = '\0';
	if (name[strlen(name)-1] != '$')
		fstrcat (name, "$");

	if ( (samaccount = samu_new( NULL )) == NULL ) {
		return -1;
	}

	if (!NT_STATUS_IS_OK(in->getsampwnam(in, samaccount, name))) {
		fprintf (stderr, "machine %s does not exist in the passdb\n", name);
		return -1;
	}

	if (!NT_STATUS_IS_OK(in->delete_sam_account (in, samaccount))) {
		fprintf (stderr, "Unable to delete machine %s\n", name);
		return -1;
	}

	return 0;
}

/*********************************************************
 Start here.
**********************************************************/

int main (int argc, char **argv)
{
	static BOOL list_users = False;
	static BOOL verbose = False;
	static BOOL spstyle = False;
	static BOOL machine = False;
	static BOOL add_user = False;
	static BOOL delete_user = False;
	static BOOL modify_user = False;
	uint32	setparms, checkparms;
	int opt;
	static char *full_name = NULL;
	static char *acct_desc = NULL;
	static const char *user_name = NULL;
	static char *home_dir = NULL;
	static char *home_drive = NULL;
	static char *backend = NULL;
	static char *backend_in = NULL;
	static char *backend_out = NULL;
	static BOOL transfer_groups = False;
	static BOOL transfer_account_policies = False;
	static BOOL reset_account_policies = False;
	static BOOL  force_initialised_password = False;
	static char *logon_script = NULL;
	static char *profile_path = NULL;
	static char *user_domain = NULL;
	static char *account_control = NULL;
	static char *account_policy = NULL;
	static char *user_sid = NULL;
	static long int account_policy_value = 0;
	BOOL account_policy_value_set = False;
	static BOOL badpw_reset = False;
	static BOOL hours_reset = False;
	static char *pwd_can_change_time = NULL;
	static char *pwd_must_change_time = NULL;
	static char *pwd_time_format = NULL;
	static BOOL pw_from_stdin = False;
	struct pdb_methods *bin, *bout, *bdef;
	poptContext pc;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{"list",	'L', POPT_ARG_NONE, &list_users, 0, "list all users", NULL},
		{"verbose",	'v', POPT_ARG_NONE, &verbose, 0, "be verbose", NULL },
		{"smbpasswd-style",	'w',POPT_ARG_NONE, &spstyle, 0, "give output in smbpasswd style", NULL},
		{"user",	'u', POPT_ARG_STRING, &user_name, 0, "use username", "USER" },
		{"account-desc",	'N', POPT_ARG_STRING, &acct_desc, 0, "set account description", NULL},
		{"fullname",	'f', POPT_ARG_STRING, &full_name, 0, "set full name", NULL},
		{"homedir",	'h', POPT_ARG_STRING, &home_dir, 0, "set home directory", NULL},
		{"drive",	'D', POPT_ARG_STRING, &home_drive, 0, "set home drive", NULL},
		{"script",	'S', POPT_ARG_STRING, &logon_script, 0, "set logon script", NULL},
		{"profile",	'p', POPT_ARG_STRING, &profile_path, 0, "set profile path", NULL},
		{"domain",	'I', POPT_ARG_STRING, &user_domain, 0, "set a users' domain", NULL},
		{"user SID",	'U', POPT_ARG_STRING, &user_sid, 0, "set user SID or RID", NULL},
		{"create",	'a', POPT_ARG_NONE, &add_user, 0, "create user", NULL},
		{"modify",	'r', POPT_ARG_NONE, &modify_user, 0, "modify user", NULL},
		{"machine",	'm', POPT_ARG_NONE, &machine, 0, "account is a machine account", NULL},
		{"delete",	'x', POPT_ARG_NONE, &delete_user, 0, "delete user", NULL},
		{"backend",	'b', POPT_ARG_STRING, &backend, 0, "use different passdb backend as default backend", NULL},
		{"import",	'i', POPT_ARG_STRING, &backend_in, 0, "import user accounts from this backend", NULL},
		{"export",	'e', POPT_ARG_STRING, &backend_out, 0, "export user accounts to this backend", NULL},
		{"group",	'g', POPT_ARG_NONE, &transfer_groups, 0, "use -i and -e for groups", NULL},
		{"policies",	'y', POPT_ARG_NONE, &transfer_account_policies, 0, "use -i and -e to move account policies between backends", NULL},
		{"policies-reset",	0, POPT_ARG_NONE, &reset_account_policies, 0, "restore default policies", NULL},
		{"account-policy",	'P', POPT_ARG_STRING, &account_policy, 0,"value of an account policy (like maximum password age)",NULL},
		{"value",       'C', POPT_ARG_LONG, &account_policy_value, 'C',"set the account policy to this value", NULL},
		{"account-control",	'c', POPT_ARG_STRING, &account_control, 0, "Values of account control", NULL},
		{"force-initialized-passwords", 0, POPT_ARG_NONE, &force_initialised_password, 0, "Force initialization of corrupt password strings in a passdb backend", NULL},
		{"bad-password-count-reset", 'z', POPT_ARG_NONE, &badpw_reset, 0, "reset bad password count", NULL},
		{"logon-hours-reset", 'Z', POPT_ARG_NONE, &hours_reset, 0, "reset logon hours", NULL},
		{"pwd-can-change-time", 0, POPT_ARG_STRING, &pwd_can_change_time, 0, "Set password can change time (unix time in seconds since 1970 if time format not provided)", NULL },
		{"pwd-must-change-time", 0, POPT_ARG_STRING, &pwd_must_change_time, 0, "Set password must change time (unix time in seconds since 1970 if time format not provided)", NULL },
		{"time-format", 0, POPT_ARG_STRING, &pwd_time_format, 0, "The time format for time parameters", NULL },
		{"password-from-stdin", 't', POPT_ARG_NONE, &pw_from_stdin, 0, "get password from standard in", NULL},
		POPT_COMMON_SAMBA
		POPT_TABLEEND
	};
	
	bin = bout = bdef = NULL;

	load_case_tables();

	setup_logging("pdbedit", True);
	
	pc = poptGetContext(NULL, argc, (const char **) argv, long_options,
			    POPT_CONTEXT_KEEP_FIRST);
	
	while((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		case 'C':
			account_policy_value_set = True;
			break;
		}
	}

	poptGetArg(pc); /* Drop argv[0], the program name */

	if (user_name == NULL)
		user_name = poptGetArg(pc);

	if (!lp_load(dyn_CONFIGFILE,True,False,False,True)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", dyn_CONFIGFILE);
		exit(1);
	}

	if(!initialize_password_db(False))
		exit(1);

	if (!init_names())
		exit(1);

	setparms =	(backend ? BIT_BACKEND : 0) +
			(verbose ? BIT_VERBOSE : 0) +
			(spstyle ? BIT_SPSTYLE : 0) +
			(full_name ? BIT_FULLNAME : 0) +
			(home_dir ? BIT_HOMEDIR : 0) +
			(home_drive ? BIT_HDIRDRIVE : 0) +
			(logon_script ? BIT_LOGSCRIPT : 0) +
			(profile_path ? BIT_PROFILE : 0) +
			(user_domain ? BIT_USERDOMAIN : 0) +
			(machine ? BIT_MACHINE : 0) +
			(user_name ? BIT_USER : 0) +
			(list_users ? BIT_LIST : 0) +
			(force_initialised_password ? BIT_FIX_INIT : 0) +
			(user_sid ? BIT_USERSIDS : 0) +
			(modify_user ? BIT_MODIFY : 0) +
			(add_user ? BIT_CREATE : 0) +
			(delete_user ? BIT_DELETE : 0) +
			(account_control ? BIT_ACCTCTRL : 0) +
			(account_policy ? BIT_ACCPOLICY : 0) +
			(account_policy_value_set ? BIT_ACCPOLVAL : 0) +
			(backend_in ? BIT_IMPORT : 0) +
			(backend_out ? BIT_EXPORT : 0) +
			(badpw_reset ? BIT_BADPWRESET : 0) +
			(hours_reset ? BIT_LOGONHOURS : 0) +
			(pwd_can_change_time ? BIT_CAN_CHANGE: 0) +
			(pwd_must_change_time ? BIT_MUST_CHANGE: 0);

	if (setparms & BIT_BACKEND) {
		if (!NT_STATUS_IS_OK(make_pdb_method_name( &bdef, backend ))) {
			fprintf(stderr, "Can't initialize passdb backend.\n");
			return 1;
		}
	} else {
		if (!NT_STATUS_IS_OK(make_pdb_method_name(&bdef, lp_passdb_backend()))) {
			fprintf(stderr, "Can't initialize passdb backend.\n");
			return 1;
		}
	}
	
	/* the lowest bit options are always accepted */
	checkparms = setparms & ~MASK_ALWAYS_GOOD;

	if (checkparms & BIT_FIX_INIT) {
		return fix_users_list(bdef);
	}

	/* account policy operations */
	if ((checkparms & BIT_ACCPOLICY) && !(checkparms & ~(BIT_ACCPOLICY + BIT_ACCPOLVAL))) {
		uint32 value;
		int field = account_policy_name_to_fieldnum(account_policy);
		if (field == 0) {
			char *apn = account_policy_names_list();
			fprintf(stderr, "No account policy by that name\n");
			if (apn) {
				fprintf(stderr, "Account policy names are :\n%s\n", apn);
			}
			SAFE_FREE(apn);
			exit(1);
		}
		if (!pdb_get_account_policy(field, &value)) {
			fprintf(stderr, "valid account policy, but unable to fetch value!\n");
			if (!account_policy_value_set)
				exit(1);
		}
		printf("account policy \"%s\" description: %s\n", account_policy, account_policy_get_desc(field));
		if (account_policy_value_set) {
			printf("account policy \"%s\" value was: %u\n", account_policy, value);
			if (!pdb_set_account_policy(field, account_policy_value)) {
				fprintf(stderr, "valid account policy, but unable to set value!\n");
				exit(1);
			}
			printf("account policy \"%s\" value is now: %lu\n", account_policy, account_policy_value);
			exit(0);
		} else {
			printf("account policy \"%s\" value is: %u\n", account_policy, value);
			exit(0);
		}
	}

	if (reset_account_policies) {
		if (!reinit_account_policies()) {
			exit(1);
		}

		exit(0);
	}

	/* import and export operations */

	if ( ((checkparms & BIT_IMPORT) 
		|| (checkparms & BIT_EXPORT))
		&& !(checkparms & ~(BIT_IMPORT +BIT_EXPORT +BIT_USER)) ) 
	{
		NTSTATUS status;

		bin = bout = bdef;

		if (backend_in) {
			status = make_pdb_method_name(&bin, backend_in);

			if ( !NT_STATUS_IS_OK(status) ) {
				fprintf(stderr, "Unable to initialize %s.\n", backend_in);
				return 1;
			}
		}

		if (backend_out) {
			status = make_pdb_method_name(&bout, backend_out);

			if ( !NT_STATUS_IS_OK(status) ) {
				fprintf(stderr, "Unable to initialize %s.\n", backend_out);
				return 1;
			}
		}

		if (transfer_account_policies) {

			if (!(checkparms & BIT_USER))
				return export_account_policies(bin, bout);

		} else 	if (transfer_groups) {

			if (!(checkparms & BIT_USER))
				return export_groups(bin, bout);

		} else {
				return export_database(bin, bout, 
					(checkparms & BIT_USER) ? user_name : NULL );
		}
	}

	/* if BIT_USER is defined but nothing else then threat it as -l -u for compatibility */
	/* fake up BIT_LIST if only BIT_USER is defined */
	if ((checkparms & BIT_USER) && !(checkparms & ~BIT_USER)) {
		checkparms += BIT_LIST;
	}
	
	/* modify flag is optional to maintain backwards compatibility */
	/* fake up BIT_MODIFY if BIT_USER  and at least one of MASK_USER_GOOD is defined */
	if (!((checkparms & ~MASK_USER_GOOD) & ~BIT_USER) && (checkparms & MASK_USER_GOOD)) {
		checkparms += BIT_MODIFY;
	}

	/* list users operations */
	if (checkparms & BIT_LIST) {
		if (!(checkparms & ~BIT_LIST)) {
			return print_users_list (bdef, verbose, spstyle);
		}
		if (!(checkparms & ~(BIT_USER + BIT_LIST))) {
			return print_user_info (bdef, user_name, verbose, spstyle);
		}
	}
	
	/* mask out users options */
	checkparms &= ~MASK_USER_GOOD;

	/* if bad password count is reset, we must be modifying */
	if (checkparms & BIT_BADPWRESET) {
		checkparms |= BIT_MODIFY;
		checkparms &= ~BIT_BADPWRESET;
	}

	/* if logon hours is reset, must modify */
	if (checkparms & BIT_LOGONHOURS) {
		checkparms |= BIT_MODIFY;
		checkparms &= ~BIT_LOGONHOURS;
	}
	
	/* account operation */
	if ((checkparms & BIT_CREATE) || (checkparms & BIT_MODIFY) || (checkparms & BIT_DELETE)) {
		/* check use of -u option */
		if (!(checkparms & BIT_USER)) {
			fprintf (stderr, "Username not specified! (use -u option)\n");
			return -1;
		}

		/* account creation operations */
		if (!(checkparms & ~(BIT_CREATE + BIT_USER + BIT_MACHINE))) {
		       	if (checkparms & BIT_MACHINE) {
				return new_machine (bdef, user_name);
			} else {
				return new_user (bdef, user_name, full_name, home_dir, 
					home_drive, logon_script, profile_path, user_sid, pw_from_stdin);
			}
		}

		/* account deletion operations */
		if (!(checkparms & ~(BIT_DELETE + BIT_USER + BIT_MACHINE))) {
		       	if (checkparms & BIT_MACHINE) {
				return delete_machine_entry (bdef, user_name);
			} else {
				return delete_user_entry (bdef, user_name);
			}
		}

		/* account modification operations */
		if (!(checkparms & ~(BIT_MODIFY + BIT_USER))) {
			time_t pwd_can_change = -1;
			time_t pwd_must_change = -1;
			const char *errstr;

			if (pwd_can_change_time) {
				errstr = "can";
				if (pwd_time_format) {
					struct tm tm;
					char *ret;

					memset(&tm, 0, sizeof(struct tm));
					ret = strptime(pwd_can_change_time, pwd_time_format, &tm);
					if (ret == NULL || *ret != '\0') {
						goto error;
					}

					pwd_can_change = mktime(&tm);

					if (pwd_can_change == -1) {
						goto error;
					}
				} else { /* assume it is unix time */
					errno = 0;
					pwd_can_change = strtol(pwd_can_change_time, NULL, 10);
					if (errno) {
						goto error;
					}
				}	
			}
			if (pwd_must_change_time) {
				errstr = "must";
				if (pwd_time_format) {
					struct tm tm;
					char *ret;

					memset(&tm, 0, sizeof(struct tm));
					ret = strptime(pwd_must_change_time, pwd_time_format, &tm);
					if (ret == NULL || *ret != '\0') {
						goto error;
					}

					pwd_must_change = mktime(&tm);

					if (pwd_must_change == -1) {
						goto error;
					}
				} else { /* assume it is unix time */
					errno = 0;
					pwd_must_change = strtol(pwd_must_change_time, NULL, 10);
					if (errno) {
						goto error;
					}
				}	
			}
			return set_user_info (bdef, user_name, full_name, home_dir,
				acct_desc, home_drive, logon_script, profile_path, account_control,
				user_sid, user_domain, badpw_reset, hours_reset, pwd_can_change, 
				pwd_must_change);
error:
			fprintf (stderr, "Error parsing the time in pwd-%s-change-time!\n", errstr);
			return -1;
		}
	}

	if (setparms >= 0x20) {
		fprintf (stderr, "Incompatible or insufficient options on command line!\n");
	}
	poptPrintHelp(pc, stderr, 0);

	return 1;
}
