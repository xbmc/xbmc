/* 
   Unix SMB/CIFS implementation.

   Winbind daemon - miscellaneous other functions

   Copyright (C) Tim Potter      2000
   Copyright (C) Andrew Bartlett 2002
   
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
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/* Check the machine account password is valid */

void winbindd_check_machine_acct(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: check machine account\n",
		  (unsigned long)state->pid));

	sendto_domain(state, find_our_domain());
}

enum winbindd_result winbindd_dual_check_machine_acct(struct winbindd_domain *domain,
						      struct winbindd_cli_state *state)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
        int num_retries = 0;
	struct winbindd_domain *contact_domain;

	DEBUG(3, ("[%5lu]: check machine account\n", (unsigned long)state->pid));

	/* Get trust account password */

 again:

	contact_domain = find_our_domain();
	
        /* This call does a cli_nt_setup_creds() which implicitly checks
           the trust account password. */

	invalidate_cm_connection(&contact_domain->conn);

	{
		struct rpc_pipe_client *netlogon_pipe;
		result = cm_connect_netlogon(contact_domain, &netlogon_pipe);
	}

        if (!NT_STATUS_IS_OK(result)) {
                DEBUG(3, ("could not open handle to NETLOGON pipe\n"));
                goto done;
        }

        /* There is a race condition between fetching the trust account
           password and the periodic machine password change.  So it's 
	   possible that the trust account password has been changed on us.  
	   We are returned NT_STATUS_ACCESS_DENIED if this happens. */

#define MAX_RETRIES 8

        if ((num_retries < MAX_RETRIES) && 
            NT_STATUS_V(result) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED)) {
                num_retries++;
                goto again;
        }

	/* Pass back result code - zero for success, other values for
	   specific failures. */

	DEBUG(3, ("secret is %s\n", NT_STATUS_IS_OK(result) ?  
                  "good" : "bad"));

 done:
	state->response.data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(state->response.data.auth.nt_status_string, nt_errstr(result));
	fstrcpy(state->response.data.auth.error_string, nt_errstr(result));
	state->response.data.auth.pam_error = nt_status_to_pam(result);

	DEBUG(NT_STATUS_IS_OK(result) ? 5 : 2, ("Checking the trust account password returned %s\n", 
						state->response.data.auth.nt_status_string));

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}

void winbindd_list_trusted_domains(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: list trusted domains\n",
		  (unsigned long)state->pid));

	sendto_domain(state, find_our_domain());
}

enum winbindd_result winbindd_dual_list_trusted_domains(struct winbindd_domain *domain,
							struct winbindd_cli_state *state)
{
	uint32 i, num_domains;
	char **names, **alt_names;
	DOM_SID *sids;
	int extra_data_len = 0;
	char *extra_data;
	NTSTATUS result;
	BOOL have_own_domain = False;

	DEBUG(3, ("[%5lu]: list trusted domains\n",
		  (unsigned long)state->pid));

	result = domain->methods->trusted_domains(domain, state->mem_ctx,
						  &num_domains, &names,
						  &alt_names, &sids);

	extra_data = talloc_strdup(state->mem_ctx, "");

	if (num_domains > 0)
		extra_data = talloc_asprintf(state->mem_ctx, "%s\\%s\\%s",
					     names[0],
					     alt_names[0] ? alt_names[0] : names[0],
					     sid_string_static(&sids[0]));

	for (i=1; i<num_domains; i++)
		extra_data = talloc_asprintf(state->mem_ctx, "%s\n%s\\%s\\%s",
					     extra_data,
					     names[i],
					     alt_names[i] ? alt_names[i] : names[i],
					     sid_string_static(&sids[i]));
	/* add our primary domain */
	
	for (i=0; i<num_domains; i++) {
		if (strequal(names[i], domain->name)) {
			have_own_domain = True;
			break;
		}
	}

	if (state->request.data.list_all_domains && !have_own_domain) {
		extra_data = talloc_asprintf(state->mem_ctx, "%s\n%s\\%s\\%s",
					     extra_data,
					     domain->name,
					     domain->alt_name ? domain->alt_name : domain->name,
					     sid_string_static(&domain->sid));
	}

	/* This is a bit excessive, but the extra data sooner or later will be
	   talloc'ed */

	extra_data_len = 0;
	if (extra_data != NULL) {
		extra_data_len = strlen(extra_data);
	}

	if (extra_data_len > 0) {
		state->response.extra_data.data = SMB_STRDUP(extra_data);
		state->response.length += extra_data_len+1;
	}

	return WINBINDD_OK;
}

void winbindd_getdcname(struct winbindd_cli_state *state)
{
	state->request.domain_name
		[sizeof(state->request.domain_name)-1] = '\0';

	DEBUG(3, ("[%5lu]: Get DC name for %s\n", (unsigned long)state->pid,
		  state->request.domain_name));

	sendto_domain(state, find_our_domain());
}

