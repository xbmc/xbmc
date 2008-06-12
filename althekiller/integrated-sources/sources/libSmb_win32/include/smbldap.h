/* 
   Unix SMB/CIFS mplementation.
   LDAP protocol helper functions for SAMBA
   Copyright (C) Gerald Carter			2001-2003
    
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

#ifndef _SMBLDAP_H
#define _SMBLDAP_H

struct smbldap_state;

#ifdef HAVE_LDAP

/* specify schema versions between 2.2. and 3.0 */

#define SCHEMAVER_SAMBAACCOUNT		1
#define SCHEMAVER_SAMBASAMACCOUNT	2

/* objectclass names */

#define LDAP_OBJ_SAMBASAMACCOUNT	"sambaSamAccount"
#define LDAP_OBJ_SAMBAACCOUNT		"sambaAccount"
#define LDAP_OBJ_GROUPMAP		"sambaGroupMapping"
#define LDAP_OBJ_DOMINFO		"sambaDomain"
#define LDAP_OBJ_IDPOOL			"sambaUnixIdPool"
#define LDAP_OBJ_IDMAP_ENTRY		"sambaIdmapEntry"
#define LDAP_OBJ_SID_ENTRY		"sambaSidEntry"
#define LDAP_OBJ_TRUST_PASSWORD         "sambaTrustPassword"

#define LDAP_OBJ_ACCOUNT		"account"
#define LDAP_OBJ_POSIXACCOUNT		"posixAccount"
#define LDAP_OBJ_POSIXGROUP		"posixGroup"
#define LDAP_OBJ_OU			"organizationalUnit"

/* some generic attributes that get reused a lot */

#define LDAP_ATTRIBUTE_SID		"sambaSID"
#define LDAP_ATTRIBUTE_UIDNUMBER	"uidNumber"
#define LDAP_ATTRIBUTE_GIDNUMBER	"gidNumber"
#define LDAP_ATTRIBUTE_SID_LIST		"sambaSIDList"

/* attribute map table indexes */

#define LDAP_ATTR_LIST_END		0
#define LDAP_ATTR_UID			1
#define LDAP_ATTR_UIDNUMBER		2
#define LDAP_ATTR_GIDNUMBER		3
#define LDAP_ATTR_UNIX_HOME		4
#define LDAP_ATTR_PWD_LAST_SET		5
#define LDAP_ATTR_PWD_CAN_CHANGE	6
#define LDAP_ATTR_PWD_MUST_CHANGE	7
#define LDAP_ATTR_LOGON_TIME		8
#define LDAP_ATTR_LOGOFF_TIME		9
#define LDAP_ATTR_KICKOFF_TIME		10
#define LDAP_ATTR_CN			11
#define LDAP_ATTR_DISPLAY_NAME		12
#define LDAP_ATTR_HOME_PATH		13
#define LDAP_ATTR_LOGON_SCRIPT		14
#define LDAP_ATTR_PROFILE_PATH		15
#define LDAP_ATTR_DESC			16
#define LDAP_ATTR_USER_WKS		17
#define LDAP_ATTR_USER_SID		18
#define LDAP_ATTR_USER_RID		18
#define LDAP_ATTR_PRIMARY_GROUP_SID	19
#define LDAP_ATTR_PRIMARY_GROUP_RID	20
#define LDAP_ATTR_LMPW			21
#define LDAP_ATTR_NTPW			22
#define LDAP_ATTR_DOMAIN		23
#define LDAP_ATTR_OBJCLASS		24
#define LDAP_ATTR_ACB_INFO		25
#define LDAP_ATTR_NEXT_USERRID		26
#define LDAP_ATTR_NEXT_GROUPRID		27
#define LDAP_ATTR_DOM_SID		28
#define LDAP_ATTR_HOME_DRIVE		29
#define LDAP_ATTR_GROUP_SID		30
#define LDAP_ATTR_GROUP_TYPE		31
#define LDAP_ATTR_SID			32
#define LDAP_ATTR_ALGORITHMIC_RID_BASE  33
#define LDAP_ATTR_NEXT_RID              34
#define LDAP_ATTR_BAD_PASSWORD_COUNT	35
#define LDAP_ATTR_LOGON_COUNT		36
#define LDAP_ATTR_MUNGED_DIAL		37
#define LDAP_ATTR_BAD_PASSWORD_TIME	38
#define LDAP_ATTR_PWD_HISTORY           39
#define LDAP_ATTR_SID_LIST		40
#define LDAP_ATTR_MOD_TIMESTAMP         41
#define LDAP_ATTR_LOGON_HOURS		42 
#define LDAP_ATTR_TRUST_PASSWD_FLAGS    43
#define LDAP_ATTR_SN			44


