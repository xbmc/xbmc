/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
 *  Copyright (C) Gerald Carter                     2002.
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

/* Implementation of registry hook cache tree */

#include "includes.h"
#include "adt_tree.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

static SORTED_TREE *cache_tree;
extern REGISTRY_OPS regdb_ops;		/* these are the default */
static REGISTRY_HOOK default_hook = { KEY_TREE_ROOT, &regdb_ops };

/**********************************************************************
 Initialize the cache tree
 *********************************************************************/

BOOL reghook_cache_init( void )
{
	cache_tree = pathtree_init( &default_hook, NULL );

	return ( cache_tree == NULL );
}

/**********************************************************************
 Add a new REGISTRY_HOOK to the cache.  Note that the keyname
 is not in the exact format that a SORTED_TREE expects.
 *********************************************************************/

BOOL reghook_cache_add( REGISTRY_HOOK *hook )
{
	pstring key;
	
	if ( !hook )
		return False;
		
	pstrcpy( key, "\\");
	pstrcat( key, hook->keyname );	
	
	pstring_sub( key, "\\", "/" );

	DEBUG(10,("reghook_cache_add: Adding key [%s]\n", key));
		
	return pathtree_add( cache_tree, key, hook );
}

/**********************************************************************
 Initialize the cache tree
 *********************************************************************/

REGISTRY_HOOK* reghook_cache_find( const char *keyname )
{
	char *key;
	int len;
	REGISTRY_HOOK *hook;
	
	if ( !keyname )
		return NULL;
	
	/* prepend the string with a '\' character */
	
	len = strlen( keyname );
	if ( !(key = SMB_MALLOC( len + 2 )) ) {
		DEBUG(0,("reghook_cache_find: malloc failed for string [%s] !?!?!\n",
			keyname));
		return NULL;
	}

	*key = '\\';
	strncpy( key+1, keyname, len+1);
	
	/* swap to a form understood by the SORTED_TREE */

	string_sub( key, "\\", "/", 0 );
		
	DEBUG(10,("reghook_cache_find: Searching for keyname [%s]\n", key));
	
	hook = pathtree_find( cache_tree, key ) ;
	
	SAFE_FREE( key );
	
	return hook;
}

/**********************************************************************
 Initialize the cache tree
 *********************************************************************/

void reghook_dump_cache( int debuglevel )
{
	DEBUG(debuglevel,("reghook_dump_cache: Starting cache dump now...\n"));
	
	pathtree_print_keys( cache_tree, debuglevel );
}
