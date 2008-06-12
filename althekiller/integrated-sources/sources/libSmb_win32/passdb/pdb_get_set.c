/* 
   Unix SMB/CIFS implementation.
   struct samu access routines
   Copyright (C) Jeremy Allison 		1996-2001
   Copyright (C) Luke Kenneth Casson Leighton 	1996-1998
   Copyright (C) Gerald (Jerry) Carter		2000-2006
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

/**
 * @todo Redefine this to NULL, but this changes the API because
 *       much of samba assumes that the pdb_get...() funtions 
 *       return pstrings.  (ie not null-pointers).
 *       See also pdb_fill_default_sam().
 */

#define PDB_NOT_QUITE_NULL ""

/*********************************************************************
 Collection of get...() functions for struct samu.
 ********************************************************************/

uint32 pdb_get_acct_ctrl(const struct samu *sampass)
{
	return sampass->acct_ctrl;
}

time_t pdb_get_logon_time(const struct samu *sampass)
{
	return sampass->logon_time;
}

time_t pdb_get_logoff_time(const struct samu *sampass)
{
	return sampass->logoff_time;
}

time_t pdb_get_kickoff_time(const struct samu *sampass)
{
	return sampass->kickoff_time;
}

time_t pdb_get_bad_password_time(const struct samu *sampass)
{
	return sampass->bad_password_time;
}

time_t pdb_get_pass_last_set_time(const struct samu *sampass)
{
	return sampass->pass_last_set_time;
}

time_t pdb_get_pass_can_change_time(const struct samu *sampass)
{
	return sampass->pass_can_change_time;
}

time_t pdb_get_pass_must_change_time(const struct samu *sampass)
{
	return sampass->pass_must_change_time;
}

uint16 pdb_get_logon_divs(const struct samu *sampass)
{
	return sampass->logon_divs;
}

uint32 pdb_get_hours_len(const struct samu *sampass)
{
	return sampass->hours_len;
}

const uint8 *pdb_get_hours(const struct samu *sampass)
{
	return (sampass->hours);
}

const uint8 *pdb_get_nt_passwd(const struct samu *sampass)
{
	SMB_ASSERT((!sampass->nt_pw.data) 
		   || sampass->nt_pw.length == NT_HASH_LEN);
	return (uint8 *)sampass->nt_pw.data;
}

const uint8 *pdb_get_lanman_passwd(const struct samu *sampass)
{
	SMB_ASSERT((!sampass->lm_pw.data) 
		   || sampass->lm_pw.length == LM_HASH_LEN);
	return (uint8 *)sampass->lm_pw.data;
}

const uint8 *pdb_get_pw_history(const struct samu *sampass, uint32 *current_hist_len)
{
	SMB_ASSERT((!sampass->nt_pw_his.data) 
	   || ((sampass->nt_pw_his.length % PW_HISTORY_ENTRY_LEN) == 0));
	*current_hist_len = sampass->nt_pw_his.length / PW_HISTORY_ENTRY_LEN;
	return (uint8 *)sampass->nt_pw_his.data;
}

/* Return the plaintext password if known.  Most of the time
   it isn't, so don't assume anything magic about this function.
   
   Used to pass the plaintext to passdb backends that might 
   want to store more than just the NTLM hashes.
*/
const char *pdb_get_plaintext_passwd(const struct samu *sampass)
{
	return sampass->plaintext_pw;
}

const DOM_SID *pdb_get_user_sid(const struct samu *sampass)
{
	return &sampass->user_sid;
}

