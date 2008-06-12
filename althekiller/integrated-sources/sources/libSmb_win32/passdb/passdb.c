/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Jeremy Allison 		1996-2001
   Copyright (C) Luke Kenneth Casson Leighton 	1996-1998
   Copyright (C) Gerald (Jerry) Carter		2000-2006
   Copyright (C) Andrew Bartlett		2001-2002
   Copyright (C) Simo Sorce			2003
   Copyright (C) Volker Lendecke 		2006
      
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

/******************************************************************
 get the default domain/netbios name to be used when 
 testing authentication.  For example, if you connect
 to a Windows member server using a bogus domain name, the
 Windows box will map the BOGUS\user to DOMAIN\user.  A 
 standalone box will map to WKS\user.
******************************************************************/

const char *my_sam_name(void)
{
	/* standalone servers can only use the local netbios name */
	if ( lp_server_role() == ROLE_STANDALONE )
		return global_myname();

	/* Windows domain members default to the DOMAIN
	   name when not specified */
	return lp_workgroup();
}

/**********************************************************************
***********************************************************************/

static int samu_destroy(void *p) 
{
	struct samu *user = p;

	data_blob_clear_free( &user->lm_pw );
	data_blob_clear_free( &user->nt_pw );

	if ( user->plaintext_pw )
		memset( user->plaintext_pw, 0x0, strlen(user->plaintext_pw) );

	return 0;
}

/**********************************************************************
 generate a new struct samuser
***********************************************************************/

struct samu *samu_new( TALLOC_CTX *ctx )
{
	struct samu *user;
	
	if ( !(user = TALLOC_ZERO_P( ctx, struct samu )) ) {
		DEBUG(0,("samuser_new: Talloc failed!\n"));
		return NULL;
	}
	
	talloc_set_destructor( user, samu_destroy );
	
	/* no initial methods */
	
	user->methods = NULL;

        /* Don't change these timestamp settings without a good reason.
           They are important for NT member server compatibility. */

	user->logon_time            = (time_t)0;
	user->pass_last_set_time    = (time_t)0;
	user->pass_can_change_time  = (time_t)0;
	user->logoff_time           = get_time_t_max();
	user->kickoff_time          = get_time_t_max();
	user->pass_must_change_time = get_time_t_max();
	user->fields_present        = 0x00ffffff;
	user->logon_divs = 168; 	/* hours per week */
	user->hours_len = 21; 		/* 21 times 8 bits = 168 */
	memset(user->hours, 0xff, user->hours_len); /* available at all hours */
	user->bad_password_count = 0;
	user->logon_count = 0;
	user->unknown_6 = 0x000004ec; /* don't know */

	/* Some parts of samba strlen their pdb_get...() returns, 
	   so this keeps the interface unchanged for now. */
	   
	user->username = "";
	user->domain = "";
	user->nt_username = "";
	user->full_name = "";
	user->home_dir = "";
	user->logon_script = "";
	user->profile_path = "";
	user->acct_desc = "";
	user->workstations = "";
	user->unknown_str = "";
	user->munged_dial = "";

	user->plaintext_pw = NULL;

	/* Unless we know otherwise have a Account Control Bit
	   value of 'normal user'.  This helps User Manager, which
	   asks for a filtered list of users. */

	user->acct_ctrl = ACB_NORMAL;
	
	
	return user;
}

/*********************************************************************
 Initialize a struct samu from a struct passwd including the user 
 and group SIDs.  The *user structure is filled out with the Unix
 attributes and a user SID.
*********************************************************************/

static NTSTATUS samu_set_unix_internal(struct samu *user, const struct passwd *pwd, BOOL create)
{
	const char *guest_account = lp_guestaccount();
	const char *domain = global_myname();
	uint32 urid;

	if ( !pwd ) {
		return NT_STATUS_NO_SUCH_USER;
	}

	/* Basic properties based upon the Unix account information */
	
	pdb_set_username(user, pwd->pw_name, PDB_SET);
	pdb_set_fullname(user, pwd->pw_gecos, PDB_SET);
	pdb_set_domain (user, get_global_sam_name(), PDB_DEFAULT);
#if 0
	/* This can lead to a primary group of S-1-22-2-XX which 
	   will be rejected by other parts of the Samba code. 
	   Rely on pdb_get_group_sid() to "Do The Right Thing" (TM)  
	   --jerry */
	   
	gid_to_sid(&group_sid, pwd->pw_gid);
	pdb_set_group_sid(user, &group_sid, PDB_SET);
#endif
	
	/* save the password structure for later use */
	
	user->unix_pw = tcopy_passwd( user, pwd );

	/* Special case for the guest account which must have a RID of 501 */
	
	if ( strequal( pwd->pw_name, guest_account ) ) {
		if ( !pdb_set_user_sid_from_rid(user, DOMAIN_USER_RID_GUEST, PDB_DEFAULT)) {
			return NT_STATUS_NO_SUCH_USER;
		}
		return NT_STATUS_OK;
	}
	
	/* Non-guest accounts...Check for a workstation or user account */

	if (pwd->pw_name[strlen(pwd->pw_name)-1] == '$') {
		/* workstation */
		
		if (!pdb_set_acct_ctrl(user, ACB_WSTRUST, PDB_DEFAULT)) {
			DEBUG(1, ("Failed to set 'workstation account' flags for user %s.\n", 
				pwd->pw_name));
			return NT_STATUS_INVALID_COMPUTER_NAME;
		}	
	} 
	else {
		/* user */
		
		if (!pdb_set_acct_ctrl(user, ACB_NORMAL, PDB_DEFAULT)) {
			DEBUG(1, ("Failed to set 'normal account' flags for user %s.\n", 
				pwd->pw_name));
			return NT_STATUS_INVALID_ACCOUNT_NAME;
		}
		
		/* set some basic attributes */
	
		pdb_set_profile_path(user, talloc_sub_specified(user, 
			lp_logon_path(), pwd->pw_name, domain, pwd->pw_uid, pwd->pw_gid), 
			PDB_DEFAULT);		
		pdb_set_homedir(user, talloc_sub_specified(user, 
			lp_logon_home(), pwd->pw_name, domain, pwd->pw_uid, pwd->pw_gid),
			PDB_DEFAULT);
		pdb_set_dir_drive(user, talloc_sub_specified(user, 
			lp_logon_drive(), pwd->pw_name, domain, pwd->pw_uid, pwd->pw_gid),
			PDB_DEFAULT);
		pdb_set_logon_script(user, talloc_sub_specified(user, 
			lp_logon_script(), pwd->pw_name, domain, pwd->pw_uid, pwd->pw_gid), 
			PDB_DEFAULT);
	}
	
	/* Now deal with the user SID.  If we have a backend that can generate 
	   RIDs, then do so.  But sometimes the caller just wanted a structure 
	   initialized and will fill in these fields later (such as from a 
	   NET_USER_INFO_3 structure) */

	if ( create && !pdb_rid_algorithm() ) {
		uint32 user_rid;
		DOM_SID user_sid;
		
		if ( !pdb_new_rid( &user_rid ) ) {
			DEBUG(3, ("Could not allocate a new RID\n"));
			return NT_STATUS_ACCESS_DENIED;
		}

		sid_copy( &user_sid, get_global_sam_sid() );
		sid_append_rid( &user_sid, user_rid );

		if ( !pdb_set_user_sid(user, &user_sid, PDB_SET) ) {
			DEBUG(3, ("pdb_set_user_sid failed\n"));
			return NT_STATUS_INTERNAL_ERROR;
		}
		
		return NT_STATUS_OK;
	}

	/* generate a SID for the user with the RID algorithm */
	
	urid = algorithmic_pdb_uid_to_user_rid( user->unix_pw->pw_uid );
		
	if ( !pdb_set_user_sid_from_rid( user, urid, PDB_SET) ) {
		return NT_STATUS_INTERNAL_ERROR;
	}
	
	return NT_STATUS_OK;
}

