/* 
   Unix SMB/CIFS implementation.
   LDAP protocol helper functions for SAMBA
   Copyright (C) Jean François Micouleau	1998
   Copyright (C) Gerald Carter			2001-2003
   Copyright (C) Shahms King			2001
   Copyright (C) Andrew Bartlett		2002-2003
   Copyright (C) Stefan (metze) Metzmacher	2002-2003
    
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
#include "smbldap.h"

#ifndef LDAP_OPT_SUCCESS
#define LDAP_OPT_SUCCESS 0
#endif

/* Try not to hit the up or down server forever */

#define SMBLDAP_DONT_PING_TIME 10	/* ping only all 10 seconds */
#define SMBLDAP_NUM_RETRIES 8	        /* retry only 8 times */

#define SMBLDAP_IDLE_TIME 150		/* After 2.5 minutes disconnect */


/* attributes used by Samba 2.2 */

ATTRIB_MAP_ENTRY attrib_map_v22[] = {
	{ LDAP_ATTR_UID,		"uid" 		},
	{ LDAP_ATTR_UIDNUMBER,		LDAP_ATTRIBUTE_UIDNUMBER},
	{ LDAP_ATTR_GIDNUMBER,		LDAP_ATTRIBUTE_GIDNUMBER},
	{ LDAP_ATTR_UNIX_HOME,		"homeDirectory"	},
	{ LDAP_ATTR_PWD_LAST_SET,	"pwdLastSet"	},
	{ LDAP_ATTR_PWD_CAN_CHANGE,	"pwdCanChange"	},
	{ LDAP_ATTR_PWD_MUST_CHANGE,	"pwdMustChange"	},
	{ LDAP_ATTR_LOGON_TIME,		"logonTime" 	},
	{ LDAP_ATTR_LOGOFF_TIME,	"logoffTime"	},
	{ LDAP_ATTR_KICKOFF_TIME,	"kickoffTime"	},
	{ LDAP_ATTR_CN,			"cn"		},
	{ LDAP_ATTR_SN,			"sn"		},
	{ LDAP_ATTR_DISPLAY_NAME,	"displayName"	},
	{ LDAP_ATTR_HOME_PATH,		"smbHome"	},
	{ LDAP_ATTR_HOME_DRIVE,		"homeDrive"	},
	{ LDAP_ATTR_LOGON_SCRIPT,	"scriptPath"	},
	{ LDAP_ATTR_PROFILE_PATH,	"profilePath"	},
	{ LDAP_ATTR_DESC,		"description"	},
	{ LDAP_ATTR_USER_WKS,		"userWorkstations"},
	{ LDAP_ATTR_USER_RID,		"rid"		},
	{ LDAP_ATTR_PRIMARY_GROUP_RID,	"primaryGroupID"},
	{ LDAP_ATTR_LMPW,		"lmPassword"	},
	{ LDAP_ATTR_NTPW,		"ntPassword"	},
	{ LDAP_ATTR_DOMAIN,		"domain"	},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"	},
	{ LDAP_ATTR_ACB_INFO,		"acctFlags"	},
	{ LDAP_ATTR_MOD_TIMESTAMP,	"modifyTimestamp"	},
	{ LDAP_ATTR_LIST_END,		NULL 		}
};

ATTRIB_MAP_ENTRY attrib_map_to_delete_v22[] = {
	{ LDAP_ATTR_PWD_LAST_SET,	"pwdLastSet"	},
	{ LDAP_ATTR_PWD_CAN_CHANGE,	"pwdCanChange"	},
	{ LDAP_ATTR_PWD_MUST_CHANGE,	"pwdMustChange"	},
	{ LDAP_ATTR_LOGON_TIME,		"logonTime" 	},
	{ LDAP_ATTR_LOGOFF_TIME,	"logoffTime"	},
	{ LDAP_ATTR_KICKOFF_TIME,	"kickoffTime"	},
	{ LDAP_ATTR_DISPLAY_NAME,	"displayName"	},
	{ LDAP_ATTR_HOME_PATH,		"smbHome"	},
	{ LDAP_ATTR_HOME_DRIVE,		"homeDrives"	},
	{ LDAP_ATTR_LOGON_SCRIPT,	"scriptPath"	},
	{ LDAP_ATTR_PROFILE_PATH,	"profilePath"	},
	{ LDAP_ATTR_USER_WKS,		"userWorkstations"},
	{ LDAP_ATTR_USER_RID,		"rid"		},
	{ LDAP_ATTR_PRIMARY_GROUP_RID,	"primaryGroupID"},
	{ LDAP_ATTR_LMPW,		"lmPassword"	},
	{ LDAP_ATTR_NTPW,		"ntPassword"	},
	{ LDAP_ATTR_DOMAIN,		"domain"	},
	{ LDAP_ATTR_ACB_INFO,		"acctFlags"	},
	{ LDAP_ATTR_LIST_END,		NULL 		}
};

/* attributes used by Samba 3.0's sambaSamAccount */

ATTRIB_MAP_ENTRY attrib_map_v30[] = {
	{ LDAP_ATTR_UID,		"uid" 			},
	{ LDAP_ATTR_UIDNUMBER,		LDAP_ATTRIBUTE_UIDNUMBER},
	{ LDAP_ATTR_GIDNUMBER,		LDAP_ATTRIBUTE_GIDNUMBER},
	{ LDAP_ATTR_UNIX_HOME,		"homeDirectory"		},
	{ LDAP_ATTR_PWD_LAST_SET,	"sambaPwdLastSet"	},
	{ LDAP_ATTR_PWD_CAN_CHANGE,	"sambaPwdCanChange"	},
	{ LDAP_ATTR_PWD_MUST_CHANGE,	"sambaPwdMustChange"	},
	{ LDAP_ATTR_LOGON_TIME,		"sambaLogonTime" 	},
	{ LDAP_ATTR_LOGOFF_TIME,	"sambaLogoffTime"	},
	{ LDAP_ATTR_KICKOFF_TIME,	"sambaKickoffTime"	},
	{ LDAP_ATTR_CN,			"cn"			},
	{ LDAP_ATTR_SN,			"sn"			},
	{ LDAP_ATTR_DISPLAY_NAME,	"displayName"		},
	{ LDAP_ATTR_HOME_DRIVE,		"sambaHomeDrive"	},
	{ LDAP_ATTR_HOME_PATH,		"sambaHomePath"		},
	{ LDAP_ATTR_LOGON_SCRIPT,	"sambaLogonScript"	},
	{ LDAP_ATTR_PROFILE_PATH,	"sambaProfilePath"	},
	{ LDAP_ATTR_DESC,		"description"		},
	{ LDAP_ATTR_USER_WKS,		"sambaUserWorkstations"	},
	{ LDAP_ATTR_USER_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_PRIMARY_GROUP_SID,	"sambaPrimaryGroupSID"	},
	{ LDAP_ATTR_LMPW,		"sambaLMPassword"	},
	{ LDAP_ATTR_NTPW,		"sambaNTPassword"	},
	{ LDAP_ATTR_DOMAIN,		"sambaDomainName"	},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"		},
	{ LDAP_ATTR_ACB_INFO,		"sambaAcctFlags"	},
	{ LDAP_ATTR_MUNGED_DIAL,	"sambaMungedDial"	},
	{ LDAP_ATTR_BAD_PASSWORD_COUNT,	"sambaBadPasswordCount" },
	{ LDAP_ATTR_BAD_PASSWORD_TIME,	"sambaBadPasswordTime" 	},
	{ LDAP_ATTR_PWD_HISTORY,	"sambaPasswordHistory"  },
	{ LDAP_ATTR_MOD_TIMESTAMP,	"modifyTimestamp"	},
	{ LDAP_ATTR_LOGON_HOURS,	"sambaLogonHours"	},
	{ LDAP_ATTR_LIST_END,		NULL 			}
};

