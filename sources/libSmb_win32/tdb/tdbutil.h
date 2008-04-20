/* 
   Unix SMB/CIFS implementation.
   tdb utility functions
   Copyright (C) Andrew Tridgell   1999
   Copyright (C) Rafal Szczesniak  2002
   
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

#ifndef __TDBUTIL_H__
#define __TDBUTIL_H__


/* single node of a list returned by tdb_search_keys */
typedef struct keys_node 
{
	struct keys_node *prev, *next;
	TDB_DATA node_key;
} TDB_LIST_NODE;


TDB_LIST_NODE *tdb_search_keys(TDB_CONTEXT*, const char*);
void tdb_search_list_free(TDB_LIST_NODE*);


#endif /* __TDBUTIL_H__ */
