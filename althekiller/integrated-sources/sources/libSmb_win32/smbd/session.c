/* 
   Unix SMB/CIFS implementation.
   session handling for utmp and PAM
   Copyright (C) tridge@samba.org 2001
   Copyright (C) abartlet@pcug.org.au 2001
   
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

/* a "session" is claimed when we do a SessionSetupX operation
   and is yielded when the corresponding vuid is destroyed.

   sessions are used to populate utmp and PAM session structures
*/

#include "includes.h"

static TDB_CONTEXT *tdb;

BOOL session_init(void)
{
	if (tdb)
		return True;

	tdb = tdb_open_log(lock_path("sessionid.tdb"), 0, TDB_CLEAR_IF_FIRST|TDB_DEFAULT, 
		       O_RDWR | O_CREAT, 0644);
	if (!tdb) {
		DEBUG(1,("session_init: failed to open sessionid tdb\n"));
		return False;
	}

	return True;
}

/* called when a session is created */
BOOL session_claim(user_struct *vuser)
{
	int i = 0;
	TDB_DATA data;
	struct sockaddr sa;
	struct in_addr *client_ip;
	struct sessionid sessionid;
	uint32 pid = (uint32)sys_getpid();
	TDB_DATA key;		
	fstring keystr;
	char * hostname;
	int tdb_store_flag;  /* If using utmp, we do an inital 'lock hold' store,
				but we don't need this if we are just using the 
				(unique) pid/vuid combination */

	vuser->session_keystr = NULL;

	/* don't register sessions for the guest user - its just too
	   expensive to go through pam session code for browsing etc */
	if (vuser->guest) {
		return True;
	}

	if (!session_init())
		return False;

	ZERO_STRUCT(sessionid);

	data.dptr = NULL;
	data.dsize = 0;

	if (lp_utmp()) {
		for (i=1;i<MAX_SESSION_ID;i++) {
			slprintf(keystr, sizeof(keystr)-1, "ID/%d", i);
			key.dptr = keystr;
			key.dsize = strlen(keystr)+1;
			
			if (tdb_store(tdb, key, data, TDB_INSERT) == 0) break;
		}
		
		if (i == MAX_SESSION_ID) {
			DEBUG(1,("session_claim: out of session IDs (max is %d)\n", 
				 MAX_SESSION_ID));
			return False;
		}
		slprintf(sessionid.id_str, sizeof(sessionid.id_str)-1, SESSION_UTMP_TEMPLATE, i);
		tdb_store_flag = TDB_MODIFY;
	} else
	{
		slprintf(keystr, sizeof(keystr)-1, "ID/%lu/%u", 
			 (long unsigned int)sys_getpid(), 
			 vuser->vuid);
		slprintf(sessionid.id_str, sizeof(sessionid.id_str)-1, 
			 SESSION_TEMPLATE, (long unsigned int)sys_getpid(), 
			 vuser->vuid);

		key.dptr = keystr;
		key.dsize = strlen(keystr)+1;
			
		tdb_store_flag = TDB_REPLACE;
	}

	/* If 'hostname lookup' == yes, then do the DNS lookup.  This is
           needed because utmp and PAM both expect DNS names 
	   
	   client_name() handles this case internally.
	*/

	hostname = client_name();
	if (strcmp(hostname, "UNKNOWN") == 0) {
		hostname = client_addr();
	}

	fstrcpy(sessionid.username, vuser->user.unix_name);
	fstrcpy(sessionid.hostname, hostname);
	sessionid.id_num = i;  /* Only valid for utmp sessions */
	sessionid.pid = pid;
	sessionid.uid = vuser->uid;
	sessionid.gid = vuser->gid;
	fstrcpy(sessionid.remote_machine, get_remote_machine_name());
	fstrcpy(sessionid.ip_addr, client_addr());

	client_ip = client_inaddr(&sa);

	if (!smb_pam_claim_session(sessionid.username, sessionid.id_str, sessionid.hostname)) {
		DEBUG(1,("pam_session rejected the session for %s [%s]\n",
				sessionid.username, sessionid.id_str));
		if (tdb_store_flag == TDB_MODIFY) {
			tdb_delete(tdb, key);
		}
		return False;
	}

	data.dptr = (char *)&sessionid;
	data.dsize = sizeof(sessionid);
	if (tdb_store(tdb, key, data, tdb_store_flag) != 0) {
		DEBUG(1,("session_claim: unable to create session id record\n"));
		return False;
	}

	if (lp_utmp()) {
		sys_utmp_claim(sessionid.username, sessionid.hostname, 
			       client_ip,
			       sessionid.id_str, sessionid.id_num);
	}

	vuser->session_keystr = SMB_STRDUP(keystr);
	if (!vuser->session_keystr) {
		DEBUG(0, ("session_claim:  strdup() failed for session_keystr\n"));
		return False;
	}
	return True;
}

/* called when a session is destroyed */
void session_yield(user_struct *vuser)
{
	TDB_DATA dbuf;
	struct sessionid sessionid;
	struct in_addr *client_ip;
	TDB_DATA key;

	if (!tdb) return;

	if (!vuser->session_keystr) {
		return;
	}

	key.dptr = vuser->session_keystr;
	key.dsize = strlen(vuser->session_keystr)+1;

	dbuf = tdb_fetch(tdb, key);

	if (dbuf.dsize != sizeof(sessionid))
		return;

	memcpy(&sessionid, dbuf.dptr, sizeof(sessionid));

	client_ip = interpret_addr2(sessionid.ip_addr);

	SAFE_FREE(dbuf.dptr);

	if (lp_utmp()) {
		sys_utmp_yield(sessionid.username, sessionid.hostname, 
			       client_ip,
			       sessionid.id_str, sessionid.id_num);
	}

	smb_pam_close_session(sessionid.username, sessionid.id_str, sessionid.hostname);

	tdb_delete(tdb, key);
}

BOOL session_traverse(int (*fn)(TDB_CONTEXT *, TDB_DATA, TDB_DATA, void *),
		      void *state)
{
	if (!session_init()) {
		DEBUG(3, ("No tdb opened\n"));
		return False;
	}

	tdb_traverse(tdb, fn, state);
	return True;
}

struct session_list {
	int count;
	struct sessionid *sessions;
};

static int gather_sessioninfo(TDB_CONTEXT *stdb, TDB_DATA kbuf, TDB_DATA dbuf,
			      void *state)
{
	struct session_list *sesslist = (struct session_list *) state;
	const struct sessionid *current = (const struct sessionid *) dbuf.dptr;

	sesslist->count += 1;
	sesslist->sessions = SMB_REALLOC_ARRAY(sesslist->sessions, struct sessionid,
					sesslist->count);
	if (!sesslist->sessions) {
		sesslist->count = 0;
		return -1;
	}

	memcpy(&sesslist->sessions[sesslist->count - 1], current, 
	       sizeof(struct sessionid));
	DEBUG(7,("gather_sessioninfo session from %s@%s\n", 
		 current->username, current->remote_machine));
	return 0;
}

int list_sessions(struct sessionid **session_list)
{
	struct session_list sesslist;

	sesslist.count = 0;
	sesslist.sessions = NULL;
	
	if (!session_traverse(gather_sessioninfo, (void *) &sesslist)) {
		DEBUG(3, ("Session traverse failed\n"));
		SAFE_FREE(sesslist.sessions);
		*session_list = NULL;
		return 0;
	}

	*session_list = sesslist.sessions;
	return sesslist.count;
}
