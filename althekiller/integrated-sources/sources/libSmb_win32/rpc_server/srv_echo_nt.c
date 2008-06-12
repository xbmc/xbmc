/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines for rpcecho
 *  Copyright (C) Tim Potter                   2003.
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

/* This is the interface to the rpcecho pipe. */

#include "includes.h"
#include "nterr.h"

#ifdef DEVELOPER

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

/* Add one to the input and return it */

void _echo_add_one(pipes_struct *p, ECHO_Q_ADD_ONE *q_u, ECHO_R_ADD_ONE *r_u)
{
	DEBUG(10, ("_echo_add_one\n"));

	r_u->response = q_u->request + 1;
}

/* Echo back an array of data */

void _echo_data(pipes_struct *p, ECHO_Q_ECHO_DATA *q_u, 
		ECHO_R_ECHO_DATA *r_u)
{
	DEBUG(10, ("_echo_data\n"));

	r_u->data = TALLOC(p->mem_ctx, q_u->size);
	r_u->size = q_u->size;
	memcpy(r_u->data, q_u->data, q_u->size);
}

/* Sink an array of data */

void _sink_data(pipes_struct *p, ECHO_Q_SINK_DATA *q_u, 
		ECHO_R_SINK_DATA *r_u)
{
	DEBUG(10, ("_sink_data\n"));

	/* My that was some yummy data! */
}

/* Source an array of data */

void _source_data(pipes_struct *p, ECHO_Q_SOURCE_DATA *q_u, 
		  ECHO_R_SOURCE_DATA *r_u)
{
	uint32 i;

	DEBUG(10, ("_source_data\n"));

	r_u->data = TALLOC(p->mem_ctx, q_u->size);
	r_u->size = q_u->size;

	for (i = 0; i < r_u->size; i++)
		r_u->data[i] = i & 0xff;
}

#endif /* DEVELOPER */
