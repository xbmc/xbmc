/* 
   Unix SMB/CIFS implementation.
   ID Mapping
   Copyright (C) Tim Potter 2000
   Copyright (C) Jim McDonough <jmcd@us.ibm.com>	2003
   Copyright (C) Simo Sorce 2003
   Copyright (C) Jeremy Allison 2003.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.*/

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

static_decl_idmap;

struct idmap_function_entry {
	const char *name;
	struct idmap_methods *methods;
	struct idmap_function_entry *prev,*next;
};

static struct idmap_function_entry *backends = NULL;

static struct idmap_methods *cache_map;
static struct idmap_methods *remote_map;

static BOOL proxyonly = False;

/**********************************************************************
 Get idmap methods. Don't allow tdb to be a remote method.
**********************************************************************/

static struct idmap_methods *get_methods(const char *name, BOOL cache_method)
{
	struct idmap_function_entry *entry = backends;

	for(entry = backends; entry; entry = entry->next) {
		if (!cache_method && strequal(entry->name, "tdb"))
			continue; /* tdb is only cache method. */
		if (strequal(entry->name, name))
			return entry->methods;
	}

	return NULL;
}

/**********************************************************************
 Allow a module to register itself as a method.
**********************************************************************/

NTSTATUS smb_register_idmap(int version, const char *name, struct idmap_methods *methods)
{
	struct idmap_function_entry *entry;

 	if ((version != SMB_IDMAP_INTERFACE_VERSION)) {
		DEBUG(0, ("smb_register_idmap: Failed to register idmap module.\n"
		          "The module was compiled against SMB_IDMAP_INTERFACE_VERSION %d,\n"
		          "current SMB_IDMAP_INTERFACE_VERSION is %d.\n"
		          "Please recompile against the current version of samba!\n",  
			  version, SMB_IDMAP_INTERFACE_VERSION));
		return NT_STATUS_OBJECT_TYPE_MISMATCH;
  	}

	if (!name || !name[0] || !methods) {
		DEBUG(0,("smb_register_idmap: called with NULL pointer or empty name!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (get_methods(name, False)) {
		DEBUG(0,("smb_register_idmap: idmap module %s already registered!\n", name));
		return NT_STATUS_OBJECT_NAME_COLLISION;
	}

	entry = SMB_XMALLOC_P(struct idmap_function_entry);
	entry->name = smb_xstrdup(name);
	entry->methods = methods;

	DLIST_ADD(backends, entry);
	DEBUG(5, ("smb_register_idmap: Successfully added idmap backend '%s'\n", name));
	return NT_STATUS_OK;
}

/**********************************************************************
 Initialise idmap cache and a remote backend (if configured).
**********************************************************************/

BOOL idmap_init(const char **remote_backend)
{
	if (!backends)
		static_init_idmap;

	if (!cache_map) {
		cache_map = get_methods("tdb", True);

		if (!cache_map) {
			DEBUG(0, ("idmap_init: could not find tdb cache backend!\n"));
			return False;
		}
		
		if (!NT_STATUS_IS_OK(cache_map->init( NULL ))) {
			DEBUG(0, ("idmap_init: could not initialise tdb cache backend!\n"));
			return False;
		}
	}
	
	if ((remote_map == NULL) && (remote_backend != NULL) &&
	    (*remote_backend != NULL) && (**remote_backend != '\0'))  {
		char *rem_backend = smb_xstrdup(*remote_backend);
		fstring params = "";
		char *pparams;
		BOOL idmap_prefix_workaround = False;
		
		/* get any mode parameters passed in */
		
		if ( (pparams = strchr( rem_backend, ':' )) != NULL ) {
			*pparams = '\0';
			pparams++;
			fstrcpy( params, pparams );
		}

		/* strip any leading idmap_ prefix of */
		if ( strncmp( rem_backend, "idmap_", 6) == 0 ) {
			rem_backend += 6;
			idmap_prefix_workaround = True;
			DEBUG(0, ("idmap_init: idmap backend uses deprecated 'idmap_' prefix.  Please replace 'idmap_%s' by '%s' in %s\n", rem_backend, rem_backend, dyn_CONFIGFILE));
		}
		
		DEBUG(3, ("idmap_init: using '%s' as remote backend\n", rem_backend));
		
		if((remote_map = get_methods(rem_backend, False)) ||
		    (NT_STATUS_IS_OK(smb_probe_module("idmap", rem_backend)) && 
		    (remote_map = get_methods(rem_backend, False)))) {
			if (!NT_STATUS_IS_OK(remote_map->init(params))) {
				DEBUG(0, ("idmap_init: failed to initialize remote backend!\n"));
				return False;
			}
		} else {
			DEBUG(0, ("idmap_init: could not load remote backend '%s'\n", rem_backend));
			if (idmap_prefix_workaround)
				rem_backend -= 6;
			SAFE_FREE(rem_backend);
			return False;
		}
		if (idmap_prefix_workaround)
			rem_backend -= 6;
		SAFE_FREE(rem_backend);
	}

	return True;
}

/**************************************************************************
 Don't do id mapping. This is used to make winbind a netlogon proxy only.
**************************************************************************/

void idmap_set_proxyonly(void)
{
	proxyonly = True;
}

BOOL idmap_proxyonly(void)
{
	return proxyonly;
}

/**************************************************************************
 This is a rare operation, designed to allow an explicit mapping to be
 set up for a sid to a POSIX id.
**************************************************************************/

NTSTATUS idmap_set_mapping(const DOM_SID *sid, unid_t id, int id_type)
{
	struct idmap_methods *map = remote_map;
	DOM_SID tmp_sid;

	if (proxyonly)
		return NT_STATUS_UNSUCCESSFUL;

	if (sid_check_is_in_our_domain(sid)) {
		DEBUG(3, ("Refusing to add SID %s to idmap, it's our own "
			  "domain\n", sid_string_static(sid)));
		return NT_STATUS_ACCESS_DENIED;
	}
		
	if (sid_check_is_in_builtin(sid)) {
		DEBUG(3, ("Refusing to add SID %s to idmap, it's our builtin "
			  "domain\n", sid_string_static(sid)));
		return NT_STATUS_ACCESS_DENIED;
	}

	DEBUG(10, ("idmap_set_mapping: Set %s to %s %lu\n",
		   sid_string_static(sid),
		   ((id_type & ID_TYPEMASK) == ID_USERID) ? "UID" : "GID",
		   ((id_type & ID_TYPEMASK) == ID_USERID) ? (unsigned long)id.uid : 
		   (unsigned long)id.gid));

	if ( (NT_STATUS_IS_OK(cache_map->
			      get_sid_from_id(&tmp_sid, id,
					      id_type | ID_QUERY_ONLY))) &&
	     sid_equal(sid, &tmp_sid) ) {
		/* Nothing to do, we already have that mapping */
		DEBUG(10, ("idmap_set_mapping: Mapping already there\n"));
		return NT_STATUS_OK;
	}

	if (map == NULL) {
		/* Ok, we don't have a authoritative remote
			mapping. So update our local cache only. */
		map = cache_map;
	}

	return map->set_mapping(sid, id, id_type);
}

/**************************************************************************
 Get ID from SID. This can create a mapping for a SID to a POSIX id.
**************************************************************************/

NTSTATUS idmap_get_id_from_sid(unid_t *id, int *id_type, const DOM_SID *sid)
{
	NTSTATUS ret;
	int loc_type;
	unid_t loc_id;

	if (proxyonly)
		return NT_STATUS_UNSUCCESSFUL;

	if (sid_check_is_in_our_domain(sid)) {
		DEBUG(9, ("sid %s is in our domain -- go look in passdb\n",
			  sid_string_static(sid)));
		return NT_STATUS_NONE_MAPPED;
	}

	if (sid_check_is_in_builtin(sid)) {
		DEBUG(9, ("sid %s is in builtin domain -- go look in passdb\n",
			  sid_string_static(sid)));
		return NT_STATUS_NONE_MAPPED;
	}

	loc_type = *id_type;

	if (remote_map) {
		/* We have a central remote idmap so only look in
                   cache, don't allocate */
		loc_type |= ID_QUERY_ONLY;
	}

	ret = cache_map->get_id_from_sid(id, &loc_type, sid);

	if (NT_STATUS_IS_OK(ret)) {
		*id_type = loc_type & ID_TYPEMASK;
		return NT_STATUS_OK;
	}

	if ((remote_map == NULL) || (loc_type & ID_CACHE_ONLY)) {
		return ret;
	}

	/* Before forking out to the possibly slow remote map, lets see if we
	 * already have the sid as uid when asking for a gid or vice versa. */

	loc_type = *id_type & ID_TYPEMASK;

	switch (loc_type) {
	case ID_USERID:
		loc_type = ID_GROUPID;
		break;
	case ID_GROUPID:
		loc_type = ID_USERID;
		break;
	default:
		loc_type = ID_EMPTY;
	}

	loc_type |= ID_QUERY_ONLY;

	ret = cache_map->get_id_from_sid(&loc_id, &loc_type, sid);

	if (NT_STATUS_IS_OK(ret)) {
		/* Ok, we have the uid as gid or vice versa. The remote map
		 * would not know anything different, so return here. */
		return NT_STATUS_UNSUCCESSFUL;
	}

	/* Ok, the mapping was not in the cache, give the remote map a
           second try. */

	ret = remote_map->get_id_from_sid(id, id_type, sid);
	
	if (NT_STATUS_IS_OK(ret)) {
		/* The remote backend gave us a valid mapping, cache it. */
		ret = cache_map->set_mapping(sid, *id, *id_type);
	}

	return ret;
}

/**************************************************************************
 Get SID from ID. This must have been created before.
**************************************************************************/

NTSTATUS idmap_get_sid_from_id(DOM_SID *sid, unid_t id, int id_type)
{
	NTSTATUS ret;
	int loc_type;

	if (proxyonly)
		return NT_STATUS_UNSUCCESSFUL;

	loc_type = id_type;
	if (remote_map) {
		loc_type = id_type | ID_QUERY_ONLY;
	}

	ret = cache_map->get_sid_from_id(sid, id, loc_type);

	if (NT_STATUS_IS_OK(ret))
		return ret;

	if ((remote_map == NULL) || (loc_type & ID_CACHE_ONLY))
		return ret;

	/* We have a second chance, ask our authoritative backend */

	ret = remote_map->get_sid_from_id(sid, id, id_type);

	if (NT_STATUS_IS_OK(ret)) {
		/* The remote backend gave us a valid mapping, cache it. */
		ret = cache_map->set_mapping(sid, id, id_type);
	}

	return ret;
}

/**************************************************************************
 Alloocate a new UNIX uid/gid
**************************************************************************/

NTSTATUS idmap_allocate_id(unid_t *id, int id_type)
{
	/* we have to allocate from the authoritative backend */
	
	if (proxyonly)
		return NT_STATUS_UNSUCCESSFUL;

	if ( remote_map )
		return remote_map->allocate_id( id, id_type );

	return cache_map->allocate_id( id, id_type );
}

/**************************************************************************
 Shutdown maps.
**************************************************************************/

NTSTATUS idmap_close(void)
{
	NTSTATUS ret;

	if (proxyonly)
		return NT_STATUS_OK;

	ret = cache_map->close_fn();
	if (!NT_STATUS_IS_OK(ret)) {
		DEBUG(3, ("idmap_close: failed to close local tdb cache!\n"));
	}
	cache_map = NULL;

	if (remote_map) {
		ret = remote_map->close_fn();
		if (!NT_STATUS_IS_OK(ret)) {
			DEBUG(3, ("idmap_close: failed to close remote idmap repository!\n"));
		}
		remote_map = NULL;
	}

	return ret;
}

/**************************************************************************
 Dump backend status.
**************************************************************************/

void idmap_status(void)
{
	cache_map->status();
	if (remote_map)
		remote_map->status();
}
