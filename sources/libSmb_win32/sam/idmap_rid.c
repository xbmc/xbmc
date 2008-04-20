/*
 *  idmap_rid: static map between Active Directory/NT RIDs and RFC 2307 accounts
 *  Copyright (C) Guenther Deschner, 2004
 *  Copyright (C) Sumit Bose, 2004
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

NTSTATUS init_module(void);

struct dom_entry {
	fstring name;
	fstring sid;
	uint32 min_id;
	uint32 max_id;
};

typedef struct trust_dom_array {
	int number;
	struct dom_entry *dom;
} trust_dom_array;

static trust_dom_array trust;

static NTSTATUS rid_idmap_parse(const char *init_param, 
				uint32 num_domains, 
				fstring *domain_names, 
				DOM_SID *domain_sids, 
				uid_t u_low, 
				uid_t u_high) 
{
	const char *p;
	int i;
	fstring sid_str;
	BOOL known_domain = False;
	fstring tok;

	p = init_param;
	trust.number = 0;

	/* falling back to automatic mapping when there were no options given */
	if (!*init_param) {

		DEBUG(3,("rid_idmap_parse: no domain list given or trusted domain-support deactivated, falling back to automatic mapping for own domain:\n"));

		sid_to_string(sid_str, &domain_sids[0]);

		fstrcpy(trust.dom[0].name, domain_names[0]);
		fstrcpy(trust.dom[0].sid, sid_str);
		trust.dom[0].min_id = u_low; 
		trust.dom[0].max_id = u_high;
		trust.number = 1;

		DEBUGADD(3,("rid_idmap_parse:\tdomain: [%s], sid: [%s], range=[%d-%d]\n", 
				trust.dom[0].name, trust.dom[0].sid, trust.dom[0].min_id, trust.dom[0].max_id));
		return NT_STATUS_OK;
	}

	/* scan through the init_param-list */
	while (next_token(&init_param, tok, LIST_SEP, sizeof(tok))) {

		p = tok;
		DEBUG(3,("rid_idmap_parse: parsing entry: %d\n", trust.number));

		/* reinit sizes */
		trust.dom = SMB_REALLOC_ARRAY(trust.dom, struct dom_entry,
					      trust.number+1);

		if ( trust.dom == NULL ) {
			return NT_STATUS_NO_MEMORY;
		}
		
		if (!next_token(&p, tok, "=", sizeof(tok))) {
			DEBUG(0, ("rid_idmap_parse: no '=' sign found in domain list [%s]\n", init_param));
			return NT_STATUS_UNSUCCESSFUL;
		}

		/* add the name */
		fstrcpy(trust.dom[trust.number].name, tok);
		DEBUGADD(3,("rid_idmap_parse:\tentry %d has name: [%s]\n", trust.number, trust.dom[trust.number].name));

		/* add the domain-sid */
		for (i=0; i<num_domains; i++) {

			known_domain = False;

			if (strequal(domain_names[i], trust.dom[trust.number].name)) {

				sid_to_string(sid_str, &domain_sids[i]);
				fstrcpy(trust.dom[trust.number].sid, sid_str);

				DEBUGADD(3,("rid_idmap_parse:\tentry %d has sid: [%s]\n", trust.number, trust.dom[trust.number].sid));
				known_domain = True;
				break;
			} 
		}

		if (!known_domain) {
			DEBUG(0,("rid_idmap_parse: your DC does not know anything about domain: [%s]\n", trust.dom[trust.number].name));
			return NT_STATUS_INVALID_PARAMETER;
		}

		if (!next_token(&p, tok, "-", sizeof(tok))) {
			DEBUG(0,("rid_idmap_parse: no mapping-range defined\n"));
			return NT_STATUS_INVALID_PARAMETER;
		}

		/* add min_id */
		trust.dom[trust.number].min_id = atoi(tok);
		DEBUGADD(3,("rid_idmap_parse:\tentry %d has min_id: [%d]\n", trust.number, trust.dom[trust.number].min_id));

		/* add max_id */
		trust.dom[trust.number].max_id = atoi(p);
		DEBUGADD(3,("rid_idmap_parse:\tentry %d has max_id: [%d]\n", trust.number, trust.dom[trust.number].max_id));

		trust.number++;
	}

	return NT_STATUS_OK;

}

