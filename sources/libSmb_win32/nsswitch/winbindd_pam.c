/*
   Unix SMB/CIFS implementation.

   Winbind daemon - pam auth funcions

   Copyright (C) Andrew Tridgell 2000
   Copyright (C) Tim Potter 2001
   Copyright (C) Andrew Bartlett 2001-2002
   Copyright (C) Guenther Deschner 2005
   
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

static NTSTATUS append_info3_as_txt(TALLOC_CTX *mem_ctx, 
				    struct winbindd_cli_state *state, 
				    NET_USER_INFO_3 *info3) 
{
	fstring str_sid;

	state->response.data.auth.info3.logon_time = 
		nt_time_to_unix(&(info3->logon_time));
	state->response.data.auth.info3.logoff_time = 
		nt_time_to_unix(&(info3->logoff_time));
	state->response.data.auth.info3.kickoff_time = 
		nt_time_to_unix(&(info3->kickoff_time));
	state->response.data.auth.info3.pass_last_set_time = 
		nt_time_to_unix(&(info3->pass_last_set_time));
	state->response.data.auth.info3.pass_can_change_time = 
		nt_time_to_unix(&(info3->pass_can_change_time));
	state->response.data.auth.info3.pass_must_change_time = 
		nt_time_to_unix(&(info3->pass_must_change_time));

	state->response.data.auth.info3.logon_count = info3->logon_count;
	state->response.data.auth.info3.bad_pw_count = info3->bad_pw_count;

	state->response.data.auth.info3.user_rid = info3->user_rid;
	state->response.data.auth.info3.group_rid = info3->group_rid;
	sid_to_string(str_sid, &(info3->dom_sid.sid));
	fstrcpy(state->response.data.auth.info3.dom_sid, str_sid);

	state->response.data.auth.info3.num_groups = info3->num_groups;
	state->response.data.auth.info3.user_flgs = info3->user_flgs;

	state->response.data.auth.info3.acct_flags = info3->acct_flags;
	state->response.data.auth.info3.num_other_sids = info3->num_other_sids;

	unistr2_to_ascii(state->response.data.auth.info3.user_name, 
		&info3->uni_user_name, -1);
	unistr2_to_ascii(state->response.data.auth.info3.full_name, 
		&info3->uni_full_name, -1);
	unistr2_to_ascii(state->response.data.auth.info3.logon_script, 
		&info3->uni_logon_script, -1);
	unistr2_to_ascii(state->response.data.auth.info3.profile_path, 
		&info3->uni_profile_path, -1);
	unistr2_to_ascii(state->response.data.auth.info3.home_dir, 
		&info3->uni_home_dir, -1);
	unistr2_to_ascii(state->response.data.auth.info3.dir_drive, 
		&info3->uni_dir_drive, -1);

	unistr2_to_ascii(state->response.data.auth.info3.logon_srv, 
		&info3->uni_logon_srv, -1);
	unistr2_to_ascii(state->response.data.auth.info3.logon_dom, 
		&info3->uni_logon_dom, -1);

	return NT_STATUS_OK;
}

static NTSTATUS append_info3_as_ndr(TALLOC_CTX *mem_ctx, 
				    struct winbindd_cli_state *state, 
				    NET_USER_INFO_3 *info3) 
{
	prs_struct ps;
	uint32 size;
	if (!prs_init(&ps, 256 /* Random, non-zero number */, mem_ctx, MARSHALL)) {
		return NT_STATUS_NO_MEMORY;
	}
	if (!net_io_user_info3("", info3, &ps, 1, 3, False)) {
		prs_mem_free(&ps);
		return NT_STATUS_UNSUCCESSFUL;
	}

	size = prs_data_size(&ps);
	SAFE_FREE(state->response.extra_data.data);
	state->response.extra_data.data = SMB_MALLOC(size);
	if (!state->response.extra_data.data) {
		prs_mem_free(&ps);
		return NT_STATUS_NO_MEMORY;
	}
	memset( state->response.extra_data.data, '\0', size );
	prs_copy_all_data_out(state->response.extra_data.data, &ps);
	state->response.length += size;
	prs_mem_free(&ps);
	return NT_STATUS_OK;
}

static NTSTATUS check_info3_in_group(TALLOC_CTX *mem_ctx, 
				     NET_USER_INFO_3 *info3,
				     const char *group_sid) 
{
	DOM_SID require_membership_of_sid;
	DOM_SID *all_sids;
	size_t num_all_sids = (2 + info3->num_groups2 + info3->num_other_sids);
	size_t i, j = 0;

	/* Parse the 'required group' SID */
	
	if (!group_sid || !group_sid[0]) {
		/* NO sid supplied, all users may access */
		return NT_STATUS_OK;
	}
	
	if (!string_to_sid(&require_membership_of_sid, group_sid)) {
		DEBUG(0, ("check_info3_in_group: could not parse %s as a SID!", 
			  group_sid));

		return NT_STATUS_INVALID_PARAMETER;
	}

	all_sids = TALLOC_ARRAY(mem_ctx, DOM_SID, num_all_sids);
	if (!all_sids)
		return NT_STATUS_NO_MEMORY;

	/* and create (by appending rids) the 'domain' sids */
	
	sid_copy(&all_sids[0], &(info3->dom_sid.sid));
	
	if (!sid_append_rid(&all_sids[0], info3->user_rid)) {
		DEBUG(3,("could not append user's primary RID 0x%x\n",
			 info3->user_rid));			
		
		return NT_STATUS_INVALID_PARAMETER;
	}
	j++;

	sid_copy(&all_sids[1], &(info3->dom_sid.sid));
		
	if (!sid_append_rid(&all_sids[1], info3->group_rid)) {
		DEBUG(3,("could not append additional group rid 0x%x\n",
			 info3->group_rid));			
		
		return NT_STATUS_INVALID_PARAMETER;
	}
	j++;	

	for (i = 0; i < info3->num_groups2; i++) {
	
		sid_copy(&all_sids[j], &(info3->dom_sid.sid));
		
		if (!sid_append_rid(&all_sids[j], info3->gids[i].g_rid)) {
			DEBUG(3,("could not append additional group rid 0x%x\n",
				info3->gids[i].g_rid));			
				
			return NT_STATUS_INVALID_PARAMETER;
		}
		j++;
	}

	/* Copy 'other' sids.  We need to do sid filtering here to
 	   prevent possible elevation of privileges.  See:

           http://www.microsoft.com/windows2000/techinfo/administration/security/sidfilter.asp
         */

	for (i = 0; i < info3->num_other_sids; i++) {
		sid_copy(&all_sids[info3->num_groups2 + i + 2],
			 &info3->other_sids[i].sid);
		j++;
	}

	for (i = 0; i < j; i++) {
		fstring sid1, sid2;
		DEBUG(10, ("User has SID: %s\n", 
			   sid_to_string(sid1, &all_sids[i])));
		if (sid_equal(&require_membership_of_sid, &all_sids[i])) {
			DEBUG(10, ("SID %s matches %s - user permitted to authenticate!\n", 
				   sid_to_string(sid1, &require_membership_of_sid), sid_to_string(sid2, &all_sids[i])));
			return NT_STATUS_OK;
		}
	}
	
	/* Do not distinguish this error from a wrong username/pw */

	return NT_STATUS_LOGON_FAILURE;
}

static struct winbindd_domain *find_auth_domain(struct winbindd_cli_state *state, 
						const char *domain_name)
{
	struct winbindd_domain *domain;

	if (IS_DC) {
		domain = find_domain_from_name_noinit(domain_name);
		if (domain == NULL) {
			DEBUG(3, ("Authentication for domain [%s] refused"
				  "as it is not a trusted domain\n", 
				  domain_name));
		}
		return domain;
	}

	if (is_myname(domain_name)) {
		DEBUG(3, ("Authentication for domain %s (local domain "
			  "to this server) not supported at this "
			  "stage\n", domain_name));
		return NULL;
	}

	/* we can auth against trusted domains */
	if (state->request.flags & WBFLAG_PAM_CONTACT_TRUSTDOM) {
		domain = find_domain_from_name_noinit(domain_name);
		if (domain == NULL) {
			DEBUG(3, ("Authentication for domain [%s] skipped " 
				  "as it is not a trusted domain\n", 
				  domain_name));
		} else {
			return domain;
		} 
		}

	return find_our_domain();
}

