/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
 *  Copyright (C) Gerald Carter                     2002-2005
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

/* Implementation of registry frontend view functions. */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

extern REGISTRY_OPS printing_ops;
extern REGISTRY_OPS eventlog_ops;
extern REGISTRY_OPS shares_reg_ops;
extern REGISTRY_OPS regdb_ops;		/* these are the default */

/* array of REGISTRY_HOOK's which are read into a tree for easy access */
/* #define REG_TDB_ONLY		1 */

REGISTRY_HOOK reg_hooks[] = {
#ifndef REG_TDB_ONLY 
  { KEY_PRINTING,    		&printing_ops },
  { KEY_PRINTING_2K, 		&printing_ops },
  { KEY_PRINTING_PORTS, 	&printing_ops },
  { KEY_SHARES,      		&shares_reg_ops },
#endif
  { NULL, NULL }
};


static struct generic_mapping reg_generic_map = 
	{ REG_KEY_READ, REG_KEY_WRITE, REG_KEY_EXECUTE, REG_KEY_ALL };

/********************************************************************
********************************************************************/

static NTSTATUS registry_access_check( SEC_DESC *sec_desc, NT_USER_TOKEN *token, 
                                     uint32 access_desired, uint32 *access_granted )
{
	NTSTATUS result;

	if ( geteuid() == sec_initial_uid() ) {
		DEBUG(5,("registry_access_check: using root's token\n"));
		token = get_root_nt_token();
	}

	se_map_generic( &access_desired, &reg_generic_map );
	se_access_check( sec_desc, token, access_desired, access_granted, &result );

	return result;
}

/********************************************************************
********************************************************************/

