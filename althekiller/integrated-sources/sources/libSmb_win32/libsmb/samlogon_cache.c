/* 
   Unix SMB/CIFS implementation.
   Net_sam_logon info3 helpers
   Copyright (C) Alexander Bokovoy              2002.
   Copyright (C) Andrew Bartlett                2002.
   Copyright (C) Gerald Carter			2003.
   Copyright (C) Tim Potter			2003.
   
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

#define NETSAMLOGON_TDB	"netsamlogon_cache.tdb"

static TDB_CONTEXT *netsamlogon_tdb = NULL;

/***********************************************************************
 open the tdb
 ***********************************************************************/
 
BOOL netsamlogon_cache_init(void)
{
	if (!netsamlogon_tdb) {
		netsamlogon_tdb = tdb_open_log(lock_path(NETSAMLOGON_TDB), 0,
						   TDB_DEFAULT, O_RDWR | O_CREAT, 0600);
	}

	return (netsamlogon_tdb != NULL);
}


/***********************************************************************
 Shutdown samlogon_cache database
***********************************************************************/

BOOL netsamlogon_cache_shutdown(void)
{
	if(netsamlogon_tdb)
		return (tdb_close(netsamlogon_tdb) == 0);
		
	return True;
}

/***********************************************************************
 Clear cache getpwnam and getgroups entries from the winbindd cache
***********************************************************************/
void netsamlogon_clear_cached_user(TDB_CONTEXT *tdb, NET_USER_INFO_3 *user)
{
	fstring domain;
	TDB_DATA key;
	BOOL got_tdb = False;

	/* We may need to call this function from smbd which will not have
           winbindd_cache.tdb open.  Open the tdb if a NULL is passed. */

	if (!tdb) {
		tdb = tdb_open_log(lock_path("winbindd_cache.tdb"), 5000,
				   TDB_DEFAULT, O_RDWR, 0600);
		if (!tdb) {
			DEBUG(5, ("netsamlogon_clear_cached_user: failed to open cache\n"));
			return;
		}
		got_tdb = True;
	}

	unistr2_to_ascii(domain, &user->uni_logon_dom, sizeof(domain) - 1);

	/* Clear U/DOMAIN/RID cache entry */

	asprintf(&key.dptr, "U/%s/%d", domain, user->user_rid);
	key.dsize = strlen(key.dptr) - 1; /* keys are not NULL terminated */

	DEBUG(10, ("netsamlogon_clear_cached_user: clearing %s\n", key.dptr));

	tdb_delete(tdb, key);

	SAFE_FREE(key.dptr);

	/* Clear UG/DOMAIN/RID cache entry */

	asprintf(&key.dptr, "UG/%s/%d", domain, user->user_rid);
	key.dsize = strlen(key.dptr) - 1; /* keys are not NULL terminated */

	DEBUG(10, ("netsamlogon_clear_cached_user: clearing %s\n", key.dptr));

	tdb_delete(tdb, key);

	SAFE_FREE(key.dptr);

	if (got_tdb)
		tdb_close(tdb);
}

/***********************************************************************
 Store a NET_USER_INFO_3 structure in a tdb for later user 
 username should be in UTF-8 format
***********************************************************************/

