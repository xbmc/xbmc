/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
 *  Copyright (C) Gerald Carter                     2005
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

/* Implementation of registry virtual views for printing information */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/**********************************************************************
 It is safe to assume that every registry path passed into on of 
 the exported functions here begins with KEY_PRINTING else
 these functions would have never been called.  This is a small utility
 function to strip the beginning of the path and make a copy that the 
 caller can modify.  Note that the caller is responsible for releasing
 the memory allocated here.
 **********************************************************************/

static char* trim_reg_path( const char *path )
{
	const char *p;
	uint16 key_len = strlen(KEY_SHARES);
	
	/* 
	 * sanity check...this really should never be True.
	 * It is only here to prevent us from accessing outside
	 * the path buffer in the extreme case.
	 */
	
	if ( strlen(path) < key_len ) {
		DEBUG(0,("trim_reg_path: Registry path too short! [%s]\n", path));
		return NULL;
	}
	
	
	p = path + strlen( KEY_SHARES );
	
	if ( *p == '\\' )
		p++;
	
	if ( *p )
		return SMB_STRDUP(p);
	else
		return NULL;
}

/**********************************************************************
 Enumerate registry subkey names given a registry path.  
 Caller is responsible for freeing memory to **subkeys
 *********************************************************************/
 
static int shares_subkey_info( const char *key, REGSUBKEY_CTR *subkey_ctr )
{
	char 		*path;
	BOOL		top_level = False;
	int		num_subkeys = 0;
	
	DEBUG(10,("printing_subkey_info: key=>[%s]\n", key));
	
	path = trim_reg_path( key );
	
	/* check to see if we are dealing with the top level key */
	
	if ( !path )
		top_level = True;
		
	if ( top_level ) {
		num_subkeys = 1;
		regsubkey_ctr_addkey( subkey_ctr, "Security" );
	}
#if 0
	else
		num_subkeys = handle_share_subpath( path, subkey_ctr, NULL );
#endif
	
	SAFE_FREE( path );
	
	return num_subkeys;
}

/**********************************************************************
 Enumerate registry values given a registry path.  
 Caller is responsible for freeing memory 
 *********************************************************************/

static int shares_value_info( const char *key, REGVAL_CTR *val )
{
	char 		*path;
	BOOL		top_level = False;
	int		num_values = 0;
	
	DEBUG(10,("printing_value_info: key=>[%s]\n", key));
	
	path = trim_reg_path( key );
	
	/* check to see if we are dealing with the top level key */
	
	if ( !path )
		top_level = True;
	
	/* fill in values from the getprinterdata_printer_server() */
	if ( top_level )
		num_values = 0;
#if 0
	else
		num_values = handle_printing_subpath( path, NULL, val );
#endif
		
	SAFE_FREE(path);
	
	return num_values;
}

/**********************************************************************
 Stub function which always returns failure since we don't want
 people storing printing information directly via regostry calls
 (for now at least)
 *********************************************************************/

static BOOL shares_store_subkey( const char *key, REGSUBKEY_CTR *subkeys )
{
	return False;
}

/**********************************************************************
 Stub function which always returns failure since we don't want
 people storing printing information directly via regostry calls
 (for now at least)
 *********************************************************************/

static BOOL shares_store_value( const char *key, REGVAL_CTR *val )
{
	return False;
}

/* 
 * Table of function pointers for accessing printing data
 */
 
REGISTRY_OPS shares_reg_ops = {
	shares_subkey_info,
	shares_value_info,
	shares_store_subkey,
	shares_store_value,
	NULL
};


