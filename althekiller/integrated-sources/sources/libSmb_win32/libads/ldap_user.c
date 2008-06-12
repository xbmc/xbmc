/* 
   Unix SMB/CIFS implementation.
   ads (active directory) utility library
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002
   
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

#ifdef HAVE_ADS

/*
  find a user account
*/
ADS_STATUS ads_find_user_acct(ADS_STRUCT *ads, void **res, const char *user)
{
	ADS_STATUS status;
	char *ldap_exp;
	const char *attrs[] = {"*", NULL};
	char *escaped_user = escape_ldap_string_alloc(user);
	if (!escaped_user) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	asprintf(&ldap_exp, "(samAccountName=%s)", escaped_user);
	status = ads_search(ads, res, ldap_exp, attrs);
	SAFE_FREE(ldap_exp);
	SAFE_FREE(escaped_user);
	return status;
}

ADS_STATUS ads_add_user_acct(ADS_STRUCT *ads, const char *user, 
			     const char *container, const char *fullname)
{
	TALLOC_CTX *ctx;
	ADS_MODLIST mods;
	ADS_STATUS status;
	const char *upn, *new_dn, *name, *controlstr;
	const char *objectClass[] = {"top", "person", "organizationalPerson",
				     "user", NULL};

	if (fullname && *fullname) name = fullname;
	else name = user;

	if (!(ctx = talloc_init("ads_add_user_acct")))
		return ADS_ERROR(LDAP_NO_MEMORY);

	status = ADS_ERROR(LDAP_NO_MEMORY);

	if (!(upn = talloc_asprintf(ctx, "%s@%s", user, ads->config.realm)))
		goto done;
	if (!(new_dn = talloc_asprintf(ctx, "cn=%s,%s,%s", name, container,
				       ads->config.bind_path)))
		goto done;
	if (!(controlstr = talloc_asprintf(ctx, "%u", (UF_NORMAL_ACCOUNT | UF_ACCOUNTDISABLE))))
		goto done;
	if (!(mods = ads_init_mods(ctx)))
		goto done;

	ads_mod_str(ctx, &mods, "cn", name);
	ads_mod_strlist(ctx, &mods, "objectClass", objectClass);
	ads_mod_str(ctx, &mods, "userPrincipalName", upn);
	ads_mod_str(ctx, &mods, "name", name);
	ads_mod_str(ctx, &mods, "displayName", name);
	ads_mod_str(ctx, &mods, "sAMAccountName", user);
	ads_mod_str(ctx, &mods, "userAccountControl", controlstr);
	status = ads_gen_add(ads, new_dn, mods);

 done:
	talloc_destroy(ctx);
	return status;
}

ADS_STATUS ads_add_group_acct(ADS_STRUCT *ads, const char *group, 
			      const char *container, const char *comment)
{
	TALLOC_CTX *ctx;
	ADS_MODLIST mods;
	ADS_STATUS status;
	char *new_dn;
	const char *objectClass[] = {"top", "group", NULL};

	if (!(ctx = talloc_init("ads_add_group_acct")))
		return ADS_ERROR(LDAP_NO_MEMORY);

	status = ADS_ERROR(LDAP_NO_MEMORY);

	if (!(new_dn = talloc_asprintf(ctx, "cn=%s,%s,%s", group, container,
				       ads->config.bind_path)))
		goto done;
	if (!(mods = ads_init_mods(ctx)))
		goto done;

	ads_mod_str(ctx, &mods, "cn", group);
	ads_mod_strlist(ctx, &mods, "objectClass",objectClass);
	ads_mod_str(ctx, &mods, "name", group);
	if (comment && *comment) 
		ads_mod_str(ctx, &mods, "description", comment);
	ads_mod_str(ctx, &mods, "sAMAccountName", group);
	status = ads_gen_add(ads, new_dn, mods);

 done:
	talloc_destroy(ctx);
	return status;
}
#endif
