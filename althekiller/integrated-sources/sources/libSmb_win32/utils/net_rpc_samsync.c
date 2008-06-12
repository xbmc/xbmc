/* 
   Unix SMB/CIFS implementation.
   dump the remote SAM using rpc samsync operations

   Copyright (C) Andrew Tridgell 2002
   Copyright (C) Tim Potter 2001,2002
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2005
   Modified by Volker Lendecke 2002
   Copyright (C) Jeremy Allison 2005.

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
#include "utils/net.h"

/* uid's and gid's for writing deltas to ldif */
static uint32 ldif_gid = 999;
static uint32 ldif_uid = 999;
/* Keep track of ldap initialization */
static int init_ldap = 1;

static void display_group_mem_info(uint32 rid, SAM_GROUP_MEM_INFO *g)
{
	int i;
	d_printf("Group mem %u: ", rid);
	for (i=0;i<g->num_members;i++) {
		d_printf("%u ", g->rids[i]);
	}
	d_printf("\n");
}

static const char *display_time(NTTIME *nttime)
{
	static fstring string;

	float high;
	float low;
	int sec;
	int days, hours, mins, secs;
	int offset = 1;

	if (nttime->high==0 && nttime->low==0)
		return "Now";

	if (nttime->high==0x80000000 && nttime->low==0)
		return "Never";

	high = 65536;	
	high = high/10000;
	high = high*65536;
	high = high/1000;
	high = high * (~nttime->high);

	low = ~nttime->low;	
	low = low/(1000*1000*10);

	sec=high+low;
	sec+=offset;

	days=sec/(60*60*24);
	hours=(sec - (days*60*60*24)) / (60*60);
	mins=(sec - (days*60*60*24) - (hours*60*60) ) / 60;
	secs=sec - (days*60*60*24) - (hours*60*60) - (mins*60);

	fstr_sprintf(string, "%u days, %u hours, %u minutes, %u seconds", days, hours, mins, secs);
	return (string);
}


static void display_alias_info(uint32 rid, SAM_ALIAS_INFO *a)
{
	d_printf("Alias '%s' ", unistr2_static(&a->uni_als_name));
	d_printf("desc='%s' rid=%u\n", unistr2_static(&a->uni_als_desc), a->als_rid);
}

static void display_alias_mem(uint32 rid, SAM_ALIAS_MEM_INFO *a)
{
	int i;
	d_printf("Alias rid %u: ", rid);
	for (i=0;i<a->num_members;i++) {
		d_printf("%s ", sid_string_static(&a->sids[i].sid));
	}
	d_printf("\n");
}

