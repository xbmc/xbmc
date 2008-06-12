/*
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 *  Copyright (C) Jim McDonough (jmcd@us.ibm.com)   2003.
 *  Copyright (C) Gerald (Jerry) Carter             2002-2005.
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
#define DBGC_CLASS DBGC_RPC_PARSE

/*******************************************************************
Inits a structure.
********************************************************************/

void init_shutdown_q_init(SHUTDOWN_Q_INIT *q_s, const char *msg,
			uint32 timeout, BOOL do_reboot, BOOL force)
{
	q_s->server = TALLOC_P( get_talloc_ctx(), uint16 );
	if (!q_s->server) {
		smb_panic("init_shutdown_q_init: talloc fail.\n");
		return;
	}

	*q_s->server = 0x1;

	q_s->message = TALLOC_ZERO_P( get_talloc_ctx(), UNISTR4 );
	if (!q_s->message) {
		smb_panic("init_shutdown_q_init: talloc fail.\n");
		return;
	}

	if ( msg && *msg ) {
		init_unistr4( q_s->message, msg, UNI_FLAGS_NONE );

		/* Win2000 is apparently very sensitive to these lengths */
		/* do a special case here */

		q_s->message->string->uni_max_len++;
		q_s->message->size += 2;
	}

	q_s->timeout = timeout;

	q_s->reboot = do_reboot ? 1 : 0;
	q_s->force = force ? 1 : 0;
}

/*******************************************************************
********************************************************************/

void init_shutdown_q_init_ex(SHUTDOWN_Q_INIT_EX * q_u_ex, const char *msg,
			uint32 timeout, BOOL do_reboot, BOOL force, uint32 reason)
{
	SHUTDOWN_Q_INIT q_u;
	
	ZERO_STRUCT( q_u );
	
	init_shutdown_q_init( &q_u, msg, timeout, do_reboot, force );
	
	/* steal memory */
	
	q_u_ex->server  = q_u.server;
	q_u_ex->message = q_u.message;
	
	q_u_ex->reboot  = q_u.reboot;
	q_u_ex->force   = q_u.force;
	
	q_u_ex->reason = reason;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/

BOOL shutdown_io_q_init(const char *desc, SHUTDOWN_Q_INIT *q_s, prs_struct *ps,
			int depth)
{
	if (q_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_q_init");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_s->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	if (!prs_pointer("message", ps, depth, (void**)&q_s->message, sizeof(UNISTR4), (PRS_POINTER_CAST)prs_unistr4))
		return False;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("timeout", ps, depth, &(q_s->timeout)))
		return False;

	if (!prs_uint8("force  ", ps, depth, &(q_s->force)))
		return False;
	if (!prs_uint8("reboot ", ps, depth, &(q_s->reboot)))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL shutdown_io_r_init(const char *desc, SHUTDOWN_R_INIT* r_s, prs_struct *ps,
			int depth)
{
	if (r_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_r_init");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_s->status))
		return False;

	return True;
}

/*******************************************************************
reads or writes a REG_Q_SHUTDOWN_EX structure.
********************************************************************/

BOOL shutdown_io_q_init_ex(const char *desc, SHUTDOWN_Q_INIT_EX * q_s, prs_struct *ps,
		       int depth)
{
	if (q_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_q_init_ex");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_s->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	if (!prs_pointer("message", ps, depth, (void**)&q_s->message, sizeof(UNISTR4), (PRS_POINTER_CAST)prs_unistr4))
		return False;

	if (!prs_align(ps))
		return False;

	if (!prs_uint32("timeout", ps, depth, &(q_s->timeout)))
		return False;

	if (!prs_uint8("force  ", ps, depth, &(q_s->force)))
		return False;
	if (!prs_uint8("reboot ", ps, depth, &(q_s->reboot)))
		return False;

	if (!prs_align(ps))
		return False;
	if (!prs_uint32("reason", ps, depth, &(q_s->reason)))
		return False;


	return True;
}

/*******************************************************************
reads or writes a REG_R_SHUTDOWN_EX structure.
********************************************************************/
BOOL shutdown_io_r_init_ex(const char *desc, SHUTDOWN_R_INIT_EX * r_s, prs_struct *ps,
		       int depth)
{
	if (r_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_r_init_ex");
	depth++;

	if(!prs_align(ps))
		return False;

	if(!prs_werror("status", ps, depth, &r_s->status))
		return False;

	return True;
}


/*******************************************************************
Inits a structure.
********************************************************************/
void init_shutdown_q_abort(SHUTDOWN_Q_ABORT *q_s)
{
	q_s->server = TALLOC_P( get_talloc_ctx(), uint16 );
	if (!q_s->server) {
		smb_panic("init_shutdown_q_abort: talloc fail.\n");
		return;
	}
		
	*q_s->server = 0x1;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL shutdown_io_q_abort(const char *desc, SHUTDOWN_Q_ABORT *q_s,
			 prs_struct *ps, int depth)
{
	if (q_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_q_abort");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_pointer("server", ps, depth, (void**)&q_s->server, sizeof(uint16), (PRS_POINTER_CAST)prs_uint16))
		return False;
	if (!prs_align(ps))
		return False;

	return True;
}

/*******************************************************************
reads or writes a structure.
********************************************************************/
BOOL shutdown_io_r_abort(const char *desc, SHUTDOWN_R_ABORT *r_s,
			 prs_struct *ps, int depth)
{
	if (r_s == NULL)
		return False;

	prs_debug(ps, depth, desc, "shutdown_io_r_abort");
	depth++;

	if (!prs_align(ps))
		return False;

	if (!prs_werror("status", ps, depth, &r_s->status))
		return False;

	return True;
}
