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

#include "python/py_winreg.h"

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {
	
	/* Registry value types */

	{ "REG_NONE", REG_NONE },
	{ "REG_SZ", REG_SZ },
	{ "REG_EXPAND_SZ", REG_EXPAND_SZ },
	{ "REG_BINARY", REG_BINARY },
	{ "REG_DWORD", REG_DWORD },
	{ "REG_DWORD_LE", REG_DWORD_LE },
	{ "REG_DWORD_BE", REG_DWORD_BE },
	{ "REG_LINK", REG_LINK },
	{ "REG_MULTI_SZ", REG_MULTI_SZ },
	{ "REG_RESOURCE_LIST", REG_RESOURCE_LIST },
	{ "REG_FULL_RESOURCE_DESCRIPTOR", REG_FULL_RESOURCE_DESCRIPTOR },
	{ "REG_RESOURCE_REQUIREMENTS_LIST", REG_RESOURCE_REQUIREMENTS_LIST },

	{ NULL },
};

static void const_init(PyObject *dict)
{
	struct const_vals *tmp;
	PyObject *obj;

	for (tmp = module_const_vals; tmp->name; tmp++) {
		obj = PyInt_FromLong(tmp->value);
		PyDict_SetItemString(dict, tmp->name, obj);
		Py_DECREF(obj);
	}
}

/*
 * Module initialisation 
 */

static PyMethodDef winreg_methods[] = {
	{ NULL }
};

void initwinreg(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("winreg", winreg_methods);
	dict = PyModule_GetDict(module);

	/* Initialise constants */

	const_init(dict);

	/* Do samba initialisation */

	py_samba_init();
}
