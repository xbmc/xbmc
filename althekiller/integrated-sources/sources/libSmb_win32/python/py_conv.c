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

#include "py_conv.h"

/* Helper for rpcstr_pull() function */

static void fstr_pull(fstring str, UNISTR *uni)
{
	rpcstr_pull(str, uni->buffer, sizeof(fstring), -1, STR_TERMINATE);
}

static void fstr_pull2(fstring str, UNISTR2 *uni)
{
	rpcstr_pull(str, uni->buffer, sizeof(fstring), -1, STR_TERMINATE);
}

/* Convert a structure to a Python dict */

PyObject *from_struct(void *s, struct pyconv *conv)
{
	PyObject *obj, *item;
	int i;

	obj = PyDict_New();

	for (i = 0; conv[i].name; i++) {
		switch (conv[i].type) {
		case PY_UNISTR: {
			UNISTR *u = (UNISTR *)((char *)s + conv[i].offset);
			fstring str = "";

			if (u->buffer)
				fstr_pull(str, u);

			item = PyString_FromString(str);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		case PY_UNISTR2: {
			UNISTR2 *u = (UNISTR2 *)((char *)s + conv[i].offset);
			fstring str = "";

			if (u->buffer)
				fstr_pull2(str, u);

			item = PyString_FromString(str);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		case PY_UINT32: {
			uint32 *u = (uint32 *)((char *)s + conv[i].offset);

			item = PyInt_FromLong(*u);
			PyDict_SetItemString(obj, conv[i].name, item);
			
			break;
		}
		case PY_UINT16: {
			uint16 *u = (uint16 *)((char *)s + conv[i].offset);

			item = PyInt_FromLong(*u);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		case PY_STRING: {
			char *str = (char *)s + conv[i].offset;

			item = PyString_FromString(str);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		case PY_UID: {
			uid_t *uid = (uid_t *)((char *)s + conv[i].offset);

			item = PyInt_FromLong(*uid);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		case PY_GID: {
			gid_t *gid = (gid_t *)((char *)s + conv[i].offset);

			item = PyInt_FromLong(*gid);
			PyDict_SetItemString(obj, conv[i].name, item);

			break;
		}
		default:
			
			break;
		}
	}

	return obj;
}

/* Convert a Python dict to a structure */

BOOL to_struct(void *s, PyObject *dict, struct pyconv *conv)
{
	PyObject *visited, *key, *value;
	BOOL result = False;
	int i;

	visited = PyDict_New();

	for (i = 0; conv[i].name; i++) {
		PyObject *obj;
		
		obj = PyDict_GetItemString(dict, conv[i].name);

		if (!obj)
			goto done;
		
		switch (conv[i].type) {
		case PY_UNISTR: {
			UNISTR *u = (UNISTR *)((char *)s + conv[i].offset);
			char *str = "";

			if (!PyString_Check(obj))
				goto done;

			str = PyString_AsString(obj);
			init_unistr(u, str);
			
			break;
		}
		case PY_UINT32: {
			uint32 *u = (uint32 *)((char *)s + conv[i].offset);

			if (!PyInt_Check(obj))
				goto done;

			*u = PyInt_AsLong(obj);

			break;
		}
		case PY_UINT16: {
			uint16 *u = (uint16 *)((char *)s + conv[i].offset);

			if (!PyInt_Check(obj)) 
				goto done;

			*u = PyInt_AsLong(obj);
			break;
		}
		default:
			break;
		}

		/* Mark as visited */

		PyDict_SetItemString(visited, conv[i].name, 
				     PyInt_FromLong(1));
	}

	/* Iterate over each item in the input dictionary and see if it was
	   visited.  If it wasn't then the user has added some extra crap
	   to the dictionary. */

	i = 0;

	while (PyDict_Next(dict, &i, &key, &value)) {
		if (!PyDict_GetItem(visited, key))
			goto done;
	}

	result = True;

done:
	/* We must decrement the reference count here or the visited
	   dictionary will not be freed. */
	       
	Py_DECREF(visited);

	return result;
}

/* Convert a NULL terminated list of NULL terminated unicode strings
   to a list of (char *) strings */

PyObject *from_unistr_list(uint16 *dependentfiles)
{
	PyObject *list;
	int offset = 0;

	list = PyList_New(0);

	while (*(dependentfiles + offset) != 0) {
		fstring name;
		int len;

		len = rpcstr_pull(name, dependentfiles + offset,
				  sizeof(fstring), -1, STR_TERMINATE);

		offset += len / 2;
		PyList_Append(list, PyString_FromString(name));
	}

	return list;
}