/********************************************************************
 Set the Unix user attributes
********************************************************************/

NTSTATUS samu_set_unix(struct samu *user, const struct passwd *pwd)
{
	return samu_set_unix_internal( user, pwd, False );
}

NTSTATUS samu_alloc_rid_unix(struct samu *user, const struct passwd *pwd)
{
	return samu_set_unix_internal( user, pwd, True );
}

/**********************************************************
 Encode the account control bits into a string.
 length = length of string to encode into (including terminating
 null). length *MUST BE MORE THAN 2* !
 **********************************************************/

char *pdb_encode_acct_ctrl(uint32 acct_ctrl, size_t length)
{
	static fstring acct_str;

	size_t i = 0;

	SMB_ASSERT(length <= sizeof(acct_str));

	acct_str[i++] = '[';

	if (acct_ctrl & ACB_PWNOTREQ ) acct_str[i++] = 'N';
	if (acct_ctrl & ACB_DISABLED ) acct_str[i++] = 'D';
	if (acct_ctrl & ACB_HOMDIRREQ) acct_str[i++] = 'H';
	if (acct_ctrl & ACB_TEMPDUP  ) acct_str[i++] = 'T'; 
	if (acct_ctrl & ACB_NORMAL   ) acct_str[i++] = 'U';
	if (acct_ctrl & ACB_MNS      ) acct_str[i++] = 'M';
	if (acct_ctrl & ACB_WSTRUST  ) acct_str[i++] = 'W';
	if (acct_ctrl & ACB_SVRTRUST ) acct_str[i++] = 'S';
	if (acct_ctrl & ACB_AUTOLOCK ) acct_str[i++] = 'L';
	if (acct_ctrl & ACB_PWNOEXP  ) acct_str[i++] = 'X';
	if (acct_ctrl & ACB_DOMTRUST ) acct_str[i++] = 'I';

	for ( ; i < length - 2 ; i++ )
		acct_str[i] = ' ';

	i = length - 2;
	acct_str[i++] = ']';
	acct_str[i++] = '\0';

	return acct_str;
}     

/**********************************************************
 Decode the account control bits from a string.
 **********************************************************/

uint32 pdb_decode_acct_ctrl(const char *p)
{
	uint32 acct_ctrl = 0;
	BOOL finished = False;

	/*
	 * Check if the account type bits have been encoded after the
	 * NT password (in the form [NDHTUWSLXI]).
	 */

	if (*p != '[')
		return 0;

	for (p++; *p && !finished; p++) {
		switch (*p) {
			case 'N': { acct_ctrl |= ACB_PWNOTREQ ; break; /* 'N'o password. */ }
			case 'D': { acct_ctrl |= ACB_DISABLED ; break; /* 'D'isabled. */ }
			case 'H': { acct_ctrl |= ACB_HOMDIRREQ; break; /* 'H'omedir required. */ }
			case 'T': { acct_ctrl |= ACB_TEMPDUP  ; break; /* 'T'emp account. */ } 
			case 'U': { acct_ctrl |= ACB_NORMAL   ; break; /* 'U'ser account (normal). */ } 
			case 'M': { acct_ctrl |= ACB_MNS      ; break; /* 'M'NS logon user account. What is this ? */ } 
			case 'W': { acct_ctrl |= ACB_WSTRUST  ; break; /* 'W'orkstation account. */ } 
			case 'S': { acct_ctrl |= ACB_SVRTRUST ; break; /* 'S'erver account. */ } 
			case 'L': { acct_ctrl |= ACB_AUTOLOCK ; break; /* 'L'ocked account. */ } 
			case 'X': { acct_ctrl |= ACB_PWNOEXP  ; break; /* No 'X'piry on password */ } 
			case 'I': { acct_ctrl |= ACB_DOMTRUST ; break; /* 'I'nterdomain trust account. */ }
            case ' ': { break; }
			case ':':
			case '\n':
			case '\0': 
			case ']':
			default:  { finished = True; }
		}
	}

	return acct_ctrl;
}

/*************************************************************
 Routine to set 32 hex password characters from a 16 byte array.
**************************************************************/