static NTSTATUS rid_idmap_get_domains(uint32 *num_domains, fstring **domain_names, DOM_SID **domain_sids) 
{
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	struct cli_state *cli;
	struct rpc_pipe_client *pipe_hnd;
	TALLOC_CTX *mem_ctx;
	POLICY_HND pol;
	uint32 des_access = SEC_RIGHTS_MAXIMUM_ALLOWED;
	fstring dc_name;
	struct in_addr dc_ip;
	const char *password = NULL;
	const char *username = NULL;
	const char *domain = NULL;
	uint32 info_class = 5;
	char *domain_name = NULL;
	DOM_SID *domain_sid, sid;
	fstring sid_str;
	int i;
	uint32 trusted_num_domains = 0;
	char **trusted_domain_names;
	DOM_SID *trusted_domain_sids;
	uint32 enum_ctx = 0;
	int own_domains = 2;

	/* put the results together */
	*num_domains = 2;
	*domain_names = SMB_MALLOC_ARRAY(fstring, *num_domains);
	*domain_sids = SMB_MALLOC_ARRAY(DOM_SID, *num_domains);

	/* avoid calling a DC when trusted domains are not allowed anyway */
	if (!lp_allow_trusted_domains()) {

		fstrcpy((*domain_names)[0], lp_workgroup());
		if (!secrets_fetch_domain_sid(lp_workgroup(), &sid)) {
			DEBUG(0,("rid_idmap_get_domains: failed to retrieve domain sid\n"));
			return status;
		}
		sid_copy(&(*domain_sids)[0], &sid);

		/* add BUILTIN */
		fstrcpy((*domain_names)[1], "BUILTIN");
		sid_copy(&(*domain_sids)[1], &global_sid_Builtin);

		return NT_STATUS_OK;
	}

	/* create mem_ctx */
	if (!(mem_ctx = talloc_init("rid_idmap_get_trusted_domains"))) {
		DEBUG(0, ("rid_idmap_get_domains: talloc_init() failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if (!get_dc_name(lp_workgroup(), 0, dc_name, &dc_ip)) {
		DEBUG(1, ("rid_idmap_get_domains: could not get dc-name\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	/* open a connection to the dc */
	username = secrets_fetch(SECRETS_AUTH_USER, NULL);
	password = secrets_fetch(SECRETS_AUTH_PASSWORD, NULL);
	domain =   secrets_fetch(SECRETS_AUTH_DOMAIN, NULL);

	if (username) {

		if (!domain)
			domain = smb_xstrdup(lp_workgroup());

		if (!password)
			password = smb_xstrdup("");

		DEBUG(3, ("rid_idmap_get_domains: IPC$ connections done by user %s\\%s\n", domain, username));

	} else {

		DEBUG(3, ("rid_idmap_get_domains: IPC$ connections done anonymously\n"));
		username = "";
		domain = "";
		password = "";
	}

	DEBUG(10, ("rid_idmap_get_domains: opening connection to [%s]\n", dc_name));

	status = cli_full_connection(&cli, global_myname(), dc_name, 
			NULL, 0,
			"IPC$", "IPC",
			username,
			lp_workgroup(),
			password,
			CLI_FULL_CONNECTION_ANNONYMOUS_FALLBACK, True, NULL);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("rid_idmap_get_domains: could not setup connection to dc\n"));
		return status;
	}	

	/* query the lsa-pipe */
	pipe_hnd = cli_rpc_pipe_open_noauth(cli, PI_LSARPC, &status);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("rid_idmap_get_domains: could not setup connection to dc\n"));
		goto out;
	}

	/* query policies */
	status = rpccli_lsa_open_policy(pipe_hnd, mem_ctx, False, des_access,
					&pol);
	if (!NT_STATUS_IS_OK(status)) {
		goto out;
	}

	status = rpccli_lsa_query_info_policy(pipe_hnd, mem_ctx, &pol,
					      info_class, &domain_name,
					      &domain_sid);
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("rid_idmap_get_domains: cannot retrieve domain-info\n"));
		goto out;
	}

	sid_to_string(sid_str, domain_sid);
	DEBUG(10,("rid_idmap_get_domains: my domain: [%s], sid: [%s]\n", domain_name, sid_str));

	/* scan trusted domains */
	DEBUG(10, ("rid_idmap_get_domains: enumerating trusted domains\n"));
	status = rpccli_lsa_enum_trust_dom(pipe_hnd, mem_ctx, &pol, &enum_ctx,
					   &trusted_num_domains,
					   &trusted_domain_names, 
					   &trusted_domain_sids);

	if (!NT_STATUS_IS_OK(status) &&
	    !NT_STATUS_EQUAL(status, NT_STATUS_NO_MORE_ENTRIES) &&
	    !NT_STATUS_EQUAL(status, STATUS_MORE_ENTRIES)) {
		DEBUG(1, ("rid_idmap_get_domains: could not enumerate trusted domains\n"));
		goto out;
	}

	/* show trusted domains */
	DEBUG(10,("rid_idmap_get_domains: scan for trusted domains gave %d results:\n", trusted_num_domains));
	for (i=0; i<trusted_num_domains; i++) {
		sid_to_string(sid_str, &trusted_domain_sids[i]);
		DEBUGADD(10,("rid_idmap_get_domains:\t#%d\tDOMAIN: [%s], SID: [%s]\n", 
					i, trusted_domain_names[i], sid_str));
	}

	if (!sid_equal(domain_sid, get_global_sam_sid()))
		++own_domains;

	/* put the results together */
	*num_domains = trusted_num_domains + own_domains;
	*domain_names = SMB_REALLOC_ARRAY(*domain_names, fstring,
					  *num_domains);
	if (!*domain_names) {
		goto out;
	}
	*domain_sids = SMB_REALLOC_ARRAY(*domain_sids, DOM_SID, *num_domains);
	if (!*domain_sids) {
		goto out;
	}

	/* first add mydomain */
	fstrcpy((*domain_names)[0], domain_name);
	sid_copy(&(*domain_sids)[0], domain_sid);

	/* then add BUILTIN */
	fstrcpy((*domain_names)[1], "BUILTIN");
	sid_copy(&(*domain_sids)[1], &global_sid_Builtin);

	/* then add my local sid */
	if (!sid_equal(domain_sid, get_global_sam_sid())) {
		fstrcpy((*domain_names)[2], global_myname());
		sid_copy(&(*domain_sids)[2], get_global_sam_sid());
	}

	/* add trusted domains */
	for (i=0; i<trusted_num_domains; i++) {
		fstrcpy((*domain_names)[i+own_domains], trusted_domain_names[i]);
		sid_copy(&((*domain_sids)[i+own_domains]), &(trusted_domain_sids[i]));
	}

	/* show complete domain list */
	DEBUG(5,("rid_idmap_get_domains: complete domain-list has %d entries:\n", *num_domains));
	for (i=0; i<*num_domains; i++) {
		sid_to_string(sid_str, &((*domain_sids)[i]));
		DEBUGADD(5,("rid_idmap_get_domains:\t#%d\tdomain: [%s], sid: [%s]\n", 
					i, (*domain_names)[i], sid_str ));
	}

	status = NT_STATUS_OK;

