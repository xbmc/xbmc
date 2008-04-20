/* 
   Unix SMB/CIFS implementation.
   uid/user handling
   Copyright (C) Andrew Tridgell         1992-1998
   Copyright (C) Gerald (Jerry) Carter   2003
   Copyright (C) Volker Lendecke	 2005
   
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

/*****************************************************************
 Dissect a user-provided name into domain, name, sid and type.

 If an explicit domain name was given in the form domain\user, it
 has to try that. If no explicit domain name was given, we have
 to do guesswork.
*****************************************************************/  

BOOL lookup_name(TALLOC_CTX *mem_ctx,
		 const char *full_name, int flags,
		 const char **ret_domain, const char **ret_name,
		 DOM_SID *ret_sid, enum SID_NAME_USE *ret_type)
{
	char *p;
	const char *tmp;
	const char *domain = NULL;
	const char *name = NULL;
	uint32 rid;
	DOM_SID sid;
	enum SID_NAME_USE type;
	TALLOC_CTX *tmp_ctx = talloc_new(mem_ctx);

	if (tmp_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	p = strchr_m(full_name, '\\');

	if (p != NULL) {
		domain = talloc_strndup(tmp_ctx, full_name,
					PTR_DIFF(p, full_name));
		name = talloc_strdup(tmp_ctx, p+1);
	} else {
		domain = talloc_strdup(tmp_ctx, "");
		name = talloc_strdup(tmp_ctx, full_name);
	}

	DEBUG(10,("lookup_name: %s => %s (domain), %s (name)\n", 
		full_name, domain, name));

	if ((domain == NULL) || (name == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		return False;
	}

	if (strequal(domain, get_global_sam_name())) {

		/* It's our own domain, lookup the name in passdb */
		if (lookup_global_sam_name(name, flags, &rid, &type)) {
			sid_copy(&sid, get_global_sam_sid());
			sid_append_rid(&sid, rid);
			goto ok;
		}
		goto failed;
	}

	if (strequal(domain, builtin_domain_name())) {

		/* Explicit request for a name in BUILTIN */
		if (lookup_builtin_name(name, &rid)) {
			sid_copy(&sid, &global_sid_Builtin);
			sid_append_rid(&sid, rid);
			type = SID_NAME_ALIAS;
			goto ok;
		}
		goto failed;
	}

	/* Try the explicit winbind lookup first, don't let it guess the
	 * domain yet at this point yet. This comes later. */

	if ((domain[0] != '\0') &&
	    (winbind_lookup_name(domain, name, &sid, &type))) {
			goto ok;
	}

	if (strequal(domain, unix_users_domain_name())) {
		if (lookup_unix_user_name(name, &sid)) {
			type = SID_NAME_USER;
			goto ok;
		}
		goto failed;
	}

	if (strequal(domain, unix_groups_domain_name())) {
		if (lookup_unix_group_name(name, &sid)) {
			type = SID_NAME_DOM_GRP;
			goto ok;
		}
		goto failed;
	}

	if ((domain[0] == '\0') && (!(flags & LOOKUP_NAME_ISOLATED))) {
		goto failed;
	}

	/* Now the guesswork begins, we haven't been given an explicit
	 * domain. Try the sequence as documented on
	 * http://msdn.microsoft.com/library/en-us/secmgmt/security/lsalookupnames.asp
	 * November 27, 2005 */

	/* 1. well-known names */

	if (lookup_wellknown_name(tmp_ctx, name, &sid, &domain)) {
		type = SID_NAME_WKN_GRP;
		goto ok;
	}

	/* 2. Builtin domain as such */

	if (strequal(name, builtin_domain_name())) {
		/* Swap domain and name */
		tmp = name; name = domain; domain = tmp;
		sid_copy(&sid, &global_sid_Builtin);
		type = SID_NAME_DOMAIN;
		goto ok;
	}

	/* 3. Account domain */

	if (strequal(name, get_global_sam_name())) {
		if (!secrets_fetch_domain_sid(name, &sid)) {
			DEBUG(3, ("Could not fetch my SID\n"));
			goto failed;
		}
		/* Swap domain and name */
		tmp = name; name = domain; domain = tmp;
		type = SID_NAME_DOMAIN;
		goto ok;
	}

	/* 4. Primary domain */

	if (!IS_DC && strequal(name, lp_workgroup())) {
		if (!secrets_fetch_domain_sid(name, &sid)) {
			DEBUG(3, ("Could not fetch the domain SID\n"));
			goto failed;
		}
		/* Swap domain and name */
		tmp = name; name = domain; domain = tmp;
		type = SID_NAME_DOMAIN;
		goto ok;
	}

	/* 5. Trusted domains as such, to me it looks as if members don't do
              this, tested an XP workstation in a NT domain -- vl */

	if (IS_DC && (secrets_fetch_trusted_domain_password(name, NULL,
							    &sid, NULL))) {
		/* Swap domain and name */
		tmp = name; name = domain; domain = tmp;
		type = SID_NAME_DOMAIN;
		goto ok;
	}

	/* 6. Builtin aliases */	

	if (lookup_builtin_name(name, &rid)) {
		domain = talloc_strdup(tmp_ctx, builtin_domain_name());
		sid_copy(&sid, &global_sid_Builtin);
		sid_append_rid(&sid, rid);
		type = SID_NAME_ALIAS;
		goto ok;
	}

	/* 7. Local systems' SAM (DCs don't have a local SAM) */
	/* 8. Primary SAM (On members, this is the domain) */

	/* Both cases are done by looking at our passdb */

	if (lookup_global_sam_name(name, flags, &rid, &type)) {
		domain = talloc_strdup(tmp_ctx, get_global_sam_name());
		sid_copy(&sid, get_global_sam_sid());
		sid_append_rid(&sid, rid);
		goto ok;
	}

	/* Now our local possibilities are exhausted. */

	if (!(flags & LOOKUP_NAME_REMOTE)) {
		goto failed;
	}

	/* If we are not a DC, we have to ask in our primary domain. Let
	 * winbind do that. */

	if (!IS_DC &&
	    (winbind_lookup_name(lp_workgroup(), name, &sid, &type))) {
		domain = talloc_strdup(tmp_ctx, lp_workgroup());
		goto ok;
	}

	/* 9. Trusted domains */

	/* If we're a DC we have to ask all trusted DC's. Winbind does not do
	 * that (yet), but give it a chance. */

	if (IS_DC && winbind_lookup_name("", name, &sid, &type)) {
		DOM_SID dom_sid;
		uint32 tmp_rid;
		enum SID_NAME_USE domain_type;
		
		if (type == SID_NAME_DOMAIN) {
			/* Swap name and type */
			tmp = name; name = domain; domain = tmp;
			goto ok;
		}

		/* Here we have to cope with a little deficiency in the
		 * winbind API: We have to ask it again for the name of the
		 * domain it figured out itself. Maybe fix that later... */

		sid_copy(&dom_sid, &sid);
		sid_split_rid(&dom_sid, &tmp_rid);

		if (!winbind_lookup_sid(tmp_ctx, &dom_sid, &domain, NULL,
					&domain_type) ||
		    (domain_type != SID_NAME_DOMAIN)) {
			DEBUG(2, ("winbind could not find the domain's name "
				  "it just looked up for us\n"));
			goto failed;
		}
		goto ok;
	}

	/* 10. Don't translate */

	/* 11. Ok, windows would end here. Samba has two more options:
               Unmapped users and unmapped groups */

	if (lookup_unix_user_name(name, &sid)) {
		domain = talloc_strdup(tmp_ctx, unix_users_domain_name());
		type = SID_NAME_USER;
		goto ok;
	}

	if (lookup_unix_group_name(name, &sid)) {
		domain = talloc_strdup(tmp_ctx, unix_groups_domain_name());
		type = SID_NAME_DOM_GRP;
		goto ok;
	}

 failed:
	TALLOC_FREE(tmp_ctx);
	return False;

 ok:
	if ((domain == NULL) || (name == NULL)) {
		DEBUG(0, ("talloc failed\n"));
		TALLOC_FREE(tmp_ctx);
		return False;
	}

	if (ret_name != NULL) {
		*ret_name = talloc_steal(mem_ctx, name);
	}

	if (ret_domain != NULL) {
		char *tmp_dom = talloc_strdup(tmp_ctx, domain);
		strupper_m(tmp_dom);
		*ret_domain = talloc_steal(mem_ctx, tmp_dom);
	}

	if (ret_sid != NULL) {
		sid_copy(ret_sid, &sid);
	}

	if (ret_type != NULL) {
		*ret_type = type;
	}

	TALLOC_FREE(tmp_ctx);
	return True;
}

/************************************************************************
 Names from smb.conf can be unqualified. eg. valid users = foo
 These names should never map to a remote name. Try global_sam_name()\foo,
 and then "Unix Users"\foo (or "Unix Groups"\foo).
************************************************************************/

BOOL lookup_name_smbconf(TALLOC_CTX *mem_ctx,
		 const char *full_name, int flags,
		 const char **ret_domain, const char **ret_name,
		 DOM_SID *ret_sid, enum SID_NAME_USE *ret_type)
{
	char *qualified_name;
	const char *p;

	/* NB. No winbindd_separator here as lookup_name needs \\' */
	if ((p = strchr_m(full_name, *lp_winbind_separator())) != NULL) {

		/* The name is already qualified with a domain. */

		if (*lp_winbind_separator() != '\\') {
			char *tmp;

			/* lookup_name() needs '\\' as a separator */

			tmp = talloc_strdup(mem_ctx, full_name);
			if (!tmp) {
				return False;
			}
			tmp[p - full_name] = '\\';
			full_name = tmp;
		}

		return lookup_name(mem_ctx, full_name, flags,
				ret_domain, ret_name,
				ret_sid, ret_type);
	}

	/* Try with our own SAM name. */
	qualified_name = talloc_asprintf(mem_ctx, "%s\\%s",
				get_global_sam_name(),
				full_name );
	if (!qualified_name) {
		return False;
	}

	if (lookup_name(mem_ctx, qualified_name, flags,
				ret_domain, ret_name,
				ret_sid, ret_type)) {
		return True;
	}

	/* Finally try with "Unix Users" or "Unix Group" */
	qualified_name = talloc_asprintf(mem_ctx, "%s\\%s",
				flags & LOOKUP_NAME_GROUP ?
					unix_groups_domain_name() :
					unix_users_domain_name(),
				full_name );
	if (!qualified_name) {
		return False;
	}

	return lookup_name(mem_ctx, qualified_name, flags,
				ret_domain, ret_name,
				ret_sid, ret_type);
}

static BOOL winbind_lookup_rids(TALLOC_CTX *mem_ctx,
				const DOM_SID *domain_sid,
				int num_rids, uint32 *rids,
				const char **domain_name,
				const char **names, uint32 *types)
{
	/* Unless the winbind interface is upgraded, fall back to ask for
	 * individual sids. I imagine introducing a lookuprids operation that
	 * directly proxies to lsa_lookupsids to the correct DC. -- vl */

	int i;
	for (i=0; i<num_rids; i++) {
		DOM_SID sid;

		sid_copy(&sid, domain_sid);
		sid_append_rid(&sid, rids[i]);

		if (winbind_lookup_sid(mem_ctx, &sid,
				       *domain_name == NULL ?
				       domain_name : NULL,
				       &names[i], &types[i])) {
			if ((names[i] == NULL) || ((*domain_name) == NULL)) {
				return False;
			}
		} else {
			types[i] = SID_NAME_UNKNOWN;
		}
	}
	return True;
}

static BOOL lookup_rids(TALLOC_CTX *mem_ctx, const DOM_SID *domain_sid,
			int num_rids, uint32_t *rids,
			const char **domain_name,
			const char ***names, enum SID_NAME_USE **types)
{
	int i;

	*names = TALLOC_ARRAY(mem_ctx, const char *, num_rids);
	*types = TALLOC_ARRAY(mem_ctx, enum SID_NAME_USE, num_rids);

	if ((*names == NULL) || (*types == NULL)) {
		return False;
	}

	if (sid_check_is_domain(domain_sid)) {
		NTSTATUS result;

		if (*domain_name == NULL) {
			*domain_name = talloc_strdup(
				mem_ctx, get_global_sam_name());
		}

		if (*domain_name == NULL) {
			return False;
		}

		become_root_uid_only();
		result = pdb_lookup_rids(domain_sid, num_rids, rids,
					 *names, *types);
		unbecome_root_uid_only();

		return (NT_STATUS_IS_OK(result) ||
			NT_STATUS_EQUAL(result, NT_STATUS_NONE_MAPPED) ||
			NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED));
	}

	if (sid_check_is_builtin(domain_sid)) {

		if (*domain_name == NULL) {
			*domain_name = talloc_strdup(
				mem_ctx, builtin_domain_name());
		}

		if (*domain_name == NULL) {
			return False;
		}

		for (i=0; i<num_rids; i++) {
			if (lookup_builtin_rid(*names, rids[i],
					       &(*names)[i])) {
				if ((*names)[i] == NULL) {
					return False;
				}
				(*types)[i] = SID_NAME_ALIAS;
			} else {
				(*types)[i] = SID_NAME_UNKNOWN;
			}
		}
		return True;
	}

	if (sid_check_is_wellknown_domain(domain_sid, NULL)) {
		for (i=0; i<num_rids; i++) {
			DOM_SID sid;
			sid_copy(&sid, domain_sid);
			sid_append_rid(&sid, rids[i]);
			if (lookup_wellknown_sid(mem_ctx, &sid,
						 domain_name, &(*names)[i])) {
				if ((*names)[i] == NULL) {
					return False;
				}
				(*types)[i] = SID_NAME_WKN_GRP;
			} else {
				(*types)[i] = SID_NAME_UNKNOWN;
			}
		}
		return True;
	}

	if (sid_check_is_unix_users(domain_sid)) {
		if (*domain_name == NULL) {
			*domain_name = talloc_strdup(
				mem_ctx, unix_users_domain_name());
		}
		for (i=0; i<num_rids; i++) {
			(*names)[i] = talloc_strdup(
				(*names), uidtoname(rids[i]));
			(*types)[i] = SID_NAME_USER;
		}
		return True;
	}

	if (sid_check_is_unix_groups(domain_sid)) {
		if (*domain_name == NULL) {
			*domain_name = talloc_strdup(
				mem_ctx, unix_groups_domain_name());
		}
		for (i=0; i<num_rids; i++) {
			(*names)[i] = talloc_strdup(
				(*names), gidtoname(rids[i]));
			(*types)[i] = SID_NAME_DOM_GRP;
		}
		return True;
	}

	return winbind_lookup_rids(mem_ctx, domain_sid, num_rids, rids,
				   domain_name, *names, *types);
}

/*
 * Is the SID a domain as such? If yes, lookup its name.
 */

static BOOL lookup_as_domain(const DOM_SID *sid, TALLOC_CTX *mem_ctx,
			     const char **name)
{
	const char *tmp;
	enum SID_NAME_USE type;

	if (sid_check_is_domain(sid)) {
		*name = talloc_strdup(mem_ctx, get_global_sam_name());
		return True;
	}

	if (sid_check_is_builtin(sid)) {
		*name = talloc_strdup(mem_ctx, builtin_domain_name());
		return True;
	}

	if (sid_check_is_wellknown_domain(sid, &tmp)) {
		*name = talloc_strdup(mem_ctx, tmp);
		return True;
	}

	if (sid->num_auths != 4) {
		/* This can't be a domain */
		return False;
	}

	if (IS_DC) {
		uint32 i, num_domains;
		struct trustdom_info **domains;

		/* This is relatively expensive, but it happens only on DCs
		 * and for SIDs that have 4 sub-authorities and thus look like
		 * domains */

		if (!NT_STATUS_IS_OK(secrets_trusted_domains(mem_ctx,
							     &num_domains,
							     &domains))) {
			return False;
		}

		for (i=0; i<num_domains; i++) {
			if (sid_equal(sid, &domains[i]->sid)) {
				*name = talloc_strdup(mem_ctx,
						      domains[i]->name);
				return True;
			}
		}
		return False;
	}

	if (winbind_lookup_sid(mem_ctx, sid, &tmp, NULL, &type) &&
	    (type == SID_NAME_DOMAIN)) {
		*name = tmp;
		return True;
	}

	return False;
}

/*
 * This tries to implement the rather weird rules for the lsa_lookup level
 * parameter.
 *
 * This is as close as we can get to what W2k3 does. With this we survive the
 * RPC-LSALOOKUP samba4 test as of 2006-01-08. NT4 as a PDC is a bit more
 * different, but I assume that's just being too liberal. For example, W2k3
 * replies to everything else but the levels 1-6 with INVALID_PARAMETER
 * whereas NT4 does the same as level 1 (I think). I did not fully test that
 * with NT4, this is what w2k3 does.
 *
 * Level 1: Ask everywhere
 * Level 2: Ask domain and trusted domains, no builtin and wkn
 * Level 3: Only ask domain
 * Level 4: W2k3ad: Only ask AD trusts
 * Level 5: Don't lookup anything
 * Level 6: Like 4
 */

static BOOL check_dom_sid_to_level(const DOM_SID *sid, int level)
{
	int ret = False;

	switch(level) {
	case 1:
		ret = True;
		break;
	case 2:
		ret = (!sid_check_is_builtin(sid) &&
		       !sid_check_is_wellknown_domain(sid, NULL));
		break;
	case 3:
	case 4:
	case 6:
		ret = sid_check_is_domain(sid);
		break;
	case 5:
		ret = False;
		break;
	}

	DEBUG(10, ("%s SID %s in level %d\n",
		   ret ? "Accepting" : "Rejecting",
		   sid_string_static(sid), level));
	return ret;
}

/*
 * Lookup a bunch of SIDs. This is modeled after lsa_lookup_sids with
 * references to domains, it is explicitly made for this.
 *
 * This attempts to be as efficient as possible: It collects all SIDs
 * belonging to a domain and hands them in bulk to the appropriate lookup
 * function. In particular pdb_lookup_rids with ldapsam_trusted benefits
 * *hugely* from this. Winbind is going to be extended with a lookup_rids
 * interface as well, so on a DC we can do a bulk lsa_lookuprids to the
 * appropriate DC.
 */

NTSTATUS lookup_sids(TALLOC_CTX *mem_ctx, int num_sids,
		     const DOM_SID **sids, int level,
		     struct lsa_dom_info **ret_domains,
		     struct lsa_name_info **ret_names)
{
	TALLOC_CTX *tmp_ctx;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	struct lsa_name_info *name_infos;
	struct lsa_dom_info *dom_infos;

	int i, j;

	tmp_ctx = talloc_new(mem_ctx);
	if (tmp_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return NT_STATUS_NO_MEMORY;
	}

	name_infos = TALLOC_ARRAY(tmp_ctx, struct lsa_name_info, num_sids);
	dom_infos = TALLOC_ZERO_ARRAY(tmp_ctx, struct lsa_dom_info,
				      MAX_REF_DOMAINS);
	if ((name_infos == NULL) || (dom_infos == NULL)) {
		result = NT_STATUS_NO_MEMORY;
		goto done;
	}

	/* First build up the data structures:
	 * 
	 * dom_infos is a list of domains referenced in the list of
	 * SIDs. Later we will walk the list of domains and look up the RIDs
	 * in bulk.
	 *
	 * name_infos is a shadow-copy of the SIDs array to collect the real
	 * data.
	 *
	 * dom_info->idxs is an index into the name_infos array. The
	 * difficulty we have here is that we need to keep the SIDs the client
	 * asked for in the same order for the reply
	 */

	for (i=0; i<num_sids; i++) {
		DOM_SID sid;
		uint32 rid;
		const char *domain_name = NULL;

		sid_copy(&sid, sids[i]);
		name_infos[i].type = SID_NAME_USE_NONE;

		if (lookup_as_domain(&sid, name_infos, &domain_name)) {
			/* We can't push that through the normal lookup
			 * process, as this would reference illegal
			 * domains.
			 *
			 * For example S-1-5-32 would end up referencing
			 * domain S-1-5- with RID 32 which is clearly wrong.
			 */
			if (domain_name == NULL) {
				result = NT_STATUS_NO_MEMORY;
				goto done;
			}
				
			name_infos[i].rid = 0;
			name_infos[i].type = SID_NAME_DOMAIN;
			name_infos[i].name = NULL;

			if (sid_check_is_builtin(&sid)) {
				/* Yes, W2k3 returns "BUILTIN" both as domain
				 * and name here */
				name_infos[i].name = talloc_strdup(
					name_infos, builtin_domain_name());
				if (name_infos[i].name == NULL) {
					result = NT_STATUS_NO_MEMORY;
					goto done;
				}
			}
		} else {
			/* This is a normal SID with rid component */
			if (!sid_split_rid(&sid, &rid)) {
				result = NT_STATUS_INVALID_PARAMETER;
				goto done;
			}
		}

		if (!check_dom_sid_to_level(&sid, level)) {
			name_infos[i].rid = 0;
			name_infos[i].type = SID_NAME_UNKNOWN;
			name_infos[i].name = NULL;
			continue;
		}

		for (j=0; j<MAX_REF_DOMAINS; j++) {
			if (!dom_infos[j].valid) {
				break;
			}
			if (sid_equal(&sid, &dom_infos[j].sid)) {
				break;
			}
		}

		if (j == MAX_REF_DOMAINS) {
			/* TODO: What's the right error message here? */
			result = NT_STATUS_NONE_MAPPED;
			goto done;
		}

		if (!dom_infos[j].valid) {
			/* We found a domain not yet referenced, create a new
			 * ref. */
			dom_infos[j].valid = True;
			sid_copy(&dom_infos[j].sid, &sid);

			if (domain_name != NULL) {
				/* This name was being found above in the case
				 * when we found a domain SID */
				dom_infos[j].name =
					talloc_steal(dom_infos, domain_name);
			} else {
				/* lookup_rids will take care of this */
				dom_infos[j].name = NULL;
			}
		}

		name_infos[i].dom_idx = j;

		if (name_infos[i].type == SID_NAME_USE_NONE) {
			name_infos[i].rid = rid;

			ADD_TO_ARRAY(dom_infos, int, i, &dom_infos[j].idxs,
				     &dom_infos[j].num_idxs);

			if (dom_infos[j].idxs == NULL) {
				result = NT_STATUS_NO_MEMORY;
				goto done;
			}
		}
	}

	/* Iterate over the domains found */

	for (i=0; i<MAX_REF_DOMAINS; i++) {
		uint32_t *rids;
		const char **names;
		enum SID_NAME_USE *types;
		struct lsa_dom_info *dom = &dom_infos[i];

		if (!dom->valid) {
			/* No domains left, we're done */
			break;
		}

		rids = TALLOC_ARRAY(tmp_ctx, uint32, dom->num_idxs);

		if (rids == NULL) {
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}

		for (j=0; j<dom->num_idxs; j++) {
			rids[j] = name_infos[dom->idxs[j]].rid;
		}

		if (!lookup_rids(tmp_ctx, &dom->sid,
				 dom->num_idxs, rids, &dom->name,
				 &names, &types)) {
			result = NT_STATUS_NO_MEMORY;
			goto done;
		}

		talloc_steal(dom_infos, dom->name);

		for (j=0; j<dom->num_idxs; j++) {
			int idx = dom->idxs[j];
			name_infos[idx].type = types[j];
			if (types[j] != SID_NAME_UNKNOWN) {
				name_infos[idx].name =
					talloc_steal(name_infos, names[j]);
			} else {
				name_infos[idx].name = NULL;
			}
		}
	}

	*ret_domains = talloc_steal(mem_ctx, dom_infos);
	*ret_names = talloc_steal(mem_ctx, name_infos);
	result = NT_STATUS_OK;

 done:
	TALLOC_FREE(tmp_ctx);
	return result;
}

/*****************************************************************
 *THE CANONICAL* convert SID to name function.
*****************************************************************/  

BOOL lookup_sid(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
		const char **ret_domain, const char **ret_name,
		enum SID_NAME_USE *ret_type)
{
	struct lsa_dom_info *domain;
	struct lsa_name_info *name;
	TALLOC_CTX *tmp_ctx;
	BOOL ret = False;

	tmp_ctx = talloc_new(mem_ctx);

	if (tmp_ctx == NULL) {
		DEBUG(0, ("talloc_new failed\n"));
		return False;
	}

	if (!NT_STATUS_IS_OK(lookup_sids(tmp_ctx, 1, &sid, 1,
					 &domain, &name))) {
		goto done;
	}

	if (name->type == SID_NAME_UNKNOWN) {
		goto done;
	}

	if (ret_domain != NULL) {
		*ret_domain = talloc_steal(mem_ctx, domain->name);
	}

	if (ret_name != NULL) {
		*ret_name = talloc_steal(mem_ctx, name->name);
	}

	if (ret_type != NULL) {
		*ret_type = name->type;
	}

	ret = True;

 done:
	if (ret) {
		DEBUG(10, ("Sid %s -> %s\\%s(%d)\n",
			   sid_string_static(sid), domain->name,
			   name->name, name->type));
	} else {
		DEBUG(10, ("failed to lookup sid %s\n",
			   sid_string_static(sid)));
	}
	TALLOC_FREE(tmp_ctx);
	return ret;
}

/*****************************************************************
 Id mapping cache.  This is to avoid Winbind mappings already
 seen by smbd to be queried too frequently, keeping winbindd
 busy, and blocking smbd while winbindd is busy with other
 stuff. Written by Michael Steffens <michael.steffens@hp.com>,
 modified to use linked lists by jra.
*****************************************************************/  

#define MAX_UID_SID_CACHE_SIZE 100
#define TURNOVER_UID_SID_CACHE_SIZE 10
#define MAX_GID_SID_CACHE_SIZE 100
#define TURNOVER_GID_SID_CACHE_SIZE 10

static size_t n_uid_sid_cache = 0;
static size_t n_gid_sid_cache = 0;

static struct uid_sid_cache {
	struct uid_sid_cache *next, *prev;
	uid_t uid;
	DOM_SID sid;
	enum SID_NAME_USE sidtype;
} *uid_sid_cache_head;

static struct gid_sid_cache {
	struct gid_sid_cache *next, *prev;
	gid_t gid;
	DOM_SID sid;
	enum SID_NAME_USE sidtype;
} *gid_sid_cache_head;

/*****************************************************************
  Find a SID given a uid.
*****************************************************************/  

static BOOL fetch_sid_from_uid_cache(DOM_SID *psid, uid_t uid)
{
	struct uid_sid_cache *pc;

	for (pc = uid_sid_cache_head; pc; pc = pc->next) {
		if (pc->uid == uid) {
			*psid = pc->sid;
			DEBUG(3,("fetch sid from uid cache %u -> %s\n",
				 (unsigned int)uid, sid_string_static(psid)));
			DLIST_PROMOTE(uid_sid_cache_head, pc);
			return True;
		}
	}
	return False;
}

/*****************************************************************
  Find a uid given a SID.
*****************************************************************/  

static BOOL fetch_uid_from_cache( uid_t *puid, const DOM_SID *psid )
{
	struct uid_sid_cache *pc;

	for (pc = uid_sid_cache_head; pc; pc = pc->next) {
		if (sid_compare(&pc->sid, psid) == 0) {
			*puid = pc->uid;
			DEBUG(3,("fetch uid from cache %u -> %s\n",
				 (unsigned int)*puid, sid_string_static(psid)));
			DLIST_PROMOTE(uid_sid_cache_head, pc);
			return True;
		}
	}
	return False;
}

/*****************************************************************
 Store uid to SID mapping in cache.
*****************************************************************/  

void store_uid_sid_cache(const DOM_SID *psid, uid_t uid)
{
	struct uid_sid_cache *pc;

	/* do not store SIDs in the "Unix Group" domain */
	
	if ( sid_check_is_in_unix_users( psid ) )
		return;

	if (n_uid_sid_cache >= MAX_UID_SID_CACHE_SIZE && n_uid_sid_cache > TURNOVER_UID_SID_CACHE_SIZE) {
		/* Delete the last TURNOVER_UID_SID_CACHE_SIZE entries. */
		struct uid_sid_cache *pc_next;
		size_t i;

		for (i = 0, pc = uid_sid_cache_head; i < (n_uid_sid_cache - TURNOVER_UID_SID_CACHE_SIZE); i++, pc = pc->next)
			;
		for(; pc; pc = pc_next) {
			pc_next = pc->next;
			DLIST_REMOVE(uid_sid_cache_head,pc);
			SAFE_FREE(pc);
			n_uid_sid_cache--;
		}
	}

	pc = SMB_MALLOC_P(struct uid_sid_cache);
	if (!pc)
		return;
	pc->uid = uid;
	sid_copy(&pc->sid, psid);
	DLIST_ADD(uid_sid_cache_head, pc);
	n_uid_sid_cache++;
}

/*****************************************************************
  Find a SID given a gid.
*****************************************************************/  

static BOOL fetch_sid_from_gid_cache(DOM_SID *psid, gid_t gid)
{
	struct gid_sid_cache *pc;

	for (pc = gid_sid_cache_head; pc; pc = pc->next) {
		if (pc->gid == gid) {
			*psid = pc->sid;
			DEBUG(3,("fetch sid from gid cache %u -> %s\n",
				 (unsigned int)gid, sid_string_static(psid)));
			DLIST_PROMOTE(gid_sid_cache_head, pc);
			return True;
		}
	}
	return False;
}

/*****************************************************************
  Find a gid given a SID.
*****************************************************************/  

static BOOL fetch_gid_from_cache(gid_t *pgid, const DOM_SID *psid)
{
	struct gid_sid_cache *pc;

	for (pc = gid_sid_cache_head; pc; pc = pc->next) {
		if (sid_compare(&pc->sid, psid) == 0) {
			*pgid = pc->gid;
			DEBUG(3,("fetch gid from cache %u -> %s\n",
				 (unsigned int)*pgid, sid_string_static(psid)));
			DLIST_PROMOTE(gid_sid_cache_head, pc);
			return True;
		}
	}
	return False;
}

/*****************************************************************
 Store gid to SID mapping in cache.
*****************************************************************/  

void store_gid_sid_cache(const DOM_SID *psid, gid_t gid)
{
	struct gid_sid_cache *pc;
	
	/* do not store SIDs in the "Unix Group" domain */
	
	if ( sid_check_is_in_unix_groups( psid ) )
		return;

	if (n_gid_sid_cache >= MAX_GID_SID_CACHE_SIZE && n_gid_sid_cache > TURNOVER_GID_SID_CACHE_SIZE) {
		/* Delete the last TURNOVER_GID_SID_CACHE_SIZE entries. */
		struct gid_sid_cache *pc_next;
		size_t i;

		for (i = 0, pc = gid_sid_cache_head; i < (n_gid_sid_cache - TURNOVER_GID_SID_CACHE_SIZE); i++, pc = pc->next)
			;
		for(; pc; pc = pc_next) {
			pc_next = pc->next;
			DLIST_REMOVE(gid_sid_cache_head,pc);
			SAFE_FREE(pc);
			n_gid_sid_cache--;
		}
	}

	pc = SMB_MALLOC_P(struct gid_sid_cache);
	if (!pc)
		return;
	pc->gid = gid;
	sid_copy(&pc->sid, psid);
	DLIST_ADD(gid_sid_cache_head, pc);

	DEBUG(3,("store_gid_sid_cache: gid %u in cache -> %s\n", (unsigned int)gid,
		sid_string_static(psid)));

	n_gid_sid_cache++;
}

/*****************************************************************
 *THE CANONICAL* convert uid_t to SID function.
*****************************************************************/  

void uid_to_sid(DOM_SID *psid, uid_t uid)
{
	uid_t low, high;
	uint32 rid;
	BOOL ret;

	ZERO_STRUCTP(psid);

	if (fetch_sid_from_uid_cache(psid, uid))
		return;

	if ((lp_winbind_trusted_domains_only() ||
	     (lp_idmap_uid(&low, &high) && (uid >= low) && (uid <= high))) &&
	    winbind_uid_to_sid(psid, uid)) {

		DEBUG(10,("uid_to_sid: winbindd %u -> %s\n",
			  (unsigned int)uid, sid_string_static(psid)));
		goto done;
	}

	become_root_uid_only();
	ret = pdb_uid_to_rid(uid, &rid);
	unbecome_root_uid_only();

	if (ret) {
		/* This is a mapped user */
		sid_copy(psid, get_global_sam_sid());
		sid_append_rid(psid, rid);
		goto done;
	}

	/* This is an unmapped user */

	uid_to_unix_users_sid(uid, psid);

 done:
	DEBUG(10,("uid_to_sid: local %u -> %s\n", (unsigned int)uid,
		  sid_string_static(psid)));

	store_uid_sid_cache(psid, uid);
	return;
}

/*****************************************************************
 *THE CANONICAL* convert gid_t to SID function.
*****************************************************************/  

void gid_to_sid(DOM_SID *psid, gid_t gid)
{
	BOOL ret;
	gid_t low, high;

	ZERO_STRUCTP(psid);

	if (fetch_sid_from_gid_cache(psid, gid))
		return;

	if ((lp_winbind_trusted_domains_only() ||
	     (lp_idmap_gid(&low, &high) && (gid >= low) && (gid <= high))) &&
	    winbind_gid_to_sid(psid, gid)) {

		DEBUG(10,("gid_to_sid: winbindd %u -> %s\n",
			  (unsigned int)gid, sid_string_static(psid)));
		goto done;
	}

	become_root_uid_only();
	ret = pdb_gid_to_sid(gid, psid);
	unbecome_root_uid_only();

	if (ret) {
		/* This is a mapped group */
		goto done;
	}
	
	/* This is an unmapped group */

	gid_to_unix_groups_sid(gid, psid);

 done:
	DEBUG(10,("gid_to_sid: local %u -> %s\n", (unsigned int)gid,
		  sid_string_static(psid)));

	store_gid_sid_cache(psid, gid);
	return;
}

/*****************************************************************
 *THE CANONICAL* convert SID to uid function.
*****************************************************************/  

BOOL sid_to_uid(const DOM_SID *psid, uid_t *puid)
{
	enum SID_NAME_USE type;
	uint32 rid;
	gid_t gid;

	if (fetch_uid_from_cache(puid, psid))
		return True;

	if (fetch_gid_from_cache(&gid, psid)) {
		return False;
	}

	if (sid_peek_check_rid(&global_sid_Unix_Users, psid, &rid)) {
		uid_t uid = rid;
		*puid = uid;
		goto done;
	}

	if (sid_peek_check_rid(get_global_sam_sid(), psid, &rid)) {
		union unid_t id;
		BOOL ret;

		become_root_uid_only();
		ret = pdb_sid_to_id(psid, &id, &type);
		unbecome_root_uid_only();

		if (ret) {
			if (type != SID_NAME_USER) {
				DEBUG(5, ("sid %s is a %s, expected a user\n",
					  sid_string_static(psid),
					  sid_type_lookup(type)));
				return False;
			}
			*puid = id.uid;
			goto done;
		}

		/* This was ours, but it was not mapped.  Fail */

		return False;
	}

	if (winbind_lookup_sid(NULL, psid, NULL, NULL, &type)) {

		if (type != SID_NAME_USER) {
			DEBUG(10, ("sid_to_uid: sid %s is a %s\n",
				   sid_string_static(psid),
				   sid_type_lookup(type)));
			return False;
		}

		if (!winbind_sid_to_uid(puid, psid)) {
			DEBUG(5, ("sid_to_uid: winbind failed to allocate a "
				  "new uid for sid %s\n",
				  sid_string_static(psid)));
			return False;
		}
		goto done;
	}

	/* TODO: Here would be the place to allocate both a gid and a uid for
	 * the SID in question */

	return False;

 done:
	DEBUG(10,("sid_to_uid: %s -> %u\n", sid_string_static(psid),
		(unsigned int)*puid ));

	store_uid_sid_cache(psid, *puid);
	return True;
}

/*****************************************************************
 *THE CANONICAL* convert SID to gid function.
 Group mapping is used for gids that maps to Wellknown SIDs
*****************************************************************/  

BOOL sid_to_gid(const DOM_SID *psid, gid_t *pgid)
{
	uint32 rid;
	GROUP_MAP map;
	union unid_t id;
	enum SID_NAME_USE type;
	uid_t uid;

	if (fetch_gid_from_cache(pgid, psid))
		return True;

	if (fetch_uid_from_cache(&uid, psid))
		return False;

	if (sid_peek_check_rid(&global_sid_Unix_Groups, psid, &rid)) {
		gid_t gid = rid;
		*pgid = gid;
		goto done;
	}

	if ((sid_check_is_in_builtin(psid) ||
	     sid_check_is_in_wellknown_domain(psid))) {
		BOOL ret;

		become_root_uid_only();
		ret = pdb_getgrsid(&map, *psid);
		unbecome_root_uid_only();

		if (ret) {
			*pgid = map.gid;
			goto done;
		}
		return False;
	}

	if (sid_peek_check_rid(get_global_sam_sid(), psid, &rid)) {
		BOOL ret;

		become_root_uid_only();
		ret = pdb_sid_to_id(psid, &id, &type);
		unbecome_root_uid_only();

		if (ret) {
			if ((type != SID_NAME_DOM_GRP) &&
			    (type != SID_NAME_ALIAS)) {
				DEBUG(5, ("sid %s is a %s, expected a group\n",
					  sid_string_static(psid),
					  sid_type_lookup(type)));
				return False;
			}
			*pgid = id.gid;
			goto done;
		}

		/* This was ours, but it was not mapped.  Fail */

		return False;
	}
	
	if (!winbind_lookup_sid(NULL, psid, NULL, NULL, &type)) {
		DEBUG(11,("sid_to_gid: no one knows the SID %s (tried local, "
			  "then winbind)\n", sid_string_static(psid)));
		
		return False;
	}

	/* winbindd knows it; Ensure this is a group sid */

	if ((type != SID_NAME_DOM_GRP) && (type != SID_NAME_ALIAS) &&
	    (type != SID_NAME_WKN_GRP)) {
		DEBUG(10,("sid_to_gid: winbind lookup succeeded but SID is "
			  "a %s\n", sid_type_lookup(type)));
		return False;
	}
	
	/* winbindd knows it and it is a type of group; sid_to_gid must succeed
	   or we are dead in the water */

	if ( !winbind_sid_to_gid(pgid, psid) ) {
		DEBUG(10,("sid_to_gid: winbind failed to allocate a new gid "
			  "for sid %s\n", sid_string_static(psid)));
		return False;
	}

 done:
	DEBUG(10,("sid_to_gid: %s -> %u\n", sid_string_static(psid),
		  (unsigned int)*pgid ));

	store_gid_sid_cache(psid, *pgid);
	
	return True;
}

