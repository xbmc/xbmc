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

/* 
 * Exceptions raised by this module 
 */

PyObject *samr_error;		/* This indicates a non-RPC related error
				   such as name lookup failure */

PyObject *samr_ntstatus;	/* This exception is raised when a RPC call
				   returns a status code other than
				   NT_STATUS_OK */

/* SAMR group handle object */

static void py_samr_group_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyMethodDef samr_group_methods[] = {
	{ NULL }
};

static PyObject *py_samr_group_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(samr_group_methods, self, attrname);
}

PyTypeObject samr_group_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SAMR Group Handle",
	sizeof(samr_group_hnd_object),
	0,
	py_samr_group_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_samr_group_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

PyObject *new_samr_group_hnd_object(struct cli_state *cli, TALLOC_CTX *mem_ctx,
				      POLICY_HND *pol)
{
	samr_group_hnd_object *o;

	o = PyObject_New(samr_group_hnd_object, &samr_group_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->group_pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

/* Alias handle object */

static void py_samr_alias_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyMethodDef samr_alias_methods[] = {
	{ NULL }
};

static PyObject *py_samr_alias_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(samr_alias_methods, self, attrname);
}

PyTypeObject samr_alias_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SAMR Alias Handle",
	sizeof(samr_alias_hnd_object),
	0,
	py_samr_alias_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_samr_alias_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

PyObject *new_samr_alias_hnd_object(struct cli_state *cli, TALLOC_CTX *mem_ctx,
				      POLICY_HND *pol)
{
	samr_alias_hnd_object *o;

	o = PyObject_New(samr_alias_hnd_object, &samr_alias_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->alias_pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

/* SAMR user handle object */

static void py_samr_user_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyObject *samr_set_user_info2(PyObject *self, PyObject *args, 
				     PyObject *kw)
{
	samr_user_hnd_object *user_hnd = (samr_user_hnd_object *)self;
	static char *kwlist[] = { "dict", NULL };
	PyObject *info, *result = NULL;
	SAM_USERINFO_CTR ctr;
	TALLOC_CTX *mem_ctx;
	uchar sess_key[16];
	NTSTATUS ntstatus;
	int level;
	union {
		SAM_USER_INFO_16 id16;
		SAM_USER_INFO_21 id21;
	} pinfo;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &info))
		return NULL;

	if (!get_level_value(info, &level)) {
		PyErr_SetString(samr_error, "invalid info level");
		return NULL;
	}	

	ZERO_STRUCT(ctr);

	ctr.switch_value = level;

	switch(level) {
	case 16:
		ctr.info.id16 = &pinfo.id16;
		
		if (!py_to_SAM_USER_INFO_16(ctr.info.id16, info)) {
			PyErr_SetString(
				samr_error, "error converting user info");
			goto done;
		}
		
		break;
	case 21:
		ctr.info.id21 = &pinfo.id21;

		if (!py_to_SAM_USER_INFO_21(ctr.info.id21, info)) {
			PyErr_SetString(
				samr_error, "error converting user info");
			goto done;
		}

		break;
	default:
		PyErr_SetString(samr_error, "unsupported info level");
		goto done;
	}

	/* Call RPC function */

	if (!(mem_ctx = talloc_init("samr_set_user_info2"))) {
		PyErr_SetString(
			samr_error, "unable to init talloc context\n");
		goto done;
	}

	ntstatus = rpccli_samr_set_userinfo2(
		user_hnd->cli, mem_ctx, &user_hnd->user_pol, level,
		sess_key, &ctr);

	talloc_destroy(mem_ctx);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(samr_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	Py_INCREF(Py_None);
	result = Py_None;
	
done:
	return result;
}

static PyObject *samr_delete_dom_user(PyObject *self, PyObject *args, 
				      PyObject *kw)
{
	samr_user_hnd_object *user_hnd = (samr_user_hnd_object *)self;
	static char *kwlist[] = { NULL };
	NTSTATUS ntstatus;
	TALLOC_CTX *mem_ctx;
	PyObject *result = NULL;
	
	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "", kwlist))
		return NULL;

	if (!(mem_ctx = talloc_init("samr_delete_dom_user"))) {
		PyErr_SetString(samr_error, "unable to init talloc context");
		return NULL;
	}

	ntstatus = rpccli_samr_delete_dom_user(
		user_hnd->cli, mem_ctx, &user_hnd->user_pol);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(samr_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	Py_INCREF(Py_None);
	result = Py_None;

done:
	talloc_destroy(mem_ctx);

	return result;
}

static PyMethodDef samr_user_methods[] = {
	{ "delete_domain_user", (PyCFunction)samr_delete_dom_user,
	  METH_VARARGS | METH_KEYWORDS,
	  "Delete domain user." },
	{ "set_user_info2", (PyCFunction)samr_set_user_info2,
	  METH_VARARGS | METH_KEYWORDS,
	  "Set user info 2" },
	{ NULL }
};

static PyObject *py_samr_user_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(samr_user_methods, self, attrname);
}