ATTRIB_MAP_ENTRY attrib_map_to_delete_v30[] = {
	{ LDAP_ATTR_PWD_LAST_SET,	"sambaPwdLastSet"	},
	{ LDAP_ATTR_PWD_CAN_CHANGE,	"sambaPwdCanChange"	},
	{ LDAP_ATTR_PWD_MUST_CHANGE,	"sambaPwdMustChange"	},
	{ LDAP_ATTR_LOGON_TIME,		"sambaLogonTime" 	},
	{ LDAP_ATTR_LOGOFF_TIME,	"sambaLogoffTime"	},
	{ LDAP_ATTR_KICKOFF_TIME,	"sambaKickoffTime"	},
	{ LDAP_ATTR_HOME_DRIVE,		"sambaHomeDrive"	},
	{ LDAP_ATTR_HOME_PATH,		"sambaHomePath"		},
	{ LDAP_ATTR_LOGON_SCRIPT,	"sambaLogonScript"	},
	{ LDAP_ATTR_PROFILE_PATH,	"sambaProfilePath"	},
	{ LDAP_ATTR_USER_WKS,		"sambaUserWorkstations"	},
	{ LDAP_ATTR_USER_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_PRIMARY_GROUP_SID,	"sambaPrimaryGroupSID"	},
	{ LDAP_ATTR_LMPW,		"sambaLMPassword"	},
	{ LDAP_ATTR_NTPW,		"sambaNTPassword"	},
	{ LDAP_ATTR_DOMAIN,		"sambaDomainName"	},
	{ LDAP_ATTR_ACB_INFO,		"sambaAcctFlags"	},
	{ LDAP_ATTR_MUNGED_DIAL,	"sambaMungedDial"	},
	{ LDAP_ATTR_BAD_PASSWORD_COUNT,	"sambaBadPasswordCount" },
	{ LDAP_ATTR_BAD_PASSWORD_TIME,	"sambaBadPasswordTime" 	},
	{ LDAP_ATTR_PWD_HISTORY,	"sambaPasswordHistory"  },
	{ LDAP_ATTR_LOGON_HOURS,	"sambaLogonHours"	},
	{ LDAP_ATTR_LIST_END,		NULL 			}
};

/* attributes used for allocating RIDs */

ATTRIB_MAP_ENTRY dominfo_attr_list[] = {
	{ LDAP_ATTR_DOMAIN,		"sambaDomainName"	},
	{ LDAP_ATTR_NEXT_RID,	        "sambaNextRid"	        },
	{ LDAP_ATTR_NEXT_USERRID,	"sambaNextUserRid"	},
	{ LDAP_ATTR_NEXT_GROUPRID,	"sambaNextGroupRid"	},
	{ LDAP_ATTR_DOM_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_ALGORITHMIC_RID_BASE,"sambaAlgorithmicRidBase"},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"		},
	{ LDAP_ATTR_LIST_END,		NULL			},
};

/* Samba 3.0 group mapping attributes */

ATTRIB_MAP_ENTRY groupmap_attr_list[] = {
	{ LDAP_ATTR_GIDNUMBER,		LDAP_ATTRIBUTE_GIDNUMBER},
	{ LDAP_ATTR_GROUP_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_GROUP_TYPE,		"sambaGroupType"	},
	{ LDAP_ATTR_SID_LIST,		"sambaSIDList"		},
	{ LDAP_ATTR_DESC,		"description"		},
	{ LDAP_ATTR_DISPLAY_NAME,	"displayName"		},
	{ LDAP_ATTR_CN,			"cn"			},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"		},
	{ LDAP_ATTR_LIST_END,		NULL			}	
};

ATTRIB_MAP_ENTRY groupmap_attr_list_to_delete[] = {
	{ LDAP_ATTR_GROUP_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_GROUP_TYPE,		"sambaGroupType"	},
	{ LDAP_ATTR_DESC,		"description"		},
	{ LDAP_ATTR_DISPLAY_NAME,	"displayName"		},
	{ LDAP_ATTR_SID_LIST,		"sambaSIDList"		},
	{ LDAP_ATTR_LIST_END,		NULL			}	
};

/* idmap_ldap sambaUnixIdPool */

ATTRIB_MAP_ENTRY idpool_attr_list[] = {
	{ LDAP_ATTR_UIDNUMBER,		LDAP_ATTRIBUTE_UIDNUMBER},
	{ LDAP_ATTR_GIDNUMBER,		LDAP_ATTRIBUTE_GIDNUMBER},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"		},
	{ LDAP_ATTR_LIST_END,		NULL			}	
};

ATTRIB_MAP_ENTRY sidmap_attr_list[] = {
	{ LDAP_ATTR_SID,		LDAP_ATTRIBUTE_SID	},
	{ LDAP_ATTR_UIDNUMBER,		LDAP_ATTRIBUTE_UIDNUMBER},
	{ LDAP_ATTR_GIDNUMBER,		LDAP_ATTRIBUTE_GIDNUMBER},
	{ LDAP_ATTR_OBJCLASS,		"objectClass"		},
	{ LDAP_ATTR_LIST_END,		NULL			}	
};

/**********************************************************************
 perform a simple table lookup and return the attribute name 
 **********************************************************************/
 
 const char* get_attr_key2string( ATTRIB_MAP_ENTRY table[], int key )
{
	int i = 0;
	
	while ( table[i].attrib != LDAP_ATTR_LIST_END ) {
		if ( table[i].attrib == key )
			return table[i].name;
		i++;
	}
	
	return NULL;
}


/**********************************************************************
 Return the list of attribute names from a mapping table
 **********************************************************************/

 const char** get_attr_list( TALLOC_CTX *mem_ctx, ATTRIB_MAP_ENTRY table[] )
{
	const char **names;
	int i = 0;
	
	while ( table[i].attrib != LDAP_ATTR_LIST_END )
		i++;
	i++;

	names = TALLOC_ARRAY( mem_ctx, const char*, i );
	if ( !names ) {
		DEBUG(0,("get_attr_list: out of memory\n"));
		return NULL;
	}

	i = 0;
	while ( table[i].attrib != LDAP_ATTR_LIST_END ) {
		names[i] = talloc_strdup( names, table[i].name );
		i++;
	}
	names[i] = NULL;
	
	return names;
}