static void set_auth_errors(struct winbindd_response *resp, NTSTATUS result)
{
	resp->data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(resp->data.auth.nt_status_string, nt_errstr(result));

	/* we might have given a more useful error above */
	if (*resp->data.auth.error_string == '\0') 
		fstrcpy(resp->data.auth.error_string,
			get_friendly_nt_error_msg(result));
	resp->data.auth.pam_error = nt_status_to_pam(result);
}

static NTSTATUS fillup_password_policy(struct winbindd_domain *domain,
				       struct winbindd_cli_state *state)
{
	struct winbindd_methods *methods;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	SAM_UNK_INFO_1 password_policy;

	methods = domain->methods;

	status = methods->password_policy(domain, state->mem_ctx, &password_policy);
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	state->response.data.auth.policy.min_length_password =
		password_policy.min_length_password;
	state->response.data.auth.policy.password_history =
		password_policy.password_history;
	state->response.data.auth.policy.password_properties =
		password_policy.password_properties;
	state->response.data.auth.policy.expire	=
		nt_time_to_unix_abs(&(password_policy.expire));
	state->response.data.auth.policy.min_passwordage = 
		nt_time_to_unix_abs(&(password_policy.min_passwordage));

	return NT_STATUS_OK;
}

static NTSTATUS get_max_bad_attempts_from_lockout_policy(struct winbindd_domain *domain, 
							 TALLOC_CTX *mem_ctx, 
							 uint16 *max_allowed_bad_attempts)
{
	struct winbindd_methods *methods;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	SAM_UNK_INFO_12 lockout_policy;

	*max_allowed_bad_attempts = 0;

	methods = domain->methods;

	status = methods->lockout_policy(domain, mem_ctx, &lockout_policy);
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	*max_allowed_bad_attempts = lockout_policy.bad_attempt_lockout;

	return NT_STATUS_OK;
}

static NTSTATUS get_pwd_properties(struct winbindd_domain *domain, 
				   TALLOC_CTX *mem_ctx, 
				   uint32 *password_properties)
{
	struct winbindd_methods *methods;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	SAM_UNK_INFO_1 password_policy;

	*password_properties = 0;

	methods = domain->methods;

	status = methods->password_policy(domain, mem_ctx, &password_policy);
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}

	*password_properties = password_policy.password_properties;

	return NT_STATUS_OK;
}

static const char *generate_krb5_ccache(TALLOC_CTX *mem_ctx, 
					const char *type,
					uid_t uid,
					BOOL *internal_ccache)
{
	/* accept FILE and WRFILE as krb5_cc_type from the client and then
	 * build the full ccname string based on the user's uid here -
	 * Guenther*/

	const char *gen_cc = NULL;

	*internal_ccache = True;

	if (uid == -1) {
		goto memory_ccache;
	}

	if (!type || type[0] == '\0') {
		goto memory_ccache;
	}

	if (strequal(type, "FILE")) {
		gen_cc = talloc_asprintf(mem_ctx, "FILE:/tmp/krb5cc_%d", uid);
	} else if (strequal(type, "WRFILE")) {
		gen_cc = talloc_asprintf(mem_ctx, "WRFILE:/tmp/krb5cc_%d", uid);
	} else {
		DEBUG(10,("we don't allow to set a %s type ccache\n", type));
		goto memory_ccache;
	}

	*internal_ccache = False;
	goto done;

  memory_ccache:
  	gen_cc = talloc_strdup(mem_ctx, "MEMORY:winbindd_pam_ccache");

  done:
  	if (gen_cc == NULL) {
		DEBUG(0,("out of memory\n"));
		return NULL;
	}

	DEBUG(10,("using ccache: %s %s\n", gen_cc, *internal_ccache ? "(internal)":""));

	return gen_cc;
}

static uid_t get_uid_from_state(struct winbindd_cli_state *state)
{
	uid_t uid = -1;

	uid = state->request.data.auth.uid;

	if (uid < 0) {
		DEBUG(1,("invalid uid: '%d'\n", uid));
		return -1;
	}
	return uid;
}

static void setup_return_cc_name(struct winbindd_cli_state *state, const char *cc)
{
	const char *type = state->request.data.auth.krb5_cc_type;

	state->response.data.auth.krb5ccname[0] = '\0';

	if (type[0] == '\0') {
		return;
	}

	if (!strequal(type, "FILE") &&
	    !strequal(type, "WRFILE")) {
	    	DEBUG(10,("won't return krbccname for a %s type ccache\n", 
			type));
		return;
	}
	
	fstrcpy(state->response.data.auth.krb5ccname, cc);
}

/**********************************************************************
 Authenticate a user with a clear text password using Kerberos and fill up
 ccache if required
 **********************************************************************/