static SEC_DESC* construct_registry_sd( TALLOC_CTX *ctx )
{
	SEC_ACE ace[2];	
	SEC_ACCESS mask;
	size_t i = 0;
	SEC_DESC *sd;
	SEC_ACL *acl;
	size_t sd_size;

	/* basic access for Everyone */
	
	init_sec_access(&mask, REG_KEY_READ );
	init_sec_ace(&ace[i++], &global_sid_World, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	
	/* Full Access 'BUILTIN\Administrators' */
	
	init_sec_access(&mask, REG_KEY_ALL );
	init_sec_ace(&ace[i++], &global_sid_Builtin_Administrators, SEC_ACE_TYPE_ACCESS_ALLOWED, mask, 0);
	
	
	/* create the security descriptor */
	
	if ( !(acl = make_sec_acl(ctx, NT4_ACL_REVISION, i, ace)) )
		return NULL;

	if ( !(sd = make_sec_desc(ctx, SEC_DESC_REVISION, SEC_DESC_SELF_RELATIVE, NULL, NULL, NULL, acl, &sd_size)) )
		return NULL;

	return sd;
}


/***********************************************************************
 Open the registry database and initialize the REGISTRY_HOOK cache
 ***********************************************************************/
 
BOOL init_registry( void )
{
	int i;
	
	
	if ( !regdb_init() ) {
		DEBUG(0,("init_registry: failed to initialize the registry tdb!\n"));
		return False;
	}

	/* build the cache tree of registry hooks */
	
	reghook_cache_init();
	
	for ( i=0; reg_hooks[i].keyname; i++ ) {
		if ( !reghook_cache_add(&reg_hooks[i]) )
			return False;
	}

	if ( DEBUGLEVEL >= 20 )
		reghook_dump_cache(20);

	/* add any keys for other services */

	svcctl_init_keys();
	eventlog_init_keys();
	perfcount_init_keys();

	/* close and let each smbd open up as necessary */

	regdb_close();

	return True;
}

/***********************************************************************
 High level wrapper function for storing registry subkeys
 ***********************************************************************/
 
BOOL store_reg_keys( REGISTRY_KEY *key, REGSUBKEY_CTR *subkeys )
{
	if ( key->hook && key->hook->ops && key->hook->ops->store_subkeys )
		return key->hook->ops->store_subkeys( key->name, subkeys );
		
	return False;

}

/***********************************************************************
 High level wrapper function for storing registry values
 ***********************************************************************/
 
BOOL store_reg_values( REGISTRY_KEY *key, REGVAL_CTR *val )
{
	if ( check_dynamic_reg_values( key ) )
		return False;

	if ( key->hook && key->hook->ops && key->hook->ops->store_values )
		return key->hook->ops->store_values( key->name, val );

	return False;
}


/***********************************************************************
 High level wrapper function for enumerating registry subkeys
 Initialize the TALLOC_CTX if necessary
 ***********************************************************************/

int fetch_reg_keys( REGISTRY_KEY *key, REGSUBKEY_CTR *subkey_ctr )
{
	int result = -1;
	
	if ( key->hook && key->hook->ops && key->hook->ops->fetch_subkeys )
		result = key->hook->ops->fetch_subkeys( key->name, subkey_ctr );

	return result;
}

/***********************************************************************
 retreive a specific subkey specified by index.  Caller is 
 responsible for freeing memory
 ***********************************************************************/

BOOL fetch_reg_keys_specific( REGISTRY_KEY *key, char** subkey, uint32 key_index )
{
	static REGSUBKEY_CTR *ctr = NULL;
	static pstring save_path;
	char *s;
	
	*subkey = NULL;
	
	/* simple caching for performance; very basic heuristic */

	DEBUG(8,("fetch_reg_keys_specific: Looking for key [%d] of  [%s]\n", key_index, key->name));
	
	if ( !ctr ) {
		DEBUG(8,("fetch_reg_keys_specific: Initializing cache of subkeys for [%s]\n", key->name));

		if ( !(ctr = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR )) ) {
			DEBUG(0,("fetch_reg_keys_specific: talloc() failed!\n"));
			return False;
		}
		
		pstrcpy( save_path, key->name );
		
		if ( fetch_reg_keys( key, ctr) == -1 )
			return False;
			
	}
	/* clear the cache when key_index == 0 or the path has changed */
	else if ( !key_index || StrCaseCmp( save_path, key->name) ) {

		DEBUG(8,("fetch_reg_keys_specific: Updating cache of subkeys for [%s]\n", key->name));
		
		TALLOC_FREE( ctr );

		if ( !(ctr = TALLOC_ZERO_P( NULL, REGSUBKEY_CTR )) ) {
			DEBUG(0,("fetch_reg_keys_specific: talloc() failed!\n"));
			return False;
		}
		
		pstrcpy( save_path, key->name );
		
		if ( fetch_reg_keys( key, ctr) == -1 )
			return False;
	}
	
	if ( !(s = regsubkey_ctr_specific_key( ctr, key_index )) )
		return False;

	*subkey = SMB_STRDUP( s );

	return True;
}

/***********************************************************************
 High level wrapper function for enumerating registry values
 ***********************************************************************/

int fetch_reg_values( REGISTRY_KEY *key, REGVAL_CTR *val )
{
	int result = -1;
	
	if ( key->hook && key->hook->ops && key->hook->ops->fetch_values )
		result = key->hook->ops->fetch_values( key->name, val );
	
	/* if the backend lookup returned no data, try the dynamic overlay */
	
	if ( result == 0 ) {
		result = fetch_dynamic_reg_values( key, val );

		return ( result != -1 ) ? result : 0;
	}
	
	return result;
}


/***********************************************************************
 retreive a specific subkey specified by index.  Caller is 
 responsible for freeing memory
 ***********************************************************************/

