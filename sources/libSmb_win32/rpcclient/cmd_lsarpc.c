/*
   Unix SMB/CIFS implementation.
   RPC pipe client

   Copyright (C) Tim Potter              2000
   Copyright (C) Rafal Szczesniak        2002

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


/* useful function to allow entering a name instead of a SID and
 * looking it up automatically */
static NTSTATUS name_to_sid(struct rpc_pipe_client *cli, 
			    TALLOC_CTX *mem_ctx,
			    DOM_SID *sid, const char *name)
{
	POLICY_HND pol;
	uint32 *sid_types;
	NTSTATUS result;
	DOM_SID *sids;

	/* maybe its a raw SID */
	if (strncmp(name, "S-", 2) == 0 &&
	    string_to_sid(sid, name)) {
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);
	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_lookup_names(cli, mem_ctx, &pol, 1, &name, NULL, &sids, &sid_types);
	if (!NT_STATUS_IS_OK(result))
		goto done;

	rpccli_lsa_close(cli, mem_ctx, &pol);

	*sid = sids[0];

done:
	return result;
}

static void display_query_info_1(DOM_QUERY_1 d)
{
	d_printf("percent_full:\t%d\n", d.percent_full);
	d_printf("log_size:\t%d\n", d.log_size);
	d_printf("retention_time:\t%08x %08x\n", d.retention_time.high, d.retention_time.low);
	d_printf("shutdown_in_progress:\t%d\n", d.shutdown_in_progress);
	d_printf("time_to_shutdown:\t%08x %08x\n", d.time_to_shutdown.high, d.time_to_shutdown.low);
	d_printf("next_audit_record:\t%d\n", d.next_audit_record);
	d_printf("unknown:\t%d\n", d.unknown);
}

static void display_query_info_2(DOM_QUERY_2 d, TALLOC_CTX *mem_ctx)
{
	int i;
	d_printf("Auditing enabled:\t%d\n", d.auditing_enabled);
	d_printf("Auditing categories:\t%d\n", d.count1);
	d_printf("Auditsettings:\n");
	for (i=0; i<d.count1; i++) {
		const char *val = audit_policy_str(mem_ctx, d.auditsettings[i]);
		const char *policy = audit_description_str(i);
		d_printf("%s:\t%s\n", policy, val);
	}
}

static void display_query_info_3(DOM_QUERY_3 d)
{
	fstring name;

	unistr2_to_ascii(name, &d.uni_domain_name, d.uni_dom_max_len);

	d_printf("Domain Name: %s\n", name);
	d_printf("Domain Sid: %s\n", sid_string_static(&d.dom_sid.sid));
}

static void display_query_info_5(DOM_QUERY_5 d)
{
	fstring name;

	unistr2_to_ascii(name, &d.uni_domain_name, d.uni_dom_max_len);

	d_printf("Domain Name: %s\n", name);
	d_printf("Domain Sid: %s\n", sid_string_static(&d.dom_sid.sid));
}

static void display_query_info_10(DOM_QUERY_10 d)
{
	d_printf("Shutdown on full: %d\n", d.shutdown_on_full);
}

static void display_query_info_11(DOM_QUERY_11 d)
{
	d_printf("Shutdown on full: %d\n", d.shutdown_on_full);
	d_printf("Log is full: %d\n", d.log_is_full);
	d_printf("Unknown: %d\n", d.unknown);
}

static void display_query_info_12(DOM_QUERY_12 d)
{
	fstring dom_name, dns_dom_name, forest_name;

	unistr2_to_ascii(dom_name, &d.uni_nb_dom_name, d.hdr_nb_dom_name.uni_max_len);
	unistr2_to_ascii(dns_dom_name, &d.uni_dns_dom_name, d.hdr_dns_dom_name.uni_max_len);
	unistr2_to_ascii(forest_name, &d.uni_forest_name, d.hdr_forest_name.uni_max_len);

	d_printf("Domain NetBios Name: %s\n", dom_name);
	d_printf("Domain DNS Name: %s\n", dns_dom_name);
	d_printf("Domain Forest Name: %s\n", forest_name);
	d_printf("Domain Sid: %s\n", sid_string_static(&d.dom_sid.sid));
	d_printf("Domain GUID: %s\n", smb_uuid_string_static(d.dom_guid));

}



