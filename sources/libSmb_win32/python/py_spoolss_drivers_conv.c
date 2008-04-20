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

/* Structure/hash conversions */

struct pyconv py_DRIVER_INFO_1[] = {
	{ "name", PY_UNISTR, offsetof(DRIVER_INFO_1, name) },
	{ NULL }
};

struct pyconv py_DRIVER_INFO_2[] = {
	{ "version", PY_UINT32, offsetof(DRIVER_INFO_2, version) },
	{ "name", PY_UNISTR, offsetof(DRIVER_INFO_2, name) },
	{ "architecture", PY_UNISTR, offsetof(DRIVER_INFO_2, architecture) },
	{ "driver_path", PY_UNISTR, offsetof(DRIVER_INFO_2, driverpath) },
	{ "data_file", PY_UNISTR, offsetof(DRIVER_INFO_2, datafile) },
	{ "config_file", PY_UNISTR, offsetof(DRIVER_INFO_2, configfile) },
	{ NULL }
};

struct pyconv py_DRIVER_INFO_3[] = {
	{ "version", PY_UINT32, offsetof(DRIVER_INFO_3, version) },
	{ "name", PY_UNISTR, offsetof(DRIVER_INFO_3, name) },
	{ "architecture", PY_UNISTR, offsetof(DRIVER_INFO_3, architecture) },
	{ "driver_path", PY_UNISTR, offsetof(DRIVER_INFO_3, driverpath) },
	{ "data_file", PY_UNISTR, offsetof(DRIVER_INFO_3, datafile) },
	{ "config_file", PY_UNISTR, offsetof(DRIVER_INFO_3, configfile) },
	{ "help_file", PY_UNISTR, offsetof(DRIVER_INFO_3, helpfile) },
	{ "monitor_name", PY_UNISTR, offsetof(DRIVER_INFO_3, monitorname) },
	{ "default_datatype", PY_UNISTR, offsetof(DRIVER_INFO_3, defaultdatatype) },
	{ NULL }
};

struct pyconv py_DRIVER_INFO_6[] = {
	{ "version", PY_UINT32, offsetof(DRIVER_INFO_6, version) },
	{ "name", PY_UNISTR, offsetof(DRIVER_INFO_6, name) },
	{ "architecture", PY_UNISTR, offsetof(DRIVER_INFO_6, architecture) },
	{ "driver_path", PY_UNISTR, offsetof(DRIVER_INFO_6, driverpath) },
	{ "data_file", PY_UNISTR, offsetof(DRIVER_INFO_6, datafile) },
	{ "config_file", PY_UNISTR, offsetof(DRIVER_INFO_6, configfile) },
	{ "help_file", PY_UNISTR, offsetof(DRIVER_INFO_6, helpfile) },
	{ "monitor_name", PY_UNISTR, offsetof(DRIVER_INFO_6, monitorname) },
	{ "default_datatype", PY_UNISTR, offsetof(DRIVER_INFO_6, defaultdatatype) },
	/* driver_date */
	{ "padding", PY_UINT32, offsetof(DRIVER_INFO_6, padding) },
	{ "driver_version_low", PY_UINT32, offsetof(DRIVER_INFO_6, driver_version_low) },
	{ "driver_version_high", PY_UINT32, offsetof(DRIVER_INFO_6, driver_version_high) },
	{ "mfg_name", PY_UNISTR, offsetof(DRIVER_INFO_6, mfgname) },
	{ "oem_url", PY_UNISTR, offsetof(DRIVER_INFO_6, oem_url) },
	{ "hardware_id", PY_UNISTR, offsetof(DRIVER_INFO_6, hardware_id) },
	{ "provider", PY_UNISTR, offsetof(DRIVER_INFO_6, provider) },
	
	{ NULL }
};

struct pyconv py_DRIVER_DIRECTORY_1[] = {
	{ "name", PY_UNISTR, offsetof(DRIVER_DIRECTORY_1, name) },
	{ NULL }
};

static uint16 *to_dependentfiles(PyObject *list, TALLOC_CTX *mem_ctx)
{
	uint32 elements, size=0, pos=0, i;
	char *str;
	uint16 *ret = NULL;
	PyObject *borrowedRef;

	if (!PyList_Check(list)) {
		goto done;
	}

	/* calculate size for dependentfiles */
	elements=PyList_Size(list);
	for (i = 0; i < elements; i++) {
		borrowedRef=PyList_GetItem(list, i);
		if (!PyString_Check(borrowedRef)) 
			/* non string found, return error */
			goto done;
		size+=PyString_Size(borrowedRef)+1;
	}

	if (!(ret = (uint16*)_talloc(mem_ctx,((size+1)*sizeof(uint16)))))
		goto done;

	/* create null terminated sequence of null terminated strings */
	for (i = 0; i < elements; i++) {
		borrowedRef=PyList_GetItem(list, i);
		str=PyString_AsString(borrowedRef);
		do {
			if (pos >= size) {
				/* dependentfiles too small.  miscalculated? */
				ret = NULL;
				goto done;
			}
			SSVAL(&ret[pos], 0, str[0]);
			pos++;
		} while (*(str++));
	}
	/* final null */
	ret[pos]='\0';

done:
	return ret;	
}

BOOL py_from_DRIVER_INFO_1(PyObject **dict, DRIVER_INFO_1 *info)
{
	*dict = from_struct(info, py_DRIVER_INFO_1);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(1));

	return True;
}

BOOL py_to_DRIVER_INFO_1(DRIVER_INFO_1 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_DRIVER_INFO_2(PyObject **dict, DRIVER_INFO_2 *info)
{
	*dict = from_struct(info, py_DRIVER_INFO_2);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(2));

	return True;
}

BOOL py_to_DRIVER_INFO_2(DRIVER_INFO_2 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_DRIVER_INFO_3(PyObject **dict, DRIVER_INFO_3 *info)
{
	*dict = from_struct(info, py_DRIVER_INFO_3);

	PyDict_SetItemString(*dict, "level", PyInt_FromLong(3));

	PyDict_SetItemString(
		*dict, "dependent_files", 
		from_unistr_list(info->dependentfiles));

	return True;
}

BOOL py_to_DRIVER_INFO_3(DRIVER_INFO_3 *info, PyObject *dict,
			 TALLOC_CTX *mem_ctx)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	if (!(obj = PyDict_GetItemString(dict_copy, "dependent_files")))
		goto done;

	if (!(info->dependentfiles = to_dependentfiles(obj, mem_ctx)))
		goto done;

	PyDict_DelItemString(dict_copy, "dependent_files");

	if (!(obj = PyDict_GetItemString(dict_copy, "level")) ||
	    !PyInt_Check(obj))
		goto done;

	PyDict_DelItemString(dict_copy, "level");

	if (!to_struct(info, dict_copy, py_DRIVER_INFO_3))
	    goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

BOOL py_from_DRIVER_INFO_6(PyObject **dict, DRIVER_INFO_6 *info)
{
	*dict = from_struct(info, py_DRIVER_INFO_6);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(6));
	PyDict_SetItemString(
		*dict, "dependent_files", 
		from_unistr_list(info->dependentfiles));
	return True;
}

BOOL py_to_DRIVER_INFO_6(DRIVER_INFO_6 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_DRIVER_DIRECTORY_1(PyObject **dict, DRIVER_DIRECTORY_1 *info)
{
	*dict = from_struct(info, py_DRIVER_DIRECTORY_1);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(1));
	return True;
}

BOOL py_to_DRIVER_DIRECTORY_1(DRIVER_DIRECTORY_1 *info, PyObject *dict)
{
	return False;
}