static NTSTATUS winbindd_raw_kerberos_login(struct winbindd_domain *domain,
					    struct winbindd_cli_state *state,
					    NET_USER_INFO_3 **info3)
{
#ifdef HAVE_KRB5
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	krb5_error_code krb5_ret;
	DATA_BLOB tkt, session_key_krb5;
	DATA_BLOB ap_rep, session_key;
	PAC_DATA *pac_data = NULL;
	PAC_LOGON_INFO *logon_info = NULL;
	char *client_princ = NULL;
	char *client_princ_out = NULL;
	char *local_service = NULL;
	const char *cc = NULL;
	const char *principal_s = NULL;
	const char *service = NULL;
	char *realm = NULL;
	fstring name_domain, name_user;
	time_t ticket_lifetime = 0;
	time_t renewal_until = 0;
	uid_t uid = -1;
	ADS_STRUCT *ads;
	time_t time_offset = 0;
	BOOL internal_ccache = True;

	ZERO_STRUCT(session_key);
	ZERO_STRUCT(session_key_krb5);
	ZERO_STRUCT(tkt);
	ZERO_STRUCT(ap_rep);

	ZERO_STRUCTP(info3);

	*info3 = NULL;
	
	/* 1st step: 
	 * prepare a krb5_cc_cache string for the user */

	uid = get_uid_from_state(state);
	if (uid == -1) {
		DEBUG(0,("no valid uid\n"));
	}

	cc = generate_krb5_ccache(state->mem_ctx,
				  state->request.data.auth.krb5_cc_type,
				  state->request.data.auth.uid, 
				  &internal_ccache);
	if (cc == NULL) {
		return NT_STATUS_NO_MEMORY;
	}


	/* 2nd step: 
	 * get kerberos properties */
	
	if (domain->private_data) {
		ads = (ADS_STRUCT *)domain->private_data;
		time_offset = ads->auth.time_offset; 
	}


	/* 3rd step: 
	 * do kerberos auth and setup ccache as the user */

	parse_domain_user(state->request.data.auth.user, name_domain, name_user);

	realm = domain->alt_name;
	strupper_m(realm);
	
	principal_s = talloc_asprintf(state->mem_ctx, "%s@%s", name_user, realm); 
	if (principal_s == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	service = talloc_asprintf(state->mem_ctx, "%s/%s@%s", KRB5_TGS_NAME, realm, realm);
	if (service == NULL) {
		return NT_STATUS_NO_MEMORY;
	}

	/* if this is a user ccache, we need to act as the user to let the krb5
	 * library handle the chown, etc. */

	/************************ NON-ROOT **********************/

	if (!internal_ccache) {

		set_effective_uid(uid);
		DEBUG(10,("winbindd_raw_kerberos_login: uid is %d\n", uid));
	}

	krb5_ret = kerberos_kinit_password_ext(principal_s, 
					       state->request.data.auth.pass, 
					       time_offset, 
					       &ticket_lifetime,
					       &renewal_until,
					       cc, 
					       True,
					       True,
					       WINBINDD_PAM_AUTH_KRB5_RENEW_TIME);

	if (krb5_ret) {
		DEBUG(1,("winbindd_raw_kerberos_login: kinit failed for '%s' with: %s (%d)\n", 
			principal_s, error_message(krb5_ret), krb5_ret));
		result = krb5_to_nt_status(krb5_ret);
		goto failed;
	}

	/* does http_timestring use heimdals libroken strftime?? - Guenther */
	DEBUG(10,("got TGT for %s in %s (valid until: %s (%d), renewable till: %s (%d))\n", 
		principal_s, cc, 
		http_timestring(ticket_lifetime), (int)ticket_lifetime, 
		http_timestring(renewal_until), (int)renewal_until));

	client_princ = talloc_strdup(state->mem_ctx, global_myname());
	if (client_princ == NULL) {
		result = NT_STATUS_NO_MEMORY;
		goto failed;
	}
	strlower_m(client_princ);

	local_service = talloc_asprintf(state->mem_ctx, "%s$@%s", client_princ, lp_realm());
	if (local_service == NULL) {
		DEBUG(0,("winbindd_raw_kerberos_login: out of memory\n"));
		result = NT_STATUS_NO_MEMORY;
		goto failed;
	}

	krb5_ret = cli_krb5_get_ticket(local_service, 
				       time_offset, 
				       &tkt, 
				       &session_key_krb5, 
				       0, 
				       cc);
	if (krb5_ret) {
		DEBUG(1,("winbindd_raw_kerberos_login: failed to get ticket for %s: %s\n", 
			local_service, error_message(krb5_ret)));
		result = krb5_to_nt_status(krb5_ret);
		goto failed;
	}

	if (!internal_ccache) {
		gain_root_privilege();
	}

	/************************ NON-ROOT **********************/

	result = ads_verify_ticket(state->mem_ctx, 
				   lp_realm(), 
				   time_offset,
				   &tkt, 
				   &client_princ_out, 
				   &pac_data, 
				   &ap_rep, 
				   &session_key);	
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(0,("winbindd_raw_kerberos_login: ads_verify_ticket failed: %s\n", 
			nt_errstr(result)));
		goto failed;
	}

	if (!pac_data) {
		DEBUG(3,("winbindd_raw_kerberos_login: no pac data\n"));
		result = NT_STATUS_INVALID_PARAMETER;
		goto failed;
	}
			
	logon_info = get_logon_info_from_pac(pac_data);
	if (logon_info == NULL) {
		DEBUG(1,("winbindd_raw_kerberos_login: no logon info\n"));
		result = NT_STATUS_INVALID_PARAMETER;
		goto failed;
	}

	DEBUG(10,("winbindd_raw_kerberos_login: winbindd validated ticket of %s\n", 
		local_service));


	/* last step: 
	 * put results together */

	*info3 = &logon_info->info3;

	/* if we had a user's ccache then return that string for the pam
	 * environment */

	if (!internal_ccache) {
		
		setup_return_cc_name(state, cc);

		result = add_ccache_to_list(principal_s,
					    cc,
					    service,
					    state->request.data.auth.user,
					    NULL,
					    state->request.data.auth.pass,
					    uid,
					    time(NULL),
					    ticket_lifetime,
					    renewal_until, 
					    lp_winbind_refresh_tickets());

		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(10,("winbindd_raw_kerberos_login: failed to add ccache to list: %s\n", 
				nt_errstr(result)));
		}
	}

	result = NT_STATUS_OK;

	goto done;

failed:

	/* we could have created a new credential cache with a valid tgt in it
	 * but we werent able to get or verify the service ticket for this
	 * local host and therefor didn't get the PAC, we need to remove that
	 * cache entirely now */

	krb5_ret = ads_kdestroy(cc);
	if (krb5_ret) {
		DEBUG(3,("winbindd_raw_kerberos_login: "
			 "could not destroy krb5 credential cache: "
			 "%s\n", error_message(krb5_ret)));
	}

	if (!NT_STATUS_IS_OK(remove_ccache_by_ccname(cc))) {
		DEBUG(3,("winbindd_raw_kerberos_login: "
			  "could not remove ccache\n"));
	}

done:
	data_blob_free(&session_key);
	data_blob_free(&session_key_krb5);
	data_blob_free(&ap_rep);
	data_blob_free(&tkt);

	SAFE_FREE(client_princ_out);

	if (!internal_ccache) {
		gain_root_privilege();
	}

	return result;
#else 
	return NT_STATUS_NOT_SUPPORTED;
#endif /* HAVE_KRB5 */
}

void winbindd_pam_auth(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;
	fstring name_domain, name_user;

	/* Ensure null termination */
	state->request.data.auth.user
		[sizeof(state->request.data.auth.user)-1]='\0';

	/* Ensure null termination */
	state->request.data.auth.pass
		[sizeof(state->request.data.auth.pass)-1]='\0';

	DEBUG(3, ("[%5lu]: pam auth %s\n", (unsigned long)state->pid,
		  state->request.data.auth.user));

	/* Parse domain and username */
	
	if (!parse_domain_user(state->request.data.auth.user,
			       name_domain, name_user)) {
		set_auth_errors(&state->response, NT_STATUS_NO_SUCH_USER);
		DEBUG(5, ("Plain text authentication for %s returned %s "
			  "(PAM: %d)\n",
			  state->request.data.auth.user, 
			  state->response.data.auth.nt_status_string,
			  state->response.data.auth.pam_error));
		request_error(state);
		return;
	}

	domain = find_auth_domain(state, name_domain);

	if (domain == NULL) {
		set_auth_errors(&state->response, NT_STATUS_NO_SUCH_USER);
		DEBUG(5, ("Plain text authentication for %s returned %s "
			  "(PAM: %d)\n",
			  state->request.data.auth.user, 
			  state->response.data.auth.nt_status_string,
			  state->response.data.auth.pam_error));
		request_error(state);
		return;
	}

	sendto_domain(state, domain);
}

NTSTATUS winbindd_dual_pam_auth_cached(struct winbindd_domain *domain,
				       struct winbindd_cli_state *state,
				       NET_USER_INFO_3 **info3)
{
	NTSTATUS result = NT_STATUS_LOGON_FAILURE;
	uint16 max_allowed_bad_attempts; 
	fstring name_domain, name_user;
	DOM_SID sid;
	enum SID_NAME_USE type;
	uchar new_nt_pass[NT_HASH_LEN];
	const uint8 *cached_nt_pass;
	NET_USER_INFO_3 *my_info3;
	time_t kickoff_time, must_change_time;

	*info3 = NULL;

	ZERO_STRUCTP(info3);

	DEBUG(10,("winbindd_dual_pam_auth_cached\n"));

	/* Parse domain and username */
	
	parse_domain_user(state->request.data.auth.user, name_domain, name_user);


	if (!lookup_cached_name(state->mem_ctx,
	                        name_domain,
				name_user,
				&sid,
				&type)) {
		DEBUG(10,("winbindd_dual_pam_auth_cached: no such user in the cache\n"));
		return NT_STATUS_NO_SUCH_USER;
	}

	if (type != SID_NAME_USER) {
		DEBUG(10,("winbindd_dual_pam_auth_cached: not a user (%s)\n", sid_type_lookup(type)));
		return NT_STATUS_LOGON_FAILURE;
	}

	result = winbindd_get_creds(domain, 
				    state->mem_ctx, 
				    &sid, 
				    &my_info3, 
				    &cached_nt_pass);
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(10,("winbindd_dual_pam_auth_cached: failed to get creds: %s\n", nt_errstr(result)));
		return result;
	}

	*info3 = my_info3;

	E_md4hash(state->request.data.auth.pass, new_nt_pass);

