/* 
   Unix SMB/CIFS implementation.
   uid/user handling
   Copyright (C) Andrew Tridgell 1992-1998
   
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

/* what user is current? */
extern struct current_user current_user;

/****************************************************************************
 Iterator functions for getting all gid's from current_user.
****************************************************************************/

gid_t get_current_user_gid_first(int *piterator)
{
	*piterator = 0;
	return current_user.ut.gid;
}

gid_t get_current_user_gid_next(int *piterator)
{
	gid_t ret;

	if (!current_user.ut.groups || *piterator >= current_user.ut.ngroups) {
		return (gid_t)-1;
	}

	ret = current_user.ut.groups[*piterator];
	(*piterator) += 1;
	return ret;
}

/****************************************************************************
 Become the guest user without changing the security context stack.
****************************************************************************/

BOOL change_to_guest(void)
{
	static struct passwd *pass=NULL;

	if (!pass) {
		/* Don't need to free() this as its stored in a static */
		pass = getpwnam_alloc(NULL, lp_guestaccount());
		if (!pass)
			return(False);
	}
	
#ifdef AIX
	/* MWW: From AIX FAQ patch to WU-ftpd: call initgroups before 
	   setting IDs */
	initgroups(pass->pw_name, pass->pw_gid);
#endif
	
	set_sec_ctx(pass->pw_uid, pass->pw_gid, 0, NULL, NULL);
	
	current_user.conn = NULL;
	current_user.vuid = UID_FIELD_INVALID;

	TALLOC_FREE(pass);
	pass = NULL;
	
	return True;
}

/*******************************************************************
 Check if a username is OK.
********************************************************************/

static BOOL check_user_ok(connection_struct *conn, user_struct *vuser,int snum)
{
	unsigned int i;
	struct vuid_cache_entry *ent = NULL;
	BOOL readonly_share;

	for (i=0;i<conn->vuid_cache.entries && i< VUID_CACHE_SIZE;i++) {
		if (conn->vuid_cache.array[i].vuid == vuser->vuid) {
			ent = &conn->vuid_cache.array[i];
			conn->read_only = ent->read_only;
			conn->admin_user = ent->admin_user;
			return(True);
		}
	}

	if (!user_ok_token(vuser->user.unix_name, vuser->nt_user_token, snum))
		return(False);

	readonly_share = is_share_read_only_for_token(vuser->user.unix_name,
						      vuser->nt_user_token,
						      conn->service);

	if (!readonly_share &&
	    !share_access_check(conn, snum, vuser, FILE_WRITE_DATA)) {
		/* smb.conf allows r/w, but the security descriptor denies
		 * write. Fall back to looking at readonly. */
		readonly_share = True;
		DEBUG(5,("falling back to read-only access-evaluation due to "
			 "security descriptor\n"));
	}

	if (!share_access_check(conn, snum, vuser,
				readonly_share ?
				FILE_READ_DATA : FILE_WRITE_DATA)) {
		return False;
	}

	i = conn->vuid_cache.entries % VUID_CACHE_SIZE;
	if (conn->vuid_cache.entries < VUID_CACHE_SIZE)
		conn->vuid_cache.entries++;

	ent = &conn->vuid_cache.array[i];
	ent->vuid = vuser->vuid;
	ent->read_only = readonly_share;

	ent->admin_user = token_contains_name_in_list(
		vuser->user.unix_name, NULL, vuser->nt_user_token,
		lp_admin_users(conn->service));

	conn->read_only = ent->read_only;
	conn->admin_user = ent->admin_user;

	return(True);
}

/****************************************************************************
 Become the user of a connection number without changing the security context
 stack, but modify the current_user entries.
****************************************************************************/

BOOL change_to_user(connection_struct *conn, uint16 vuid)
{
	user_struct *vuser = get_valid_user_struct(vuid);
	int snum;
	gid_t gid;
	uid_t uid;
	char group_c;
	BOOL must_free_token = False;
	NT_USER_TOKEN *token = NULL;

	if (!conn) {
		DEBUG(2,("change_to_user: Connection not open\n"));
		return(False);
	}

	/*
	 * We need a separate check in security=share mode due to vuid
	 * always being UID_FIELD_INVALID. If we don't do this then
	 * in share mode security we are *always* changing uid's between
	 * SMB's - this hurts performance - Badly.
	 */

	if((lp_security() == SEC_SHARE) && (current_user.conn == conn) &&
	   (current_user.ut.uid == conn->uid)) {
		DEBUG(4,("change_to_user: Skipping user change - already "
			 "user\n"));
		return(True);
	} else if ((current_user.conn == conn) && 
		   (vuser != 0) && (current_user.vuid == vuid) && 
		   (current_user.ut.uid == vuser->uid)) {
		DEBUG(4,("change_to_user: Skipping user change - already "
			 "user\n"));
		return(True);
	}

	snum = SNUM(conn);

	if ((vuser) && !check_user_ok(conn, vuser, snum)) {
		DEBUG(2,("change_to_user: SMB user %s (unix user %s, vuid %d) "
			 "not permitted access to share %s.\n",
			 vuser->user.smb_name, vuser->user.unix_name, vuid,
			 lp_servicename(snum)));
		return False;
	}

	if (conn->force_user) /* security = share sets this too */ {
		uid = conn->uid;
		gid = conn->gid;
		current_user.ut.groups = conn->groups;
		current_user.ut.ngroups = conn->ngroups;
		token = conn->nt_user_token;
	} else if (vuser) {
		uid = conn->admin_user ? 0 : vuser->uid;
		gid = vuser->gid;
		current_user.ut.ngroups = vuser->n_groups;
		current_user.ut.groups  = vuser->groups;
		token = vuser->nt_user_token;
	} else {
		DEBUG(2,("change_to_user: Invalid vuid used %d in accessing "
			 "share %s.\n",vuid, lp_servicename(snum) ));
		return False;
	}

	/*
	 * See if we should force group for this service.
	 * If so this overrides any group set in the force
	 * user code.
	 */

	if((group_c = *lp_force_group(snum))) {

		token = dup_nt_token(NULL, token);
		if (token == NULL) {
			DEBUG(0, ("dup_nt_token failed\n"));
			return False;
		}
		must_free_token = True;

		if(group_c == '+') {

			/*
			 * Only force group if the user is a member of
			 * the service group. Check the group memberships for
			 * this user (we already have this) to
			 * see if we should force the group.
			 */

			int i;
			for (i = 0; i < current_user.ut.ngroups; i++) {
				if (current_user.ut.groups[i] == conn->gid) {
					gid = conn->gid;
					gid_to_sid(&token->user_sids[1], gid);
					break;
				}
			}
		} else {
			gid = conn->gid;
			gid_to_sid(&token->user_sids[1], gid);
		}
	}
	
	set_sec_ctx(uid, gid, current_user.ut.ngroups, current_user.ut.groups,
		    token);

	/*
	 * Free the new token (as set_sec_ctx copies it).
	 */

	if (must_free_token)
		TALLOC_FREE(token);

	current_user.conn = conn;
	current_user.vuid = vuid;

	DEBUG(5,("change_to_user uid=(%d,%d) gid=(%d,%d)\n",
		 (int)getuid(),(int)geteuid(),(int)getgid(),(int)getegid()));
  
	return(True);
}