BOOL fetch_reg_values_specific( REGISTRY_KEY *key, REGISTRY_VALUE **val, uint32 val_index )
{
	static REGVAL_CTR 	*ctr = NULL;
	static pstring		save_path;
	REGISTRY_VALUE		*v;
	
	*val = NULL;
	
	/* simple caching for performance; very basic heuristic */
	
	if ( !ctr ) {
		DEBUG(8,("fetch_reg_values_specific: Initializing cache of values for [%s]\n", key->name));

		if ( !(ctr = TALLOC_ZERO_P( NULL, REGVAL_CTR )) ) {
			DEBUG(0,("fetch_reg_values_specific: talloc() failed!\n"));
			return False;
		}

		pstrcpy( save_path, key->name );
		
		if ( fetch_reg_values( key, ctr) == -1 )
			return False;
	}
	/* clear the cache when val_index == 0 or the path has changed */
	else if ( !val_index || !strequal(save_path, key->name) ) {

		DEBUG(8,("fetch_reg_values_specific: Updating cache of values for [%s]\n", key->name));		
		
		TALLOC_FREE( ctr );

		if ( !(ctr = TALLOC_ZERO_P( NULL, REGVAL_CTR )) ) {
			DEBUG(0,("fetch_reg_values_specific: talloc() failed!\n"));
			return False;
		}

		pstrcpy( save_path, key->name );
		
		if ( fetch_reg_values( key, ctr) == -1 )
			return False;
	}
	
	if ( !(v = regval_ctr_specific_value( ctr, val_index )) )
		return False;

	*val = dup_registry_value( v );

	return True;
}

/***********************************************************************
 High level access check for passing the required access mask to the 
 underlying registry backend
 ***********************************************************************/

BOOL regkey_access_check( REGISTRY_KEY *key, uint32 requested, uint32 *granted, NT_USER_TOKEN *token )
{
	/* use the default security check if the backend has not defined its own */
	
	if ( !(key->hook && key->hook->ops && key->hook->ops->reg_access_check) ) {
		SEC_DESC *sec_desc;
		NTSTATUS status;
		
		if ( !(sec_desc = construct_registry_sd( get_talloc_ctx() )) )
			return False;
		
		status = registry_access_check( sec_desc, token, requested, granted );		
		
		return NT_STATUS_IS_OK(status);
	}
	
	return key->hook->ops->reg_access_check( key->name, requested, granted, token );
}

/***********************************************************************
***********************************************************************/

WERROR regkey_open_internal( REGISTRY_KEY **regkey, const char *path, 
                             NT_USER_TOKEN *token, uint32 access_desired )
{
	WERROR     	result = WERR_OK;
	REGISTRY_KEY    *keyinfo;
	REGSUBKEY_CTR	*subkeys = NULL;
	uint32 access_granted;
	
	if ( !(W_ERROR_IS_OK(result = regdb_open()) ) )
		return result;

	DEBUG(7,("regkey_open_internal: name = [%s]\n", path));

	if ( !(*regkey = TALLOC_ZERO_P(NULL, REGISTRY_KEY)) ) {
		regdb_close();
		return WERR_NOMEM;
	}
		
	keyinfo = *regkey;
		
	/* initialization */
	
	keyinfo->type = REG_KEY_GENERIC;
	keyinfo->name = talloc_strdup( keyinfo, path );
	
	
	/* Tag this as a Performance Counter Key */

	if( StrnCaseCmp(path, KEY_HKPD, strlen(KEY_HKPD)) == 0 )
		keyinfo->type = REG_KEY_HKPD;
	
	/* Look up the table of registry I/O operations */

	if ( !(keyinfo->hook = reghook_cache_find( keyinfo->name )) ) {
		DEBUG(0,("open_registry_key: Failed to assigned a REGISTRY_HOOK to [%s]\n",
			keyinfo->name ));
		result = WERR_BADFILE;
		goto done;
	}
	
	/* check if the path really exists; failed is indicated by -1 */
	/* if the subkey count failed, bail out */

	if ( !(subkeys = TALLOC_ZERO_P( keyinfo, REGSUBKEY_CTR )) ) {
		result = WERR_NOMEM;
		goto done;
	}

	if ( fetch_reg_keys( keyinfo, subkeys ) == -1 )  {
		result = WERR_BADFILE;
		goto done;
	}
	
	TALLOC_FREE( subkeys );

	if ( !regkey_access_check( keyinfo, access_desired, &access_granted, token ) ) {
		result = WERR_ACCESS_DENIED;
		goto done;
	}
	
	keyinfo->access_granted = access_granted;

done:
	if ( !W_ERROR_IS_OK(result) ) {
		regkey_close_internal( *regkey );
	}

	return result;
}

/*******************************************************************
*******************************************************************/

WERROR regkey_close_internal( REGISTRY_KEY *key )
{
	TALLOC_FREE( key );
	regdb_close();

	return WERR_OK;
}
