/* 
   Python wrappers for DCERPC/SMB client routines.

   Copyright (C) Tim Potter, 2003
   
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

#ifndef _PY_SRVSVC_H
#define _PY_SRVSVC_H

#include "python/py_common.h"

/* The following definitions come from python/py_srvsv.c */

BOOL py_from_SRV_INFO_101(PyObject **dict, SRV_INFO_101 *info);

#endif /* _PY_SRVSVC_H */
