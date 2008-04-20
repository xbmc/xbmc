/* 
   Samba Unix/Linux SMB client library 
   net ads commands for Group Policy
   Copyright (C) 2005 Guenther Deschner (gd@samba.org)

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
#include "utils/net.h"

#ifdef HAVE_ADS

static int net_ads_gpo_usage(int argc, const char **argv)
{
	d_printf(
		"net ads gpo <COMMAND>\n"\
"<COMMAND> can be either:\n"\
"  ADDLINK      Link a container to a GPO\n"\
"  APPLY        Apply all GPOs\n"\
"  DELETELINK   Delete a gPLink from a container\n"\
"  EFFECTIVE    Lists all GPOs assigned to a machine\n"\
"  GETGPO       Lists specified GPO\n"\
"  GETLINK      Lists gPLink of a containter\n"\
"  HELP         Prints this help message\n"\
"  LIST         Lists all GPOs\n"\
"\n"
		);
	return -1;
}

static int net_ads_gpo_effective(int argc, const char **argv)
{
	TALLOC_CTX *mem_ctx;
	ADS_STRUCT *ads;
	ADS_STATUS status;
	const char *attrs[] = {"distinguishedName", "userAccountControl", NULL};
	void *res = NULL;
	const char *filter;
	char *dn = NULL;
	struct GROUP_POLICY_OBJECT *gpo_list;
	uint32 uac = 0;
	uint32 flags = 0;
	
	if (argc < 1) {
		return -1;
	}

	mem_ctx = talloc_init("net_ads_gpo_effective");
	if (mem_ctx == NULL) {
		return -1;
	}

	filter = talloc_asprintf(mem_ctx, "(&(objectclass=user)(sAMAccountName=%s))", argv[0]);
	if (filter == NULL) {
		goto out;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_do_search_all(ads, ads->config.bind_path,
				   LDAP_SCOPE_SUBTREE,
				   filter, attrs, &res);
	
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

	if (ads_count_replies(ads, res) != 1) {
		printf("no result\n");
		goto out;
	}

	dn = ads_get_dn(ads, res);
	if (dn == NULL) {
		goto out;
	}

	if (!ads_pull_uint32(ads, res, "userAccountControl", &uac)) {
		goto out;
	}

	if (uac & UF_WORKSTATION_TRUST_ACCOUNT) {
		flags |= GPO_LIST_FLAG_MACHINE;
	}

	printf("%s: '%s' has dn: '%s'\n", 
		(uac & UF_WORKSTATION_TRUST_ACCOUNT) ? "machine" : "user", 
		argv[0], dn);

	status = ads_get_gpo_list(ads, mem_ctx, dn, flags, &gpo_list);
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

	printf("unsorted full dump of all GPOs for this machine:\n");

	{
		struct GROUP_POLICY_OBJECT *gpo = gpo_list;

		for (gpo = gpo_list; gpo; gpo = gpo->next) {
			dump_gpo(mem_ctx, gpo);
		}
	}

	printf("sorted full dump of all GPOs valid for this machine:\n");
      
out:
	ads_memfree(ads, dn);
	ads_msgfree(ads, res);

	ads_destroy(&ads);
	talloc_destroy(mem_ctx);
	return 0;
}

static int net_ads_gpo_list(int argc, const char **argv)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	void *res = NULL;
	int num_reply = 0;
	void *msg = NULL;
	struct GROUP_POLICY_OBJECT gpo;
	TALLOC_CTX *mem_ctx;

	mem_ctx = talloc_init("net_ads_gpo_list");
	if (mem_ctx == NULL) {
		return -1;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_do_search_all(ads, ads->config.bind_path,
				   LDAP_SCOPE_SUBTREE,
				   "(objectclass=groupPolicyContainer)", NULL, &res);
	if (!ADS_ERR_OK(status)) {
		d_printf("search failed: %s\n", ads_errstr(status));
		goto out;
	}	

	num_reply = ads_count_replies(ads, res);
	
	d_printf("Got %d replies\n\n", num_reply);

	/* dump the results */
	for (msg = ads_first_entry(ads, res); msg; msg = ads_next_entry(ads, msg)) {
	
		status = ads_parse_gpo(ads, mem_ctx, msg, ads_get_dn(ads, msg), &gpo);

		if (!ADS_ERR_OK(status)) {
			d_printf("parse failed: %s\n", ads_errstr(status));
			goto out;
		}	

		dump_gpo(mem_ctx, &gpo);

	}

out:
	ads_msgfree(ads, res);

	talloc_destroy(mem_ctx);
	ads_destroy(&ads);
	
	return 0;
}