/*******************************************************************
 Search an attribute and return the first value found.
******************************************************************/

 BOOL smbldap_get_single_attribute (LDAP * ldap_struct, LDAPMessage * entry,
				    const char *attribute, char *value,
				    int max_len)
{
	char **values;
	
	if ( !attribute )
		return False;
		
	value[0] = '\0';

	if ((values = ldap_get_values (ldap_struct, entry, attribute)) == NULL) {
		DEBUG (10, ("smbldap_get_single_attribute: [%s] = [<does not exist>]\n", attribute));
		
		return False;
	}
	
	if (convert_string(CH_UTF8, CH_UNIX,values[0], -1, value, max_len, False) == (size_t)-1) {
		DEBUG(1, ("smbldap_get_single_attribute: string conversion of [%s] = [%s] failed!\n", 
			  attribute, values[0]));
		ldap_value_free(values);
		return False;
	}
	
	ldap_value_free(values);
#ifdef DEBUG_PASSWORDS
	DEBUG (100, ("smbldap_get_single_attribute: [%s] = [%s]\n", attribute, value));
#endif	
	return True;
}

 BOOL smbldap_get_single_pstring (LDAP * ldap_struct, LDAPMessage * entry,
				  const char *attribute, pstring value)
{
	return smbldap_get_single_attribute(ldap_struct, entry,
					    attribute, value, 
					    sizeof(pstring));
}

 char * smbldap_talloc_single_attribute(LDAP *ldap_struct, LDAPMessage *entry,
					const char *attribute,
					TALLOC_CTX *mem_ctx)
{
	char **values;
	char *result;

	if (attribute == NULL) {
		return NULL;
	}

	values = ldap_get_values(ldap_struct, entry, attribute);

	if (values == NULL) {
		DEBUG(10, ("attribute %s does not exist\n", attribute));
		return NULL;
	}

	if (ldap_count_values(values) != 1) {
		DEBUG(10, ("attribute %s has %d values, expected only one\n",
			   attribute, ldap_count_values(values)));
		ldap_value_free(values);
		return NULL;
	}

	if (pull_utf8_talloc(mem_ctx, &result, values[0]) == (size_t)-1) {
		DEBUG(10, ("pull_utf8_talloc failed\n"));
		ldap_value_free(values);
		return NULL;
	}

	ldap_value_free(values);

#ifdef DEBUG_PASSWORDS
	DEBUG (100, ("smbldap_get_single_attribute: [%s] = [%s]\n",
		     attribute, result));
#endif	
	return result;
}

 static int ldapmsg_destructor(void *p) {
	LDAPMessage **result = talloc_get_type_abort(p, LDAPMessage *);
	ldap_msgfree(*result);
	return 0;
}

 void talloc_autofree_ldapmsg(TALLOC_CTX *mem_ctx, LDAPMessage *result)
{
	LDAPMessage **handle;

	if (result == NULL) {
		return;
	}

	handle = TALLOC_P(mem_ctx, LDAPMessage *);
	SMB_ASSERT(handle != NULL);

	*handle = result;
	talloc_set_destructor(handle, ldapmsg_destructor);
}

 static int ldapmod_destructor(void *p) {
	LDAPMod ***result = talloc_get_type_abort(p, LDAPMod **);
	ldap_mods_free(*result, True);
	return 0;
}

 void talloc_autofree_ldapmod(TALLOC_CTX *mem_ctx, LDAPMod **mod)
{
	LDAPMod ***handle;

	if (mod == NULL) {
		return;
	}

	handle = TALLOC_P(mem_ctx, LDAPMod **);
	SMB_ASSERT(handle != NULL);

	*handle = mod;
	talloc_set_destructor(handle, ldapmod_destructor);
}

/************************************************************************
 Routine to manage the LDAPMod structure array
 manage memory used by the array, by each struct, and values
 ***********************************************************************/

 void smbldap_set_mod (LDAPMod *** modlist, int modop, const char *attribute, const char *value)
{
	LDAPMod **mods;
	int i;
	int j;

	mods = *modlist;

	/* sanity checks on the mod values */

	if (attribute == NULL || *attribute == '\0') {
		return;	
	}

#if 0	/* commented out after discussion with abartlet.  Do not reenable.
	   left here so other do not re-add similar code   --jerry */
       	if (value == NULL || *value == '\0')
		return;
#endif

	if (mods == NULL) {
		mods = SMB_MALLOC_P(LDAPMod *);
		if (mods == NULL) {
			smb_panic("smbldap_set_mod: out of memory!\n");
			/* notreached. */
			abort();
		}
		mods[0] = NULL;
	}

	for (i = 0; mods[i] != NULL; ++i) {
		if (mods[i]->mod_op == modop && strequal(mods[i]->mod_type, attribute))
			break;
	}

	if (mods[i] == NULL) {
		mods = SMB_REALLOC_ARRAY (mods, LDAPMod *, i + 2);
		if (mods == NULL) {
			smb_panic("smbldap_set_mod: out of memory!\n");
			/* notreached. */
			abort();
		}
		mods[i] = SMB_MALLOC_P(LDAPMod);
		if (mods[i] == NULL) {
			smb_panic("smbldap_set_mod: out of memory!\n");
			/* notreached. */
			abort();
		}
		mods[i]->mod_op = modop;
		mods[i]->mod_values = NULL;
		mods[i]->mod_type = SMB_STRDUP(attribute);
		mods[i + 1] = NULL;
	}

	if (value != NULL) {
		char *utf8_value = NULL;

		j = 0;
		if (mods[i]->mod_values != NULL) {
			for (; mods[i]->mod_values[j] != NULL; j++);
		}
		mods[i]->mod_values = SMB_REALLOC_ARRAY(mods[i]->mod_values, char *, j + 2);
					       
		if (mods[i]->mod_values == NULL) {
			smb_panic("smbldap_set_mod: out of memory!\n");
			/* notreached. */
			abort();
		}

		if (push_utf8_allocate(&utf8_value, value) == (size_t)-1) {
			smb_panic("smbldap_set_mod: String conversion failure!\n");
			/* notreached. */
			abort();
		}

		mods[i]->mod_values[j] = utf8_value;

		mods[i]->mod_values[j + 1] = NULL;
	}
	*modlist = mods;
}

/**********************************************************************
  Set attribute to newval in LDAP, regardless of what value the
  attribute had in LDAP before.
*********************************************************************/

 void smbldap_make_mod(LDAP *ldap_struct, LDAPMessage *existing,
		      LDAPMod ***mods,
		      const char *attribute, const char *newval)
{
	char oldval[2048]; /* current largest allowed value is mungeddial */
	BOOL existed;

	if (attribute == NULL) {
		/* This can actually happen for ldapsam_compat where we for
		 * example don't have a password history */
		return;
	}

	if (existing != NULL) {
		existed = smbldap_get_single_attribute(ldap_struct, existing, attribute, oldval, sizeof(oldval));
	} else {
		existed = False;
		*oldval = '\0';
	}

	/* all of our string attributes are case insensitive */
	
	if (existed && newval && (StrCaseCmp(oldval, newval) == 0)) {
		
		/* Believe it or not, but LDAP will deny a delete and
		   an add at the same time if the values are the
		   same... */
		DEBUG(10,("smbldap_make_mod: attribute |%s| not changed.\n", attribute));
		return;
	}

	if (existed) {
		/* There has been no value before, so don't delete it.
		 * Here's a possible race: We might end up with
		 * duplicate attributes */
		/* By deleting exactly the value we found in the entry this
		 * should be race-free in the sense that the LDAP-Server will
		 * deny the complete operation if somebody changed the
		 * attribute behind our back. */
		/* This will also allow modifying single valued attributes 
		 * in Novell NDS. In NDS you have to first remove attribute and then
		 * you could add new value */
		
		DEBUG(10,("smbldap_make_mod: deleting attribute |%s| values |%s|\n", attribute, oldval));
		smbldap_set_mod(mods, LDAP_MOD_DELETE, attribute, oldval);
	}

	/* Regardless of the real operation (add or modify)
	   we add the new value here. We rely on deleting
	   the old value, should it exist. */

	if ((newval != NULL) && (strlen(newval) > 0)) {
		DEBUG(10,("smbldap_make_mod: adding attribute |%s| value |%s|\n", attribute, newval));
		smbldap_set_mod(mods, LDAP_MOD_ADD, attribute, newval);
	}
}

/**********************************************************************
 Some varients of the LDAP rebind code do not pass in the third 'arg' 
 pointer to a void*, so we try and work around it by assuming that the 
 value of the 'LDAP *' pointer is the same as the one we had passed in
 **********************************************************************/

struct smbldap_state_lookup {
	LDAP *ld;
	struct smbldap_state *smbldap_state;
	struct smbldap_state_lookup *prev, *next;
};

static struct smbldap_state_lookup *smbldap_state_lookup_list;

static struct smbldap_state *smbldap_find_state(LDAP *ld) 
{
	struct smbldap_state_lookup *t;

