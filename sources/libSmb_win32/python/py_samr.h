/* 
   Python wrappers for DCERPC/SMB client routines.

   Copyright (C) Tim Potter, 2002
   
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

#ifndef _PY_SAMR_H
#define _PY_SAMR_H

#include "python/py_common.h"

/* SAMR connect policy handle object */

typedef struct {
	PyObject_HEAD
	struct rpc_pipe_client *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND connect_pol;
} samr_connect_hnd_object;
     
/* SAMR domain policy handle object */

typedef struct {
	PyObject_HEAD
	struct rpc_pipe_client *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND domain_pol;
} samr_domain_hnd_object;

/* SAMR user policy handle object */

typedef struct {
	PyObject_HEAD
	struct rpc_pipe_client *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND user_pol;
} samr_user_hnd_object;

/* SAMR group policy handle object */

typedef struct {
	PyObject_HEAD
	struct cli_state *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND group_pol;
} samr_group_hnd_object;
     
/* SAMR alias policy handle object */

typedef struct {
	PyObject_HEAD
	struct cli_state *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND alias_pol;
} samr_alias_hnd_object;
     
extern PyTypeObject samr_connect_hnd_type, samr_domain_hnd_type,
	samr_user_hnd_type, samr_group_hnd_type, samr_alias_hnd_type; 

/* Exceptions raised by this module */

extern PyObject *samr_error;

/* The following definitions are from py_samr_conv.c */

BOOL py_from_acct_info(PyObject **array, struct acct_info *info, int num_accts);
BOOL py_from_SAM_USER_INFO_16(PyObject **dict, SAM_USER_INFO_16 *info);
BOOL py_to_SAM_USER_INFO_16(SAM_USER_INFO_16 *info, PyObject *dict);
BOOL py_from_SAM_USER_INFO_21(PyObject **dict, SAM_USER_INFO_21 *info);
BOOL py_to_SAM_USER_INFO_21(SAM_USER_INFO_21 *info, PyObject *dict);

#endif /* _PY_SAMR_H */