enum winbindd_result winbindd_dual_getdcname(struct winbindd_domain *domain,
					     struct winbindd_cli_state *state)
{
	fstring dcname_slash;
	char *p;
	struct rpc_pipe_client *netlogon_pipe;
	NTSTATUS result;
	WERROR werr;
	unsigned int orig_timeout;

	state->request.domain_name
		[sizeof(state->request.domain_name)-1] = '\0';

	DEBUG(3, ("[%5lu]: Get DC name for %s\n", (unsigned long)state->pid,
		  state->request.domain_name));

	result = cm_connect_netlogon(domain, &netlogon_pipe);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(1, ("Can't contact the NETLOGON pipe\n"));
		return WINBINDD_ERROR;
	}

	/* This call can take a long time - allow the server to time out.
	   35 seconds should do it. */

	orig_timeout = cli_set_timeout(netlogon_pipe->cli, 35000);

	werr = rpccli_netlogon_getdcname(netlogon_pipe, state->mem_ctx, domain->dcname,
					   state->request.domain_name,
					   dcname_slash);
	/* And restore our original timeout. */
	cli_set_timeout(netlogon_pipe->cli, orig_timeout);

	if (!W_ERROR_IS_OK(werr)) {
		DEBUG(5, ("Error requesting DCname: %s\n", dos_errstr(werr)));
		return WINBINDD_ERROR;
	}

	p = dcname_slash;
	if (*p == '\\') {
		p+=1;
	}
	if (*p == '\\') {
		p+=1;
	}

	fstrcpy(state->response.data.dc_name, p);
	return WINBINDD_OK;
}

struct sequence_state {
	TALLOC_CTX *mem_ctx;
	struct winbindd_cli_state *cli_state;
	struct winbindd_domain *domain;
	struct winbindd_request *request;
	struct winbindd_response *response;
	char *extra_data;
};

static void sequence_recv(void *private_data, BOOL success);