const DOM_SID *pdb_get_group_sid(struct samu *sampass)
{
	DOM_SID *gsid;
	struct passwd *pwd;
	
	/* Return the cached group SID if we have that */
	if ( sampass->group_sid ) {
		return sampass->group_sid;
	}
		
	/* generate the group SID from the user's primary Unix group */
	
	if ( !(gsid  = TALLOC_P( sampass, DOM_SID )) ) {
		return NULL;
	}
	
	/* No algorithmic mapping, meaning that we have to figure out the
	   primary group SID according to group mapping and the user SID must
	   be a newly allocated one.  We rely on the user's Unix primary gid.
	   We have no choice but to fail if we can't find it. */

	if ( sampass->unix_pw ) {
		pwd = sampass->unix_pw;
	} else {
		pwd = Get_Pwnam_alloc( sampass, pdb_get_username(sampass) );
	}

	if ( !pwd ) {
		DEBUG(0,("pdb_get_group_sid: Failed to find Unix account for %s\n", pdb_get_username(sampass) ));
		return NULL;
	}
	
	if ( pdb_gid_to_sid(pwd->pw_gid, gsid) ) {
		enum SID_NAME_USE type = SID_NAME_UNKNOWN;
		TALLOC_CTX *mem_ctx = talloc_init("pdb_get_group_sid");
		BOOL lookup_ret;
		
		if (!mem_ctx) {
			return NULL;
		}

		/* Now check that it's actually a domain group and not something else */

		lookup_ret = lookup_sid(mem_ctx, gsid, NULL, NULL, &type);

		TALLOC_FREE( mem_ctx );

		if ( lookup_ret && (type == SID_NAME_DOM_GRP) ) {
			sampass->group_sid = gsid;
			return sampass->group_sid;
		}

		DEBUG(3, ("Primary group for user %s is a %s and not a domain group\n", 
			pwd->pw_name, sid_type_lookup(type)));
	}

	/* Just set it to the 'Domain Users' RID of 512 which will 
	   always resolve to a name */
		   
	sid_copy( gsid, get_global_sam_sid() );
	sid_append_rid( gsid, DOMAIN_GROUP_RID_USERS );
		
	sampass->group_sid = gsid;
		
	return sampass->group_sid;
}	

/**
 * Get flags showing what is initalised in the struct samu
 * @param sampass the struct samu in question
 * @return the flags indicating the members initialised in the struct.
 **/
 
enum pdb_value_state pdb_get_init_flags(const struct samu *sampass, enum pdb_elements element)
{
	enum pdb_value_state ret = PDB_DEFAULT;
	
        if (!sampass->change_flags || !sampass->set_flags)
        	return ret;
        	
        if (bitmap_query(sampass->set_flags, element)) {
		DEBUG(11, ("element %d: SET\n", element)); 
        	ret = PDB_SET;
	}
		
        if (bitmap_query(sampass->change_flags, element)) {
		DEBUG(11, ("element %d: CHANGED\n", element)); 
        	ret = PDB_CHANGED;
	}

	if (ret == PDB_DEFAULT) {
		DEBUG(11, ("element %d: DEFAULT\n", element)); 
	}

        return ret;
}

const char *pdb_get_username(const struct samu *sampass)
{
	return sampass->username;
}

const char *pdb_get_domain(const struct samu *sampass)
{
	return sampass->domain;
}

const char *pdb_get_nt_username(const struct samu *sampass)
{
	return sampass->nt_username;
}

const char *pdb_get_fullname(const struct samu *sampass)
{
	return sampass->full_name;
}

const char *pdb_get_homedir(const struct samu *sampass)
{
	return sampass->home_dir;
}

const char *pdb_get_unix_homedir(const struct samu *sampass)
{
	if (sampass->unix_pw ) {
		return sampass->unix_pw->pw_dir;
	}
	return NULL;
}

const char *pdb_get_dir_drive(const struct samu *sampass)
{
	return sampass->dir_drive;
}

const char *pdb_get_logon_script(const struct samu *sampass)
{
	return sampass->logon_script;
}

const char *pdb_get_profile_path(const struct samu *sampass)
{
	return sampass->profile_path;
}

const char *pdb_get_acct_desc(const struct samu *sampass)
{
	return sampass->acct_desc;
}

const char *pdb_get_workstations(const struct samu *sampass)
{
	return sampass->workstations;
}

const char *pdb_get_unknown_str(const struct samu *sampass)
{
	return sampass->unknown_str;
}

const char *pdb_get_munged_dial(const struct samu *sampass)
{
	return sampass->munged_dial;
}

uint16 pdb_get_bad_password_count(const struct samu *sampass)
{
	return sampass->bad_password_count;
}

uint16 pdb_get_logon_count(const struct samu *sampass)
{
	return sampass->logon_count;
}