out:
	rpccli_lsa_close(pipe_hnd, mem_ctx, &pol);
	cli_rpc_pipe_close(pipe_hnd);
	talloc_destroy(mem_ctx);
	cli_shutdown(cli);

	return status;
}

static NTSTATUS rid_idmap_init(char *init_param)
{
	int i, j;
	uid_t u_low, u_high;
	gid_t g_low, g_high;
	uint32 num_domains = 0;
	fstring *domain_names;
	DOM_SID *domain_sids;
	NTSTATUS nt_status = NT_STATUS_INVALID_PARAMETER;
	trust.dom = NULL;

	/* basic sanity checks */
	if (!lp_idmap_uid(&u_low, &u_high) || !lp_idmap_gid(&g_low, &g_high)) {
		DEBUG(0, ("rid_idmap_init: cannot get required global idmap-ranges.\n"));
		return nt_status;
	}

	if (u_low != g_low || u_high != g_high) {
		DEBUG(0, ("rid_idmap_init: range defined in \"idmap uid\" must match range of \"idmap gid\".\n"));
		return nt_status;
	}

	if (lp_allow_trusted_domains()) {
#if IDMAP_RID_SUPPORT_TRUSTED_DOMAINS
		DEBUG(3,("rid_idmap_init: enabling trusted-domain-mapping\n"));
#else
		DEBUG(0,("rid_idmap_init: idmap_rid does not work with trusted domains\n"));
		DEBUGADD(0,("rid_idmap_init: please set \"allow trusted domains\" to \"no\" when using idmap_rid\n"));
		return nt_status;
#endif
	}

	/* init sizes */
	trust.dom = SMB_MALLOC_P(struct dom_entry);
	if (trust.dom == NULL) { 
		return NT_STATUS_NO_MEMORY;
	}

	/* retrieve full domain list */
	nt_status = rid_idmap_get_domains(&num_domains, &domain_names, &domain_sids);
	if (!NT_STATUS_IS_OK(nt_status) &&
	    !NT_STATUS_EQUAL(nt_status, NT_STATUS_NO_MORE_ENTRIES) &&
	    !NT_STATUS_EQUAL(nt_status, STATUS_MORE_ENTRIES)) {
		DEBUG(0, ("rid_idmap_init: cannot fetch sids for domain and/or trusted-domains from domain-controller.\n"));
		return nt_status;
	}

	/* parse the init string */
	nt_status = rid_idmap_parse(init_param, num_domains, domain_names, domain_sids, u_low, u_high);
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("rid_idmap_init: cannot parse module-configuration\n"));
		goto out;
	}

	nt_status = NT_STATUS_INVALID_PARAMETER;

	/* some basic sanity checks */
	for (i=0; i<trust.number; i++) {

		if (trust.dom[i].min_id > trust.dom[i].max_id) {
			DEBUG(0, ("rid_idmap_init: min_id (%d) has to be smaller than max_id (%d) for domain [%s]\n", 
						trust.dom[i].min_id, trust.dom[i].max_id, trust.dom[i].name));
			goto out;
		}

		if (trust.dom[i].min_id < u_low || trust.dom[i].max_id > u_high) {
			DEBUG(0, ("rid_idmap_init: mapping of domain [%s] (%d-%d) has to fit into global idmap range (%d-%d).\n",
						trust.dom[i].name, trust.dom[i].min_id, trust.dom[i].max_id, u_low, u_high));
			goto out;
		}
	}

	/* check for overlaps */
	for (i=0; i<trust.number-1; i++) {
		for (j=i+1; j<trust.number; j++) {
			if (trust.dom[i].min_id <= trust.dom[j].max_id && trust.dom[j].min_id <= trust.dom[i].max_id) {
				DEBUG(0, ("rid_idmap_init: the ranges of domain [%s] and [%s] overlap\n", 
							trust.dom[i+1].name, trust.dom[i].name));
				goto out;
			}
		}
	}
	
	DEBUG(3, ("rid_idmap_init: using %d mappings:\n", trust.number));
	for (i=0; i<trust.number; i++) {
		DEBUGADD(3, ("rid_idmap_init:\tdomain: [%s], sid: [%s], min_id: [%d], max_id: [%d]\n", 
				trust.dom[i].name, trust.dom[i].sid, trust.dom[i].min_id, trust.dom[i].max_id));
	}

	nt_status = NT_STATUS_OK;

