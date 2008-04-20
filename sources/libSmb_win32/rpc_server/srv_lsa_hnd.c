/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Andrew Tridgell              1992-1997,
 *  Copyright (C) Luke Kenneth Casson Leighton 1996-1997,
 *  Copyright (C) Jeremy Allison			   2001.
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/* This is the max handles across all instances of a pipe name. */
#ifndef MAX_OPEN_POLS
#define MAX_OPEN_POLS 1024
#endif

/****************************************************************************
 Hack as handles need to be persisant over lsa pipe closes so long as a samr
 pipe is open. JRA.
****************************************************************************/

static BOOL is_samr_lsa_pipe(const char *pipe_name)
{
	return (strstr(pipe_name, "samr") || strstr(pipe_name, "lsa"));
}

/****************************************************************************
 Initialise a policy handle list on a pipe. Handle list is shared between all
 pipes of the same name.
****************************************************************************/

BOOL init_pipe_handle_list(pipes_struct *p, char *pipe_name)
{
	pipes_struct *plist = get_first_internal_pipe();
	struct handle_list *hl = NULL;

	for (plist = get_first_internal_pipe(); plist; plist = get_next_internal_pipe(plist)) {
		if (strequal( plist->name, pipe_name) ||
				(is_samr_lsa_pipe(plist->name) && is_samr_lsa_pipe(pipe_name))) {
			if (!plist->pipe_handles) {
				pstring msg;
				slprintf(msg, sizeof(msg)-1, "init_pipe_handles: NULL pipe_handle pointer in pipe %s",
						pipe_name );
				smb_panic(msg);
			}
			hl = plist->pipe_handles;
			break;
		}
	}

	if (!hl) {
		/*
		 * No handle list for this pipe (first open of pipe).
		 * Create list.
		 */

		if ((hl = SMB_MALLOC_P(struct handle_list)) == NULL)
			return False;
		ZERO_STRUCTP(hl);

		DEBUG(10,("init_pipe_handles: created handle list for pipe %s\n", pipe_name ));
	}

	/*
	 * One more pipe is using this list.
	 */

	hl->pipe_ref_count++;

	/*
	 * Point this pipe at this list.
	 */

	p->pipe_handles = hl;

	DEBUG(10,("init_pipe_handles: pipe_handles ref count = %lu for pipe %s\n",
		  (unsigned long)p->pipe_handles->pipe_ref_count, pipe_name ));

	return True;
}

/****************************************************************************
  find first available policy slot.  creates a policy handle for you.
****************************************************************************/

BOOL create_policy_hnd(pipes_struct *p, POLICY_HND *hnd, void (*free_fn)(void *), void *data_ptr)
{
	static uint32 pol_hnd_low  = 0;
	static uint32 pol_hnd_high = 0;

	struct policy *pol;

	if (p->pipe_handles->count > MAX_OPEN_POLS) {
		DEBUG(0,("create_policy_hnd: ERROR: too many handles (%d) on this pipe.\n",
				(int)p->pipe_handles->count));
		return False;
	}

	pol = SMB_MALLOC_P(struct policy);
	if (!pol) {
		DEBUG(0,("create_policy_hnd: ERROR: out of memory!\n"));
		return False;
	}

	ZERO_STRUCTP(pol);

	pol->data_ptr = data_ptr;
	pol->free_fn = free_fn;

	pol_hnd_low++;
	if (pol_hnd_low == 0)
		(pol_hnd_high)++;

	SIVAL(&pol->pol_hnd.data1, 0 , 0);  /* first bit must be null */
	SIVAL(&pol->pol_hnd.data2, 0 , pol_hnd_low ); /* second bit is incrementing */
	SSVAL(&pol->pol_hnd.data3, 0 , pol_hnd_high); /* second bit is incrementing */
	SSVAL(&pol->pol_hnd.data4, 0 , (pol_hnd_high>>16)); /* second bit is incrementing */
	SIVAL(pol->pol_hnd.data5, 0, time(NULL)); /* something random */
	SIVAL(pol->pol_hnd.data5, 4, sys_getpid()); /* something more random */

	DLIST_ADD(p->pipe_handles->Policy, pol);
	p->pipe_handles->count++;

	*hnd = pol->pol_hnd;
	
	DEBUG(4,("Opened policy hnd[%d] ", (int)p->pipe_handles->count));
	dump_data(4, (char *)hnd, sizeof(*hnd));

	return True;
}

