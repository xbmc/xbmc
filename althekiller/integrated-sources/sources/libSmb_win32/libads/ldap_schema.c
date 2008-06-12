/* 
   Unix SMB/CIFS implementation.
   ads (active directory) utility library
   Copyright (C) Guenther Deschner 2005-2006
   Copyright (C) Gerald (Jerry) Carter 2006
   
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

#ifdef HAVE_LDAP

ADS_STATUS ads_get_attrnames_by_oids(ADS_STRUCT *ads, TALLOC_CTX *mem_ctx,
				     const char *schema_path,
				     const char **OIDs, size_t num_OIDs, 
				     char ***OIDs_out, char ***names, size_t *count)
{
	ADS_STATUS status;
	void *res = NULL;
	LDAPMessage *msg;
	char *expr = NULL;
	const char *attrs[] = { "lDAPDisplayName", "attributeId", NULL };
	int i = 0, p = 0;
	
	if (!ads || !mem_ctx || !names || !count || !OIDs || !OIDs_out) {
		return ADS_ERROR(LDAP_PARAM_ERROR);
	}

	if (num_OIDs == 0 || OIDs[0] == NULL) {
		return ADS_ERROR_NT(NT_STATUS_NONE_MAPPED);
	}

	if ((expr = talloc_asprintf(mem_ctx, "(|")) == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	for (i=0; i<num_OIDs; i++) {

		if ((expr = talloc_asprintf_append(expr, "(attributeId=%s)", 
						   OIDs[i])) == NULL) {
			return ADS_ERROR(LDAP_NO_MEMORY);
		}
	}

	if ((expr = talloc_asprintf_append(expr, ")")) == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	status = ads_do_search_retry(ads, schema_path, 
				     LDAP_SCOPE_SUBTREE, expr, attrs, &res);
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	*count = ads_count_replies(ads, res);
	if (*count == 0 || !res) {
		status = ADS_ERROR_NT(NT_STATUS_NONE_MAPPED);
		goto out;
	}

	if (((*names) = TALLOC_ARRAY(mem_ctx, char *, *count)) == NULL) {
		status = ADS_ERROR(LDAP_NO_MEMORY);
		goto out;
	}
	if (((*OIDs_out) = TALLOC_ARRAY(mem_ctx, char *, *count)) == NULL) {
		status = ADS_ERROR(LDAP_NO_MEMORY);
		goto out;
	}

	for (msg = ads_first_entry(ads, res); msg != NULL; 
	     msg = ads_next_entry(ads, msg)) {

		(*names)[p] 	= ads_pull_string(ads, mem_ctx, msg, 
						  "lDAPDisplayName");
		(*OIDs_out)[p] 	= ads_pull_string(ads, mem_ctx, msg, 
						  "attributeId");
		if (((*names)[p] == NULL) || ((*OIDs_out)[p] == NULL)) {
			status = ADS_ERROR(LDAP_NO_MEMORY);
			goto out;
		}

		p++;
	}

	if (*count < num_OIDs) {
		status = ADS_ERROR_NT(STATUS_SOME_UNMAPPED);
		goto out;
	}

	status = ADS_ERROR(LDAP_SUCCESS);
out:
	ads_msgfree(ads, res);

	return status;
}

const char *ads_get_attrname_by_oid(ADS_STRUCT *ads, const char *schema_path, TALLOC_CTX *mem_ctx, const char * OID)
{
	ADS_STATUS rc;
	int count = 0;
	void *res = NULL;
	char *expr = NULL;
	const char *attrs[] = { "lDAPDisplayName", NULL };
	char *result;

	if (ads == NULL || mem_ctx == NULL || OID == NULL) {
		goto failed;
	}

	expr = talloc_asprintf(mem_ctx, "(attributeId=%s)", OID);
	if (expr == NULL) {
		goto failed;
	}

	rc = ads_do_search_retry(ads, schema_path, LDAP_SCOPE_SUBTREE, 
		expr, attrs, &res);
	if (!ADS_ERR_OK(rc)) {
		goto failed;
	}

	count = ads_count_replies(ads, res);
	if (count == 0 || !res) {
		goto failed;
	}

	result = ads_pull_string(ads, mem_ctx, res, "lDAPDisplayName");
	ads_msgfree(ads, res);

	return result;
	
failed:
	DEBUG(0,("ads_get_attrname_by_oid: failed to retrieve name for oid: %s\n", 
		OID));
	
	ads_msgfree(ads, res);
	return NULL;
}

/*********************************************************************
*********************************************************************/

static ADS_STATUS ads_schema_path(ADS_STRUCT *ads, TALLOC_CTX *mem_ctx, char **schema_path)
{
	ADS_STATUS status;
	void *res;
	const char *schema;
	const char *attrs[] = { "schemaNamingContext", NULL };

	status = ads_do_search(ads, "", LDAP_SCOPE_BASE, "(objectclass=*)", attrs, &res);
	if (!ADS_ERR_OK(status)) {
		return status;
	}

	if ( (schema = ads_pull_string(ads, mem_ctx, res, "schemaNamingContext")) == NULL ) {
		return ADS_ERROR(LDAP_NO_RESULTS_RETURNED);
	}

	if ( (*schema_path = talloc_strdup(mem_ctx, schema)) == NULL ) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	ads_msgfree(ads, res);

	return status;
}

/**
 * Check for "Services for Unix" or rfc2307 Schema and load some attributes into the ADS_STRUCT
 * @param ads connection to ads server
 * @param enum mapping type
 * @return ADS_STATUS status of search (False if one or more attributes couldn't be
 * found in Active Directory)
 **/ 
