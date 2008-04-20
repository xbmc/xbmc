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

#define DEFAULT_DOMAIN_POLICY "Default Domain Policy"
#define DEFAULT_DOMAIN_CONTROLLERS_POLICY "Default Domain Controllers Policy"

/* should we store a parsed guid ? UUID_FLAT guid; */
struct gpo_table {
	const char *name;
	const char *guid_string;
};

struct snapin_table {
	const char *name;
	const char *guid_string;
	ADS_STATUS (*snapin_fn)(ADS_STRUCT *, TALLOC_CTX *mem_ctx, const char *, const char *);
};

static struct gpo_table gpo_default_policy[] = {
	{ DEFAULT_DOMAIN_POLICY, 
		"31B2F340-016D-11D2-945F-00C04FB984F9" },
	{ DEFAULT_DOMAIN_CONTROLLERS_POLICY, 
		"6AC1786C-016F-11D2-945F-00C04fB984F9" },
	{ NULL, NULL }
};


/* the following is seen in gPCMachineExtensionNames or gPCUserExtensionNames */

static struct gpo_table gpo_cse_extensions[] = {
	{ "Administrative Templates Extension", 
		"35378EAC-683F-11D2-A89A-00C04FBBCFA2" }, /* Registry Policy ? */
	{ "Microsoft Disc Quota", 
		"3610EDA5-77EF-11D2-8DC5-00C04FA31A66" },
	{ "EFS recovery", 
		"B1BE8D72-6EAC-11D2-A4EA-00C04F79F83A" },
	{ "Folder Redirection", 
		"25537BA6-77A8-11D2-9B6C-0000F8080861" },
	{ "IP Security", 
		"E437BC1C-AA7D-11D2-A382-00C04F991E27" },
	{ "Internet Explorer Branding", 
		"A2E30F80-D7DE-11d2-BBDE-00C04F86AE3B" },
	{ "QoS Packet Scheduler", 
		"426031c0-0b47-4852-b0ca-ac3d37bfcb39" },
	{ "Scripts", 
		"42B5FAAE-6536-11D2-AE5A-0000F87571E3" },
	{ "Security", 
		"827D319E-6EAC-11D2-A4EA-00C04F79F83A" },
	{ "Software Installation", 
		"C6DC5466-785A-11D2-84D0-00C04FB169F7" },
	{ "Wireless Group Policy", 
		"0ACDD40C-75AC-BAA0-BF6DE7E7FE63" },
	{ NULL, NULL }
};

/* guess work */
static struct snapin_table gpo_cse_snapin_extensions[] = {
	{ "Administrative Templates", 
		"0F6B957D-509E-11D1-A7CC-0000F87571E3", gpo_snapin_handler_none },
	{ "Certificates", 
		"53D6AB1D-2488-11D1-A28C-00C04FB94F17", gpo_snapin_handler_none },
	{ "EFS recovery policy processing", 
		"B1BE8D72-6EAC-11D2-A4EA-00C04F79F83A", gpo_snapin_handler_none },
	{ "Folder Redirection policy processing", 
		"25537BA6-77A8-11D2-9B6C-0000F8080861", gpo_snapin_handler_none },
	{ "Folder Redirection", 
		"88E729D6-BDC1-11D1-BD2A-00C04FB9603F", gpo_snapin_handler_none },
	{ "Registry policy processing", 
		"35378EAC-683F-11D2-A89A-00C04FBBCFA2", gpo_snapin_handler_none },
	{ "Remote Installation Services", 
		"3060E8CE-7020-11D2-842D-00C04FA372D4", gpo_snapin_handler_none },
	{ "Security Settings", 
		"803E14A0-B4FB-11D0-A0D0-00A0C90F574B", gpo_snapin_handler_security_settings },
	{ "Security policy processing", 
		"827D319E-6EAC-11D2-A4EA-00C04F79F83A", gpo_snapin_handler_security_settings },
	{ "unknown", 
		"3060E8D0-7020-11D2-842D-00C04FA372D4", gpo_snapin_handler_none },
	{ "unknown2", 
		"53D6AB1B-2488-11D1-A28C-00C04FB94F17", gpo_snapin_handler_none },
	{ NULL, NULL, NULL }
};

