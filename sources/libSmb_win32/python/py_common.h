/* 
   Python wrappers for DCERPC/SMB client routines.

   Copyright (C) Tim Potter, 2002-2003
   
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

#ifndef _PY_COMMON_H
#define _PY_COMMON_H

#include "includes.h"

/* This symbol is used in both includes.h and Python.h which causes an
   annoying compiler warning. */

#ifdef HAVE_FSTAT
#undef HAVE_FSTAT
#endif

#include "Python.h"

/* The following definitions come from python/py_common.c  */

PyObject *py_werror_tuple(WERROR werror);
PyObject *py_ntstatus_tuple(NTSTATUS ntstatus);
void py_samba_init(void);
PyObject *get_debuglevel(PyObject *self, PyObject *args);
PyObject *set_debuglevel(PyObject *self, PyObject *args);
PyObject *py_setup_logging(PyObject *self, PyObject *args, PyObject *kw);
BOOL py_parse_creds(PyObject *creds, char **username, char **domain, 
		    char **password, char **errstr);
struct cli_state *open_pipe_creds(char *server, PyObject *creds, 
				  int pipe_idx, char **errstr);
BOOL get_level_value(PyObject *dict, uint32 *level);

/* The following definitions come from python/py_ntsec.c  */

BOOL py_from_SID(PyObject **obj, DOM_SID *sid);
BOOL py_to_SID(DOM_SID *sid, PyObject *obj);
BOOL py_from_ACE(PyObject **dict, SEC_ACE *ace);
BOOL py_to_ACE(SEC_ACE *ace, PyObject *dict);
BOOL py_from_ACL(PyObject **dict, SEC_ACL *acl);
BOOL py_to_ACL(SEC_ACL *acl, PyObject *dict, TALLOC_CTX *mem_ctx);
BOOL py_from_SECDESC(PyObject **dict, SEC_DESC *sd);
BOOL py_to_SECDESC(SEC_DESC **sd, PyObject *dict, TALLOC_CTX *mem_ctx);

#endif /*  _PY_COMMON_H  */