ADS_STATUS ads_check_posix_schema_mapping(ADS_STRUCT *ads, enum wb_posix_mapping map_type) 
{
	TALLOC_CTX *ctx = NULL; 
	ADS_STATUS status;
	char **oids_out, **names_out;
	size_t num_names;
	char *schema_path = NULL;
	int i;

	const char *oids_sfu[] = { 	ADS_ATTR_SFU_UIDNUMBER_OID,
					ADS_ATTR_SFU_GIDNUMBER_OID,
					ADS_ATTR_SFU_HOMEDIR_OID,
					ADS_ATTR_SFU_SHELL_OID,
					ADS_ATTR_SFU_GECOS_OID};

	const char *oids_rfc2307[] = {	ADS_ATTR_RFC2307_UIDNUMBER_OID,
					ADS_ATTR_RFC2307_GIDNUMBER_OID,
					ADS_ATTR_RFC2307_HOMEDIR_OID,
					ADS_ATTR_RFC2307_SHELL_OID,
					ADS_ATTR_RFC2307_GECOS_OID };

	DEBUG(10,("ads_check_posix_schema_mapping\n"));

	switch (map_type) {
	
		case WB_POSIX_MAP_TEMPLATE:
		case WB_POSIX_MAP_UNIXINFO:
			DEBUG(10,("ads_check_posix_schema_mapping: nothing to do\n"));
			return ADS_ERROR(LDAP_SUCCESS);

		case WB_POSIX_MAP_SFU:
		case WB_POSIX_MAP_RFC2307:
			break;

		default:
			DEBUG(0,("ads_check_posix_schema_mapping: "
				 "unknown enum %d\n", map_type));
			return ADS_ERROR(LDAP_PARAM_ERROR);
	}

	ads->schema.posix_uidnumber_attr = NULL;
	ads->schema.posix_gidnumber_attr = NULL;
	ads->schema.posix_homedir_attr = NULL;
	ads->schema.posix_shell_attr = NULL;
	ads->schema.posix_gecos_attr = NULL;

	ctx = talloc_init("ads_check_posix_schema_mapping");
	if (ctx == NULL) {
		return ADS_ERROR(LDAP_NO_MEMORY);
	}

	status = ads_schema_path(ads, ctx, &schema_path);
	if (!ADS_ERR_OK(status)) {
		DEBUG(3,("ads_check_posix_mapping: Unable to retrieve schema DN!\n"));
		goto done;
	}

	if (map_type == WB_POSIX_MAP_SFU) {
		status = ads_get_attrnames_by_oids(ads, ctx, schema_path, oids_sfu, 
						   ARRAY_SIZE(oids_sfu), 
						   &oids_out, &names_out, &num_names);
	} else { 
		status = ads_get_attrnames_by_oids(ads, ctx, schema_path, oids_rfc2307, 
						   ARRAY_SIZE(oids_rfc2307), 
						   &oids_out, &names_out, &num_names);
	}

	if (!ADS_ERR_OK(status)) {
		DEBUG(3,("ads_check_posix_schema_mapping: failed %s\n", 
			ads_errstr(status)));
		goto done;
	} 
	
	DEBUG(10,("ads_check_posix_schema_mapping: query succeeded, identified: %s\n",
		wb_posix_map_str(map_type)));

	for (i=0; i<num_names; i++) {

		DEBUGADD(10,("\tOID %s has name: %s\n", oids_out[i], names_out[i]));

		if (strequal(ADS_ATTR_RFC2307_UIDNUMBER_OID, oids_out[i]) ||
		    strequal(ADS_ATTR_SFU_UIDNUMBER_OID, oids_out[i])) {
			SAFE_FREE(ads->schema.posix_uidnumber_attr);
			ads->schema.posix_uidnumber_attr = SMB_STRDUP(names_out[i]);
		}
		if (strequal(ADS_ATTR_RFC2307_GIDNUMBER_OID, oids_out[i]) ||
		    strequal(ADS_ATTR_SFU_GIDNUMBER_OID, oids_out[i])) {
			SAFE_FREE(ads->schema.posix_gidnumber_attr);
			ads->schema.posix_gidnumber_attr = SMB_STRDUP(names_out[i]);
		}
		if (strequal(ADS_ATTR_RFC2307_HOMEDIR_OID, oids_out[i]) ||
		    strequal(ADS_ATTR_SFU_HOMEDIR_OID, oids_out[i])) {
			SAFE_FREE(ads->schema.posix_homedir_attr);
			ads->schema.posix_homedir_attr = SMB_STRDUP(names_out[i]);
		}
		if (strequal(ADS_ATTR_RFC2307_SHELL_OID, oids_out[i]) ||
		    strequal(ADS_ATTR_SFU_SHELL_OID, oids_out[i])) {
			SAFE_FREE(ads->schema.posix_shell_attr);
			ads->schema.posix_shell_attr = SMB_STRDUP(names_out[i]);
		}
		if (strequal(ADS_ATTR_RFC2307_GECOS_OID, oids_out[i]) ||
		    strequal(ADS_ATTR_SFU_GECOS_OID, oids_out[i])) {
			SAFE_FREE(ads->schema.posix_gecos_attr);
			ads->schema.posix_gecos_attr = SMB_STRDUP(names_out[i]);
		}
	}

	if (!ads->schema.posix_uidnumber_attr ||
	    !ads->schema.posix_gidnumber_attr ||
	    !ads->schema.posix_homedir_attr ||
	    !ads->schema.posix_shell_attr ||
	    !ads->schema.posix_gecos_attr) {
	    	status = ADS_ERROR(LDAP_NO_MEMORY);
	    	goto done;
	}
	
	status = ADS_ERROR(LDAP_SUCCESS);
	
	ads->schema.map_type = map_type;
done:
	if (ctx) {
		talloc_destroy(ctx);
	}

	return status;
}

#endif