static const char *name_to_guid_string(const char *name, struct gpo_table *table)
{
	int i;

	for (i = 0; table[i].name; i++) {
		if (strequal(name, table[i].name)) {
			return table[i].guid_string;
		}
	}
	
	return NULL;
}

static const char *guid_string_to_name(const char *guid_string, struct gpo_table *table)
{
	int i;

	for (i = 0; table[i].guid_string; i++) {
		if (strequal(guid_string, table[i].guid_string)) {
			return table[i].name;
		}
	}
	
	return NULL;
}

static const char *default_gpo_name_to_guid_string(const char *name)
{
	return name_to_guid_string(name, gpo_default_policy);
}

static const char *default_gpo_guid_string_to_name(const char *guid)
{
	return guid_string_to_name(guid, gpo_default_policy);
}

const char *cse_gpo_guid_string_to_name(const char *guid)
{
	return guid_string_to_name(guid, gpo_cse_extensions);
}

static const char *cse_gpo_name_to_guid_string(const char *name)
{
	return name_to_guid_string(name, gpo_cse_extensions);
}

const char *cse_snapin_gpo_guid_string_to_name(const char *guid)
{
	return guid_string_to_name(guid, gpo_cse_snapin_extensions);
}

void dump_gp_ext(struct GP_EXT *gp_ext)
{
	int lvl = 10;
	int i;

	if (gp_ext == NULL) {
		return;
	}

	DEBUG(lvl,("---------------------\n\n"));
	DEBUGADD(lvl,("name:\t\t\t%s\n", gp_ext->gp_extension));

	for (i=0; i< gp_ext->num_exts; i++) {

		DEBUGADD(lvl,("extension:\t\t\t%s\n", gp_ext->extensions_guid[i]));
		DEBUGADD(lvl,("extension (name):\t\t\t%s\n", gp_ext->extensions[i]));
		
		DEBUGADD(lvl,("snapin:\t\t\t%s\n", gp_ext->snapins_guid[i]));
		DEBUGADD(lvl,("snapin (name):\t\t\t%s\n", gp_ext->snapins[i]));
	}
}

void dump_gpo(TALLOC_CTX *mem_ctx, struct GROUP_POLICY_OBJECT *gpo) 
{
	int lvl = 1;

	if (gpo == NULL) {
		return;
	}

	DEBUG(lvl,("---------------------\n\n"));

	DEBUGADD(lvl,("name:\t\t\t%s\n", gpo->name));
	DEBUGADD(lvl,("displayname:\t\t%s\n", gpo->display_name));
	DEBUGADD(lvl,("version:\t\t%d (0x%08x)\n", gpo->version, gpo->version));
	DEBUGADD(lvl,("version_user:\t\t%d (0x%04x)\n", gpo->version_user, gpo->version_user));
	DEBUGADD(lvl,("version_machine:\t%d (0x%04x)\n", gpo->version_machine, gpo->version_machine));
	DEBUGADD(lvl,("filesyspath:\t\t%s\n", gpo->file_sys_path));
	DEBUGADD(lvl,("dspath:\t\t%s\n", gpo->ds_path));

	DEBUGADD(lvl,("options:\t\t%d ", gpo->options));
	if (gpo->options & GPFLAGS_USER_SETTINGS_DISABLED) {
		DEBUGADD(lvl,("GPFLAGS_USER_SETTINGS_DISABLED ")); 
	}
	if (gpo->options & GPFLAGS_MACHINE_SETTINGS_DISABLED) {
		DEBUGADD(lvl,("GPFLAGS_MACHINE_SETTINGS_DISABLED")); 
	}
	DEBUGADD(lvl,("\n"));

	DEBUGADD(lvl,("link:\t\t\t%s\n", gpo->link));
	DEBUGADD(lvl,("link_type:\t\t%d ", gpo->link_type));
	switch (gpo->link_type) {
	case GP_LINK_UNKOWN:
		DEBUGADD(lvl,("GP_LINK_UNKOWN\n"));
		break;
	case GP_LINK_OU:
		DEBUGADD(lvl,("GP_LINK_OU\n"));
		break;
	case GP_LINK_DOMAIN:
		DEBUGADD(lvl,("GP_LINK_DOMAIN\n"));
		break;
	case GP_LINK_SITE:
		DEBUGADD(lvl,("GP_LINK_SITE\n"));
		break;
	case GP_LINK_MACHINE:
		DEBUGADD(lvl,("GP_LINK_MACHINE\n"));
		break;
	default:
		break;
	}

	if (gpo->machine_extensions) {

		struct GP_EXT gp_ext;
		ADS_STATUS status;

		DEBUGADD(lvl,("machine_extensions:\t%s\n", gpo->machine_extensions));

		status = ads_parse_gp_ext(mem_ctx, gpo->machine_extensions, &gp_ext);
		if (!ADS_ERR_OK(status)) {
			return;
		}
		dump_gp_ext(&gp_ext);
	}
	
	if (gpo->user_extensions) {
	
		struct GP_EXT gp_ext;
		ADS_STATUS status;
		
		DEBUGADD(lvl,("user_extensions:\t%s\n", gpo->user_extensions));

		status = ads_parse_gp_ext(mem_ctx, gpo->user_extensions, &gp_ext);
		if (!ADS_ERR_OK(status)) {
			return;
		}
		dump_gp_ext(&gp_ext);
	}
};