void pdb_sethexpwd(char *p, const unsigned char *pwd, uint32 acct_ctrl)
{
	if (pwd != NULL) {
		int i;
		for (i = 0; i < 16; i++)
			slprintf(&p[i*2], 3, "%02X", pwd[i]);
	} else {
		if (acct_ctrl & ACB_PWNOTREQ)
			safe_strcpy(p, "NO PASSWORDXXXXXXXXXXXXXXXXXXXXX", 33);
		else
			safe_strcpy(p, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", 33);
	}
}

/*************************************************************
 Routine to get the 32 hex characters and turn them
 into a 16 byte array.
**************************************************************/

BOOL pdb_gethexpwd(const char *p, unsigned char *pwd)
{
	int i;
	unsigned char   lonybble, hinybble;
	const char      *hexchars = "0123456789ABCDEF";
	char           *p1, *p2;
	
	if (!p)
		return (False);
	
	for (i = 0; i < 32; i += 2) {
		hinybble = toupper_ascii(p[i]);
		lonybble = toupper_ascii(p[i + 1]);

		p1 = strchr(hexchars, hinybble);
		p2 = strchr(hexchars, lonybble);

		if (!p1 || !p2)
			return (False);

		hinybble = PTR_DIFF(p1, hexchars);
		lonybble = PTR_DIFF(p2, hexchars);

		pwd[i / 2] = (hinybble << 4) | lonybble;
	}
	return (True);
}

/*************************************************************
 Routine to set 42 hex hours characters from a 21 byte array.
**************************************************************/

void pdb_sethexhours(char *p, const unsigned char *hours)
{
	if (hours != NULL) {
		int i;
		for (i = 0; i < 21; i++) {
			slprintf(&p[i*2], 3, "%02X", hours[i]);
		}
	} else {
		safe_strcpy(p, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 43);
	}
}

/*************************************************************
 Routine to get the 42 hex characters and turn them
 into a 21 byte array.
**************************************************************/

BOOL pdb_gethexhours(const char *p, unsigned char *hours)
{
	int i;
	unsigned char   lonybble, hinybble;
	const char      *hexchars = "0123456789ABCDEF";
	char           *p1, *p2;

	if (!p) {
		return (False);
	}

	for (i = 0; i < 42; i += 2) {
		hinybble = toupper_ascii(p[i]);
		lonybble = toupper_ascii(p[i + 1]);

		p1 = strchr(hexchars, hinybble);
		p2 = strchr(hexchars, lonybble);

		if (!p1 || !p2) {
			return (False);
		}

		hinybble = PTR_DIFF(p1, hexchars);
		lonybble = PTR_DIFF(p2, hexchars);

		hours[i / 2] = (hinybble << 4) | lonybble;
	}
	return (True);
}

/********************************************************************
********************************************************************/

int algorithmic_rid_base(void)
{
	static int rid_offset = 0;

	if (rid_offset != 0)
		return rid_offset;

	rid_offset = lp_algorithmic_rid_base();

	if (rid_offset < BASE_RID) {  
		/* Try to prevent admin foot-shooting, we can't put algorithmic
		   rids below 1000, that's the 'well known RIDs' on NT */
		DEBUG(0, ("'algorithmic rid base' must be equal to or above %ld\n", BASE_RID));
		rid_offset = BASE_RID;
	}
	if (rid_offset & 1) {
		DEBUG(0, ("algorithmic rid base must be even\n"));
		rid_offset += 1;
	}
	return rid_offset;
}

/*******************************************************************
 Converts NT user RID to a UNIX uid.
 ********************************************************************/

uid_t algorithmic_pdb_user_rid_to_uid(uint32 user_rid)
{
	int rid_offset = algorithmic_rid_base();
	return (uid_t)(((user_rid & (~USER_RID_TYPE)) - rid_offset)/RID_MULTIPLIER);
}

uid_t max_algorithmic_uid(void)
{
	return algorithmic_pdb_user_rid_to_uid(0xfffffffe);
}

/*******************************************************************
 converts UNIX uid to an NT User RID.
 ********************************************************************/

uint32 algorithmic_pdb_uid_to_user_rid(uid_t uid)
{
	int rid_offset = algorithmic_rid_base();
	return (((((uint32)uid)*RID_MULTIPLIER) + rid_offset) | USER_RID_TYPE);
}

/*******************************************************************
 Converts NT group RID to a UNIX gid.
 ********************************************************************/

gid_t pdb_group_rid_to_gid(uint32 group_rid)
{
	int rid_offset = algorithmic_rid_base();
	return (gid_t)(((group_rid & (~GROUP_RID_TYPE))- rid_offset)/RID_MULTIPLIER);
}

gid_t max_algorithmic_gid(void)
{
	return pdb_group_rid_to_gid(0xffffffff);
}

/*******************************************************************
 converts NT Group RID to a UNIX uid.
 
 warning: you must not call that function only
 you must do a call to the group mapping first.
 there is not anymore a direct link between the gid and the rid.
 ********************************************************************/

uint32 algorithmic_pdb_gid_to_group_rid(gid_t gid)
{
	int rid_offset = algorithmic_rid_base();
	return (((((uint32)gid)*RID_MULTIPLIER) + rid_offset) | GROUP_RID_TYPE);
}

/*******************************************************************
 Decides if a RID is a well known RID.
 ********************************************************************/

static BOOL rid_is_well_known(uint32 rid)
{
	/* Not using rid_offset here, because this is the actual
	   NT fixed value (1000) */

	return (rid < BASE_RID);
}

/*******************************************************************
 Decides if a RID is a user or group RID.
 ********************************************************************/

BOOL algorithmic_pdb_rid_is_user(uint32 rid)
{
	if ( rid_is_well_known(rid) ) {
		/*
		 * The only well known user RIDs are DOMAIN_USER_RID_ADMIN
		 * and DOMAIN_USER_RID_GUEST.
		 */
		if(rid == DOMAIN_USER_RID_ADMIN || rid == DOMAIN_USER_RID_GUEST)
			return True;
	} else if((rid & RID_TYPE_MASK) == USER_RID_TYPE) {
		return True;
	}
	return False;
}

/*******************************************************************
 Convert a name into a SID. Used in the lookup name rpc.
 ********************************************************************/

BOOL lookup_global_sam_name(const char *user, int flags, uint32_t *rid,
			    enum SID_NAME_USE *type)
{
	GROUP_MAP map;
	BOOL ret;
	
	/* Windows treats "MACHINE\None" as a special name for 
	   rid 513 on non-DCs.  You cannot create a user or group
	   name "None" on Windows.  You will get an error that 
	   the group already exists. */
	   
	if ( strequal( user, "None" ) ) {
		*rid = DOMAIN_GROUP_RID_USERS;
		*type = SID_NAME_DOM_GRP;
		
		return True;
	}

	/* LOOKUP_NAME_GROUP is a hack to allow valid users = @foo to work
	 * correctly in the case where foo also exists as a user. If the flag
	 * is set, don't look for users at all. */

	if ((flags & LOOKUP_NAME_GROUP) == 0) {
		struct samu *sam_account = NULL;
		DOM_SID user_sid;

		if ( !(sam_account = samu_new( NULL )) ) {
			return False;
		}
	
		become_root();
		ret =  pdb_getsampwnam(sam_account, user);
		unbecome_root();

		if (ret) {
			sid_copy(&user_sid, pdb_get_user_sid(sam_account));
		}
		
		TALLOC_FREE(sam_account);

		if (ret) {
			if (!sid_check_is_in_our_domain(&user_sid)) {
				DEBUG(0, ("User %s with invalid SID %s in passdb\n",
					  user, sid_string_static(&user_sid)));
				return False;
			}

			sid_peek_rid(&user_sid, rid);
			*type = SID_NAME_USER;
			return True;
		}
	}

	/*
	 * Maybe it is a group ?
	 */

	become_root();
	ret = pdb_getgrnam(&map, user);
	unbecome_root();

 	if (!ret) {
		return False;
	}

	/* BUILTIN groups are looked up elsewhere */
	if (!sid_check_is_in_our_domain(&map.sid)) {
		DEBUG(10, ("Found group %s (%s) not in our domain -- "
			   "ignoring.", user,
			   sid_string_static(&map.sid)));
		return False;
	}

	/* yes it's a mapped group */
	sid_peek_rid(&map.sid, rid);
	*type = map.sid_name_use;
	return True;
}

/*************************************************************
 Change a password entry in the local smbpasswd file.
 *************************************************************/

NTSTATUS local_password_change(const char *user_name, int local_flags,
			   const char *new_passwd, 
			   char *err_str, size_t err_str_len,
			   char *msg_str, size_t msg_str_len)
{
	struct samu *sam_pass=NULL;
	uint32 other_acb;
	NTSTATUS result;

	*err_str = '\0';
	*msg_str = '\0';

	/* Get the smb passwd entry for this user */

	if ( !(sam_pass = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	become_root();
	if(!pdb_getsampwnam(sam_pass, user_name)) {
		unbecome_root();
		TALLOC_FREE(sam_pass);
		
		if ((local_flags & LOCAL_ADD_USER) || (local_flags & LOCAL_DELETE_USER)) {
			int tmp_debug = DEBUGLEVEL;
			struct passwd *pwd;

			/* Might not exist in /etc/passwd. */

			if (tmp_debug < 1) {
				DEBUGLEVEL = 1;
			}

			if ( !(pwd = getpwnam_alloc( NULL, user_name)) ) {
				return NT_STATUS_NO_SUCH_USER;
			}

			/* create the struct samu and initialize the basic Unix properties */

			if ( !(sam_pass = samu_new( NULL )) ) {
				return NT_STATUS_NO_MEMORY;
			}

			result = samu_set_unix( sam_pass, pwd );

			DEBUGLEVEL = tmp_debug;

			TALLOC_FREE( pwd );

			if (NT_STATUS_EQUAL(result, NT_STATUS_INVALID_PRIMARY_GROUP)) {
				return result;
			}

			if (!NT_STATUS_IS_OK(result)) {
				slprintf(err_str, err_str_len-1, "Failed to " "initialize account for user %s: %s\n",
					user_name, nt_errstr(result));
				return result;
			}
		} else {
			slprintf(err_str, err_str_len-1,"Failed to find entry for user %s.\n", user_name);
			return NT_STATUS_NO_SUCH_USER;
		}
	} else {
		unbecome_root();
		/* the entry already existed */
		local_flags &= ~LOCAL_ADD_USER;
	}

	/* the 'other' acb bits not being changed here */
	other_acb =  (pdb_get_acct_ctrl(sam_pass) & (!(ACB_WSTRUST|ACB_DOMTRUST|ACB_SVRTRUST|ACB_NORMAL)));
	if (local_flags & LOCAL_TRUST_ACCOUNT) {
		if (!pdb_set_acct_ctrl(sam_pass, ACB_WSTRUST | other_acb, PDB_CHANGED) ) {
			slprintf(err_str, err_str_len - 1, "Failed to set 'trusted workstation account' flags for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	} else if (local_flags & LOCAL_INTERDOM_ACCOUNT) {
		if (!pdb_set_acct_ctrl(sam_pass, ACB_DOMTRUST | other_acb, PDB_CHANGED)) {
			slprintf(err_str, err_str_len - 1, "Failed to set 'domain trust account' flags for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	} else {
		if (!pdb_set_acct_ctrl(sam_pass, ACB_NORMAL | other_acb, PDB_CHANGED)) {
			slprintf(err_str, err_str_len - 1, "Failed to set 'normal account' flags for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}

	/*
	 * We are root - just write the new password
	 * and the valid last change time.
	 */

	if (local_flags & LOCAL_DISABLE_USER) {
		if (!pdb_set_acct_ctrl (sam_pass, pdb_get_acct_ctrl(sam_pass)|ACB_DISABLED, PDB_CHANGED)) {
			slprintf(err_str, err_str_len-1, "Failed to set 'disabled' flag for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	} else if (local_flags & LOCAL_ENABLE_USER) {
		if (!pdb_set_acct_ctrl (sam_pass, pdb_get_acct_ctrl(sam_pass)&(~ACB_DISABLED), PDB_CHANGED)) {
			slprintf(err_str, err_str_len-1, "Failed to unset 'disabled' flag for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}
	
	if (local_flags & LOCAL_SET_NO_PASSWORD) {
		if (!pdb_set_acct_ctrl (sam_pass, pdb_get_acct_ctrl(sam_pass)|ACB_PWNOTREQ, PDB_CHANGED)) {
			slprintf(err_str, err_str_len-1, "Failed to set 'no password required' flag for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	} else if (local_flags & LOCAL_SET_PASSWORD) {
		/*
		 * If we're dealing with setting a completely empty user account
		 * ie. One with a password of 'XXXX', but not set disabled (like
		 * an account created from scratch) then if the old password was
		 * 'XX's then getsmbpwent will have set the ACB_DISABLED flag.
		 * We remove that as we're giving this user their first password
		 * and the decision hasn't really been made to disable them (ie.
		 * don't create them disabled). JRA.
		 */
		if ((pdb_get_lanman_passwd(sam_pass)==NULL) && (pdb_get_acct_ctrl(sam_pass)&ACB_DISABLED)) {
			if (!pdb_set_acct_ctrl (sam_pass, pdb_get_acct_ctrl(sam_pass)&(~ACB_DISABLED), PDB_CHANGED)) {
				slprintf(err_str, err_str_len-1, "Failed to unset 'disabled' flag for user %s.\n", user_name);
				TALLOC_FREE(sam_pass);
				return NT_STATUS_UNSUCCESSFUL;
			}
		}
		if (!pdb_set_acct_ctrl (sam_pass, pdb_get_acct_ctrl(sam_pass)&(~ACB_PWNOTREQ), PDB_CHANGED)) {
			slprintf(err_str, err_str_len-1, "Failed to unset 'no password required' flag for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
		
		if (!pdb_set_plaintext_passwd (sam_pass, new_passwd)) {
			slprintf(err_str, err_str_len-1, "Failed to set password for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	}	

	if (local_flags & LOCAL_ADD_USER) {
		if (NT_STATUS_IS_OK(pdb_add_sam_account(sam_pass))) {
			slprintf(msg_str, msg_str_len-1, "Added user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_OK;
		} else {
			slprintf(err_str, err_str_len-1, "Failed to add entry for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
	} else if (local_flags & LOCAL_DELETE_USER) {
		if (!NT_STATUS_IS_OK(pdb_delete_sam_account(sam_pass))) {
			slprintf(err_str,err_str_len-1, "Failed to delete entry for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return NT_STATUS_UNSUCCESSFUL;
		}
		slprintf(msg_str, msg_str_len-1, "Deleted user %s.\n", user_name);
	} else {
		result = pdb_update_sam_account(sam_pass);
		if(!NT_STATUS_IS_OK(result)) {
			slprintf(err_str, err_str_len-1, "Failed to modify entry for user %s.\n", user_name);
			TALLOC_FREE(sam_pass);
			return result;
		}
		if(local_flags & LOCAL_DISABLE_USER)
			slprintf(msg_str, msg_str_len-1, "Disabled user %s.\n", user_name);
		else if (local_flags & LOCAL_ENABLE_USER)
			slprintf(msg_str, msg_str_len-1, "Enabled user %s.\n", user_name);
		else if (local_flags & LOCAL_SET_NO_PASSWORD)
			slprintf(msg_str, msg_str_len-1, "User %s password set to none.\n", user_name);
	}

	TALLOC_FREE(sam_pass);
	return NT_STATUS_OK;
}

/**********************************************************************
 Marshall/unmarshall struct samu structs.
 *********************************************************************/

#define TDB_FORMAT_STRING_V3       "dddddddBBBBBBBBBBBBddBBBdwdBwwd"

/*********************************************************************
*********************************************************************/

BOOL init_sam_from_buffer_v3(struct samu *sampass, uint8 *buf, uint32 buflen)
{

	/* times are stored as 32bit integer
	   take care on system with 64bit wide time_t
	   --SSS */
	uint32	logon_time,
		logoff_time,
		kickoff_time,
		bad_password_time,
		pass_last_set_time,
		pass_can_change_time,
		pass_must_change_time;
	char *username = NULL;
	char *domain = NULL;
	char *nt_username = NULL;
	char *dir_drive = NULL;
	char *unknown_str = NULL;
	char *munged_dial = NULL;
	char *fullname = NULL;
	char *homedir = NULL;
	char *logon_script = NULL;
	char *profile_path = NULL;
	char *acct_desc = NULL;
	char *workstations = NULL;
	uint32	username_len, domain_len, nt_username_len,
		dir_drive_len, unknown_str_len, munged_dial_len,
		fullname_len, homedir_len, logon_script_len,
		profile_path_len, acct_desc_len, workstations_len;
		
	uint32	user_rid, group_rid, hours_len, unknown_6, acct_ctrl;
	uint16  logon_divs;
	uint16	bad_password_count, logon_count;
	uint8	*hours = NULL;
	uint8	*lm_pw_ptr = NULL, *nt_pw_ptr = NULL, *nt_pw_hist_ptr = NULL;
	uint32		len = 0;
	uint32		lm_pw_len, nt_pw_len, nt_pw_hist_len, hourslen;
	uint32 pwHistLen = 0;
	BOOL ret = True;
	fstring tmpstring;
	BOOL expand_explicit = lp_passdb_expand_explicit();
	
	if(sampass == NULL || buf == NULL) {
		DEBUG(0, ("init_sam_from_buffer_v3: NULL parameters found!\n"));
		return False;
	}
									
/* TDB_FORMAT_STRING_V3       "dddddddBBBBBBBBBBBBddBBBdwdBwwd" */

	/* unpack the buffer into variables */
	len = tdb_unpack ((char *)buf, buflen, TDB_FORMAT_STRING_V3,
		&logon_time,						/* d */
		&logoff_time,						/* d */
		&kickoff_time,						/* d */
		&bad_password_time,					/* d */
		&pass_last_set_time,					/* d */
		&pass_can_change_time,					/* d */
		&pass_must_change_time,					/* d */
		&username_len, &username,				/* B */
		&domain_len, &domain,					/* B */
		&nt_username_len, &nt_username,				/* B */
		&fullname_len, &fullname,				/* B */
		&homedir_len, &homedir,					/* B */
		&dir_drive_len, &dir_drive,				/* B */
		&logon_script_len, &logon_script,			/* B */
		&profile_path_len, &profile_path,			/* B */
		&acct_desc_len, &acct_desc,				/* B */
		&workstations_len, &workstations,			/* B */
		&unknown_str_len, &unknown_str,				/* B */
		&munged_dial_len, &munged_dial,				/* B */
		&user_rid,						/* d */
		&group_rid,						/* d */
		&lm_pw_len, &lm_pw_ptr,					/* B */
		&nt_pw_len, &nt_pw_ptr,					/* B */
		/* Change from V1 is addition of password history field. */
		&nt_pw_hist_len, &nt_pw_hist_ptr,			/* B */
		/* Change from V2 is the uint32 acb_mask */
		&acct_ctrl,						/* d */
		/* Also "remove_me" field was removed. */
		&logon_divs,						/* w */
		&hours_len,						/* d */
		&hourslen, &hours,					/* B */
		&bad_password_count,					/* w */
		&logon_count,						/* w */
		&unknown_6);						/* d */
		
	if (len == (uint32) -1)  {
		ret = False;
		goto done;
	}

	pdb_set_logon_time(sampass, logon_time, PDB_SET);
	pdb_set_logoff_time(sampass, logoff_time, PDB_SET);
	pdb_set_kickoff_time(sampass, kickoff_time, PDB_SET);
	pdb_set_bad_password_time(sampass, bad_password_time, PDB_SET);
	pdb_set_pass_can_change_time(sampass, pass_can_change_time, PDB_SET);
	pdb_set_pass_must_change_time(sampass, pass_must_change_time, PDB_SET);
	pdb_set_pass_last_set_time(sampass, pass_last_set_time, PDB_SET);

	pdb_set_username(sampass, username, PDB_SET); 
	pdb_set_domain(sampass, domain, PDB_SET);
	pdb_set_nt_username(sampass, nt_username, PDB_SET);
	pdb_set_fullname(sampass, fullname, PDB_SET);

	if (homedir) {
		fstrcpy( tmpstring, homedir );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_homedir(sampass, tmpstring, PDB_SET);
	}
	else {
		pdb_set_homedir(sampass, 
			talloc_sub_basic(sampass, username, lp_logon_home()),
			PDB_DEFAULT);
	}

	if (dir_drive) 	
		pdb_set_dir_drive(sampass, dir_drive, PDB_SET);
	else
		pdb_set_dir_drive(sampass, lp_logon_drive(), PDB_DEFAULT );

	if (logon_script) {
		fstrcpy( tmpstring, logon_script );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_logon_script(sampass, tmpstring, PDB_SET);
	}
	else {
		pdb_set_logon_script(sampass, 
			talloc_sub_basic(sampass, username, lp_logon_script()),
			PDB_DEFAULT);
	}
	
	if (profile_path) {	
		fstrcpy( tmpstring, profile_path );
		if (expand_explicit) {
			standard_sub_basic( username, tmpstring,
					    sizeof(tmpstring) );
		}
		pdb_set_profile_path(sampass, tmpstring, PDB_SET);
	} 
	else {
		pdb_set_profile_path(sampass, 
			talloc_sub_basic(sampass, username, lp_logon_path()),
			PDB_DEFAULT);
	}

	pdb_set_acct_desc(sampass, acct_desc, PDB_SET);
	pdb_set_workstations(sampass, workstations, PDB_SET);
	pdb_set_munged_dial(sampass, munged_dial, PDB_SET);

	if (lm_pw_ptr && lm_pw_len == LM_HASH_LEN) {
		if (!pdb_set_lanman_passwd(sampass, lm_pw_ptr, PDB_SET)) {
			ret = False;
			goto done;
		}
	}

	if (nt_pw_ptr && nt_pw_len == NT_HASH_LEN) {
		if (!pdb_set_nt_passwd(sampass, nt_pw_ptr, PDB_SET)) {
			ret = False;
			goto done;
		}
	}

	pdb_get_account_policy(AP_PASSWORD_HISTORY, &pwHistLen);
	if (pwHistLen) {
		uint8 *pw_hist = SMB_MALLOC(pwHistLen * PW_HISTORY_ENTRY_LEN);
		if (!pw_hist) {
			ret = False;
			goto done;
		}
		memset(pw_hist, '\0', pwHistLen * PW_HISTORY_ENTRY_LEN);
		if (nt_pw_hist_ptr && nt_pw_hist_len) {
			int i;
			SMB_ASSERT((nt_pw_hist_len % PW_HISTORY_ENTRY_LEN) == 0);
			nt_pw_hist_len /= PW_HISTORY_ENTRY_LEN;
			for (i = 0; (i < pwHistLen) && (i < nt_pw_hist_len); i++) {
				memcpy(&pw_hist[i*PW_HISTORY_ENTRY_LEN],
					&nt_pw_hist_ptr[i*PW_HISTORY_ENTRY_LEN],
					PW_HISTORY_ENTRY_LEN);
			}
		}
		if (!pdb_set_pw_history(sampass, pw_hist, pwHistLen, PDB_SET)) {
			SAFE_FREE(pw_hist);
			ret = False;
			goto done;
		}
		SAFE_FREE(pw_hist);
	} else {
		pdb_set_pw_history(sampass, NULL, 0, PDB_SET);
	}

	pdb_set_user_sid_from_rid(sampass, user_rid, PDB_SET);
	pdb_set_group_sid_from_rid(sampass, group_rid, PDB_SET);
	pdb_set_hours_len(sampass, hours_len, PDB_SET);
	pdb_set_bad_password_count(sampass, bad_password_count, PDB_SET);
	pdb_set_logon_count(sampass, logon_count, PDB_SET);
	pdb_set_unknown_6(sampass, unknown_6, PDB_SET);
	/* Change from V2 is the uint32 acct_ctrl */
	pdb_set_acct_ctrl(sampass, acct_ctrl, PDB_SET);
	pdb_set_logon_divs(sampass, logon_divs, PDB_SET);
	pdb_set_hours(sampass, hours, PDB_SET);

done:

	SAFE_FREE(username);
	SAFE_FREE(domain);
	SAFE_FREE(nt_username);
	SAFE_FREE(fullname);
	SAFE_FREE(homedir);
	SAFE_FREE(dir_drive);
	SAFE_FREE(logon_script);
	SAFE_FREE(profile_path);
	SAFE_FREE(acct_desc);
	SAFE_FREE(workstations);
	SAFE_FREE(munged_dial);
	SAFE_FREE(unknown_str);
	SAFE_FREE(lm_pw_ptr);
	SAFE_FREE(nt_pw_ptr);
	SAFE_FREE(nt_pw_hist_ptr);
	SAFE_FREE(hours);

	return ret;
}

/*********************************************************************
*********************************************************************/

uint32 init_buffer_from_sam_v3 (uint8 **buf, struct samu *sampass, BOOL size_only)
{
	size_t len, buflen;

	/* times are stored as 32bit integer
	   take care on system with 64bit wide time_t
	   --SSS */
	uint32	logon_time,
		logoff_time,
		kickoff_time,
		bad_password_time,
		pass_last_set_time,
		pass_can_change_time,
		pass_must_change_time;

	uint32  user_rid, group_rid;

	const char *username;
	const char *domain;
	const char *nt_username;
	const char *dir_drive;
	const char *unknown_str;
	const char *munged_dial;
	const char *fullname;
	const char *homedir;
	const char *logon_script;
	const char *profile_path;
	const char *acct_desc;
	const char *workstations;
	uint32	username_len, domain_len, nt_username_len,
		dir_drive_len, unknown_str_len, munged_dial_len,
		fullname_len, homedir_len, logon_script_len,
		profile_path_len, acct_desc_len, workstations_len;

	const uint8 *lm_pw;
	const uint8 *nt_pw;
	const uint8 *nt_pw_hist;
	uint32	lm_pw_len = 16;
	uint32	nt_pw_len = 16;
	uint32  nt_pw_hist_len;
	uint32 pwHistLen = 0;

	*buf = NULL;
	buflen = 0;

	logon_time = (uint32)pdb_get_logon_time(sampass);
	logoff_time = (uint32)pdb_get_logoff_time(sampass);
	kickoff_time = (uint32)pdb_get_kickoff_time(sampass);
	bad_password_time = (uint32)pdb_get_bad_password_time(sampass);
	pass_can_change_time = (uint32)pdb_get_pass_can_change_time(sampass);
	pass_must_change_time = (uint32)pdb_get_pass_must_change_time(sampass);
	pass_last_set_time = (uint32)pdb_get_pass_last_set_time(sampass);

	user_rid = pdb_get_user_rid(sampass);
	group_rid = pdb_get_group_rid(sampass);

	username = pdb_get_username(sampass);
	if (username) {
		username_len = strlen(username) +1;
	} else {
		username_len = 0;
	}

	domain = pdb_get_domain(sampass);
	if (domain) {
		domain_len = strlen(domain) +1;
	} else {
		domain_len = 0;
	}

	nt_username = pdb_get_nt_username(sampass);
	if (nt_username) {
		nt_username_len = strlen(nt_username) +1;
	} else {
		nt_username_len = 0;
	}

	fullname = pdb_get_fullname(sampass);
	if (fullname) {
		fullname_len = strlen(fullname) +1;
	} else {
		fullname_len = 0;
	}

	/*
	 * Only updates fields which have been set (not defaults from smb.conf)
	 */

	if (!IS_SAM_DEFAULT(sampass, PDB_DRIVE)) {
		dir_drive = pdb_get_dir_drive(sampass);
	} else {
		dir_drive = NULL;
	}
	if (dir_drive) {
		dir_drive_len = strlen(dir_drive) +1;
	} else {
		dir_drive_len = 0;
	}

	if (!IS_SAM_DEFAULT(sampass, PDB_SMBHOME)) {
		homedir = pdb_get_homedir(sampass);
	} else {
		homedir = NULL;
	}
	if (homedir) {
		homedir_len = strlen(homedir) +1;
	} else {
		homedir_len = 0;
	}

	if (!IS_SAM_DEFAULT(sampass, PDB_LOGONSCRIPT)) {
		logon_script = pdb_get_logon_script(sampass);
	} else {
		logon_script = NULL;
	}
	if (logon_script) {
		logon_script_len = strlen(logon_script) +1;
	} else {
		logon_script_len = 0;
	}

	if (!IS_SAM_DEFAULT(sampass, PDB_PROFILE)) {
		profile_path = pdb_get_profile_path(sampass);
	} else {
		profile_path = NULL;
	}
	if (profile_path) {
		profile_path_len = strlen(profile_path) +1;
	} else {
		profile_path_len = 0;
	}
	
	lm_pw = pdb_get_lanman_passwd(sampass);
	if (!lm_pw) {
		lm_pw_len = 0;
	}
	
	nt_pw = pdb_get_nt_passwd(sampass);
	if (!nt_pw) {
		nt_pw_len = 0;
	}

	pdb_get_account_policy(AP_PASSWORD_HISTORY, &pwHistLen);
	nt_pw_hist =  pdb_get_pw_history(sampass, &nt_pw_hist_len);
	if (pwHistLen && nt_pw_hist && nt_pw_hist_len) {
		nt_pw_hist_len *= PW_HISTORY_ENTRY_LEN;
	} else {
		nt_pw_hist_len = 0;
	}

	acct_desc = pdb_get_acct_desc(sampass);
	if (acct_desc) {
		acct_desc_len = strlen(acct_desc) +1;
	} else {
		acct_desc_len = 0;
	}

	workstations = pdb_get_workstations(sampass);
	if (workstations) {
		workstations_len = strlen(workstations) +1;
	} else {
		workstations_len = 0;
	}

	unknown_str = NULL;
	unknown_str_len = 0;

	munged_dial = pdb_get_munged_dial(sampass);
	if (munged_dial) {
		munged_dial_len = strlen(munged_dial) +1;
	} else {
		munged_dial_len = 0;	
	}

/* TDB_FORMAT_STRING_V3       "dddddddBBBBBBBBBBBBddBBBdwdBwwd" */

	/* one time to get the size needed */
	len = tdb_pack(NULL, 0,  TDB_FORMAT_STRING_V3,
		logon_time,				/* d */
		logoff_time,				/* d */
		kickoff_time,				/* d */
		bad_password_time,			/* d */
		pass_last_set_time,			/* d */
		pass_can_change_time,			/* d */
		pass_must_change_time,			/* d */
		username_len, username,			/* B */
		domain_len, domain,			/* B */
		nt_username_len, nt_username,		/* B */
		fullname_len, fullname,			/* B */
		homedir_len, homedir,			/* B */
		dir_drive_len, dir_drive,		/* B */
		logon_script_len, logon_script,		/* B */
		profile_path_len, profile_path,		/* B */
		acct_desc_len, acct_desc,		/* B */
		workstations_len, workstations,		/* B */
		unknown_str_len, unknown_str,		/* B */
		munged_dial_len, munged_dial,		/* B */
		user_rid,				/* d */
		group_rid,				/* d */
		lm_pw_len, lm_pw,			/* B */
		nt_pw_len, nt_pw,			/* B */
		nt_pw_hist_len, nt_pw_hist,		/* B */
		pdb_get_acct_ctrl(sampass),		/* d */
		pdb_get_logon_divs(sampass),		/* w */
		pdb_get_hours_len(sampass),		/* d */
		MAX_HOURS_LEN, pdb_get_hours(sampass),	/* B */
		pdb_get_bad_password_count(sampass),	/* w */
		pdb_get_logon_count(sampass),		/* w */
		pdb_get_unknown_6(sampass));		/* d */

	if (size_only) {
		return buflen;
	}

	/* malloc the space needed */
	if ( (*buf=(uint8*)SMB_MALLOC(len)) == NULL) {
		DEBUG(0,("init_buffer_from_sam_v3: Unable to malloc() memory for buffer!\n"));
		return (-1);
	}
	
	/* now for the real call to tdb_pack() */
	buflen = tdb_pack((char *)*buf, len,  TDB_FORMAT_STRING_V3,
		logon_time,				/* d */
		logoff_time,				/* d */
		kickoff_time,				/* d */
		bad_password_time,			/* d */
		pass_last_set_time,			/* d */
		pass_can_change_time,			/* d */
		pass_must_change_time,			/* d */
		username_len, username,			/* B */
		domain_len, domain,			/* B */
		nt_username_len, nt_username,		/* B */
		fullname_len, fullname,			/* B */
		homedir_len, homedir,			/* B */
		dir_drive_len, dir_drive,		/* B */
		logon_script_len, logon_script,		/* B */
		profile_path_len, profile_path,		/* B */
		acct_desc_len, acct_desc,		/* B */
		workstations_len, workstations,		/* B */
		unknown_str_len, unknown_str,		/* B */
		munged_dial_len, munged_dial,		/* B */
		user_rid,				/* d */
		group_rid,				/* d */
		lm_pw_len, lm_pw,			/* B */
		nt_pw_len, nt_pw,			/* B */
		nt_pw_hist_len, nt_pw_hist,		/* B */
		pdb_get_acct_ctrl(sampass),		/* d */
		pdb_get_logon_divs(sampass),		/* w */
		pdb_get_hours_len(sampass),		/* d */
		MAX_HOURS_LEN, pdb_get_hours(sampass),	/* B */
		pdb_get_bad_password_count(sampass),	/* w */
		pdb_get_logon_count(sampass),		/* w */
		pdb_get_unknown_6(sampass));		/* d */
	
	/* check to make sure we got it correct */
	if (buflen != len) {
		DEBUG(0, ("init_buffer_from_sam_v3: somthing odd is going on here: bufflen (%lu) != len (%lu) in tdb_pack operations!\n", 
			  (unsigned long)buflen, (unsigned long)len));  
		/* error */
		SAFE_FREE (*buf);
		return (-1);
	}

	return (buflen);
}


/*********************************************************************
*********************************************************************/

BOOL pdb_copy_sam_account(struct samu *dst, struct samu *src )
{
	uint8 *buf = NULL;
	int len;

	len = init_buffer_from_sam_v3(&buf, src, False);
	if (len == -1 || !buf) {
		SAFE_FREE(buf);
		return False;
	}

	if (!init_sam_from_buffer_v3( dst, buf, len )) {
		free(buf);
		return False;
	}

	dst->methods = src->methods;
	
	if ( src->unix_pw ) {
		dst->unix_pw = tcopy_passwd( dst, src->unix_pw );
		if (!dst->unix_pw) {
			free(buf);
			return False;
		}
	}

	free(buf);
	return True;
}

/*********************************************************************
 Update the bad password count checking the AP_RESET_COUNT_TIME 
*********************************************************************/

BOOL pdb_update_bad_password_count(struct samu *sampass, BOOL *updated)
{
	time_t LastBadPassword;
	uint16 BadPasswordCount;
	uint32 resettime; 

	BadPasswordCount = pdb_get_bad_password_count(sampass);
	if (!BadPasswordCount) {
		DEBUG(9, ("No bad password attempts.\n"));
		return True;
	}

	if (!pdb_get_account_policy(AP_RESET_COUNT_TIME, &resettime)) {
		DEBUG(0, ("pdb_update_bad_password_count: pdb_get_account_policy failed.\n"));
		return False;
	}

	/* First, check if there is a reset time to compare */
	if ((resettime == (uint32) -1) || (resettime == 0)) {
		DEBUG(9, ("No reset time, can't reset bad pw count\n"));
		return True;
	}

	LastBadPassword = pdb_get_bad_password_time(sampass);
	DEBUG(7, ("LastBadPassword=%d, resettime=%d, current time=%d.\n", 
		   (uint32) LastBadPassword, resettime, (uint32)time(NULL)));
	if (time(NULL) > (LastBadPassword + (time_t)resettime*60)){
		pdb_set_bad_password_count(sampass, 0, PDB_CHANGED);
		pdb_set_bad_password_time(sampass, 0, PDB_CHANGED);
		if (updated) {
			*updated = True;
		}
	}

	return True;
}

/*********************************************************************
 Update the ACB_AUTOLOCK flag checking the AP_LOCK_ACCOUNT_DURATION 
*********************************************************************/

BOOL pdb_update_autolock_flag(struct samu *sampass, BOOL *updated)
{
	uint32 duration;
	time_t LastBadPassword;

	if (!(pdb_get_acct_ctrl(sampass) & ACB_AUTOLOCK)) {
		DEBUG(9, ("pdb_update_autolock_flag: Account %s not autolocked, no check needed\n",
			pdb_get_username(sampass)));
		return True;
	}

	if (!pdb_get_account_policy(AP_LOCK_ACCOUNT_DURATION, &duration)) {
		DEBUG(0, ("pdb_update_autolock_flag: pdb_get_account_policy failed.\n"));
		return False;
	}

	/* First, check if there is a duration to compare */
	if ((duration == (uint32) -1)  || (duration == 0)) {
		DEBUG(9, ("pdb_update_autolock_flag: No reset duration, can't reset autolock\n"));
		return True;
	}
		      
	LastBadPassword = pdb_get_bad_password_time(sampass);
	DEBUG(7, ("pdb_update_autolock_flag: Account %s, LastBadPassword=%d, duration=%d, current time =%d.\n",
		  pdb_get_username(sampass), (uint32)LastBadPassword, duration*60, (uint32)time(NULL)));

	if (LastBadPassword == (time_t)0) {
		DEBUG(1,("pdb_update_autolock_flag: Account %s administratively locked out with no \
bad password time. Leaving locked out.\n",
			pdb_get_username(sampass) ));
			return True;
	}

	if ((time(NULL) > (LastBadPassword + (time_t) duration * 60))) {
		pdb_set_acct_ctrl(sampass,
				  pdb_get_acct_ctrl(sampass) & ~ACB_AUTOLOCK,
				  PDB_CHANGED);
		pdb_set_bad_password_count(sampass, 0, PDB_CHANGED);
		pdb_set_bad_password_time(sampass, 0, PDB_CHANGED);
		if (updated) {
			*updated = True;
		}
	}
	
	return True;
}

/*********************************************************************
 Increment the bad_password_count 
*********************************************************************/

BOOL pdb_increment_bad_password_count(struct samu *sampass)
{
	uint32 account_policy_lockout;
	BOOL autolock_updated = False, badpw_updated = False;
	BOOL ret;

	/* Retrieve the account lockout policy */
	become_root();
	ret = pdb_get_account_policy(AP_BAD_ATTEMPT_LOCKOUT, &account_policy_lockout);
	unbecome_root();
	if ( !ret ) {
		DEBUG(0, ("pdb_increment_bad_password_count: pdb_get_account_policy failed.\n"));
		return False;
	}

	/* If there is no policy, we don't need to continue checking */
	if (!account_policy_lockout) {
		DEBUG(9, ("No lockout policy, don't track bad passwords\n"));
		return True;
	}

	/* Check if the autolock needs to be cleared */
	if (!pdb_update_autolock_flag(sampass, &autolock_updated))
		return False;

	/* Check if the badpw count needs to be reset */
	if (!pdb_update_bad_password_count(sampass, &badpw_updated))
		return False;

	/*
	  Ok, now we can assume that any resetting that needs to be 
	  done has been done, and just get on with incrementing
	  and autolocking if necessary
	*/

	pdb_set_bad_password_count(sampass, 
				   pdb_get_bad_password_count(sampass)+1,
				   PDB_CHANGED);
	pdb_set_bad_password_time(sampass, time(NULL), PDB_CHANGED);


	if (pdb_get_bad_password_count(sampass) < account_policy_lockout) 
		return True;

	if (!pdb_set_acct_ctrl(sampass,
			       pdb_get_acct_ctrl(sampass) | ACB_AUTOLOCK,
			       PDB_CHANGED)) {
		DEBUG(1, ("pdb_increment_bad_password_count:failed to set 'autolock' flag. \n")); 
		return False;
	}

	return True;
}