uint32 pdb_get_unknown_6(const struct samu *sampass)
{
	return sampass->unknown_6;
}

void *pdb_get_backend_private_data(const struct samu *sampass, const struct pdb_methods *my_methods)
{
	if (my_methods == sampass->backend_private_methods) {
		return sampass->backend_private_data;
	} else {
		return NULL;
	}
}

/*********************************************************************
 Collection of set...() functions for struct samu.
 ********************************************************************/

BOOL pdb_set_acct_ctrl(struct samu *sampass, uint32 acct_ctrl, enum pdb_value_state flag)
{
	sampass->acct_ctrl = acct_ctrl;
	return pdb_set_init_flags(sampass, PDB_ACCTCTRL, flag);
}

BOOL pdb_set_logon_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->logon_time = mytime;
	return pdb_set_init_flags(sampass, PDB_LOGONTIME, flag);
}

BOOL pdb_set_logoff_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->logoff_time = mytime;
	return pdb_set_init_flags(sampass, PDB_LOGOFFTIME, flag);
}

BOOL pdb_set_kickoff_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->kickoff_time = mytime;
	return pdb_set_init_flags(sampass, PDB_KICKOFFTIME, flag);
}

BOOL pdb_set_bad_password_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->bad_password_time = mytime;
	return pdb_set_init_flags(sampass, PDB_BAD_PASSWORD_TIME, flag);
}

BOOL pdb_set_pass_can_change_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->pass_can_change_time = mytime;
	return pdb_set_init_flags(sampass, PDB_CANCHANGETIME, flag);
}

BOOL pdb_set_pass_must_change_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->pass_must_change_time = mytime;
	return pdb_set_init_flags(sampass, PDB_MUSTCHANGETIME, flag);
}

BOOL pdb_set_pass_last_set_time(struct samu *sampass, time_t mytime, enum pdb_value_state flag)
{
	sampass->pass_last_set_time = mytime;
	return pdb_set_init_flags(sampass, PDB_PASSLASTSET, flag);
}

BOOL pdb_set_hours_len(struct samu *sampass, uint32 len, enum pdb_value_state flag)
{
	sampass->hours_len = len;
	return pdb_set_init_flags(sampass, PDB_HOURSLEN, flag);
}

BOOL pdb_set_logon_divs(struct samu *sampass, uint16 hours, enum pdb_value_state flag)
{
	sampass->logon_divs = hours;
	return pdb_set_init_flags(sampass, PDB_LOGONDIVS, flag);
}

/**
 * Set flags showing what is initalised in the struct samu
 * @param sampass the struct samu in question
 * @param flag The *new* flag to be set.  Old flags preserved
 *             this flag is only added.  
 **/
 
BOOL pdb_set_init_flags(struct samu *sampass, enum pdb_elements element, enum pdb_value_state value_flag)
{
        if (!sampass->set_flags) {
        	if ((sampass->set_flags = 
        		bitmap_talloc(sampass, 
        				PDB_COUNT))==NULL) {
        		DEBUG(0,("bitmap_talloc failed\n"));
        		return False;
        	}
        }
        if (!sampass->change_flags) {
        	if ((sampass->change_flags = 
        		bitmap_talloc(sampass, 
        				PDB_COUNT))==NULL) {
        		DEBUG(0,("bitmap_talloc failed\n"));
        		return False;
        	}
        }
        
        switch(value_flag) {
        	case PDB_CHANGED:
        		if (!bitmap_set(sampass->change_flags, element)) {
				DEBUG(0,("Can't set flag: %d in change_flags.\n",element));
				return False;
			}
        		if (!bitmap_set(sampass->set_flags, element)) {
				DEBUG(0,("Can't set flag: %d in set_flags.\n",element));
				return False;
			}
			DEBUG(11, ("element %d -> now CHANGED\n", element)); 
        		break;
        	case PDB_SET:
        		if (!bitmap_clear(sampass->change_flags, element)) {
				DEBUG(0,("Can't set flag: %d in change_flags.\n",element));
				return False;
			}
        		if (!bitmap_set(sampass->set_flags, element)) {
				DEBUG(0,("Can't set flag: %d in set_flags.\n",element));
				return False;
			}
			DEBUG(11, ("element %d -> now SET\n", element)); 
        		break;
        	case PDB_DEFAULT:
        	default:
        		if (!bitmap_clear(sampass->change_flags, element)) {
				DEBUG(0,("Can't set flag: %d in change_flags.\n",element));
				return False;
			}
        		if (!bitmap_clear(sampass->set_flags, element)) {
				DEBUG(0,("Can't set flag: %d in set_flags.\n",element));
				return False;
			}
			DEBUG(11, ("element %d -> now DEFAULT\n", element)); 
        		break;
	}

        return True;
}