void dump_gplink(ADS_STRUCT *ads, TALLOC_CTX *mem_ctx, struct GP_LINK *gp_link)
{
	ADS_STATUS status;
	int i;
	int lvl = 10;

	if (gp_link == NULL) {
		return;
	}

	DEBUG(lvl,("---------------------\n\n"));

	DEBUGADD(lvl,("gplink: %s\n", gp_link->gp_link));
	DEBUGADD(lvl,("gpopts: %d ", gp_link->gp_opts));
	switch (gp_link->gp_opts) {
	case GPOPTIONS_INHERIT:
		DEBUGADD(lvl,("GPOPTIONS_INHERIT\n"));
		break;
	case GPOPTIONS_BLOCK_INHERITANCE:
		DEBUGADD(lvl,("GPOPTIONS_BLOCK_INHERITANCE\n"));
		break;
	default:
		break;
	}

	DEBUGADD(lvl,("num links: %d\n", gp_link->num_links));

	for (i = 0; i < gp_link->num_links; i++) {
	
		DEBUGADD(lvl,("---------------------\n\n"));
	
		DEBUGADD(lvl,("link: #%d\n", i + 1));
		DEBUGADD(lvl,("name: %s\n", gp_link->link_names[i]));

		DEBUGADD(lvl,("opt: %d ", gp_link->link_opts[i]));
		if (gp_link->link_opts[i] & GPO_LINK_OPT_ENFORCED) {
			DEBUGADD(lvl,("GPO_LINK_OPT_ENFORCED "));
		}
		if (gp_link->link_opts[i] & GPO_LINK_OPT_DISABLED) {
			DEBUGADD(lvl,("GPO_LINK_OPT_DISABLED"));
		}
		DEBUGADD(lvl,("\n"));

		if (ads != NULL && mem_ctx != NULL) {

			struct GROUP_POLICY_OBJECT gpo;

			status = ads_get_gpo(ads, mem_ctx, gp_link->link_names[i], NULL, NULL, &gpo);
			if (!ADS_ERR_OK(status)) {
				DEBUG(lvl,("get gpo for %s failed: %s\n", gp_link->link_names[i], ads_errstr(status)));
				return;
			}
			dump_gpo(mem_ctx, &gpo);
		}
	}
}

