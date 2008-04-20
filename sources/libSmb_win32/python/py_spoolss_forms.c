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

/* Add a form */

PyObject *spoolss_hnd_addform(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *info;
	FORM form;
	int level;
	static char *kwlist[] = {"form", NULL};

	/* Parse parameters */
	
	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &info))
		return NULL;
	
	/* Call rpc function */
	
	if (!py_to_FORM(&form, info)) {
		PyErr_SetString(spoolss_error, "invalid form");
		return NULL;
	}

	if (!get_level_value(info, &level)) {
		PyErr_SetString(spoolss_error, "invalid info level");
		return NULL;
	}

	if (level != 1) {
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}
	
	switch (level) {
	case 1: {
		PyObject *obj = PyDict_GetItemString(info, "name");
		char *form_name = PyString_AsString(obj);

		init_unistr2(&form.name, form_name, UNI_STR_TERMINATE);
		break;
	}
	default:
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}
		
	werror = rpccli_spoolss_addform(hnd->cli, hnd->mem_ctx, &hnd->pol,
					level, &form);


	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Get form properties */

PyObject *spoolss_hnd_getform(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *result;
	char *form_name;
	int level = 1;
	static char *kwlist[] = {"form_name", "level", NULL};
	FORM_1 form;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|i", kwlist, &form_name, &level))
		return NULL;
	
	/* Call rpc function */

	werror = rpccli_spoolss_getform(
		hnd->cli, hnd->mem_ctx, &hnd->pol, form_name, level, &form);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	result = Py_None;

	switch(level) {
	case 1:
		py_from_FORM_1(&result, &form);
		break;
	}

	Py_INCREF(result);
	return result;
}

/* Set form properties */

PyObject *spoolss_hnd_setform(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *info, *form_name;
	int level;
	static char *kwlist[] = { "form", NULL};
	FORM form;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &info))
		return NULL;

	if (!get_level_value(info, &level)) {
		PyErr_SetString(spoolss_error, "invalid info level");
		return NULL;
	}

	if (level != 1) {
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}

	/* Call rpc function */

	if (!py_to_FORM(&form, info)) {
		PyErr_SetString(spoolss_error, "invalid form");
		return NULL;
	}

	form_name = PyDict_GetItemString(info, "name");

	werror = rpccli_spoolss_setform(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, 
		PyString_AsString(form_name), &form);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Delete a form */

PyObject *spoolss_hnd_deleteform(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = {"form_name", NULL};
	char *form_name;

	/* Parse parameters */
	
	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s", kwlist, &form_name))
		return NULL;
	
	/* Call rpc function */

	werror = rpccli_spoolss_deleteform(
		hnd->cli, hnd->mem_ctx, &hnd->pol, form_name);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Enumerate forms */

PyObject *spoolss_hnd_enumforms(PyObject *self, PyObject *args, PyObject *kw)
{
	PyObject *result;
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	uint32 level = 1, num_forms, i;
	static char *kwlist[] = {"level", NULL};
	FORM_1 *forms;

	/* Parse parameters */
	
	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "|i", kwlist, &level))
		return NULL;
	
	/* Call rpc function */

	werror = rpccli_spoolss_enumforms(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, &num_forms, &forms);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	switch(level) {
	case 1:
		result = PyDict_New();

		for (i = 0; i < num_forms; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, forms[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_FORM_1(&value, &forms[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(1));

			PyDict_SetItemString(result, name, value);
		}

		break;
	default:
		PyErr_SetString(spoolss_error, "unknown info level");
		return NULL;
	}

	return result;
}
