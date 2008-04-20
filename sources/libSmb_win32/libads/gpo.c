/* 
 *  Unix SMB/CIFS implementation.
 *  Group Policy Object Support
 *  Copyright (C) Guenther Deschner 2005
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
 */

#include "includes.h"

ADS_STATUS ads_parse_gp_ext(TALLOC_CTX *mem_ctx,
			    const char *extension_raw,
			    struct GP_EXT *gp_ext)
{
	char **ext_list;
	char **ext_strings;
	int i;

	DEBUG(20,("ads_parse_gp_ext: %s\n", extension_raw));

	ext_list = str_list_make_talloc(mem_ctx, extension_raw, "]");
	if (ext_list == NULL) {
		goto parse_error;
	}

	for (i = 0; ext_list[i] != NULL; i++) {
		/* no op */
	}

	gp_ext->num_exts = i;
	
	gp_ext->extensions = TALLOC_ZERO_ARRAY(mem_ctx, char *, gp_ext->num_exts);
	gp_ext->extensions_guid = TALLOC_ZERO_ARRAY(mem_ctx, char *, gp_ext->num_exts);
	gp_ext->snapins = TALLOC_ZERO_ARRAY(mem_ctx, char *, gp_ext->num_exts);
	gp_ext->snapins_guid = TALLOC_ZERO_ARRAY(mem_ctx, char *, gp_ext->num_exts);

	gp_ext->gp_extension = talloc_strdup(mem_ctx, extension_raw);

	if (gp_ext->extensions == NULL || gp_ext->extensions_guid == NULL || 
	    gp_ext->snapins == NULL || gp_ext->snapins_guid == NULL || 
	    gp_ext->gp_extension == NULL) {
		goto parse_error;
	}

	for (i = 0; ext_list[i] != NULL; i++) {

		int k;
		char *p, *q;
		
		DEBUGADD(10,("extension #%d\n", i));

		p = ext_list[i];

		if (p[0] == '[') {
			p++;
		}

		ext_strings = str_list_make_talloc(mem_ctx, p, "}");
		if (ext_strings == NULL) {
			goto parse_error;
		}

		for (k = 0; ext_strings[k] != NULL; k++) {
			/* no op */
		}

		q = ext_strings[0];

		if (q[0] == '{') {
			q++;
		}

		gp_ext->extensions[i] = talloc_strdup(mem_ctx, cse_gpo_guid_string_to_name(q));
		gp_ext->extensions_guid[i] = talloc_strdup(mem_ctx, q);

		/* we might have no name for the guid */
		if (gp_ext->extensions_guid[i] == NULL) {
			goto parse_error;
		}
		
		for (k = 1; ext_strings[k] != NULL; k++) {

			char *m = ext_strings[k];

			if (m[0] == '{') {
				m++;
			}

			/* FIXME: theoretically there could be more than one snapin per extension */
			gp_ext->snapins[i] = talloc_strdup(mem_ctx, cse_snapin_gpo_guid_string_to_name(m));
			gp_ext->snapins_guid[i] = talloc_strdup(mem_ctx, m);

			/* we might have no name for the guid */
			if (gp_ext->snapins_guid[i] == NULL) {
				goto parse_error;
			}
		}
	}

	if (ext_list) {
		str_list_free_talloc(mem_ctx, &ext_list); 
	}
	if (ext_strings) {
		str_list_free_talloc(mem_ctx, &ext_strings); 
	}

	return ADS_ERROR(LDAP_SUCCESS);

parse_error:
	if (ext_list) {
		str_list_free_talloc(mem_ctx, &ext_list); 
	}
	if (ext_strings) {
		str_list_free_talloc(mem_ctx, &ext_strings); 
	}

	return ADS_ERROR(LDAP_NO_MEMORY);
}

ADS_STATUS ads_parse_gplink(TALLOC_CTX *mem_ctx, 
			    const char *gp_link_raw,
			    uint32 options,
			    struct GP_LINK *gp_link)
{
	char **link_list;
	int i;
	
	DEBUG(10,("ads_parse_gplink: gPLink: %s\n", gp_link_raw));

	link_list = str_list_make_talloc(mem_ctx, gp_link_raw, "]");
	if (link_list == NULL) {
		goto parse_error;
	}

