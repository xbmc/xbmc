/* 
 *  Unix SMB/CIFS implementation.
 *  Generic Abstract Data Types
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

#include "includes.h"
#include "adt_tree.h"


/**************************************************************************
 *************************************************************************/

static BOOL trim_tree_keypath( char *path, char **base, char **new_path )
{
	char *p;
	
	*new_path = *base = NULL;
	
	if ( !path )
		return False;
	
	*base = path;
	
	p = strchr( path, '/' );
	
	if ( p ) {
		*p = '\0';
		*new_path = p+1;
	}
	
	return True;
}

 
/**************************************************************************
 Initialize the tree's root.  The cmp_fn is a callback function used
 for comparision of two children
 *************************************************************************/

 SORTED_TREE* pathtree_init( void *data_p, int (cmp_fn)(void*, void*) )
{
	SORTED_TREE *tree = NULL;
	
	if ( !(tree = TALLOC_ZERO_P(NULL, SORTED_TREE)) )
		return NULL;
		
	tree->compare = cmp_fn;
	
	if ( !(tree->root = TALLOC_ZERO_P(tree, TREE_NODE)) ) {
		TALLOC_FREE( tree );
		return NULL;
	}
	
	tree->root->data_p = data_p;
	
	return tree;
}


/**************************************************************************
 Find the next child given a key string
 *************************************************************************/

static TREE_NODE* pathtree_birth_child( TREE_NODE *node, char* key )
{
	TREE_NODE *infant = NULL;
	TREE_NODE **siblings;
	int i;
	
	if ( !(infant = TALLOC_ZERO_P( node, TREE_NODE)) )
		return NULL;
	
	infant->key = talloc_strdup( infant, key );
	infant->parent = node;
	
	siblings = TALLOC_REALLOC_ARRAY( node, node->children, TREE_NODE *, node->num_children+1 );
	
	if ( siblings )
		node->children = siblings;
	
	node->num_children++;
	
	/* first child */
	
	if ( node->num_children == 1 ) {
		DEBUG(11,("pathtree_birth_child: First child of node [%s]! [%s]\n", 
			node->key ? node->key : "NULL", infant->key ));
		node->children[0] = infant;
	}
	else 
	{
		/* 
		 * multiple siblings .... (at least 2 children)
		 * 
		 * work from the end of the list forward 
		 * The last child is not set at this point 
		 * Insert the new infanct in ascending order 
		 * from left to right
		 */
	
		for ( i = node->num_children-1; i>=1; i-- )
		{
			DEBUG(11,("pathtree_birth_child: Looking for crib; infant -> [%s], child -> [%s]\n",
				infant->key, node->children[i-1]->key));
			
			/* the strings should never match assuming that we 
			   have called pathtree_find_child() first */
		
			if ( StrCaseCmp( infant->key, node->children[i-1]->key ) > 0 ) {
				DEBUG(11,("pathtree_birth_child: storing infant in i == [%d]\n", 
					i));
				node->children[i] = infant;
				break;
			}
			
			/* bump everything towards the end on slot */
			
			node->children[i] = node->children[i-1];
		}

		DEBUG(11,("pathtree_birth_child: Exiting loop (i == [%d])\n", i ));
		
		/* if we haven't found the correct slot yet, the child 
		   will be first in the list */
		   
		if ( i == 0 )
			node->children[0] = infant;
	}

	return infant;
}

/**************************************************************************
 Find the next child given a key string
 *************************************************************************/

static TREE_NODE* pathtree_find_child( TREE_NODE *node, char* key )
{
	TREE_NODE *next = NULL;
	int i, result;
	
	if ( !node ) {
		DEBUG(0,("pathtree_find_child: NULL node passed into function!\n"));
		return NULL;
	}
	
	if ( !key ) {
		DEBUG(0,("pathtree_find_child: NULL key string passed into function!\n"));
		return NULL;
	}
	
	for ( i=0; i<node->num_children; i++ )
	{	
		DEBUG(11,("pathtree_find_child: child key => [%s]\n",
			node->children[i]->key));
			
		result = StrCaseCmp( node->children[i]->key, key );
		
		if ( result == 0 )
			next = node->children[i];
		
		/* if result > 0 then we've gone to far because
		   the list of children is sorted by key name 
		   If result == 0, then we have a match         */
		   
		if ( result > 0 )
			break;
	}

	DEBUG(11,("pathtree_find_child: %s [%s]\n",
		next ? "Found" : "Did not find", key ));	
	
	return next;
}

