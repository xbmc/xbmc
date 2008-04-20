/* 
   Unix SMB/CIFS implementation.
   Manage connections_struct structures
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Alexander Bokovoy 2002
   
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

/* The connections bitmap is expanded in increments of BITMAP_BLOCK_SZ. The
 * maximum size of the bitmap is the largest positive integer, but you will hit
 * the "max connections" limit, looong before that.
 */
#define BITMAP_BLOCK_SZ 128

static connection_struct *Connections;

/* number of open connections */
static struct bitmap *bmap;
static int num_open;

/****************************************************************************
init the conn structures
****************************************************************************/
void conn_init(void)
{
	bmap = bitmap_allocate(BITMAP_BLOCK_SZ);
}

/****************************************************************************
return the number of open connections
****************************************************************************/
int conn_num_open(void)
{
	return num_open;
}


/****************************************************************************
check if a snum is in use
****************************************************************************/
BOOL conn_snum_used(int snum)
{
	connection_struct *conn;
	for (conn=Connections;conn;conn=conn->next) {
		if (conn->service == snum) {
			return(True);
		}
	}
	return(False);
}


/****************************************************************************
find a conn given a cnum
****************************************************************************/
connection_struct *conn_find(unsigned cnum)
{
	int count=0;
	connection_struct *conn;

	for (conn=Connections;conn;conn=conn->next,count++) {
		if (conn->cnum == cnum) {
			if (count > 10) {
				DLIST_PROMOTE(Connections, conn);
			}
			return conn;
		}
	}

	return NULL;
}


/****************************************************************************
  find first available connection slot, starting from a random position.
The randomisation stops problems with the server dieing and clients
thinking the server is still available.
****************************************************************************/
connection_struct *conn_new(void)
{
	TALLOC_CTX *mem_ctx;
	connection_struct *conn;
	int i;
        int find_offset = 1;

find_again:
	i = bitmap_find(bmap, find_offset);
	
	if (i == -1) {
                /* Expand the connections bitmap. */
                int             oldsz = bmap->n;
                int             newsz = bmap->n + BITMAP_BLOCK_SZ;
                struct bitmap * nbmap;

                if (newsz <= 0) {
                        /* Integer wrap. */
		        DEBUG(0,("ERROR! Out of connection structures\n"));
                        return NULL;
                }

		DEBUG(4,("resizing connections bitmap from %d to %d\n",
                        oldsz, newsz));

                nbmap = bitmap_allocate(newsz);
		if (!nbmap) {
			DEBUG(0,("ERROR! malloc fail.\n"));
			return NULL;
		}

                bitmap_copy(nbmap, bmap);
                bitmap_free(bmap);

                bmap = nbmap;
                find_offset = oldsz; /* Start next search in the new portion. */

                goto find_again;
	}

	if ((mem_ctx=talloc_init("connection_struct"))==NULL) {
		DEBUG(0,("talloc_init(connection_struct) failed!\n"));
		return NULL;
	}

	if ((conn=TALLOC_ZERO_P(mem_ctx, connection_struct))==NULL) {
		DEBUG(0,("talloc_zero() failed!\n"));
		return NULL;
	}
	conn->mem_ctx = mem_ctx;
	conn->cnum = i;

	bitmap_set(bmap, i);

	num_open++;

	string_set(&conn->user,"");
	string_set(&conn->dirpath,"");
	string_set(&conn->connectpath,"");
	string_set(&conn->origpath,"");
	
	DLIST_ADD(Connections, conn);

	return conn;
}

/****************************************************************************
 Close all conn structures.
****************************************************************************/

void conn_close_all(void)
{
	connection_struct *conn, *next;
	for (conn=Connections;conn;conn=next) {
		next=conn->next;
		set_current_service(conn, 0, True);
		close_cnum(conn, conn->vuid);
	}
}

/****************************************************************************
 Idle inactive connections.
****************************************************************************/

