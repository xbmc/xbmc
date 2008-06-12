/*
   Unix SMB/CIFS implementation.
   Auditing helper functions.
   Copyright (C) Guenther Deschner 2006

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

static const struct audit_category_tab {
	uint32 category;
	const char *category_str;
	const char *param_str;
	const char *description;
} audit_category_tab [] = {
	{ LSA_AUDIT_CATEGORY_LOGON, 
	 "LSA_AUDIT_CATEGORY_LOGON", 
	 "LOGON", "Logon events" },
	{ LSA_AUDIT_CATEGORY_USE_OF_USER_RIGHTS, 
	 "LSA_AUDIT_CATEGORY_USE_OF_USER_RIGHTS", 
	 "PRIVILEGE", "Privilege Use" },
	{ LSA_AUDIT_CATEGORY_SYSTEM, 
	 "LSA_AUDIT_CATEGORY_SYSTEM", 
	 "SYSTEM", "System Events" },
	{ LSA_AUDIT_CATEGORY_SECURITY_POLICY_CHANGES, 
	 "LSA_AUDIT_CATEGORY_SECURITY_POLICY_CHANGES", 
	 "POLICY", "Policy Change" },
	{ LSA_AUDIT_CATEGORY_PROCCESS_TRACKING, 
	 "LSA_AUDIT_CATEGORY_PROCCESS_TRACKING", 
	 "PROCESS", "Process Tracking" },
	{ LSA_AUDIT_CATEGORY_FILE_AND_OBJECT_ACCESS, 
	 "LSA_AUDIT_CATEGORY_FILE_AND_OBJECT_ACCESS", 
	 "OBJECT", "Object Access" },
	{ LSA_AUDIT_CATEGORY_ACCOUNT_MANAGEMENT, 
	 "LSA_AUDIT_CATEGORY_ACCOUNT_MANAGEMENT", 
	 "SAM", "Account Management" },
	{ LSA_AUDIT_CATEGORY_DIRECTORY_SERVICE_ACCESS, 
	 "LSA_AUDIT_CATEGORY_DIRECTORY_SERVICE_ACCESS", 
	 "DIRECTORY", "Directory service access" },
	{ LSA_AUDIT_CATEGORY_ACCOUNT_LOGON, 
	 "LSA_AUDIT_CATEGORY_ACCOUNT_LOGON", 
	 "ACCOUNT", "Account logon events" },
	{ 0, NULL, NULL }
};

const char *audit_category_str(uint32 category)
{
	int i;
	for (i=0; audit_category_tab[i].category_str; i++) {
		if (category == audit_category_tab[i].category) {
			return audit_category_tab[i].category_str;
		}
	}
	return NULL;
}

const char *audit_param_str(uint32 category)
{
	int i;
	for (i=0; audit_category_tab[i].param_str; i++) {
		if (category == audit_category_tab[i].category) {
			return audit_category_tab[i].param_str;
		}
	}
	return NULL;
}

const char *audit_description_str(uint32 category)
{
	int i;
	for (i=0; audit_category_tab[i].description; i++) {
		if (category == audit_category_tab[i].category) {
			return audit_category_tab[i].description;
		}
	}
	return NULL;
}

BOOL get_audit_category_from_param(const char *param, uint32 *audit_category)
{
	*audit_category = Undefined;

	if (strequal(param, "SYSTEM")) {
		*audit_category = LSA_AUDIT_CATEGORY_SYSTEM;
	} else if (strequal(param, "LOGON")) {
		*audit_category = LSA_AUDIT_CATEGORY_LOGON;
	} else if (strequal(param, "OBJECT")) {
		*audit_category = LSA_AUDIT_CATEGORY_FILE_AND_OBJECT_ACCESS;
	} else if (strequal(param, "PRIVILEGE")) {
		*audit_category = LSA_AUDIT_CATEGORY_USE_OF_USER_RIGHTS;
	} else if (strequal(param, "PROCESS")) {
		*audit_category = LSA_AUDIT_CATEGORY_PROCCESS_TRACKING;
	} else if (strequal(param, "POLICY")) {
		*audit_category = LSA_AUDIT_CATEGORY_SECURITY_POLICY_CHANGES;
	} else if (strequal(param, "SAM")) {
		*audit_category = LSA_AUDIT_CATEGORY_ACCOUNT_MANAGEMENT;
	} else if (strequal(param, "DIRECTORY")) {
		*audit_category = LSA_AUDIT_CATEGORY_DIRECTORY_SERVICE_ACCESS;
	} else if (strequal(param, "ACCOUNT")) {
		*audit_category = LSA_AUDIT_CATEGORY_ACCOUNT_LOGON;
	} else {
		DEBUG(0,("unknown parameter: %s\n", param));
		return False;
	}

	return True;
}

const char *audit_policy_str(TALLOC_CTX *mem_ctx, uint32 policy)
{
	const char *ret = NULL;

	if (policy == LSA_AUDIT_POLICY_NONE) {
		return talloc_strdup(mem_ctx, "None");
	}

	if (policy & LSA_AUDIT_POLICY_SUCCESS) {
		ret = talloc_strdup(mem_ctx, "Success");
		if (ret == NULL) {
			return NULL;
		}
	}

	if (policy & LSA_AUDIT_POLICY_FAILURE) {
		if (ret) {
			ret = talloc_asprintf(mem_ctx, "%s, %s", ret, "Failure");
			if (ret == NULL) {
				return NULL;
			}
		} else {
			return talloc_strdup(mem_ctx, "Failure");
		}
	}

	return ret;
}
