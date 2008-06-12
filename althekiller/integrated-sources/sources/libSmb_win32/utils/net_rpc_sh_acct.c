/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) 2006 Volker Lendecke (vl@samba.org)

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

/*
 * Do something with the account policies. Read them all, run a function on
 * them and possibly write them back. "fn" has to return the container index
 * it has modified, it can return 0 for no change.
 */

static NTSTATUS rpc_sh_acct_do(TALLOC_CTX *mem_ctx,
			       struct rpc_sh_ctx *ctx,
			       struct rpc_pipe_client *pipe_hnd,
			       int argc, const char **argv,
			       BOOL (*fn)(TALLOC_CTX *mem_ctx,
					  struct rpc_sh_ctx *ctx,
					  SAM_UNK_INFO_1 *i1,
					  SAM_UNK_INFO_3 *i3,
					  SAM_UNK_INFO_12 *i12,
					  int argc, const char **argv))
{
	POLICY_HND connect_pol, domain_pol;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	SAM_UNK_CTR ctr1, ctr3, ctr12;
	int store;

	ZERO_STRUCT(connect_pol);
	ZERO_STRUCT(domain_pol);

	/* Get sam policy handle */
	
	result = rpccli_samr_connect(pipe_hnd, mem_ctx,
				     MAXIMUM_ALLOWED_ACCESS, 
				     &connect_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}
	
	/* Get domain policy handle */
	
	result = rpccli_samr_open_domain(pipe_hnd, mem_ctx, &connect_pol,
					 MAXIMUM_ALLOWED_ACCESS,
					 ctx->domain_sid, &domain_pol);
	if (!NT_STATUS_IS_OK(result)) {
		goto done;
	}

	result = rpccli_samr_query_dom_info(pipe_hnd, mem_ctx, &domain_pol,
					    1, &ctr1);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "query_domain_info level 1 failed: %s\n",
			  nt_errstr(result));
		goto done;
	}

	result = rpccli_samr_query_dom_info(pipe_hnd, mem_ctx, &domain_pol,
					    3, &ctr3);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "query_domain_info level 3 failed: %s\n",
			  nt_errstr(result));
		goto done;
	}

	result = rpccli_samr_query_dom_info(pipe_hnd, mem_ctx, &domain_pol,
					    12, &ctr12);

	if (!NT_STATUS_IS_OK(result)) {
		d_fprintf(stderr, "query_domain_info level 12 failed: %s\n",
			  nt_errstr(result));
		goto done;
	}

	store = fn(mem_ctx, ctx, &ctr1.info.inf1, &ctr3.info.inf3,
		   &ctr12.info.inf12, argc, argv);

	if (store <= 0) {
		/* Don't save anything */
		goto done;
	}

	switch (store) {
	case 1:
		result = rpccli_samr_set_domain_info(pipe_hnd, mem_ctx,
						     &domain_pol, 1, &ctr1);
		break;
	case 3:
		result = rpccli_samr_set_domain_info(pipe_hnd, mem_ctx,
						     &domain_pol, 3, &ctr3);
		break;
	case 12:
		result = rpccli_samr_set_domain_info(pipe_hnd, mem_ctx,
						     &domain_pol, 12, &ctr12);
		break;
	default:
		d_fprintf(stderr, "Got unexpected info level %d\n", store);
		result = NT_STATUS_INTERNAL_ERROR;
		goto done;
	}

 done:
	if (is_valid_policy_hnd(&domain_pol)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &domain_pol);
	}
	if (is_valid_policy_hnd(&connect_pol)) {
		rpccli_samr_close(pipe_hnd, mem_ctx, &connect_pol);
	}

	return result;
}

