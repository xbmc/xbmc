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

/* Open a printer */

PyObject *spoolss_openprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	char *unc_name, *server, *errstr;
	TALLOC_CTX *mem_ctx = NULL;
	POLICY_HND hnd;
	WERROR werror;
	PyObject *result = NULL, *creds = NULL;
	static char *kwlist[] = { "printername", "creds", "access", NULL };
	uint32 desired_access = MAXIMUM_ALLOWED_ACCESS;
	struct cli_state *cli;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|Oi", kwlist, &unc_name, &creds,
		    &desired_access))
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

	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}

	if (!(mem_ctx = talloc_init("spoolss_openprinter"))) {
		PyErr_SetString(spoolss_error, 
				"unable to init talloc context\n");
		goto done;
	}

	werror = rpccli_spoolss_open_printer_ex(
		cli->pipe_list, mem_ctx, unc_name, "", desired_access, server, 
		"", &hnd);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	result = new_spoolss_policy_hnd_object(cli, mem_ctx, &hnd);

 done:
	if (!result) {
		if (cli)
			cli_shutdown(cli);

		if (mem_ctx)
			talloc_destroy(mem_ctx);
	}

	SAFE_FREE(server);

	return result;
}

/* Close a printer */

PyObject *spoolss_closeprinter(PyObject *self, PyObject *args)
{
	PyObject *po;
	spoolss_policy_hnd_object *hnd;
	WERROR result;

	/* Parse parameters */

	if (!PyArg_ParseTuple(args, "O!", &spoolss_policy_hnd_type, &po))
		return NULL;

	hnd = (spoolss_policy_hnd_object *)po;

	/* Call rpc function */

	result = rpccli_spoolss_close_printer(
		hnd->cli, hnd->mem_ctx, &hnd->pol);

	/* Return value */

	Py_INCREF(Py_None);
	return Py_None;	
}

/* Fetch printer information */

PyObject *spoolss_hnd_getprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *result = NULL;
	PRINTER_INFO_CTR ctr;
	int level = 1;
	static char *kwlist[] = {"level", NULL};
	
	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|i", kwlist, &level))
		return NULL;
	
	ZERO_STRUCT(ctr);

	/* Call rpc function */
	
	werror = rpccli_spoolss_getprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, &ctr);

	/* Return value */

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	result = Py_None;

	switch (level) {
		
	case 0:
		py_from_PRINTER_INFO_0(&result, ctr.printers_0);
		break;

	case 1:
		py_from_PRINTER_INFO_1(&result, ctr.printers_1);
		break;

	case 2:
		py_from_PRINTER_INFO_2(&result, ctr.printers_2);
		break;

	case 3:
		py_from_PRINTER_INFO_3(&result, ctr.printers_3);
		break;
	}

	Py_INCREF(result);
	return result;
}

/* Set printer information */

PyObject *spoolss_hnd_setprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *info;
	PRINTER_INFO_CTR ctr;
	uint32 level;
	static char *kwlist[] = {"dict", NULL};
	union {
		PRINTER_INFO_1 printers_1;
		PRINTER_INFO_2 printers_2;
		PRINTER_INFO_3 printers_3;
	} pinfo;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &info))
		return NULL;
	
	if (!get_level_value(info, &level)) {
		PyErr_SetString(spoolss_error, "invalid info level");
		return NULL;
	}

	if (level < 1 && level > 3) {
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}

	/* Fill in printer info */

	ZERO_STRUCT(ctr);

	switch (level) {
	case 1:
		ctr.printers_1 = &pinfo.printers_1;

		if (!py_to_PRINTER_INFO_1(ctr.printers_1, info)){
			PyErr_SetString(spoolss_error, 
					"error converting printer to info 1");
			return NULL;
		}

		break;
	case 2:
		ctr.printers_2 = &pinfo.printers_2;

		if (!py_to_PRINTER_INFO_2(ctr.printers_2, info,
					  hnd->mem_ctx)){
			PyErr_SetString(spoolss_error, 
					"error converting printer to info 2");
			return NULL;
		}

		break;
	case 3:
		ctr.printers_3 = &pinfo.printers_3;

		if (!py_to_PRINTER_INFO_3(ctr.printers_3, info,
					  hnd->mem_ctx)) {
			PyErr_SetString(spoolss_error,
					"error converting to printer info 3");
			return NULL;
		}

		break;
	default:
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}

	/* Call rpc function */
	
	werror = rpccli_spoolss_setprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, &ctr, 0);

	/* Return value */

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Enumerate printers */

