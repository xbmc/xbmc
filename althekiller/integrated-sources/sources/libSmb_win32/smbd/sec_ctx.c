/* 
   Unix SMB/CIFS implementation.
   uid/user handling
   Copyright (C) Tim Potter 2000
   
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

extern struct current_user current_user;

struct sec_ctx {
	UNIX_USER_TOKEN ut;
	NT_USER_TOKEN *token;
};

/* A stack of security contexts.  We include the current context as being
   the first one, so there is room for another MAX_SEC_CTX_DEPTH more. */

static struct sec_ctx sec_ctx_stack[MAX_SEC_CTX_DEPTH + 1];
static int sec_ctx_stack_ndx;

/****************************************************************************
 Become the specified uid.
****************************************************************************/

static BOOL become_uid(uid_t uid)
{
	/* Check for dodgy uid values */

	if (uid == (uid_t)-1 || 
	    ((sizeof(uid_t) == 2) && (uid == (uid_t)65535))) {
		static int done;
 
		if (!done) {
			DEBUG(1,("WARNING: using uid %d is a security risk\n",
				 (int)uid));
			done = 1;
		}
	}

	/* Set effective user id */

	set_effective_uid(uid);

	DO_PROFILE_INC(uid_changes);
	return True;
}

/****************************************************************************
 Become the specified gid.
****************************************************************************/

static BOOL become_gid(gid_t gid)
{
	/* Check for dodgy gid values */

	if (gid == (gid_t)-1 || ((sizeof(gid_t) == 2) && 
				 (gid == (gid_t)65535))) {
		static int done;
		
		if (!done) {
			DEBUG(1,("WARNING: using gid %d is a security risk\n",
				 (int)gid));  
			done = 1;
		}
	}
  
	/* Set effective group id */

	set_effective_gid(gid);
	return True;
}

/****************************************************************************
 Become the specified uid and gid.
****************************************************************************/

static BOOL become_id(uid_t uid, gid_t gid)
{
	return become_gid(gid) && become_uid(uid);
}

/****************************************************************************
 Drop back to root privileges in order to change to another user.
****************************************************************************/

static void gain_root(void)
{
	if (non_root_mode()) {
		return;
	}

	if (geteuid() != 0) {
		set_effective_uid(0);

		if (geteuid() != 0) {
			DEBUG(0,
			      ("Warning: You appear to have a trapdoor "
			       "uid system\n"));
		}
	}

	if (getegid() != 0) {
		set_effective_gid(0);

		if (getegid() != 0) {
			DEBUG(0,
			      ("Warning: You appear to have a trapdoor "
			       "gid system\n"));
		}
	}
}

/****************************************************************************
 Get the list of current groups.
****************************************************************************/

static int get_current_groups(gid_t gid, int *p_ngroups, gid_t **p_groups)
{
	int i;
	gid_t grp;
	int ngroups;
	gid_t *groups = NULL;

	(*p_ngroups) = 0;
	(*p_groups) = NULL;

	/* this looks a little strange, but is needed to cope with
	   systems that put the current egid in the group list
	   returned from getgroups() (tridge) */
	save_re_gid();
	set_effective_gid(gid);
	setgid(gid);

	ngroups = sys_getgroups(0,&grp);
	if (ngroups <= 0) {
		goto fail;
	}

	if((groups = SMB_MALLOC_ARRAY(gid_t, ngroups+1)) == NULL) {
		DEBUG(0,("setup_groups malloc fail !\n"));
		goto fail;
	}

	if ((ngroups = sys_getgroups(ngroups,groups)) == -1) {
		goto fail;
	}

	restore_re_gid();

	(*p_ngroups) = ngroups;
	(*p_groups) = groups;

	DEBUG( 3, ( "get_current_groups: user is in %u groups: ", ngroups));
	for (i = 0; i < ngroups; i++ ) {
		DEBUG( 3, ( "%s%d", (i ? ", " : ""), (int)groups[i] ) );
	}
	DEBUG( 3, ( "\n" ) );

	return ngroups;

fail:
	SAFE_FREE(groups);
	restore_re_gid();
	return -1;
}

/****************************************************************************
 Create a new security context on the stack.  It is the same as the old
 one.  User changes are done using the set_sec_ctx() function.
****************************************************************************/

BOOL push_sec_ctx(void)
{
	struct sec_ctx *ctx_p;

	/* Check we don't overflow our stack */

	if (sec_ctx_stack_ndx == MAX_SEC_CTX_DEPTH) {
		DEBUG(0, ("Security context stack overflow!\n"));
		smb_panic("Security context stack overflow!\n");
	}

	/* Store previous user context */

	sec_ctx_stack_ndx++;

	ctx_p = &sec_ctx_stack[sec_ctx_stack_ndx];

	ctx_p->ut.uid = geteuid();
	ctx_p->ut.gid = getegid();

 	DEBUG(3, ("push_sec_ctx(%u, %u) : sec_ctx_stack_ndx = %d\n", 
 		  (unsigned int)ctx_p->ut.uid, (unsigned int)ctx_p->ut.gid, sec_ctx_stack_ndx ));

	ctx_p->token = dup_nt_token(NULL,
				    sec_ctx_stack[sec_ctx_stack_ndx-1].token);

	ctx_p->ut.ngroups = sys_getgroups(0, NULL);

	if (ctx_p->ut.ngroups != 0) {
		if (!(ctx_p->ut.groups = SMB_MALLOC_ARRAY(gid_t, ctx_p->ut.ngroups))) {
			DEBUG(0, ("Out of memory in push_sec_ctx()\n"));
			TALLOC_FREE(ctx_p->token);
			return False;
		}

		sys_getgroups(ctx_p->ut.ngroups, ctx_p->ut.groups);
	} else {
		ctx_p->ut.groups = NULL;
	}

	return True;
}

