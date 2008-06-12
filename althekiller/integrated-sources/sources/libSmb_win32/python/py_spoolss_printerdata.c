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

static BOOL py_from_printerdata(PyObject **dict, char *key, char *value,
				uint16 data_type, uint8 *data, 
				uint32 data_size) 
{
	*dict = PyDict_New();

	PyDict_SetItemString(*dict, "key", Py_BuildValue("s", key ? key : ""));
	PyDict_SetItemString(*dict, "value", Py_BuildValue("s", value));
	PyDict_SetItemString(*dict, "type", Py_BuildValue("i", data_type));

	PyDict_SetItemString(*dict, "data", 
			     Py_BuildValue("s#", data, data_size));

	return True;
}

static BOOL py_to_printerdata(char **key, char **value, uint16 *data_type, 
			      uint8 **data, uint32 *data_size, 
			      PyObject *dict)
{
	PyObject *obj;

	if ((obj = PyDict_GetItemString(dict, "key"))) {

		if (!PyString_Check(obj)) {
			PyErr_SetString(spoolss_error,
					"key not a string");
			return False;
		}

		if (key) {
			*key = PyString_AsString(obj);

			if (!key[0])
				*key = NULL;
		}
	} else
		*key = NULL;

	if ((obj = PyDict_GetItemString(dict, "value"))) {

		if (!PyString_Check(obj)) {
			PyErr_SetString(spoolss_error,
					"value not a string");
			return False;
		}

		*value = PyString_AsString(obj);
	} else {
		PyErr_SetString(spoolss_error, "no value present");
		return False;
	}

	if ((obj = PyDict_GetItemString(dict, "type"))) {

		if (!PyInt_Check(obj)) {
			PyErr_SetString(spoolss_error,
					"type not an integer");
			return False;
		}

		*data_type = PyInt_AsLong(obj);
	} else {
		PyErr_SetString(spoolss_error, "no type present");
		return False;
	}
	
	if ((obj = PyDict_GetItemString(dict, "data"))) {

		if (!PyString_Check(obj)) {
			PyErr_SetString(spoolss_error,
					"data not a string");
			return False;
		}

		*data = PyString_AsString(obj);
		*data_size = PyString_Size(obj);
	} else {
		PyErr_SetString(spoolss_error, "no data present");
		return False;
	}

	return True;
}

PyObject *spoolss_hnd_getprinterdata(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "value", NULL };
	char *valuename;
	WERROR werror;
	PyObject *result;
	REGISTRY_VALUE value;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &valuename))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_getprinterdata(
		hnd->cli, hnd->mem_ctx, &hnd->pol, valuename,
		&value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	py_from_printerdata(
		&result, NULL, valuename, value.type, value.data_p, 
		value.size);

	return result;
}

PyObject *spoolss_hnd_setprinterdata(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "data", NULL };
	PyObject *py_data;
	char *valuename;
	WERROR werror;
	REGISTRY_VALUE value;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &py_data))
		return NULL;
	
	if (!py_to_printerdata(
		    NULL, &valuename, &value.type, &value.data_p, 
		    &value.size, py_data))
		return NULL;

	fstrcpy(value.valuename, valuename);
	
	/* Call rpc function */

	werror = rpccli_spoolss_setprinterdata(
		hnd->cli, hnd->mem_ctx, &hnd->pol, &value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *spoolss_hnd_enumprinterdata(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { NULL };
	uint32 data_needed, value_needed, ndx = 0;
	WERROR werror;
	PyObject *result;
	REGISTRY_VALUE value;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;

	/* Get max buffer sizes for value and data */

	werror = rpccli_spoolss_enumprinterdata(
		hnd->cli, hnd->mem_ctx, &hnd->pol, ndx, 0, 0,
		&value_needed, &data_needed, NULL);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	/* Iterate over all printerdata */

	result = PyDict_New();

	while (W_ERROR_IS_OK(werror)) {
		PyObject *obj;

		werror = rpccli_spoolss_enumprinterdata(
			hnd->cli, hnd->mem_ctx, &hnd->pol, ndx,
			value_needed, data_needed, NULL, NULL, &value);

		if (py_from_printerdata(
			    &obj, NULL, value.valuename, value.type, 
			    value.data_p, value.size))
			PyDict_SetItemString(result, value.valuename, obj);

		ndx++;
	}

	return result;
}

PyObject *spoolss_hnd_deleteprinterdata(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "value", NULL };
	char *value;
	WERROR werror;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &value))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_deleteprinterdata(
		hnd->cli, hnd->mem_ctx, &hnd->pol, value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *spoolss_hnd_getprinterdataex(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "key", "value", NULL };
	char *key, *valuename;
	WERROR werror;
	PyObject *result;
	REGISTRY_VALUE value;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "ss", kwlist, &key, &valuename))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_getprinterdataex(
		hnd->cli, hnd->mem_ctx, &hnd->pol, key,
		valuename, &value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	py_from_printerdata(
		&result, key, valuename, value.type, value.data_p, value.size);

	return result;
}

PyObject *spoolss_hnd_setprinterdataex(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "data", NULL };
	PyObject *py_data;
	char *keyname, *valuename;
	WERROR werror;
	REGISTRY_VALUE value;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &py_data))
		return NULL;
	
	if (!py_to_printerdata(
		    &keyname, &valuename, &value.type, &value.data_p, &value.size, py_data))
		return NULL;

	fstrcpy(value.valuename,  valuename);

	/* Call rpc function */

	werror = rpccli_spoolss_setprinterdataex(
		hnd->cli, hnd->mem_ctx, &hnd->pol, keyname, &value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *spoolss_hnd_enumprinterdataex(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "key", NULL };
	uint32 i;
	char *key;
	WERROR werror;
	PyObject *result;
	REGVAL_CTR *ctr;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &key))
		return NULL;

	if (!(ctr = TALLOC_ZERO_P(hnd->mem_ctx, REGVAL_CTR))) {
		PyErr_SetString(spoolss_error, "talloc failed");
		return NULL;
	}

	/* Get max buffer sizes for value and data */

	werror = rpccli_spoolss_enumprinterdataex(
		hnd->cli, hnd->mem_ctx, &hnd->pol, key, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	/* Iterate over all printerdata */

	result = PyDict_New();

	for (i = 0; i < regval_ctr_numvals(&ctr); i++) {
		REGISTRY_VALUE *value;
		PyObject *item;

		item = PyDict_New();
		value = regval_ctr_specific_value(&ctr, i);

		if (py_from_printerdata(
			    &item, key, value->valuename, value->type, 
			    value->data_p, value->size))
			PyDict_SetItemString(result, value->valuename, item);
	}
	
	return result;
}

PyObject *spoolss_hnd_deleteprinterdataex(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "key", "value", NULL };
	char *key, *value;
	WERROR werror;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "ss", kwlist, &key, &value))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_deleteprinterdataex(
		hnd->cli, hnd->mem_ctx, &hnd->pol, key, value);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *spoolss_hnd_enumprinterkey(PyObject *self, PyObject *args,
				     PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "key", NULL };
	char *keyname;
	WERROR werror;
	uint32 keylist_len;
	uint16 *keylist;
	PyObject *result;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &keyname))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_enumprinterkey(
		hnd->cli, hnd->mem_ctx, &hnd->pol, keyname, &keylist, 
		&keylist_len);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	result = from_unistr_list(keylist);

	return result;
}

#if 0

PyObject *spoolss_hnd_deleteprinterkey(PyObject *self, PyObject *args,
				       PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	static char *kwlist[] = { "key", NULL };
	char *keyname;
	WERROR werror;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &keyname))
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

#endif
