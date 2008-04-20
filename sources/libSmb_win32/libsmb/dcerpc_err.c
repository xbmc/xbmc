/* 
 *  Unix SMB/CIFS implementation.
 *  Copyright (C) Stefan Metzmacher 2004
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

struct dcerpc_fault_table {
	const char *errstr;
	uint32_t faultcode;
};

static const struct dcerpc_fault_table dcerpc_faults[] =
{
	{ "DCERPC_FAULT_OP_RNG_ERROR",		DCERPC_FAULT_OP_RNG_ERROR },
	{ "DCERPC_FAULT_UNK_IF",		DCERPC_FAULT_UNK_IF },
	{ "DCERPC_FAULT_NDR",			DCERPC_FAULT_NDR },
	{ "DCERPC_FAULT_INVALID_TAG",		DCERPC_FAULT_INVALID_TAG },
	{ "DCERPC_FAULT_CONTEXT_MISMATCH",	DCERPC_FAULT_CONTEXT_MISMATCH },
	{ "DCERPC_FAULT_OTHER",			DCERPC_FAULT_OTHER },
	{ "DCERPC_FAULT_ACCESS_DENIED",		DCERPC_FAULT_ACCESS_DENIED },

	{ NULL,					0}	
};

const char *dcerpc_errstr(uint32 fault_code)
{
	static pstring msg;
	int idx = 0;

	slprintf(msg, sizeof(msg), "DCERPC fault 0x%08x", fault_code);

	while (dcerpc_faults[idx].errstr != NULL) {
		if (dcerpc_faults[idx].faultcode == fault_code) {
			return dcerpc_faults[idx].errstr;
		}
		idx++;
	}

	return msg;
}