	for (i = 0; link_list[i] != NULL; i++) {
		/* no op */
	}

	gp_link->gp_opts = options;
	gp_link->num_links = i;
	
	gp_link->link_names = TALLOC_ZERO_ARRAY(mem_ctx, char *, gp_link->num_links);
	gp_link->link_opts = TALLOC_ZERO_ARRAY(mem_ctx, uint32, gp_link->num_links);
	
	gp_link->gp_link = talloc_strdup(mem_ctx, gp_link_raw);

	if (gp_link->link_names == NULL || gp_link->link_opts == NULL || gp_link->gp_link == NULL) {
		goto parse_error;
	}

	for (i = 0; link_list[i] != NULL; i++) {

		char *p, *q;

		DEBUGADD(10,("ads_parse_gplink: processing link #%d\n", i));

		q = link_list[i];
		if (q[0] == '[') {
			q++;
		};

		p = strchr(q, ';');
		
		if (p == NULL) {
			goto parse_error;
		}

		gp_link->link_names[i] = talloc_strdup(mem_ctx, q);
		if (gp_link->link_names[i] == NULL) {
			goto parse_error;
		}
		gp_link->link_names[i][PTR_DIFF(p, q)] = 0;

		gp_link->link_opts[i] = atoi(p + 1);

		DEBUGADD(10,("ads_parse_gplink: link: %s\n", gp_link->link_names[i]));
		DEBUGADD(10,("ads_parse_gplink: opt: %d\n", gp_link->link_opts[i]));

	}

	if (link_list) {
		str_list_free_talloc(mem_ctx, &link_list);
	}

	return ADS_ERROR(LDAP_SUCCESS);

parse_error:
	if (link_list) {
		str_list_free_talloc(mem_ctx, &link_list);
	}

	return ADS_ERROR(LDAP_NO_MEMORY);
}

ADS_STATUS ads_get_gpo_link(ADS_STRUCT *ads,
			    TALLOC_CTX *mem_ctx,
			    const char *link_dn,
			    struct GP_LINK *gp_link_struct)
{
	ADS_STATUS status;
	const char *attrs[] = {"gPLink", "gPOptions", NULL};
	void *res = NULL;
	const char *gp_link;
	uint32 gp_options;

	ZERO_STRUCTP(gp_link_struct);

	status = ads_search_dn(ads, &res, link_dn, attrs);
	if (!ADS_ERR_OK(status)) {
		DEBUG(10,("ads_get_gpo_link: search failed with %s\n", ads_errstr(status)));
		return status;
	}

	if (ads_count_replies(ads, res) != 1) {
		DEBUG(10,("ads_get_gpo_link: no result\n"));
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	gp_link = ads_pull_string(ads, mem_ctx, res, "gPLink"); 
	if (gp_link == NULL) {
		DEBUG(10,("ads_get_gpo_link: no 'gPLink' attribute found\n"));
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_SUCH_ATTRIBUTE);	
	}

	if (!ads_pull_uint32(ads, res, "gPOptions", &gp_options)) {
		DEBUG(10,("ads_get_gpo_link: no 'gPOptions' attribute found\n"));
		gp_options = 0;
	}

	ads_msgfree(ads, res);

	return ads_parse_gplink(mem_ctx, gp_link, gp_options, gp_link_struct); 
}