	for (t = smbldap_state_lookup_list; t; t = t->next) {
		if (t->ld == ld) {
			return t->smbldap_state;
		}
	}
	return NULL;
}

static void smbldap_delete_state(struct smbldap_state *smbldap_state) 
{
	struct smbldap_state_lookup *t;

	for (t = smbldap_state_lookup_list; t; t = t->next) {
		if (t->smbldap_state == smbldap_state) {
			DLIST_REMOVE(smbldap_state_lookup_list, t);
			SAFE_FREE(t);
			return;
		}
	}
}

static void smbldap_store_state(LDAP *ld, struct smbldap_state *smbldap_state) 
{
	struct smbldap_state *tmp_ldap_state;
	struct smbldap_state_lookup *t;
	struct smbldap_state_lookup *tmp;
	
	if ((tmp_ldap_state = smbldap_find_state(ld))) {
		SMB_ASSERT(tmp_ldap_state == smbldap_state);
		return;
	}

	t = SMB_XMALLOC_P(struct smbldap_state_lookup);
	ZERO_STRUCTP(t);
	
	DLIST_ADD_END(smbldap_state_lookup_list, t, tmp);
	t->ld = ld;
	t->smbldap_state = smbldap_state;
}

/********************************************************************
 start TLS on an existing LDAP connection
*******************************************************************/

int smb_ldap_start_tls(LDAP *ldap_struct, int version)
{ 
	int rc;
	
	if (lp_ldap_ssl() != LDAP_SSL_START_TLS) {
		return LDAP_SUCCESS;
	}
	
#ifdef LDAP_OPT_X_TLS
	if (version != LDAP_VERSION3) {
		DEBUG(0, ("Need LDAPv3 for Start TLS\n"));
		return LDAP_OPERATIONS_ERROR;
	}

	if ((rc = ldap_start_tls_s (ldap_struct, NULL, NULL)) != LDAP_SUCCESS)	{
		DEBUG(0,("Failed to issue the StartTLS instruction: %s\n",
			 ldap_err2string(rc)));
		return rc;
	}

	DEBUG (3, ("StartTLS issued: using a TLS connection\n"));
	return LDAP_SUCCESS;
#else
	DEBUG(0,("StartTLS not supported by LDAP client libraries!\n"));
	return LDAP_OPERATIONS_ERROR;
#endif
}

/********************************************************************
 setup a connection to the LDAP server based on a uri
*******************************************************************/

int smb_ldap_setup_conn(LDAP **ldap_struct, const char *uri)
{
	int rc;

	DEBUG(10, ("smb_ldap_setup_connection: %s\n", uri));
	
#ifdef HAVE_LDAP_INITIALIZE
	
	rc = ldap_initialize(ldap_struct, uri);
	if (rc) {
		DEBUG(0, ("ldap_initialize: %s\n", ldap_err2string(rc)));
	}

	return rc;
#else 

	/* Parse the string manually */

	{
		int port = 0;
		fstring protocol;
		fstring host;
		SMB_ASSERT(sizeof(protocol)>10 && sizeof(host)>254);


		/* skip leading "URL:" (if any) */
		if ( strnequal( uri, "URL:", 4 ) ) {
			uri += 4;
		}
		
		sscanf(uri, "%10[^:]://%254[^:/]:%d", protocol, host, &port);
		
		if (port == 0) {
			if (strequal(protocol, "ldap")) {
				port = LDAP_PORT;
			} else if (strequal(protocol, "ldaps")) {
				port = LDAPS_PORT;
			} else {
				DEBUG(0, ("unrecognised protocol (%s)!\n", protocol));
			}
		}
		
		if ((*ldap_struct = ldap_init(host, port)) == NULL)	{
			DEBUG(0, ("ldap_init failed !\n"));
			return LDAP_OPERATIONS_ERROR;
		}
		
	        if (strequal(protocol, "ldaps")) {
#ifdef LDAP_OPT_X_TLS
			int tls = LDAP_OPT_X_TLS_HARD;
			if (ldap_set_option (*ldap_struct, LDAP_OPT_X_TLS, &tls) != LDAP_SUCCESS)
			{
				DEBUG(0, ("Failed to setup a TLS session\n"));
			}
			
			DEBUG(3,("LDAPS option set...!\n"));
#else
			DEBUG(0,("smbldap_open_connection: Secure connection not supported by LDAP client libraries!\n"));
			return LDAP_OPERATIONS_ERROR;
#endif /* LDAP_OPT_X_TLS */
		}

	}
#endif /* HAVE_LDAP_INITIALIZE */
	return LDAP_SUCCESS;
}

/********************************************************************
 try to upgrade to Version 3 LDAP if not already, in either case return current
 version 
 *******************************************************************/

int smb_ldap_upgrade_conn(LDAP *ldap_struct, int *new_version) 
{
	int version;
	int rc;
	
	/* assume the worst */
	*new_version = LDAP_VERSION2;

	rc = ldap_get_option(ldap_struct, LDAP_OPT_PROTOCOL_VERSION, &version);
	if (rc) {
		return rc;
	}

	if (version == LDAP_VERSION3) {
		*new_version = LDAP_VERSION3;
		return LDAP_SUCCESS;
	}

	/* try upgrade */
	version = LDAP_VERSION3;
	rc = ldap_set_option (ldap_struct, LDAP_OPT_PROTOCOL_VERSION, &version);
	if (rc) {
		return rc;
	}
		
	*new_version = LDAP_VERSION3;
	return LDAP_SUCCESS;
}

/*******************************************************************
 open a connection to the ldap server (just until the bind)
 ******************************************************************/

int smb_ldap_setup_full_conn(LDAP **ldap_struct, const char *uri)
{
	int rc, version;

	rc = smb_ldap_setup_conn(ldap_struct, uri);
	if (rc) {
		return rc;
	}

	rc = smb_ldap_upgrade_conn(*ldap_struct, &version);
	if (rc) {
		return rc;
	}

	rc = smb_ldap_start_tls(*ldap_struct, version);
	if (rc) {
		return rc;
	}

	return LDAP_SUCCESS;
}

/*******************************************************************
 open a connection to the ldap server.
******************************************************************/
static int smbldap_open_connection (struct smbldap_state *ldap_state)

{
	int rc = LDAP_SUCCESS;
	int version;
	LDAP **ldap_struct = &ldap_state->ldap_struct;

	rc = smb_ldap_setup_conn(ldap_struct, ldap_state->uri);
	if (rc) {
		return rc;
	}

	/* Store the LDAP pointer in a lookup list */

	smbldap_store_state(*ldap_struct, ldap_state);

	/* Upgrade to LDAPv3 if possible */

	rc = smb_ldap_upgrade_conn(*ldap_struct, &version);
	if (rc) {
		return rc;
	}

	/* Start TLS if required */

	rc = smb_ldap_start_tls(*ldap_struct, version);
	if (rc) {
		return rc;
	}
	
	DEBUG(2, ("smbldap_open_connection: connection opened\n"));
	return rc;
}

/*******************************************************************
 a rebind function for authenticated referrals
 This version takes a void* that we can shove useful stuff in :-)
******************************************************************/
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)
#else
static int rebindproc_with_state  (LDAP * ld, char **whop, char **credp, 
				   int *methodp, int freeit, void *arg)
{
	struct smbldap_state *ldap_state = arg;
	
	/** @TODO Should we be doing something to check what servers we rebind to?
	    Could we get a referral to a machine that we don't want to give our
	    username and password to? */
	
	if (freeit) {
		SAFE_FREE(*whop);
		memset(*credp, '\0', strlen(*credp));
		SAFE_FREE(*credp);
	} else {
		DEBUG(5,("rebind_proc_with_state: Rebinding as \"%s\"\n", 
			  ldap_state->bind_dn));

		*whop = SMB_STRDUP(ldap_state->bind_dn);
		if (!*whop) {
			return LDAP_NO_MEMORY;
		}
		*credp = SMB_STRDUP(ldap_state->bind_secret);
		if (!*credp) {
			SAFE_FREE(*whop);
			return LDAP_NO_MEMORY;
		}
		*methodp = LDAP_AUTH_SIMPLE;
	}

	GetTimeOfDay(&ldap_state->last_rebind);
		
	return 0;
}
#endif /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/