#if DEBUG_PASSWORD
	dump_data(100, (const char *)new_nt_pass, NT_HASH_LEN);
	dump_data(100, (const char *)cached_nt_pass, NT_HASH_LEN);
#endif

	if (!memcmp(cached_nt_pass, new_nt_pass, NT_HASH_LEN)) {

		/* User *DOES* know the password, update logon_time and reset
		 * bad_pw_count */
	
		my_info3->user_flgs |= LOGON_CACHED_ACCOUNT;
	
		if (my_info3->acct_flags & ACB_AUTOLOCK) {
			return NT_STATUS_ACCOUNT_LOCKED_OUT;
		}
	
		if (my_info3->acct_flags & ACB_DISABLED) {
			return NT_STATUS_ACCOUNT_DISABLED;
		}
	
		if (my_info3->acct_flags & ACB_WSTRUST) {
			return NT_STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT;
		}
	
		if (my_info3->acct_flags & ACB_SVRTRUST) {
			return NT_STATUS_NOLOGON_SERVER_TRUST_ACCOUNT;
		}
	
		if (my_info3->acct_flags & ACB_DOMTRUST) {
			return NT_STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT;
		}

		/* The info3 acct_flags in NT4's samlogon reply don't have
		 * ACB_NORMAL set. */
#if 0
		if (!(my_info3->acct_flags & ACB_NORMAL)) {
			DEBUG(10,("winbindd_dual_pam_auth_cached: whats wrong with that one?: 0x%08x\n", 
				my_info3->acct_flags));
			return NT_STATUS_LOGON_FAILURE;
		}
#endif
		kickoff_time = nt_time_to_unix(&my_info3->kickoff_time);
		if (kickoff_time != 0 && time(NULL) > kickoff_time) {
			return NT_STATUS_ACCOUNT_EXPIRED;
		}

		must_change_time = nt_time_to_unix(&my_info3->pass_must_change_time);
		if (must_change_time != 0 && must_change_time < time(NULL)) {
			return NT_STATUS_PASSWORD_EXPIRED;
		}
	
		/* FIXME: we possibly should handle logon hours as well (does xp when
		 * offline?) see auth/auth_sam.c:sam_account_ok for details */

		unix_to_nt_time(&my_info3->logon_time, time(NULL));
		my_info3->bad_pw_count = 0;

		result = winbindd_update_creds_by_info3(domain,
							state->mem_ctx,
							state->request.data.auth.user,
							state->request.data.auth.pass,
							my_info3);
		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(1,("failed to update creds: %s\n", nt_errstr(result)));
			return result;
		}

		return NT_STATUS_OK;

	}

	/* User does *NOT* know the correct password, modify info3 accordingly */

	/* failure of this is not critical */
	result = get_max_bad_attempts_from_lockout_policy(domain, state->mem_ctx, &max_allowed_bad_attempts);
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(10,("winbindd_dual_pam_auth_cached: failed to get max_allowed_bad_attempts. "
			  "Won't be able to honour account lockout policies\n"));
	}

	/* increase counter */
	my_info3->bad_pw_count++;

	if (max_allowed_bad_attempts == 0) {
		goto failed;
	}

	/* lockout user */
	if (my_info3->bad_pw_count >= max_allowed_bad_attempts) {

		uint32 password_properties;

		result = get_pwd_properties(domain, state->mem_ctx, &password_properties);
		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(10,("winbindd_dual_pam_auth_cached: failed to get password properties.\n"));
		}

		if ((my_info3->user_rid != DOMAIN_USER_RID_ADMIN) || 
		    (password_properties & DOMAIN_LOCKOUT_ADMINS)) {
			my_info3->acct_flags |= ACB_AUTOLOCK;
		}
	}

failed:
	result = winbindd_update_creds_by_info3(domain,
						state->mem_ctx,
						state->request.data.auth.user,
						NULL,
						my_info3);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(0,("winbindd_dual_pam_auth_cached: failed to update creds %s\n", 
			nt_errstr(result)));
	}

	return NT_STATUS_LOGON_FAILURE;
}

NTSTATUS winbindd_dual_pam_auth_kerberos(struct winbindd_domain *domain,
					 struct winbindd_cli_state *state, 
					 NET_USER_INFO_3 **info3)
{
	struct winbindd_domain *contact_domain;
	fstring name_domain, name_user;
	NTSTATUS result;

	DEBUG(10,("winbindd_dual_pam_auth_kerberos\n"));
	
	/* Parse domain and username */
	
	parse_domain_user(state->request.data.auth.user, name_domain, name_user);

	/* what domain should we contact? */
	
	if ( IS_DC ) {
		if (!(contact_domain = find_domain_from_name(name_domain))) {
			DEBUG(3, ("Authentication for domain for [%s] -> [%s]\\[%s] failed as %s is not a trusted domain\n", 
				  state->request.data.auth.user, name_domain, name_user, name_domain)); 
			result = NT_STATUS_NO_SUCH_USER;
			goto done;
		}
		
	} else {
		if (is_myname(name_domain)) {
			DEBUG(3, ("Authentication for domain %s (local domain to this server) not supported at this stage\n", name_domain));
			result =  NT_STATUS_NO_SUCH_USER;
			goto done;
		}
		
		contact_domain = find_domain_from_name(name_domain);
		if (contact_domain == NULL) {
			DEBUG(3, ("Authentication for domain for [%s] -> [%s]\\[%s] failed as %s is not a trusted domain\n", 
				  state->request.data.auth.user, name_domain, name_user, name_domain)); 

			contact_domain = find_our_domain();
		}
	}

	if (contact_domain->initialized && 
	    contact_domain->active_directory) {
	    	goto try_login;
	}

	if (!contact_domain->initialized) {
		set_dc_type_and_flags(contact_domain);
	}

	if (!contact_domain->active_directory) {
		DEBUG(3,("krb5 auth requested but domain is not Active Directory\n"));
		return NT_STATUS_INVALID_LOGON_TYPE;
	}
try_login:
	result = winbindd_raw_kerberos_login(contact_domain, state, info3);
done:
	return result;
}