typedef struct _attrib_map_entry {
	int		attrib;
	const char 	*name;
} ATTRIB_MAP_ENTRY;


/* structures */

extern ATTRIB_MAP_ENTRY attrib_map_v22[];
extern ATTRIB_MAP_ENTRY attrib_map_to_delete_v22[];
extern ATTRIB_MAP_ENTRY attrib_map_v30[];
extern ATTRIB_MAP_ENTRY attrib_map_to_delete_v30[];
extern ATTRIB_MAP_ENTRY dominfo_attr_list[];
extern ATTRIB_MAP_ENTRY groupmap_attr_list[];
extern ATTRIB_MAP_ENTRY groupmap_attr_list_to_delete[];
extern ATTRIB_MAP_ENTRY idpool_attr_list[];
extern ATTRIB_MAP_ENTRY sidmap_attr_list[];
extern ATTRIB_MAP_ENTRY trustpw_attr_list[];


/* Function declarations -- not included in proto.h so we don't
   have to worry about LDAP structure types */

NTSTATUS smbldap_init(TALLOC_CTX *mem_ctx,
                      const char *location,
                      struct smbldap_state **smbldap_state);

const char* get_attr_key2string( ATTRIB_MAP_ENTRY table[], int key );
const char** get_attr_list( TALLOC_CTX *mem_ctx, ATTRIB_MAP_ENTRY table[] );
void smbldap_set_mod (LDAPMod *** modlist, int modop, const char *attribute, const char *value);
void smbldap_make_mod(LDAP *ldap_struct, LDAPMessage *existing,
		      LDAPMod ***mods,
		      const char *attribute, const char *newval);
BOOL smbldap_get_single_attribute (LDAP * ldap_struct, LDAPMessage * entry,
				   const char *attribute, char *value,
				   int max_len);
BOOL smbldap_get_single_pstring (LDAP * ldap_struct, LDAPMessage * entry,
				 const char *attribute, pstring value);
char *smbldap_get_dn(LDAP *ld, LDAPMessage *entry);
int smbldap_modify(struct smbldap_state *ldap_state,
                   const char *dn,
                   LDAPMod *attrs[]);

/**
 * Struct to keep the state for all the ldap stuff 
 *
 */

struct smbldap_state {
	LDAP *ldap_struct;
	pid_t pid;
	time_t last_ping;
	/* retrive-once info */
	const char *uri;
	char *bind_dn;
	char *bind_secret;
	BOOL paged_results;

	unsigned int num_failures;

	time_t last_use;
	smb_event_id_t event_id;

	struct timeval last_rebind;
};

/* struct used by both pdb_ldap.c and pdb_nds.c */

struct ldapsam_privates {
	struct smbldap_state *smbldap_state;

	/* Former statics */
	LDAPMessage *result;
	LDAPMessage *entry;
	int index;

	const char *domain_name;
	DOM_SID domain_sid;

	/* configuration items */
	int schema_ver;

	char *domain_dn;

	/* Is this NDS ldap? */
	int is_nds_ldap;

	/* ldap server location parameter */
	char *location;
};

/* Functions shared between pdb_ldap.c and pdb_nds.c. */
NTSTATUS pdb_init_ldapsam_compat( struct pdb_methods **pdb_method, const char *location);
void private_data_free_fn(void **result);
int ldapsam_search_suffix_by_name(struct ldapsam_privates *ldap_state,
                                  const char *user,
                                  LDAPMessage ** result,
                                  const char **attr);
NTSTATUS pdb_init_ldapsam( struct pdb_methods **pdb_method, const char *location);
const char** get_userattr_list( TALLOC_CTX *mem_ctx, int schema_ver );

char * smbldap_talloc_single_attribute(LDAP *ldap_struct, LDAPMessage *entry,
				       const char *attribute,
				       TALLOC_CTX *mem_ctx);
void talloc_autofree_ldapmsg(TALLOC_CTX *mem_ctx, LDAPMessage *result);
void talloc_autofree_ldapmod(TALLOC_CTX *mem_ctx, LDAPMod **mod);
const char *smbldap_talloc_dn(TALLOC_CTX *mem_ctx, LDAP *ld,
			      LDAPMessage *entry);


#endif 	/* HAVE_LDAP */

#define LDAP_CONNECT_DEFAULT_TIMEOUT   15
#define LDAP_PAGE_SIZE 1024

#endif	/* _SMBLDAP_H */