static void display_lsa_query_info(LSA_INFO_CTR *dom, TALLOC_CTX *mem_ctx)
{
	switch (dom->info_class) {
		case 1:
			display_query_info_1(dom->info.id1);
			break;
		case 2:
			display_query_info_2(dom->info.id2, mem_ctx);
			break;
		case 3:
			display_query_info_3(dom->info.id3);
			break;
		case 5:
			display_query_info_5(dom->info.id5);
			break;
		case 10:
			display_query_info_10(dom->info.id10);
			break;
		case 11:
			display_query_info_11(dom->info.id11);
			break;
		case 12:
			display_query_info_12(dom->info.id12);
			break;
		default:
			printf("can't display info level: %d\n", dom->info_class);
			break;
	}
}

static NTSTATUS cmd_lsa_query_info_policy(struct rpc_pipe_client *cli, 
                                          TALLOC_CTX *mem_ctx, int argc, 
                                          const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LSA_INFO_CTR dom;

	uint32 info_class = 3;

	if (argc > 2) {
		printf("Usage: %s [info_class]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 2)
		info_class = atoi(argv[1]);

	switch (info_class) {
	case 12:
		result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
						 SEC_RIGHTS_MAXIMUM_ALLOWED,
						 &pol);

		if (!NT_STATUS_IS_OK(result))
			goto done;
			
		result = rpccli_lsa_query_info_policy2_new(cli, mem_ctx, &pol,
							   info_class, &dom);
		break;
	default:
		result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
						SEC_RIGHTS_MAXIMUM_ALLOWED,
						&pol);

		if (!NT_STATUS_IS_OK(result))
			goto done;
		
		result = rpccli_lsa_query_info_policy_new(cli, mem_ctx, &pol, 
							  info_class, &dom);
	}


	display_lsa_query_info(&dom, mem_ctx);

	rpccli_lsa_close(cli, mem_ctx, &pol);

 done:
	return result;
}

/* Resolve a list of names to a list of sids */

