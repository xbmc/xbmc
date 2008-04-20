/* 
   Python wrappers for DCERPC/SMB client routines.

   Copyright (C) Tim Potter, 2003
   
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

#include "python/py_srvsvc.h"

/* Exceptions this module can raise */

PyObject *srvsvc_error, *srvsvc_werror;

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {
	{ "SV_TYPE_WORKSTATION", SV_TYPE_WORKSTATION },
	{ "SV_TYPE_SERVER", SV_TYPE_SERVER },
	{ "SV_TYPE_SQLSERVER", SV_TYPE_SQLSERVER },
	{ "SV_TYPE_DOMAIN_CTRL", SV_TYPE_DOMAIN_CTRL },
	{ "SV_TYPE_DOMAIN_BAKCTRL", SV_TYPE_DOMAIN_BAKCTRL },
	{ "SV_TYPE_TIME_SOURCE", SV_TYPE_TIME_SOURCE },
	{ "SV_TYPE_AFP", SV_TYPE_AFP },
	{ "SV_TYPE_NOVELL", SV_TYPE_NOVELL },
	{ "SV_TYPE_DOMAIN_MEMBER", SV_TYPE_DOMAIN_MEMBER },
	{ "SV_TYPE_PRINTQ_SERVER", SV_TYPE_PRINTQ_SERVER },
	{ "SV_TYPE_DIALIN_SERVER", SV_TYPE_DIALIN_SERVER },
	{ "SV_TYPE_SERVER_UNIX", SV_TYPE_SERVER_UNIX },
	{ "SV_TYPE_NT", SV_TYPE_NT },
	{ "SV_TYPE_WFW", SV_TYPE_WFW },
	{ "SV_TYPE_SERVER_MFPN", SV_TYPE_SERVER_MFPN },
	{ "SV_TYPE_SERVER_NT", SV_TYPE_SERVER_NT },
	{ "SV_TYPE_POTENTIAL_BROWSER", SV_TYPE_POTENTIAL_BROWSER },
	{ "SV_TYPE_BACKUP_BROWSER", SV_TYPE_BACKUP_BROWSER },
	{ "SV_TYPE_MASTER_BROWSER", SV_TYPE_MASTER_BROWSER },
	{ "SV_TYPE_DOMAIN_MASTER", SV_TYPE_DOMAIN_MASTER },
	{ "SV_TYPE_SERVER_OSF", SV_TYPE_SERVER_OSF },
	{ "SV_TYPE_SERVER_VMS", SV_TYPE_SERVER_VMS },
	{ "SV_TYPE_WIN95_PLUS", SV_TYPE_WIN95_PLUS },
	{ "SV_TYPE_DFS_SERVER", SV_TYPE_DFS_SERVER },
	{ "SV_TYPE_ALTERNATE_XPORT", SV_TYPE_ALTERNATE_XPORT },
	{ "SV_TYPE_LOCAL_LIST_ONLY", SV_TYPE_LOCAL_LIST_ONLY },
	{ "SV_TYPE_DOMAIN_ENUM", SV_TYPE_DOMAIN_ENUM },
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

/* NetServerGetInfo */

PyObject *srvsvc_netservergetinfo(PyObject *self, PyObject *args,
				  PyObject *kw)
{
	static char *kwlist[] = { "server", "level", "creds", NULL };
	char *unc_name, *server, *errstr;
	PyObject *creds = NULL, *result = NULL;
	struct cli_state *cli;
	TALLOC_CTX *mem_ctx = NULL;
	uint32 level;
	SRV_INFO_CTR ctr;
	WERROR status;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "si|O", kwlist, &unc_name, &level, &creds))
		return NULL;

	if (unc_name[0] != '\\' || unc_name[1] != '\\') {
		PyErr_SetString(PyExc_ValueError, "UNC name required");
		return NULL;
	}

	server = SMB_STRDUP(unc_name + 2);

	if (strchr(server, '\\')) {
		char *c = strchr(server, '\\');
		*c = 0;
	}

	if (creds && creds != Py_None && !PyDict_Check(creds)) {
		PyErr_SetString(PyExc_TypeError, 
				"credentials must be dictionary or None");
		return NULL;
	}

	if (!(cli = open_pipe_creds(server, creds, PI_SRVSVC, &errstr))) {
		PyErr_SetString(srvsvc_error, errstr);
		free(errstr);
		goto done;
	}

	if (!(mem_ctx = talloc_init("srvsvc_netservergetinfo"))) {
		PyErr_SetString(srvsvc_error, 
				"unable to init talloc context\n");
		goto done;
	}

	ZERO_STRUCT(ctr);

	status = rpccli_srvsvc_net_srv_get_info(cli->pipe_list, mem_ctx, level, &ctr);

	if (!NT_STATUS_IS_OK(status)) {
		PyErr_SetObject(srvsvc_error, py_werror_tuple(status));
		goto done;
	}

	if (level != ctr.switch_value) {
		PyErr_SetString(srvsvc_error, "container level value wrong");
		goto done;
	}

	switch(level) {
	case 101:
		py_from_SRV_INFO_101(&result, &ctr.srv.sv101);
		break;
	}

	Py_INCREF(result);

done:
	if (mem_ctx)
		talloc_destroy(mem_ctx);

	return result;
}

/*
 * Module initialisation 
 */

static PyMethodDef srvsvc_methods[] = {
	{ "netservergetinfo", (PyCFunction)srvsvc_netservergetinfo,
	  METH_VARARGS | METH_KEYWORDS,
	  "Retrieve information about a particular server." },

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
">>> srvsvc.setup_logging(interactive = 1)" },

	{ "get_debuglevel", (PyCFunction)get_debuglevel, 
	  METH_VARARGS, 
	  "Set the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> srvsvc.get_debuglevel()\n"
"0" },

	{ "set_debuglevel", (PyCFunction)set_debuglevel, 
	  METH_VARARGS, 
	  "Get the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> srvsvc.set_debuglevel(10)" },

	{ NULL }
};

void initsrvsvc(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("srvsvc", srvsvc_methods);
	dict = PyModule_GetDict(module);

	/* Exceptions we can raise */

	srvsvc_error = PyErr_NewException("srvsvc.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", srvsvc_error);

	srvsvc_werror = PyErr_NewException("srvsvc.werror", NULL, NULL);
	PyDict_SetItemString(dict, "werror", srvsvc_werror);

	/* Initialise constants */

	const_init(dict);

	/* Do samba initialisation */

	py_samba_init();
}
