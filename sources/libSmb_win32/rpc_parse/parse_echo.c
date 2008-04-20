/* 
 *  Unix SMB/CIFS implementation.
 *
 *  RPC Pipe client / server routines
 *
 *  Copyright (C) Tim Potter 2003
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

void init_echo_q_add_one(ECHO_Q_ADD_ONE *q_d, uint32 request)
{
	q_d->request = request;
}

BOOL echo_io_q_add_one(const char *desc, ECHO_Q_ADD_ONE *q_d,
		       prs_struct *ps, int depth)
{
	if (!prs_uint32("request", ps, 0, &q_d->request))
		return False;

	return True;
}

BOOL echo_io_r_add_one(const char *desc, ECHO_R_ADD_ONE *q_d,
		       prs_struct *ps, int depth)
{
	if (!prs_uint32("response", ps, 0, &q_d->response))
		return False;

	return True;
}


void init_echo_q_echo_data(ECHO_Q_ECHO_DATA *q_d, uint32 size, char *data)
{
	q_d->size = size;
	q_d->data = data;
}

BOOL echo_io_q_echo_data(const char *desc, ECHO_Q_ECHO_DATA *q_d,
			  prs_struct *ps, int depth)
{
	if (!prs_uint32("size", ps, depth, &q_d->size))
		return False;

	if (!prs_uint32("size", ps, depth, &q_d->size))
		return False;

	if (UNMARSHALLING(ps)) {
		q_d->data = PRS_ALLOC_MEM(ps, char, q_d->size);

		if (!q_d->data)
			return False;
	}

	if (!prs_uint8s(False, "data", ps, depth, (unsigned char *)q_d->data, q_d->size))
		return False;

	return True;
}

BOOL echo_io_r_echo_data(const char *desc, ECHO_R_ECHO_DATA *q_d,
			  prs_struct *ps, int depth)
{
	if (!prs_uint32("size", ps, 0, &q_d->size))
		return False;

	if (UNMARSHALLING(ps)) {
		q_d->data = PRS_ALLOC_MEM(ps, char, q_d->size);

		if (!q_d->data)
			return False;
	}

	if (!prs_uint8s(False, "data", ps, depth, (unsigned char *)q_d->data, q_d->size))
		return False;

	return True;
}

void init_echo_q_sink_data(ECHO_Q_SINK_DATA *q_d, uint32 size, char *data)
{
	q_d->size = size;
	q_d->data = data;
}

BOOL echo_io_q_sink_data(const char *desc, ECHO_Q_SINK_DATA *q_d,
			 prs_struct *ps, int depth)
{
	if (!prs_uint32("size", ps, depth, &q_d->size))
		return False;

	if (!prs_uint32("size", ps, depth, &q_d->size))
		return False;

	if (UNMARSHALLING(ps)) {
		q_d->data = PRS_ALLOC_MEM(ps, char, q_d->size);

		if (!q_d->data)
			return False;
	}

	if (!prs_uint8s(False, "data", ps, depth, (unsigned char *)q_d->data, q_d->size))
		return False;

	return True;
}

BOOL echo_io_r_sink_data(const char *desc, ECHO_R_SINK_DATA *q_d,
			 prs_struct *ps, int depth)
{
	return True;
}

void init_echo_q_source_data(ECHO_Q_SOURCE_DATA *q_d, uint32 size)
{
	q_d->size = size;
}

BOOL echo_io_q_source_data(const char *desc, ECHO_Q_SOURCE_DATA *q_d,
			 prs_struct *ps, int depth)
{
	if (!prs_uint32("size", ps, depth, &q_d->size))
		return False;

	return True;
}

BOOL echo_io_r_source_data(const char *desc, ECHO_R_SOURCE_DATA *q_d,
			   prs_struct *ps, int depth)
{
	if (!prs_uint32("size", ps, 0, &q_d->size))
		return False;

	if (UNMARSHALLING(ps)) {
		q_d->data = PRS_ALLOC_MEM(ps, char, q_d->size);

		if (!q_d->data)
			return False;
	}

	if (!prs_uint8s(False, "data", ps, depth, (unsigned char *)q_d->data, q_d->size))
		return False;

	return True;
}