PyTypeObject samr_user_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SAMR User Handle",
	sizeof(samr_user_hnd_object),
	0,
	py_samr_user_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_samr_user_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

PyObject *new_samr_user_hnd_object(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				   POLICY_HND *pol)
{
	samr_user_hnd_object *o;

	o = PyObject_New(samr_user_hnd_object, &samr_user_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->user_pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

/* SAMR connect handle object */

static void py_samr_connect_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

PyObject *new_samr_domain_hnd_object(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				     POLICY_HND *pol)
{
	samr_domain_hnd_object *o;

	o = PyObject_New(samr_domain_hnd_object, &samr_domain_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->domain_pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

static PyObject *samr_open_domain(PyObject *self, PyObject *args, PyObject *kw)
{
	samr_connect_hnd_object *connect_hnd = (samr_connect_hnd_object *)self;
	static char *kwlist[] = { "sid", "access", NULL };
	uint32 desired_access = MAXIMUM_ALLOWED_ACCESS;
	char *sid_str;
	DOM_SID sid;
	TALLOC_CTX *mem_ctx = NULL;
	POLICY_HND domain_pol;
	NTSTATUS ntstatus;
	PyObject *result = NULL;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|i", kwlist, &sid_str, &desired_access))
		return NULL;

	if (!string_to_sid(&sid, sid_str)) {
		PyErr_SetString(PyExc_TypeError, "string is not a sid");
		return NULL;
	}

	if (!(mem_ctx = talloc_init("samr_open_domain"))) {
		PyErr_SetString(samr_error, "unable to init talloc context");
		return NULL;
	}

	ntstatus = rpccli_samr_open_domain(
		connect_hnd->cli, mem_ctx, &connect_hnd->connect_pol,
		desired_access, &sid, &domain_pol);
					
	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(samr_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	result = new_samr_domain_hnd_object(
		connect_hnd->cli, mem_ctx, &domain_pol);

done:
	if (!result) {
		if (mem_ctx)
			talloc_destroy(mem_ctx);
	}

	return result;
}

static PyMethodDef samr_connect_methods[] = {
	{ "open_domain", (PyCFunction)samr_open_domain,
	  METH_VARARGS | METH_KEYWORDS,
	  "Open a handle on a domain" },

	{ NULL }
};

static PyObject *py_samr_connect_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(samr_connect_methods, self, attrname);
}

PyTypeObject samr_connect_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SAMR Connect Handle",
	sizeof(samr_connect_hnd_object),
	0,
	py_samr_connect_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_samr_connect_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

PyObject *new_samr_connect_hnd_object(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx,
				      POLICY_HND *pol)
{
	samr_connect_hnd_object *o;

	o = PyObject_New(samr_connect_hnd_object, &samr_connect_hnd_type);

	o->cli = cli;
	o->mem_ctx = mem_ctx;
	memcpy(&o->connect_pol, pol, sizeof(POLICY_HND));

	return (PyObject*)o;
}

/* SAMR domain handle object */

static void py_samr_domain_hnd_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyObject *samr_enum_dom_groups(PyObject *self, PyObject *args, 
				      PyObject *kw)
{
	samr_domain_hnd_object *domain_hnd = (samr_domain_hnd_object *)self;
	static char *kwlist[] = { NULL };
	TALLOC_CTX *mem_ctx;
/*	uint32 desired_access = MAXIMUM_ALLOWED_ACCESS; */
	uint32 start_idx, size, num_dom_groups;
	struct acct_info *dom_groups;
	NTSTATUS result;
	PyObject *py_result = NULL;
	
	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;

	if (!(mem_ctx = talloc_init("samr_enum_dom_groups"))) {
		PyErr_SetString(samr_error, "unable to init talloc context");
		return NULL;
	}

	start_idx = 0;
	size = 0xffff;

	do {
		result = rpccli_samr_enum_dom_groups(
			domain_hnd->cli, mem_ctx, &domain_hnd->domain_pol,
			&start_idx, size, &dom_groups, &num_dom_groups);

		if (NT_STATUS_IS_OK(result) ||
		    NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES)) {
			py_from_acct_info(&py_result, dom_groups,
					  num_dom_groups);
		}

	} while (NT_STATUS_V(result) == NT_STATUS_V(STATUS_MORE_ENTRIES));

	return py_result;
}	

static PyObject *samr_create_dom_user(PyObject *self, PyObject *args, 
				      PyObject *kw)
{
	samr_domain_hnd_object *domain_hnd = (samr_domain_hnd_object *)self;
	static char *kwlist[] = { "account_name", "acb_info", NULL };
	char *account_name;
	NTSTATUS ntstatus;
	uint32 unknown = 0xe005000b; /* Access mask? */
	uint32 user_rid;
	PyObject *result = NULL;
	TALLOC_CTX *mem_ctx;
	uint32 acb_info = ACB_NORMAL;
	POLICY_HND user_pol;
	
	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|i", kwlist, &account_name, &acb_info))
		return NULL;

	if (!(mem_ctx = talloc_init("samr_create_dom_user"))) {
		PyErr_SetString(samr_error, "unable to init talloc context");
		return NULL;
	}

	ntstatus = rpccli_samr_create_dom_user(
		domain_hnd->cli, mem_ctx, &domain_hnd->domain_pol,
		account_name, acb_info, unknown, &user_pol, &user_rid);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		PyErr_SetObject(samr_ntstatus, py_ntstatus_tuple(ntstatus));
		talloc_destroy(mem_ctx);
		goto done;
	}

	result = new_samr_user_hnd_object(
		domain_hnd->cli, mem_ctx, &user_pol);

done:

	return result;
}

static PyMethodDef samr_domain_methods[] = {
	{ "enum_domain_groups", (PyCFunction)samr_enum_dom_groups,
	  METH_VARARGS | METH_KEYWORDS, "Enumerate domain groups" },
	{ "create_domain_user", (PyCFunction)samr_create_dom_user,
	  METH_VARARGS | METH_KEYWORDS, "Create domain user" },
  	{ NULL }
};

static PyObject *py_samr_domain_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(samr_domain_methods, self, attrname);
}

PyTypeObject samr_domain_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SAMR Domain Handle",
	sizeof(samr_domain_hnd_object),
	0,
	py_samr_domain_hnd_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_samr_domain_hnd_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

static PyObject *samr_connect(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = { "server", "creds", "access", NULL };
	uint32 desired_access = MAXIMUM_ALLOWED_ACCESS;
	char *server, *errstr;
	struct cli_state *cli = NULL;
	POLICY_HND hnd;
	TALLOC_CTX *mem_ctx = NULL;
	PyObject *result = NULL, *creds = NULL;
	NTSTATUS ntstatus;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|Oi", kwlist, &server, &creds,
		    &desired_access)) 
		return NULL;

	if (server[0] != '\\' || server[1] != '\\') {
		PyErr_SetString(PyExc_ValueError, "UNC name required");
		return NULL;
	}

	server += 2;

	if (creds && creds != Py_None && !PyDict_Check(creds)) {
		PyErr_SetString(PyExc_TypeError, 
				"credentials must be dictionary or None");
		return NULL;
	}

	if (!(cli = open_pipe_creds(server, creds, PI_SAMR, &errstr))) {
		PyErr_SetString(samr_error, errstr);
		free(errstr);
		return NULL;
	}

	if (!(mem_ctx = talloc_init("samr_connect"))) {
		PyErr_SetString(samr_ntstatus,
				"unable to init talloc context\n");
		goto done;
	}

	ntstatus = rpccli_samr_connect(cli->pipe_list, mem_ctx, desired_access, &hnd);

	if (!NT_STATUS_IS_OK(ntstatus)) {
		cli_shutdown(cli);
		PyErr_SetObject(samr_ntstatus, py_ntstatus_tuple(ntstatus));
		goto done;
	}

	result = new_samr_connect_hnd_object(cli->pipe_list, mem_ctx, &hnd);