void winbindd_show_sequence(struct winbindd_cli_state *state)
{
	struct sequence_state *seq;

	/* Ensure null termination */
	state->request.domain_name[sizeof(state->request.domain_name)-1]='\0';

	if (strlen(state->request.domain_name) > 0) {
		struct winbindd_domain *domain;
		domain = find_domain_from_name_noinit(
			state->request.domain_name);
		if (domain == NULL) {
			request_error(state);
			return;
		}
		sendto_domain(state, domain);
		return;
	}

	/* Ask all domains in sequence, collect the results in sequence_recv */

	seq = TALLOC_P(state->mem_ctx, struct sequence_state);
	if (seq == NULL) {
		DEBUG(0, ("talloc failed\n"));
		request_error(state);
		return;
	}

	seq->mem_ctx = state->mem_ctx;
	seq->cli_state = state;
	seq->domain = domain_list();
	if (seq->domain == NULL) {
		DEBUG(0, ("domain list empty\n"));
		request_error(state);
		return;
	}
	seq->request = TALLOC_ZERO_P(state->mem_ctx,
				     struct winbindd_request);
	seq->response = TALLOC_ZERO_P(state->mem_ctx,
				      struct winbindd_response);
	seq->extra_data = talloc_strdup(state->mem_ctx, "");

	if ((seq->request == NULL) || (seq->response == NULL) ||
	    (seq->extra_data == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		request_error(state);
		return;
	}

	seq->request->length = sizeof(*seq->request);
	seq->request->cmd = WINBINDD_SHOW_SEQUENCE;
	fstrcpy(seq->request->domain_name, seq->domain->name);

	async_domain_request(state->mem_ctx, seq->domain,
			     seq->request, seq->response,
			     sequence_recv, seq);
}

static void sequence_recv(void *private_data, BOOL success)
{
	struct sequence_state *state = private_data;
	uint32 seq = DOM_SEQUENCE_NONE;

	if ((success) && (state->response->result == WINBINDD_OK))
		seq = state->response->data.domain_info.sequence_number;

	if (seq == DOM_SEQUENCE_NONE) {
		state->extra_data = talloc_asprintf(state->mem_ctx,
						    "%s%s : DISCONNECTED\n",
						    state->extra_data,
						    state->domain->name);
	} else {
		state->extra_data = talloc_asprintf(state->mem_ctx,
						    "%s%s : %d\n",
						    state->extra_data,
						    state->domain->name, seq);
	}

	state->domain->sequence_number = seq;

	state->domain = state->domain->next;

	if (state->domain == NULL) {
		struct winbindd_cli_state *cli_state = state->cli_state;
		cli_state->response.length =
			sizeof(cli_state->response) +
			strlen(state->extra_data) + 1;
		cli_state->response.extra_data.data =
			SMB_STRDUP(state->extra_data);
		request_ok(cli_state);
		return;
	}

	/* Ask the next domain */
	fstrcpy(state->request->domain_name, state->domain->name);
	async_domain_request(state->mem_ctx, state->domain,
			     state->request, state->response,
			     sequence_recv, state);
}

/* This is the child-only version of --sequence. It only allows for a single
 * domain (ie "our" one) to be displayed. */

enum winbindd_result winbindd_dual_show_sequence(struct winbindd_domain *domain,
						 struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: show sequence\n", (unsigned long)state->pid));

	/* Ensure null termination */
	state->request.domain_name[sizeof(state->request.domain_name)-1]='\0';

	domain->methods->sequence_number(domain, &domain->sequence_number);

	state->response.data.domain_info.sequence_number =
		domain->sequence_number;

	return WINBINDD_OK;
}

struct domain_info_state {
	struct winbindd_domain *domain;
	struct winbindd_cli_state *cli_state;
};

static void domain_info_init_recv(void *private_data, BOOL success);

void winbindd_domain_info(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;

	DEBUG(3, ("[%5lu]: domain_info [%s]\n", (unsigned long)state->pid,
		  state->request.domain_name));

	domain = find_domain_from_name_noinit(state->request.domain_name);

	if (domain == NULL) {
		DEBUG(3, ("Did not find domain [%s]\n",
			  state->request.domain_name));
		request_error(state);
		return;
	}

	if (!domain->initialized) {
		struct domain_info_state *istate;

		istate = TALLOC_P(state->mem_ctx, struct domain_info_state);
		if (istate == NULL) {
			DEBUG(0, ("talloc failed\n"));
			request_error(state);
			return;
		}

		istate->cli_state = state;
		istate->domain = domain;

		init_child_connection(domain, domain_info_init_recv, istate);
				      
		return;
	}

	fstrcpy(state->response.data.domain_info.name,
		domain->name);
	fstrcpy(state->response.data.domain_info.alt_name,
		domain->alt_name);
	fstrcpy(state->response.data.domain_info.sid,
		sid_string_static(&domain->sid));
	
	state->response.data.domain_info.native_mode =
		domain->native_mode;
	state->response.data.domain_info.active_directory =
		domain->active_directory;
	state->response.data.domain_info.primary =
		domain->primary;
	state->response.data.domain_info.sequence_number =
		domain->sequence_number;

	request_ok(state);
}

static void domain_info_init_recv(void *private_data, BOOL success)
{
	struct domain_info_state *istate = private_data;
	struct winbindd_cli_state *state = istate->cli_state;
	struct winbindd_domain *domain = istate->domain;

	DEBUG(10, ("Got back from child init: %d\n", success));

	if ((!success) || (!domain->initialized)) {
		DEBUG(5, ("Could not init child for domain %s\n",
			  domain->name));
		request_error(state);
		return;
	}

	fstrcpy(state->response.data.domain_info.name,
		domain->name);
	fstrcpy(state->response.data.domain_info.alt_name,
		domain->alt_name);
	fstrcpy(state->response.data.domain_info.sid,
		sid_string_static(&domain->sid));
	
	state->response.data.domain_info.native_mode =
		domain->native_mode;
	state->response.data.domain_info.active_directory =
		domain->active_directory;
	state->response.data.domain_info.primary =
		domain->primary;
	state->response.data.domain_info.sequence_number =
		domain->sequence_number;

	request_ok(state);
}

void winbindd_ping(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: ping\n", (unsigned long)state->pid));
	request_ok(state);
}

/* List various tidbits of information */

void winbindd_info(struct winbindd_cli_state *state)
{

	DEBUG(3, ("[%5lu]: request misc info\n", (unsigned long)state->pid));

	state->response.data.info.winbind_separator = *lp_winbind_separator();
	fstrcpy(state->response.data.info.samba_version, SAMBA_VERSION_STRING);
	request_ok(state);
}

/* Tell the client the current interface version */

void winbindd_interface_version(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: request interface version\n",
		  (unsigned long)state->pid));
	
	state->response.data.interface_version = WINBIND_INTERFACE_VERSION;
	request_ok(state);
}

/* What domain are we a member of? */

void winbindd_domain_name(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: request domain name\n", (unsigned long)state->pid));
	
	fstrcpy(state->response.data.domain_name, lp_workgroup());
	request_ok(state);
}

/* What's my name again? */

void winbindd_netbios_name(struct winbindd_cli_state *state)
{
	DEBUG(3, ("[%5lu]: request netbios name\n",
		  (unsigned long)state->pid));
	
	fstrcpy(state->response.data.netbios_name, global_myname());
	request_ok(state);
}

/* Where can I find the privilaged pipe? */

void winbindd_priv_pipe_dir(struct winbindd_cli_state *state)
{

	DEBUG(3, ("[%5lu]: request location of privileged pipe\n",
		  (unsigned long)state->pid));
	
	state->response.extra_data.data = SMB_STRDUP(get_winbind_priv_pipe_dir());
	if (!state->response.extra_data.data) {
		DEBUG(0, ("malloc failed\n"));
		request_error(state);
		return;
	}

	/* must add one to length to copy the 0 for string termination */
	state->response.length +=
		strlen((char *)state->response.extra_data.data) + 1;

	request_ok(state);
}