/****************************************************************************
 Go back to being root without changing the security context stack,
 but modify the current_user entries.
****************************************************************************/

BOOL change_to_root_user(void)
{
	set_root_sec_ctx();

	DEBUG(5,("change_to_root_user: now uid=(%d,%d) gid=(%d,%d)\n",
		(int)getuid(),(int)geteuid(),(int)getgid(),(int)getegid()));

	current_user.conn = NULL;
	current_user.vuid = UID_FIELD_INVALID;

	return(True);
}

/****************************************************************************
 Become the user of an authenticated connected named pipe.
 When this is called we are currently running as the connection
 user. Doesn't modify current_user.
****************************************************************************/

BOOL become_authenticated_pipe_user(pipes_struct *p)
{
	if (!push_sec_ctx())
		return False;

	set_sec_ctx(p->pipe_user.ut.uid, p->pipe_user.ut.gid, 
		    p->pipe_user.ut.ngroups, p->pipe_user.ut.groups,
		    p->pipe_user.nt_user_token);

	return True;
}

/****************************************************************************
 Unbecome the user of an authenticated connected named pipe.
 When this is called we are running as the authenticated pipe
 user and need to go back to being the connection user. Doesn't modify
 current_user.
****************************************************************************/

BOOL unbecome_authenticated_pipe_user(void)
{
	return pop_sec_ctx();
}

/****************************************************************************
 Utility functions used by become_xxx/unbecome_xxx.
****************************************************************************/

struct conn_ctx {
	connection_struct *conn;
	uint16 vuid;
};
 
/* A stack of current_user connection contexts. */
 
static struct conn_ctx conn_ctx_stack[MAX_SEC_CTX_DEPTH];
static int conn_ctx_stack_ndx;

static void push_conn_ctx(void)
{
	struct conn_ctx *ctx_p;
 
	/* Check we don't overflow our stack */
 
	if (conn_ctx_stack_ndx == MAX_SEC_CTX_DEPTH) {
		DEBUG(0, ("Connection context stack overflow!\n"));
		smb_panic("Connection context stack overflow!\n");
	}
 
	/* Store previous user context */
	ctx_p = &conn_ctx_stack[conn_ctx_stack_ndx];
 
	ctx_p->conn = current_user.conn;
	ctx_p->vuid = current_user.vuid;
 
	DEBUG(3, ("push_conn_ctx(%u) : conn_ctx_stack_ndx = %d\n",
		(unsigned int)ctx_p->vuid, conn_ctx_stack_ndx ));

	conn_ctx_stack_ndx++;
}

static void pop_conn_ctx(void)
{
	struct conn_ctx *ctx_p;
 
	/* Check for stack underflow. */

	if (conn_ctx_stack_ndx == 0) {
		DEBUG(0, ("Connection context stack underflow!\n"));
		smb_panic("Connection context stack underflow!\n");
	}

	conn_ctx_stack_ndx--;
	ctx_p = &conn_ctx_stack[conn_ctx_stack_ndx];

	current_user.conn = ctx_p->conn;
	current_user.vuid = ctx_p->vuid;

	ctx_p->conn = NULL;
	ctx_p->vuid = UID_FIELD_INVALID;
}

/****************************************************************************
 Temporarily become a root user.  Must match with unbecome_root(). Saves and
 restores the connection context.
****************************************************************************/

void become_root(void)
{
	push_sec_ctx();
	push_conn_ctx();
	set_root_sec_ctx();
}

/* Unbecome the root user */

void unbecome_root(void)
{
	pop_sec_ctx();
	pop_conn_ctx();
}

/****************************************************************************
 Push the current security context then force a change via change_to_user().
 Saves and restores the connection context.
****************************************************************************/

BOOL become_user(connection_struct *conn, uint16 vuid)
{
	if (!push_sec_ctx())
		return False;

	push_conn_ctx();

	if (!change_to_user(conn, vuid)) {
		pop_sec_ctx();
		pop_conn_ctx();
		return False;
	}

	return True;
}

BOOL unbecome_user(void)
{
	pop_sec_ctx();
	pop_conn_ctx();
	return True;
}

