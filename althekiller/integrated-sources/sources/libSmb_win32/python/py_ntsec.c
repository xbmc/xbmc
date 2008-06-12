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

#include "python/py_common.h"

/* Convert a SID to a Python dict */

BOOL py_from_SID(PyObject **obj, DOM_SID *sid)
{
	fstring sidstr;

	if (!sid) {
		Py_INCREF(Py_None);
		*obj = Py_None;
		return True;
	}

	if (!sid_to_string(sidstr, sid))
		return False;

	*obj = PyString_FromString(sidstr);

	return True;
}

BOOL py_to_SID(DOM_SID *sid, PyObject *obj)
{
	if (!PyString_Check(obj))
		return False;

	return string_to_sid(sid, PyString_AsString(obj));
}

BOOL py_from_ACE(PyObject **dict, SEC_ACE *ace)
{
	PyObject *obj;

	if (!ace) {
		Py_INCREF(Py_None);
		*dict = Py_None;
		return True;
	}

	*dict = Py_BuildValue("{sisisi}", "type", ace->type,
	    			"flags", ace->flags,
				"mask", ace->info.mask);

	if (py_from_SID(&obj, &ace->trustee)) {
		PyDict_SetItemString(*dict, "trustee", obj);
		Py_DECREF(obj);
	}

	return True;
}

BOOL py_to_ACE(SEC_ACE *ace, PyObject *dict)
{
	PyObject *obj;
	uint8 ace_type, ace_flags;
	DOM_SID trustee;
	SEC_ACCESS sec_access;

	if (!PyDict_Check(dict))
		return False;

	if (!(obj = PyDict_GetItemString(dict, "type")) ||
	    !PyInt_Check(obj))
		return False;

	ace_type = PyInt_AsLong(obj);

	if (!(obj = PyDict_GetItemString(dict, "flags")) ||
	    !PyInt_Check(obj))
		return False;

	ace_flags = PyInt_AsLong(obj);

	if (!(obj = PyDict_GetItemString(dict, "trustee")) ||
	    !PyString_Check(obj))
		return False;

	if (!py_to_SID(&trustee, obj))
		return False;

	if (!(obj = PyDict_GetItemString(dict, "mask")) ||
	    !PyInt_Check(obj))
		return False;

	sec_access.mask = PyInt_AsLong(obj);

	init_sec_ace(ace, &trustee, ace_type, sec_access, ace_flags);

	/* Fill in size field */

	ace->size = SEC_ACE_HEADER_SIZE + sid_size(&trustee);

	return True;
}

BOOL py_from_ACL(PyObject **dict, SEC_ACL *acl)
{
	PyObject *ace_list;
	int i;

	if (!acl) {
		Py_INCREF(Py_None);
		*dict = Py_None;
		return True;
	}

	ace_list = PyList_New(acl->num_aces);

	for (i = 0; i < acl->num_aces; i++) {
		PyObject *obj;

		if (py_from_ACE(&obj, &acl->ace[i]))
			PyList_SetItem(ace_list, i, obj);
	}

	*dict = Py_BuildValue("{sisN}", "revision", acl->revision,
			"ace_list", ace_list);

	return True;
}

BOOL py_to_ACL(SEC_ACL *acl, PyObject *dict, TALLOC_CTX *mem_ctx)
{
	PyObject *obj;
	uint32 i;

	if (!(obj = PyDict_GetItemString(dict, "revision")) ||
	    !PyInt_Check(obj))
		return False;

	acl->revision = PyInt_AsLong(obj);

	if (!(obj = PyDict_GetItemString(dict, "ace_list")) ||
	    !PyList_Check(obj)) 
		return False;
	
	acl->num_aces = PyList_Size(obj);

	acl->ace = _talloc(mem_ctx, acl->num_aces * sizeof(SEC_ACE));
	acl->size = SEC_ACL_HEADER_SIZE;

	for (i = 0; i < acl->num_aces; i++) {
		PyObject *py_ace = PyList_GetItem(obj, i);

		if (!py_to_ACE(&acl->ace[i], py_ace))
			return False;

		acl->size += acl->ace[i].size;
	}

	return True;
}

