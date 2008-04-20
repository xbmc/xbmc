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

#ifndef _PY_CONV_H
#define _PY_CONV_H

#include "python/py_common.h"

enum pyconv_types { PY_UNISTR, PY_UNISTR2, PY_UINT32, PY_UINT16, PY_STRING, 
		    PY_UID, PY_GID };

struct pyconv {
	char *name;		/* Name of member */
	enum pyconv_types type; /* Type */
	size_t offset;		/* Offset into structure */
};

PyObject *from_struct(void *s, struct pyconv *conv);
BOOL to_struct(void *s, PyObject *dict, struct pyconv *conv);
PyObject *from_unistr_list(uint16 *dependentfiles);

/* Another version of offsetof (-: */

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif /* _PY_CONV_H */
