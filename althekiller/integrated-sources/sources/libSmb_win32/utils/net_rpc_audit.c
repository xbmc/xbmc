/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2006 Guenther Deschner

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
#include "includes.h"
#include "utils/net.h"

/********************************************************************
********************************************************************/

static int net_help_audit(int argc, const char **argv)
{
	d_printf("net rpc audit list                       View configured Auditing policies\n");
	d_printf("net rpc audit enable                     Enable Auditing\n");
	d_printf("net rpc audit disable                    Disable Auditing\n");
	d_printf("net rpc audit get <category>             View configured Auditing policy setting\n");
	d_printf("net rpc audit set <category> <policy>    Set Auditing policies\n\n");
	d_printf("\tcategory can be one of: SYSTEM, LOGON, OBJECT, PRIVILEGE, PROCESS, POLICY, SAM, DIRECTORY or ACCOUNT\n");
	d_printf("\tpolicy can be one of: SUCCESS, FAILURE, ALL or NONE\n\n");

	return -1;
}

/********************************************************************
********************************************************************/

static void print_auditing_category(const char *policy, const char *value)
{
	fstring padding;
	int pad_len, col_len = 30;

	if (policy == NULL) {
		policy = "Unknown";
	}
	if (value == NULL) {
		value = "Invalid";
	}

	/* calculate padding space for d_printf to look nicer */
	pad_len = col_len - strlen(policy);
	padding[pad_len] = 0;
	do padding[--pad_len] = ' '; while (pad_len > 0);
			
	d_printf("\t%s%s%s\n", policy, padding, value);
}


/********************************************************************
********************************************************************/

