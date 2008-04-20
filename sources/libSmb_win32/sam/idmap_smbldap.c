/*
   Unix SMB/CIFS implementation.

   idmap LDAP backend

   Copyright (C) Tim Potter 		2000
   Copyright (C) Jim McDonough <jmcd@us.ibm.com>	2003
   Copyright (C) Simo Sorce 		2003
   Copyright (C) Gerald Carter 		2003

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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

struct ldap_connection *ldap_conn = NULL;

/* number tries while allocating new id */
#define LDAP_MAX_ALLOC_ID 128


/***********************************************************************
 This function cannot be called to modify a mapping, only set a new one
***********************************************************************/

static NTSTATUS ldap_set_mapping(const DOM_SID *sid, unid_t id, int id_type)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	pstring id_str;
	const char *type;
	fstring sid_string;
	struct ldap_message *msg;
	struct ldap_message *mod_res = NULL;
	char *mod;

	type = (id_type & ID_USERID) ? "uidNumber" : "gidNumber";

	sid_to_string( sid_string, sid );

	pstr_sprintf(id_str, "%lu",
		     ((id_type & ID_USERID) ?
		      (unsigned long)id.uid : (unsigned long)id.gid));

	asprintf(&mod,
		 "dn: sambaSID=%s,%s\n"
		 "changetype: add\n"
		 "objectClass: sambaIdmapEntry\n"
		 "objectClass: sambaSidEntry\n"
		 "sambaSID: %s\n"
		 "%s: %lu\n",
		 sid_string, lp_ldap_idmap_suffix(), sid_string, type,
		 ((id_type & ID_USERID) ?
		  (unsigned long)id.uid : (unsigned long)id.gid));

	msg = ldap_ldif2msg(mod);

	SAFE_FREE(mod);

	if (msg == NULL)
		return NT_STATUS_NO_MEMORY;

	mod_res = ldap_transaction(ldap_conn, msg);

	if ((mod_res == NULL) || (mod_res->r.ModifyResponse.resultcode != 0))
		goto out;

	ret = NT_STATUS_OK;
 out:
	destroy_ldap_message(msg);
	destroy_ldap_message(mod_res);
	return ret;
}

/*****************************************************************************
 Allocate a new uid or gid
*****************************************************************************/

static NTSTATUS ldap_allocate_id(unid_t *id, int id_type)
{
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	uid_t	luid, huid;
	gid_t	lgid, hgid;
	const char *attrs[] = { "uidNumber", "gidNumber" };
	struct ldap_message *idpool_s = NULL;
	struct ldap_message *idpool = NULL;
	struct ldap_message *mod_msg = NULL;
	struct ldap_message *mod_res = NULL;
	int value;
	const char *id_attrib;
	char *mod;

	id_attrib = (id_type & ID_USERID) ? "uidNumber" : "gidNumber";

	idpool_s = new_ldap_search_message(lp_ldap_suffix(),
					   LDAP_SEARCH_SCOPE_SUB,
					   "(objectclass=sambaUnixIdPool)",
					   2, attrs);

	if (idpool_s == NULL)
		return NT_STATUS_NO_MEMORY;

	idpool = ldap_searchone(ldap_conn, idpool_s, NULL);

	if (idpool == NULL)
		goto out;

	if (!ldap_find_single_int(idpool, id_attrib, &value))
		goto out;

	/* this must succeed or else we wouldn't have initialized */
		
	lp_idmap_uid( &luid, &huid);
	lp_idmap_gid( &lgid, &hgid);
	
	/* make sure we still have room to grow */
	
	if (id_type & ID_USERID) {
		id->uid = value;
		if (id->uid > huid ) {
			DEBUG(0,("ldap_allocate_id: Cannot allocate uid "
				 "above %lu!\n",  (unsigned long)huid));
			goto out;
		}
	}
	else { 
		id->gid = value;
		if (id->gid > hgid ) {
			DEBUG(0,("ldap_allocate_id: Cannot allocate gid "
				 "above %lu!\n", (unsigned long)hgid));
			goto out;
		}
	}
	
	asprintf(&mod,
		 "dn: %s\n"
		 "changetype: modify\n"
		 "delete: %s\n"
		 "%s: %d\n"
		 "-\n"
		 "add: %s\n"
		 "%s: %d\n",
		 idpool->r.SearchResultEntry.dn, id_attrib, id_attrib, value,
		 id_attrib, id_attrib, value+1);

	mod_msg = ldap_ldif2msg(mod);

	SAFE_FREE(mod);

	if (mod_msg == NULL)
		goto out;

	mod_res = ldap_transaction(ldap_conn, mod_msg);

	if ((mod_res == NULL) || (mod_res->r.ModifyResponse.resultcode != 0))
		goto out;

	ret = NT_STATUS_OK;
out:
	destroy_ldap_message(idpool_s);
	destroy_ldap_message(idpool);
	destroy_ldap_message(mod_msg);
	destroy_ldap_message(mod_res);

	return ret;
}