done:
	if (!result) {
		if (cli)
			cli_shutdown(cli);

		if (mem_ctx)
			talloc_destroy(mem_ctx);
	}

	return result;
}

/*
 * Module initialisation 
 */

static PyMethodDef samr_methods[] = {

	/* Open/close samr connect handles */
	
	{ "connect", (PyCFunction)samr_connect, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Open a connect handle" },
	
	{ NULL }
};

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {

	/* Account control bits */

	{ "ACB_DISABLED", 0x0001 },
	{ "ACB_HOMDIRREQ", 0x0002 },
	{ "ACB_PWNOTREQ", 0x0004 },
	{ "ACB_TEMPDUP", 0x0008 },
	{ "ACB_NORMAL", 0x0010 },
	{ "ACB_MNS", 0x0020 },
	{ "ACB_DOMTRUST", 0x0040 },
	{ "ACB_WSTRUST", 0x0080 },
	{ "ACB_SVRTRUST", 0x0100 },
	{ "ACB_PWNOEXP", 0x0200 },
	{ "ACB_AUTOLOCK", 0x0400 },

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

void initsamr(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("samr", samr_methods);
	dict = PyModule_GetDict(module);

	samr_error = PyErr_NewException("samr.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", samr_error);

	samr_ntstatus = PyErr_NewException("samr.ntstatus", NULL, NULL);
	PyDict_SetItemString(dict, "ntstatus", samr_ntstatus);

	/* Initialise policy handle object */

	samr_connect_hnd_type.ob_type = &PyType_Type;
	samr_domain_hnd_type.ob_type = &PyType_Type;
	samr_user_hnd_type.ob_type = &PyType_Type;
	samr_group_hnd_type.ob_type = &PyType_Type;
	samr_alias_hnd_type.ob_type = &PyType_Type;

	/* Initialise constants */

	const_init(dict);

	/* Do samba initialisation */

	py_samba_init();

	setup_logging("samr", True);
	DEBUGLEVEL = 10;
}