/*******************************************************************
 a rebind function for authenticated referrals
 This version takes a void* that we can shove useful stuff in :-)
 and actually does the connection.
******************************************************************/
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)
static int rebindproc_connect_with_state (LDAP *ldap_struct, 
					  LDAP_CONST char *url, 
					  ber_tag_t request,
					  ber_int_t msgid, void *arg)
{
	struct smbldap_state *ldap_state = arg;
	int rc;
	int version;

	DEBUG(5,("rebindproc_connect_with_state: Rebinding to %s as \"%s\"\n", 
		 url, ldap_state->bind_dn));

	/* call START_TLS again (ldaps:// is handled by the OpenLDAP library
	 * itself) before rebinding to another LDAP server to avoid to expose
	 * our credentials. At least *try* to secure the connection - Guenther */

	smb_ldap_upgrade_conn(ldap_struct, &version);
	smb_ldap_start_tls(ldap_struct, version);

	/** @TODO Should we be doing something to check what servers we rebind to?
	    Could we get a referral to a machine that we don't want to give our
	    username and password to? */

	rc = ldap_simple_bind_s(ldap_struct, ldap_state->bind_dn, ldap_state->bind_secret);

	/* only set the last rebind timestamp when we did rebind after a
	 * non-read LDAP operation. That way we avoid the replication sleep
	 * after a simple redirected search operation - Guenther */

	switch (request) {

		case LDAP_REQ_MODIFY:
		case LDAP_REQ_ADD:
		case LDAP_REQ_DELETE:
		case LDAP_REQ_MODDN:
		case LDAP_REQ_EXTENDED:
			DEBUG(10,("rebindproc_connect_with_state: "
				"setting last_rebind timestamp "
				"(req: 0x%02x)\n", (unsigned int)request));
			GetTimeOfDay(&ldap_state->last_rebind);
			break;
		default:
			ZERO_STRUCT(ldap_state->last_rebind);
			break;
	}

	return rc;
}
#endif /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/

/*******************************************************************
 Add a rebind function for authenticated referrals
******************************************************************/
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)
#else
# if LDAP_SET_REBIND_PROC_ARGS == 2
static int rebindproc (LDAP *ldap_struct, char **whop, char **credp,
		       int *method, int freeit )
{
	struct smbldap_state *ldap_state = smbldap_find_state(ldap_struct);

	return rebindproc_with_state(ldap_struct, whop, credp,
				     method, freeit, ldap_state);
	
}
# endif /*LDAP_SET_REBIND_PROC_ARGS == 2*/
#endif /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/

/*******************************************************************
 a rebind function for authenticated referrals
 this also does the connection, but no void*.
******************************************************************/
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)
# if LDAP_SET_REBIND_PROC_ARGS == 2
static int rebindproc_connect (LDAP * ld, LDAP_CONST char *url, int request,
			       ber_int_t msgid)
{
	struct smbldap_state *ldap_state = smbldap_find_state(ld);

	return rebindproc_connect_with_state(ld, url, (ber_tag_t)request, msgid, 
					     ldap_state);
}
# endif /*LDAP_SET_REBIND_PROC_ARGS == 2*/
#endif /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/

/*******************************************************************
 connect to the ldap server under system privilege.
******************************************************************/
static int smbldap_connect_system(struct smbldap_state *ldap_state, LDAP * ldap_struct)
{
	int rc;
	char *ldap_dn;
	char *ldap_secret;
	int version;

	/* get the password */
	if (!fetch_ldap_pw(&ldap_dn, &ldap_secret)) {
		DEBUG(0, ("ldap_connect_system: Failed to retrieve password from secrets.tdb\n"));
		return LDAP_INVALID_CREDENTIALS;
	}

	ldap_state->bind_dn = ldap_dn;
	ldap_state->bind_secret = ldap_secret;

	/* removed the sasl_bind_s "EXTERNAL" stuff, as my testsuite 
	   (OpenLDAP) doesnt' seem to support it */
	   
	DEBUG(10,("ldap_connect_system: Binding to ldap server %s as \"%s\"\n",
		  ldap_state->uri, ldap_dn));

#ifdef HAVE_LDAP_SET_REBIND_PROC
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)
# if LDAP_SET_REBIND_PROC_ARGS == 2	
	ldap_set_rebind_proc(ldap_struct, &rebindproc_connect);	
# endif
# if LDAP_SET_REBIND_PROC_ARGS == 3	
	ldap_set_rebind_proc(ldap_struct, &rebindproc_connect_with_state, (void *)ldap_state);	
# endif
#else /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/
# if LDAP_SET_REBIND_PROC_ARGS == 2	
	ldap_set_rebind_proc(ldap_struct, &rebindproc);	
# endif
# if LDAP_SET_REBIND_PROC_ARGS == 3	
	ldap_set_rebind_proc(ldap_struct, &rebindproc_with_state, (void *)ldap_state);	
# endif
#endif /*defined(LDAP_API_FEATURE_X_OPENLDAP) && (LDAP_API_VERSION > 2000)*/
#endif

	rc = ldap_simple_bind_s(ldap_struct, ldap_dn, ldap_secret);

	if (rc != LDAP_SUCCESS) {
		char *ld_error = NULL;
		ldap_get_option(ldap_state->ldap_struct, LDAP_OPT_ERROR_STRING,
				&ld_error);
		DEBUG(ldap_state->num_failures ? 2 : 0,
		      ("failed to bind to server %s with dn=\"%s\" Error: %s\n\t%s\n",
			       ldap_state->uri,
			       ldap_dn ? ldap_dn : "(unknown)", ldap_err2string(rc),
			       ld_error ? ld_error : "(unknown)"));
		SAFE_FREE(ld_error);
		ldap_state->num_failures++;
		return rc;
	}

	ldap_state->num_failures = 0;
	ldap_state->paged_results = False;

	ldap_get_option(ldap_state->ldap_struct, LDAP_OPT_PROTOCOL_VERSION, &version);

	if (smbldap_has_control(ldap_state->ldap_struct, ADS_PAGE_CTL_OID) && version == 3) {
		ldap_state->paged_results = True;
	}

	DEBUG(3, ("ldap_connect_system: succesful connection to the LDAP server\n"));
	DEBUGADD(10, ("ldap_connect_system: LDAP server %s support paged results\n", 
		ldap_state->paged_results ? "does" : "does not"));
	return rc;
}