BOOL netsamlogon_cache_store( const char *username, NET_USER_INFO_3 *user )
{
	TDB_DATA 	data;
        fstring 	keystr;
	prs_struct 	ps;
	BOOL 		result = False;
	DOM_SID		user_sid;
	time_t		t = time(NULL);
	TALLOC_CTX 	*mem_ctx;
	

	if (!netsamlogon_cache_init()) {
		DEBUG(0,("netsamlogon_cache_store: cannot open %s for write!\n", NETSAMLOGON_TDB));
		return False;
	}

	sid_copy( &user_sid, &user->dom_sid.sid );
	sid_append_rid( &user_sid, user->user_rid );

	/* Prepare key as DOMAIN-SID/USER-RID string */
	slprintf(keystr, sizeof(keystr), "%s", sid_string_static(&user_sid));

	DEBUG(10,("netsamlogon_cache_store: SID [%s]\n", keystr));
	
	/* only Samba fills in the username, not sure why NT doesn't */
	/* so we fill it in since winbindd_getpwnam() makes use of it */
	
	if ( !user->uni_user_name.buffer ) {
		init_unistr2( &user->uni_user_name, username, UNI_STR_TERMINATE );
		init_uni_hdr( &user->hdr_user_name, &user->uni_user_name );
	}
		
	/* Prepare data */
	
	if ( !(mem_ctx = TALLOC_P( NULL, int )) ) {
		DEBUG(0,("netsamlogon_cache_store: talloc() failed!\n"));
		return False;
	}

	prs_init( &ps, RPC_MAX_PDU_FRAG_LEN, mem_ctx, MARSHALL);
	
	{
		uint32 ts = (uint32)t;
		if ( !prs_uint32( "timestamp", &ps, 0, &ts ) )
			return False;
	}
	
	if ( net_io_user_info3("", user, &ps, 0, 3, 0) ) 
	{
		data.dsize = prs_offset( &ps );
		data.dptr = prs_data_p( &ps );

		if (tdb_store_bystring(netsamlogon_tdb, keystr, data, TDB_REPLACE) != -1)
			result = True;
		
		prs_mem_free( &ps );
	}

	TALLOC_FREE( mem_ctx );
		
	return result;
}

/***********************************************************************
 Retrieves a NET_USER_INFO_3 structure from a tdb.  Caller must 
 free the user_info struct (malloc()'d memory)
***********************************************************************/

NET_USER_INFO_3* netsamlogon_cache_get( TALLOC_CTX *mem_ctx, const DOM_SID *user_sid)
{
	NET_USER_INFO_3	*user = NULL;
	TDB_DATA 	data, key;
	prs_struct	ps;
        fstring 	keystr;
	uint32		t;
	
	if (!netsamlogon_cache_init()) {
		DEBUG(0,("netsamlogon_cache_get: cannot open %s for write!\n", NETSAMLOGON_TDB));
		return False;
	}

	/* Prepare key as DOMAIN-SID/USER-RID string */
	slprintf(keystr, sizeof(keystr), "%s", sid_string_static(user_sid));
	DEBUG(10,("netsamlogon_cache_get: SID [%s]\n", keystr));
	key.dptr = keystr;
	key.dsize = strlen(keystr)+1;
	data = tdb_fetch( netsamlogon_tdb, key );
	
	if ( data.dptr ) {
		
		if ( (user = SMB_MALLOC_P(NET_USER_INFO_3)) == NULL )
			return NULL;
			
		prs_init( &ps, 0, mem_ctx, UNMARSHALL );
		prs_give_memory( &ps, data.dptr, data.dsize, True );
		
		if ( !prs_uint32( "timestamp", &ps, 0, &t ) ) {
			prs_mem_free( &ps );
			SAFE_FREE(user);
			return False;
		}
		
		if ( !net_io_user_info3("", user, &ps, 0, 3, 0) ) {
			SAFE_FREE( user );
		}
			
		prs_mem_free( &ps );

#if 0	/* The netsamlogon cache needs to hang around.  Something about 
	   this feels wrong, but it is the only way we can get all of the
	   groups.  The old universal groups cache didn't expire either.
	   --jerry */
	{
		time_t		now = time(NULL);
		uint32		time_diff;
	   
		/* is the entry expired? */
		time_diff = now - t;
		
		if ( (time_diff < 0 ) || (time_diff > lp_winbind_cache_time()) ) {
			DEBUG(10,("netsamlogon_cache_get: cache entry expired \n"));
			tdb_delete( netsamlogon_tdb, key );
			SAFE_FREE( user );
		}
#endif
	}
	
	return user;
}

BOOL netsamlogon_cache_have(const DOM_SID *user_sid)
{
	TALLOC_CTX *mem_ctx = talloc_init("netsamlogon_cache_have");
	NET_USER_INFO_3 *user = NULL;
	BOOL result;

	if (!mem_ctx)
		return False;

	user = netsamlogon_cache_get(mem_ctx, user_sid);

	result = (user != NULL);

	talloc_destroy(mem_ctx);
	SAFE_FREE(user);

	return result;
}
