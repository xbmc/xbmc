/* 
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Andrew Tridgell              1992-2000,
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
   Copyright (C) Elrond                            2000,
   Copyright (C) Tim Potter                        2000

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
#include "rpcclient.h"

extern DOM_SID domain_sid;

/****************************************************************************
 display sam_user_info_7 structure
 ****************************************************************************/
static void display_sam_user_info_7(SAM_USER_INFO_7 *usr)
{
	fstring temp;

	unistr2_to_ascii(temp, &usr->uni_name, sizeof(temp)-1);
	printf("\tUser Name   :\t%s\n", temp);
}

/****************************************************************************
 display sam_user_info_9 structure
 ****************************************************************************/
static void display_sam_user_info_9(SAM_USER_INFO_9 *usr)
{
	printf("\tPrimary group RID   :\tox%x\n", usr->rid_group);
}

/****************************************************************************
 display sam_user_info_21 structure
 ****************************************************************************/
static void display_sam_user_info_21(SAM_USER_INFO_21 *usr)
{
	fstring temp;

	unistr2_to_ascii(temp, &usr->uni_user_name, sizeof(temp)-1);
	printf("\tUser Name   :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_full_name, sizeof(temp)-1);
	printf("\tFull Name   :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_home_dir, sizeof(temp)-1);
	printf("\tHome Drive  :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_dir_drive, sizeof(temp)-1);
	printf("\tDir Drive   :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_profile_path, sizeof(temp)-1);
	printf("\tProfile Path:\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_logon_script, sizeof(temp)-1);
	printf("\tLogon Script:\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_acct_desc, sizeof(temp)-1);
	printf("\tDescription :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_workstations, sizeof(temp)-1);
	printf("\tWorkstations:\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_unknown_str, sizeof(temp)-1);
	printf("\tUnknown Str :\t%s\n", temp);
	
	unistr2_to_ascii(temp, &usr->uni_munged_dial, sizeof(temp)-1);
	printf("\tRemote Dial :\t%s\n", temp);
	
	printf("\tLogon Time               :\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->logon_time)));
	printf("\tLogoff Time              :\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->logoff_time)));
	printf("\tKickoff Time             :\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->kickoff_time)));
	printf("\tPassword last set Time   :\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->pass_last_set_time)));
	printf("\tPassword can change Time :\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->pass_can_change_time)));
	printf("\tPassword must change Time:\t%s\n", 
	       http_timestring(nt_time_to_unix(&usr->pass_must_change_time)));
	
	printf("\tunknown_2[0..31]...\n"); /* user passwords? */
	
	printf("\tuser_rid :\t0x%x\n"  , usr->user_rid ); /* User ID */
	printf("\tgroup_rid:\t0x%x\n"  , usr->group_rid); /* Group ID */
	printf("\tacb_info :\t0x%08x\n", usr->acb_info ); /* Account Control Info */
	
	printf("\tfields_present:\t0x%08x\n", usr->fields_present); /* 0x00ff ffff */
	printf("\tlogon_divs:\t%d\n", usr->logon_divs); /* 0x0000 00a8 which is 168 which is num hrs in a week */
	printf("\tbad_password_count:\t0x%08x\n", usr->bad_password_count);
	printf("\tlogon_count:\t0x%08x\n", usr->logon_count);
	
	printf("\tpadding1[0..7]...\n");
	
	if (usr->ptr_logon_hrs) {
		printf("\tlogon_hrs[0..%d]...\n", usr->logon_hrs.len);
	}
}

static const char *display_time(NTTIME nttime)
{
	static fstring string;

	float high;
	float low;
	int sec;
	int days, hours, mins, secs;

	if (nttime.high==0 && nttime.low==0)
		return "Now";

	if (nttime.high==0x80000000 && nttime.low==0)
		return "Never";

	high = 65536;	
	high = high/10000;
	high = high*65536;
	high = high/1000;
	high = high * (~nttime.high);

	low = ~nttime.low;	
	low = low/(1000*1000*10);

	sec=high+low;

	days=sec/(60*60*24);
	hours=(sec - (days*60*60*24)) / (60*60);
	mins=(sec - (days*60*60*24) - (hours*60*60) ) / 60;
	secs=sec - (days*60*60*24) - (hours*60*60) - (mins*60);

	fstr_sprintf(string, "%u days, %u hours, %u minutes, %u seconds", days, hours, mins, secs);
	return (string);
}

static void display_password_properties(uint32 password_properties) 
{
	printf("password_properties: 0x%08x\n", password_properties);
		
	if (password_properties & DOMAIN_PASSWORD_COMPLEX)
		printf("\tDOMAIN_PASSWORD_COMPLEX\n");
			
	if (password_properties & DOMAIN_PASSWORD_NO_ANON_CHANGE)
		printf("\tDOMAIN_PASSWORD_NO_ANON_CHANGE\n");
			
	if (password_properties & DOMAIN_PASSWORD_NO_CLEAR_CHANGE)
		printf("\tDOMAIN_PASSWORD_NO_CLEAR_CHANGE\n");
			
	if (password_properties & DOMAIN_LOCKOUT_ADMINS)
		printf("\tDOMAIN_LOCKOUT_ADMINS\n");
			
	if (password_properties & DOMAIN_PASSWORD_STORE_CLEARTEXT)
		printf("\tDOMAIN_PASSWORD_STORE_CLEARTEXT\n");
			
	if (password_properties & DOMAIN_REFUSE_PASSWORD_CHANGE)
		printf("\tDOMAIN_REFUSE_PASSWORD_CHANGE\n");
}

static void display_sam_unk_info_1(SAM_UNK_INFO_1 *info1)
{
	
	printf("Minimum password length:\t\t\t%d\n", info1->min_length_password);
	printf("Password uniqueness (remember x passwords):\t%d\n", info1->password_history);
	display_password_properties(info1->password_properties);
	printf("password expire in:\t\t\t\t%s\n", display_time(info1->expire));
	printf("Min password age (allow changing in x days):\t%s\n", display_time(info1->min_passwordage));
}

static void display_sam_unk_info_2(SAM_UNK_INFO_2 *info2)
{
	fstring name;

	unistr2_to_ascii(name, &info2->uni_domain, sizeof(name) - 1); 
	printf("Domain:\t\t%s\n", name);

	unistr2_to_ascii(name, &info2->uni_server, sizeof(name) - 1); 
	printf("Server:\t\t%s\n", name);

	unistr2_to_ascii(name, &info2->uni_comment, sizeof(name) - 1); 
	printf("Comment:\t%s\n", name);

	printf("Total Users:\t%d\n", info2->num_domain_usrs);
	printf("Total Groups:\t%d\n", info2->num_domain_grps);
	printf("Total Aliases:\t%d\n", info2->num_local_grps);
	
	printf("Sequence No:\t%d\n", info2->seq_num.low);

	printf("Force Logoff:\t%d\n", (int)nt_time_to_unix_abs(&info2->logout));

	printf("Unknown 4:\t0x%x\n", info2->unknown_4);
	printf("Server Role:\t%s\n", server_role_str(info2->server_role));
	printf("Unknown 6:\t0x%x\n", info2->unknown_6);
}

static void display_sam_unk_info_3(SAM_UNK_INFO_3 *info3)
{
	printf("Force Logoff:\t%d\n", (int)nt_time_to_unix_abs(&info3->logout));
}

static void display_sam_unk_info_4(SAM_UNK_INFO_4 *info4)
{
	fstring name;

	unistr2_to_ascii(name, &info4->uni_comment, sizeof(name) - 1); 
	printf("Comment:\t%s\n", name);
}

static void display_sam_unk_info_5(SAM_UNK_INFO_5 *info5)
{
	fstring name;

	unistr2_to_ascii(name, &info5->uni_domain, sizeof(name) - 1); 
	printf("Domain:\t\t%s\n", name);
}

static void display_sam_unk_info_6(SAM_UNK_INFO_6 *info6)
{
	fstring name;

	unistr2_to_ascii(name, &info6->uni_server, sizeof(name) - 1); 
	printf("Server:\t\t%s\n", name);
}

static void display_sam_unk_info_7(SAM_UNK_INFO_7 *info7)
{
	printf("Server Role:\t%s\n", server_role_str(info7->server_role));
}

static void display_sam_unk_info_8(SAM_UNK_INFO_8 *info8)
{
	printf("Sequence No:\t%d\n", info8->seq_num.low);
	printf("Domain Create Time:\t%s\n", 
		http_timestring(nt_time_to_unix(&info8->domain_create_time)));
}

static void display_sam_unk_info_9(SAM_UNK_INFO_9 *info9)
{
	printf("unknown:\t%d (0x%08x)\n", info9->unknown, info9->unknown);
}

static void display_sam_unk_info_12(SAM_UNK_INFO_12 *info12)
{
	printf("Bad password lockout duration:               %s\n", display_time(info12->duration));
	printf("Reset Lockout after:                         %s\n", display_time(info12->reset_count));
	printf("Lockout after bad attempts:                  %d\n", info12->bad_attempt_lockout);
}

static void display_sam_unk_info_13(SAM_UNK_INFO_13 *info13)
{
	printf("Sequence No:\t%d\n", info13->seq_num.low);
	printf("Domain Create Time:\t%s\n", 
		http_timestring(nt_time_to_unix(&info13->domain_create_time)));
	printf("Unknown1:\t%d\n", info13->unknown1);
	printf("Unknown2:\t%d\n", info13->unknown2);

}

static void display_sam_info_1(SAM_ENTRY1 *e1, SAM_STR1 *s1)
{
	fstring tmp;

	printf("index: 0x%x ", e1->user_idx);
	printf("RID: 0x%x ", e1->rid_user);
	printf("acb: 0x%x ", e1->acb_info);

	unistr2_to_ascii(tmp, &s1->uni_acct_name, sizeof(tmp)-1);
	printf("Account: %s\t", tmp);

	unistr2_to_ascii(tmp, &s1->uni_full_name, sizeof(tmp)-1);
	printf("Name: %s\t", tmp);

	unistr2_to_ascii(tmp, &s1->uni_acct_desc, sizeof(tmp)-1);
	printf("Desc: %s\n", tmp);
}

static void display_sam_info_2(SAM_ENTRY2 *e2, SAM_STR2 *s2)
{
	fstring tmp;

	printf("index: 0x%x ", e2->user_idx);
	printf("RID: 0x%x ", e2->rid_user);
	printf("acb: 0x%x ", e2->acb_info);
	
	unistr2_to_ascii(tmp, &s2->uni_srv_name, sizeof(tmp)-1);
	printf("Account: %s\t", tmp);

	unistr2_to_ascii(tmp, &s2->uni_srv_desc, sizeof(tmp)-1);
	printf("Name: %s\n", tmp);

}

static void display_sam_info_3(SAM_ENTRY3 *e3, SAM_STR3 *s3)
{
	fstring tmp;

	printf("index: 0x%x ", e3->grp_idx);
	printf("RID: 0x%x ", e3->rid_grp);
	printf("attr: 0x%x ", e3->attr);
	
	unistr2_to_ascii(tmp, &s3->uni_grp_name, sizeof(tmp)-1);
	printf("Account: %s\t", tmp);

	unistr2_to_ascii(tmp, &s3->uni_grp_desc, sizeof(tmp)-1);
	printf("Name: %s\n", tmp);

}

static void display_sam_info_4(SAM_ENTRY4 *e4, SAM_STR4 *s4)
{
	int i;

	printf("index: %d ", e4->user_idx);
	
	printf("Account: ");
	for (i=0; i<s4->acct_name.str_str_len; i++)
		printf("%c", s4->acct_name.buffer[i]);
	printf("\n");

}

static void display_sam_info_5(SAM_ENTRY5 *e5, SAM_STR5 *s5)
{
	int i;

	printf("index: 0x%x ", e5->grp_idx);
	
	printf("Account: ");
	for (i=0; i<s5->grp_name.str_str_len; i++)
		printf("%c", s5->grp_name.buffer[i]);
	printf("\n");

}

/****************************************************************************
 Try samr_connect4 first, then samr_conenct if it fails
 ****************************************************************************/
static NTSTATUS try_samr_connects(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, 
				  uint32 access_mask, POLICY_HND *connect_pol)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	
	result = rpccli_samr_connect4(cli, mem_ctx, access_mask, connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		result = rpccli_samr_connect(cli, mem_ctx, access_mask,
					  connect_pol);
	}
	return result;
}

/**********************************************************************
 * Query user information 
 */
static NTSTATUS cmd_samr_query_user(struct rpc_pipe_client *cli, 
                                    TALLOC_CTX *mem_ctx,
                                    int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 info_level = 21;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	SAM_USERINFO_CTR *user_ctr;
	fstring server;
	uint32 user_rid;
	
	if ((argc < 2) || (argc > 4)) {
		printf("Usage: %s rid [info level] [access mask] \n", argv[0]);
		return NT_STATUS_OK;
	}
	
	user_rid = strtoul(argv[1], NULL, 10);
	
	if (argc > 2)
		sscanf(argv[2], "%i", &info_level);
		
	if (argc > 3)
		sscanf(argv[3], "%x", &access_mask);
	

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);
	
	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_user(cli, mem_ctx, &domain_pol,
				    access_mask,
				    user_rid, &user_pol);

	if (NT_STATUS_EQUAL(result, NT_STATUS_NO_SUCH_USER) &&
	    (user_rid == 0)) {

		/* Probably this was a user name, try lookupnames */
		uint32 num_rids;
		uint32 *rids, *types;
		
		result = rpccli_samr_lookup_names(cli, mem_ctx, &domain_pol,
						  1000, 1, &argv[1],
						  &num_rids, &rids,
						  &types);

		if (NT_STATUS_IS_OK(result)) {
			result = rpccli_samr_open_user(cli, mem_ctx,
						       &domain_pol,
						       access_mask,
						       rids[0], &user_pol);
		}
	}


	if (!NT_STATUS_IS_OK(result))
		goto done;

	ZERO_STRUCT(user_ctr);

	result = rpccli_samr_query_userinfo(cli, mem_ctx, &user_pol, 
					 info_level, &user_ctr);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	switch (user_ctr->switch_value) {
	case 21:
		display_sam_user_info_21(user_ctr->info.id21);
		break;
	case 7:
		display_sam_user_info_7(user_ctr->info.id7);
		break;
	case 9:
		display_sam_user_info_9(user_ctr->info.id9);
		break;
	default:
		printf("Unsupported infolevel: %d\n", info_level);
		break;
	}

	rpccli_samr_close(cli, mem_ctx, &user_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);

done:
	return result;
}

/****************************************************************************
 display group info
 ****************************************************************************/
static void display_group_info1(GROUP_INFO1 *info1)
{
	fstring temp;

	unistr2_to_ascii(temp, &info1->uni_acct_name, sizeof(temp)-1);
	printf("\tGroup Name:\t%s\n", temp);
	unistr2_to_ascii(temp, &info1->uni_acct_desc, sizeof(temp)-1);
	printf("\tDescription:\t%s\n", temp);
	printf("\tGroup Attribute:%d\n", info1->group_attr);
	printf("\tNum Members:%d\n", info1->num_members);
}

/****************************************************************************
 display group info
 ****************************************************************************/
static void display_group_info2(GROUP_INFO2 *info2)
{
	fstring name;

	unistr2_to_ascii(name, &info2->uni_acct_name, sizeof(name)-1);
	printf("\tGroup Description:%s\n", name);
}


/****************************************************************************
 display group info
 ****************************************************************************/
static void display_group_info3(GROUP_INFO3 *info3)
{
	printf("\tGroup Attribute:%d\n", info3->group_attr);
}


/****************************************************************************
 display group info
 ****************************************************************************/
static void display_group_info4(GROUP_INFO4 *info4)
{
	fstring desc;

	unistr2_to_ascii(desc, &info4->uni_acct_desc, sizeof(desc)-1);
	printf("\tGroup Description:%s\n", desc);
}

/****************************************************************************
 display group info
 ****************************************************************************/
static void display_group_info5(GROUP_INFO5 *info5)
{
	fstring temp;

	unistr2_to_ascii(temp, &info5->uni_acct_name, sizeof(temp)-1);
	printf("\tGroup Name:\t%s\n", temp);
	unistr2_to_ascii(temp, &info5->uni_acct_desc, sizeof(temp)-1);
	printf("\tDescription:\t%s\n", temp);
	printf("\tGroup Attribute:%d\n", info5->group_attr);
	printf("\tNum Members:%d\n", info5->num_members);
}

/****************************************************************************
 display sam sync structure
 ****************************************************************************/
static void display_group_info_ctr(GROUP_INFO_CTR *ctr)
{
	switch (ctr->switch_value1) {
		case 1:
			display_group_info1(&ctr->group.info1);
			break;
		case 2:
			display_group_info2(&ctr->group.info2);
			break;
		case 3:
			display_group_info3(&ctr->group.info3);
			break;
		case 4:
			display_group_info4(&ctr->group.info4);
			break;
		case 5:
			display_group_info5(&ctr->group.info5);
			break;

	}
}

/***********************************************************************
 * Query group information 
 */
static NTSTATUS cmd_samr_query_group(struct rpc_pipe_client *cli, 
                                     TALLOC_CTX *mem_ctx,
                                     int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, group_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 info_level = 1;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	GROUP_INFO_CTR *group_ctr;
	fstring			server;	
	uint32 group_rid;
	
	if ((argc < 2) || (argc > 4)) {
		printf("Usage: %s rid [info level] [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

        sscanf(argv[1], "%i", &group_rid);
	
	if (argc > 2)
		sscanf(argv[2], "%i", &info_level);
	
	if (argc > 3)
		sscanf(argv[3], "%x", &access_mask);

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_group(cli, mem_ctx, &domain_pol,
				     access_mask,
				     group_rid, &group_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_query_groupinfo(cli, mem_ctx, &group_pol, 
					  info_level, &group_ctr);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	display_group_info_ctr(group_ctr);

	rpccli_samr_close(cli, mem_ctx, &group_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
done:
	return result;
}

/* Query groups a user is a member of */

static NTSTATUS cmd_samr_query_usergroups(struct rpc_pipe_client *cli, 
                                          TALLOC_CTX *mem_ctx,
                                          int argc, const char **argv) 
{
	POLICY_HND 		connect_pol, 
				domain_pol, 
				user_pol;
	NTSTATUS		result = NT_STATUS_UNSUCCESSFUL;
	uint32 			num_groups, 
				user_rid;
	uint32			access_mask = MAXIMUM_ALLOWED_ACCESS;
	DOM_GID 		*user_gids;
	int 			i;
	fstring			server;
	
	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s rid [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	sscanf(argv[1], "%i", &user_rid);
	
	if (argc > 2)
		sscanf(argv[2], "%x", &access_mask);

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);
		
	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_user(cli, mem_ctx, &domain_pol,
				    access_mask,
				    user_rid, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_query_usergroups(cli, mem_ctx, &user_pol,
					   &num_groups, &user_gids);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < num_groups; i++) {
		printf("\tgroup rid:[0x%x] attr:[0x%x]\n", 
		       user_gids[i].g_rid, user_gids[i].attr);
	}

	rpccli_samr_close(cli, mem_ctx, &user_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Query aliases a user is a member of */

static NTSTATUS cmd_samr_query_useraliases(struct rpc_pipe_client *cli, 
					   TALLOC_CTX *mem_ctx,
					   int argc, const char **argv) 
{
	POLICY_HND 		connect_pol, domain_pol;
	NTSTATUS		result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID                *sids;
	size_t                     num_sids;
	uint32 			num_aliases, *alias_rids;
	uint32			access_mask = MAXIMUM_ALLOWED_ACCESS;
	int 			i;
	fstring			server;
	DOM_SID2	       *sid2;

	if (argc < 3) {
		printf("Usage: %s builtin|domain sid1 sid2 ...\n", argv[0]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	sids = NULL;
	num_sids = 0;

	for (i=2; i<argc; i++) {
		DOM_SID tmp_sid;
		if (!string_to_sid(&tmp_sid, argv[i])) {
			printf("%s is not a legal SID\n", argv[i]);
			return NT_STATUS_INVALID_PARAMETER;
		}
		add_sid_to_array(mem_ctx, &tmp_sid, &sids, &num_sids);
	}

	sid2 = TALLOC_ARRAY(mem_ctx, DOM_SID2, num_sids);
	if (sid2 == NULL)
		return NT_STATUS_NO_MEMORY;

	for (i=0; i<num_sids; i++) {
		sid_copy(&sid2[i].sid, &sids[i]);
		sid2[i].num_auths = sid2[i].sid.num_auths;
	}

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);
		
	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      access_mask,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      access_mask,
					      &global_sid_Builtin,
					      &domain_pol);
	else {
		printf("Usage: %s builtin|domain sid1 sid2 ...\n", argv[0]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_query_useraliases(cli, mem_ctx, &domain_pol,
					    num_sids, sid2,
					    &num_aliases, &alias_rids);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < num_aliases; i++) {
		printf("\tgroup rid:[0x%x]\n", alias_rids[i]);
	}

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Query members of a group */

static NTSTATUS cmd_samr_query_groupmem(struct rpc_pipe_client *cli, 
                                        TALLOC_CTX *mem_ctx,
                                        int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, group_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 num_members, *group_rids, *group_attrs, group_rid;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	int i;
	fstring			server;
	unsigned int old_timeout;
	
	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s rid [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	sscanf(argv[1], "%i", &group_rid);
	
	if (argc > 2)
		sscanf(argv[2], "%x", &access_mask);

	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_group(cli, mem_ctx, &domain_pol,
				     access_mask,
				     group_rid, &group_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Make sure to wait for our DC's reply */
	old_timeout = cli_set_timeout(cli->cli, 30000); /* 30 seconds. */

	result = rpccli_samr_query_groupmem(cli, mem_ctx, &group_pol,
					 &num_members, &group_rids,
					 &group_attrs);

	cli_set_timeout(cli->cli, old_timeout);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < num_members; i++) {
		printf("\trid:[0x%x] attr:[0x%x]\n", group_rids[i],
		       group_attrs[i]);
	}

	rpccli_samr_close(cli, mem_ctx, &group_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Enumerate domain users */

static NTSTATUS cmd_samr_enum_dom_users(struct rpc_pipe_client *cli, 
					TALLOC_CTX *mem_ctx,
					int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx, size, num_dom_users, i;
	char **dom_users;
	uint32 *dom_rids;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	uint32 acb_mask = ACB_NORMAL;
	BOOL got_connect_pol = False, got_domain_pol = False;

	if ((argc < 1) || (argc > 3)) {
		printf("Usage: %s [access_mask] [acb_mask]\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	if (argc > 1)
		sscanf(argv[1], "%x", &access_mask);

	if (argc > 2)
		sscanf(argv[2], "%x", &acb_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_connect_pol = True;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_domain_pol = True;

	/* Enumerate domain users */

	start_idx = 0;
	size = 0xffff;

	do {
		result = rpccli_samr_enum_dom_users(
			cli, mem_ctx, &domain_pol, &start_idx, acb_mask,
			size, &dom_users, &dom_rids, &num_dom_users);

		if (NT_STATUS_IS_OK(result) ||
		    NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES)) {

			for (i = 0; i < num_dom_users; i++)
                               printf("user:[%s] rid:[0x%x]\n", 
				       dom_users[i], dom_rids[i]);
		}

	} while (NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES));

 done:
	if (got_domain_pol)
		rpccli_samr_close(cli, mem_ctx, &domain_pol);

	if (got_connect_pol)
		rpccli_samr_close(cli, mem_ctx, &connect_pol);

	return result;
}

/* Enumerate domain groups */

static NTSTATUS cmd_samr_enum_dom_groups(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx, size, num_dom_groups, i;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	struct acct_info *dom_groups;
	BOOL got_connect_pol = False, got_domain_pol = False;

	if ((argc < 1) || (argc > 2)) {
		printf("Usage: %s [access_mask]\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	if (argc > 1)
		sscanf(argv[1], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_connect_pol = True;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_domain_pol = True;

	/* Enumerate domain groups */

	start_idx = 0;
	size = 0xffff;

	do {
		result = rpccli_samr_enum_dom_groups(
			cli, mem_ctx, &domain_pol, &start_idx, size,
			&dom_groups, &num_dom_groups);

		if (NT_STATUS_IS_OK(result) ||
		    NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES)) {

			for (i = 0; i < num_dom_groups; i++)
				printf("group:[%s] rid:[0x%x]\n", 
				       dom_groups[i].acct_name,
				       dom_groups[i].rid);
		}

	} while (NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES));

 done:
	if (got_domain_pol)
		rpccli_samr_close(cli, mem_ctx, &domain_pol);

	if (got_connect_pol)
		rpccli_samr_close(cli, mem_ctx, &connect_pol);

	return result;
}

/* Enumerate alias groups */

static NTSTATUS cmd_samr_enum_als_groups(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx, size, num_als_groups, i;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	struct acct_info *als_groups;
	BOOL got_connect_pol = False, got_domain_pol = False;

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s builtin|domain [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	if (argc > 2)
		sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_connect_pol = True;

	/* Get domain policy handle */

	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      access_mask,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      access_mask,
					      &global_sid_Builtin, &domain_pol);
	else
		return NT_STATUS_OK;

	if (!NT_STATUS_IS_OK(result))
		goto done;

	got_domain_pol = True;

	/* Enumerate alias groups */

	start_idx = 0;
	size = 0xffff;		/* Number of groups to retrieve */

	do {
		result = rpccli_samr_enum_als_groups(
			cli, mem_ctx, &domain_pol, &start_idx, size,
			&als_groups, &num_als_groups);

		if (NT_STATUS_IS_OK(result) ||
		    NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES)) {

			for (i = 0; i < num_als_groups; i++)
				printf("group:[%s] rid:[0x%x]\n", 
				       als_groups[i].acct_name,
				       als_groups[i].rid);
		}
	} while (NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES));

 done:
	if (got_domain_pol)
		rpccli_samr_close(cli, mem_ctx, &domain_pol);
	
	if (got_connect_pol)
		rpccli_samr_close(cli, mem_ctx, &connect_pol);
	
	return result;
}

/* Query alias membership */

static NTSTATUS cmd_samr_query_aliasmem(struct rpc_pipe_client *cli, 
                                        TALLOC_CTX *mem_ctx,
                                        int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, alias_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 alias_rid, num_members, i;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	DOM_SID *alias_sids;

	if ((argc < 3) || (argc > 4)) {
		printf("Usage: %s builtin|domain rid [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	sscanf(argv[2], "%i", &alias_rid);
	
	if (argc > 3)
		sscanf(argv[3], "%x", &access_mask);

	/* Open SAMR handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Open handle on domain */
	
	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &global_sid_Builtin, &domain_pol);
	else
		return NT_STATUS_OK;

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Open handle on alias */

	result = rpccli_samr_open_alias(cli, mem_ctx, &domain_pol,
				     access_mask,
				     alias_rid, &alias_pol);
	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_query_aliasmem(cli, mem_ctx, &alias_pol,
					 &num_members, &alias_sids);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	for (i = 0; i < num_members; i++) {
		fstring sid_str;

		sid_to_string(sid_str, &alias_sids[i]);
		printf("\tsid:[%s]\n", sid_str);
	}

	rpccli_samr_close(cli, mem_ctx, &alias_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Query delete an alias membership */

static NTSTATUS cmd_samr_delete_alias(struct rpc_pipe_client *cli, 
				      TALLOC_CTX *mem_ctx,
				      int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, alias_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 alias_rid;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;

	if (argc != 3) {
		printf("Usage: %s builtin|domain [rid|name]\n", argv[0]);
		return NT_STATUS_OK;
	}

	alias_rid = strtoul(argv[2], NULL, 10);
	
	/* Open SAMR handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Open handle on domain */
	
	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &global_sid_Builtin, &domain_pol);
	else
		return NT_STATUS_INVALID_PARAMETER;

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Open handle on alias */

	result = rpccli_samr_open_alias(cli, mem_ctx, &domain_pol,
				     access_mask,
				     alias_rid, &alias_pol);
	if (!NT_STATUS_IS_OK(result) && (alias_rid == 0)) {
		/* Probably this was a user name, try lookupnames */
		uint32 num_rids;
		uint32 *rids, *types;
		
		result = rpccli_samr_lookup_names(cli, mem_ctx, &domain_pol,
						  1000, 1, &argv[2],
						  &num_rids, &rids,
						  &types);

		if (NT_STATUS_IS_OK(result)) {
			result = rpccli_samr_open_alias(cli, mem_ctx,
						       &domain_pol,
						       access_mask,
						       rids[0], &alias_pol);
		}
	}

	result = rpccli_samr_delete_dom_alias(cli, mem_ctx, &alias_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Query display info */

static NTSTATUS cmd_samr_query_dispinfo(struct rpc_pipe_client *cli, 
                                        TALLOC_CTX *mem_ctx,
                                        int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 start_idx=0, max_entries=250, max_size = 0xffff, num_entries, i;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	uint32 info_level = 1;
	SAM_DISPINFO_CTR ctr;
	SAM_DISPINFO_1 info1;
	SAM_DISPINFO_2 info2;
	SAM_DISPINFO_3 info3;
	SAM_DISPINFO_4 info4;
	SAM_DISPINFO_5 info5;
	int loop_count = 0;
	BOOL got_params = False; /* Use get_query_dispinfo_params() or not? */

	if (argc > 6) {
		printf("Usage: %s [info level] [start index] [max entries] [max size] [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc >= 2)
                sscanf(argv[1], "%i", &info_level);
        
	if (argc >= 3)
                sscanf(argv[2], "%i", &start_idx);
        
	if (argc >= 4) {
                sscanf(argv[3], "%i", &max_entries);
		got_params = True;
	}
	
	if (argc >= 5) {
                sscanf(argv[4], "%i", &max_size);
		got_params = True;
	}
	
	if (argc >= 6)
                sscanf(argv[5], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask, 
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Query display info */

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(info1);
	
	switch (info_level) {
	case 1:
		ZERO_STRUCT(info1);
		ctr.sam.info1 = &info1;
		break;
	case 2:
		ZERO_STRUCT(info2);
		ctr.sam.info2 = &info2;
		break;
	case 3:
		ZERO_STRUCT(info3);
		ctr.sam.info3 = &info3;
		break;
	case 4:
		ZERO_STRUCT(info4);
		ctr.sam.info4 = &info4;
		break;
	case 5:
		ZERO_STRUCT(info5);
		ctr.sam.info5 = &info5;
		break;
	}


	do {

		if (!got_params)
			get_query_dispinfo_params(
				loop_count, &max_entries, &max_size);
		
		result = rpccli_samr_query_dispinfo(cli, mem_ctx, &domain_pol,
						 &start_idx, info_level,
						 &num_entries, max_entries, 
						 max_size, &ctr);

		loop_count++;

		if (NT_STATUS_IS_ERR(result))
			break;

		if (num_entries == 0) 
			break;

		for (i = 0; i < num_entries; i++) {
			switch (info_level) {
			case 1:
				display_sam_info_1(&ctr.sam.info1->sam[i], &ctr.sam.info1->str[i]);
				break;
			case 2:
				display_sam_info_2(&ctr.sam.info2->sam[i], &ctr.sam.info2->str[i]);
				break;
			case 3:
				display_sam_info_3(&ctr.sam.info3->sam[i], &ctr.sam.info3->str[i]);
				break;
			case 4:
				display_sam_info_4(&ctr.sam.info4->sam[i], &ctr.sam.info4->str[i]);
				break;
			case 5:
				display_sam_info_5(&ctr.sam.info5->sam[i], &ctr.sam.info5->str[i]);
				break;
			}
		}
	} while ( NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES));

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Query domain info */

static NTSTATUS cmd_samr_query_dominfo(struct rpc_pipe_client *cli, 
                                       TALLOC_CTX *mem_ctx,
                                       int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 switch_level = 2;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	SAM_UNK_CTR ctr;

	if (argc > 3) {
		printf("Usage: %s [info level] [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc > 1)
                sscanf(argv[1], "%i", &switch_level);
	
	if (argc > 2)
                sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Query domain info */

	result = rpccli_samr_query_dom_info(cli, mem_ctx, &domain_pol,
					 switch_level, &ctr);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Display domain info */

	switch (switch_level) {
	case 1:
		display_sam_unk_info_1(&ctr.info.inf1);
		break;
	case 2:
		display_sam_unk_info_2(&ctr.info.inf2);
		break;
	case 3:
		display_sam_unk_info_3(&ctr.info.inf3);
		break;
	case 4:
		display_sam_unk_info_4(&ctr.info.inf4);
		break;
	case 5:
		display_sam_unk_info_5(&ctr.info.inf5);
		break;
	case 6:
		display_sam_unk_info_6(&ctr.info.inf6);
		break;
	case 7:
		display_sam_unk_info_7(&ctr.info.inf7);
		break;
	case 8:
		display_sam_unk_info_8(&ctr.info.inf8);
		break;
	case 9:
		display_sam_unk_info_9(&ctr.info.inf9);
		break;
	case 12:
		display_sam_unk_info_12(&ctr.info.inf12);
		break;
	case 13:
		display_sam_unk_info_13(&ctr.info.inf13);
		break;

	default:
		printf("cannot display domain info for switch value %d\n",
		       switch_level);
		break;
	}

 done:
 
 	rpccli_samr_close(cli, mem_ctx, &domain_pol);
 	rpccli_samr_close(cli, mem_ctx, &connect_pol);
	return result;
}

/* Create domain user */

static NTSTATUS cmd_samr_create_dom_user(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	const char *acct_name;
	uint32 acb_info;
	uint32 unknown, user_rid;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s username [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	acct_name = argv[1];
	
	if (argc > 2)
                sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Create domain user */

	acb_info = ACB_NORMAL;
	unknown = 0xe005000b; /* No idea what this is - a permission mask? */

	result = rpccli_samr_create_dom_user(cli, mem_ctx, &domain_pol,
					  acct_name, acb_info, unknown,
					  &user_pol, &user_rid);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_close(cli, mem_ctx, &user_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

 done:
	return result;
}

/* Create domain group */

static NTSTATUS cmd_samr_create_dom_group(struct rpc_pipe_client *cli, 
                                          TALLOC_CTX *mem_ctx,
                                          int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, group_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	const char *grp_name;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s groupname [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	grp_name = argv[1];
	
	if (argc > 2)
                sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Create domain user */

	result = rpccli_samr_create_dom_group(cli, mem_ctx, &domain_pol,
					   grp_name, MAXIMUM_ALLOWED_ACCESS,
					   &group_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_close(cli, mem_ctx, &group_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

 done:
	return result;
}

/* Create domain alias */

static NTSTATUS cmd_samr_create_dom_alias(struct rpc_pipe_client *cli, 
                                          TALLOC_CTX *mem_ctx,
                                          int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, alias_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	const char *alias_name;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s aliasname [access mask]\n", argv[0]);
		return NT_STATUS_OK;
	}

	alias_name = argv[1];
	
	if (argc > 2)
                sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					 access_mask,
					 &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Create domain user */

	result = rpccli_samr_create_dom_alias(cli, mem_ctx, &domain_pol,
					      alias_name, &alias_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_close(cli, mem_ctx, &alias_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

 done:
	return result;
}

/* Lookup sam names */

static NTSTATUS cmd_samr_lookup_names(struct rpc_pipe_client *cli, 
                                      TALLOC_CTX *mem_ctx,
                                      int argc, const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol;
	uint32 flags = 0x000003e8; /* Unknown */
	uint32 num_rids, num_names, *name_types, *rids;
	const char **names;
	int i;

	if (argc < 3) {
		printf("Usage: %s  domain|builtin name1 [name2 [name3] [...]]\n", argv[0]);
		printf("check on the domain SID: S-1-5-21-x-y-z\n");
		printf("or check on the builtin SID: S-1-5-32\n");
		return NT_STATUS_OK;
	}

	/* Get sam policy and domain handles */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &global_sid_Builtin, &domain_pol);
	else
		return NT_STATUS_OK;

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Look up names */

	num_names = argc - 2;
	if ((names = TALLOC_ARRAY(mem_ctx, const char *, num_names)) == NULL) {
		rpccli_samr_close(cli, mem_ctx, &domain_pol);
		rpccli_samr_close(cli, mem_ctx, &connect_pol);
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (i = 0; i < argc - 2; i++)
		names[i] = argv[i + 2];

	result = rpccli_samr_lookup_names(cli, mem_ctx, &domain_pol,
				       flags, num_names, names,
				       &num_rids, &rids, &name_types);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Display results */

	for (i = 0; i < num_names; i++)
		printf("name %s: 0x%x (%d)\n", names[i], rids[i], 
		       name_types[i]);

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Lookup sam rids */

static NTSTATUS cmd_samr_lookup_rids(struct rpc_pipe_client *cli, 
                                     TALLOC_CTX *mem_ctx,
                                     int argc, const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol;
	uint32 num_rids, num_names, *rids, *name_types;
	char **names;
	int i;

	if (argc < 3) {
		printf("Usage: %s domain|builtin rid1 [rid2 [rid3] [...]]\n", argv[0]);
		return NT_STATUS_OK;
	}

	/* Get sam policy and domain handles */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	if (StrCaseCmp(argv[1], "domain")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &domain_sid, &domain_pol);
	else if (StrCaseCmp(argv[1], "builtin")==0)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &global_sid_Builtin, &domain_pol);
	else
		return NT_STATUS_OK;

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Look up rids */

	num_rids = argc - 2;
	rids = TALLOC_ARRAY(mem_ctx, uint32, num_rids);
	if ((rids = TALLOC_ARRAY(mem_ctx, uint32, num_rids)) == NULL) {
		rpccli_samr_close(cli, mem_ctx, &domain_pol);
		rpccli_samr_close(cli, mem_ctx, &connect_pol);
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (i = 0; i < argc - 2; i++)
                sscanf(argv[i + 2], "%i", &rids[i]);

	result = rpccli_samr_lookup_rids(cli, mem_ctx, &domain_pol, num_rids, rids,
				      &num_names, &names, &name_types);

	if (!NT_STATUS_IS_OK(result) &&
	    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED))
		goto done;

	/* Display results */

	for (i = 0; i < num_names; i++)
		printf("rid 0x%x: %s (%d)\n", rids[i], names[i], name_types[i]);

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
 done:
	return result;
}

/* Delete domain user */

static NTSTATUS cmd_samr_delete_dom_user(struct rpc_pipe_client *cli, 
                                         TALLOC_CTX *mem_ctx,
                                         int argc, const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol, user_pol;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;

	if ((argc < 2) || (argc > 3)) {
		printf("Usage: %s username\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	if (argc > 2)
                sscanf(argv[2], "%x", &access_mask);

	/* Get sam policy and domain handles */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      MAXIMUM_ALLOWED_ACCESS,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get handle on user */

	{
		uint32 *user_rids, num_rids, *name_types;
		uint32 flags = 0x000003e8; /* Unknown */

		result = rpccli_samr_lookup_names(cli, mem_ctx, &domain_pol,
					       flags, 1, (const char **)&argv[1],
					       &num_rids, &user_rids,
					       &name_types);

		if (!NT_STATUS_IS_OK(result))
			goto done;

		result = rpccli_samr_open_user(cli, mem_ctx, &domain_pol,
					    access_mask,
					    user_rids[0], &user_pol);

		if (!NT_STATUS_IS_OK(result))
			goto done;
	}

	/* Delete user */

	result = rpccli_samr_delete_dom_user(cli, mem_ctx, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Display results */

	rpccli_samr_close(cli, mem_ctx, &user_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);

 done:
	return result;
}

/**********************************************************************
 * Query user security object 
 */
static NTSTATUS cmd_samr_query_sec_obj(struct rpc_pipe_client *cli, 
                                    TALLOC_CTX *mem_ctx,
                                    int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol, user_pol, *pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 sec_info = DACL_SECURITY_INFORMATION;
	fstring server;
	uint32 user_rid = 0;
	TALLOC_CTX *ctx = NULL;
	SEC_DESC_BUF *sec_desc_buf=NULL;
	BOOL domain = False;

	ctx=talloc_init("cmd_samr_query_sec_obj");
	
	if ((argc < 1) || (argc > 3)) {
		printf("Usage: %s [rid|-d] [sec_info]\n", argv[0]);
		printf("\tSpecify rid for security on user, -d for security on domain\n");
		return NT_STATUS_OK;
	}
	
	if (argc > 1) {
		if (strcmp(argv[1], "-d") == 0)
			domain = True;
		else
			sscanf(argv[1], "%i", &user_rid);
	}

	if (argc == 3) {
		sec_info = atoi(argv[2]);
	}
	
	slprintf(server, sizeof(fstring)-1, "\\\\%s", cli->cli->desthost);
	strupper_m(server);
	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS,
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	if (domain || user_rid)
		result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					      MAXIMUM_ALLOWED_ACCESS,
					      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	if (user_rid)
		result = rpccli_samr_open_user(cli, mem_ctx, &domain_pol,
					    MAXIMUM_ALLOWED_ACCESS,
					    user_rid, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Pick which query pol to use */

	pol = &connect_pol;

	if (domain)
		pol = &domain_pol;

	if (user_rid)
		pol = &user_pol;

	/* Query SAM security object */

	result = rpccli_samr_query_sec_obj(cli, mem_ctx, pol, sec_info, ctx, 
					&sec_desc_buf);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	display_sec_desc(sec_desc_buf->sec);

	rpccli_samr_close(cli, mem_ctx, &user_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
done:
	talloc_destroy(ctx);
	return result;
}

static NTSTATUS cmd_samr_get_usrdom_pwinfo(struct rpc_pipe_client *cli, 
					   TALLOC_CTX *mem_ctx,
					   int argc, const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	POLICY_HND connect_pol, domain_pol, user_pol;
	uint16 min_pwd_length;
	uint32 password_properties, unknown1, rid;

	if (argc != 2) {
		printf("Usage: %s rid\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	sscanf(argv[1], "%i", &rid);

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
					 MAXIMUM_ALLOWED_ACCESS, &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_open_user(cli, mem_ctx, &domain_pol,
				       MAXIMUM_ALLOWED_ACCESS,
				       rid, &user_pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_get_usrdom_pwinfo(cli, mem_ctx, &user_pol,
					       &min_pwd_length, &password_properties, 
					       &unknown1) ;

	if (NT_STATUS_IS_OK(result)) {
		printf("min_pwd_length: %d\n", min_pwd_length);
		printf("unknown1: %d\n", unknown1);
		display_password_properties(password_properties);
	}

 done:
	rpccli_samr_close(cli, mem_ctx, &user_pol);
	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);

	return result;
}


static NTSTATUS cmd_samr_get_dom_pwinfo(struct rpc_pipe_client *cli, 
					TALLOC_CTX *mem_ctx,
					int argc, const char **argv) 
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint16 min_pwd_length;
	uint32 password_properties;

	if (argc != 1) {
		printf("Usage: %s\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_samr_get_dom_pwinfo(cli, mem_ctx, &min_pwd_length, &password_properties) ;
	
	if (NT_STATUS_IS_OK(result)) {
		printf("min_pwd_length: %d\n", min_pwd_length);
		display_password_properties(password_properties);
	}

	return result;
}

/* Look up domain name */

static NTSTATUS cmd_samr_lookup_domain(struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx,
				       int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	fstring domain_name,sid_string;
	DOM_SID sid;
	
	if (argc != 2) {
		printf("Usage: %s domain_name\n", argv[0]);
		return NT_STATUS_OK;
	}
	
	sscanf(argv[1], "%s", domain_name);
	
	result = try_samr_connects(cli, mem_ctx, access_mask, &connect_pol);
	
	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask, &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;
	
	result = rpccli_samr_lookup_domain(
		cli, mem_ctx, &connect_pol, domain_name, &sid);

	sid_to_string(sid_string,&sid);
 
	if (NT_STATUS_IS_OK(result)) 
		printf("SAMR_LOOKUP_DOMAIN: Domain Name: %s Domain SID: %s\n",
		       domain_name,sid_string);

	rpccli_samr_close(cli, mem_ctx, &domain_pol);
	rpccli_samr_close(cli, mem_ctx, &connect_pol);
done:
	return result;
}

/* Change user password */

static NTSTATUS cmd_samr_chgpasswd3(struct rpc_pipe_client *cli, 
				    TALLOC_CTX *mem_ctx,
				    int argc, const char **argv) 
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	const char *user, *oldpass, *newpass;
	uint32 access_mask = MAXIMUM_ALLOWED_ACCESS;
	SAM_UNK_INFO_1 info;
	SAMR_CHANGE_REJECT reject;

	if (argc < 3) {
		printf("Usage: %s username oldpass newpass\n", argv[0]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	user = argv[1];
	oldpass = argv[2];
	newpass = argv[3];
	
	/* Get sam policy handle */

	result = try_samr_connects(cli, mem_ctx, MAXIMUM_ALLOWED_ACCESS, 
				   &connect_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Get domain policy handle */

	result = rpccli_samr_open_domain(cli, mem_ctx, &connect_pol,
				      access_mask,
				      &domain_sid, &domain_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Change user password */
	result = rpccli_samr_chgpasswd3(cli, mem_ctx, user, newpass, oldpass, &info, &reject);

	if (NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_RESTRICTION)) {
	
		display_sam_unk_info_1(&info);

		switch (reject.reject_reason) {
			case REJECT_REASON_TOO_SHORT:
				d_printf("REJECT_REASON_TOO_SHORT\n");
				break;
			case REJECT_REASON_IN_HISTORY:
				d_printf("REJECT_REASON_IN_HISTORY\n");
				break;
			case REJECT_REASON_NOT_COMPLEX:
				d_printf("REJECT_REASON_NOT_COMPLEX\n");
				break;
			case REJECT_REASON_OTHER:
				d_printf("REJECT_REASON_OTHER\n");
				break;
			default:
				d_printf("unknown reject reason: %d\n", reject.reject_reason);
				break;
		}
	}

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_samr_close(cli, mem_ctx, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

	result = rpccli_samr_close(cli, mem_ctx, &connect_pol);
	if (!NT_STATUS_IS_OK(result)) goto done;

 done:
	return result;
}

/* List of commands exported by this module */

struct cmd_set samr_commands[] = {

	{ "SAMR" },

	{ "queryuser", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_user, 		NULL, PI_SAMR, NULL,	"Query user info",         "" },
	{ "querygroup", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_group, 		NULL, PI_SAMR, NULL,	"Query group info",        "" },
	{ "queryusergroups", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_usergroups, 	NULL, PI_SAMR, NULL,	"Query user groups",       "" },
	{ "queryuseraliases", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_useraliases, 	NULL, PI_SAMR, NULL,	"Query user aliases",      "" },
	{ "querygroupmem", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_groupmem, 	NULL, PI_SAMR, NULL,	"Query group membership",  "" },
	{ "queryaliasmem", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_aliasmem, 	NULL, PI_SAMR, NULL,	"Query alias membership",  "" },
	{ "deletealias", 	RPC_RTYPE_NTSTATUS, cmd_samr_delete_alias, 	NULL, PI_SAMR, NULL,	"Delete an alias",  "" },
	{ "querydispinfo", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_dispinfo, 	NULL, PI_SAMR, NULL,	"Query display info",      "" },
	{ "querydominfo", 	RPC_RTYPE_NTSTATUS, cmd_samr_query_dominfo, 	NULL, PI_SAMR, NULL,	"Query domain info",       "" },
	{ "enumdomusers",      RPC_RTYPE_NTSTATUS, cmd_samr_enum_dom_users,       NULL, PI_SAMR, NULL,	"Enumerate domain users", "" },
	{ "enumdomgroups",      RPC_RTYPE_NTSTATUS, cmd_samr_enum_dom_groups,       NULL, PI_SAMR, NULL,	"Enumerate domain groups", "" },
	{ "enumalsgroups",      RPC_RTYPE_NTSTATUS, cmd_samr_enum_als_groups,       NULL, PI_SAMR, NULL,	"Enumerate alias groups",  "" },

	{ "createdomuser",      RPC_RTYPE_NTSTATUS, cmd_samr_create_dom_user,       NULL, PI_SAMR, NULL,	"Create domain user",      "" },
	{ "createdomgroup",     RPC_RTYPE_NTSTATUS, cmd_samr_create_dom_group,      NULL, PI_SAMR, NULL,	"Create domain group",     "" },
	{ "createdomalias",     RPC_RTYPE_NTSTATUS, cmd_samr_create_dom_alias,      NULL, PI_SAMR, NULL,	"Create domain alias",     "" },
	{ "samlookupnames",     RPC_RTYPE_NTSTATUS, cmd_samr_lookup_names,          NULL, PI_SAMR, NULL,	"Look up names",           "" },
	{ "samlookuprids",      RPC_RTYPE_NTSTATUS, cmd_samr_lookup_rids,           NULL, PI_SAMR, NULL,	"Look up names",           "" },
	{ "deletedomuser",      RPC_RTYPE_NTSTATUS, cmd_samr_delete_dom_user,       NULL, PI_SAMR, NULL,	"Delete domain user",      "" },
	{ "samquerysecobj",     RPC_RTYPE_NTSTATUS, cmd_samr_query_sec_obj,         NULL, PI_SAMR, NULL, "Query SAMR security object",   "" },
	{ "getdompwinfo",       RPC_RTYPE_NTSTATUS, cmd_samr_get_dom_pwinfo,        NULL, PI_SAMR, NULL, "Retrieve domain password info", "" },
	{ "getusrdompwinfo",    RPC_RTYPE_NTSTATUS, cmd_samr_get_usrdom_pwinfo,     NULL, PI_SAMR, NULL, "Retrieve user domain password info", "" },

	{ "lookupdomain",       RPC_RTYPE_NTSTATUS, cmd_samr_lookup_domain,         NULL, PI_SAMR, NULL, "Lookup Domain Name", "" },
	{ "chgpasswd3",         RPC_RTYPE_NTSTATUS, cmd_samr_chgpasswd3,            NULL, PI_SAMR, NULL, "Change user password", "" },
	{ NULL }
};