BOOL pdb_set_user_sid(struct samu *sampass, const DOM_SID *u_sid, enum pdb_value_state flag)
{
	if (!u_sid)
		return False;
	
	sid_copy(&sampass->user_sid, u_sid);

	DEBUG(10, ("pdb_set_user_sid: setting user sid %s\n", 
		    sid_string_static(&sampass->user_sid)));

	return pdb_set_init_flags(sampass, PDB_USERSID, flag);
}

BOOL pdb_set_user_sid_from_string(struct samu *sampass, fstring u_sid, enum pdb_value_state flag)
{
	DOM_SID new_sid;
	
	if (!u_sid)
		return False;

	DEBUG(10, ("pdb_set_user_sid_from_string: setting user sid %s\n",
		   u_sid));

	if (!string_to_sid(&new_sid, u_sid)) { 
		DEBUG(1, ("pdb_set_user_sid_from_string: %s isn't a valid SID!\n", u_sid));
		return False;
	}
	 
	if (!pdb_set_user_sid(sampass, &new_sid, flag)) {
		DEBUG(1, ("pdb_set_user_sid_from_string: could not set sid %s on struct samu!\n", u_sid));
		return False;
	}

	return True;
}

/********************************************************************
 We never fill this in from a passdb backend but rather set is 
 based on the user's primary group membership.  However, the 
 struct samu* is overloaded and reused in domain memship code 
 as well and built from the NET_USER_INFO_3 or PAC so we 
 have to allow the explicitly setting of a group SID here.
********************************************************************/

BOOL pdb_set_group_sid(struct samu *sampass, const DOM_SID *g_sid, enum pdb_value_state flag)
{
	gid_t gid;

	if (!g_sid)
		return False;

	if ( !(sampass->group_sid = TALLOC_P( sampass, DOM_SID )) ) {
		return False;
	}

	/* if we cannot resolve the SID to gid, then just ignore it and 
	   store DOMAIN_USERS as the primary groupSID */

	if ( sid_to_gid( g_sid, &gid ) ) {
		sid_copy(sampass->group_sid, g_sid);
	} else {
		sid_copy( sampass->group_sid, get_global_sam_sid() );
		sid_append_rid( sampass->group_sid, DOMAIN_GROUP_RID_USERS );
	}

	DEBUG(10, ("pdb_set_group_sid: setting group sid %s\n", 
		sid_string_static(sampass->group_sid)));

	return pdb_set_init_flags(sampass, PDB_GROUPSID, flag);
}

/*********************************************************************
 Set the user's UNIX name.
 ********************************************************************/