/**********************************************************************
 Connect to LDAP server (called before every ldap operation)
*********************************************************************/
static int smbldap_open(struct smbldap_state *ldap_state)
{
	int rc, opt_rc;
	BOOL reopen = False;
	SMB_ASSERT(ldap_state);
		
#ifndef NO_LDAP_SECURITY
	if (geteuid() != 0) {
		DEBUG(0, ("smbldap_open: cannot access LDAP when not root..\n"));
		return  LDAP_INSUFFICIENT_ACCESS;
	}
#endif

	if ((ldap_state->ldap_struct != NULL) && ((ldap_state->last_ping + SMBLDAP_DONT_PING_TIME) < time(NULL))) {

#ifdef HAVE_UNIXSOCKET
		struct sockaddr_un addr;
#else
		struct sockaddr addr;
#endif
		socklen_t len = sizeof(addr);
		int sd;

		opt_rc = ldap_get_option(ldap_state->ldap_struct, LDAP_OPT_DESC, &sd);
		if (opt_rc == 0 && (getpeername(sd, (struct sockaddr *) &addr, &len)) < 0 )
			reopen = True;

#ifdef HAVE_UNIXSOCKET
		if (opt_rc == 0 && addr.sun_family == AF_UNIX)
			reopen = True;
#endif
		if (reopen) {
		    	/* the other end has died. reopen. */
		    	ldap_unbind(ldap_state->ldap_struct);
		    	ldap_state->ldap_struct = NULL;
		    	ldap_state->last_ping = (time_t)0;
		} else {
			ldap_state->last_ping = time(NULL);
		} 
    	}

	if (ldap_state->ldap_struct != NULL) {
		DEBUG(11,("smbldap_open: already connected to the LDAP server\n"));
		return LDAP_SUCCESS;
	}

	if ((rc = smbldap_open_connection(ldap_state))) {
		return rc;
	}

	if ((rc = smbldap_connect_system(ldap_state, ldap_state->ldap_struct))) {
		ldap_unbind(ldap_state->ldap_struct);
		ldap_state->ldap_struct = NULL;
		return rc;
	}


	ldap_state->last_ping = time(NULL);
	ldap_state->pid = sys_getpid();
	DEBUG(4,("The LDAP server is succesfully connected\n"));

	return LDAP_SUCCESS;
}

/**********************************************************************
Disconnect from LDAP server 
*********************************************************************/
static NTSTATUS smbldap_close(struct smbldap_state *ldap_state)
{
	if (!ldap_state)
		return NT_STATUS_INVALID_PARAMETER;
		
	if (ldap_state->ldap_struct != NULL) {
		ldap_unbind(ldap_state->ldap_struct);
		ldap_state->ldap_struct = NULL;
	}

	smbldap_delete_state(ldap_state);
	
	DEBUG(5,("The connection to the LDAP server was closed\n"));
	/* maybe free the results here --metze */
	
	

	return NT_STATUS_OK;
}

static BOOL got_alarm;

static void (*old_handler)(int);

static void gotalarm_sig(int dummy)
{
	got_alarm = True;
}

static int another_ldap_try(struct smbldap_state *ldap_state, int *rc,
			    int *attempts, time_t endtime)
{
	time_t now = time(NULL);
	int open_rc = LDAP_SERVER_DOWN;

	if (*rc != LDAP_SERVER_DOWN)
		goto no_next;

	if (now >= endtime) {
		smbldap_close(ldap_state);
		*rc = LDAP_TIMEOUT;
		goto no_next;
	}

	if (*attempts == 0) {
		got_alarm = False;
		old_handler = CatchSignal(SIGALRM, gotalarm_sig);
		alarm(endtime - now);

		if (ldap_state->pid != sys_getpid())
			smbldap_close(ldap_state);
	}

	while (1) {

		if (*attempts != 0)
			smb_msleep(1000);

		*attempts += 1;

		open_rc = smbldap_open(ldap_state);

		if (open_rc == LDAP_SUCCESS) {
			ldap_state->last_use = now;
			return True;
		}

		if (open_rc == LDAP_INSUFFICIENT_ACCESS) {
			/* The fact that we are non-root or any other
			 * access-denied condition will not change in the next
			 * round of trying */
			*rc = open_rc;
			break;
		}

		if (got_alarm) {
			*rc = LDAP_TIMEOUT;
			break;
		}

		if (open_rc != LDAP_SUCCESS) {
			DEBUG(1, ("Connection to LDAP server failed for the "
				  "%d try!\n", *attempts));
		}
	}

 no_next:
	CatchSignal(SIGALRM, old_handler);
	alarm(0);
	ldap_state->last_use = now;
	return False;
}

/*********************************************************************
 ********************************************************************/

static int smbldap_search_ext(struct smbldap_state *ldap_state,
			      const char *base, int scope, const char *filter, 
			      const char *attrs[], int attrsonly,
			      LDAPControl **sctrls, LDAPControl **cctrls, 
			      int sizelimit, LDAPMessage **res)
{
	int 		rc = LDAP_SERVER_DOWN;
	int 		attempts = 0;
	char           *utf8_filter;
	time_t		endtime = time(NULL)+lp_ldap_timeout();
	struct		timeval timeout;

	SMB_ASSERT(ldap_state);
	
	DEBUG(5,("smbldap_search_ext: base => [%s], filter => [%s], "
		 "scope => [%d]\n", base, filter, scope));

	if (ldap_state->last_rebind.tv_sec > 0) {
		struct timeval	tval;
		SMB_BIG_INT	tdiff = 0;
		int		sleep_time = 0;

		ZERO_STRUCT(tval);
		GetTimeOfDay(&tval);

		tdiff = usec_time_diff(&tval, &ldap_state->last_rebind);
		tdiff /= 1000; /* Convert to milliseconds. */

		sleep_time = lp_ldap_replication_sleep()-(int)tdiff;
		sleep_time = MIN(sleep_time, MAX_LDAP_REPLICATION_SLEEP_TIME);

		if (sleep_time > 0) {
			/* we wait for the LDAP replication */
			DEBUG(5,("smbldap_search_ext: waiting %d milliseconds "
				 "for LDAP replication.\n",sleep_time));
			smb_msleep(sleep_time);
			DEBUG(5,("smbldap_search_ext: go on!\n"));
		}
		ZERO_STRUCT(ldap_state->last_rebind);
	}

	if (push_utf8_allocate(&utf8_filter, filter) == (size_t)-1) {
		return LDAP_NO_MEMORY;
	}

	/* Setup timeout for the ldap_search_ext_s call - local and remote. */
	timeout.tv_sec = lp_ldap_timeout();
	timeout.tv_usec = 0;

	/* Setup alarm timeout.... Do we need both of these ? JRA.
	 * Yes, I think we do need both of these. The server timeout only
	 * covers the case where the server's operation takes too long. It
	 * does not cover the case where the request hangs on its way to the
	 * server. The server side timeout is not strictly necessary, it's
	 * just a bit more kind to the server. VL. */

	got_alarm = 0;
	CatchSignal(SIGALRM, SIGNAL_CAST gotalarm_sig);
	alarm(lp_ldap_timeout());
	/* End setup timeout. */

	while (another_ldap_try(ldap_state, &rc, &attempts, endtime)) {
		rc = ldap_search_ext_s(ldap_state->ldap_struct, base, scope, 
				       utf8_filter,
				       CONST_DISCARD(char **, attrs),
				       attrsonly, sctrls, cctrls, &timeout,
				       sizelimit, res);
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->ldap_struct,
					LDAP_OPT_ERROR_STRING, &ld_error);
			DEBUG(10,("Failed search for base: %s, error: %s "
				  "(%s)\n", base, ldap_err2string(rc),
				  ld_error ? ld_error : "unknown"));
			SAFE_FREE(ld_error);
		}
	}

	SAFE_FREE(utf8_filter);

	/* Teardown timeout. */
	CatchSignal(SIGALRM, SIGNAL_CAST SIG_IGN);
	alarm(0);

	if (got_alarm != 0)
		return LDAP_TIMELIMIT_EXCEEDED;

	return rc;
}

int smbldap_search(struct smbldap_state *ldap_state, 
		   const char *base, int scope, const char *filter, 
		   const char *attrs[], int attrsonly, 
		   LDAPMessage **res)
{
	return smbldap_search_ext(ldap_state, base, scope, filter, attrs,
				  attrsonly, NULL, NULL, LDAP_NO_LIMIT, res);
}