NTSTATUS winbindd_dual_pam_auth_samlogon(struct winbindd_domain *domain,
					 struct winbindd_cli_state *state,
					 NET_USER_INFO_3 **info3)
{

	struct rpc_pipe_client *netlogon_pipe;
	uchar chal[8];
	DATA_BLOB lm_resp;
	DATA_BLOB nt_resp;
	int attempts = 0;
	unsigned char local_lm_response[24];
	unsigned char local_nt_response[24];
	struct winbindd_domain *contact_domain;
	fstring name_domain, name_user;
	BOOL retry;
	NTSTATUS result;
	NET_USER_INFO_3 *my_info3;

	ZERO_STRUCTP(info3);

	*info3 = NULL;

	my_info3 = TALLOC_ZERO_P(state->mem_ctx, NET_USER_INFO_3);
	if (my_info3 == NULL) {
		return NT_STATUS_NO_MEMORY;
	}


	DEBUG(10,("winbindd_dual_pam_auth_samlogon\n"));
	
	/* Parse domain and username */
	
	parse_domain_user(state->request.data.auth.user, name_domain, name_user);

	/* do password magic */
	

	generate_random_buffer(chal, 8);
	if (lp_client_ntlmv2_auth()) {
		DATA_BLOB server_chal;
		DATA_BLOB names_blob;
		DATA_BLOB nt_response;
		DATA_BLOB lm_response;
		server_chal = data_blob_talloc(state->mem_ctx, chal, 8); 
		
		/* note that the 'workgroup' here is a best guess - we don't know
		   the server's domain at this point.  The 'server name' is also
		   dodgy... 
		*/
		names_blob = NTLMv2_generate_names_blob(global_myname(), lp_workgroup());
		
		if (!SMBNTLMv2encrypt(name_user, name_domain, 
				      state->request.data.auth.pass, 
				      &server_chal, 
				      &names_blob,
				      &lm_response, &nt_response, NULL)) {
			data_blob_free(&names_blob);
			data_blob_free(&server_chal);
			DEBUG(0, ("winbindd_pam_auth: SMBNTLMv2encrypt() failed!\n"));
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}
		data_blob_free(&names_blob);
		data_blob_free(&server_chal);
		lm_resp = data_blob_talloc(state->mem_ctx, lm_response.data,
					   lm_response.length);
		nt_resp = data_blob_talloc(state->mem_ctx, nt_response.data,
					   nt_response.length);
		data_blob_free(&lm_response);
		data_blob_free(&nt_response);

	} else {
		if (lp_client_lanman_auth() 
		    && SMBencrypt(state->request.data.auth.pass, 
				  chal, 
				  local_lm_response)) {
			lm_resp = data_blob_talloc(state->mem_ctx, 
						   local_lm_response, 
						   sizeof(local_lm_response));
		} else {
			lm_resp = data_blob(NULL, 0);
		}
		SMBNTencrypt(state->request.data.auth.pass, 
			     chal,
			     local_nt_response);

		nt_resp = data_blob_talloc(state->mem_ctx, 
					   local_nt_response, 
					   sizeof(local_nt_response));
	}
	
	/* what domain should we contact? */
	
	if ( IS_DC ) {
		if (!(contact_domain = find_domain_from_name(name_domain))) {
			DEBUG(3, ("Authentication for domain for [%s] -> [%s]\\[%s] failed as %s is not a trusted domain\n", 
				  state->request.data.auth.user, name_domain, name_user, name_domain)); 
			result = NT_STATUS_NO_SUCH_USER;
			goto done;
		}
		
	} else {
		if (is_myname(name_domain)) {
			DEBUG(3, ("Authentication for domain %s (local domain to this server) not supported at this stage\n", name_domain));
			result =  NT_STATUS_NO_SUCH_USER;
			goto done;
		}

		contact_domain = find_our_domain();
	}

	/* check authentication loop */

	do {

		ZERO_STRUCTP(my_info3);
		retry = False;

		result = cm_connect_netlogon(contact_domain, &netlogon_pipe);

		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(3, ("could not open handle to NETLOGON pipe\n"));
			goto done;
		}

		result = rpccli_netlogon_sam_network_logon(netlogon_pipe,
							   state->mem_ctx,
							   0,
							   contact_domain->dcname, /* server name */
							   name_user,              /* user name */
							   name_domain,            /* target domain */
							   global_myname(),        /* workstation */
							   chal,
							   lm_resp,
							   nt_resp,
							   my_info3);
		attempts += 1;

		/* We have to try a second time as cm_connect_netlogon
		   might not yet have noticed that the DC has killed
		   our connection. */

		if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL)) {
			retry = True;
			continue;
		}
		
		/* if we get access denied, a possible cause was that we had
		   and open connection to the DC, but someone changed our
		   machine account password out from underneath us using 'net
		   rpc changetrustpw' */
		   
		if ( NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) ) {
			DEBUG(3,("winbindd_pam_auth: sam_logon returned "
				 "ACCESS_DENIED.  Maybe the trust account "
				"password was changed and we didn't know it. "
				 "Killing connections to domain %s\n",
				name_domain));
			invalidate_cm_connection(&contact_domain->conn);
			retry = True;
		} 
		
	} while ( (attempts < 2) && retry );

	*info3 = my_info3;
done:
	return result;
}

enum winbindd_result winbindd_dual_pam_auth(struct winbindd_domain *domain,
					    struct winbindd_cli_state *state) 
{
	NTSTATUS result = NT_STATUS_LOGON_FAILURE;
	fstring name_domain, name_user;
	NET_USER_INFO_3 *info3 = NULL;
	
	/* Ensure null termination */
	state->request.data.auth.user[sizeof(state->request.data.auth.user)-1]='\0';

	/* Ensure null termination */
	state->request.data.auth.pass[sizeof(state->request.data.auth.pass)-1]='\0';

	DEBUG(3, ("[%5lu]: dual pam auth %s\n", (unsigned long)state->pid,
		  state->request.data.auth.user));

	/* Parse domain and username */
	
	parse_domain_user(state->request.data.auth.user, name_domain, name_user);

	DEBUG(10,("winbindd_dual_pam_auth: domain: %s last was %s\n", domain->name, domain->online ? "online":"offline"));

	/* Check for Kerberos authentication */
	if (domain->online && (state->request.flags & WBFLAG_PAM_KRB5)) {
	
		result = winbindd_dual_pam_auth_kerberos(domain, state, &info3);

		if (NT_STATUS_IS_OK(result)) {
			DEBUG(10,("winbindd_dual_pam_auth_kerberos succeeded\n"));
			goto process_result;
		} else {
			DEBUG(10,("winbindd_dual_pam_auth_kerberos failed: %s\n", nt_errstr(result)));
		}

		if (NT_STATUS_EQUAL(result, NT_STATUS_NO_LOGON_SERVERS) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_IO_TIMEOUT) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND)) {
			DEBUG(10,("winbindd_dual_pam_auth_kerberos setting domain to offline\n"));
			domain->online = False;
		}

		/* there are quite some NT_STATUS errors where there is no
		 * point in retrying with a samlogon, we explictly have to take
		 * care not to increase the bad logon counter on the DC */

		if (NT_STATUS_EQUAL(result, NT_STATUS_ACCOUNT_DISABLED) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_ACCOUNT_EXPIRED) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_ACCOUNT_LOCKED_OUT) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_INVALID_LOGON_HOURS) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_INVALID_WORKSTATION) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_LOGON_FAILURE) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_NO_SUCH_USER) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_EXPIRED) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_MUST_CHANGE) ||
		    NT_STATUS_EQUAL(result, NT_STATUS_WRONG_PASSWORD)) {
			goto process_result;
		}
		
		if (state->request.flags & WBFLAG_PAM_FALLBACK_AFTER_KRB5) {
			DEBUG(3,("falling back to samlogon\n"));
			goto sam_logon;
		} else {
			goto cached_logon;
		}
	}

sam_logon:
	/* Check for Samlogon authentication */
	if (domain->online) {
		result = winbindd_dual_pam_auth_samlogon(domain, state, &info3);
	
		if (NT_STATUS_IS_OK(result)) {
			DEBUG(10,("winbindd_dual_pam_auth_samlogon succeeded\n"));
			goto process_result;
		} else {
			DEBUG(10,("winbindd_dual_pam_auth_samlogon failed: %s\n", nt_errstr(result)));
			if (domain->online) {
				/* We're still online - fail. */
				goto done;
			}
			/* Else drop through and see if we can check offline.... */
		}
	}

cached_logon:
	/* Check for Cached logons */
	if (!domain->online && (state->request.flags & WBFLAG_PAM_CACHED_LOGIN) && 
	    lp_winbind_offline_logon()) {
	
		result = winbindd_dual_pam_auth_cached(domain, state, &info3);

		if (NT_STATUS_IS_OK(result)) {
			DEBUG(10,("winbindd_dual_pam_auth_cached succeeded\n"));
			goto process_result;
		} else {
			DEBUG(10,("winbindd_dual_pam_auth_cached failed: %s\n", nt_errstr(result)));
			goto done;
		}
	}