static int account_show(TALLOC_CTX *mem_ctx, struct rpc_sh_ctx *ctx,
			SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
			SAM_UNK_INFO_12 *i12,
			int argc, const char **argv)
{
	if (argc != 0) {
		d_fprintf(stderr, "usage: %s\n", ctx->whoami);
		return -1;
	}

	d_printf("Minimum password length: %d\n", i1->min_length_password);
	d_printf("Password history length: %d\n", i1->password_history);

	d_printf("Minimum password age: ");
	if (!nt_time_is_zero(&i1->min_passwordage)) {
		time_t t = nt_time_to_unix_abs(&i1->min_passwordage);
		d_printf("%d seconds\n", (int)t);
	} else {
		d_printf("not set\n");
	}

	d_printf("Maximum password age: ");
	if (nt_time_is_set(&i1->expire)) {
		time_t t = nt_time_to_unix_abs(&i1->expire);
		d_printf("%d seconds\n", (int)t);
	} else {
		d_printf("not set\n");
	}

	d_printf("Bad logon attempts: %d\n", i12->bad_attempt_lockout);

	if (i12->bad_attempt_lockout != 0) {

		d_printf("Account lockout duration: ");
		if (nt_time_is_set(&i12->duration)) {
			time_t t = nt_time_to_unix_abs(&i12->duration);
			d_printf("%d seconds\n", (int)t);
		} else {
			d_printf("not set\n");
		}

		d_printf("Bad password count reset after: ");
		if (nt_time_is_set(&i12->reset_count)) {
			time_t t = nt_time_to_unix_abs(&i12->reset_count);
			d_printf("%d seconds\n", (int)t);
		} else {
			d_printf("not set\n");
		}
	}

	d_printf("Disconnect users when logon hours expire: %s\n",
		 nt_time_is_zero(&i3->logout) ? "yes" : "no");

	d_printf("User must logon to change password: %s\n",
		 (i1->password_properties & 0x2) ? "yes" : "no");
	
	return 0;		/* Don't save */
}

static NTSTATUS rpc_sh_acct_pol_show(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx,
				     struct rpc_pipe_client *pipe_hnd,
				     int argc, const char **argv) {
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_show);
}

static int account_set_badpw(TALLOC_CTX *mem_ctx, struct rpc_sh_ctx *ctx,
			     SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
			     SAM_UNK_INFO_12 *i12,
			     int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	i12->bad_attempt_lockout = atoi(argv[0]);
	d_printf("Setting bad password count to %d\n",
		 i12->bad_attempt_lockout);

	return 12;
}

static NTSTATUS rpc_sh_acct_set_badpw(TALLOC_CTX *mem_ctx,
				      struct rpc_sh_ctx *ctx,
				      struct rpc_pipe_client *pipe_hnd,
				      int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_badpw);
}

static int account_set_lockduration(TALLOC_CTX *mem_ctx,
				    struct rpc_sh_ctx *ctx,
				    SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				    SAM_UNK_INFO_12 *i12,
				    int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	unix_to_nt_time_abs(&i12->duration, atoi(argv[0]));
	d_printf("Setting lockout duration to %d seconds\n",
		 (int)nt_time_to_unix_abs(&i12->duration));

	return 12;
}

static NTSTATUS rpc_sh_acct_set_lockduration(TALLOC_CTX *mem_ctx,
					     struct rpc_sh_ctx *ctx,
					     struct rpc_pipe_client *pipe_hnd,
					     int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_lockduration);
}

static int account_set_resetduration(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx,
				     SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				     SAM_UNK_INFO_12 *i12,
				     int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	unix_to_nt_time_abs(&i12->reset_count, atoi(argv[0]));
	d_printf("Setting bad password reset duration to %d seconds\n",
		 (int)nt_time_to_unix_abs(&i12->reset_count));

	return 12;
}

static NTSTATUS rpc_sh_acct_set_resetduration(TALLOC_CTX *mem_ctx,
					      struct rpc_sh_ctx *ctx,
					      struct rpc_pipe_client *pipe_hnd,
					      int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_resetduration);
}

static int account_set_minpwage(TALLOC_CTX *mem_ctx,
				struct rpc_sh_ctx *ctx,
				SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				SAM_UNK_INFO_12 *i12,
				int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	unix_to_nt_time_abs(&i1->min_passwordage, atoi(argv[0]));
	d_printf("Setting minimum password age to %d seconds\n",
		 (int)nt_time_to_unix_abs(&i1->min_passwordage));

	return 1;
}

