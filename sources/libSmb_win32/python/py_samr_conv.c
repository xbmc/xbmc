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

#include "python/py_samr.h"
#include "python/py_conv.h"

/*
 * Convert between SAM_USER_INFO_16 and Python
 */

struct pyconv py_SAM_USER_INFO_16[] = {
	{ "acb_info", PY_UINT32, offsetof(SAM_USER_INFO_16, acb_info) },
	{ NULL }
};

BOOL py_from_SAM_USER_INFO_16(PyObject **dict, SAM_USER_INFO_16 *info)
{
	*dict = from_struct(info, py_SAM_USER_INFO_16);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(16));
	return True;
}

BOOL py_to_SAM_USER_INFO_16(SAM_USER_INFO_16 *info, PyObject *dict)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	if (!(obj = PyDict_GetItemString(dict_copy, "level")) ||
	    !PyInt_Check(obj))
		goto done;

	PyDict_DelItemString(dict_copy, "level");

	if (!to_struct(info, dict_copy, py_SAM_USER_INFO_16))
		goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

/*
 * Convert between SAM_USER_INFO_21 and Python
 */

struct pyconv py_SAM_USER_INFO_21[] = {
	{ NULL }
};

BOOL py_from_SAM_USER_INFO_21(PyObject **dict, SAM_USER_INFO_21 *info)
{
	*dict = from_struct(info, py_SAM_USER_INFO_21);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(21));
	return True;
}

BOOL py_to_SAM_USER_INFO_21(SAM_USER_INFO_21 *info, PyObject *dict)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	if (!(obj = PyDict_GetItemString(dict_copy, "level")) ||
	    !PyInt_Check(obj))
		goto done;

	PyDict_DelItemString(dict_copy, "level");

	if (!to_struct(info, dict_copy, py_SAM_USER_INFO_21))
		goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

/*
 * Convert between acct_info and Python 
 */

BOOL py_from_acct_info(PyObject **array, struct acct_info *info, int num_accts)
{
	int i;

	*array = PyList_New(num_accts);

	for (i = 0; i < num_accts; i++) {
		PyObject *obj;	

		obj = PyDict_New();
		
		PyDict_SetItemString(
			obj, "name", PyString_FromString(info[i].acct_name));

		PyDict_SetItemString(
			obj, "description", 
			PyString_FromString(info[i].acct_desc));

		PyDict_SetItemString(obj, "rid", PyInt_FromLong(info[i].rid));

		PyList_SetItem(*array, i, obj);
	}

	return True;
}

BOOL py_to_acct_info(PRINTER_INFO_3 *info, PyObject *dict,
		     TALLOC_CTX *mem_ctx)
{
	return False;
}
