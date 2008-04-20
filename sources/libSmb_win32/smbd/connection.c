/* 
   Unix SMB/CIFS implementation.
   connection claim routines
   Copyright (C) Andrew Tridgell 1998
   
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

static TDB_CONTEXT *tdb;

/****************************************************************************
 Return the connection tdb context (used for message send all).
****************************************************************************/

TDB_CONTEXT *conn_tdb_ctx(void)
{
	if (!tdb)
		tdb = tdb_open_log(lock_path("connections.tdb"), 0, TDB_CLEAR_IF_FIRST|TDB_DEFAULT, 
			       O_RDWR | O_CREAT, 0644);

	return tdb;
}

static void make_conn_key(connection_struct *conn, const char *name, TDB_DATA *pkbuf, struct connections_key *pkey)
{
	ZERO_STRUCTP(pkey);
	pkey->pid = procid_self();
	pkey->cnum = conn?conn->cnum:-1;
	fstrcpy(pkey->name, name);
#ifdef DEVELOPER
	/* valgrind fixer... */
	{
		size_t sl = strlen(pkey->name);
		if (sizeof(fstring)-sl)
			memset(&pkey->name[sl], '\0', sizeof(fstring)-sl);
	}
#endif

	pkbuf->dptr = (char *)pkey;
	pkbuf->dsize = sizeof(*pkey);
}

/****************************************************************************
 Delete a connection record.
****************************************************************************/

BOOL yield_connection(connection_struct *conn, const char *name)
{
	struct connections_key key;
	TDB_DATA kbuf;

	if (!tdb)
		return False;

	DEBUG(3,("Yielding connection to %s\n",name));

	make_conn_key(conn, name, &kbuf, &key);

	if (tdb_delete(tdb, kbuf) != 0) {
		int dbg_lvl = (!conn && (tdb_error(tdb) == TDB_ERR_NOEXIST)) ? 3 : 0;
		DEBUG(dbg_lvl,("yield_connection: tdb_delete for name %s failed with error %s.\n",
			name, tdb_errorstr(tdb) ));
		return (False);
	}

	return(True);
}

struct count_stat {
	pid_t mypid;
	int curr_connections;
	char *name;
	BOOL Clear;
};

/****************************************************************************
 Count the entries belonging to a service in the connection db.
****************************************************************************/

static int count_fn( TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *udp)
{
	struct connections_data crec;
	struct count_stat *cs = (struct count_stat *)udp;
 
	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));
 
	if (crec.cnum == -1)
		return 0;

	/* If the pid was not found delete the entry from connections.tdb */

	if (cs->Clear && !process_exists(crec.pid) && (errno == ESRCH)) {
		DEBUG(2,("pid %s doesn't exist - deleting connections %d [%s]\n",
			procid_str_static(&crec.pid), crec.cnum, crec.name));
		if (tdb_delete(the_tdb, kbuf) != 0)
			DEBUG(0,("count_fn: tdb_delete failed with error %s\n", tdb_errorstr(tdb) ));
		return 0;
	}

	if (strequal(crec.name, cs->name))
		cs->curr_connections++;

	return 0;
}

/****************************************************************************
 Claim an entry in the connections database.
****************************************************************************/

BOOL claim_connection(connection_struct *conn, const char *name,int max_connections,BOOL Clear, uint32 msg_flags)
{
	struct connections_key key;
	struct connections_data crec;
	TDB_DATA kbuf, dbuf;

	if (!tdb)
		tdb = tdb_open_log(lock_path("connections.tdb"), 0, TDB_CLEAR_IF_FIRST|TDB_DEFAULT, 
			       O_RDWR | O_CREAT, 0644);

	if (!tdb)
		return False;

	/*
	 * Enforce the max connections parameter.
	 */

	if (max_connections > 0) {
		struct count_stat cs;

		cs.mypid = sys_getpid();
		cs.curr_connections = 0;
		cs.name = lp_servicename(SNUM(conn));
		cs.Clear = Clear;

		/*
		 * This has a race condition, but locking the chain before hand is worse
		 * as it leads to deadlock.
		 */

		if (tdb_traverse(tdb, count_fn, &cs) == -1) {
			DEBUG(0,("claim_connection: traverse of connections.tdb failed with error %s.\n",
				tdb_errorstr(tdb) ));
			return False;
		}

		if (cs.curr_connections >= max_connections) {
			DEBUG(1,("claim_connection: Max connections (%d) exceeded for %s\n",
				max_connections, name ));
			return False;
		}
	}

	DEBUG(5,("claiming %s %d\n",name,max_connections));

	make_conn_key(conn, name, &kbuf, &key);

	/* fill in the crec */
	ZERO_STRUCT(crec);
	crec.magic = 0x280267;
	crec.pid = procid_self();
	crec.cnum = conn?conn->cnum:-1;
	if (conn) {
		crec.uid = conn->uid;
		crec.gid = conn->gid;
		safe_strcpy(crec.name,
			    lp_servicename(SNUM(conn)),sizeof(crec.name)-1);
	}
	crec.start = time(NULL);
	crec.bcast_msg_flags = msg_flags;
	
	safe_strcpy(crec.machine,get_remote_machine_name(),sizeof(crec.machine)-1);
	safe_strcpy(crec.addr,conn?conn->client_address:client_addr(),sizeof(crec.addr)-1);

	dbuf.dptr = (char *)&crec;
	dbuf.dsize = sizeof(crec);

	if (tdb_store(tdb, kbuf, dbuf, TDB_REPLACE) != 0) {
		DEBUG(0,("claim_connection: tdb_store failed with error %s.\n",
			tdb_errorstr(tdb) ));
		return False;
	}

	return True;
}

BOOL register_message_flags(BOOL doreg, uint32 msg_flags)
{
	struct connections_key key;
	struct connections_data *pcrec;
	TDB_DATA kbuf, dbuf;

	if (!tdb)
		return False;

	DEBUG(10,("register_message_flags: %s flags 0x%x\n",
		doreg ? "adding" : "removing",
		(unsigned int)msg_flags ));

	make_conn_key(NULL, "", &kbuf, &key);

        dbuf = tdb_fetch(tdb, kbuf);
        if (!dbuf.dptr) {
		DEBUG(0,("register_message_flags: tdb_fetch failed: %s\n",
			tdb_errorstr(tdb)));
		return False;
	}

	pcrec = (struct connections_data *)dbuf.dptr;
	if (doreg)
		pcrec->bcast_msg_flags |= msg_flags;
	else
		pcrec->bcast_msg_flags &= ~msg_flags;

	if (tdb_store(tdb, kbuf, dbuf, TDB_REPLACE) != 0) {
		DEBUG(0,("register_message_flags: tdb_store failed: %s.\n",
			tdb_errorstr(tdb) ));
		SAFE_FREE(dbuf.dptr);
		return False;
	}

	DEBUG(10,("register_message_flags: new flags 0x%x\n",
		(unsigned int)pcrec->bcast_msg_flags ));

	SAFE_FREE(dbuf.dptr);
	return True;
}