PyObject *spoolss_enumprinters(PyObject *self, PyObject *args, PyObject *kw)
{
	WERROR werror;
	PyObject *result = NULL, *creds = NULL;
	PRINTER_INFO_CTR ctr;
	int level = 1, flags = PRINTER_ENUM_LOCAL, i;
	uint32 num_printers;
	static char *kwlist[] = {"server", "name", "level", "flags", 
				 "creds", NULL};
	TALLOC_CTX *mem_ctx = NULL;
	struct cli_state *cli = NULL;
	char *server, *errstr, *name = NULL;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|siiO", kwlist, &server, &name, &level, 
		    &flags, &creds))
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

	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}

	if (!(mem_ctx = talloc_init("spoolss_enumprinters"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		goto done;
	}

	/* This RPC is weird.  By setting the server name to different
	   values we can get different behaviour.  If however the server
	   name is not specified, we default it to being the full server
	   name as this is probably what the caller intended.  To pass a
	   NULL name, pass a value of "" */

	if (!name)
		name = server;
	else {
		if (!name[0])
			name = NULL;
	}

	/* Call rpc function */
	
	werror = rpccli_spoolss_enum_printers(
		cli->pipe_list, mem_ctx, name, flags, level, &num_printers, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	/* Return value */
	
	switch (level) {
	case 0: 
		result = PyDict_New();

		for (i = 0; i < num_printers; i++) {
			PyObject *value;
			fstring s;

			rpcstr_pull(s, ctr.printers_0[i].printername.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_PRINTER_INFO_0(&value, &ctr.printers_0[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(0));

			PyDict_SetItemString(result, s, value);
		}

		break;
	case 1:
		result = PyDict_New();

		for(i = 0; i < num_printers; i++) {
			PyObject *value;
			fstring s;

			rpcstr_pull(s, ctr.printers_1[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_PRINTER_INFO_1(&value, &ctr.printers_1[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(1));

			PyDict_SetItemString(result, s, value);
		}
		
		break;
	case 2:
		result = PyDict_New();

		for(i = 0; i < num_printers; i++) {
			PyObject *value;
			fstring s;

			rpcstr_pull(s, ctr.printers_2[i].printername.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_PRINTER_INFO_2(&value, &ctr.printers_2[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(2));

			PyDict_SetItemString(result, s, value);
		}
		
		break;
	default:
		PyErr_SetString(spoolss_error, "unknown info level");
		goto done;
	}

done:
	if (cli)
		cli_shutdown(cli);

	if (mem_ctx)
		talloc_destroy(mem_ctx);

	return result;
}

/* Add a printer */

PyObject *spoolss_addprinterex(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = { "server", "printername", "info", "creds", 
				  NULL};
	char *printername, *server, *errstr;
	PyObject *info, *result = NULL, *creds = NULL;
	struct cli_state *cli = NULL;
	TALLOC_CTX *mem_ctx = NULL;
	PRINTER_INFO_CTR ctr;
	PRINTER_INFO_2 info2;
	WERROR werror;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "ssO!|O!", kwlist, &server, &printername,
		    &PyDict_Type, &info, &PyDict_Type, &creds))
		return NULL;

	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}

	if (!(mem_ctx = talloc_init("spoolss_addprinterex"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		goto done;
	}

	if (!py_to_PRINTER_INFO_2(&info2, info, mem_ctx)) {
		PyErr_SetString(spoolss_error,
				"error converting to printer info 2");
		goto done;
	}

	ctr.printers_2 = &info2;

	werror = rpccli_spoolss_addprinterex(cli->pipe_list, mem_ctx, 2, &ctr);

	Py_INCREF(Py_None);
	result = Py_None;

done:
	if (cli)
		cli_shutdown(cli);

	if (mem_ctx)
		talloc_destroy(mem_ctx);

	return result;
}
