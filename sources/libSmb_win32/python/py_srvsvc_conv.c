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

#include "python/py_srvsvc.h"
#include "python/py_conv.h"

static struct pyconv py_SRV_INFO_101[] = {
	{ "platform_id", PY_UINT32, offsetof(SRV_INFO_101, platform_id) },
	{ "major_version", PY_UINT32, offsetof(SRV_INFO_101, ver_major) },
	{ "minor_version", PY_UINT32, offsetof(SRV_INFO_101, ver_minor) },
	{ "server_type", PY_UINT32, offsetof(SRV_INFO_101, srv_type) },
	{ "name", PY_UNISTR2, offsetof(SRV_INFO_101, uni_name) },
	{ "comment", PY_UNISTR2, offsetof(SRV_INFO_101, uni_comment) },
	{ NULL }
};	

BOOL py_from_SRV_INFO_101(PyObject **dict, SRV_INFO_101 *info)
{
	*dict = from_struct(info, py_SRV_INFO_101);

	PyDict_SetItemString(*dict, "level", PyInt_FromLong(101));

	return True;
}
