/* 
 *  Unix SMB/CIFS implementation.
 *  Generic Abstract Data Types
 *  Copyright (C) Gerald Carter                     2002-2005.
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

#ifndef ADT_TREE_H
#define ADT_TREE_H

#ifndef _BOOL
typedef int BOOL;
#define _BOOL  
#endif


/* data structure used to build the tree */

typedef struct _tree_node {
	struct _tree_node	*parent;
	struct _tree_node	**children;
	int 			num_children;
	char			*key;
	void			*data_p;
} TREE_NODE;

typedef struct _tree_root {
	TREE_NODE	*root;

	/* not used currently (is it needed?) */
	int 		(*compare)(void* x, void *y);
} SORTED_TREE;

/* 
 * API
 */

/* create a new tree, talloc_free() to throw it away */

SORTED_TREE*  pathtree_init( void *data_p, int (cmp_fn)(void*, void*) );

/* add a new path component */

BOOL          pathtree_add( SORTED_TREE *tree, const char *path, void *data_p );

/* search path */

void*         pathtree_find( SORTED_TREE *tree, char *key );

/* debug (print) functions */

void          pathtree_print_keys( SORTED_TREE *tree, int debug );


#endif