BOOL pdb_set_username(struct samu *sampass, const char *username, enum pdb_value_state flag)
{
	if (username) { 
		DEBUG(10, ("pdb_set_username: setting username %s, was %s\n", username,
			(sampass->username)?(sampass->username):"NULL"));

		sampass->username = talloc_strdup(sampass, username);

		if (!sampass->username) {
			DEBUG(0, ("pdb_set_username: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->username = PDB_NOT_QUITE_NULL;
	}
	
	return pdb_set_init_flags(sampass, PDB_USERNAME, flag);
}

/*********************************************************************
 Set the domain name.
 ********************************************************************/

BOOL pdb_set_domain(struct samu *sampass, const char *domain, enum pdb_value_state flag)
{
	if (domain) { 
		DEBUG(10, ("pdb_set_domain: setting domain %s, was %s\n", domain,
			(sampass->domain)?(sampass->domain):"NULL"));

		sampass->domain = talloc_strdup(sampass, domain);

		if (!sampass->domain) {
			DEBUG(0, ("pdb_set_domain: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->domain = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_DOMAIN, flag);
}

/*********************************************************************
 Set the user's NT name.
 ********************************************************************/

BOOL pdb_set_nt_username(struct samu *sampass, const char *nt_username, enum pdb_value_state flag)
{
	if (nt_username) { 
		DEBUG(10, ("pdb_set_nt_username: setting nt username %s, was %s\n", nt_username,
			(sampass->nt_username)?(sampass->nt_username):"NULL"));
 
		sampass->nt_username = talloc_strdup(sampass, nt_username);
		
		if (!sampass->nt_username) {
			DEBUG(0, ("pdb_set_nt_username: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->nt_username = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_NTUSERNAME, flag);
}

/*********************************************************************
 Set the user's full name.
 ********************************************************************/

BOOL pdb_set_fullname(struct samu *sampass, const char *full_name, enum pdb_value_state flag)
{
	if (full_name) { 
		DEBUG(10, ("pdb_set_full_name: setting full name %s, was %s\n", full_name,
			(sampass->full_name)?(sampass->full_name):"NULL"));
	
		sampass->full_name = talloc_strdup(sampass, full_name);

		if (!sampass->full_name) {
			DEBUG(0, ("pdb_set_fullname: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->full_name = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_FULLNAME, flag);
}

/*********************************************************************
 Set the user's logon script.
 ********************************************************************/

BOOL pdb_set_logon_script(struct samu *sampass, const char *logon_script, enum pdb_value_state flag)
{
	if (logon_script) { 
		DEBUG(10, ("pdb_set_logon_script: setting logon script %s, was %s\n", logon_script,
			(sampass->logon_script)?(sampass->logon_script):"NULL"));
 
		sampass->logon_script = talloc_strdup(sampass, logon_script);

		if (!sampass->logon_script) {
			DEBUG(0, ("pdb_set_logon_script: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->logon_script = PDB_NOT_QUITE_NULL;
	}
	
	return pdb_set_init_flags(sampass, PDB_LOGONSCRIPT, flag);
}

/*********************************************************************
 Set the user's profile path.
 ********************************************************************/

BOOL pdb_set_profile_path(struct samu *sampass, const char *profile_path, enum pdb_value_state flag)
{
	if (profile_path) { 
		DEBUG(10, ("pdb_set_profile_path: setting profile path %s, was %s\n", profile_path,
			(sampass->profile_path)?(sampass->profile_path):"NULL"));
 
		sampass->profile_path = talloc_strdup(sampass, profile_path);
		
		if (!sampass->profile_path) {
			DEBUG(0, ("pdb_set_profile_path: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->profile_path = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_PROFILE, flag);
}

/*********************************************************************
 Set the user's directory drive.
 ********************************************************************/

BOOL pdb_set_dir_drive(struct samu *sampass, const char *dir_drive, enum pdb_value_state flag)
{
	if (dir_drive) { 
		DEBUG(10, ("pdb_set_dir_drive: setting dir drive %s, was %s\n", dir_drive,
			(sampass->dir_drive)?(sampass->dir_drive):"NULL"));
 
		sampass->dir_drive = talloc_strdup(sampass, dir_drive);
		
		if (!sampass->dir_drive) {
			DEBUG(0, ("pdb_set_dir_drive: talloc_strdup() failed!\n"));
			return False;
		}

	} else {
		sampass->dir_drive = PDB_NOT_QUITE_NULL;
	}
	
	return pdb_set_init_flags(sampass, PDB_DRIVE, flag);
}

/*********************************************************************
 Set the user's home directory.
 ********************************************************************/

BOOL pdb_set_homedir(struct samu *sampass, const char *home_dir, enum pdb_value_state flag)
{
	if (home_dir) { 
		DEBUG(10, ("pdb_set_homedir: setting home dir %s, was %s\n", home_dir,
			(sampass->home_dir)?(sampass->home_dir):"NULL"));
 
		sampass->home_dir = talloc_strdup(sampass, home_dir);
		
		if (!sampass->home_dir) {
			DEBUG(0, ("pdb_set_home_dir: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->home_dir = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_SMBHOME, flag);
}

/*********************************************************************
 Set the user's account description.
 ********************************************************************/

BOOL pdb_set_acct_desc(struct samu *sampass, const char *acct_desc, enum pdb_value_state flag)
{
	if (acct_desc) { 
		sampass->acct_desc = talloc_strdup(sampass, acct_desc);

		if (!sampass->acct_desc) {
			DEBUG(0, ("pdb_set_acct_desc: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->acct_desc = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_ACCTDESC, flag);
}

/*********************************************************************
 Set the user's workstation allowed list.
 ********************************************************************/

BOOL pdb_set_workstations(struct samu *sampass, const char *workstations, enum pdb_value_state flag)
{
	if (workstations) { 
		DEBUG(10, ("pdb_set_workstations: setting workstations %s, was %s\n", workstations,
			(sampass->workstations)?(sampass->workstations):"NULL"));
 
		sampass->workstations = talloc_strdup(sampass, workstations);

		if (!sampass->workstations) {
			DEBUG(0, ("pdb_set_workstations: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->workstations = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_WORKSTATIONS, flag);
}

/*********************************************************************
 Set the user's 'unknown_str', whatever the heck this actually is...
 ********************************************************************/

BOOL pdb_set_unknown_str(struct samu *sampass, const char *unknown_str, enum pdb_value_state flag)
{
	if (unknown_str) { 
		sampass->unknown_str = talloc_strdup(sampass, unknown_str);
		
		if (!sampass->unknown_str) {
			DEBUG(0, ("pdb_set_unknown_str: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->unknown_str = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_UNKNOWNSTR, flag);
}

/*********************************************************************
 Set the user's dial string.
 ********************************************************************/

BOOL pdb_set_munged_dial(struct samu *sampass, const char *munged_dial, enum pdb_value_state flag)
{
	if (munged_dial) { 
		sampass->munged_dial = talloc_strdup(sampass, munged_dial);
		
		if (!sampass->munged_dial) {
			DEBUG(0, ("pdb_set_munged_dial: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->munged_dial = PDB_NOT_QUITE_NULL;
	}

	return pdb_set_init_flags(sampass, PDB_MUNGEDDIAL, flag);
}

/*********************************************************************
 Set the user's NT hash.
 ********************************************************************/

BOOL pdb_set_nt_passwd(struct samu *sampass, const uint8 pwd[NT_HASH_LEN], enum pdb_value_state flag)
{
	data_blob_clear_free(&sampass->nt_pw);
	
       if (pwd) {
               sampass->nt_pw =
		       data_blob_talloc(sampass, pwd, NT_HASH_LEN);
       } else {
               sampass->nt_pw = data_blob(NULL, 0);
       }

	return pdb_set_init_flags(sampass, PDB_NTPASSWD, flag);
}

/*********************************************************************
 Set the user's LM hash.
 ********************************************************************/

BOOL pdb_set_lanman_passwd(struct samu *sampass, const uint8 pwd[LM_HASH_LEN], enum pdb_value_state flag)
{
	data_blob_clear_free(&sampass->lm_pw);
	
	/* on keep the password if we are allowing LANMAN authentication */

	if (pwd && lp_lanman_auth() ) {
		sampass->lm_pw = data_blob_talloc(sampass, pwd, LM_HASH_LEN);
	} else {
		sampass->lm_pw = data_blob(NULL, 0);
	}

	return pdb_set_init_flags(sampass, PDB_LMPASSWD, flag);
}

/*********************************************************************
 Set the user's password history hash. historyLen is the number of 
 PW_HISTORY_SALT_LEN+SALTED_MD5_HASH_LEN length
 entries to store in the history - this must match the size of the uint8 array
 in pwd.
********************************************************************/

BOOL pdb_set_pw_history(struct samu *sampass, const uint8 *pwd, uint32 historyLen, enum pdb_value_state flag)
{
	if (historyLen && pwd){
		sampass->nt_pw_his = data_blob_talloc(sampass,
						pwd, historyLen*PW_HISTORY_ENTRY_LEN);
		if (!sampass->nt_pw_his.length) {
			DEBUG(0, ("pdb_set_pw_history: data_blob_talloc() failed!\n"));
			return False;
		}
	} else {
		sampass->nt_pw_his = data_blob_talloc(sampass, NULL, 0);
	}

	return pdb_set_init_flags(sampass, PDB_PWHISTORY, flag);
}

/*********************************************************************
 Set the user's plaintext password only (base procedure, see helper
 below)
 ********************************************************************/

BOOL pdb_set_plaintext_pw_only(struct samu *sampass, const char *password, enum pdb_value_state flag)
{
	if (password) { 
		if (sampass->plaintext_pw!=NULL) 
			memset(sampass->plaintext_pw,'\0',strlen(sampass->plaintext_pw)+1);

		sampass->plaintext_pw = talloc_strdup(sampass, password);
		
		if (!sampass->plaintext_pw) {
			DEBUG(0, ("pdb_set_unknown_str: talloc_strdup() failed!\n"));
			return False;
		}
	} else {
		sampass->plaintext_pw = NULL;
	}

	return pdb_set_init_flags(sampass, PDB_PLAINTEXT_PW, flag);
}

BOOL pdb_set_bad_password_count(struct samu *sampass, uint16 bad_password_count, enum pdb_value_state flag)
{
	sampass->bad_password_count = bad_password_count;
	return pdb_set_init_flags(sampass, PDB_BAD_PASSWORD_COUNT, flag);
}

BOOL pdb_set_logon_count(struct samu *sampass, uint16 logon_count, enum pdb_value_state flag)
{
	sampass->logon_count = logon_count;
	return pdb_set_init_flags(sampass, PDB_LOGON_COUNT, flag);
}

BOOL pdb_set_unknown_6(struct samu *sampass, uint32 unkn, enum pdb_value_state flag)
{
	sampass->unknown_6 = unkn;
	return pdb_set_init_flags(sampass, PDB_UNKNOWN6, flag);
}

BOOL pdb_set_hours(struct samu *sampass, const uint8 *hours, enum pdb_value_state flag)
{
	if (!hours) {
		memset ((char *)sampass->hours, 0, MAX_HOURS_LEN);
	} else {
		memcpy (sampass->hours, hours, MAX_HOURS_LEN);
	}

	return pdb_set_init_flags(sampass, PDB_HOURS, flag);
}

BOOL pdb_set_backend_private_data(struct samu *sampass, void *private_data, 
				   void (*free_fn)(void **), 
				   const struct pdb_methods *my_methods, 
				   enum pdb_value_state flag)
{
	if (sampass->backend_private_data &&
	    sampass->backend_private_data_free_fn) {
		sampass->backend_private_data_free_fn(
			&sampass->backend_private_data);
	}

	sampass->backend_private_data = private_data;
	sampass->backend_private_data_free_fn = free_fn;
	sampass->backend_private_methods = my_methods;

	return pdb_set_init_flags(sampass, PDB_BACKEND_PRIVATE_DATA, flag);
}


/* Helpful interfaces to the above */

/*********************************************************************
 Sets the last changed times and must change times for a normal
 password change.
 ********************************************************************/

BOOL pdb_set_pass_changed_now(struct samu *sampass)
{
	uint32 expire;
	uint32 min_age;

	if (!pdb_set_pass_last_set_time (sampass, time(NULL), PDB_CHANGED))
		return False;

	if (!pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &expire) 
	    || (expire==(uint32)-1) || (expire == 0)) {
		if (!pdb_set_pass_must_change_time (sampass, get_time_t_max(), PDB_CHANGED))
			return False;
	} else {
		if (!pdb_set_pass_must_change_time (sampass, 
						    pdb_get_pass_last_set_time(sampass)
						    + expire, PDB_CHANGED))
			return False;
	}
	
	if (!pdb_get_account_policy(AP_MIN_PASSWORD_AGE, &min_age) 
	    || (min_age==(uint32)-1)) {
		if (!pdb_set_pass_can_change_time (sampass, 0, PDB_CHANGED))
			return False;
	} else {
		if (!pdb_set_pass_can_change_time (sampass, 
						    pdb_get_pass_last_set_time(sampass)
						    + min_age, PDB_CHANGED))
			return False;
	}
	return True;
}

/*********************************************************************
 Set the user's PLAINTEXT password.  Used as an interface to the above.
 Also sets the last change time to NOW.
 ********************************************************************/

BOOL pdb_set_plaintext_passwd(struct samu *sampass, const char *plaintext)
{
	uchar new_lanman_p16[LM_HASH_LEN];
	uchar new_nt_p16[NT_HASH_LEN];

	if (!plaintext)
		return False;

	/* Calculate the MD4 hash (NT compatible) of the password */
	E_md4hash(plaintext, new_nt_p16);

	if (!pdb_set_nt_passwd (sampass, new_nt_p16, PDB_CHANGED)) 
		return False;

	if (!E_deshash(plaintext, new_lanman_p16)) {
		/* E_deshash returns false for 'long' passwords (> 14
		   DOS chars).  This allows us to match Win2k, which
		   does not store a LM hash for these passwords (which
		   would reduce the effective password length to 14 */

		if (!pdb_set_lanman_passwd (sampass, NULL, PDB_CHANGED)) 
			return False;
	} else {
		if (!pdb_set_lanman_passwd (sampass, new_lanman_p16, PDB_CHANGED)) 
			return False;
	}

	if (!pdb_set_plaintext_pw_only (sampass, plaintext, PDB_CHANGED)) 
		return False;

	if (!pdb_set_pass_changed_now (sampass))
		return False;

	/* Store the password history. */
	if (pdb_get_acct_ctrl(sampass) & ACB_NORMAL) {
		uchar *pwhistory;
		uint32 pwHistLen;
		pdb_get_account_policy(AP_PASSWORD_HISTORY, &pwHistLen);
		if (pwHistLen != 0){
			uint32 current_history_len;
			/* We need to make sure we don't have a race condition here - the
			   account policy history length can change between when the pw_history
			   was first loaded into the struct samu struct and now.... JRA. */
			pwhistory = (uchar *)pdb_get_pw_history(sampass, &current_history_len);

			if (current_history_len != pwHistLen) {
				/* After closing and reopening struct samu the history
					values will sync up. We can't do this here. */

				/* current_history_len > pwHistLen is not a problem - we
					have more history than we need. */

				if (current_history_len < pwHistLen) {
					/* Ensure we have space for the needed history. */
					uchar *new_history = TALLOC(sampass,
								pwHistLen*PW_HISTORY_ENTRY_LEN);
					if (!new_history) {
						return False;
					}

					/* And copy it into the new buffer. */
					if (current_history_len) {
						memcpy(new_history, pwhistory,
							current_history_len*PW_HISTORY_ENTRY_LEN);
					}
					/* Clearing out any extra space. */
					memset(&new_history[current_history_len*PW_HISTORY_ENTRY_LEN],
						'\0', (pwHistLen-current_history_len)*PW_HISTORY_ENTRY_LEN);
					/* Finally replace it. */
					pwhistory = new_history;
				}
			}
			if (pwhistory && pwHistLen){
				/* Make room for the new password in the history list. */
				if (pwHistLen > 1) {
					memmove(&pwhistory[PW_HISTORY_ENTRY_LEN],
						pwhistory, (pwHistLen -1)*PW_HISTORY_ENTRY_LEN );
				}
				/* Create the new salt as the first part of the history entry. */
				generate_random_buffer(pwhistory, PW_HISTORY_SALT_LEN);

				/* Generate the md5 hash of the salt+new password as the second
					part of the history entry. */

				E_md5hash(pwhistory, new_nt_p16, &pwhistory[PW_HISTORY_SALT_LEN]);
				pdb_set_pw_history(sampass, pwhistory, pwHistLen, PDB_CHANGED);
			} else {
				DEBUG (10,("pdb_get_set.c: pdb_set_plaintext_passwd: pwhistory was NULL!\n"));
			}
		} else {
			/* Set the history length to zero. */
			pdb_set_pw_history(sampass, NULL, 0, PDB_CHANGED);
		}
	}

	return True;
}

/* check for any PDB_SET/CHANGED field and fill the appropriate mask bit */
uint32 pdb_build_fields_present(struct samu *sampass)
{
	/* value set to all for testing */
	return 0x00ffffff;
}