BOOL conn_idle_all(time_t t, int deadtime)
{
	pipes_struct *plist = NULL;
	BOOL allidle = True;
	connection_struct *conn, *next;

	for (conn=Connections;conn;conn=next) {
		next=conn->next;

		/* Update if connection wasn't idle. */
		if (conn->lastused != conn->lastused_count) {
			conn->lastused = t;
			conn->lastused_count = t;
		}

		/* close dirptrs on connections that are idle */
		if ((t-conn->lastused) > DPTR_IDLE_TIMEOUT) {
			dptr_idlecnum(conn);
		}

		if (conn->num_files_open > 0 || (t-conn->lastused)<deadtime) {
			allidle = False;
		}
	}

	/*
	 * Check all pipes for any open handles. We cannot
	 * idle with a handle open.
	 */

	for (plist = get_first_internal_pipe(); plist; plist = get_next_internal_pipe(plist))
		if (plist->pipe_handles && plist->pipe_handles->count)
			allidle = False;
	
	return allidle;
}

/****************************************************************************
 Clear a vuid out of the validity cache, and as the 'owner' of a connection.
****************************************************************************/

void conn_clear_vuid_cache(uint16 vuid)
{
	connection_struct *conn;
	unsigned int i;

	for (conn=Connections;conn;conn=conn->next) {
		if (conn->vuid == vuid) {
			conn->vuid = UID_FIELD_INVALID;
		}

		for (i=0;i<conn->vuid_cache.entries && i< VUID_CACHE_SIZE;i++) {
			if (conn->vuid_cache.array[i].vuid == vuid) {
				struct vuid_cache_entry *ent = &conn->vuid_cache.array[i];
				ent->vuid = UID_FIELD_INVALID;
				ent->read_only = False;
				ent->admin_user = False;
			}
		}
	}
}

/****************************************************************************
 Free a conn structure - internal part.
****************************************************************************/

void conn_free_internal(connection_struct *conn)
{
 	vfs_handle_struct *handle = NULL, *thandle = NULL;
 	TALLOC_CTX *mem_ctx = NULL;

	/* Free vfs_connection_struct */
	handle = conn->vfs_handles;
	while(handle) {
		DLIST_REMOVE(conn->vfs_handles, handle);
		thandle = handle->next;
		if (handle->free_data)
			handle->free_data(&handle->data);
		handle = thandle;
	}

	if (conn->ngroups && conn->groups) {
		SAFE_FREE(conn->groups);
		conn->ngroups = 0;
	}

	if (conn->nt_user_token) {
		TALLOC_FREE(conn->nt_user_token);
	}

	free_namearray(conn->veto_list);
	free_namearray(conn->hide_list);
	free_namearray(conn->veto_oplock_list);
	free_namearray(conn->aio_write_behind_list);
	
	string_free(&conn->user);
	string_free(&conn->dirpath);
	string_free(&conn->connectpath);
	string_free(&conn->origpath);

	mem_ctx = conn->mem_ctx;
	ZERO_STRUCTP(conn);
	talloc_destroy(mem_ctx);
}

/****************************************************************************
 Free a conn structure.
****************************************************************************/

void conn_free(connection_struct *conn)
{
	DLIST_REMOVE(Connections, conn);

	bitmap_clear(bmap, conn->cnum);
	num_open--;

	conn_free_internal(conn);
}
 
/****************************************************************************
receive a smbcontrol message to forcibly unmount a share
the message contains just a share name and all instances of that
share are unmounted
the special sharename '*' forces unmount of all shares
****************************************************************************/
void msg_force_tdis(int msg_type, struct process_id pid, void *buf, size_t len)
{
	connection_struct *conn, *next;
	fstring sharename;

	fstrcpy(sharename, (const char *)buf);

	if (strcmp(sharename, "*") == 0) {
		DEBUG(1,("Forcing close of all shares\n"));
		conn_close_all();
		return;
	}

	for (conn=Connections;conn;conn=next) {
		next=conn->next;
		if (strequal(lp_servicename(conn->service), sharename)) {
			DEBUG(1,("Forcing close of share %s cnum=%d\n",
				 sharename, conn->cnum));
			close_cnum(conn, (uint16)-1);
		}
	}
}