static NTSTATUS rpc_sh_acct_set_minpwage(TALLOC_CTX *mem_ctx,
					 struct rpc_sh_ctx *ctx,
					 struct rpc_pipe_client *pipe_hnd,
					 int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_minpwage);
}

static int account_set_maxpwage(TALLOC_CTX *mem_ctx,
				struct rpc_sh_ctx *ctx,
				SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				SAM_UNK_INFO_12 *i12,
				int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	unix_to_nt_time_abs(&i1->expire, atoi(argv[0]));
	d_printf("Setting maximum password age to %d seconds\n",
		 (int)nt_time_to_unix_abs(&i1->expire));

	return 1;
}

static NTSTATUS rpc_sh_acct_set_maxpwage(TALLOC_CTX *mem_ctx,
					 struct rpc_sh_ctx *ctx,
					 struct rpc_pipe_client *pipe_hnd,
					 int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_maxpwage);
}

static int account_set_minpwlen(TALLOC_CTX *mem_ctx,
				struct rpc_sh_ctx *ctx,
				SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				SAM_UNK_INFO_12 *i12,
				int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	i1->min_length_password = atoi(argv[0]);
	d_printf("Setting minimum password length to %d\n",
		 i1->min_length_password);

	return 1;
}

static NTSTATUS rpc_sh_acct_set_minpwlen(TALLOC_CTX *mem_ctx,
					 struct rpc_sh_ctx *ctx,
					 struct rpc_pipe_client *pipe_hnd,
					 int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_minpwlen);
}

static int account_set_pwhistlen(TALLOC_CTX *mem_ctx,
				 struct rpc_sh_ctx *ctx,
				 SAM_UNK_INFO_1 *i1, SAM_UNK_INFO_3 *i3,
				 SAM_UNK_INFO_12 *i12,
				 int argc, const char **argv)
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: %s <count>\n", ctx->whoami);
		return -1;
	}

	i1->password_history = atoi(argv[0]);
	d_printf("Setting password history length to %d\n",
		 i1->password_history);

	return 1;
}

static NTSTATUS rpc_sh_acct_set_pwhistlen(TALLOC_CTX *mem_ctx,
					  struct rpc_sh_ctx *ctx,
					  struct rpc_pipe_client *pipe_hnd,
					  int argc, const char **argv)
{
	return rpc_sh_acct_do(mem_ctx, ctx, pipe_hnd, argc, argv,
			      account_set_pwhistlen);
}

struct rpc_sh_cmd *net_rpc_acct_cmds(TALLOC_CTX *mem_ctx,
				     struct rpc_sh_ctx *ctx)
{
	static struct rpc_sh_cmd cmds[9] = {
		{ "show", NULL, PI_SAMR, rpc_sh_acct_pol_show,
		  "Show current account policy settings" },
		{ "badpw", NULL, PI_SAMR, rpc_sh_acct_set_badpw,
		  "Set bad password count before lockout" },
		{ "lockduration", NULL, PI_SAMR, rpc_sh_acct_set_lockduration,
		  "Set account lockout duration" },
		{ "resetduration", NULL, PI_SAMR,
		  rpc_sh_acct_set_resetduration,
		  "Set bad password count reset duration" },
		{ "minpwage", NULL, PI_SAMR, rpc_sh_acct_set_minpwage,
		  "Set minimum password age" },
		{ "maxpwage", NULL, PI_SAMR, rpc_sh_acct_set_maxpwage,
		  "Set maximum password age" },
		{ "minpwlen", NULL, PI_SAMR, rpc_sh_acct_set_minpwlen,
		  "Set minimum password length" },
		{ "pwhistlen", NULL, PI_SAMR, rpc_sh_acct_set_pwhistlen,
		  "Set the password history length" },
		{ NULL, NULL, 0, NULL, NULL }
	};

	return cmds;
}