static NTSTATUS cmd_lsa_lookup_names(struct rpc_pipe_client *cli, 
                                     TALLOC_CTX *mem_ctx, int argc, 
                                     const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID *sids;
	uint32 *types;
	int i;

	if (argc == 1) {
		printf("Usage: %s [name1 [name2 [...]]]\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_lookup_names(cli, mem_ctx, &pol, argc - 1, 
				      (const char**)(argv + 1), NULL, &sids, &types);

	if (!NT_STATUS_IS_OK(result) && NT_STATUS_V(result) != 
	    NT_STATUS_V(STATUS_SOME_UNMAPPED))
		goto done;

	result = NT_STATUS_OK;

	/* Print results */

	for (i = 0; i < (argc - 1); i++) {
		fstring sid_str;
		sid_to_string(sid_str, &sids[i]);
		printf("%s %s (%s: %d)\n", argv[i + 1], sid_str,
		       sid_type_lookup(types[i]), types[i]);
	}

	rpccli_lsa_close(cli, mem_ctx, &pol);

 done:
	return result;
}

/* Resolve a list of SIDs to a list of names */

static NTSTATUS cmd_lsa_lookup_sids(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
                                    int argc, const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID *sids;
	char **domains;
	char **names;
	uint32 *types;
	int i;

	if (argc == 1) {
		printf("Usage: %s [sid1 [sid2 [...]]]\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Convert arguments to sids */

	sids = TALLOC_ARRAY(mem_ctx, DOM_SID, argc - 1);

	if (!sids) {
		printf("could not allocate memory for %d sids\n", argc - 1);
		goto done;
	}

	for (i = 0; i < argc - 1; i++) 
		if (!string_to_sid(&sids[i], argv[i + 1])) {
			result = NT_STATUS_INVALID_SID;
			goto done;
		}

	/* Lookup the SIDs */

	result = rpccli_lsa_lookup_sids(cli, mem_ctx, &pol, argc - 1, sids, 
				     &domains, &names, &types);

	if (!NT_STATUS_IS_OK(result) && NT_STATUS_V(result) != 
	    NT_STATUS_V(STATUS_SOME_UNMAPPED))
		goto done;

	result = NT_STATUS_OK;

	/* Print results */

	for (i = 0; i < (argc - 1); i++) {
		fstring sid_str;

		sid_to_string(sid_str, &sids[i]);
		printf("%s %s\\%s (%d)\n", sid_str, 
		       domains[i] ? domains[i] : "*unknown*", 
		       names[i] ? names[i] : "*unknown*", types[i]);
	}

	rpccli_lsa_close(cli, mem_ctx, &pol);

 done:
	return result;
}

/* Enumerate list of trusted domains */

static NTSTATUS cmd_lsa_enum_trust_dom(struct rpc_pipe_client *cli, 
                                       TALLOC_CTX *mem_ctx, int argc, 
                                       const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID *domain_sids;
	char **domain_names;

	/* defaults, but may be changed using params */
	uint32 enum_ctx = 0;
	uint32 num_domains = 0;
	int i;

	if (argc > 2) {
		printf("Usage: %s [enum context (0)]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 2 && argv[1]) {
		enum_ctx = atoi(argv[2]);
	}	

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     POLICY_VIEW_LOCAL_INFORMATION,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = STATUS_MORE_ENTRIES;

	while (NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES)) {

		/* Lookup list of trusted domains */

		result = rpccli_lsa_enum_trust_dom(cli, mem_ctx, &pol, &enum_ctx,
						&num_domains,
						&domain_names, &domain_sids);
		if (!NT_STATUS_IS_OK(result) &&
		    !NT_STATUS_EQUAL(result, NT_STATUS_NO_MORE_ENTRIES) &&
		    !NT_STATUS_EQUAL(result, STATUS_MORE_ENTRIES))
			goto done;

		/* Print results: list of names and sids returned in this
		 * response. */	 
		for (i = 0; i < num_domains; i++) {
			fstring sid_str;

			sid_to_string(sid_str, &domain_sids[i]);
			printf("%s %s\n", domain_names[i] ? domain_names[i] : 
			       "*unknown*", sid_str);
		}
	}

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

/* Enumerates privileges */

static NTSTATUS cmd_lsa_enum_privilege(struct rpc_pipe_client *cli, 
				       TALLOC_CTX *mem_ctx, int argc, 
				       const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	uint32 enum_context=0;
	uint32 pref_max_length=0x1000;
	uint32 count=0;
	char   **privs_name;
	uint32 *privs_high;
	uint32 *privs_low;
	int i;

	if (argc > 3) {
		printf("Usage: %s [enum context] [max length]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc>=2)
		enum_context=atoi(argv[1]);

	if (argc==3)
		pref_max_length=atoi(argv[2]);

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_enum_privilege(cli, mem_ctx, &pol, &enum_context, pref_max_length,
					&count, &privs_name, &privs_high, &privs_low);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */
	printf("found %d privileges\n\n", count);

	for (i = 0; i < count; i++) {
		printf("%s \t\t%d:%d (0x%x:0x%x)\n", privs_name[i] ? privs_name[i] : "*unknown*",
		       privs_high[i], privs_low[i], privs_high[i], privs_low[i]);
	}

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

/* Get privilege name */

static NTSTATUS cmd_lsa_get_dispname(struct rpc_pipe_client *cli, 
                                     TALLOC_CTX *mem_ctx, int argc, 
                                     const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	uint16 lang_id=0;
	uint16 lang_id_sys=0;
	uint16 lang_id_desc;
	fstring description;

	if (argc != 2) {
		printf("Usage: %s privilege name\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_get_dispname(cli, mem_ctx, &pol, argv[1], lang_id, lang_id_sys, description, &lang_id_desc);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */
	printf("%s -> %s (language: 0x%x)\n", argv[1], description, lang_id_desc);

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

/* Enumerate the LSA SIDS */

static NTSTATUS cmd_lsa_enum_sids(struct rpc_pipe_client *cli, 
				  TALLOC_CTX *mem_ctx, int argc, 
				  const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	uint32 enum_context=0;
	uint32 pref_max_length=0x1000;
	DOM_SID *sids;
	uint32 count=0;
	int i;

	if (argc > 3) {
		printf("Usage: %s [enum context] [max length]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc>=2)
		enum_context=atoi(argv[1]);

	if (argc==3)
		pref_max_length=atoi(argv[2]);

	result = rpccli_lsa_open_policy(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_enum_sids(cli, mem_ctx, &pol, &enum_context, pref_max_length,
					&count, &sids);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */
	printf("found %d SIDs\n\n", count);

	for (i = 0; i < count; i++) {
		fstring sid_str;

		sid_to_string(sid_str, &sids[i]);
		printf("%s\n", sid_str);
	}

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

/* Create a new account */

static NTSTATUS cmd_lsa_create_account(struct rpc_pipe_client *cli, 
                                           TALLOC_CTX *mem_ctx, int argc, 
                                           const char **argv) 
{
	POLICY_HND dom_pol;
	POLICY_HND user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 des_access = 0x000f000f;
	
	DOM_SID sid;

	if (argc != 2 ) {
		printf("Usage: %s SID\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = name_to_sid(cli, mem_ctx, &sid, argv[1]);
	if (!NT_STATUS_IS_OK(result))
		goto done;	

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_create_account(cli, mem_ctx, &dom_pol, &sid, des_access, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	printf("Account for SID %s successfully created\n\n", argv[1]);
	result = NT_STATUS_OK;

	rpccli_lsa_close(cli, mem_ctx, &dom_pol);
 done:
	return result;
}


/* Enumerate the privileges of an SID */

static NTSTATUS cmd_lsa_enum_privsaccounts(struct rpc_pipe_client *cli, 
                                           TALLOC_CTX *mem_ctx, int argc, 
                                           const char **argv) 
{
	POLICY_HND dom_pol;
	POLICY_HND user_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 access_desired = 0x000f000f;
	
	DOM_SID sid;
	uint32 count=0;
	LUID_ATTR *set;
	int i;

	if (argc != 2 ) {
		printf("Usage: %s SID\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = name_to_sid(cli, mem_ctx, &sid, argv[1]);
	if (!NT_STATUS_IS_OK(result))
		goto done;	

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_open_account(cli, mem_ctx, &dom_pol, &sid, access_desired, &user_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_enum_privsaccount(cli, mem_ctx, &user_pol, &count, &set);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */
	printf("found %d privileges for SID %s\n\n", count, argv[1]);
	printf("high\tlow\tattribute\n");

	for (i = 0; i < count; i++) {
		printf("%u\t%u\t%u\n", set[i].luid.high, set[i].luid.low, set[i].attr);
	}

	rpccli_lsa_close(cli, mem_ctx, &dom_pol);
 done:
	return result;
}


/* Enumerate the privileges of an SID via LsaEnumerateAccountRights */

static NTSTATUS cmd_lsa_enum_acct_rights(struct rpc_pipe_client *cli, 
					 TALLOC_CTX *mem_ctx, int argc, 
					 const char **argv) 
{
	POLICY_HND dom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DOM_SID sid;
	uint32 count;
	char **rights;

	int i;

	if (argc != 2 ) {
		printf("Usage: %s SID\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = name_to_sid(cli, mem_ctx, &sid, argv[1]);
	if (!NT_STATUS_IS_OK(result))
		goto done;	

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_enum_account_rights(cli, mem_ctx, &dom_pol, &sid, &count, &rights);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	printf("found %d privileges for SID %s\n", count, sid_string_static(&sid));

	for (i = 0; i < count; i++) {
		printf("\t%s\n", rights[i]);
	}

	rpccli_lsa_close(cli, mem_ctx, &dom_pol);
 done:
	return result;
}


/* add some privileges to a SID via LsaAddAccountRights */

static NTSTATUS cmd_lsa_add_acct_rights(struct rpc_pipe_client *cli, 
					TALLOC_CTX *mem_ctx, int argc, 
					const char **argv) 
{
	POLICY_HND dom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DOM_SID sid;

	if (argc < 3 ) {
		printf("Usage: %s SID [rights...]\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = name_to_sid(cli, mem_ctx, &sid, argv[1]);
	if (!NT_STATUS_IS_OK(result))
		goto done;	

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_add_account_rights(cli, mem_ctx, &dom_pol, sid, 
					    argc-2, argv+2);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	rpccli_lsa_close(cli, mem_ctx, &dom_pol);
 done:
	return result;
}


/* remove some privileges to a SID via LsaRemoveAccountRights */

static NTSTATUS cmd_lsa_remove_acct_rights(struct rpc_pipe_client *cli, 
					TALLOC_CTX *mem_ctx, int argc, 
					const char **argv) 
{
	POLICY_HND dom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;

	DOM_SID sid;

	if (argc < 3 ) {
		printf("Usage: %s SID [rights...]\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = name_to_sid(cli, mem_ctx, &sid, argv[1]);
	if (!NT_STATUS_IS_OK(result))
		goto done;	

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &dom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_remove_account_rights(cli, mem_ctx, &dom_pol, sid, 
					       False, argc-2, argv+2);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	rpccli_lsa_close(cli, mem_ctx, &dom_pol);

 done:
	return result;
}


/* Get a privilege value given its name */

static NTSTATUS cmd_lsa_lookup_priv_value(struct rpc_pipe_client *cli, 
					TALLOC_CTX *mem_ctx, int argc, 
					const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LUID luid;

	if (argc != 2 ) {
		printf("Usage: %s name\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				     SEC_RIGHTS_MAXIMUM_ALLOWED,
				     &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_lookup_priv_value(cli, mem_ctx, &pol, argv[1], &luid);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */

	printf("%u:%u (0x%x:0x%x)\n", luid.high, luid.low, luid.high, luid.low);

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

/* Query LSA security object */

static NTSTATUS cmd_lsa_query_secobj(struct rpc_pipe_client *cli, 
				     TALLOC_CTX *mem_ctx, int argc, 
				     const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	SEC_DESC_BUF *sdb;
	uint32 sec_info = DACL_SECURITY_INFORMATION;

	if (argc < 1 || argc > 2) {
		printf("Usage: %s [sec_info]\n", argv[0]);
		return NT_STATUS_OK;
	}

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, 
				      SEC_RIGHTS_MAXIMUM_ALLOWED,
				      &pol);

	if (argc == 2) 
		sscanf(argv[1], "%x", &sec_info);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_query_secobj(cli, mem_ctx, &pol, sec_info, &sdb);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	/* Print results */

	display_sec_desc(sdb->sec);

	rpccli_lsa_close(cli, mem_ctx, &pol);
 done:
	return result;
}

static void display_trust_dom_info_1(TRUSTED_DOMAIN_INFO_NAME *n)
{
	printf("NetBIOS Name:\t%s\n", unistr2_static(&n->netbios_name.unistring));
}

static void display_trust_dom_info_3(TRUSTED_DOMAIN_INFO_POSIX_OFFSET *p)
{
	printf("Posix Offset:\t%08x (%d)\n", p->posix_offset, p->posix_offset);
}

static void display_trust_dom_info_4(TRUSTED_DOMAIN_INFO_PASSWORD *p, const char *password)
{
	char *pwd, *pwd_old;
	
	DATA_BLOB data 	   = data_blob(NULL, p->password.length);
	DATA_BLOB data_old = data_blob(NULL, p->old_password.length);

	memcpy(data.data, p->password.data, p->password.length);
	memcpy(data_old.data, p->old_password.data, p->old_password.length);
	
	pwd 	= decrypt_trustdom_secret(password, &data);
	pwd_old = decrypt_trustdom_secret(password, &data_old);
	
	d_printf("Password:\t%s\n", pwd);
	d_printf("Old Password:\t%s\n", pwd_old);

	SAFE_FREE(pwd);
	SAFE_FREE(pwd_old);
	
	data_blob_free(&data);
	data_blob_free(&data_old);
}

static void display_trust_dom_info_6(TRUSTED_DOMAIN_INFO_EX *i)
{
	printf("Domain Name:\t\t%s\n", unistr2_static(&i->domain_name.unistring));
	printf("NetBIOS Name:\t\t%s\n", unistr2_static(&i->netbios_name.unistring));
	printf("SID:\t\t\t%s\n", sid_string_static(&i->sid.sid));
	printf("Trust Direction:\t0x%08x\n", i->trust_direction);
	printf("Trust Type:\t\t0x%08x\n", i->trust_type);
	printf("Trust Attributes:\t0x%08x\n", i->trust_attributes);
}


static void display_trust_dom_info(LSA_TRUSTED_DOMAIN_INFO *info, uint32 info_class, const char *pass)
{
	switch (info_class) {
	case 1:
		display_trust_dom_info_1(&info->name);
		break;
	case 3:
		display_trust_dom_info_3(&info->posix_offset);
		break;
	case 4:
		display_trust_dom_info_4(&info->password, pass);
		break;
	case 6:
		display_trust_dom_info_6(&info->info_ex);
		break;
	default:
		printf("unsupported info-class: %d\n", info_class);
		break;
	}
}

static NTSTATUS cmd_lsa_query_trustdominfobysid(struct rpc_pipe_client *cli,
						TALLOC_CTX *mem_ctx, int argc, 
						const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	DOM_SID dom_sid;
	uint32 access_mask = SEC_RIGHTS_MAXIMUM_ALLOWED;
	LSA_TRUSTED_DOMAIN_INFO *info;

	uint32 info_class = 1; 

	if (argc > 3 || argc < 2) {
		printf("Usage: %s [sid] [info_class]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (!string_to_sid(&dom_sid, argv[1]))
		return NT_STATUS_NO_MEMORY;

	if (argc == 3)
		info_class = atoi(argv[2]);

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, access_mask, &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_query_trusted_domain_info_by_sid(cli, mem_ctx, &pol,
							  info_class, &dom_sid, &info);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	display_trust_dom_info(info, info_class, cli->pwd.password);

 done:
	if (&pol)
		rpccli_lsa_close(cli, mem_ctx, &pol);

	return result;
}

static NTSTATUS cmd_lsa_query_trustdominfobyname(struct rpc_pipe_client *cli,
						 TALLOC_CTX *mem_ctx, int argc,
						 const char **argv) 
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 access_mask = SEC_RIGHTS_MAXIMUM_ALLOWED;
	LSA_TRUSTED_DOMAIN_INFO *info;
	uint32 info_class = 1; 

	if (argc > 3 || argc < 2) {
		printf("Usage: %s [name] [info_class]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (argc == 3)
		info_class = atoi(argv[2]);

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, access_mask, &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_query_trusted_domain_info_by_name(cli, mem_ctx, &pol, 
							   info_class, argv[1], &info);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	display_trust_dom_info(info, info_class, cli->pwd.password);

 done:
	if (&pol)
		rpccli_lsa_close(cli, mem_ctx, &pol);

	return result;
}

static NTSTATUS cmd_lsa_query_trustdominfo(struct rpc_pipe_client *cli,
					   TALLOC_CTX *mem_ctx, int argc,
					   const char **argv) 
{
	POLICY_HND pol, trustdom_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	uint32 access_mask = SEC_RIGHTS_MAXIMUM_ALLOWED;
	LSA_TRUSTED_DOMAIN_INFO *info;
	DOM_SID dom_sid;
	uint32 info_class = 1; 

	if (argc > 3 || argc < 2) {
		printf("Usage: %s [sid] [info_class]\n", argv[0]);
		return NT_STATUS_OK;
	}

	if (!string_to_sid(&dom_sid, argv[1]))
		return NT_STATUS_NO_MEMORY;


	if (argc == 3)
		info_class = atoi(argv[2]);

	result = rpccli_lsa_open_policy2(cli, mem_ctx, True, access_mask, &pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;
	
	result = rpccli_lsa_open_trusted_domain(cli, mem_ctx, &pol,
					     &dom_sid, access_mask, &trustdom_pol);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	result = rpccli_lsa_query_trusted_domain_info(cli, mem_ctx, &trustdom_pol, 
						   info_class, &info);

	if (!NT_STATUS_IS_OK(result))
		goto done;

	display_trust_dom_info(info, info_class, cli->pwd.password);

 done:
	if (&pol)
		rpccli_lsa_close(cli, mem_ctx, &pol);

	return result;
}



/* List of commands exported by this module */

struct cmd_set lsarpc_commands[] = {

	{ "LSARPC" },

	{ "lsaquery", 	         RPC_RTYPE_NTSTATUS, cmd_lsa_query_info_policy,  NULL, PI_LSARPC, NULL, "Query info policy",                    "" },
	{ "lookupsids",          RPC_RTYPE_NTSTATUS, cmd_lsa_lookup_sids,        NULL, PI_LSARPC, NULL, "Convert SIDs to names",                "" },
	{ "lookupnames",         RPC_RTYPE_NTSTATUS, cmd_lsa_lookup_names,       NULL, PI_LSARPC, NULL, "Convert names to SIDs",                "" },
	{ "enumtrust", 	         RPC_RTYPE_NTSTATUS, cmd_lsa_enum_trust_dom,     NULL, PI_LSARPC, NULL, "Enumerate trusted domains",            "Usage: [preferred max number] [enum context (0)]" },
	{ "enumprivs", 	         RPC_RTYPE_NTSTATUS, cmd_lsa_enum_privilege,     NULL, PI_LSARPC, NULL, "Enumerate privileges",                 "" },
	{ "getdispname",         RPC_RTYPE_NTSTATUS, cmd_lsa_get_dispname,       NULL, PI_LSARPC, NULL, "Get the privilege name",               "" },
	{ "lsaenumsid",          RPC_RTYPE_NTSTATUS, cmd_lsa_enum_sids,          NULL, PI_LSARPC, NULL, "Enumerate the LSA SIDS",               "" },
	{ "lsacreateaccount",    RPC_RTYPE_NTSTATUS, cmd_lsa_create_account,     NULL, PI_LSARPC, NULL, "Create a new lsa account",   "" },
	{ "lsaenumprivsaccount", RPC_RTYPE_NTSTATUS, cmd_lsa_enum_privsaccounts, NULL, PI_LSARPC, NULL, "Enumerate the privileges of an SID",   "" },
	{ "lsaenumacctrights",   RPC_RTYPE_NTSTATUS, cmd_lsa_enum_acct_rights,   NULL, PI_LSARPC, NULL, "Enumerate the rights of an SID",   "" },
#if 0
	{ "lsaaddpriv",          RPC_RTYPE_NTSTATUS, cmd_lsa_add_priv,           NULL, PI_LSARPC, "Assign a privilege to a SID", "" },
	{ "lsadelpriv",          RPC_RTYPE_NTSTATUS, cmd_lsa_del_priv,           NULL, PI_LSARPC, "Revoke a privilege from a SID", "" },
#endif
	{ "lsaaddacctrights",    RPC_RTYPE_NTSTATUS, cmd_lsa_add_acct_rights,    NULL, PI_LSARPC, NULL, "Add rights to an account",   "" },
	{ "lsaremoveacctrights", RPC_RTYPE_NTSTATUS, cmd_lsa_remove_acct_rights, NULL, PI_LSARPC, NULL, "Remove rights from an account",   "" },
	{ "lsalookupprivvalue",  RPC_RTYPE_NTSTATUS, cmd_lsa_lookup_priv_value,  NULL, PI_LSARPC, NULL, "Get a privilege value given its name", "" },
	{ "lsaquerysecobj",      RPC_RTYPE_NTSTATUS, cmd_lsa_query_secobj,       NULL, PI_LSARPC, NULL, "Query LSA security object", "" },
	{ "lsaquerytrustdominfo",RPC_RTYPE_NTSTATUS, cmd_lsa_query_trustdominfo, NULL, PI_LSARPC, NULL, "Query LSA trusted domains info (given a SID)", "" },
	{ "lsaquerytrustdominfobyname",RPC_RTYPE_NTSTATUS, cmd_lsa_query_trustdominfobyname, NULL, PI_LSARPC, NULL, "Query LSA trusted domains info (given a name), only works for Windows > 2k", "" },
	{ "lsaquerytrustdominfobysid",RPC_RTYPE_NTSTATUS, cmd_lsa_query_trustdominfobysid, NULL, PI_LSARPC, NULL, "Query LSA trusted domains info (given a SID)", "" },

	{ NULL }
};