ADS_STATUS ads_add_gpo_link(ADS_STRUCT *ads, 
			    TALLOC_CTX *mem_ctx, 
			    const char *link_dn, 
			    const char *gpo_dn, 
			    uint32 gpo_opt)
{
	ADS_STATUS status;
	const char *attrs[] = {"gPLink", NULL};
	void *res = NULL;
	const char *gp_link, *gp_link_new;
	ADS_MODLIST mods;


	/* although ADS allows to set anything here, we better check here if
	 * the gpo_dn is sane */

	if (!strnequal(gpo_dn, "LDAP://CN={", strlen("LDAP://CN={")) != 0) {
		return ADS_ERROR(LDAP_INVALID_DN_SYNTAX);	
	}

	status = ads_search_dn(ads, &res, link_dn, attrs);
	if (!ADS_ERR_OK(status)) {
		DEBUG(10,("ads_add_gpo_link: search failed with %s\n", ads_errstr(status)));
		return status;
	}

	if (ads_count_replies(ads, res) != 1) {
		DEBUG(10,("ads_add_gpo_link: no result\n"));
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	gp_link = ads_pull_string(ads, mem_ctx, res, "gPLink"); 
	if (gp_link == NULL) {
		gp_link_new = talloc_asprintf(mem_ctx, "[%s;%d]", gpo_dn, gpo_opt);
	} else {
		gp_link_new = talloc_asprintf(mem_ctx, "%s[%s;%d]", gp_link, gpo_dn, gpo_opt);
	}

	if (gp_link_new == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	mods = ads_init_mods(mem_ctx);
	if (mods == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	status = ads_mod_str(mem_ctx, &mods, "gPLink", gp_link_new);
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	return ads_gen_mod(ads, link_dn, mods); 
}

/* untested & broken */
ADS_STATUS ads_delete_gpo_link(ADS_STRUCT *ads, 
			       TALLOC_CTX *mem_ctx, 
			       const char *link_dn, 
			       const char *gpo_dn)
{
	ADS_STATUS status;
	const char *attrs[] = {"gPLink", NULL};
	void *res = NULL;
	const char *gp_link, *gp_link_new = NULL;
	ADS_MODLIST mods;

	/* check for a sane gpo_dn */
	if (gpo_dn[0] != '[') {
		DEBUG(10,("ads_delete_gpo_link: first char not: [\n"));
		return ADS_ERROR(LDAP_INVALID_DN_SYNTAX);
	}
		
	if (gpo_dn[strlen(gpo_dn)] != ']') {
		DEBUG(10,("ads_delete_gpo_link: last char not: ]\n"));
		return ADS_ERROR(LDAP_INVALID_DN_SYNTAX);
	}

	status = ads_search_dn(ads, &res, link_dn, attrs);
	if (!ADS_ERR_OK(status)) {
		DEBUG(10,("ads_delete_gpo_link: search failed with %s\n", ads_errstr(status)));
		return status;
	}

	if (ads_count_replies(ads, res) != 1) {
		DEBUG(10,("ads_delete_gpo_link: no result\n"));
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	gp_link = ads_pull_string(ads, mem_ctx, res, "gPLink"); 
	if (gp_link == NULL) {
		return ADS_ERROR(LDAP_NO_SUCH_ATTRIBUTE);
	}

	/* find link to delete */
//	gp_link_new = talloc_asprintf(mem_ctx, "%s[%s;%d]", gp_link, gpo_dn, gpo_opt);

	if (gp_link_new == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	mods = ads_init_mods(mem_ctx);
	if (mods == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	status = ads_mod_str(mem_ctx, &mods, "gPLink", gp_link_new);
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	return ads_gen_mod(ads, link_dn, mods); 
}

ADS_STATUS ads_parse_gpo(ADS_STRUCT *ads,
			 TALLOC_CTX *mem_ctx,
			 void *res,
			 const char *gpo_dn,
			 struct GROUP_POLICY_OBJECT *gpo)
{
	ZERO_STRUCTP(gpo);

	if (res == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	if (gpo_dn) {
		gpo->ds_path = talloc_strdup(mem_ctx, gpo_dn);
	} else {
		gpo->ds_path = ads_get_dn(ads, res);
	}
	if (gpo->ds_path == NULL) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	if (!ads_pull_uint32(ads, res, "versionNumber", &gpo->version)) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	/* split here for convenience */
	gpo->version_user = gpo->version >> 16;
	gpo->version_machine = gpo->version & 0xffff;

	/* sure ??? */
	if (!ads_pull_uint32(ads, res, "flags", &gpo->options)) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	gpo->file_sys_path = ads_pull_string(ads, mem_ctx, res, "gPCFileSysPath");
	if (gpo->file_sys_path == NULL) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	gpo->display_name = ads_pull_string(ads, mem_ctx, res, "displayName");
	if (gpo->display_name == NULL) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	gpo->name = ads_pull_string(ads, mem_ctx, res, "name");
	if (gpo->name == NULL) {
		ads_msgfree(ads, res);
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	/* ???, this is optional to have and what does it depend on, the 'flags' ?) */
	gpo->machine_extensions = ads_pull_string(ads, mem_ctx, res, "gPCMachineExtensionNames");
	gpo->user_extensions = ads_pull_string(ads, mem_ctx, res, "gPCUserExtensionNames");

	ads_msgfree(ads, res);

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS ads_get_gpo(ADS_STRUCT *ads,
		       TALLOC_CTX *mem_ctx,
		       const char *gpo_dn,
		       const char *display_name,
		       const char *guid_name,
		       struct GROUP_POLICY_OBJECT *gpo)
{
	ADS_STATUS status;
	void *res = NULL;
	char *dn;
	const char *filter;
	const char *attrs[] = { "cn", "displayName", "flags", "gPCFileSysPath", 
				"gPCFunctionalityVersion", "gPCMachineExtensionNames", 
				"gPCUserExtensionNames", "gPCWQLFilter", "name", 
				"versionNumber", NULL};

	ZERO_STRUCTP(gpo);

	if (!gpo_dn && !display_name && !guid_name) {
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	if (gpo_dn) {
	
		if (strnequal(gpo_dn, "LDAP://", strlen("LDAP://")) != 0) {
			gpo_dn = gpo_dn + strlen("LDAP://");
		}

		status = ads_search_dn(ads, &res, gpo_dn, attrs);
		
	} else if (display_name || guid_name) {

		filter = talloc_asprintf(mem_ctx, 
					 "(&(objectclass=groupPolicyContainer)(%s=%s))", 
					 display_name ? "displayName" : "name",
					 display_name ? display_name : guid_name);
		if (filter == NULL) {
			return ADS_ERROR(LDAP_NO_MEMORY);
		}

		status = ads_do_search_all(ads, ads->config.bind_path,
					   LDAP_SCOPE_SUBTREE, filter, 
					   attrs, &res);
	}

	if (!ADS_ERR_OK(status)) {
		DEBUG(10,("ads_get_gpo: search failed with %s\n", ads_errstr(status)));
		return status;
	}

	if (ads_count_replies(ads, res) != 1) {
		DEBUG(10,("ads_get_gpo: no result\n"));
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	dn = ads_get_dn(ads, res);
	if (dn == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}
	
	status = ads_parse_gpo(ads, mem_ctx, res, dn, gpo);
	
	ads_memfree(ads, dn);

	return status;
}

ADS_STATUS add_gplink_to_gpo_list(ADS_STRUCT *ads,
				  TALLOC_CTX *mem_ctx, 
				  struct GROUP_POLICY_OBJECT **gpo_list,
				  const char *link_dn,
				  struct GP_LINK *gp_link,
				  enum GPO_LINK_TYPE link_type,
				  BOOL only_add_forced_gpos)
{
	ADS_STATUS status;
	int i;

	for (i = 0; i < gp_link->num_links; i++) {

		struct GROUP_POLICY_OBJECT *new_gpo = NULL;

		if (gp_link->link_opts[i] & GPO_LINK_OPT_DISABLED) {
			DEBUG(10,("skipping disabled GPO\n"));
			continue;
		}

		if (only_add_forced_gpos) {
		
			if (! (gp_link->link_opts[i] & GPO_LINK_OPT_ENFORCED)) {
				DEBUG(10,("skipping nonenforced GPO link because GPOPTIONS_BLOCK_INHERITANCE has been set\n"));
				continue;
			} else {
				DEBUG(10,("adding enforced GPO link although the GPOPTIONS_BLOCK_INHERITANCE has been set\n"));
			}
		}

		new_gpo = TALLOC_P(mem_ctx, struct GROUP_POLICY_OBJECT);
		if (new_gpo == NULL) {
			return ADS_ERROR(LDAP_NO_MEMORY);
		}

		ZERO_STRUCTP(new_gpo);

		status = ads_get_gpo(ads, mem_ctx, gp_link->link_names[i], NULL, NULL, new_gpo);
		if (!ADS_ERR_OK(status)) {
			return status;
		}

		new_gpo->link = link_dn;
		new_gpo->link_type = link_type; 

		DLIST_ADD(*gpo_list, new_gpo);

		DEBUG(10,("add_gplink_to_gplist: added GPLINK #%d %s to GPO list\n", 
			i, gp_link->link_names[i]));
	}

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS ads_get_gpo_list(ADS_STRUCT *ads, 
			    TALLOC_CTX *mem_ctx, 
			    const char *dn,
			    uint32 flags,
			    struct GROUP_POLICY_OBJECT **gpo_list)
{
	/* (L)ocal (S)ite (D)omain (O)rganizational(U)nit */
	
	ADS_STATUS status;
	struct GP_LINK gp_link;
	const char *parent_dn, *site_dn, *tmp_dn;
	BOOL add_only_forced_gpos = False;

	ZERO_STRUCTP(gpo_list);

	DEBUG(10,("ads_get_gpo_list: getting GPO list for [%s]\n", dn));

	/* (L)ocal */
	/* not yet... */
	
	/* (S)ite */

	/* are site GPOs valid for users as well ??? */
	if (flags & GPO_LIST_FLAG_MACHINE) {

		status = ads_site_dn_for_machine(ads, mem_ctx, ads->config.ldap_server_name, &site_dn);
		if (!ADS_ERR_OK(status)) {
			return status;
		}

		DEBUG(10,("ads_get_gpo_list: query SITE: [%s] for GPOs\n", site_dn));

		status = ads_get_gpo_link(ads, mem_ctx, site_dn, &gp_link);
		if (ADS_ERR_OK(status)) {
		
			if (DEBUGLEVEL >= 100) {
				dump_gplink(ads, mem_ctx, &gp_link);
			}
			
			status = add_gplink_to_gpo_list(ads, mem_ctx, gpo_list, 
							site_dn, &gp_link, GP_LINK_SITE, 
							add_only_forced_gpos);
			if (!ADS_ERR_OK(status)) {
				return status;
			}
	
			if (flags & GPO_LIST_FLAG_SITEONLY) {
				return ADS_ERROR(LDAP_SUCCESS);
			}
	
			/* inheritance can't be blocked at the site level */
		}
	}

	tmp_dn = dn;

	while ( (parent_dn = ads_parent_dn(tmp_dn)) && 
		(!strequal(parent_dn, ads_parent_dn(ads->config.bind_path))) ) {

		/* (D)omain */

		/* An account can just be a member of one domain */
		if (strncmp(parent_dn, "DC=", strlen("DC=")) == 0) {

			DEBUG(10,("ads_get_gpo_list: query DC: [%s] for GPOs\n", parent_dn));

			status = ads_get_gpo_link(ads, mem_ctx, parent_dn, &gp_link);
			if (ADS_ERR_OK(status)) {
				
				if (DEBUGLEVEL >= 100) {
					dump_gplink(ads, mem_ctx, &gp_link);
				}

				/* block inheritance from now on */
				if (gp_link.gp_opts & GPOPTIONS_BLOCK_INHERITANCE) {
					add_only_forced_gpos = True;
				}

				status = add_gplink_to_gpo_list(ads, mem_ctx, 
								gpo_list, parent_dn, 
								&gp_link, GP_LINK_DOMAIN, 
								add_only_forced_gpos);
				if (!ADS_ERR_OK(status)) {
					return status;
				}
			}
		}

		tmp_dn = parent_dn;
	}

	/* reset dn again */
	tmp_dn = dn;
	
	while ( (parent_dn = ads_parent_dn(tmp_dn)) && 
		(!strequal(parent_dn, ads_parent_dn(ads->config.bind_path))) ) {


		/* (O)rganizational(U)nit */

		/* An account can be a member of more OUs */
		if (strncmp(parent_dn, "OU=", strlen("OU=")) == 0) {
		
			DEBUG(10,("ads_get_gpo_list: query OU: [%s] for GPOs\n", parent_dn));

			status = ads_get_gpo_link(ads, mem_ctx, parent_dn, &gp_link);
			if (ADS_ERR_OK(status)) {

				if (DEBUGLEVEL >= 100) {
					dump_gplink(ads, mem_ctx, &gp_link);
				}

				/* block inheritance from now on */
				if (gp_link.gp_opts & GPOPTIONS_BLOCK_INHERITANCE) {
					add_only_forced_gpos = True;
				}

				status = add_gplink_to_gpo_list(ads, mem_ctx, 
								gpo_list, parent_dn, 
								&gp_link, GP_LINK_OU, 
								add_only_forced_gpos);
				if (!ADS_ERR_OK(status)) {
					return status;
				}
			}
		}

		tmp_dn = parent_dn;

	};

	return ADS_ERROR(LDAP_SUCCESS);
}
