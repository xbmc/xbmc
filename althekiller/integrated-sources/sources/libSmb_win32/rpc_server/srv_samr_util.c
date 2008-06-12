/* 
   Unix SMB/CIFS implementation.
   SAMR Pipe utility functions.
   
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
#define DBGC_CLASS DBGC_RPC_SRV

#define STRING_CHANGED (old_string && !new_string) ||\
		    (!old_string && new_string) ||\
		(old_string && new_string && (strcmp(old_string, new_string) != 0))

#define STRING_CHANGED_NC(s1,s2) ((s1) && !(s2)) ||\
		    (!(s1) && (s2)) ||\
		((s1) && (s2) && (strcmp((s1), (s2)) != 0))

/*************************************************************
 Copies a SAM_USER_INFO_20 to a struct samu
**************************************************************/

void copy_id20_to_sam_passwd(struct samu *to, SAM_USER_INFO_20 *from)
{
	const char *old_string;
	char *new_string;
	DATA_BLOB mung;

	if (from == NULL || to == NULL) 
		return;
	
	if (from->hdr_munged_dial.buffer) {
		old_string = pdb_get_munged_dial(to);
		mung.length = from->hdr_munged_dial.uni_str_len;
		mung.data = (uint8 *) from->uni_munged_dial.buffer;
		new_string = (mung.length == 0) ?
			NULL : base64_encode_data_blob(mung);
		DEBUG(10,("INFO_20 UNI_MUNGED_DIAL: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED_NC(old_string,new_string))
			pdb_set_munged_dial(to   , new_string, PDB_CHANGED);

		SAFE_FREE(new_string);
	}
}

/*************************************************************
 Copies a SAM_USER_INFO_21 to a struct samu
**************************************************************/

void copy_id21_to_sam_passwd(struct samu *to, SAM_USER_INFO_21 *from)
{
	time_t unix_time, stored_time;
	const char *old_string, *new_string;
	DATA_BLOB mung;

	if (from == NULL || to == NULL) 
		return;

	if (from->fields_present & ACCT_LAST_LOGON) {
		unix_time=nt_time_to_unix(&from->logon_time);
		stored_time = pdb_get_logon_time(to);
		DEBUG(10,("INFO_21 LOGON_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logon_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_LOGOFF) {
		unix_time=nt_time_to_unix(&from->logoff_time);
		stored_time = pdb_get_logoff_time(to);
		DEBUG(10,("INFO_21 LOGOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logoff_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_EXPIRY) {
		unix_time=nt_time_to_unix(&from->kickoff_time);
		stored_time = pdb_get_kickoff_time(to);
		DEBUG(10,("INFO_21 KICKOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_kickoff_time(to, unix_time , PDB_CHANGED);
	}	

	if (from->fields_present & ACCT_ALLOW_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_can_change_time);
		stored_time = pdb_get_pass_can_change_time(to);
		DEBUG(10,("INFO_21 PASS_CAN_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_can_change_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_last_set_time);
		stored_time = pdb_get_pass_last_set_time(to);
		DEBUG(10,("INFO_21 PASS_LAST_SET: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_last_set_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_FORCE_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_must_change_time);
		stored_time=pdb_get_pass_must_change_time(to);
		DEBUG(10,("INFO_21 PASS_MUST_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_must_change_time(to, unix_time, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_USERNAME) &&
	    (from->hdr_user_name.buffer)) {
		old_string = pdb_get_username(to);
		new_string = unistr2_static(&from->uni_user_name);
		DEBUG(10,("INFO_21 UNI_USER_NAME: %s -> %s\n", old_string, new_string));
		if (STRING_CHANGED)
		    pdb_set_username(to      , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_FULL_NAME) &&
	    (from->hdr_full_name.buffer)) {
		old_string = pdb_get_fullname(to);
		new_string = unistr2_static(&from->uni_full_name);
		DEBUG(10,("INFO_21 UNI_FULL_NAME: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_fullname(to      , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_HOME_DIR) &&
	    (from->hdr_home_dir.buffer)) {
		old_string = pdb_get_homedir(to);
		new_string = unistr2_static(&from->uni_home_dir);
		DEBUG(10,("INFO_21 UNI_HOME_DIR: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_homedir(to       , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_HOME_DRIVE) &&
	    (from->hdr_dir_drive.buffer)) {
		old_string = pdb_get_dir_drive(to);
		new_string = unistr2_static(&from->uni_dir_drive);
		DEBUG(10,("INFO_21 UNI_DIR_DRIVE: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_dir_drive(to     , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_LOGON_SCRIPT) &&
	    (from->hdr_logon_script.buffer)) {
		old_string = pdb_get_logon_script(to);
		new_string = unistr2_static(&from->uni_logon_script);
		DEBUG(10,("INFO_21 UNI_LOGON_SCRIPT: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_logon_script(to  , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_PROFILE) &&
	    (from->hdr_profile_path.buffer)) {
		old_string = pdb_get_profile_path(to);
		new_string = unistr2_static(&from->uni_profile_path);
		DEBUG(10,("INFO_21 UNI_PROFILE_PATH: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_profile_path(to  , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_DESCRIPTION) &&
	    (from->hdr_acct_desc.buffer)) {
		old_string = pdb_get_acct_desc(to);
		new_string = unistr2_static(&from->uni_acct_desc);
		DEBUG(10,("INFO_21 UNI_ACCT_DESC: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_acct_desc(to     , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_WORKSTATIONS) &&
	    (from->hdr_workstations.buffer)) {
		old_string = pdb_get_workstations(to);
		new_string = unistr2_static(&from->uni_workstations);
		DEBUG(10,("INFO_21 UNI_WORKSTATIONS: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_workstations(to  , new_string, PDB_CHANGED);
	}

	/* is this right? */
	if ((from->fields_present & ACCT_ADMIN_DESC) &&
	    (from->hdr_unknown_str.buffer)) {
		old_string = pdb_get_unknown_str(to);
		new_string = unistr2_static(&from->uni_unknown_str);
		DEBUG(10,("INFO_21 UNI_UNKNOWN_STR: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_unknown_str(to   , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_CALLBACK) &&
	    (from->hdr_munged_dial.buffer)) {
		char *newstr;
		old_string = pdb_get_munged_dial(to);
		mung.length = from->hdr_munged_dial.uni_str_len;
		mung.data = (uint8 *) from->uni_munged_dial.buffer;
		newstr = (mung.length == 0) ?
			NULL : base64_encode_data_blob(mung);
		DEBUG(10,("INFO_21 UNI_MUNGED_DIAL: %s -> %s\n",old_string, newstr));
		if (STRING_CHANGED_NC(old_string,newstr))
			pdb_set_munged_dial(to   , newstr, PDB_CHANGED);

		SAFE_FREE(newstr);
	}
	
	if (from->fields_present & ACCT_RID) {
		if (from->user_rid == 0) {
			DEBUG(10, ("INFO_21: Asked to set User RID to 0 !? Skipping change!\n"));
		} else if (from->user_rid != pdb_get_user_rid(to)) {
			DEBUG(10,("INFO_21 USER_RID: %u -> %u NOT UPDATED!\n",pdb_get_user_rid(to),from->user_rid));
		}
	}
	
	if (from->fields_present & ACCT_PRIMARY_GID) {
		if (from->group_rid == 0) {
			DEBUG(10, ("INFO_21: Asked to set Group RID to 0 !? Skipping change!\n"));
		} else if (from->group_rid != pdb_get_group_rid(to)) {
			DEBUG(10,("INFO_21 GROUP_RID: %u -> %u\n",pdb_get_group_rid(to),from->group_rid));
			pdb_set_group_sid_from_rid(to, from->group_rid, PDB_CHANGED);
		}
	}
	
	if (from->fields_present & ACCT_FLAGS) {
		DEBUG(10,("INFO_21 ACCT_CTRL: %08X -> %08X\n",pdb_get_acct_ctrl(to),from->acb_info));
		if (from->acb_info != pdb_get_acct_ctrl(to)) {
			if (!(from->acb_info & ACB_AUTOLOCK) && (pdb_get_acct_ctrl(to) & ACB_AUTOLOCK)) {
				/* We're unlocking a previously locked user. Reset bad password counts.
				   Patch from Jianliang Lu. <Jianliang.Lu@getronics.com> */
				pdb_set_bad_password_count(to, 0, PDB_CHANGED);
				pdb_set_bad_password_time(to, 0, PDB_CHANGED);
			}
			pdb_set_acct_ctrl(to, from->acb_info, PDB_CHANGED);
		}
	}

	if (from->fields_present & ACCT_LOGON_HOURS) {
		pstring oldstr, newstr;
		DEBUG(15,("INFO_21 LOGON_DIVS: %08X -> %08X\n",pdb_get_logon_divs(to),from->logon_divs));
		if (from->logon_divs != pdb_get_logon_divs(to)) {
			pdb_set_logon_divs(to, from->logon_divs, PDB_CHANGED);
		}

		DEBUG(15,("INFO_21 LOGON_HRS.LEN: %08X -> %08X\n",pdb_get_hours_len(to),from->logon_hrs.len));
		if (from->logon_hrs.len != pdb_get_hours_len(to)) {
			pdb_set_hours_len(to, from->logon_hrs.len, PDB_CHANGED);
		}

		DEBUG(15,("INFO_21 LOGON_HRS.HOURS: %s -> %s\n",pdb_get_hours(to),from->logon_hrs.hours));
		pdb_sethexhours(oldstr, pdb_get_hours(to));
		pdb_sethexhours(newstr, from->logon_hrs.hours);
		if (!strequal(oldstr, newstr)) {
			pdb_set_hours(to, from->logon_hrs.hours, PDB_CHANGED);
		}
	}

	if (from->fields_present & ACCT_BAD_PWD_COUNT) {
		DEBUG(10,("INFO_21 BAD_PASSWORD_COUNT: %08X -> %08X\n",pdb_get_bad_password_count(to),from->bad_password_count));
		if (from->bad_password_count != pdb_get_bad_password_count(to)) {
			pdb_set_bad_password_count(to, from->bad_password_count, PDB_CHANGED);
		}
	}

	if (from->fields_present & ACCT_NUM_LOGONS) {
		DEBUG(10,("INFO_21 LOGON_COUNT: %08X -> %08X\n",pdb_get_logon_count(to),from->logon_count));
		if (from->logon_count != pdb_get_logon_count(to)) {
			pdb_set_logon_count(to, from->logon_count, PDB_CHANGED);
		}
	}

	DEBUG(10,("INFO_21 PASS_MUST_CHANGE_AT_NEXT_LOGON: %02X\n",from->passmustchange));
	if (from->passmustchange==PASS_MUST_CHANGE_AT_NEXT_LOGON) {
		pdb_set_pass_must_change_time(to,0, PDB_CHANGED);
	} else {
		uint32 expire;
		time_t new_time;
		if (pdb_get_pass_must_change_time(to) == 0) {
			if (!pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &expire)
			    || expire == (uint32)-1) {
				new_time = get_time_t_max();
			} else {
				time_t old_time = pdb_get_pass_last_set_time(to);
				new_time = old_time + expire;
				if ((new_time) < time(0)) {
					new_time = time(0) + expire;
				}
			}
			if (!pdb_set_pass_must_change_time (to, new_time, PDB_CHANGED)) {
				DEBUG (0, ("pdb_set_pass_must_change_time failed!\n"));
			}
		}
	}

	DEBUG(10,("INFO_21 PADDING_2: %02X\n",from->padding2));
}


/*************************************************************
 Copies a SAM_USER_INFO_23 to a struct samu
**************************************************************/

void copy_id23_to_sam_passwd(struct samu *to, SAM_USER_INFO_23 *from)
{
	time_t unix_time, stored_time;
	const char *old_string, *new_string;
	DATA_BLOB mung;

	if (from == NULL || to == NULL) 
		return;

	if (from->fields_present & ACCT_LAST_LOGON) {
		unix_time=nt_time_to_unix(&from->logon_time);
		stored_time = pdb_get_logon_time(to);
		DEBUG(10,("INFO_23 LOGON_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logon_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_LOGOFF) {
		unix_time=nt_time_to_unix(&from->logoff_time);
		stored_time = pdb_get_logoff_time(to);
		DEBUG(10,("INFO_23 LOGOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logoff_time(to, unix_time, PDB_CHANGED);
	}
	
	if (from->fields_present & ACCT_EXPIRY) {
		unix_time=nt_time_to_unix(&from->kickoff_time);
		stored_time = pdb_get_kickoff_time(to);
		DEBUG(10,("INFO_23 KICKOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_kickoff_time(to, unix_time , PDB_CHANGED);
	}	

	if (from->fields_present & ACCT_ALLOW_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_can_change_time);
		stored_time = pdb_get_pass_can_change_time(to);
		DEBUG(10,("INFO_23 PASS_CAN_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_can_change_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_last_set_time);
		stored_time = pdb_get_pass_last_set_time(to);
		DEBUG(10,("INFO_23 PASS_LAST_SET: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_last_set_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_FORCE_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_must_change_time);
		stored_time=pdb_get_pass_must_change_time(to);
		DEBUG(10,("INFO_23 PASS_MUST_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_must_change_time(to, unix_time, PDB_CHANGED);
	}

	/* Backend should check this for sanity */
	if ((from->fields_present & ACCT_USERNAME) &&
	    (from->hdr_user_name.buffer)) {
		old_string = pdb_get_username(to);
		new_string = unistr2_static(&from->uni_user_name);
		DEBUG(10,("INFO_23 UNI_USER_NAME: %s -> %s\n", old_string, new_string));
		if (STRING_CHANGED)
		    pdb_set_username(to      , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_FULL_NAME) &&
	    (from->hdr_full_name.buffer)) {
		old_string = pdb_get_fullname(to);
		new_string = unistr2_static(&from->uni_full_name);
		DEBUG(10,("INFO_23 UNI_FULL_NAME: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_fullname(to      , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_HOME_DIR) &&
	    (from->hdr_home_dir.buffer)) {
		old_string = pdb_get_homedir(to);
		new_string = unistr2_static(&from->uni_home_dir);
		DEBUG(10,("INFO_23 UNI_HOME_DIR: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_homedir(to       , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_HOME_DRIVE) &&
	    (from->hdr_dir_drive.buffer)) {
		old_string = pdb_get_dir_drive(to);
		new_string = unistr2_static(&from->uni_dir_drive);
		DEBUG(10,("INFO_23 UNI_DIR_DRIVE: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_dir_drive(to     , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_LOGON_SCRIPT) &&
	    (from->hdr_logon_script.buffer)) {
		old_string = pdb_get_logon_script(to);
		new_string = unistr2_static(&from->uni_logon_script);
		DEBUG(10,("INFO_23 UNI_LOGON_SCRIPT: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_logon_script(to  , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_PROFILE) &&
	    (from->hdr_profile_path.buffer)) {
		old_string = pdb_get_profile_path(to);
		new_string = unistr2_static(&from->uni_profile_path);
		DEBUG(10,("INFO_23 UNI_PROFILE_PATH: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_profile_path(to  , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_DESCRIPTION) &&
	    (from->hdr_acct_desc.buffer)) {
		old_string = pdb_get_acct_desc(to);
		new_string = unistr2_static(&from->uni_acct_desc);
		DEBUG(10,("INFO_23 UNI_ACCT_DESC: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_acct_desc(to     , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_WORKSTATIONS) &&
	    (from->hdr_workstations.buffer)) {
		old_string = pdb_get_workstations(to);
		new_string = unistr2_static(&from->uni_workstations);
		DEBUG(10,("INFO_23 UNI_WORKSTATIONS: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_workstations(to  , new_string, PDB_CHANGED);
	}

	/* is this right? */
	if ((from->fields_present & ACCT_ADMIN_DESC) &&
	    (from->hdr_unknown_str.buffer)) {
		old_string = pdb_get_unknown_str(to);
		new_string = unistr2_static(&from->uni_unknown_str);
		DEBUG(10,("INFO_23 UNI_UNKNOWN_STR: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_unknown_str(to   , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_CALLBACK) &&
	    (from->hdr_munged_dial.buffer)) {
		char *newstr;
		old_string = pdb_get_munged_dial(to);
		mung.length = from->hdr_munged_dial.uni_str_len;
		mung.data = (uint8 *) from->uni_munged_dial.buffer;
		newstr = (mung.length == 0) ?
			NULL : base64_encode_data_blob(mung);
		DEBUG(10,("INFO_23 UNI_MUNGED_DIAL: %s -> %s\n",old_string, newstr));
		if (STRING_CHANGED_NC(old_string, newstr))
			pdb_set_munged_dial(to   , newstr, PDB_CHANGED);

		SAFE_FREE(newstr);
	}
	
	if (from->fields_present & ACCT_RID) {
		if (from->user_rid == 0) {
			DEBUG(10, ("INFO_23: Asked to set User RID to 0 !? Skipping change!\n"));
		} else if (from->user_rid != pdb_get_user_rid(to)) {
			DEBUG(10,("INFO_23 USER_RID: %u -> %u NOT UPDATED!\n",pdb_get_user_rid(to),from->user_rid));
		}
	}

	if (from->fields_present & ACCT_PRIMARY_GID) {
		if  (from->group_rid == 0) {
			DEBUG(10, ("INFO_23: Asked to set Group RID to 0 !? Skipping change!\n"));
		} else if (from->group_rid != pdb_get_group_rid(to)) {
			DEBUG(10,("INFO_23 GROUP_RID: %u -> %u\n",pdb_get_group_rid(to),from->group_rid));
			pdb_set_group_sid_from_rid(to, from->group_rid, PDB_CHANGED);
		}
	}
	
	if (from->fields_present & ACCT_FLAGS) {
		DEBUG(10,("INFO_23 ACCT_CTRL: %08X -> %08X\n",pdb_get_acct_ctrl(to),from->acb_info));
		if (from->acb_info != pdb_get_acct_ctrl(to)) {
			pdb_set_acct_ctrl(to, from->acb_info, PDB_CHANGED);
		}
	}

	if (from->fields_present & ACCT_LOGON_HOURS) {
		DEBUG(15,("INFO_23 LOGON_DIVS: %08X -> %08X\n",pdb_get_logon_divs(to),from->logon_divs));
		if (from->logon_divs != pdb_get_logon_divs(to)) {
			pdb_set_logon_divs(to, from->logon_divs, PDB_CHANGED);
		}

		DEBUG(15,("INFO_23 LOGON_HRS.LEN: %08X -> %08X\n",pdb_get_hours_len(to),from->logon_hrs.len));
		if (from->logon_hrs.len != pdb_get_hours_len(to)) {
			pdb_set_hours_len(to, from->logon_hrs.len, PDB_CHANGED);
		}

		DEBUG(15,("INFO_23 LOGON_HRS.HOURS: %s -> %s\n",pdb_get_hours(to),from->logon_hrs.hours));
		/* Fix me: only update if it changes --metze */
		pdb_set_hours(to, from->logon_hrs.hours, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_BAD_PWD_COUNT) {
		DEBUG(10,("INFO_23 BAD_PASSWORD_COUNT: %08X -> %08X\n",pdb_get_bad_password_count(to),from->bad_password_count));
		if (from->bad_password_count != pdb_get_bad_password_count(to)) {
			pdb_set_bad_password_count(to, from->bad_password_count, PDB_CHANGED);
		}
	}

	if (from->fields_present & ACCT_NUM_LOGONS) {
		DEBUG(10,("INFO_23 LOGON_COUNT: %08X -> %08X\n",pdb_get_logon_count(to),from->logon_count));
		if (from->logon_count != pdb_get_logon_count(to)) {
			pdb_set_logon_count(to, from->logon_count, PDB_CHANGED);
		}
	}

	DEBUG(10,("INFO_23 PASS_MUST_CHANGE_AT_NEXT_LOGON: %02X\n",from->passmustchange));
	if (from->passmustchange==PASS_MUST_CHANGE_AT_NEXT_LOGON) {
		pdb_set_pass_must_change_time(to,0, PDB_CHANGED);		
	} else {
		uint32 expire;
		time_t new_time;
		if (pdb_get_pass_must_change_time(to) == 0) {
			if (!pdb_get_account_policy(AP_MAX_PASSWORD_AGE, &expire)
			    || expire == (uint32)-1) {
				new_time = get_time_t_max();
			} else {
				time_t old_time = pdb_get_pass_last_set_time(to);
				new_time = old_time + expire;
				if ((new_time) < time(0)) {
					new_time = time(0) + expire;
				}
			}
			if (!pdb_set_pass_must_change_time (to, new_time, PDB_CHANGED)) {
				DEBUG (0, ("pdb_set_pass_must_change_time failed!\n"));
			}
		}
	}

	DEBUG(10,("INFO_23 PADDING_2: %02X\n",from->padding2));
}

/*************************************************************
 Copies a SAM_USER_INFO_25 to a struct samu
**************************************************************/

void copy_id25_to_sam_passwd(struct samu *to, SAM_USER_INFO_25 *from)
{
	time_t unix_time, stored_time;
	const char *old_string, *new_string;
	DATA_BLOB mung;

	if (from == NULL || to == NULL) 
		return;

	if (from->fields_present & ACCT_LAST_LOGON) {
		unix_time=nt_time_to_unix(&from->logon_time);
		stored_time = pdb_get_logon_time(to);
		DEBUG(10,("INFO_25 LOGON_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logon_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_LOGOFF) {
		unix_time=nt_time_to_unix(&from->logoff_time);
		stored_time = pdb_get_logoff_time(to);
		DEBUG(10,("INFO_25 LOGOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_logoff_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_EXPIRY) {
		unix_time=nt_time_to_unix(&from->kickoff_time);
		stored_time = pdb_get_kickoff_time(to);
		DEBUG(10,("INFO_25 KICKOFF_TIME: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_kickoff_time(to, unix_time , PDB_CHANGED);
	}	

	if (from->fields_present & ACCT_ALLOW_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_can_change_time);
		stored_time = pdb_get_pass_can_change_time(to);
		DEBUG(10,("INFO_25 PASS_CAN_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_can_change_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_LAST_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_last_set_time);
		stored_time = pdb_get_pass_last_set_time(to);
		DEBUG(10,("INFO_25 PASS_LAST_SET: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_last_set_time(to, unix_time, PDB_CHANGED);
	}

	if (from->fields_present & ACCT_FORCE_PWD_CHANGE) {
		unix_time=nt_time_to_unix(&from->pass_must_change_time);
		stored_time=pdb_get_pass_must_change_time(to);
		DEBUG(10,("INFO_25 PASS_MUST_CH: %lu -> %lu\n",(long unsigned int)stored_time, (long unsigned int)unix_time));
		if (stored_time != unix_time) 
			pdb_set_pass_must_change_time(to, unix_time, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_USERNAME) &&
	    (from->hdr_user_name.buffer)) {
		old_string = pdb_get_username(to);
		new_string = unistr2_static(&from->uni_user_name);
		DEBUG(10,("INFO_25 UNI_USER_NAME: %s -> %s\n", old_string, new_string));
		if (STRING_CHANGED)
		    pdb_set_username(to      , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_FULL_NAME) &&
	    (from->hdr_full_name.buffer)) {
		old_string = pdb_get_fullname(to);
		new_string = unistr2_static(&from->uni_full_name);
		DEBUG(10,("INFO_25 UNI_FULL_NAME: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_fullname(to      , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_HOME_DIR) &&
	    (from->hdr_home_dir.buffer)) {
		old_string = pdb_get_homedir(to);
		new_string = unistr2_static(&from->uni_home_dir);
		DEBUG(10,("INFO_25 UNI_HOME_DIR: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_homedir(to       , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_HOME_DRIVE) &&
	    (from->hdr_dir_drive.buffer)) {
		old_string = pdb_get_dir_drive(to);
		new_string = unistr2_static(&from->uni_dir_drive);
		DEBUG(10,("INFO_25 UNI_DIR_DRIVE: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_dir_drive(to     , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_LOGON_SCRIPT) &&
	    (from->hdr_logon_script.buffer)) {
		old_string = pdb_get_logon_script(to);
		new_string = unistr2_static(&from->uni_logon_script);
		DEBUG(10,("INFO_25 UNI_LOGON_SCRIPT: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_logon_script(to  , new_string, PDB_CHANGED);
	}

	if ((from->fields_present & ACCT_PROFILE) &&
	    (from->hdr_profile_path.buffer)) {
		old_string = pdb_get_profile_path(to);
		new_string = unistr2_static(&from->uni_profile_path);
		DEBUG(10,("INFO_25 UNI_PROFILE_PATH: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_profile_path(to  , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_DESCRIPTION) &&
	    (from->hdr_acct_desc.buffer)) {
		old_string = pdb_get_acct_desc(to);
		new_string = unistr2_static(&from->uni_acct_desc);
		DEBUG(10,("INFO_25 UNI_ACCT_DESC: %s -> %s\n",old_string,new_string));
		if (STRING_CHANGED)
			pdb_set_acct_desc(to     , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_WORKSTATIONS) &&
	    (from->hdr_workstations.buffer)) {
		old_string = pdb_get_workstations(to);
		new_string = unistr2_static(&from->uni_workstations);
		DEBUG(10,("INFO_25 UNI_WORKSTATIONS: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_workstations(to  , new_string, PDB_CHANGED);
	}

	/* is this right? */
	if ((from->fields_present & ACCT_ADMIN_DESC) &&
	    (from->hdr_unknown_str.buffer)) {
		old_string = pdb_get_unknown_str(to);
		new_string = unistr2_static(&from->uni_unknown_str);
		DEBUG(10,("INFO_25 UNI_UNKNOWN_STR: %s -> %s\n",old_string, new_string));
		if (STRING_CHANGED)
			pdb_set_unknown_str(to   , new_string, PDB_CHANGED);
	}
	
	if ((from->fields_present & ACCT_CALLBACK) &&
	    (from->hdr_munged_dial.buffer)) {
		char *newstr;
		old_string = pdb_get_munged_dial(to);
		mung.length = from->hdr_munged_dial.uni_str_len;
		mung.data = (uint8 *) from->uni_munged_dial.buffer;
		newstr = (mung.length == 0) ?
			NULL : base64_encode_data_blob(mung);
		DEBUG(10,("INFO_25 UNI_MUNGED_DIAL: %s -> %s\n",old_string, newstr));
		if (STRING_CHANGED_NC(old_string,newstr))
			pdb_set_munged_dial(to   , newstr, PDB_CHANGED);

		SAFE_FREE(newstr);
	}
	
	if (from->fields_present & ACCT_RID) {
		if (from->user_rid == 0) {
			DEBUG(10, ("INFO_25: Asked to set User RID to 0 !? Skipping change!\n"));
		} else if (from->user_rid != pdb_get_user_rid(to)) {
			DEBUG(10,("INFO_25 USER_RID: %u -> %u NOT UPDATED!\n",pdb_get_user_rid(to),from->user_rid));
		}
	}
	
	if (from->fields_present & ACCT_PRIMARY_GID) {
		if (from->group_rid == 0) {
			DEBUG(10, ("INFO_25: Asked to set Group RID to 0 !? Skipping change!\n"));
		} else if (from->group_rid != pdb_get_group_rid(to)) {
			DEBUG(10,("INFO_25 GROUP_RID: %u -> %u\n",pdb_get_group_rid(to),from->group_rid));
			pdb_set_group_sid_from_rid(to, from->group_rid, PDB_CHANGED);
		}
	}
	
	if (from->fields_present & ACCT_FLAGS) {
		DEBUG(10,("INFO_25 ACCT_CTRL: %08X -> %08X\n",pdb_get_acct_ctrl(to),from->acb_info));
		if (from->acb_info != pdb_get_acct_ctrl(to)) {
			if (!(from->acb_info & ACB_AUTOLOCK) && (pdb_get_acct_ctrl(to) & ACB_AUTOLOCK)) {
				/* We're unlocking a previously locked user. Reset bad password counts.
				   Patch from Jianliang Lu. <Jianliang.Lu@getronics.com> */
				pdb_set_bad_password_count(to, 0, PDB_CHANGED);
				pdb_set_bad_password_time(to, 0, PDB_CHANGED);
			}
			pdb_set_acct_ctrl(to, from->acb_info, PDB_CHANGED);
		}
	}
}