/**************************************************************************
 Add a new node into the tree given a key path and a blob of data
 *************************************************************************/

 BOOL pathtree_add( SORTED_TREE *tree, const char *path, void *data_p )
{
	char *str, *base, *path2;
	TREE_NODE *current, *next;
	BOOL ret = True;
	
	DEBUG(8,("pathtree_add: Enter\n"));
		
	if ( !path || *path != '/' ) {
		DEBUG(0,("pathtree_add: Attempt to add a node with a bad path [%s]\n",
			path ? path : "NULL" ));
		return False;
	}
	
	if ( !tree ) {
		DEBUG(0,("pathtree_add: Attempt to add a node to an uninitialized tree!\n"));
		return False;
	}
	
	/* move past the first '/' */
	
	path++;	
	path2 = SMB_STRDUP( path );
	if ( !path2 ) {
		DEBUG(0,("pathtree_add: strdup() failed on string [%s]!?!?!\n", path));
		return False;
	}
	

	/* 
	 * this works sort of like a 'mkdir -p'	call, possibly 
	 * creating an entire path to the new node at once
	 * The path should be of the form /<key1>/<key2>/...
	 */
	
	base = path2;
	str  = path2;
	current = tree->root;
	
	do {
		/* break off the remaining part of the path */
		
		str = strchr( str, '/' );
		if ( str )
			*str = '\0';
			
		/* iterate to the next child--birth it if necessary */
		
		next = pathtree_find_child( current, base );
		if ( !next ) {
			next = pathtree_birth_child( current, base );
			if ( !next ) {
				DEBUG(0,("pathtree_add: Failed to create new child!\n"));
				ret =  False;
				goto done;
			}
		}
		current = next;
		
		/* setup the next part of the path */
		
		base = str;
		if ( base ) {
			*base = '/';
			base++;
			str = base;
		}
	
	} while ( base != NULL );
	
	current->data_p = data_p;
	
	DEBUG(10,("pathtree_add: Successfully added node [%s] to tree\n",
		path ));

	DEBUG(8,("pathtree_add: Exit\n"));

done:
	SAFE_FREE( path2 );
	return ret;
}


/**************************************************************************
 Recursive routine to print out all children of a TREE_NODE
 *************************************************************************/

static void pathtree_print_children( TREE_NODE *node, int debug, const char *path )
{
	int i;
	int num_children;
	pstring path2;
	
	if ( !node )
		return;
	
	
	if ( node->key )
		DEBUG(debug,("%s: [%s] (%s)\n", path ? path : "NULL", node->key,
			node->data_p ? "data" : "NULL" ));

	*path2 = '\0';
	if ( path )
		pstrcpy( path2, path );
	pstrcat( path2, node->key ? node->key : "NULL" );
	pstrcat( path2, "/" );
		
	num_children = node->num_children;
	for ( i=0; i<num_children; i++ )
		pathtree_print_children( node->children[i], debug, path2 );
	

}

/**************************************************************************
 Dump the kys for a tree to the log file
 *************************************************************************/

 void pathtree_print_keys( SORTED_TREE *tree, int debug )
{
	int i;
	int num_children = tree->root->num_children;
	
	if ( tree->root->key )
		DEBUG(debug,("ROOT/: [%s] (%s)\n", tree->root->key,
			tree->root->data_p ? "data" : "NULL" ));
	
	for ( i=0; i<num_children; i++ ) {
		pathtree_print_children( tree->root->children[i], debug, 
			tree->root->key ? tree->root->key : "ROOT/" );
	}
	
}

/**************************************************************************
 return the data_p for for the node in tree matching the key string
 The key string is the full path.  We must break it apart and walk 
 the tree
 *************************************************************************/

 void* pathtree_find( SORTED_TREE *tree, char *key )
{
	char *keystr, *base, *str, *p;
	TREE_NODE *current;
	void *result = NULL;
	
	DEBUG(10,("pathtree_find: Enter [%s]\n", key ? key : "NULL" ));

	/* sanity checks first */
	
	if ( !key ) {
		DEBUG(0,("pathtree_find: Attempt to search tree using NULL search string!\n"));
		return NULL;
	}
	
	if ( !tree ) {
		DEBUG(0,("pathtree_find: Attempt to search an uninitialized tree using string [%s]!\n",
			key ? key : "NULL" ));
		return NULL;
	}
	
	if ( !tree->root )
		return NULL;
	
	/* make a copy to play with */
	
	if ( *key == '/' )
		keystr = SMB_STRDUP( key+1 );
	else
		keystr = SMB_STRDUP( key );
	
	if ( !keystr ) {
		DEBUG(0,("pathtree_find: strdup() failed on string [%s]!?!?!\n", key));
		return NULL;
	}

	/* start breaking the path apart */
	
	p = keystr;
	current = tree->root;
	
	if ( tree->root->data_p )
		result = tree->root->data_p;
		
	do
	{
		/* break off the remaining part of the path */

		trim_tree_keypath( p, &base, &str );
			
		DEBUG(11,("pathtree_find: [loop] base => [%s], new_path => [%s]\n", 
			base, str));

		/* iterate to the next child */
		
		current = pathtree_find_child( current, base );
	
		/* 
		 * the idea is that the data_p for a parent should 
		 * be inherited by all children, but allow it to be 
		 * overridden farther down
		 */
		
		if ( current && current->data_p )
			result = current->data_p;

		/* reset the path pointer 'p' to the remaining part of the key string */

		p = str;
	   
	} while ( str && current );
	
	/* result should be the data_p from the lowest match node in the tree */
	if ( result )
		DEBUG(11,("pathtree_find: Found data_p!\n"));
	
	SAFE_FREE( keystr );
	
	DEBUG(10,("pathtree_find: Exit\n"));
	
	return result;
}