int smbldap_search_paged(struct smbldap_state *ldap_state, 
			 const char *base, int scope, const char *filter, 
			 const char **attrs, int attrsonly, int pagesize,
			 LDAPMessage **res, void **cookie)
{
	LDAPControl     pr;
	LDAPControl 	**rcontrols;
	LDAPControl 	*controls[2] = { NULL, NULL};
	BerElement 	*cookie_be = NULL;
	struct berval 	*cookie_bv = NULL;
	int		tmp = 0, i, rc;
	BOOL 		critical = True;

	*res = NULL;

	DEBUG(3,("smbldap_search_paged: base => [%s], filter => [%s],"
		 "scope => [%d], pagesize => [%d]\n",
		 base, filter, scope, pagesize));

	cookie_be = ber_alloc_t(LBER_USE_DER);
	if (cookie_be == NULL) {
		DEBUG(0,("smbldap_create_page_control: ber_alloc_t returns "
			 "NULL\n"));
		return LDAP_NO_MEMORY;
	}

	/* construct cookie */
	if (*cookie != NULL) {
		ber_printf(cookie_be, "{iO}", (ber_int_t) pagesize, *cookie);
		ber_bvfree(*cookie); /* don't need it from last time */
		*cookie = NULL;
	} else {
		ber_printf(cookie_be, "{io}", (ber_int_t) pagesize, "", 0);
	}
	ber_flatten(cookie_be, &cookie_bv);

	pr.ldctl_oid = CONST_DISCARD(char *, ADS_PAGE_CTL_OID);
	pr.ldctl_iscritical = (char) critical;
	pr.ldctl_value.bv_len = cookie_bv->bv_len;
	pr.ldctl_value.bv_val = cookie_bv->bv_val;

	controls[0] = &pr;
	controls[1] = NULL;

	rc = smbldap_search_ext(ldap_state, base, scope, filter, attrs, 
				 0, controls, NULL, LDAP_NO_LIMIT, res);

	ber_free(cookie_be, 1);
	ber_bvfree(cookie_bv);

	if (rc != 0) {
		DEBUG(3,("smbldap_search_paged: smbldap_search_ext(%s) "
			 "failed with [%s]\n", filter, ldap_err2string(rc)));
		goto done;
	}

	DEBUG(3,("smbldap_search_paged: search was successfull\n"));

	rc = ldap_parse_result(ldap_state->ldap_struct, *res, NULL, NULL, 
			       NULL, NULL, &rcontrols,  0);
	if (rc != 0) {
		DEBUG(3,("smbldap_search_paged: ldap_parse_result failed " \
			 "with [%s]\n", ldap_err2string(rc)));
		goto done;
	}

	if (rcontrols == NULL)
		goto done;

	for (i=0; rcontrols[i]; i++) {

		if (strcmp(ADS_PAGE_CTL_OID, rcontrols[i]->ldctl_oid) != 0)
			continue;

		cookie_be = ber_init(&rcontrols[i]->ldctl_value);
		ber_scanf(cookie_be,"{iO}", &tmp, &cookie_bv);
		/* the berval is the cookie, but must be freed when it is all
		   done */
		if (cookie_bv->bv_len)
			*cookie=ber_bvdup(cookie_bv);
		else
			*cookie=NULL;
		ber_bvfree(cookie_bv);
		ber_free(cookie_be, 1);
		break;
	}
	ldap_controls_free(rcontrols);
done:	
	return rc;
}

int smbldap_modify(struct smbldap_state *ldap_state, const char *dn, LDAPMod *attrs[])
{
	int 		rc = LDAP_SERVER_DOWN;
	int 		attempts = 0;
	char           *utf8_dn;
	time_t		endtime = time(NULL)+lp_ldap_timeout();

	SMB_ASSERT(ldap_state);

	DEBUG(5,("smbldap_modify: dn => [%s]\n", dn ));

	if (push_utf8_allocate(&utf8_dn, dn) == (size_t)-1) {
		return LDAP_NO_MEMORY;
	}

	while (another_ldap_try(ldap_state, &rc, &attempts, endtime)) {
		rc = ldap_modify_s(ldap_state->ldap_struct, utf8_dn, attrs);
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->ldap_struct,
					LDAP_OPT_ERROR_STRING, &ld_error);
			DEBUG(10,("Failed to modify dn: %s, error: %s "
				  "(%s)\n", dn, ldap_err2string(rc),
				  ld_error ? ld_error : "unknown"));
			SAFE_FREE(ld_error);
		}
	}
		
	SAFE_FREE(utf8_dn);
	return rc;
}

int smbldap_add(struct smbldap_state *ldap_state, const char *dn, LDAPMod *attrs[])
{
	int 		rc = LDAP_SERVER_DOWN;
	int 		attempts = 0;
	char           *utf8_dn;
	time_t		endtime = time(NULL)+lp_ldap_timeout();
	
	SMB_ASSERT(ldap_state);

	DEBUG(5,("smbldap_add: dn => [%s]\n", dn ));

	if (push_utf8_allocate(&utf8_dn, dn) == (size_t)-1) {
		return LDAP_NO_MEMORY;
	}

	while (another_ldap_try(ldap_state, &rc, &attempts, endtime)) {
		rc = ldap_add_s(ldap_state->ldap_struct, utf8_dn, attrs);
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->ldap_struct,
					LDAP_OPT_ERROR_STRING, &ld_error);
			DEBUG(10,("Failed to add dn: %s, error: %s "
				  "(%s)\n", dn, ldap_err2string(rc),
				  ld_error ? ld_error : "unknown"));
			SAFE_FREE(ld_error);
		}
	}
	
	SAFE_FREE(utf8_dn);
	return rc;
}

int smbldap_delete(struct smbldap_state *ldap_state, const char *dn)
{
	int 		rc = LDAP_SERVER_DOWN;
	int 		attempts = 0;
	char           *utf8_dn;
	time_t		endtime = time(NULL)+lp_ldap_timeout();
	
	SMB_ASSERT(ldap_state);

	DEBUG(5,("smbldap_delete: dn => [%s]\n", dn ));

	if (push_utf8_allocate(&utf8_dn, dn) == (size_t)-1) {
		return LDAP_NO_MEMORY;
	}

	while (another_ldap_try(ldap_state, &rc, &attempts, endtime)) {
		rc = ldap_delete_s(ldap_state->ldap_struct, utf8_dn);
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->ldap_struct,
					LDAP_OPT_ERROR_STRING, &ld_error);
			DEBUG(10,("Failed to delete dn: %s, error: %s "
				  "(%s)\n", dn, ldap_err2string(rc),
				  ld_error ? ld_error : "unknown"));
			SAFE_FREE(ld_error);
		}
	}
	
	SAFE_FREE(utf8_dn);
	return rc;
}

int smbldap_extended_operation(struct smbldap_state *ldap_state, 
			       LDAP_CONST char *reqoid, struct berval *reqdata, 
			       LDAPControl **serverctrls, LDAPControl **clientctrls, 
			       char **retoidp, struct berval **retdatap)
{
	int 		rc = LDAP_SERVER_DOWN;
	int 		attempts = 0;
	time_t		endtime = time(NULL)+lp_ldap_timeout();
	
	if (!ldap_state)
		return (-1);

	while (another_ldap_try(ldap_state, &rc, &attempts, endtime)) {
		rc = ldap_extended_operation_s(ldap_state->ldap_struct, reqoid,
					       reqdata, serverctrls,
					       clientctrls, retoidp, retdatap);
		if (rc != LDAP_SUCCESS) {
			char *ld_error = NULL;
			ldap_get_option(ldap_state->ldap_struct,
					LDAP_OPT_ERROR_STRING, &ld_error);
			DEBUG(10,("Extended operation failed with error: %s "
				  "(%s)\n", ldap_err2string(rc),
				  ld_error ? ld_error : "unknown"));
			SAFE_FREE(ld_error);
		}
	}
		
	return rc;
}

