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

#include "python/py_spoolss.h"
#include "python/py_conv.h"

struct pyconv py_PORT_INFO_1[] = {
	{ "name", PY_UNISTR, offsetof(PORT_INFO_1, port_name) },
	{ NULL }
};

struct pyconv py_PORT_INFO_2[] = {
	{ "name", PY_UNISTR, offsetof(PORT_INFO_2, port_name) },
	{ "monitor_name", PY_UNISTR, offsetof(PORT_INFO_2, monitor_name) },
	{ "description", PY_UNISTR, offsetof(PORT_INFO_2, description) },
	{ "reserved", PY_UINT32, offsetof(PORT_INFO_2, reserved) },
	{ "type", PY_UINT32, offsetof(PORT_INFO_2, port_type) },
	{ NULL }
};	

BOOL py_from_PORT_INFO_1(PyObject **dict, PORT_INFO_1 *info)
{
	*dict = from_struct(info, py_PORT_INFO_1);
	return True;
}

BOOL py_to_PORT_INFO_1(PORT_INFO_1 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_PORT_INFO_2(PyObject **dict, PORT_INFO_2 *info)
{
	*dict = from_struct(info, py_PORT_INFO_2);
	return True;
}

BOOL py_to_PORT_INFO_2(PORT_INFO_2 *info, PyObject *dict)
{
	return False;
}