/*****************************************************************************
 get a sid from an id
*****************************************************************************/

static NTSTATUS ldap_get_sid_from_id(DOM_SID *sid, unid_t id, int id_type)
{
	pstring filter;
	const char *type;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	const char *attr_list[] = { "sambaSID" };
	struct ldap_message *msg;
	struct ldap_message *entry = NULL;
	char *sid_str;

	type = (id_type & ID_USERID) ? "uidNumber" : "gidNumber";

	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%lu))", "sambaIdmapEntry",
		     type,
		     ((id_type & ID_USERID) ?
		      (unsigned long)id.uid : (unsigned long)id.gid));

	msg = new_ldap_search_message(lp_ldap_idmap_suffix(),
				      LDAP_SEARCH_SCOPE_SUB,
				      filter, 1, attr_list);

	if (msg == NULL)
		return NT_STATUS_NO_MEMORY;

	entry = ldap_searchone(ldap_conn, msg, NULL);

	if (entry == NULL)
		goto out;

	if (!ldap_find_single_string(entry, "sambaSID", entry->mem_ctx,
				     &sid_str))
		goto out;

	if (!string_to_sid(sid, sid_str))
		goto out;

	ret = NT_STATUS_OK;
out:
	destroy_ldap_message(msg);
	destroy_ldap_message(entry);

	return ret;
}

/***********************************************************************
 Get an id from a sid 
***********************************************************************/

static NTSTATUS ldap_get_id_from_sid(unid_t *id, int *id_type,
				     const DOM_SID *sid)
{
	pstring filter;
	const char *type;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	struct ldap_message *msg;
	struct ldap_message *entry = NULL;
	int i;

	DEBUG(8,("ldap_get_id_from_sid: %s (%s)\n", sid_string_static(sid),
		(*id_type & ID_GROUPID ? "group" : "user") ));

	type = ((*id_type) & ID_USERID) ? "uidNumber" : "gidNumber";

	pstr_sprintf(filter, "(&(objectClass=%s)(%s=%s))", 
		     "sambaIdmapEntry", "sambaSID", sid_string_static(sid));

	msg = new_ldap_search_message(lp_ldap_idmap_suffix(),
				      LDAP_SEARCH_SCOPE_SUB,
				      filter, 1, &type);

	if (msg == NULL)
		return NT_STATUS_NO_MEMORY;

	entry = ldap_searchone(ldap_conn, msg, NULL);

	if (entry != NULL) {
		int value;

		if (!ldap_find_single_int(entry, type, &value))
			goto out;

		if ((*id_type) & ID_USERID)
			id->uid = value;
		else
			id->gid = value;

		ret = NT_STATUS_OK;
		goto out;
	}

	if ((*id_type) & ID_QUERY_ONLY)
		goto out;

	/* Allocate a new RID */

	for (i = 0; i < LDAP_MAX_ALLOC_ID; i++) {
		ret = ldap_allocate_id(id, *id_type);
		if ( NT_STATUS_IS_OK(ret) )
			break;
	}
		
	if ( !NT_STATUS_IS_OK(ret) ) {
		DEBUG(0,("Could not allocate id\n"));
		goto out;
	}

	DEBUG(10,("ldap_get_id_from_sid: Allocated new %cid [%ul]\n",
		  (*id_type & ID_GROUPID ? 'g' : 'u'), (uint32)id->uid ));

	ret = ldap_set_mapping(sid, *id, *id_type);

out:
	destroy_ldap_message(msg);
	destroy_ldap_message(entry);

	return ret;
}