/*******************************************************************
 run the search by name.
******************************************************************/
int smbldap_search_suffix (struct smbldap_state *ldap_state,
			   const char *filter, const char **search_attr,
			   LDAPMessage ** result)
{
	return smbldap_search(ldap_state, lp_ldap_suffix(), LDAP_SCOPE_SUBTREE,
			      filter, search_attr, 0, result);
}

static void smbldap_idle_fn(void **data, time_t *interval, time_t now)
{
	struct smbldap_state *state = (struct smbldap_state *)(*data);

	if (state->ldap_struct == NULL) {
		DEBUG(10,("ldap connection not connected...\n"));
		return;
	}
		
	if ((state->last_use+SMBLDAP_IDLE_TIME) > now) {
		DEBUG(10,("ldap connection not idle...\n"));
		return;
	}
		
	DEBUG(7,("ldap connection idle...closing connection\n"));
	smbldap_close(state);
}

/**********************************************************************
 Housekeeping
 *********************************************************************/

void smbldap_free_struct(struct smbldap_state **ldap_state) 
{
	smbldap_close(*ldap_state);
	
	if ((*ldap_state)->bind_secret) {
		memset((*ldap_state)->bind_secret, '\0', strlen((*ldap_state)->bind_secret));
	}

	SAFE_FREE((*ldap_state)->bind_dn);
	SAFE_FREE((*ldap_state)->bind_secret);

	smb_unregister_idle_event((*ldap_state)->event_id);

	*ldap_state = NULL;

	/* No need to free any further, as it is talloc()ed */
}


/**********************************************************************
 Intitalise the 'general' ldap structures, on which ldap operations may be conducted
 *********************************************************************/

NTSTATUS smbldap_init(TALLOC_CTX *mem_ctx, const char *location, struct smbldap_state **smbldap_state) 
{
	*smbldap_state = TALLOC_ZERO_P(mem_ctx, struct smbldap_state);
	if (!*smbldap_state) {
		DEBUG(0, ("talloc() failed for ldapsam private_data!\n"));
		return NT_STATUS_NO_MEMORY;
	}

	if (location) {
		(*smbldap_state)->uri = talloc_strdup(mem_ctx, location);
	} else {
		(*smbldap_state)->uri = "ldap://localhost";
	}

	(*smbldap_state)->event_id =
		smb_register_idle_event(smbldap_idle_fn, (void *)(*smbldap_state),
					SMBLDAP_IDLE_TIME);

	if ((*smbldap_state)->event_id == SMB_EVENT_ID_INVALID) {
		DEBUG(0,("Failed to register LDAP idle event!\n"));
		return NT_STATUS_INVALID_HANDLE;
	}

	return NT_STATUS_OK;
}

/*******************************************************************
 Return a copy of the DN for a LDAPMessage. Convert from utf8 to CH_UNIX.
********************************************************************/
char *smbldap_get_dn(LDAP *ld, LDAPMessage *entry)
{
	char *utf8_dn, *unix_dn;

	utf8_dn = ldap_get_dn(ld, entry);
	if (!utf8_dn) {
		DEBUG (5, ("smbldap_get_dn: ldap_get_dn failed\n"));
		return NULL;
	}
	if (pull_utf8_allocate(&unix_dn, utf8_dn) == (size_t)-1) {
		DEBUG (0, ("smbldap_get_dn: String conversion failure utf8 [%s]\n", utf8_dn));
		return NULL;
	}
	ldap_memfree(utf8_dn);
	return unix_dn;
}

 const char *smbldap_talloc_dn(TALLOC_CTX *mem_ctx, LDAP *ld,
			       LDAPMessage *entry)
{
	char *utf8_dn, *unix_dn;

	utf8_dn = ldap_get_dn(ld, entry);
	if (!utf8_dn) {
		DEBUG (5, ("smbldap_get_dn: ldap_get_dn failed\n"));
		return NULL;
	}
	if (pull_utf8_talloc(mem_ctx, &unix_dn, utf8_dn) == (size_t)-1) {
		DEBUG (0, ("smbldap_get_dn: String conversion failure utf8 "
			   "[%s]\n", utf8_dn));
		return NULL;
	}
	ldap_memfree(utf8_dn);
	return unix_dn;
}

/*******************************************************************
 Check if root-dse has a certain Control or Extension
********************************************************************/

static BOOL smbldap_check_root_dse(LDAP *ld, const char **attrs, const char *value) 
{
	LDAPMessage *msg = NULL;
	LDAPMessage *entry = NULL;
	char **values = NULL;
	int rc, num_result, num_values, i;
	BOOL result = False;

	if (!attrs[0]) {
		DEBUG(3,("smbldap_check_root_dse: nothing to look for\n"));
		return False;
	}

	if (!strequal(attrs[0], "supportedExtension") && 
	    !strequal(attrs[0], "supportedControl") && 
	    !strequal(attrs[0], "namingContexts")) {
		DEBUG(3,("smbldap_check_root_dse: no idea what to query root-dse for: %s ?\n", attrs[0]));
		return False;
	}

	rc = ldap_search_s(ld, "", LDAP_SCOPE_BASE, 
			   "(objectclass=*)", CONST_DISCARD(char **, attrs), 0 , &msg);

	if (rc != LDAP_SUCCESS) {
		DEBUG(3,("smbldap_check_root_dse: Could not search rootDSE\n"));
		return False;
	}

	num_result = ldap_count_entries(ld, msg);

	if (num_result != 1) {
		DEBUG(3,("smbldap_check_root_dse: Expected one rootDSE, got %d\n", num_result));
		goto done;
	}

	entry = ldap_first_entry(ld, msg);

	if (entry == NULL) {
		DEBUG(3,("smbldap_check_root_dse: Could not retrieve rootDSE\n"));
		goto done;
	}

	values = ldap_get_values(ld, entry, attrs[0]);

	if (values == NULL) {
		DEBUG(5,("smbldap_check_root_dse: LDAP Server does not support any %s\n", attrs[0]));
		goto done;
	}

	num_values = ldap_count_values(values);

	if (num_values == 0) {
		DEBUG(5,("smbldap_check_root_dse: LDAP Server does not have any %s\n", attrs[0]));
		goto done;
	}

	for (i=0; i<num_values; i++) {
		if (strcmp(values[i], value) == 0)
			result = True;
	}


 done:
	if (values != NULL)
		ldap_value_free(values);
	if (msg != NULL)
		ldap_msgfree(msg);

	return result;

}

/*******************************************************************
 Check if LDAP-Server supports a certain Control (OID in string format)
********************************************************************/

BOOL smbldap_has_control(LDAP *ld, const char *control)
{
	const char *attrs[] = { "supportedControl", NULL };
	return smbldap_check_root_dse(ld, attrs, control);
}

/*******************************************************************
 Check if LDAP-Server supports a certain Extension (OID in string format)
********************************************************************/

BOOL smbldap_has_extension(LDAP *ld, const char *extension)
{
	const char *attrs[] = { "supportedExtension", NULL };
	return smbldap_check_root_dse(ld, attrs, extension);
}

/*******************************************************************
 Check if LDAP-Server holds a given namingContext
********************************************************************/

BOOL smbldap_has_naming_context(LDAP *ld, const char *naming_context)
{
	const char *attrs[] = { "namingContexts", NULL };
	return smbldap_check_root_dse(ld, attrs, naming_context);
}
