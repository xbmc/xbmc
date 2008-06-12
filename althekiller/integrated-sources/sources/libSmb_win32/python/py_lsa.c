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

#include "python/py_lsa.h"

PyObject *new_lsa_policy_hnd_object(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				    POLICY_HND *pol)
{
	lsa_policy_hnd_object *o;

	o = PyObject_New(lsa_policy_hnd_object, &lsa_policy_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

/* 
 * Exceptions raised by this module 
 */

PyObject *lsa_error;		/* This indicates a non-RPC related error
				   such as name lookup failure */

PyObject *lsa_ntstatus;		/* This exception is raised when a RPC call
				   returns a status code other than
				   NT_STATUS_OK */

/*
 * Open/close lsa handles
 */

static PyObject *lsa_open_policy(PyObject *self, PyObject *args, 
				PyObject *kw) 
{
	static char *kwlist[] = { "servername", "creds", "access", NULL };
	char *server, *errstr;
	PyObject *creds = NULL, *result = NULL;
	uint32 desired_access = GENERIC_EXECUTE_ACCESS;
	struct cli_state *cli = NULL;
	NTSTATUS ntstatus;
	TALLOC_CTX *mem_ctx = NULL;
	POLICY_HND hnd;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|Oi", kwlist, &server, &creds, &desired_access))
		return NULL;

	if (creds && creds != Py_None && !PyDict_Check(creds)) {
		PyErr_SetString(PyExc_TypeError, 
				"credentials must be dictionary or None");
		return NULL;
	}

	if (server[0] != '\\' || server[1] != '\\') {
		PyErr_SetString(PyExc_ValueError, "UNC name required");
		return NULL;
	}

	server += 2;

	if (!(cli = open_pipe_creds(server, creds, PI_LSARPC, &errstr))) {
		PyErr_SetString(lsa_error, errstr);
		free(errstr);
		return NULL;
	}

	if (!(mem_ctx = talloc_init("lsa_open_policy"))) {
		PyErr_SetString(lsa_error, "unable to init talloc context\n");
		goto done;
	}

	ntstatus = rpccli_lsa_open_policy(
		cli->pipe_list, mem_ctx, True, desired_access, &hnd);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(lsa_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	result = new_lsa_policy_hnd_object(cli->pipe_list, mem_ctx, &hnd);

done:
	if (!result) {
		if (cli)
			cli_shutdown(cli);

		talloc_destroy(mem_ctx);
	}

	return result;
}

static PyObject *lsa_close(PyObject *self, PyObject *args, PyObject *kw) 
{
	PyObject *po;
	lsa_policy_hnd_object *hnd;
	NTSTATUS result;

	/* Parse parameters */

	if (!PyArg_ParseTuple(args, "O!", &lsa_policy_hnd_type, &po))
		return NULL;

	hnd = (lsa_policy_hnd_object *)po;

	/* Call rpc function */

	result = rpccli_lsa_close(hnd->cli, hnd->mem_ctx, &hnd->pol);

	/* Cleanup samba stuff */

	cli_shutdown(hnd->cli);
	talloc_destroy(hnd->mem_ctx);

	/* Return value */

	Py_INCREF(Py_None);
	return Py_None;	
}

static PyObject *lsa_lookup_names(PyObject *self, PyObject *args)
{
	PyObject *py_names, *result = NULL;
	NTSTATUS ntstatus;
	lsa_policy_hnd_object *hnd = (lsa_policy_hnd_object *)self;
	int num_names, i;
	const char **names;
	DOM_SID *sids;
	TALLOC_CTX *mem_ctx = NULL;
	uint32 *name_types;

	if (!PyArg_ParseTuple(args, "O", &py_names))
		return NULL;

	if (!PyList_Check(py_names) && !PyString_Check(py_names)) {
		PyErr_SetString(PyExc_TypeError, "must be list or string");
		return NULL;
	}

	if (!(mem_ctx = talloc_init("lsa_lookup_names"))) {
		PyErr_SetString(lsa_error, "unable to init talloc context\n");
		goto done;
	}

	if (PyList_Check(py_names)) {

		/* Convert list to char ** array */

		num_names = PyList_Size(py_names);
		names = (const char **)_talloc(mem_ctx, num_names * sizeof(char *));
		
		for (i = 0; i < num_names; i++) {
			PyObject *obj = PyList_GetItem(py_names, i);
			
			names[i] = talloc_strdup(mem_ctx, PyString_AsString(obj));
		}

	} else {

		/* Just a single element */

		num_names = 1;
		names = (const char **)_talloc(mem_ctx, sizeof(char *));

		names[0] = PyString_AsString(py_names);
	}

	ntstatus = rpccli_lsa_lookup_names(
		hnd->cli, mem_ctx, &hnd->pol, num_names, names, 
		NULL, &sids, &name_types);

	if (!NT_STATUS_IS_OK(ntstatus) && NT_STATUS_V(ntstatus) != 0x107) {
		PyErr_SetObject(lsa_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	result = PyList_New(num_names);

	for (i = 0; i < num_names; i++) {
		PyObject *sid_obj, *obj;

		py_from_SID(&sid_obj, &sids[i]);

		obj = Py_BuildValue("(Ni)", sid_obj, name_types[i]);

		PyList_SetItem(result, i, obj);
	}

 done:
	talloc_destroy(mem_ctx);
	
	return result;
}

static PyObject *lsa_lookup_sids(PyObject *self, PyObject *args, 
				 PyObject *kw) 
{
	PyObject *py_sids, *result = NULL;
	NTSTATUS ntstatus;
	int num_sids, i;
	char **domains, **names;
	uint32 *types;
	lsa_policy_hnd_object *hnd = (lsa_policy_hnd_object *)self;
	TALLOC_CTX *mem_ctx = NULL;
	DOM_SID *sids;

	if (!PyArg_ParseTuple(args, "O", &py_sids))
		return NULL;

	if (!PyList_Check(py_sids) && !PyString_Check(py_sids)) {
		PyErr_SetString(PyExc_TypeError, "must be list or string");
		return NULL;
	}

	if (!(mem_ctx = talloc_init("lsa_lookup_sids"))) {
		PyErr_SetString(lsa_error, "unable to init talloc context\n");
		goto done;
	}

	if (PyList_Check(py_sids)) {

		/* Convert dictionary to char ** array */
		
		num_sids = PyList_Size(py_sids);
		sids = (DOM_SID *)_talloc(mem_ctx, num_sids * sizeof(DOM_SID));
		
		memset(sids, 0, num_sids * sizeof(DOM_SID));
		
		for (i = 0; i < num_sids; i++) {
			PyObject *obj = PyList_GetItem(py_sids, i);
			
			if (!string_to_sid(&sids[i], PyString_AsString(obj))) {
				PyErr_SetString(PyExc_ValueError, "string_to_sid failed");
				goto done;
			}
		}

	} else {

		/* Just a single element */

		num_sids = 1;
		sids = (DOM_SID *)_talloc(mem_ctx, sizeof(DOM_SID));

		if (!string_to_sid(&sids[0], PyString_AsString(py_sids))) {
			PyErr_SetString(PyExc_ValueError, "string_to_sid failed");
			goto done;
		}
	}

	ntstatus = rpccli_lsa_lookup_sids(
		hnd->cli, mem_ctx, &hnd->pol, num_sids, sids, &domains, 
		&names, &types);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(lsa_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	result = PyList_New(num_sids);

	for (i = 0; i < num_sids; i++) {
		PyObject *obj;

		obj = Py_BuildValue("{sssssi}", "username", names[i],
				    "domain", domains[i], "name_type", 
				    types[i]);

		PyList_SetItem(result, i, obj);
	}

 done:
	talloc_destroy(mem_ctx);

	return result;
}

static PyObject *lsa_enum_trust_dom(PyObject *self, PyObject *args)
{
	lsa_policy_hnd_object *hnd = (lsa_policy_hnd_object *)self;
	NTSTATUS ntstatus;
	uint32 enum_ctx = 0, num_domains, i;
	char **domain_names;
	DOM_SID *domain_sids;
	PyObject *result;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	
	ntstatus = rpccli_lsa_enum_trust_dom(
		hnd->cli, hnd->mem_ctx, &hnd->pol, &enum_ctx,
		&num_domains, &domain_names, &domain_sids);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(lsa_ntstatus, py_ntstatus_tuple(ntstatus));
		return NULL;
	}

	result = PyList_New(num_domains);

	for (i = 0; i < num_domains; i++) {
		fstring sid_str;

		sid_to_string(sid_str, &domain_sids[i]);
		PyList_SetItem(
			result, i, 
			Py_BuildValue("(ss)", domain_names[i], sid_str));
	}

	return result;
}

/*
 * Method dispatch tables
 */

static PyMethodDef lsa_hnd_methods[] = {

	/* SIDs<->names */

	{ "lookup_sids", (PyCFunction)lsa_lookup_sids, 
	  METH_VARARGS | METH_KEYWORDS,
	  "Convert sids to names." },

	{ "lookup_names", (PyCFunction)lsa_lookup_names, 
	  METH_VARARGS | METH_KEYWORDS,
	  "Convert names to sids." },

	/* Trusted domains */

	{ "enum_trusted_domains", (PyCFunction)lsa_enum_trust_dom, 
	  METH_VARARGS, 
	  "Enumerate trusted domains." },

	{ NULL }
};

static void py_lsa_policy_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyObject *py_lsa_policy_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(lsa_hnd_methods, self, attrname);
}

PyTypeObject lsa_policy_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"LSA Policy Handle",
	sizeof(lsa_policy_hnd_object),
	0,
	py_lsa_policy_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_lsa_policy_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

static PyMethodDef lsa_methods[] = {

	/* Open/close lsa handles */
	
	{ "open_policy", (PyCFunction)lsa_open_policy, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Open a policy handle" },
	
	{ "close", (PyCFunction)lsa_close, 
	  METH_VARARGS, 
	  "Close a policy handle" },

	/* Other stuff - this should really go into a samba config module
  	   but for the moment let's leave it here. */

	{ "setup_logging", (PyCFunction)py_setup_logging, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Set up debug logging.\n"
"\n"
"Initialises Samba's debug logging system.  One argument is expected which\n"
"is a boolean specifying whether debugging is interactive and sent to stdout\n"
"or logged to a file.\n"
"\n"
"Example:\n"
"\n"
">>> lsa.setup_logging(interactive = 1)" },

	{ "get_debuglevel", (PyCFunction)get_debuglevel, 
	  METH_VARARGS, 
	  "Set the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> lsa.get_debuglevel()\n"
"0" },

	{ "set_debuglevel", (PyCFunction)set_debuglevel, 
	  METH_VARARGS, 
	  "Get the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> lsa.set_debuglevel(10)" },

	{ NULL }
};

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {
	{ NULL }
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

void initlsa(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("lsa", lsa_methods);
	dict = PyModule_GetDict(module);

	lsa_error = PyErr_NewException("lsa.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", lsa_error);

	lsa_ntstatus = PyErr_NewException("lsa.ntstatus", NULL, NULL);
	PyDict_SetItemString(dict, "ntstatus", lsa_ntstatus);

	/* Initialise policy handle object */

	lsa_policy_hnd_type.ob_type = &PyType_Type;

	/* Initialise constants */

	const_init(dict);

	/* Do samba initialisation */

	py_samba_init();

	setup_logging("lsa", True);
	DEBUGLEVEL = 10;
}