process_result:

	if (NT_STATUS_IS_OK(result)) {
	
		DOM_SID user_sid;

		/* In all codepaths were result == NT_STATUS_OK info3 must have
		   been initialized. */
		if (!info3) {
			result = NT_STATUS_INTERNAL_ERROR;
			goto done;
		}

		netsamlogon_cache_store(name_user, info3);
		wcache_invalidate_samlogon(find_domain_from_name(name_domain), info3);

		/* save name_to_sid info as early as possible */
		sid_compose(&user_sid, &info3->dom_sid.sid, info3->user_rid);
		cache_name2sid(domain, name_domain, name_user, SID_NAME_USER, &user_sid);
		
		/* Check if the user is in the right group */

		if (!NT_STATUS_IS_OK(result = check_info3_in_group(state->mem_ctx, info3,
					state->request.data.auth.require_membership_of_sid))) {
			DEBUG(3, ("User %s is not in the required group (%s), so plaintext authentication is rejected\n",
				  state->request.data.auth.user, 
				  state->request.data.auth.require_membership_of_sid));
			goto done;
		}

		if (state->request.flags & WBFLAG_PAM_INFO3_NDR) {
			result = append_info3_as_ndr(state->mem_ctx, state, info3);
			if (!NT_STATUS_IS_OK(result)) {
				DEBUG(10,("Failed to append INFO3 (NDR): %s\n", nt_errstr(result)));
				goto done;
			}
		}

		if (state->request.flags & WBFLAG_PAM_INFO3_TEXT) {
			result = append_info3_as_txt(state->mem_ctx, state, info3);
			if (!NT_STATUS_IS_OK(result)) {
				DEBUG(10,("Failed to append INFO3 (TXT): %s\n", nt_errstr(result)));
				goto done;
			}

		}

		if ((state->request.flags & WBFLAG_PAM_CACHED_LOGIN) &&
		    lp_winbind_offline_logon()) {

			result = winbindd_store_creds(domain,
						      state->mem_ctx,
						      state->request.data.auth.user,
						      state->request.data.auth.pass,
						      info3, NULL);
			if (!NT_STATUS_IS_OK(result)) {
				DEBUG(10,("Failed to store creds: %s\n", nt_errstr(result)));
				goto done;
			}

		}

			result = fillup_password_policy(domain, state);

			if (!NT_STATUS_IS_OK(result)) {
				DEBUG(10,("Failed to get password policies: %s\n", nt_errstr(result)));
				goto done;
			}
	
	} 

done:
	/* give us a more useful (more correct?) error code */
	if ((NT_STATUS_EQUAL(result, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND) ||
	    (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL)))) {
		result = NT_STATUS_NO_LOGON_SERVERS;
	}
	
	state->response.data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(state->response.data.auth.nt_status_string, nt_errstr(result));

	/* we might have given a more useful error above */
	if (!*state->response.data.auth.error_string) 
		fstrcpy(state->response.data.auth.error_string, get_friendly_nt_error_msg(result));
	state->response.data.auth.pam_error = nt_status_to_pam(result);

	DEBUG(NT_STATUS_IS_OK(result) ? 5 : 2, ("Plain-text authentication for user %s returned %s (PAM: %d)\n", 
	      state->request.data.auth.user, 
	      state->response.data.auth.nt_status_string,
	      state->response.data.auth.pam_error));	      

	if ( NT_STATUS_IS_OK(result) &&
	     (state->request.flags & WBFLAG_PAM_AFS_TOKEN) ) {

		char *afsname = talloc_strdup(state->mem_ctx,
					      lp_afs_username_map());
		char *cell;

		if (afsname == NULL) {
			goto no_token;
		}

		afsname = talloc_string_sub(state->mem_ctx,
					    lp_afs_username_map(),
					    "%D", name_domain);
		afsname = talloc_string_sub(state->mem_ctx, afsname,
					    "%u", name_user);
		afsname = talloc_string_sub(state->mem_ctx, afsname,
					    "%U", name_user);

		{
			DOM_SID user_sid;
			fstring sidstr;

			sid_copy(&user_sid, &info3->dom_sid.sid);
			sid_append_rid(&user_sid, info3->user_rid);
			sid_to_string(sidstr, &user_sid);
			afsname = talloc_string_sub(state->mem_ctx, afsname,
						    "%s", sidstr);
		}

		if (afsname == NULL) {
			goto no_token;
		}

		strlower_m(afsname);

		DEBUG(10, ("Generating token for user %s\n", afsname));

		cell = strchr(afsname, '@');

		if (cell == NULL) {
			goto no_token;
		}

		*cell = '\0';
		cell += 1;

		/* Append an AFS token string */
		SAFE_FREE(state->response.extra_data.data);
		state->response.extra_data.data =
			afs_createtoken_str(afsname, cell);

		if (state->response.extra_data.data != NULL)
			state->response.length +=
				strlen(state->response.extra_data.data)+1;

	no_token:
		TALLOC_FREE(afsname);
	}

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}


/**********************************************************************
 Challenge Response Authentication Protocol 
**********************************************************************/

void winbindd_pam_auth_crap(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain = NULL;
	const char *domain_name = NULL;
	NTSTATUS result;

	if (!state->privileged) {
		char *error_string = NULL;
		DEBUG(2, ("winbindd_pam_auth_crap: non-privileged access "
			  "denied.  !\n"));
		DEBUGADD(2, ("winbindd_pam_auth_crap: Ensure permissions "
			     "on %s are set correctly.\n",
			     get_winbind_priv_pipe_dir()));
		/* send a better message than ACCESS_DENIED */
		error_string = talloc_asprintf(state->mem_ctx,
					       "winbind client not authorized "
					       "to use winbindd_pam_auth_crap."
					       " Ensure permissions on %s "
					       "are set correctly.",
					       get_winbind_priv_pipe_dir());
		fstrcpy(state->response.data.auth.error_string, error_string);
		result = NT_STATUS_ACCESS_DENIED;
		goto done;
	}

	/* Ensure null termination */
	state->request.data.auth_crap.user
		[sizeof(state->request.data.auth_crap.user)-1]=0;
	state->request.data.auth_crap.domain
		[sizeof(state->request.data.auth_crap.domain)-1]=0;

	DEBUG(3, ("[%5lu]: pam auth crap domain: [%s] user: %s\n",
		  (unsigned long)state->pid,
		  state->request.data.auth_crap.domain,
		  state->request.data.auth_crap.user));

	if (*state->request.data.auth_crap.domain != '\0') {
		domain_name = state->request.data.auth_crap.domain;
	} else if (lp_winbind_use_default_domain()) {
		domain_name = lp_workgroup();
	}

	if (domain_name != NULL)
		domain = find_auth_domain(state, domain_name);

	if (domain != NULL) {
		sendto_domain(state, domain);
		return;
	}

	result = NT_STATUS_NO_SUCH_USER;

 done:
	set_auth_errors(&state->response, result);
	DEBUG(5, ("CRAP authentication for %s\\%s returned %s (PAM: %d)\n",
		  state->request.data.auth_crap.domain,
		  state->request.data.auth_crap.user, 
		  state->response.data.auth.nt_status_string,
		  state->response.data.auth.pam_error));
	request_error(state);
	return;
}


enum winbindd_result winbindd_dual_pam_auth_crap(struct winbindd_domain *domain,
						 struct winbindd_cli_state *state) 
{
	NTSTATUS result;
        NET_USER_INFO_3 info3;
	struct rpc_pipe_client *netlogon_pipe;
	const char *name_user = NULL;
	const char *name_domain = NULL;
	const char *workstation;
	struct winbindd_domain *contact_domain;
	int attempts = 0;
	BOOL retry;

	DATA_BLOB lm_resp, nt_resp;

	/* This is child-only, so no check for privileged access is needed
	   anymore */

	/* Ensure null termination */
	state->request.data.auth_crap.user[sizeof(state->request.data.auth_crap.user)-1]=0;
	state->request.data.auth_crap.domain[sizeof(state->request.data.auth_crap.domain)-1]=0;

	name_user = state->request.data.auth_crap.user;

	if (*state->request.data.auth_crap.domain) {
		name_domain = state->request.data.auth_crap.domain;
	} else if (lp_winbind_use_default_domain()) {
		name_domain = lp_workgroup();
	} else {
		DEBUG(5,("no domain specified with username (%s) - failing auth\n", 
			 name_user));
		result = NT_STATUS_NO_SUCH_USER;
		goto done;
	}