static int net_ads_gpo_apply(int argc, const char **argv)
{
	TALLOC_CTX *mem_ctx;
	ADS_STRUCT *ads;
	ADS_STATUS status;
	const char *attrs[] = {"distinguishedName", "userAccountControl", NULL};
	void *res = NULL;
	const char *filter;
	char *dn = NULL;
	struct GROUP_POLICY_OBJECT *gpo_list;
	uint32 uac = 0;
	uint32 flags = 0;
	
	if (argc < 1) {
		return -1;
	}

	mem_ctx = talloc_init("net_ads_gpo_apply");
	if (mem_ctx == NULL) {
		goto out;
	}

	filter = talloc_asprintf(mem_ctx, "(&(objectclass=user)(sAMAccountName=%s))", argv[0]);
	if (filter == NULL) {
		goto out;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_do_search_all(ads, ads->config.bind_path,
				   LDAP_SCOPE_SUBTREE,
				   filter, attrs, &res);
	
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

	if (ads_count_replies(ads, res) != 1) {
		printf("no result\n");
		goto out;
	}

	dn = ads_get_dn(ads, res);
	if (dn == NULL) {
		goto out;
	}

	if (!ads_pull_uint32(ads, res, "userAccountControl", &uac)) {
		goto out;
	}

	if (uac & UF_WORKSTATION_TRUST_ACCOUNT) {
		flags |= GPO_LIST_FLAG_MACHINE;
	}

	printf("%s: '%s' has dn: '%s'\n", 
		(uac & UF_WORKSTATION_TRUST_ACCOUNT) ? "machine" : "user", 
		argv[0], dn);

	status = ads_get_gpo_list(ads, mem_ctx, dn, flags, &gpo_list);
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

	/* FIXME: allow to process just a single extension */
	status = gpo_process_gpo_list(ads, mem_ctx, &gpo_list, NULL, flags); 
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

out:
	ads_memfree(ads, dn);
	ads_msgfree(ads, res);

	ads_destroy(&ads);
	talloc_destroy(mem_ctx);
	return 0;
}


static int net_ads_gpo_get_link(int argc, const char **argv)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	TALLOC_CTX *mem_ctx;
	struct GP_LINK gp_link;

	if (argc < 1) {
		return -1;
	}

	mem_ctx = talloc_init("add_gpo_link");
	if (mem_ctx == NULL) {
		return -1;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_get_gpo_link(ads, mem_ctx, argv[0], &gp_link);
	if (!ADS_ERR_OK(status)) {
		d_printf("get link for %s failed: %s\n", argv[0], ads_errstr(status));
		goto out;
	}	

	dump_gplink(ads, mem_ctx, &gp_link);

out:
	talloc_destroy(mem_ctx);
	ads_destroy(&ads);

	return 0;
}

static int net_ads_gpo_add_link(int argc, const char **argv)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	uint32 gpo_opt = 0;
	TALLOC_CTX *mem_ctx;

	if (argc < 2) {
		return -1;
	}

	mem_ctx = talloc_init("add_gpo_link");
	if (mem_ctx == NULL) {
		return -1;
	}

	if (argc == 3) {
		gpo_opt = atoi(argv[2]);
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_add_gpo_link(ads, mem_ctx, argv[0], argv[1], gpo_opt);
	if (!ADS_ERR_OK(status)) {
		d_printf("add link failed: %s\n", ads_errstr(status));
		goto out;
	}

out:
	talloc_destroy(mem_ctx);
	ads_destroy(&ads);

	return 0;
}

static int net_ads_gpo_delete_link(int argc, const char **argv)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	TALLOC_CTX *mem_ctx;

	if (argc < 2) {
		return -1;
	}

	mem_ctx = talloc_init("delete_gpo_link");
	if (mem_ctx == NULL) {
		return -1;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	status = ads_delete_gpo_link(ads, mem_ctx, argv[0], argv[1]);
	if (!ADS_ERR_OK(status)) {
		d_printf("delete link failed: %s\n", ads_errstr(status));
		goto out;
	}	

out:
	talloc_destroy(mem_ctx);
	ads_destroy(&ads);

	return 0;
}

static int net_ads_gpo_get_gpo(int argc, const char **argv)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	TALLOC_CTX *mem_ctx;
	struct GROUP_POLICY_OBJECT gpo;
	uint32 sysvol_gpt_version;

	if (argc < 1) {
		return -1;
	}

	mem_ctx = talloc_init("add_gpo_get_gpo");
	if (mem_ctx == NULL) {
		return -1;
	}

	if (!(ads = ads_startup())) {
		goto out;
	}

	if (strnequal(argv[0], "CN={", strlen("CN={"))) {
		status = ads_get_gpo(ads, mem_ctx, argv[0], NULL, NULL, &gpo);
	} else {
		status = ads_get_gpo(ads, mem_ctx, NULL, argv[0], NULL, &gpo);
	}

	if (!ADS_ERR_OK(status)) {
		d_printf("get gpo for [%s] failed: %s\n", argv[0], ads_errstr(status));
		goto out;
	}	

	dump_gpo(mem_ctx, &gpo);

	status = ADS_ERROR_NT(ads_gpo_get_sysvol_gpt_version(ads, mem_ctx, gpo.file_sys_path, &sysvol_gpt_version)); 
	if (!ADS_ERR_OK(status)) {
		goto out;
	}

	printf("sysvol GPT version: %d\n", sysvol_gpt_version);

out:
	talloc_destroy(mem_ctx);
	ads_destroy(&ads);

	return 0;
}

int net_ads_gpo(int argc, const char **argv)
{
	struct functable func[] = {
		{"LIST", net_ads_gpo_list},
		{"EFFECTIVE", net_ads_gpo_effective},
		{"ADDLINK", net_ads_gpo_add_link},
		{"DELETELINK", net_ads_gpo_delete_link},
		{"GETLINK", net_ads_gpo_get_link},
		{"GETGPO", net_ads_gpo_get_gpo},
		{"HELP", net_ads_gpo_usage},
		{"APPLY", net_ads_gpo_apply},
		{NULL, NULL}
	};

	return net_run_function(argc, argv, func, net_ads_gpo_usage);
}

#endif