/****************************************************************************
  find policy by handle - internal version.
****************************************************************************/

static struct policy *find_policy_by_hnd_internal(pipes_struct *p, POLICY_HND *hnd, void **data_p)
{
	struct policy *pol;
	size_t i;

	if (data_p)
		*data_p = NULL;

	for (i = 0, pol=p->pipe_handles->Policy;pol;pol=pol->next, i++) {
		if (memcmp(&pol->pol_hnd, hnd, sizeof(*hnd)) == 0) {
			DEBUG(4,("Found policy hnd[%d] ", (int)i));
			dump_data(4, (char *)hnd, sizeof(*hnd));
			if (data_p)
				*data_p = pol->data_ptr;
			return pol;
		}
	}

	DEBUG(4,("Policy not found: "));
	dump_data(4, (char *)hnd, sizeof(*hnd));

	p->bad_handle_fault_state = True;

	return NULL;
}

/****************************************************************************
  find policy by handle
****************************************************************************/

BOOL find_policy_by_hnd(pipes_struct *p, POLICY_HND *hnd, void **data_p)
{
	return find_policy_by_hnd_internal(p, hnd, data_p) == NULL ? False : True;
}

/****************************************************************************
  Close a policy.
****************************************************************************/

BOOL close_policy_hnd(pipes_struct *p, POLICY_HND *hnd)
{
	struct policy *pol = find_policy_by_hnd_internal(p, hnd, NULL);

	if (!pol) {
		DEBUG(3,("Error closing policy\n"));
		return False;
	}

	DEBUG(3,("Closed policy\n"));

	if (pol->free_fn && pol->data_ptr)
		(*pol->free_fn)(pol->data_ptr);

	p->pipe_handles->count--;

	DLIST_REMOVE(p->pipe_handles->Policy, pol);

	ZERO_STRUCTP(pol);

	SAFE_FREE(pol);

	return True;
}

/****************************************************************************
 Close a pipe - free the handle list if it was the last pipe reference.
****************************************************************************/

void close_policy_by_pipe(pipes_struct *p)
{
	p->pipe_handles->pipe_ref_count--;

	if (p->pipe_handles->pipe_ref_count == 0) {
		/*
		 * Last pipe open on this list - free the list.
		 */
		while (p->pipe_handles->Policy)
			close_policy_hnd(p, &p->pipe_handles->Policy->pol_hnd);

		p->pipe_handles->Policy = NULL;
		p->pipe_handles->count = 0;

		SAFE_FREE(p->pipe_handles);
		DEBUG(10,("close_policy_by_pipe: deleted handle list for pipe %s\n", p->name ));
	}
}

/*******************************************************************
Shall we allow access to this rpc?  Currently this function
implements the 'restrict anonymous' setting by denying access to
anonymous users if the restrict anonymous level is > 0.  Further work
will be checking a security descriptor to determine whether a user
token has enough access to access the pipe.
********************************************************************/

BOOL pipe_access_check(pipes_struct *p)
{
	/* Don't let anonymous users access this RPC if restrict
	   anonymous > 0 */

	if (lp_restrict_anonymous() > 0) {
		user_struct *user = get_valid_user_struct(p->vuid);

		/* schannel, so we must be ok */
		if (p->pipe_bound && (p->auth.auth_type == PIPE_AUTH_TYPE_SCHANNEL)) {
			return True;
		}

		if (!user) {
			DEBUG(3, ("invalid vuid %d\n", p->vuid));
			return False;
		}

		if (user->guest) {
			return False;
		}
	}

	return True;
}