	DEBUG(3, ("[%5lu]: pam auth crap domain: %s user: %s\n", (unsigned long)state->pid,
		  name_domain, name_user));
	   
	if (*state->request.data.auth_crap.workstation) {
		workstation = state->request.data.auth_crap.workstation;
	} else {
		workstation = global_myname();
	}

	if (state->request.data.auth_crap.lm_resp_len > sizeof(state->request.data.auth_crap.lm_resp)
		|| state->request.data.auth_crap.nt_resp_len > sizeof(state->request.data.auth_crap.nt_resp)) {
		DEBUG(0, ("winbindd_pam_auth_crap: invalid password length %u/%u\n", 
			  state->request.data.auth_crap.lm_resp_len, 
			  state->request.data.auth_crap.nt_resp_len));
		result = NT_STATUS_INVALID_PARAMETER;
		goto done;
	}

	lm_resp = data_blob_talloc(state->mem_ctx, state->request.data.auth_crap.lm_resp,
					state->request.data.auth_crap.lm_resp_len);
	nt_resp = data_blob_talloc(state->mem_ctx, state->request.data.auth_crap.nt_resp,
					state->request.data.auth_crap.nt_resp_len);

	/* what domain should we contact? */
	
	if ( IS_DC ) {
		if (!(contact_domain = find_domain_from_name(name_domain))) {
			DEBUG(3, ("Authentication for domain for [%s] -> [%s]\\[%s] failed as %s is not a trusted domain\n", 
				  state->request.data.auth_crap.user, name_domain, name_user, name_domain)); 
			result = NT_STATUS_NO_SUCH_USER;
			goto done;
		}
	} else {
		if (is_myname(name_domain)) {
			DEBUG(3, ("Authentication for domain %s (local domain to this server) not supported at this stage\n", name_domain));
			result =  NT_STATUS_NO_SUCH_USER;
			goto done;
		}
		contact_domain = find_our_domain();
	}

	do {
		ZERO_STRUCT(info3);
		retry = False;

		netlogon_pipe = NULL;
		result = cm_connect_netlogon(contact_domain, &netlogon_pipe);

		if (!NT_STATUS_IS_OK(result)) {
			DEBUG(3, ("could not open handle to NETLOGON pipe (error: %s)\n",
				  nt_errstr(result)));
			goto done;
		}

		result = rpccli_netlogon_sam_network_logon(netlogon_pipe,
							   state->mem_ctx,
							   state->request.data.auth_crap.logon_parameters,
							   contact_domain->dcname,
							   name_user,
							   name_domain, 
									/* Bug #3248 - found by Stefan Burkei. */
							   workstation, /* We carefully set this above so use it... */
							   state->request.data.auth_crap.chal,
							   lm_resp,
							   nt_resp,
							   &info3);

		attempts += 1;

		/* We have to try a second time as cm_connect_netlogon
		   might not yet have noticed that the DC has killed
		   our connection. */

		if (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL)) {
			retry = True;
			continue;
		}

		/* if we get access denied, a possible cause was that we had and open
		   connection to the DC, but someone changed our machine account password
		   out from underneath us using 'net rpc changetrustpw' */
		   
		if ( NT_STATUS_EQUAL(result, NT_STATUS_ACCESS_DENIED) ) {
			DEBUG(3,("winbindd_pam_auth: sam_logon returned "
				 "ACCESS_DENIED.  Maybe the trust account "
				"password was changed and we didn't know it. "
				 "Killing connections to domain %s\n",
				name_domain));
			invalidate_cm_connection(&contact_domain->conn);
			retry = True;
		} 

	} while ( (attempts < 2) && retry );

	if (NT_STATUS_IS_OK(result)) {

		netsamlogon_cache_store(name_user, &info3);
		wcache_invalidate_samlogon(find_domain_from_name(name_domain), &info3);

		/* Check if the user is in the right group */

		if (!NT_STATUS_IS_OK(result = check_info3_in_group(state->mem_ctx, &info3,
							state->request.data.auth_crap.require_membership_of_sid))) {
			DEBUG(3, ("User %s is not in the required group (%s), so plaintext authentication is rejected\n",
				  state->request.data.auth_crap.user, 
				  state->request.data.auth_crap.require_membership_of_sid));
			goto done;
		}

		if (state->request.flags & WBFLAG_PAM_INFO3_NDR) {
			result = append_info3_as_ndr(state->mem_ctx, state, &info3);
		} else if (state->request.flags & WBFLAG_PAM_UNIX_NAME) {
			/* ntlm_auth should return the unix username, per 
			   'winbind use default domain' settings and the like */

			fstring username_out;
			const char *nt_username, *nt_domain;
			if (!(nt_username = unistr2_tdup(state->mem_ctx, &(info3.uni_user_name)))) {
				/* If the server didn't give us one, just use the one we sent them */
				nt_username = name_user;
			}

			if (!(nt_domain = unistr2_tdup(state->mem_ctx, &(info3.uni_logon_dom)))) {
				/* If the server didn't give us one, just use the one we sent them */
				nt_domain = name_domain;
			}

			fill_domain_username(username_out, nt_domain, nt_username, True);

			DEBUG(5, ("Setting unix username to [%s]\n", username_out));

			SAFE_FREE(state->response.extra_data.data);
			state->response.extra_data.data = SMB_STRDUP(username_out);
			if (!state->response.extra_data.data) {
				result = NT_STATUS_NO_MEMORY;
				goto done;
			}
			state->response.length +=  strlen(state->response.extra_data.data)+1;
		}
		
		if (state->request.flags & WBFLAG_PAM_USER_SESSION_KEY) {
			memcpy(state->response.data.auth.user_session_key, info3.user_sess_key,
					sizeof(state->response.data.auth.user_session_key) /* 16 */);
		}
		if (state->request.flags & WBFLAG_PAM_LMKEY) {
			memcpy(state->response.data.auth.first_8_lm_hash, info3.lm_sess_key,
					sizeof(state->response.data.auth.first_8_lm_hash) /* 8 */);
		}
	}

done:

	/* give us a more useful (more correct?) error code */
	if ((NT_STATUS_EQUAL(result, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND) ||
	    (NT_STATUS_EQUAL(result, NT_STATUS_UNSUCCESSFUL)))) {
		result = NT_STATUS_NO_LOGON_SERVERS;
	}

	if (state->request.flags & WBFLAG_PAM_NT_STATUS_SQUASH) {
		result = nt_status_squash(result);
	}

	state->response.data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(state->response.data.auth.nt_status_string, nt_errstr(result));

	/* we might have given a more useful error above */
	if (!*state->response.data.auth.error_string) {
		fstrcpy(state->response.data.auth.error_string, get_friendly_nt_error_msg(result));
	}
	state->response.data.auth.pam_error = nt_status_to_pam(result);

	DEBUG(NT_STATUS_IS_OK(result) ? 5 : 2, 
	      ("NTLM CRAP authentication for user [%s]\\[%s] returned %s (PAM: %d)\n", 
	       name_domain,
	       name_user,
	       state->response.data.auth.nt_status_string,
	       state->response.data.auth.pam_error));	      

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}

/* Change a user password */