static NTSTATUS rpc_audit_get_internal(const DOM_SID *domain_sid,
				       const char *domain_name, 
				       struct cli_state *cli,
				       struct rpc_pipe_client *pipe_hnd,
				       TALLOC_CTX *mem_ctx, 
				       int argc,
				       const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LSA_INFO_CTR dom; 
	int i;

	uint32 info_class = 2;
	uint32 audit_category;

	if (argc < 1 || argc > 2) {
		d_printf("insufficient arguments\n");
		net_help_audit(argc, argv);
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!get_audit_category_from_param(argv[0], &audit_category)) {
		d_printf("invalid auditing category: %s\n", argv[0]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
					SEC_RIGHTS_MAXIMUM_ALLOWED,
					&pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_query_info_policy_new(pipe_hnd, mem_ctx, &pol, 
						  info_class,
						  &dom);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	for (i=0; i < dom.info.id2.count1; i++) {

		const char *val = NULL, *policy = NULL;

		if (i != audit_category) {
			continue;
		}

		val = audit_policy_str(mem_ctx, dom.info.id2.auditsettings[i]);
		policy = audit_description_str(i);
		print_auditing_category(policy, val);
	}

 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_printf("failed to get auditing policy: %s\n", nt_errstr(result));
	}

	return result;
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_audit_set_internal(const DOM_SID *domain_sid,
				       const char *domain_name, 
				       struct cli_state *cli,
				       struct rpc_pipe_client *pipe_hnd,
				       TALLOC_CTX *mem_ctx, 
				       int argc,
				       const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LSA_INFO_CTR dom; 

	uint32 info_class = 2;
	uint32 audit_policy, audit_category;

	if (argc < 2 || argc > 3) {
		d_printf("insufficient arguments\n");
		net_help_audit(argc, argv);
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!get_audit_category_from_param(argv[0], &audit_category)) {
		d_printf("invalid auditing category: %s\n", argv[0]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	audit_policy = LSA_AUDIT_POLICY_CLEAR;

	if (strequal(argv[1], "Success")) {
		audit_policy |= LSA_AUDIT_POLICY_SUCCESS;
	} else if (strequal(argv[1], "Failure")) {
		audit_policy |= LSA_AUDIT_POLICY_FAILURE;
	} else if (strequal(argv[1], "All")) {
		audit_policy |= LSA_AUDIT_POLICY_ALL;
	} else if (strequal(argv[1], "None")) {
		audit_policy = LSA_AUDIT_POLICY_CLEAR;
	} else {
		d_printf("invalid auditing policy: %s\n", argv[1]);
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
					SEC_RIGHTS_MAXIMUM_ALLOWED,
					&pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_query_info_policy_new(pipe_hnd, mem_ctx, &pol, 
						  info_class,
						  &dom);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	dom.info.id2.auditsettings[audit_category] = audit_policy;

	result = rpccli_lsa_set_info_policy(pipe_hnd, mem_ctx, &pol, 
					    info_class,
					    dom);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_query_info_policy_new(pipe_hnd, mem_ctx, &pol, 
						  info_class,
						  &dom);

	{
		const char *val = audit_policy_str(mem_ctx, dom.info.id2.auditsettings[audit_category]);
		const char *policy = audit_description_str(audit_category);
		print_auditing_category(policy, val);
	}

 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_printf("failed to set audit policy: %s\n", nt_errstr(result));
	}
 
	return result;
}

static NTSTATUS rpc_audit_enable_internal_ext(struct rpc_pipe_client *pipe_hnd, 
					      TALLOC_CTX *mem_ctx,
					      int argc,
					      const char **argv,
					      BOOL enable)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LSA_INFO_CTR dom;

	uint32 info_class = 2;

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
					SEC_RIGHTS_MAXIMUM_ALLOWED,
					&pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_query_info_policy_new(pipe_hnd, mem_ctx, &pol, 
						  info_class,
						  &dom);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	dom.info.id2.auditing_enabled = enable;

	result = rpccli_lsa_set_info_policy(pipe_hnd, mem_ctx, &pol, 
					    info_class,
					    dom);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_printf("failed to %s audit policy: %s\n", enable ? "enable":"disable", 
			nt_errstr(result));
	}

	return result;
}
/********************************************************************
********************************************************************/

static NTSTATUS rpc_audit_disable_internal(const DOM_SID *domain_sid,
					   const char *domain_name, 
					   struct cli_state *cli,
					   struct rpc_pipe_client *pipe_hnd,
					   TALLOC_CTX *mem_ctx, 
					   int argc,
					   const char **argv)
{
	return rpc_audit_enable_internal_ext(pipe_hnd, mem_ctx, argc, argv, False);
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_audit_enable_internal(const DOM_SID *domain_sid,
					  const char *domain_name, 
					  struct cli_state *cli,
					  struct rpc_pipe_client *pipe_hnd,
					  TALLOC_CTX *mem_ctx, 
					  int argc,
					  const char **argv)
{
	return rpc_audit_enable_internal_ext(pipe_hnd, mem_ctx, argc, argv, True);
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_audit_list_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv)
{
	POLICY_HND pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	LSA_INFO_CTR dom;
	int i;

	uint32 info_class = 2;

	result = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, True, 
					SEC_RIGHTS_MAXIMUM_ALLOWED,
					&pol);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_lsa_query_info_policy_new(pipe_hnd, mem_ctx, &pol, 
						  info_class,
						  &dom);

	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	printf("Auditing:\t\t");
	switch (dom.info.id2.auditing_enabled) {
		case True:
			printf("Enabled");
			break;
		case False:
			printf("Disabled");
			break;
		default:
			printf("unknown (%d)", dom.info.id2.auditing_enabled);
			break;
	}
	printf("\n");

	printf("Auditing categories:\t%d\n", dom.info.id2.count1);
	printf("Auditing settings:\n");

	for (i=0; i < dom.info.id2.count1; i++) {
		const char *val = audit_policy_str(mem_ctx, dom.info.id2.auditsettings[i]);
		const char *policy = audit_description_str(i);
		print_auditing_category(policy, val);
	}

 done:
	if (!NT_STATUS_IS_OK(result)) {
		d_printf("failed to list auditing policies: %s\n", nt_errstr(result));
	}

	return result;
}



/********************************************************************
********************************************************************/

static int rpc_audit_get(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_LSARPC, 0, 
		rpc_audit_get_internal, argc, argv);
}

/********************************************************************
********************************************************************/

static int rpc_audit_set(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_LSARPC, 0, 
		rpc_audit_set_internal, argc, argv);
}

/********************************************************************
********************************************************************/

static int rpc_audit_enable(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_LSARPC, 0, 
		rpc_audit_enable_internal, argc, argv);
}

/********************************************************************
********************************************************************/

static int rpc_audit_disable(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_LSARPC, 0, 
		rpc_audit_disable_internal, argc, argv);
}

/********************************************************************
********************************************************************/

static int rpc_audit_list(int argc, const char **argv)
{
	return run_rpc_command(NULL, PI_LSARPC, 0, 
		rpc_audit_list_internal, argc, argv);
}

/********************************************************************
********************************************************************/

int net_rpc_audit(int argc, const char **argv) 
{
	struct functable func[] = {
		{"get", rpc_audit_get},
		{"set", rpc_audit_set},
		{"enable", rpc_audit_enable},
		{"disable", rpc_audit_disable},
		{"list", rpc_audit_list},
		{NULL, NULL}
	};
	
	if (argc)
		return net_run_function(argc, argv, func, net_help_audit);
		
	return net_help_audit(argc, argv);
}