/****************************************************************************
 Set the current security context to a given user.
****************************************************************************/

void set_sec_ctx(uid_t uid, gid_t gid, int ngroups, gid_t *groups, NT_USER_TOKEN *token)
{
	struct sec_ctx *ctx_p = &sec_ctx_stack[sec_ctx_stack_ndx];
	
	/* Set the security context */

	DEBUG(3, ("setting sec ctx (%u, %u) - sec_ctx_stack_ndx = %d\n", 
		(unsigned int)uid, (unsigned int)gid, sec_ctx_stack_ndx));

	debug_nt_user_token(DBGC_CLASS, 5, token);
	debug_unix_user_token(DBGC_CLASS, 5, uid, gid, ngroups, groups);

	gain_root();

#ifdef HAVE_SETGROUPS
	sys_setgroups(ngroups, groups);
#endif

	ctx_p->ut.ngroups = ngroups;

	SAFE_FREE(ctx_p->ut.groups);
	if (token && (token == ctx_p->token))
		smb_panic("DUPLICATE_TOKEN");

	TALLOC_FREE(ctx_p->token);
	
	ctx_p->ut.groups = memdup(groups, sizeof(gid_t) * ngroups);
	ctx_p->token = dup_nt_token(NULL, token);

	become_id(uid, gid);

	ctx_p->ut.uid = uid;
	ctx_p->ut.gid = gid;

	/* Update current_user stuff */

	current_user.ut.uid = uid;
	current_user.ut.gid = gid;
	current_user.ut.ngroups = ngroups;
	current_user.ut.groups = groups;
	current_user.nt_user_token = ctx_p->token;
}

/****************************************************************************
 Become root context.
****************************************************************************/

void set_root_sec_ctx(void)
{
	/* May need to worry about supplementary groups at some stage */

	set_sec_ctx(0, 0, 0, NULL, NULL);
}

/****************************************************************************
 Pop a security context from the stack.
****************************************************************************/

BOOL pop_sec_ctx(void)
{
	struct sec_ctx *ctx_p;
	struct sec_ctx *prev_ctx_p;

	/* Check for stack underflow */

	if (sec_ctx_stack_ndx == 0) {
		DEBUG(0, ("Security context stack underflow!\n"));
		smb_panic("Security context stack underflow!\n");
	}

	ctx_p = &sec_ctx_stack[sec_ctx_stack_ndx];

	/* Clear previous user info */

	ctx_p->ut.uid = (uid_t)-1;
	ctx_p->ut.gid = (gid_t)-1;

	SAFE_FREE(ctx_p->ut.groups);
	ctx_p->ut.ngroups = 0;

	TALLOC_FREE(ctx_p->token);

	/* Pop back previous user */

	sec_ctx_stack_ndx--;

	gain_root();

	prev_ctx_p = &sec_ctx_stack[sec_ctx_stack_ndx];

#ifdef HAVE_SETGROUPS
	sys_setgroups(prev_ctx_p->ut.ngroups, prev_ctx_p->ut.groups);
#endif

	become_id(prev_ctx_p->ut.uid, prev_ctx_p->ut.gid);

	/* Update current_user stuff */

	current_user.ut.uid = prev_ctx_p->ut.uid;
	current_user.ut.gid = prev_ctx_p->ut.gid;
	current_user.ut.ngroups = prev_ctx_p->ut.ngroups;
	current_user.ut.groups = prev_ctx_p->ut.groups;
	current_user.nt_user_token = prev_ctx_p->token;

	DEBUG(3, ("pop_sec_ctx (%u, %u) - sec_ctx_stack_ndx = %d\n", 
		(unsigned int)geteuid(), (unsigned int)getegid(), sec_ctx_stack_ndx));

	return True;
}

/* Initialise the security context system */

void init_sec_ctx(void)
{
	int i;
	struct sec_ctx *ctx_p;

	/* Initialise security context stack */

	memset(sec_ctx_stack, 0, sizeof(struct sec_ctx) * MAX_SEC_CTX_DEPTH);

	for (i = 0; i < MAX_SEC_CTX_DEPTH; i++) {
		sec_ctx_stack[i].ut.uid = (uid_t)-1;
		sec_ctx_stack[i].ut.gid = (gid_t)-1;
	}

	/* Initialise first level of stack.  It is the current context */
	ctx_p = &sec_ctx_stack[0];

	ctx_p->ut.uid = geteuid();
	ctx_p->ut.gid = getegid();

	get_current_groups(ctx_p->ut.gid, &ctx_p->ut.ngroups, &ctx_p->ut.groups);

	ctx_p->token = NULL; /* Maps to guest user. */

	/* Initialise current_user global */

	current_user.ut.uid = ctx_p->ut.uid;
	current_user.ut.gid = ctx_p->ut.gid;
	current_user.ut.ngroups = ctx_p->ut.ngroups;
	current_user.ut.groups = ctx_p->ut.groups;

	/* The conn and vuid are usually taken care of by other modules.
	   We initialise them here. */

	current_user.conn = NULL;
	current_user.vuid = UID_FIELD_INVALID;
	current_user.nt_user_token = NULL;
}