/**********************************************************************
 Verify the sambaUnixIdPool entry in the directory.  
**********************************************************************/
static NTSTATUS verify_idpool(void)
{
	const char *attr_list[3] = { "uidnumber", "gidnumber", "objectclass" };
	BOOL result;
	char *mod;
	struct ldap_message *msg, *entry, *res;

	uid_t	luid, huid;
	gid_t	lgid, hgid;

	msg = new_ldap_search_message(lp_ldap_suffix(),
				      LDAP_SEARCH_SCOPE_SUB,
				      "(objectClass=sambaUnixIdPool)",
				      3, attr_list);

	if (msg == NULL)
		return NT_STATUS_NO_MEMORY;

	entry = ldap_searchone(ldap_conn, msg, NULL);

	result = (entry != NULL);

	destroy_ldap_message(msg);
	destroy_ldap_message(entry);

	if (result)
		return NT_STATUS_OK;

	if ( !lp_idmap_uid(&luid, &huid) || !lp_idmap_gid( &lgid, &hgid ) ) {
		DEBUG(3,("ldap_idmap_init: idmap uid/gid parameters not "
			 "specified\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	asprintf(&mod,
		 "dn: %s\n"
		 "changetype: modify\n"
		 "add: objectClass\n"
		 "objectClass: sambaUnixIdPool\n"
		 "-\n"
		 "add: uidNumber\n"
		 "uidNumber: %lu\n"
		 "-\n"
		 "add: gidNumber\n"
		 "gidNumber: %lu\n",
		 lp_ldap_idmap_suffix(),
		 (unsigned long)luid, (unsigned long)lgid);
		 
	msg = ldap_ldif2msg(mod);

	SAFE_FREE(mod);

	if (msg == NULL)
		return NT_STATUS_NO_MEMORY;

	res = ldap_transaction(ldap_conn, msg);

	if ((res == NULL) || (res->r.ModifyResponse.resultcode != 0)) {
		destroy_ldap_message(msg);
		destroy_ldap_message(res);
		DEBUG(5, ("Could not add sambaUnixIdPool\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	destroy_ldap_message(msg);
	destroy_ldap_message(res);
	return NT_STATUS_OK;
}

/*****************************************************************************
 Initialise idmap database. 
*****************************************************************************/

static NTSTATUS ldap_idmap_init( char *params )
{
	NTSTATUS nt_status;
	char *dn, *pw;

	ldap_conn = new_ldap_connection();

	if (!fetch_ldap_pw(&dn, &pw))
		return NT_STATUS_UNSUCCESSFUL;

	ldap_conn->auth_dn = talloc_strdup(ldap_conn->mem_ctx, dn);
	ldap_conn->simple_pw = talloc_strdup(ldap_conn->mem_ctx, pw);

	SAFE_FREE(dn);
	SAFE_FREE(pw);

	if (!ldap_setup_connection(ldap_conn, params, NULL, NULL))
		return NT_STATUS_UNSUCCESSFUL;

	/* see if the idmap suffix and sub entries exists */
	
	nt_status = verify_idpool();	
	if ( !NT_STATUS_IS_OK(nt_status) )
		return nt_status;
		
	return NT_STATUS_OK;
}

/*****************************************************************************
 End the LDAP session
*****************************************************************************/

static NTSTATUS ldap_idmap_close(void)
{

	DEBUG(5,("The connection to the LDAP server was closed\n"));
	/* maybe free the results here --metze */
	
	return NT_STATUS_OK;
}


/* This function doesn't make as much sense in an LDAP world since the calling
   node doesn't really control the ID ranges */
static void ldap_idmap_status(void)
{
	DEBUG(0, ("LDAP IDMAP Status not available\n"));
}

static struct idmap_methods ldap_methods = {
	ldap_idmap_init,
	ldap_allocate_id,
	ldap_get_sid_from_id,
	ldap_get_id_from_sid,
	ldap_set_mapping,
	ldap_idmap_close,
	ldap_idmap_status

};

NTSTATUS idmap_smbldap_init(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION, "smbldap", &ldap_methods);
}
