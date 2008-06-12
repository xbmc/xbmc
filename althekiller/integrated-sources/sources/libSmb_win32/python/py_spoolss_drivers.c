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

/* Enumerate printer drivers */

PyObject *spoolss_enumprinterdrivers(PyObject *self, PyObject *args,
				     PyObject *kw)
{
	WERROR werror;
	PyObject *result = NULL, *creds = NULL;
	PRINTER_DRIVER_CTR ctr;
	int level = 1, i;
	uint32 num_drivers;
	char *arch = "Windows NT x86", *server, *errstr;
	static char *kwlist[] = {"server", "level", "creds", "arch", NULL};
	struct cli_state *cli = NULL;
	TALLOC_CTX *mem_ctx = NULL;
	
	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|iOs", kwlist, &server, &level, &creds,
		    &arch)) 
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

	/* Call rpc function */
	
	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}

	if (!(mem_ctx = talloc_init("spoolss_enumprinterdrivers"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		goto done;
	}	

	werror = rpccli_spoolss_enumprinterdrivers(
		cli->pipe_list, mem_ctx, level, arch,
		&num_drivers, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	/* Return value */

	switch (level) {
	case 1:
		result = PyDict_New();
		
		for (i = 0; i < num_drivers; i++) {
			PyObject *value;
			fstring name;
			
			rpcstr_pull(name, ctr.info1[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_DRIVER_INFO_1(&value, &ctr.info1[i]);

			PyDict_SetItemString(result, name, value);
		}
		
		break;
	case 2: 
		result = PyDict_New();

		for(i = 0; i < num_drivers; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, ctr.info2[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_DRIVER_INFO_2(&value, &ctr.info2[i]);

			PyDict_SetItemString(result, name, value);
		}

		break;
	case 3: 
		result = PyDict_New();

		for(i = 0; i < num_drivers; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, ctr.info3[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_DRIVER_INFO_3(&value, &ctr.info3[i]);

			PyDict_SetItemString(result, name, value);
		}

		break;
	case 6: 
		result = PyDict_New();

		for(i = 0; i < num_drivers; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, ctr.info6[i].name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_DRIVER_INFO_6(&value, &ctr.info6[i]);

			PyList_SetItem(result, i, value);
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

/* Fetch printer driver */

PyObject *spoolss_hnd_getprinterdriver(PyObject *self, PyObject *args,
				   PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *result = Py_None;
	PRINTER_DRIVER_CTR ctr;
	int level = 1;
	char *arch = "Windows NT x86";
	int version = 2;
	static char *kwlist[] = {"level", "arch", NULL};

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "|is", kwlist, &level, &arch))
		return NULL;

	/* Call rpc function */

	werror = rpccli_spoolss_getprinterdriver(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, arch, version, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	/* Return value */
	
	switch (level) {
	case 1:
		py_from_DRIVER_INFO_1(&result, ctr.info1);
		break;
	case 2: 
		py_from_DRIVER_INFO_2(&result, ctr.info2);
		break;
	case 3: 
		py_from_DRIVER_INFO_3(&result, ctr.info3);
		break;
	case 6:
		py_from_DRIVER_INFO_6(&result, ctr.info6);
		break;
	default:
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}
	
	Py_INCREF(result);
	return result;
}

/* Fetch printer driver directory */

PyObject *spoolss_getprinterdriverdir(PyObject *self, PyObject *args, 
				      PyObject *kw)
{
	WERROR werror;
	PyObject *result = NULL, *creds = NULL;
	DRIVER_DIRECTORY_CTR ctr;
	uint32 level = 1;
	char *arch = "Windows NT x86", *server, *errstr;
	static char *kwlist[] = {"server", "level", "arch", "creds", NULL};
	struct cli_state *cli = NULL;
	TALLOC_CTX *mem_ctx = NULL;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|isO", kwlist, &server, &level,
		    &arch, &creds))
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

	/* Call rpc function */

	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}
	
	if (!(mem_ctx = talloc_init("spoolss_getprinterdriverdir"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		goto done;
	}	

	werror = rpccli_spoolss_getprinterdriverdir(
		cli->pipe_list, mem_ctx, level, arch, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	/* Return value */
	
	switch (level) {
	case 1:
		py_from_DRIVER_DIRECTORY_1(&result, ctr.info1);
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

PyObject *spoolss_addprinterdriver(PyObject *self, PyObject *args,
				   PyObject *kw)
{
	static char *kwlist[] = { "server", "info", "creds", NULL };
	char *server, *errstr;
	uint32 level;
	PyObject *info, *result = NULL, *creds = NULL;
	WERROR werror;
	TALLOC_CTX *mem_ctx = NULL;
	struct cli_state *cli = NULL;
	PRINTER_DRIVER_CTR ctr;
	union {
		DRIVER_INFO_3 driver_3;
	} dinfo;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "sO!|O", kwlist, &server, &PyDict_Type,
		    &info, &creds))
		return NULL;
	
	if (server[0] == '\\' || server[1] == '\\')
		server += 2;

	if (creds && creds != Py_None && !PyDict_Check(creds)) {
		PyErr_SetString(PyExc_TypeError, 
				"credentials must be dictionary or None");
		return NULL;
	}

	if (!(mem_ctx = talloc_init("spoolss_addprinterdriver"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		return NULL;
	}

	if (!(cli = open_pipe_creds(server, creds, PI_SPOOLSS, &errstr))) {
		PyErr_SetString(spoolss_error, errstr);
		free(errstr);
		goto done;
	}

	if (!get_level_value(info, &level)) {
		PyErr_SetString(spoolss_error, "invalid info level");
		goto done;
	}

	if (level != 3) {
		PyErr_SetString(spoolss_error, "unsupported info level");
		goto done;
	}

	ZERO_STRUCT(ctr);
	ZERO_STRUCT(dinfo);

	switch(level) {
	case 3:
		ctr.info3 = &dinfo.driver_3;

		if (!py_to_DRIVER_INFO_3(&dinfo.driver_3, info, mem_ctx)) {
			PyErr_SetString(spoolss_error,
					"error converting to driver info 3");
			goto done;
		}

		break;
	default:
		PyErr_SetString(spoolss_error, "unsupported info level");
		goto done;
	}

	werror = rpccli_spoolss_addprinterdriver(cli->pipe_list, mem_ctx, level, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	Py_INCREF(Py_None);
	result = Py_None;

done:
	if (cli)
		cli_shutdown(cli);

	if (mem_ctx)
		talloc_destroy(mem_ctx);
	
	return result;
	
}

PyObject *spoolss_addprinterdriverex(PyObject *self, PyObject *args,
					     PyObject *kw)
{
	/* Not supported by Samba server */
	
	PyErr_SetString(spoolss_error, "Not implemented");
	return NULL;
}
	
PyObject *spoolss_deleteprinterdriver(PyObject *self, PyObject *args,
				      PyObject *kw)
{
	PyErr_SetString(spoolss_error, "Not implemented");
	return NULL;
}

PyObject *spoolss_deleteprinterdriverex(PyObject *self, PyObject *args,
					PyObject *kw)
{
	PyErr_SetString(spoolss_error, "Not implemented");
	return NULL;
}