out:
	SAFE_FREE(domain_names);
	SAFE_FREE(domain_sids);
		
	return nt_status;
}

static NTSTATUS rid_idmap_get_sid_from_id(DOM_SID *sid, unid_t unid, int id_type)
{
	fstring sid_string;
	int i;
	DOM_SID sidstr;

	/* find range */
	for (i=0; i<trust.number; i++) {
		if (trust.dom[i].min_id <= unid.uid && trust.dom[i].max_id >= unid.uid ) 
			break;
	}

	if (i == trust.number) {
		DEBUG(0,("rid_idmap_get_sid_from_id: no suitable range available for id: %d\n", unid.uid));
		return NT_STATUS_INVALID_PARAMETER;
	}
	
	/* use lower-end of idmap-range as offset for users and groups*/
	unid.uid -= trust.dom[i].min_id;

	if (!trust.dom[i].sid)
		return NT_STATUS_INVALID_PARAMETER;

	string_to_sid(&sidstr, trust.dom[i].sid);
	sid_copy(sid, &sidstr);
	if (!sid_append_rid( sid, (unsigned long)unid.uid )) {
		DEBUG(0,("rid_idmap_get_sid_from_id: could not append rid to domain sid\n"));
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(3, ("rid_idmap_get_sid_from_id: mapped POSIX %s %d to SID [%s]\n",
		(id_type == ID_GROUPID) ? "GID" : "UID", unid.uid,
		sid_to_string(sid_string, sid)));

	return NT_STATUS_OK;
}

static NTSTATUS rid_idmap_get_id_from_sid(unid_t *unid, int *id_type, const DOM_SID *sid)
{
	fstring sid_string;
	int i;
	uint32 rid;
	DOM_SID sidstr;

	/* check if we have a mapping for the sid */
	for (i=0; i<trust.number; i++) {
		if (!trust.dom[i].sid) {
			return NT_STATUS_INVALID_PARAMETER;
		}
		string_to_sid(&sidstr, trust.dom[i].sid);	
		if ( sid_compare_domain(sid, &sidstr) == 0 )
			break;
	}
	
	if (i == trust.number) {
		DEBUG(0,("rid_idmap_get_id_from_sid: no suitable range available for sid: %s\n",
			sid_string_static(sid)));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!sid_peek_rid(sid, &rid)) {
		DEBUG(0,("rid_idmap_get_id_from_sid: could not peek rid\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* use lower-end of idmap-range as offset for users and groups */
	unid->uid = rid + trust.dom[i].min_id;

	if (unid->uid > trust.dom[i].max_id) {
		DEBUG(0,("rid_idmap_get_id_from_sid: rid: %d (%s: %d) too high for mapping of domain: %s (%d-%d)\n", 
			rid, (*id_type == ID_GROUPID) ? "GID" : "UID", unid->uid, trust.dom[i].name, 
			trust.dom[i].min_id, trust.dom[i].max_id));
		return NT_STATUS_INVALID_PARAMETER;
	}
	if (unid->uid < trust.dom[i].min_id) {
		DEBUG(0,("rid_idmap_get_id_from_sid: rid: %d (%s: %d) too low for mapping of domain: %s (%d-%d)\n", 
			rid, (*id_type == ID_GROUPID) ? "GID" : "UID", unid->uid, 
			trust.dom[i].name, trust.dom[i].min_id, trust.dom[i].max_id));
		return NT_STATUS_INVALID_PARAMETER;
	}

	DEBUG(3,("rid_idmap_get_id_from_sid: mapped SID [%s] to POSIX %s %d\n",
		sid_to_string(sid_string, sid),
		(*id_type == ID_GROUPID) ? "GID" : "UID", unid->uid));

	return NT_STATUS_OK;

}

static NTSTATUS rid_idmap_set_mapping(const DOM_SID *sid, unid_t id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS rid_idmap_close(void)
{
	SAFE_FREE(trust.dom);

	return NT_STATUS_OK;
}

static NTSTATUS rid_idmap_allocate_id(unid_t *id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

static void rid_idmap_status(void)
{
	DEBUG(0, ("RID IDMAP Status not available\n"));      
}

static struct idmap_methods rid_methods = {
	rid_idmap_init,
	rid_idmap_allocate_id,
	rid_idmap_get_sid_from_id,
	rid_idmap_get_id_from_sid,
	rid_idmap_set_mapping,
	rid_idmap_close,
	rid_idmap_status
};

NTSTATUS init_module(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION, "rid", &rid_methods);
}