static void display_account_info(uint32 rid, SAM_ACCOUNT_INFO *a)
{
	fstring hex_nt_passwd, hex_lm_passwd;
	uchar lm_passwd[16], nt_passwd[16];
	static uchar zero_buf[16];

	/* Decode hashes from password hash (if they are not NULL) */
	
	if (memcmp(a->pass.buf_lm_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(a->user_rid, a->pass.buf_lm_pwd, lm_passwd, 0);
		pdb_sethexpwd(hex_lm_passwd, lm_passwd, a->acb_info);
	} else {
		pdb_sethexpwd(hex_lm_passwd, NULL, 0);
	}

	if (memcmp(a->pass.buf_nt_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(a->user_rid, a->pass.buf_nt_pwd, nt_passwd, 0);
		pdb_sethexpwd(hex_nt_passwd, nt_passwd, a->acb_info);
	} else {
		pdb_sethexpwd(hex_nt_passwd, NULL, 0);
	}
	
	printf("%s:%d:%s:%s:%s:LCT-0\n", unistr2_static(&a->uni_acct_name),
	       a->user_rid, hex_lm_passwd, hex_nt_passwd,
	       pdb_encode_acct_ctrl(a->acb_info, NEW_PW_FORMAT_SPACE_PADDED_LEN));
}

static void display_domain_info(SAM_DOMAIN_INFO *a)
{
	time_t u_logout;

	u_logout = nt_time_to_unix_abs((NTTIME *)&a->force_logoff);

	d_printf("Domain name: %s\n", unistr2_static(&a->uni_dom_name));

	d_printf("Minimal Password Length: %d\n", a->min_pwd_len);
	d_printf("Password History Length: %d\n", a->pwd_history_len);

	d_printf("Force Logoff: %d\n", (int)u_logout);

	d_printf("Max Password Age: %s\n", display_time((NTTIME *)&a->max_pwd_age));
	d_printf("Min Password Age: %s\n", display_time((NTTIME *)&a->min_pwd_age));

	d_printf("Lockout Time: %s\n", display_time((NTTIME *)&a->account_lockout.lockout_duration));
	d_printf("Lockout Reset Time: %s\n", display_time((NTTIME *)&a->account_lockout.reset_count));

	d_printf("Bad Attempt Lockout: %d\n", a->account_lockout.bad_attempt_lockout);
	d_printf("User must logon to change password: %d\n", a->logon_chgpass);
}

static void display_group_info(uint32 rid, SAM_GROUP_INFO *a)
{
	d_printf("Group '%s' ", unistr2_static(&a->uni_grp_name));
	d_printf("desc='%s', rid=%u\n", unistr2_static(&a->uni_grp_desc), rid);
}

static void display_sam_entry(SAM_DELTA_HDR *hdr_delta, SAM_DELTA_CTR *delta)
{
	switch (hdr_delta->type) {
	case SAM_DELTA_ACCOUNT_INFO:
		display_account_info(hdr_delta->target_rid, &delta->account_info);
		break;
	case SAM_DELTA_GROUP_MEM:
		display_group_mem_info(hdr_delta->target_rid, &delta->grp_mem_info);
		break;
	case SAM_DELTA_ALIAS_INFO:
		display_alias_info(hdr_delta->target_rid, &delta->alias_info);
		break;
	case SAM_DELTA_ALIAS_MEM:
		display_alias_mem(hdr_delta->target_rid, &delta->als_mem_info);
		break;
	case SAM_DELTA_DOMAIN_INFO:
		display_domain_info(&delta->domain_info);
		break;
	case SAM_DELTA_GROUP_INFO:
		display_group_info(hdr_delta->target_rid, &delta->group_info);
		break;
		/* The following types are recognised but not handled */
	case SAM_DELTA_RENAME_GROUP:
		d_printf("SAM_DELTA_RENAME_GROUP not handled\n");
		break;
	case SAM_DELTA_RENAME_USER:
		d_printf("SAM_DELTA_RENAME_USER not handled\n");
		break;
	case SAM_DELTA_RENAME_ALIAS:
		d_printf("SAM_DELTA_RENAME_ALIAS not handled\n");
		break;
	case SAM_DELTA_POLICY_INFO:
		d_printf("SAM_DELTA_POLICY_INFO not handled\n");
		break;
	case SAM_DELTA_TRUST_DOMS:
		d_printf("SAM_DELTA_TRUST_DOMS not handled\n");
		break;
	case SAM_DELTA_PRIVS_INFO:
		d_printf("SAM_DELTA_PRIVS_INFO not handled\n");
		break;
	case SAM_DELTA_SECRET_INFO:
		d_printf("SAM_DELTA_SECRET_INFO not handled\n");
		break;
	case SAM_DELTA_DELETE_GROUP:
		d_printf("SAM_DELTA_DELETE_GROUP not handled\n");
		break;
	case SAM_DELTA_DELETE_USER:
		d_printf("SAM_DELTA_DELETE_USER not handled\n");
		break;
	case SAM_DELTA_MODIFIED_COUNT:
		d_printf("SAM_DELTA_MODIFIED_COUNT not handled\n");
		break;
	default:
		d_printf("Unknown delta record type %d\n", hdr_delta->type);
		break;
	}
}

static void dump_database(struct rpc_pipe_client *pipe_hnd, uint32 db_type)
{
	uint32 sync_context = 0;
        NTSTATUS result;
	int i;
        TALLOC_CTX *mem_ctx;
        SAM_DELTA_HDR *hdr_deltas;
        SAM_DELTA_CTR *deltas;
        uint32 num_deltas;

	if (!(mem_ctx = talloc_init("dump_database"))) {
		return;
	}

	switch( db_type ) {
	case SAM_DATABASE_DOMAIN:
		d_printf("Dumping DOMAIN database\n");
		break;
	case SAM_DATABASE_BUILTIN:
		d_printf("Dumping BUILTIN database\n");
		break;
	case SAM_DATABASE_PRIVS:
		d_printf("Dumping PRIVS databases\n");
		break;
	default:
		d_printf("Dumping unknown database type %u\n", db_type );
		break;
	}

	do {
		result = rpccli_netlogon_sam_sync(pipe_hnd, mem_ctx, db_type,
					       sync_context,
					       &num_deltas, &hdr_deltas, &deltas);
		if (NT_STATUS_IS_ERR(result))
			break;

                for (i = 0; i < num_deltas; i++) {
			display_sam_entry(&hdr_deltas[i], &deltas[i]);
                }
		sync_context += 1;
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	talloc_destroy(mem_ctx);
}

/* dump sam database via samsync rpc calls */
NTSTATUS rpc_samdump_internals(const DOM_SID *domain_sid, 
				const char *domain_name, 
				struct cli_state *cli,
				struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				int argc,
				const char **argv) 
{
#if 0
	/* net_rpc.c now always tries to create an schannel pipe.. */

	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uchar trust_password[16];
	uint32 neg_flags = NETLOGON_NEG_AUTH2_FLAGS;
	uint32 sec_channel_type = 0;

	if (!secrets_fetch_trust_account_password(domain_name,
						  trust_password,
						  NULL, &sec_channel_type)) {
		DEBUG(0,("Could not fetch trust account password\n"));
		goto fail;
	}

	nt_status = rpccli_netlogon_setup_creds(pipe_hnd,
						cli->desthost,
						domain_name,
                                                global_myname(),
                                                trust_password,
                                                sec_channel_type,
                                                &neg_flags);

	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("Error connecting to NETLOGON pipe\n"));
		goto fail;
	}
#endif

	dump_database(pipe_hnd, SAM_DATABASE_DOMAIN);
	dump_database(pipe_hnd, SAM_DATABASE_BUILTIN);
	dump_database(pipe_hnd, SAM_DATABASE_PRIVS);

	return NT_STATUS_OK;
}

/* Convert a struct samu_DELTA to a struct samu. */
#define STRING_CHANGED (old_string && !new_string) ||\
		    (!old_string && new_string) ||\
		(old_string && new_string && (strcmp(old_string, new_string) != 0))

#define STRING_CHANGED_NC(s1,s2) ((s1) && !(s2)) ||\
		    (!(s1) && (s2)) ||\
		((s1) && (s2) && (strcmp((s1), (s2)) != 0))

static NTSTATUS sam_account_from_delta(struct samu *account, SAM_ACCOUNT_INFO *delta)
{
	const char *old_string, *new_string;
	time_t unix_time, stored_time;
	uchar lm_passwd[16], nt_passwd[16];
	static uchar zero_buf[16];

	/* Username, fullname, home dir, dir drive, logon script, acct
	   desc, workstations, profile. */

	if (delta->hdr_acct_name.buffer) {
		old_string = pdb_get_nt_username(account);
		new_string = unistr2_static(&delta->uni_acct_name);

		if (STRING_CHANGED) {
			pdb_set_nt_username(account, new_string, PDB_CHANGED);
              
		}
         
		/* Unix username is the same - for sanity */
		old_string = pdb_get_username( account );
		if (STRING_CHANGED) {
			pdb_set_username(account, new_string, PDB_CHANGED);
		}
	}

	if (delta->hdr_full_name.buffer) {
		old_string = pdb_get_fullname(account);
		new_string = unistr2_static(&delta->uni_full_name);

		if (STRING_CHANGED)
			pdb_set_fullname(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_home_dir.buffer) {
		old_string = pdb_get_homedir(account);
		new_string = unistr2_static(&delta->uni_home_dir);

		if (STRING_CHANGED)
			pdb_set_homedir(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_dir_drive.buffer) {
		old_string = pdb_get_dir_drive(account);
		new_string = unistr2_static(&delta->uni_dir_drive);

		if (STRING_CHANGED)
			pdb_set_dir_drive(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_logon_script.buffer) {
		old_string = pdb_get_logon_script(account);
		new_string = unistr2_static(&delta->uni_logon_script);

		if (STRING_CHANGED)
			pdb_set_logon_script(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_acct_desc.buffer) {
		old_string = pdb_get_acct_desc(account);
		new_string = unistr2_static(&delta->uni_acct_desc);

		if (STRING_CHANGED)
			pdb_set_acct_desc(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_workstations.buffer) {
		old_string = pdb_get_workstations(account);
		new_string = unistr2_static(&delta->uni_workstations);

		if (STRING_CHANGED)
			pdb_set_workstations(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_profile.buffer) {
		old_string = pdb_get_profile_path(account);
		new_string = unistr2_static(&delta->uni_profile);

		if (STRING_CHANGED)
			pdb_set_profile_path(account, new_string, PDB_CHANGED);
	}

	if (delta->hdr_parameters.buffer) {
		DATA_BLOB mung;
		char *newstr;
		old_string = pdb_get_munged_dial(account);
		mung.length = delta->hdr_parameters.uni_str_len;
		mung.data = (uint8 *) delta->uni_parameters.buffer;
		newstr = (mung.length == 0) ? NULL : base64_encode_data_blob(mung);

		if (STRING_CHANGED_NC(old_string, newstr))
			pdb_set_munged_dial(account, newstr, PDB_CHANGED);
		SAFE_FREE(newstr);
	}

	/* User and group sid */
	if (pdb_get_user_rid(account) != delta->user_rid)
		pdb_set_user_sid_from_rid(account, delta->user_rid, PDB_CHANGED);
	if (pdb_get_group_rid(account) != delta->group_rid)
		pdb_set_group_sid_from_rid(account, delta->group_rid, PDB_CHANGED);

	/* Logon and password information */
	if (!nt_time_is_zero(&delta->logon_time)) {
		unix_time = nt_time_to_unix(&delta->logon_time);
		stored_time = pdb_get_logon_time(account);
		if (stored_time != unix_time)
			pdb_set_logon_time(account, unix_time, PDB_CHANGED);
	}

	if (!nt_time_is_zero(&delta->logoff_time)) {
		unix_time = nt_time_to_unix(&delta->logoff_time);
		stored_time = pdb_get_logoff_time(account);
		if (stored_time != unix_time)
			pdb_set_logoff_time(account, unix_time,PDB_CHANGED);
	}

	/* Logon Divs */
	if (pdb_get_logon_divs(account) != delta->logon_divs)
		pdb_set_logon_divs(account, delta->logon_divs, PDB_CHANGED);

	/* Max Logon Hours */
	if (delta->unknown1 != pdb_get_unknown_6(account)) {
		pdb_set_unknown_6(account, delta->unknown1, PDB_CHANGED);
	}

	/* Logon Hours Len */
	if (delta->buf_logon_hrs.buf_len != pdb_get_hours_len(account)) {
		pdb_set_hours_len(account, delta->buf_logon_hrs.buf_len, PDB_CHANGED);
	}

	/* Logon Hours */
	if (delta->buf_logon_hrs.buffer) {
		pstring oldstr, newstr;
		pdb_sethexhours(oldstr, pdb_get_hours(account));
		pdb_sethexhours(newstr, delta->buf_logon_hrs.buffer);
		if (!strequal(oldstr, newstr))
			pdb_set_hours(account, (const uint8 *)delta->buf_logon_hrs.buffer, PDB_CHANGED);
	}

	if (pdb_get_bad_password_count(account) != delta->bad_pwd_count)
		pdb_set_bad_password_count(account, delta->bad_pwd_count, PDB_CHANGED);

	if (pdb_get_logon_count(account) != delta->logon_count)
		pdb_set_logon_count(account, delta->logon_count, PDB_CHANGED);

	if (!nt_time_is_zero(&delta->pwd_last_set_time)) {
		unix_time = nt_time_to_unix(&delta->pwd_last_set_time);
		stored_time = pdb_get_pass_last_set_time(account);
		if (stored_time != unix_time)
			pdb_set_pass_last_set_time(account, unix_time, PDB_CHANGED);
	} else {
		/* no last set time, make it now */
		pdb_set_pass_last_set_time(account, time(NULL), PDB_CHANGED);
	}

#if 0
/*	No kickoff time in the delta? */
	if (!nt_time_is_zero(&delta->kickoff_time)) {
		unix_time = nt_time_to_unix(&delta->kickoff_time);
		stored_time = pdb_get_kickoff_time(account);
		if (stored_time != unix_time)
			pdb_set_kickoff_time(account, unix_time, PDB_CHANGED);
	}
#endif

	/* Decode hashes from password hash 
	   Note that win2000 may send us all zeros for the hashes if it doesn't 
	   think this channel is secure enough - don't set the passwords at all
	   in that case
	*/
	if (memcmp(delta->pass.buf_lm_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(delta->user_rid, delta->pass.buf_lm_pwd, lm_passwd, 0);
		pdb_set_lanman_passwd(account, lm_passwd, PDB_CHANGED);
	}

	if (memcmp(delta->pass.buf_nt_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(delta->user_rid, delta->pass.buf_nt_pwd, nt_passwd, 0);
		pdb_set_nt_passwd(account, nt_passwd, PDB_CHANGED);
	}

	/* TODO: account expiry time */

	pdb_set_acct_ctrl(account, delta->acb_info, PDB_CHANGED);

	pdb_set_domain(account, lp_workgroup(), PDB_CHANGED);

	return NT_STATUS_OK;
}

static NTSTATUS fetch_account_info(uint32 rid, SAM_ACCOUNT_INFO *delta)
{
	NTSTATUS nt_ret = NT_STATUS_UNSUCCESSFUL;
	fstring account;
	pstring add_script;
	struct samu *sam_account=NULL;
	GROUP_MAP map;
	struct group *grp;
	DOM_SID user_sid;
	DOM_SID group_sid;
	struct passwd *passwd;
	fstring sid_string;

	fstrcpy(account, unistr2_static(&delta->uni_acct_name));
	d_printf("Creating account: %s\n", account);

	if ( !(sam_account = samu_new( NULL )) ) {
		return NT_STATUS_NO_MEMORY;
	}

	if (!(passwd = Get_Pwnam(account))) {
		/* Create appropriate user */
		if (delta->acb_info & ACB_NORMAL) {
			pstrcpy(add_script, lp_adduser_script());
		} else if ( (delta->acb_info & ACB_WSTRUST) ||
			    (delta->acb_info & ACB_SVRTRUST) ||
			    (delta->acb_info & ACB_DOMTRUST) ) {
			pstrcpy(add_script, lp_addmachine_script());
		} else {
			DEBUG(1, ("Unknown user type: %s\n",
				  pdb_encode_acct_ctrl(delta->acb_info, NEW_PW_FORMAT_SPACE_PADDED_LEN)));
			nt_ret = NT_STATUS_UNSUCCESSFUL;
			goto done;
		}
		if (*add_script) {
			int add_ret;
			all_string_sub(add_script, "%u", account,
				       sizeof(account));
			add_ret = smbrun(add_script,NULL);
			DEBUG(add_ret ? 0 : 1,("fetch_account: Running the command `%s' "
				 "gave %d\n", add_script, add_ret));
			if (add_ret == 0) {
				smb_nscd_flush_user_cache();
			}
		}
		
		/* try and find the possible unix account again */
		if ( !(passwd = Get_Pwnam(account)) ) {
			d_fprintf(stderr, "Could not create posix account info for '%s'\n", account);
			nt_ret = NT_STATUS_NO_SUCH_USER;
			goto done;
		}
	}
	
	sid_copy(&user_sid, get_global_sam_sid());
	sid_append_rid(&user_sid, delta->user_rid);

	DEBUG(3, ("Attempting to find SID %s for user %s in the passdb\n", sid_to_string(sid_string, &user_sid), account));
	if (!pdb_getsampwsid(sam_account, &user_sid)) {
		sam_account_from_delta(sam_account, delta);
		DEBUG(3, ("Attempting to add user SID %s for user %s in the passdb\n", 
			  sid_to_string(sid_string, &user_sid), pdb_get_username(sam_account)));
		if (!NT_STATUS_IS_OK(pdb_add_sam_account(sam_account))) {
			DEBUG(1, ("SAM Account for %s failed to be added to the passdb!\n",
				  account));
			return NT_STATUS_ACCESS_DENIED; 
		}
	} else {
		sam_account_from_delta(sam_account, delta);
		DEBUG(3, ("Attempting to update user SID %s for user %s in the passdb\n", 
			  sid_to_string(sid_string, &user_sid), pdb_get_username(sam_account)));
		if (!NT_STATUS_IS_OK(pdb_update_sam_account(sam_account))) {
			DEBUG(1, ("SAM Account for %s failed to be updated in the passdb!\n",
				  account));
			TALLOC_FREE(sam_account);
			return NT_STATUS_ACCESS_DENIED; 
		}
	}

	if (pdb_get_group_sid(sam_account) == NULL) {
		return NT_STATUS_UNSUCCESSFUL;
	}

	group_sid = *pdb_get_group_sid(sam_account);

	if (!pdb_getgrsid(&map, group_sid)) {
		DEBUG(0, ("Primary group of %s has no mapping!\n",
			  pdb_get_username(sam_account)));
	} else {
		if (map.gid != passwd->pw_gid) {
			if (!(grp = getgrgid(map.gid))) {
				DEBUG(0, ("Could not find unix group %lu for user %s (group SID=%s)\n", 
					  (unsigned long)map.gid, pdb_get_username(sam_account), sid_string_static(&group_sid)));
			} else {
				smb_set_primary_group(grp->gr_name, pdb_get_username(sam_account));
			}
		}
	}	

	if ( !passwd ) {
		DEBUG(1, ("No unix user for this account (%s), cannot adjust mappings\n", 
			pdb_get_username(sam_account)));
	}

 done:
	TALLOC_FREE(sam_account);
	return nt_ret;
}

static NTSTATUS fetch_group_info(uint32 rid, SAM_GROUP_INFO *delta)
{
	fstring name;
	fstring comment;
	struct group *grp = NULL;
	DOM_SID group_sid;
	fstring sid_string;
	GROUP_MAP map;
	BOOL insert = True;

	unistr2_to_ascii(name, &delta->uni_grp_name, sizeof(name)-1);
	unistr2_to_ascii(comment, &delta->uni_grp_desc, sizeof(comment)-1);

	/* add the group to the mapping table */
	sid_copy(&group_sid, get_global_sam_sid());
	sid_append_rid(&group_sid, rid);
	sid_to_string(sid_string, &group_sid);

	if (pdb_getgrsid(&map, group_sid)) {
		if ( map.gid != -1 )
			grp = getgrgid(map.gid);
		insert = False;
	}

	if (grp == NULL) {
		gid_t gid;

		/* No group found from mapping, find it from its name. */
		if ((grp = getgrnam(name)) == NULL) {
		
			/* No appropriate group found, create one */
			
			d_printf("Creating unix group: '%s'\n", name);
			
			if (smb_create_group(name, &gid) != 0)
				return NT_STATUS_ACCESS_DENIED;
				
			if ((grp = getgrnam(name)) == NULL)
				return NT_STATUS_ACCESS_DENIED;
		}
	}

	map.gid = grp->gr_gid;
	map.sid = group_sid;
	map.sid_name_use = SID_NAME_DOM_GRP;
	fstrcpy(map.nt_name, name);
	if (delta->hdr_grp_desc.buffer) {
		fstrcpy(map.comment, comment);
	} else {
		fstrcpy(map.comment, "");
	}

	if (insert)
		pdb_add_group_mapping_entry(&map);
	else
		pdb_update_group_mapping_entry(&map);

	return NT_STATUS_OK;
}

static NTSTATUS fetch_group_mem_info(uint32 rid, SAM_GROUP_MEM_INFO *delta)
{
	int i;
	TALLOC_CTX *t = NULL;
	char **nt_members = NULL;
	char **unix_members;
	DOM_SID group_sid;
	GROUP_MAP map;
	struct group *grp;

	if (delta->num_members == 0) {
		return NT_STATUS_OK;
	}

	sid_copy(&group_sid, get_global_sam_sid());
	sid_append_rid(&group_sid, rid);

	if (!get_domain_group_from_sid(group_sid, &map)) {
		DEBUG(0, ("Could not find global group %d\n", rid));
		return NT_STATUS_NO_SUCH_GROUP;
	}

	if (!(grp = getgrgid(map.gid))) {
		DEBUG(0, ("Could not find unix group %lu\n", (unsigned long)map.gid));
		return NT_STATUS_NO_SUCH_GROUP;
	}

	d_printf("Group members of %s: ", grp->gr_name);

	if (!(t = talloc_init("fetch_group_mem_info"))) {
		DEBUG(0, ("could not talloc_init\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if ((nt_members = TALLOC_ZERO_ARRAY(t, char *, delta->num_members)) == NULL) {
		DEBUG(0, ("talloc failed\n"));
		talloc_free(t);
		return NT_STATUS_NO_MEMORY;
	}

	for (i=0; i<delta->num_members; i++) {
		struct samu *member = NULL;
		DOM_SID member_sid;

		if ( !(member = samu_new(t)) ) {
			talloc_destroy(t);
			return NT_STATUS_NO_MEMORY;
		}

		sid_copy(&member_sid, get_global_sam_sid());
		sid_append_rid(&member_sid, delta->rids[i]);

		if (!pdb_getsampwsid(member, &member_sid)) {
			DEBUG(1, ("Found bogus group member: %d (member_sid=%s group=%s)\n",
				  delta->rids[i], sid_string_static(&member_sid), grp->gr_name));
			TALLOC_FREE(member);
			continue;
		}

		if (pdb_get_group_rid(member) == rid) {
			d_printf("%s(primary),", pdb_get_username(member));
			TALLOC_FREE(member);
			continue;
		}
		
		d_printf("%s,", pdb_get_username(member));
		nt_members[i] = talloc_strdup(t, pdb_get_username(member));
		TALLOC_FREE(member);
	}

	d_printf("\n");

	unix_members = grp->gr_mem;

	while (*unix_members) {
		BOOL is_nt_member = False;
		for (i=0; i<delta->num_members; i++) {
			if (nt_members[i] == NULL) {
				/* This was a primary group */
				continue;
			}

			if (strcmp(*unix_members, nt_members[i]) == 0) {
				is_nt_member = True;
				break;
			}
		}
		if (!is_nt_member) {
			/* We look at a unix group member that is not
			   an nt group member. So, remove it. NT is
			   boss here. */
			smb_delete_user_group(grp->gr_name, *unix_members);
		}
		unix_members += 1;
	}

	for (i=0; i<delta->num_members; i++) {
		BOOL is_unix_member = False;

		if (nt_members[i] == NULL) {
			/* This was the primary group */
			continue;
		}

		unix_members = grp->gr_mem;

		while (*unix_members) {
			if (strcmp(*unix_members, nt_members[i]) == 0) {
				is_unix_member = True;
				break;
			}
			unix_members += 1;
		}

		if (!is_unix_member) {
			/* We look at a nt group member that is not a
                           unix group member currently. So, add the nt
                           group member. */
			smb_add_user_group(grp->gr_name, nt_members[i]);
		}
	}
	
	talloc_destroy(t);
	return NT_STATUS_OK;
}

static NTSTATUS fetch_alias_info(uint32 rid, SAM_ALIAS_INFO *delta,
				 DOM_SID dom_sid)
{
	fstring name;
	fstring comment;
	struct group *grp = NULL;
	DOM_SID alias_sid;
	fstring sid_string;
	GROUP_MAP map;
	BOOL insert = True;

	unistr2_to_ascii(name, &delta->uni_als_name, sizeof(name)-1);
	unistr2_to_ascii(comment, &delta->uni_als_desc, sizeof(comment)-1);

	/* Find out whether the group is already mapped */
	sid_copy(&alias_sid, &dom_sid);
	sid_append_rid(&alias_sid, rid);
	sid_to_string(sid_string, &alias_sid);

	if (pdb_getgrsid(&map, alias_sid)) {
		grp = getgrgid(map.gid);
		insert = False;
	}

	if (grp == NULL) {
		gid_t gid;

		/* No group found from mapping, find it from its name. */
		if ((grp = getgrnam(name)) == NULL) {
			/* No appropriate group found, create one */
			d_printf("Creating unix group: '%s'\n", name);
			if (smb_create_group(name, &gid) != 0)
				return NT_STATUS_ACCESS_DENIED;
			if ((grp = getgrgid(gid)) == NULL)
				return NT_STATUS_ACCESS_DENIED;
		}
	}

	map.gid = grp->gr_gid;
	map.sid = alias_sid;

	if (sid_equal(&dom_sid, &global_sid_Builtin))
		map.sid_name_use = SID_NAME_WKN_GRP;
	else
		map.sid_name_use = SID_NAME_ALIAS;

	fstrcpy(map.nt_name, name);
	fstrcpy(map.comment, comment);

	if (insert)
		pdb_add_group_mapping_entry(&map);
	else
		pdb_update_group_mapping_entry(&map);

	return NT_STATUS_OK;
}

static NTSTATUS fetch_alias_mem(uint32 rid, SAM_ALIAS_MEM_INFO *delta, DOM_SID dom_sid)
{
	return NT_STATUS_OK;
}

static NTSTATUS fetch_domain_info(uint32 rid, SAM_DOMAIN_INFO *delta)
{
	time_t u_max_age, u_min_age, u_logout, u_lockoutreset, u_lockouttime;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	pstring domname;

	u_max_age = nt_time_to_unix_abs((NTTIME *)&delta->max_pwd_age);
	u_min_age = nt_time_to_unix_abs((NTTIME *)&delta->min_pwd_age);
	u_logout = nt_time_to_unix_abs((NTTIME *)&delta->force_logoff);
	u_lockoutreset = nt_time_to_unix_abs((NTTIME *)&delta->account_lockout.reset_count);
	u_lockouttime = nt_time_to_unix_abs((NTTIME *)&delta->account_lockout.lockout_duration);

	unistr2_to_ascii(domname, &delta->uni_dom_name, sizeof(domname) - 1);

	/* we don't handle BUILTIN account policies */	
	if (!strequal(domname, get_global_sam_name())) {
		printf("skipping SAM_DOMAIN_INFO delta for '%s' (is not my domain)\n", domname);
		return NT_STATUS_OK;
	}


	if (!pdb_set_account_policy(AP_PASSWORD_HISTORY, delta->pwd_history_len))
		return nt_status;

	if (!pdb_set_account_policy(AP_MIN_PASSWORD_LEN, delta->min_pwd_len))
		return nt_status;

	if (!pdb_set_account_policy(AP_MAX_PASSWORD_AGE, (uint32)u_max_age))
		return nt_status;

	if (!pdb_set_account_policy(AP_MIN_PASSWORD_AGE, (uint32)u_min_age))
		return nt_status;

	if (!pdb_set_account_policy(AP_TIME_TO_LOGOUT, (uint32)u_logout))
		return nt_status;

	if (!pdb_set_account_policy(AP_BAD_ATTEMPT_LOCKOUT, delta->account_lockout.bad_attempt_lockout))
		return nt_status;

	if (!pdb_set_account_policy(AP_RESET_COUNT_TIME, (uint32)u_lockoutreset/60))
		return nt_status;

	if (u_lockouttime != -1)
		u_lockouttime /= 60;

	if (!pdb_set_account_policy(AP_LOCK_ACCOUNT_DURATION, (uint32)u_lockouttime))
		return nt_status;

	if (!pdb_set_account_policy(AP_USER_MUST_LOGON_TO_CHG_PASS, delta->logon_chgpass))
		return nt_status;

	return NT_STATUS_OK;
}


static void fetch_sam_entry(SAM_DELTA_HDR *hdr_delta, SAM_DELTA_CTR *delta,
		DOM_SID dom_sid)
{
	switch(hdr_delta->type) {
	case SAM_DELTA_ACCOUNT_INFO:
		fetch_account_info(hdr_delta->target_rid,
				   &delta->account_info);
		break;
	case SAM_DELTA_GROUP_INFO:
		fetch_group_info(hdr_delta->target_rid,
				 &delta->group_info);
		break;
	case SAM_DELTA_GROUP_MEM:
		fetch_group_mem_info(hdr_delta->target_rid,
				     &delta->grp_mem_info);
		break;
	case SAM_DELTA_ALIAS_INFO:
		fetch_alias_info(hdr_delta->target_rid,
				 &delta->alias_info, dom_sid);
		break;
	case SAM_DELTA_ALIAS_MEM:
		fetch_alias_mem(hdr_delta->target_rid,
				&delta->als_mem_info, dom_sid);
		break;
	case SAM_DELTA_DOMAIN_INFO:
		fetch_domain_info(hdr_delta->target_rid,
				&delta->domain_info);
		break;
	/* The following types are recognised but not handled */
	case SAM_DELTA_RENAME_GROUP:
		d_printf("SAM_DELTA_RENAME_GROUP not handled\n");
		break;
	case SAM_DELTA_RENAME_USER:
		d_printf("SAM_DELTA_RENAME_USER not handled\n");
		break;
	case SAM_DELTA_RENAME_ALIAS:
		d_printf("SAM_DELTA_RENAME_ALIAS not handled\n");
		break;
	case SAM_DELTA_POLICY_INFO:
		d_printf("SAM_DELTA_POLICY_INFO not handled\n");
		break;
	case SAM_DELTA_TRUST_DOMS:
		d_printf("SAM_DELTA_TRUST_DOMS not handled\n");
		break;
	case SAM_DELTA_PRIVS_INFO:
		d_printf("SAM_DELTA_PRIVS_INFO not handled\n");
		break;
	case SAM_DELTA_SECRET_INFO:
		d_printf("SAM_DELTA_SECRET_INFO not handled\n");
		break;
	case SAM_DELTA_DELETE_GROUP:
		d_printf("SAM_DELTA_DELETE_GROUP not handled\n");
		break;
	case SAM_DELTA_DELETE_USER:
		d_printf("SAM_DELTA_DELETE_USER not handled\n");
		break;
	case SAM_DELTA_MODIFIED_COUNT:
		d_printf("SAM_DELTA_MODIFIED_COUNT not handled\n");
		break;
	default:
		d_printf("Unknown delta record type %d\n", hdr_delta->type);
		break;
	}
}

static NTSTATUS fetch_database(struct rpc_pipe_client *pipe_hnd, uint32 db_type, DOM_SID dom_sid)
{
	uint32 sync_context = 0;
        NTSTATUS result;
	int i;
        TALLOC_CTX *mem_ctx;
        SAM_DELTA_HDR *hdr_deltas;
        SAM_DELTA_CTR *deltas;
        uint32 num_deltas;

	if (!(mem_ctx = talloc_init("fetch_database")))
		return NT_STATUS_NO_MEMORY;

	switch( db_type ) {
	case SAM_DATABASE_DOMAIN:
		d_printf("Fetching DOMAIN database\n");
		break;
	case SAM_DATABASE_BUILTIN:
		d_printf("Fetching BUILTIN database\n");
		break;
	case SAM_DATABASE_PRIVS:
		d_printf("Fetching PRIVS databases\n");
		break;
	default:
		d_printf("Fetching unknown database type %u\n", db_type );
		break;
	}

	do {
		result = rpccli_netlogon_sam_sync(pipe_hnd, mem_ctx,
					       db_type, sync_context,
					       &num_deltas,
					       &hdr_deltas, &deltas);

		if (NT_STATUS_IS_OK(result) ||
		    NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES)) {
			for (i = 0; i < num_deltas; i++) {
				fetch_sam_entry(&hdr_deltas[i], &deltas[i], dom_sid);
			}
		} else
			return result;

		sync_context += 1;
	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	talloc_destroy(mem_ctx);

	return result;
}

static NTSTATUS populate_ldap_for_ldif(fstring sid, const char *suffix, const char 
		       *builtin_sid, FILE *add_fd)
{
	const char *user_suffix, *group_suffix, *machine_suffix, *idmap_suffix;
	char *user_attr=NULL, *group_attr=NULL;
	char *suffix_attr;
	int len;

	/* Get the suffix attribute */
	suffix_attr = sstring_sub(suffix, '=', ',');
	if (suffix_attr == NULL) {
		len = strlen(suffix);
		suffix_attr = (char*)SMB_MALLOC(len+1);
		memcpy(suffix_attr, suffix, len);
		suffix_attr[len] = '\0';
	}

	/* Write the base */
	fprintf(add_fd, "# %s\n", suffix);
	fprintf(add_fd, "dn: %s\n", suffix);
	fprintf(add_fd, "objectClass: dcObject\n");
	fprintf(add_fd, "objectClass: organization\n");
	fprintf(add_fd, "o: %s\n", suffix_attr);
	fprintf(add_fd, "dc: %s\n", suffix_attr);
	fprintf(add_fd, "\n");
	fflush(add_fd);

	user_suffix = lp_ldap_user_suffix();
	if (user_suffix == NULL) {
		SAFE_FREE(suffix_attr);
		return NT_STATUS_NO_MEMORY;
	}
	/* If it exists and is distinct from other containers, 
	   Write the Users entity */
	if (*user_suffix && strcmp(user_suffix, suffix)) {
		user_attr = sstring_sub(lp_ldap_user_suffix(), '=', ',');
		fprintf(add_fd, "# %s\n", user_suffix);
		fprintf(add_fd, "dn: %s\n", user_suffix);
		fprintf(add_fd, "objectClass: organizationalUnit\n");
		fprintf(add_fd, "ou: %s\n", user_attr);
		fprintf(add_fd, "\n");
		fflush(add_fd);
	}


	group_suffix = lp_ldap_group_suffix();
	if (group_suffix == NULL) {
		SAFE_FREE(suffix_attr);
		SAFE_FREE(user_attr);
		return NT_STATUS_NO_MEMORY;
	}
	/* If it exists and is distinct from other containers, 
	   Write the Groups entity */
	if (*group_suffix && strcmp(group_suffix, suffix)) {
		group_attr = sstring_sub(lp_ldap_group_suffix(), '=', ',');
		fprintf(add_fd, "# %s\n", group_suffix);
		fprintf(add_fd, "dn: %s\n", group_suffix);
		fprintf(add_fd, "objectClass: organizationalUnit\n");
		fprintf(add_fd, "ou: %s\n", group_attr);
		fprintf(add_fd, "\n");
		fflush(add_fd);
	}

	/* If it exists and is distinct from other containers, 
	   Write the Computers entity */
	machine_suffix = lp_ldap_machine_suffix();
	if (machine_suffix == NULL) {
		SAFE_FREE(suffix_attr);
		SAFE_FREE(user_attr);
		SAFE_FREE(group_attr);
		return NT_STATUS_NO_MEMORY;
	}
	if (*machine_suffix && strcmp(machine_suffix, user_suffix) &&
	    strcmp(machine_suffix, suffix)) {
		char *machine_ou = NULL;
		fprintf(add_fd, "# %s\n", machine_suffix);
		fprintf(add_fd, "dn: %s\n", machine_suffix);
		fprintf(add_fd, "objectClass: organizationalUnit\n");
		/* this isn't totally correct as it assumes that
		   there _must_ be an ou. just fixing memleak now. jmcd */
		machine_ou = sstring_sub(lp_ldap_machine_suffix(), '=', ',');
		fprintf(add_fd, "ou: %s\n", machine_ou);
		SAFE_FREE(machine_ou);
		fprintf(add_fd, "\n");
		fflush(add_fd);
	}

	/* If it exists and is distinct from other containers, 
	   Write the IdMap entity */
	idmap_suffix = lp_ldap_idmap_suffix();
	if (idmap_suffix == NULL) {
		SAFE_FREE(suffix_attr);
		SAFE_FREE(user_attr);
		SAFE_FREE(group_attr);
		return NT_STATUS_NO_MEMORY;
	}
	if (*idmap_suffix &&
	    strcmp(idmap_suffix, user_suffix) &&
	    strcmp(idmap_suffix, suffix)) {
		char *s;
		fprintf(add_fd, "# %s\n", idmap_suffix);
		fprintf(add_fd, "dn: %s\n", idmap_suffix);
		fprintf(add_fd, "ObjectClass: organizationalUnit\n");
		s = sstring_sub(lp_ldap_idmap_suffix(), '=', ',');
		fprintf(add_fd, "ou: %s\n", s);
		SAFE_FREE(s);
		fprintf(add_fd, "\n");
		fflush(add_fd);
	}

	/* Write the domain entity */
	fprintf(add_fd, "# %s, %s\n", lp_workgroup(), suffix);
	fprintf(add_fd, "dn: sambaDomainName=%s,%s\n", lp_workgroup(),
		suffix);
	fprintf(add_fd, "objectClass: sambaDomain\n");
	fprintf(add_fd, "objectClass: sambaUnixIdPool\n");
	fprintf(add_fd, "sambaDomainName: %s\n", lp_workgroup());
	fprintf(add_fd, "sambaSID: %s\n", sid);
	fprintf(add_fd, "uidNumber: %d\n", ++ldif_uid);
	fprintf(add_fd, "gidNumber: %d\n", ++ldif_gid);
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Domain Admins entity */ 
	fprintf(add_fd, "# Domain Admins, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Domain Admins,ou=%s,%s\n", group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "cn: Domain Admins\n");
	fprintf(add_fd, "memberUid: Administrator\n");
	fprintf(add_fd, "description: Netbios Domain Administrators\n");
	fprintf(add_fd, "gidNumber: 512\n");
	fprintf(add_fd, "sambaSID: %s-512\n", sid);
	fprintf(add_fd, "sambaGroupType: 2\n");
	fprintf(add_fd, "displayName: Domain Admins\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Domain Users entity */ 
	fprintf(add_fd, "# Domain Users, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Domain Users,ou=%s,%s\n", group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "cn: Domain Users\n");
	fprintf(add_fd, "description: Netbios Domain Users\n");
	fprintf(add_fd, "gidNumber: 513\n");
	fprintf(add_fd, "sambaSID: %s-513\n", sid);
	fprintf(add_fd, "sambaGroupType: 2\n");
	fprintf(add_fd, "displayName: Domain Users\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Domain Guests entity */ 
	fprintf(add_fd, "# Domain Guests, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Domain Guests,ou=%s,%s\n", group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "cn: Domain Guests\n");
	fprintf(add_fd, "description: Netbios Domain Guests\n");
	fprintf(add_fd, "gidNumber: 514\n");
	fprintf(add_fd, "sambaSID: %s-514\n", sid);
	fprintf(add_fd, "sambaGroupType: 2\n");
	fprintf(add_fd, "displayName: Domain Guests\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Domain Computers entity */
	fprintf(add_fd, "# Domain Computers, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Domain Computers,ou=%s,%s\n",
		group_attr, suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "gidNumber: 515\n");
	fprintf(add_fd, "cn: Domain Computers\n");
	fprintf(add_fd, "description: Netbios Domain Computers accounts\n");
	fprintf(add_fd, "sambaSID: %s-515\n", sid);
	fprintf(add_fd, "sambaGroupType: 2\n");
	fprintf(add_fd, "displayName: Domain Computers\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Admininistrators Groups entity */
	fprintf(add_fd, "# Administrators, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Administrators,ou=%s,%s\n", group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "gidNumber: 544\n");
	fprintf(add_fd, "cn: Administrators\n");
	fprintf(add_fd, "description: Netbios Domain Members can fully administer the computer/sambaDomainName\n");
	fprintf(add_fd, "sambaSID: %s-544\n", builtin_sid);
	fprintf(add_fd, "sambaGroupType: 5\n");
	fprintf(add_fd, "displayName: Administrators\n");
	fprintf(add_fd, "\n");

	/* Write the Print Operator entity */
	fprintf(add_fd, "# Print Operators, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Print Operators,ou=%s,%s\n",
		group_attr, suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "gidNumber: 550\n");
	fprintf(add_fd, "cn: Print Operators\n");
	fprintf(add_fd, "description: Netbios Domain Print Operators\n");
	fprintf(add_fd, "sambaSID: %s-550\n", builtin_sid);
	fprintf(add_fd, "sambaGroupType: 5\n");
	fprintf(add_fd, "displayName: Print Operators\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Backup Operators entity */
	fprintf(add_fd, "# Backup Operators, %s, %s\n", group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=Backup Operators,ou=%s,%s\n",
		group_attr, suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "gidNumber: 551\n");
	fprintf(add_fd, "cn: Backup Operators\n");
	fprintf(add_fd, "description: Netbios Domain Members can bypass file security to back up files\n");
	fprintf(add_fd, "sambaSID: %s-551\n", builtin_sid);
	fprintf(add_fd, "sambaGroupType: 5\n");
	fprintf(add_fd, "displayName: Backup Operators\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Write the Replicators entity */
	fprintf(add_fd, "# Replicators, %s, %s\n", group_attr, suffix);
	fprintf(add_fd, "dn: cn=Replicators,ou=%s,%s\n", group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "gidNumber: 552\n");
	fprintf(add_fd, "cn: Replicators\n");
	fprintf(add_fd, "description: Netbios Domain Supports file replication in a sambaDomainName\n");
	fprintf(add_fd, "sambaSID: %s-552\n", builtin_sid);
	fprintf(add_fd, "sambaGroupType: 5\n");
	fprintf(add_fd, "displayName: Replicators\n");
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Deallocate memory, and return */
	SAFE_FREE(suffix_attr);
	SAFE_FREE(user_attr);
	SAFE_FREE(group_attr);
	return NT_STATUS_OK;
}

static NTSTATUS map_populate_groups(GROUPMAP *groupmap, ACCOUNTMAP *accountmap, fstring sid, 
		    const char *suffix, const char *builtin_sid)
{
	char *group_attr = sstring_sub(lp_ldap_group_suffix(), '=', ',');

	/* Map the groups created by populate_ldap_for_ldif */
	groupmap[0].rid = 512;
	groupmap[0].gidNumber = 512;
	pstr_sprintf(groupmap[0].sambaSID, "%s-512", sid);
	pstr_sprintf(groupmap[0].group_dn, "cn=Domain Admins,ou=%s,%s", 
                     group_attr, suffix);
	accountmap[0].rid = 512;
	pstr_sprintf(accountmap[0].cn, "%s", "Domain Admins");

	groupmap[1].rid = 513;
	groupmap[1].gidNumber = 513;
	pstr_sprintf(groupmap[1].sambaSID, "%s-513", sid);
	pstr_sprintf(groupmap[1].group_dn, "cn=Domain Users,ou=%s,%s", 
                     group_attr, suffix);
	accountmap[1].rid = 513;
	pstr_sprintf(accountmap[1].cn, "%s", "Domain Users");

	groupmap[2].rid = 514;
	groupmap[2].gidNumber = 514;
	pstr_sprintf(groupmap[2].sambaSID, "%s-514", sid);
	pstr_sprintf(groupmap[2].group_dn, "cn=Domain Guests,ou=%s,%s", 
                     group_attr, suffix);
	accountmap[2].rid = 514;
	pstr_sprintf(accountmap[2].cn, "%s", "Domain Guests");

	groupmap[3].rid = 515;
	groupmap[3].gidNumber = 515;
	pstr_sprintf(groupmap[3].sambaSID, "%s-515", sid);
	pstr_sprintf(groupmap[3].group_dn, "cn=Domain Computers,ou=%s,%s",
                     group_attr, suffix);
	accountmap[3].rid = 515;
	pstr_sprintf(accountmap[3].cn, "%s", "Domain Computers");

	groupmap[4].rid = 544;
	groupmap[4].gidNumber = 544;
	pstr_sprintf(groupmap[4].sambaSID, "%s-544", builtin_sid);
	pstr_sprintf(groupmap[4].group_dn, "cn=Administrators,ou=%s,%s",
                     group_attr, suffix);
	accountmap[4].rid = 515;
	pstr_sprintf(accountmap[4].cn, "%s", "Administrators");

	groupmap[5].rid = 550;
	groupmap[5].gidNumber = 550;
	pstr_sprintf(groupmap[5].sambaSID, "%s-550", builtin_sid);
	pstr_sprintf(groupmap[5].group_dn, "cn=Print Operators,ou=%s,%s",
                     group_attr, suffix);
	accountmap[5].rid = 550;
	pstr_sprintf(accountmap[5].cn, "%s", "Print Operators");

	groupmap[6].rid = 551;
	groupmap[6].gidNumber = 551;
	pstr_sprintf(groupmap[6].sambaSID, "%s-551", builtin_sid);
	pstr_sprintf(groupmap[6].group_dn, "cn=Backup Operators,ou=%s,%s",
                     group_attr, suffix);
	accountmap[6].rid = 551;
	pstr_sprintf(accountmap[6].cn, "%s", "Backup Operators");

	groupmap[7].rid = 552;
	groupmap[7].gidNumber = 552;
	pstr_sprintf(groupmap[7].sambaSID, "%s-552", builtin_sid);
	pstr_sprintf(groupmap[7].group_dn, "cn=Replicators,ou=%s,%s",
		     group_attr, suffix);
	accountmap[7].rid = 551;
	pstr_sprintf(accountmap[7].cn, "%s", "Replicators");
	SAFE_FREE(group_attr);
	return NT_STATUS_OK;
}

static NTSTATUS fetch_group_info_to_ldif(SAM_DELTA_CTR *delta, GROUPMAP *groupmap,
			 FILE *add_fd, fstring sid, char *suffix)
{
	fstring groupname;
	uint32 grouptype = 0, g_rid = 0;
	char *group_attr = sstring_sub(lp_ldap_group_suffix(), '=', ',');

	/* Get the group name */
	unistr2_to_ascii(groupname, 
	  		 &(delta->group_info.uni_grp_name),
			 sizeof(groupname)-1);

	/* Set up the group type (always 2 for group info) */
	grouptype = 2;

	/* These groups are entered by populate_ldap_for_ldif */
	if (strcmp(groupname, "Domain Admins") == 0 ||
            strcmp(groupname, "Domain Users") == 0 ||
	    strcmp(groupname, "Domain Guests") == 0 ||
	    strcmp(groupname, "Domain Computers") == 0 ||
	    strcmp(groupname, "Administrators") == 0 ||
	    strcmp(groupname, "Print Operators") == 0 ||
	    strcmp(groupname, "Backup Operators") == 0 ||
	    strcmp(groupname, "Replicators") == 0) {
		SAFE_FREE(group_attr);
		return NT_STATUS_OK;
	} else {
		/* Increment the gid for the new group */
	        ldif_gid++;
	}

	/* Map the group rid, gid, and dn */
	g_rid = delta->group_info.gid.g_rid;
	groupmap->rid = g_rid;
	groupmap->gidNumber = ldif_gid;
	pstr_sprintf(groupmap->sambaSID, "%s-%d", sid, g_rid);
	pstr_sprintf(groupmap->group_dn, 
		     "cn=%s,ou=%s,%s", groupname, group_attr, suffix);

	/* Write the data to the temporary add ldif file */
	fprintf(add_fd, "# %s, %s, %s\n", groupname, group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=%s,ou=%s,%s\n", groupname, group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "cn: %s\n", groupname);
	fprintf(add_fd, "gidNumber: %d\n", ldif_gid);
	fprintf(add_fd, "sambaSID: %s\n", groupmap->sambaSID);
	fprintf(add_fd, "sambaGroupType: %d\n", grouptype);
	fprintf(add_fd, "displayName: %s\n", groupname);
	fprintf(add_fd, "\n");
	fflush(add_fd);

	SAFE_FREE(group_attr);
	/* Return */
	return NT_STATUS_OK;
}

static NTSTATUS fetch_account_info_to_ldif(SAM_DELTA_CTR *delta, GROUPMAP *groupmap,
		   	   ACCOUNTMAP *accountmap, FILE *add_fd,
			   fstring sid, char *suffix, int alloced)
{
	fstring username, logonscript, homedrive, homepath = "", homedir = "";
	fstring hex_nt_passwd, hex_lm_passwd;
	fstring description, fullname, sambaSID;
	uchar lm_passwd[16], nt_passwd[16];
	char *flags, *user_rdn;
	const char* nopasswd = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
	static uchar zero_buf[16];
	uint32 rid = 0, group_rid = 0, gidNumber = 0;
	time_t unix_time;
	int i;

	/* Get the username */
	unistr2_to_ascii(username, 
			 &(delta->account_info.uni_acct_name),
			 sizeof(username)-1);

	/* Get the rid */
	rid = delta->account_info.user_rid;

	/* Map the rid and username for group member info later */
	accountmap->rid = rid;
	pstr_sprintf(accountmap->cn, "%s", username);

	/* Get the home directory */
	if (delta->account_info.acb_info & ACB_NORMAL) {
		unistr2_to_ascii(homedir, &(delta->account_info.uni_home_dir),
				 sizeof(homedir)-1);
		if (!*homedir) {
			pstr_sprintf(homedir, "/home/%s", username);
		} else {
			pstr_sprintf(homedir, "/nobodyshomedir");
		}
	}	

        /* Get the logon script */
        unistr2_to_ascii(logonscript, &(delta->account_info.uni_logon_script),
                        sizeof(logonscript)-1);

        /* Get the home drive */
        unistr2_to_ascii(homedrive, &(delta->account_info.uni_dir_drive),
                        sizeof(homedrive)-1);

	/* Get the description */
	unistr2_to_ascii(description, &(delta->account_info.uni_acct_desc),
			 sizeof(description)-1);
	if (!*description) {
		pstr_sprintf(description, "System User");
	}

	/* Get the display name */
	unistr2_to_ascii(fullname, &(delta->account_info.uni_full_name),
			 sizeof(fullname)-1);

	/* Get lm and nt password data */
	if (memcmp(delta->account_info.pass.buf_lm_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(delta->account_info.user_rid, 
			     delta->account_info.pass.buf_lm_pwd, 
			     lm_passwd, 0);
		pdb_sethexpwd(hex_lm_passwd, lm_passwd, 
			      delta->account_info.acb_info);
	} else {
		pdb_sethexpwd(hex_lm_passwd, NULL, 0);
	}
	if (memcmp(delta->account_info.pass.buf_nt_pwd, zero_buf, 16) != 0) {
		sam_pwd_hash(delta->account_info.user_rid, 
			     delta->account_info.pass.buf_nt_pwd, 
                                     nt_passwd, 0);
		pdb_sethexpwd(hex_nt_passwd, nt_passwd, 
			      delta->account_info.acb_info);
	} else {
		pdb_sethexpwd(hex_nt_passwd, NULL, 0);
	}
	unix_time = nt_time_to_unix(&(delta->account_info.pwd_last_set_time));

	/* The nobody user is entered by populate_ldap_for_ldif */
	if (strcmp(username, "nobody") == 0) {
		return NT_STATUS_OK;
	} else {
		/* Increment the uid for the new user */
		ldif_uid++;
	}

	/* Set up group id and sambaSID for the user */
	group_rid = delta->account_info.group_rid;
	for (i=0; i<alloced; i++) {
		if (groupmap[i].rid == group_rid) break;
	}
	if (i == alloced){
		DEBUG(1, ("Could not find rid %d in groupmap array\n", 
			  group_rid));
		return NT_STATUS_UNSUCCESSFUL;
	}
	gidNumber = groupmap[i].gidNumber;
	pstr_sprintf(sambaSID, groupmap[i].sambaSID);

	/* Set up sambaAcctFlags */
	flags = pdb_encode_acct_ctrl(delta->account_info.acb_info,
				     NEW_PW_FORMAT_SPACE_PADDED_LEN);

	/* Add the user to the temporary add ldif file */
	/* this isn't quite right...we can't assume there's just OU=. jmcd */
	user_rdn = sstring_sub(lp_ldap_user_suffix(), '=', ',');
	fprintf(add_fd, "# %s, %s, %s\n", username, user_rdn, suffix);
	fprintf(add_fd, "dn: uid=%s,ou=%s,%s\n", username, user_rdn, suffix);
	SAFE_FREE(user_rdn);
	fprintf(add_fd, "ObjectClass: top\n");
	fprintf(add_fd, "objectClass: inetOrgPerson\n");
	fprintf(add_fd, "objectClass: posixAccount\n");
	fprintf(add_fd, "objectClass: shadowAccount\n");
	fprintf(add_fd, "objectClass: sambaSamAccount\n");
	fprintf(add_fd, "cn: %s\n", username);
	fprintf(add_fd, "sn: %s\n", username);
	fprintf(add_fd, "uid: %s\n", username);
	fprintf(add_fd, "uidNumber: %d\n", ldif_uid);
	fprintf(add_fd, "gidNumber: %d\n", gidNumber);
	fprintf(add_fd, "homeDirectory: %s\n", homedir);
	if (*homepath)
		fprintf(add_fd, "SambaHomePath: %s\n", homepath);
        if (*homedrive)
                fprintf(add_fd, "SambaHomeDrive: %s\n", homedrive);
        if (*logonscript)
                fprintf(add_fd, "SambaLogonScript: %s\n", logonscript);
	fprintf(add_fd, "loginShell: %s\n", 
		((delta->account_info.acb_info & ACB_NORMAL) ?
		 "/bin/bash" : "/bin/false"));
	fprintf(add_fd, "gecos: System User\n");
	fprintf(add_fd, "description: %s\n", description);
	fprintf(add_fd, "sambaSID: %s-%d\n", sid, rid);
	fprintf(add_fd, "sambaPrimaryGroupSID: %s\n", sambaSID);
	if(*fullname)
		fprintf(add_fd, "displayName: %s\n", fullname);
	if (strcmp(nopasswd, hex_lm_passwd) != 0)
		fprintf(add_fd, "sambaLMPassword: %s\n", hex_lm_passwd);
	if (strcmp(nopasswd, hex_nt_passwd) != 0)
		fprintf(add_fd, "sambaNTPassword: %s\n", hex_nt_passwd);
	fprintf(add_fd, "sambaPwdLastSet: %d\n", (int)unix_time);
	fprintf(add_fd, "sambaAcctFlags: %s\n", flags);
	fprintf(add_fd, "\n");
	fflush(add_fd);

	/* Return */
	return NT_STATUS_OK;
}

static NTSTATUS fetch_alias_info_to_ldif(SAM_DELTA_CTR *delta, GROUPMAP *groupmap,
			 FILE *add_fd, fstring sid, char *suffix, 
			 unsigned db_type)
{
	fstring aliasname, description;
	uint32 grouptype = 0, g_rid = 0;
	char *group_attr = sstring_sub(lp_ldap_group_suffix(), '=', ',');

	/* Get the alias name */
	unistr2_to_ascii(aliasname, &(delta->alias_info.uni_als_name),
			 sizeof(aliasname)-1);

	/* Get the alias description */
	unistr2_to_ascii(description, &(delta->alias_info.uni_als_desc),
			 sizeof(description)-1);

	/* Set up the group type */
	switch (db_type) {
		case SAM_DATABASE_DOMAIN:
			grouptype = 4;
			break;
		case SAM_DATABASE_BUILTIN:
			grouptype = 5;
			break;
		default:
			grouptype = 4;
			break;
	}

	/*
	These groups are entered by populate_ldap_for_ldif
	Note that populate creates a group called Relicators, 
	but NT returns a group called Replicator
	*/
	if (strcmp(aliasname, "Domain Admins") == 0 ||
	    strcmp(aliasname, "Domain Users") == 0 ||
	    strcmp(aliasname, "Domain Guests") == 0 ||
	    strcmp(aliasname, "Domain Computers") == 0 ||
	    strcmp(aliasname, "Administrators") == 0 ||
	    strcmp(aliasname, "Print Operators") == 0 ||
	    strcmp(aliasname, "Backup Operators") == 0 ||
	    strcmp(aliasname, "Replicator") == 0) {
		SAFE_FREE(group_attr);
		return NT_STATUS_OK;
	} else {
		/* Increment the gid for the new group */
		ldif_gid++;
	}

	/* Map the group rid and gid */
	g_rid = delta->group_info.gid.g_rid;
	groupmap->gidNumber = ldif_gid;
	pstr_sprintf(groupmap->sambaSID, "%s-%d", sid, g_rid);

	/* Write the data to the temporary add ldif file */
	fprintf(add_fd, "# %s, %s, %s\n", aliasname, group_attr,
		suffix);
	fprintf(add_fd, "dn: cn=%s,ou=%s,%s\n", aliasname, group_attr,
		suffix);
	fprintf(add_fd, "objectClass: posixGroup\n");
	fprintf(add_fd, "objectClass: sambaGroupMapping\n");
	fprintf(add_fd, "cn: %s\n", aliasname);
	fprintf(add_fd, "gidNumber: %d\n", ldif_gid);
	fprintf(add_fd, "sambaSID: %s\n", groupmap->sambaSID);
	fprintf(add_fd, "sambaGroupType: %d\n", grouptype);
	fprintf(add_fd, "displayName: %s\n", aliasname);
	fprintf(add_fd, "description: %s\n", description);
	fprintf(add_fd, "\n");
	fflush(add_fd);

	SAFE_FREE(group_attr);
	/* Return */
	return NT_STATUS_OK;
}

static NTSTATUS fetch_groupmem_info_to_ldif(SAM_DELTA_CTR *delta, SAM_DELTA_HDR *hdr_delta,
			    GROUPMAP *groupmap, ACCOUNTMAP *accountmap, 
			    FILE *mod_fd, int alloced)
{
	fstring group_dn;
	uint32 group_rid = 0, rid = 0;
	int i, j, k;

	/* Get the dn for the group */
	if (delta->grp_mem_info.num_members > 0) {
		group_rid = hdr_delta->target_rid;
		for (j=0; j<alloced; j++) {
			if (groupmap[j].rid == group_rid) break;
		}
		if (j == alloced){
			DEBUG(1, ("Could not find rid %d in groupmap array\n", 
				  group_rid));
			return NT_STATUS_UNSUCCESSFUL;
		}
		pstr_sprintf(group_dn, "%s", groupmap[j].group_dn);
		fprintf(mod_fd, "dn: %s\n", group_dn);

		/* Get the cn for each member */
		for (i=0; i<delta->grp_mem_info.num_members; i++) {
			rid = delta->grp_mem_info.rids[i];
			for (k=0; k<alloced; k++) {
				if (accountmap[k].rid == rid) break;
			}
			if (k == alloced){
				DEBUG(1, ("Could not find rid %d in accountmap array\n", rid));
				return NT_STATUS_UNSUCCESSFUL;
			}
			fprintf(mod_fd, "memberUid: %s\n", accountmap[k].cn);
		}
		fprintf(mod_fd, "\n");
	}
	fflush(mod_fd);

	/* Return */
	return NT_STATUS_OK;
}

static NTSTATUS fetch_database_to_ldif(struct rpc_pipe_client *pipe_hnd,
					uint32 db_type,
					DOM_SID dom_sid,
					const char *user_file)
{
	char *suffix;
	const char *builtin_sid = "S-1-5-32";
	char *add_name = NULL, *mod_name = NULL;
	const char *add_template = "/tmp/add.ldif.XXXXXX";
	const char *mod_template = "/tmp/mod.ldif.XXXXXX";
	fstring sid, domainname;
	uint32 sync_context = 0;
	NTSTATUS ret = NT_STATUS_OK, result;
	int k;
	TALLOC_CTX *mem_ctx;
	SAM_DELTA_HDR *hdr_deltas;
	SAM_DELTA_CTR *deltas;
	uint32 num_deltas;
	FILE *add_file = NULL, *mod_file = NULL, *ldif_file = NULL;
	int num_alloced = 0, g_index = 0, a_index = 0;

	/* Set up array for mapping accounts to groups */
	/* Array element is the group rid */
	GROUPMAP *groupmap = NULL;

	/* Set up array for mapping account rid's to cn's */
	/* Array element is the account rid */
	ACCOUNTMAP *accountmap = NULL; 

	if (!(mem_ctx = talloc_init("fetch_database"))) {
		return NT_STATUS_NO_MEMORY;
	}

	/* Ensure we have an output file */
	if (user_file)
		ldif_file = fopen(user_file, "a");
	else
		ldif_file = stdout;

	if (!ldif_file) {
		fprintf(stderr, "Could not open %s\n", user_file);
		DEBUG(1, ("Could not open %s\n", user_file));
		ret = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	add_name = talloc_strdup(mem_ctx, add_template);
	mod_name = talloc_strdup(mem_ctx, mod_template);
 	if (!add_name || !mod_name) {
		ret = NT_STATUS_NO_MEMORY;
		goto done;
	}

	/* Open the add and mod ldif files */
	if (!(add_file = fdopen(smb_mkstemp(add_name),"w"))) {
		DEBUG(1, ("Could not open %s\n", add_name));
		ret = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}
	if (!(mod_file = fdopen(smb_mkstemp(mod_name),"w"))) {
		DEBUG(1, ("Could not open %s\n", mod_name));
		ret = NT_STATUS_UNSUCCESSFUL;
		goto done;
	} 

	/* Get the sid */
	sid_to_string(sid, &dom_sid);

	/* Get the ldap suffix */
	suffix = lp_ldap_suffix();
	if (suffix == NULL || strcmp(suffix, "") == 0) {
		DEBUG(0,("ldap suffix missing from smb.conf--exiting\n"));
		exit(1);
	}

	/* Get other smb.conf data */
	if (!(lp_workgroup()) || !*(lp_workgroup())) {
		DEBUG(0,("workgroup missing from smb.conf--exiting\n"));
		exit(1);
	}

	/* Allocate initial memory for groupmap and accountmap arrays */
	if (init_ldap == 1) {
		groupmap = SMB_MALLOC_ARRAY(GROUPMAP, 8);
		accountmap = SMB_MALLOC_ARRAY(ACCOUNTMAP, 8);
		if (groupmap == NULL || accountmap == NULL) {
			DEBUG(1,("GROUPMAP malloc failed\n"));
			ret = NT_STATUS_NO_MEMORY;
			goto done;
		}

		/* Initialize the arrays */
		memset(groupmap, 0, sizeof(GROUPMAP)*8);
		memset(accountmap, 0, sizeof(ACCOUNTMAP)*8);

		/* Remember how many we malloced */
		num_alloced = 8;

		/* Initial database population */
		populate_ldap_for_ldif(sid, suffix, builtin_sid, add_file);
		map_populate_groups(groupmap, accountmap, sid, suffix,
			    builtin_sid);

		/* Don't do this again */
		init_ldap = 0;
	}

	/* Announce what we are doing */
	switch( db_type ) {
		case SAM_DATABASE_DOMAIN:
			d_fprintf(stderr, "Fetching DOMAIN database\n");
			break;
		case SAM_DATABASE_BUILTIN:
			d_fprintf(stderr, "Fetching BUILTIN database\n");
			break;
		case SAM_DATABASE_PRIVS:
			d_fprintf(stderr, "Fetching PRIVS databases\n");
			break;
		default:
			d_fprintf(stderr, 
				  "Fetching unknown database type %u\n", 
				  db_type );
			break;
	}

	do {
		result = rpccli_netlogon_sam_sync(pipe_hnd, mem_ctx,
					       db_type, sync_context,
					       &num_deltas, &hdr_deltas, 
					       &deltas);
		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES)) {
			ret = NT_STATUS_OK;
			goto done; /* is this correct? jmcd */
		}

		/* Re-allocate memory for groupmap and accountmap arrays */
		groupmap = SMB_REALLOC_ARRAY(groupmap, GROUPMAP,
					num_deltas+num_alloced);
		accountmap = SMB_REALLOC_ARRAY(accountmap, ACCOUNTMAP,
					num_deltas+num_alloced);
		if (groupmap == NULL || accountmap == NULL) {
			DEBUG(1,("GROUPMAP malloc failed\n"));
			ret = NT_STATUS_NO_MEMORY;
			goto done;
		}

		/* Initialize the new records */
		memset(&groupmap[num_alloced], 0, 
		       sizeof(GROUPMAP)*num_deltas);
		memset(&accountmap[num_alloced], 0,
		       sizeof(ACCOUNTMAP)*num_deltas);

		/* Remember how many we alloced this time */
		num_alloced += num_deltas;

		/* Loop through the deltas */
		for (k=0; k<num_deltas; k++) {
			switch(hdr_deltas[k].type) {
				case SAM_DELTA_DOMAIN_INFO:
					/* Is this case needed? */
					unistr2_to_ascii(domainname, 
				    	&deltas[k].domain_info.uni_dom_name,
					    	sizeof(domainname)-1);
					break;

				case SAM_DELTA_GROUP_INFO:
					fetch_group_info_to_ldif(
						&deltas[k], &groupmap[g_index],
						add_file, sid, suffix);
					g_index++;
					break;

				case SAM_DELTA_ACCOUNT_INFO:
					fetch_account_info_to_ldif(
						&deltas[k], groupmap, 
						&accountmap[a_index], add_file,
						sid, suffix, num_alloced);
					a_index++;
					break;

				case SAM_DELTA_ALIAS_INFO:
					fetch_alias_info_to_ldif(
						&deltas[k], &groupmap[g_index],
						add_file, sid, suffix, db_type);
					g_index++;
					break;

				case SAM_DELTA_GROUP_MEM:
					fetch_groupmem_info_to_ldif(
						&deltas[k], &hdr_deltas[k], 
						groupmap, accountmap, 
						mod_file, num_alloced);
					break;

				case SAM_DELTA_ALIAS_MEM:
					break;
				case SAM_DELTA_POLICY_INFO:
					break;
				case SAM_DELTA_PRIVS_INFO:
					break;
				case SAM_DELTA_TRUST_DOMS:
					/* Implemented but broken */
					break;
				case SAM_DELTA_SECRET_INFO:
					/* Implemented but broken */
					break;
				case SAM_DELTA_RENAME_GROUP:
					/* Not yet implemented */
					break;
				case SAM_DELTA_RENAME_USER:
					/* Not yet implemented */
					break;
				case SAM_DELTA_RENAME_ALIAS:
					/* Not yet implemented */
					break;
				case SAM_DELTA_DELETE_GROUP:
					/* Not yet implemented */
					break;
				case SAM_DELTA_DELETE_USER:
					/* Not yet implemented */
					break;
				case SAM_DELTA_MODIFIED_COUNT:
					break;
				default:
				break;
			} /* end of switch */
		} /* end of for loop */

		/* Increment sync_context */
		sync_context += 1;

	} while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	/* Write ldif data to the user's file */
	if (db_type == SAM_DATABASE_DOMAIN) {
		fprintf(ldif_file,
			"# SAM_DATABASE_DOMAIN: ADD ENTITIES\n");
		fprintf(ldif_file,
			"# =================================\n\n");
		fflush(ldif_file);
	} else if (db_type == SAM_DATABASE_BUILTIN) {
		fprintf(ldif_file,
			"# SAM_DATABASE_BUILTIN: ADD ENTITIES\n");
		fprintf(ldif_file,
			"# ==================================\n\n");
		fflush(ldif_file);
	}
	fseek(add_file, 0, SEEK_SET);
	transfer_file(fileno(add_file), fileno(ldif_file), (size_t) -1);

	if (db_type == SAM_DATABASE_DOMAIN) {
		fprintf(ldif_file,
			"# SAM_DATABASE_DOMAIN: MODIFY ENTITIES\n");
		fprintf(ldif_file,
			"# ====================================\n\n");
		fflush(ldif_file);
	} else if (db_type == SAM_DATABASE_BUILTIN) {
		fprintf(ldif_file,
			"# SAM_DATABASE_BUILTIN: MODIFY ENTITIES\n");
		fprintf(ldif_file,
			"# =====================================\n\n");
		fflush(ldif_file);
	}
	fseek(mod_file, 0, SEEK_SET);
	transfer_file(fileno(mod_file), fileno(ldif_file), (size_t) -1);


  done:
	/* Close and delete the ldif files */
	if (add_file) {
		fclose(add_file);
	}

	if ((add_name != NULL) && strcmp(add_name, add_template) && (unlink(add_name))) {
		DEBUG(1,("unlink(%s) failed, error was (%s)\n",
			 add_name, strerror(errno)));
	}

	if (mod_file) {
		fclose(mod_file);
	}

	if ((mod_name != NULL) && strcmp(mod_name, mod_template) && (unlink(mod_name))) {
		DEBUG(1,("unlink(%s) failed, error was (%s)\n",
			 mod_name, strerror(errno)));
	}
	
	if (ldif_file && (ldif_file != stdout)) {
		fclose(ldif_file);
	}

	/* Deallocate memory for the mapping arrays */
	SAFE_FREE(groupmap);
	SAFE_FREE(accountmap);

	/* Return */
	talloc_destroy(mem_ctx);
	return ret;
}

/** 
 * Basic usage function for 'net rpc vampire'
 * @param argc  Standard main() style argc
 * @param argc  Standard main() style argv.  Initial components are already
 *              stripped
 **/

int rpc_vampire_usage(int argc, const char **argv) 
{	
	d_printf("net rpc vampire [ldif [<ldif-filename>] [options]\n"\
		 "\t to pull accounts from a remote PDC where we are a BDC\n"\
		 "\t\t no args puts accounts in local passdb from smb.conf\n"\
		 "\t\t ldif - put accounts in ldif format (file defaults to /tmp/tmp.ldif\n");

	net_common_flags_usage(argc, argv);
	return -1;
}


/* dump sam database via samsync rpc calls */
NTSTATUS rpc_vampire_internals(const DOM_SID *domain_sid, 
				const char *domain_name, 
				struct cli_state *cli,
				struct rpc_pipe_client *pipe_hnd,
				TALLOC_CTX *mem_ctx, 
				int argc,
				const char **argv) 
{
        NTSTATUS result;
	fstring my_dom_sid_str;
	fstring rem_dom_sid_str;

	if (!sid_equal(domain_sid, get_global_sam_sid())) {
		d_printf("Cannot import users from %s at this time, "
			 "as the current domain:\n\t%s: %s\nconflicts "
			 "with the remote domain\n\t%s: %s\n"
			 "Perhaps you need to set: \n\n\tsecurity=user\n\tworkgroup=%s\n\n in your smb.conf?\n",
			 domain_name,
			 get_global_sam_name(), sid_to_string(my_dom_sid_str, 
							      get_global_sam_sid()),
			 domain_name, sid_to_string(rem_dom_sid_str, domain_sid),
			 domain_name);
		return NT_STATUS_UNSUCCESSFUL;
	}

        if (argc >= 1 && (strcmp(argv[0], "ldif") == 0)) {
		result = fetch_database_to_ldif(pipe_hnd, SAM_DATABASE_DOMAIN,
					*domain_sid, argv[1]);
        } else {
		result = fetch_database(pipe_hnd, SAM_DATABASE_DOMAIN, *domain_sid);
        }

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Failed to fetch domain database: %s\n",
			 nt_errstr(result));
		if (NT_STATUS_EQUAL(result, NT_STATUS_NOT_SUPPORTED))
			d_fprintf(stderr, "Perhaps %s is a Windows 2000 native "
				 "mode domain?\n", domain_name);
		goto fail;
	}

        if (argc >= 1 && (strcmp(argv[0], "ldif") == 0)) {
		result = fetch_database_to_ldif(pipe_hnd, SAM_DATABASE_BUILTIN, 
					global_sid_Builtin, argv[1]);
        } else {
		result = fetch_database(pipe_hnd, SAM_DATABASE_BUILTIN, global_sid_Builtin);
        }

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "Failed to fetch builtin database: %s\n",
			 nt_errstr(result));
		goto fail;
	}

	/* Currently we crash on PRIVS somewhere in unmarshalling */
	/* Dump_database(cli, SAM_DATABASE_PRIVS, &ret_creds); */

fail:
	return result;
}