BOOL py_from_SECDESC(PyObject **dict, SEC_DESC *sd)
{
	PyObject *obj;

	*dict = PyDict_New();

	obj = PyInt_FromLong(sd->revision);
	PyDict_SetItemString(*dict, "revision", obj);
	Py_DECREF(obj);

	obj = PyInt_FromLong(sd->type);
	PyDict_SetItemString(*dict, "type", obj);
	Py_DECREF(obj);

	if (py_from_SID(&obj, sd->owner_sid)) {
		PyDict_SetItemString(*dict, "owner_sid", obj);
		Py_DECREF(obj);
	}

	if (py_from_SID(&obj, sd->grp_sid)) {
		PyDict_SetItemString(*dict, "group_sid", obj);
		Py_DECREF(obj);
	}

	if (py_from_ACL(&obj, sd->dacl)) {
		PyDict_SetItemString(*dict, "dacl", obj);
		Py_DECREF(obj);
	}

	if (py_from_ACL(&obj, sd->sacl)) {
		PyDict_SetItemString(*dict, "sacl", obj);
		Py_DECREF(obj);
	}

	return True;
}

BOOL py_to_SECDESC(SEC_DESC **sd, PyObject *dict, TALLOC_CTX *mem_ctx)
{
	PyObject *obj;
	uint16 revision;
	uint16 type = SEC_DESC_SELF_RELATIVE;
	DOM_SID owner_sid, group_sid;
	SEC_ACL sacl, dacl;
	BOOL got_dacl = False, got_sacl = False;
	BOOL got_owner_sid = False, got_group_sid = False;

	ZERO_STRUCT(dacl); ZERO_STRUCT(sacl);
	ZERO_STRUCT(owner_sid); ZERO_STRUCT(group_sid);

	if (!(obj = PyDict_GetItemString(dict, "revision")))
		return False;

	revision = PyInt_AsLong(obj);

	if ((obj = PyDict_GetItemString(dict, "type"))) {
		if (obj != Py_None) {
			type = PyInt_AsLong(obj);
		}
	}

	if ((obj = PyDict_GetItemString(dict, "owner_sid"))) {

		if (obj != Py_None) {

			if (!py_to_SID(&owner_sid, obj))
				return False;

			got_owner_sid = True;
		}
	}

	if ((obj = PyDict_GetItemString(dict, "group_sid"))) {

		if (obj != Py_None) {

			if (!py_to_SID(&group_sid, obj))
				return False;
			
			got_group_sid = True;
		}
	}

	if ((obj = PyDict_GetItemString(dict, "dacl"))) {

		if (obj != Py_None) {

			if (!py_to_ACL(&dacl, obj, mem_ctx))
				return False;
			
			got_dacl = True;
		}
	}

	if ((obj = PyDict_GetItemString(dict, "sacl"))) {

		if (obj != Py_None) {

			if (!py_to_ACL(&sacl, obj, mem_ctx))
				return False;

			got_sacl = True;
		}
	}

#if 0				/* For new secdesc code */
	*sd = make_sec_desc(mem_ctx, revision, 
			    got_owner_sid ? &owner_sid : NULL, 
			    got_group_sid ? &group_sid : NULL,
			    got_sacl ? &sacl : NULL, 
			    got_dacl ? &dacl : NULL);
#else
	{
		size_t sd_size;

		*sd = make_sec_desc(mem_ctx, revision, type,
			    got_owner_sid ? &owner_sid : NULL, 
			    got_group_sid ? &group_sid : NULL,
			    got_sacl ? &sacl : NULL, 
			    got_dacl ? &dacl : NULL, &sd_size);
	}
#endif

	return True;
}