ADS_STATUS process_extension_with_snapin(ADS_STRUCT *ads,
					 TALLOC_CTX *mem_ctx,
					 const char *extension_guid,
					 const char *snapin_guid)
{
	int i;

	for (i=0; gpo_cse_snapin_extensions[i].guid_string; i++) {
	
		if (strcmp(gpo_cse_snapin_extensions[i].guid_string, snapin_guid) == 0) {
		
			return gpo_cse_snapin_extensions[i].snapin_fn(ads, mem_ctx, 
								      extension_guid, snapin_guid);
		}
	}

	DEBUG(10,("process_extension_with_snapin: no snapin handler for extension %s (%s) found\n", 
		extension_guid, snapin_guid));

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS gpo_process_a_gpo(ADS_STRUCT *ads,
			     TALLOC_CTX *mem_ctx,
			     struct GROUP_POLICY_OBJECT *gpo,
			     const char *extension_guid,
			     uint32 flags)
{
	ADS_STATUS status;
	struct GP_EXT gp_ext;
	int i;
	
	if (flags & GPO_LIST_FLAG_MACHINE) {

		if (gpo->machine_extensions) {

			status = ads_parse_gp_ext(mem_ctx, gpo->machine_extensions, &gp_ext);

			if (!ADS_ERR_OK(status)) {
				return status;
			}

		} else {
			/* nothing to apply */
			return ADS_ERROR(LDAP_SUCCESS);
		}
	
	} else {

		if (gpo->user_extensions) {
		
			status = ads_parse_gp_ext(mem_ctx, gpo->user_extensions, &gp_ext);

			if (!ADS_ERR_OK(status)) {
				return status;
			}
		} else {
			/* nothing to apply */
			return ADS_ERROR(LDAP_SUCCESS);
		}
	}

	for (i=0; i<gp_ext.num_exts; i++) {

		if (extension_guid && !strequal(extension_guid, gp_ext.extensions_guid[i])) {
			continue;
		}

		status = process_extension_with_snapin(ads, mem_ctx, gp_ext.extensions_guid[i], 
						       gp_ext.snapins_guid[i]);
		if (!ADS_ERR_OK(status)) {
			return status;
		}
	}

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS gpo_process_gpo_list(ADS_STRUCT *ads,
				TALLOC_CTX *mem_ctx,
				struct GROUP_POLICY_OBJECT **gpo_list,
				const char *extensions_guid,
				uint32 flags)
{
	ADS_STATUS status;
	struct GROUP_POLICY_OBJECT *gpo = *gpo_list;

	for (gpo = *gpo_list; gpo; gpo = gpo->next) {
	
		status = gpo_process_a_gpo(ads, mem_ctx, gpo, 
					   extensions_guid, flags);
	
		if (!ADS_ERR_OK(status)) {
			return status;
		}

	}

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS gpo_snapin_handler_none(ADS_STRUCT *ads, 
				   TALLOC_CTX *mem_ctx, 
				   const char *extension_guid, 
				   const char *snapin_guid)
{
	DEBUG(10,("gpo_snapin_handler_none\n"));

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS gpo_snapin_handler_security_settings(ADS_STRUCT *ads, 
						TALLOC_CTX *mem_ctx, 
						const char *extension_guid, 
						const char *snapin_guid)
{
	DEBUG(10,("gpo_snapin_handler_security_settings\n"));

	return ADS_ERROR(LDAP_SUCCESS);
}

ADS_STATUS gpo_lockout_policy(ADS_STRUCT *ads,
			      TALLOC_CTX *mem_ctx,
			      const char *hostname,
			      SAM_UNK_INFO_12 *lockout_policy)
{
	return ADS_ERROR_NT(NT_STATUS_NOT_IMPLEMENTED);
}

ADS_STATUS gpo_password_policy(ADS_STRUCT *ads,
			       TALLOC_CTX *mem_ctx,
			       const char *hostname,
			       SAM_UNK_INFO_1 *password_policy)
{
	ADS_STATUS status;
	struct GROUP_POLICY_OBJECT *gpo_list;
	const char *attrs[] = {"distinguishedName", "userAccountControl", NULL};
	char *filter, *dn;
	void *res = NULL;
	uint32 uac;

	return ADS_ERROR_NT(NT_STATUS_NOT_IMPLEMENTED);

	filter = talloc_asprintf(mem_ctx, "(&(objectclass=user)(sAMAccountName=%s))", hostname);
	if (filter == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	status = ads_do_search_all(ads, ads->config.bind_path,
				   LDAP_SCOPE_SUBTREE,
				   filter, attrs, &res);
	
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	if (ads_count_replies(ads, res) != 1) {
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	dn = ads_get_dn(ads, res);
	if (dn == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	if (!ads_pull_uint32(ads, res, "userAccountControl", &uac)) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	if (!(uac & UF_WORKSTATION_TRUST_ACCOUNT)) {
		return ADS_ERROR(LDAP_NO_SUCH_OBJECT);
	}

	status = ads_get_gpo_list(ads, mem_ctx, dn, GPO_LIST_FLAG_MACHINE, &gpo_list);
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	status = gpo_process_gpo_list(ads, mem_ctx, &gpo_list, 
				      cse_gpo_name_to_guid_string("Security"), 
				      GPO_LIST_FLAG_MACHINE); 
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	return ADS_ERROR(LDAP_SUCCESS);
}