void winbindd_pam_chauthtok(struct winbindd_cli_state *state)
{
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	char *oldpass;
	char *newpass = NULL;
	fstring domain, user;
	POLICY_HND dom_pol;
	struct winbindd_domain *contact_domain;
	struct rpc_pipe_client *cli;
	BOOL got_info = False;
	SAM_UNK_INFO_1 info;
	SAMR_CHANGE_REJECT reject;

	DEBUG(3, ("[%5lu]: pam chauthtok %s\n", (unsigned long)state->pid,
		state->request.data.chauthtok.user));

	/* Setup crap */

	parse_domain_user(state->request.data.chauthtok.user, domain, user);

	contact_domain = find_domain_from_name(domain);
	if (!contact_domain) {
		DEBUG(3, ("Cannot change password for [%s] -> [%s]\\[%s] as %s is not a trusted domain\n", 
			  state->request.data.chauthtok.user, domain, user, domain)); 
		result = NT_STATUS_NO_SUCH_USER;
		goto done;
	}

	/* Change password */

	oldpass = state->request.data.chauthtok.oldpass;
	newpass = state->request.data.chauthtok.newpass;

	/* Initialize reject reason */
	state->response.data.auth.reject_reason = Undefined;

	/* Get sam handle */

	result = cm_connect_sam(contact_domain, state->mem_ctx, &cli,
				&dom_pol);
	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(1, ("could not get SAM handle on DC for %s\n", domain));
		goto done;
	}

	result = rpccli_samr_chgpasswd3(cli, state->mem_ctx, user, newpass, oldpass, &info, &reject);

	/* FIXME: need to check for other error codes ? */
	if (NT_STATUS_EQUAL(result, NT_STATUS_PASSWORD_RESTRICTION)) {

		state->response.data.auth.policy.min_length_password = 
			info.min_length_password;
		state->response.data.auth.policy.password_history = 
			info.password_history;
		state->response.data.auth.policy.password_properties = 
			info.password_properties;
		state->response.data.auth.policy.expire = 
			nt_time_to_unix_abs(&info.expire);
		state->response.data.auth.policy.min_passwordage = 
			nt_time_to_unix_abs(&info.min_passwordage);

		state->response.data.auth.reject_reason = 
			reject.reject_reason;

		got_info = True;

	/* only fallback when the chgpasswd3 call is not supported */
	} else if ((NT_STATUS_EQUAL(result, NT_STATUS(DCERPC_FAULT_OP_RNG_ERROR))) ||
		   (NT_STATUS_EQUAL(result, NT_STATUS_NOT_SUPPORTED)) ||
		   (NT_STATUS_EQUAL(result, NT_STATUS_NOT_IMPLEMENTED))) {

		DEBUG(10,("Password change with chgpasswd3 failed with: %s, retrying chgpasswd_user\n", 
			nt_errstr(result)));
		
		result = rpccli_samr_chgpasswd_user(cli, state->mem_ctx, user, newpass, oldpass);
	}

done: 
	if (NT_STATUS_IS_OK(result) && (state->request.flags & WBFLAG_PAM_CACHED_LOGIN) &&
	    lp_winbind_offline_logon()) {

		NTSTATUS cred_ret;
		
		cred_ret = winbindd_update_creds_by_name(contact_domain,
							 state->mem_ctx, user,
							 newpass);
		if (!NT_STATUS_IS_OK(cred_ret)) {
			DEBUG(10,("Failed to store creds: %s\n", nt_errstr(cred_ret)));
			goto process_result; /* FIXME: hm, risking inconsistant cache ? */
		}
	}		

	if (!NT_STATUS_IS_OK(result) && !got_info && contact_domain) {

		NTSTATUS policy_ret;
		
		policy_ret = fillup_password_policy(contact_domain, state);

		/* failure of this is non critical, it will just provide no
		 * additional information to the client why the change has
		 * failed - Guenther */

		if (!NT_STATUS_IS_OK(policy_ret)) {
			DEBUG(10,("Failed to get password policies: %s\n", nt_errstr(policy_ret)));
			goto process_result;
		}
	}

process_result:

	state->response.data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(state->response.data.auth.nt_status_string, nt_errstr(result));
	fstrcpy(state->response.data.auth.error_string, get_friendly_nt_error_msg(result));
	state->response.data.auth.pam_error = nt_status_to_pam(result);

	DEBUG(NT_STATUS_IS_OK(result) ? 5 : 2, 
	      ("Password change for user [%s]\\[%s] returned %s (PAM: %d)\n", 
	       domain,
	       user,
	       state->response.data.auth.nt_status_string,
	       state->response.data.auth.pam_error));	      

	if (NT_STATUS_IS_OK(result)) {
		request_ok(state);
	} else {
		request_error(state);
	}
}

void winbindd_pam_logoff(struct winbindd_cli_state *state)
{
	struct winbindd_domain *domain;
	fstring name_domain, user;
	
	DEBUG(3, ("[%5lu]: pam logoff %s\n", (unsigned long)state->pid,
		state->request.data.logoff.user));

	/* Ensure null termination */
	state->request.data.logoff.user
		[sizeof(state->request.data.logoff.user)-1]='\0';

	state->request.data.logoff.krb5ccname
		[sizeof(state->request.data.logoff.krb5ccname)-1]='\0';

	parse_domain_user(state->request.data.logoff.user, name_domain, user);

	domain = find_auth_domain(state, name_domain);

	if (domain == NULL) {
		set_auth_errors(&state->response, NT_STATUS_NO_SUCH_USER);
		DEBUG(5, ("Pam Logoff for %s returned %s "
			  "(PAM: %d)\n",
			  state->request.data.logoff.user, 
			  state->response.data.auth.nt_status_string,
			  state->response.data.auth.pam_error));
		request_error(state);
		return;
	}

	sendto_domain(state, domain);
}

enum winbindd_result winbindd_dual_pam_logoff(struct winbindd_domain *domain,
					      struct winbindd_cli_state *state) 
{
	NTSTATUS result = NT_STATUS_NOT_SUPPORTED;
	struct WINBINDD_CCACHE_ENTRY *entry;
	int ret;

	DEBUG(3, ("[%5lu]: pam dual logoff %s\n", (unsigned long)state->pid,
		state->request.data.logoff.user));

	if (!(state->request.flags & WBFLAG_PAM_KRB5)) {
		result = NT_STATUS_OK;
		goto process_result;
	}

#ifdef HAVE_KRB5
	
	/* what we need here is to find the corresponding krb5 ccache name *we*
	 * created for a given username and destroy it (as the user who created it) */
	
	entry = get_ccache_by_username(state->request.data.logoff.user);
	if (entry == NULL) {
		DEBUG(10,("winbindd_pam_logoff: could not get ccname for user %s\n", 
			state->request.data.logoff.user));
		goto process_result;
	}

	DEBUG(10,("winbindd_pam_logoff: found ccache [%s]\n", entry->ccname));

	if (entry->uid < 0 || state->request.data.logoff.uid < 0) {
		DEBUG(0,("winbindd_pam_logoff: invalid uid\n"));
		goto process_result;
	}

	if (entry->uid != state->request.data.logoff.uid) {
		DEBUG(0,("winbindd_pam_logoff: uid's differ: %d != %d\n", 
			entry->uid, state->request.data.logoff.uid));
		goto process_result;
	}

	if (!strcsequal(entry->ccname, state->request.data.logoff.krb5ccname)) {
		DEBUG(0,("winbindd_pam_logoff: krb5ccnames differ: (daemon) %s != (client) %s\n", 
			entry->ccname, state->request.data.logoff.krb5ccname));
		goto process_result;
	}

	ret = ads_kdestroy(entry->ccname);

	if (ret) {
		DEBUG(0,("winbindd_pam_logoff: failed to destroy user ccache %s with: %s\n", 
			entry->ccname, error_message(ret)));
	} else {
		DEBUG(10,("winbindd_pam_logoff: successfully destroyed ccache %s for user %s\n", 
			entry->ccname, state->request.data.logoff.user));
		remove_ccache_by_ccname(entry->ccname);
	}

	result = krb5_to_nt_status(ret);
#else
	result = NT_STATUS_NOT_SUPPORTED;
#endif

process_result:
	state->response.data.auth.nt_status = NT_STATUS_V(result);
	fstrcpy(state->response.data.auth.nt_status_string, nt_errstr(result));
	fstrcpy(state->response.data.auth.error_string, get_friendly_nt_error_msg(result));
	state->response.data.auth.pam_error = nt_status_to_pam(result);

	return NT_STATUS_IS_OK(result) ? WINBINDD_OK : WINBINDD_ERROR;
}

