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

/* Enumerate ports */

PyObject *spoolss_enumports(PyObject *self, PyObject *args, PyObject *kw)
{
	WERROR werror;
	PyObject *result = NULL, *creds = NULL;
	uint32 level = 1;
	uint32 i, needed, num_ports;
	static char *kwlist[] = {"server", "level", "creds", NULL};
	TALLOC_CTX *mem_ctx = NULL;
	struct cli_state *cli = NULL;
	char *server, *errstr;
	PORT_INFO_CTR ctr;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s|iO", kwlist, &server, &level, &creds))
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

	if (!(mem_ctx = talloc_init("spoolss_enumports"))) {
		PyErr_SetString(
			spoolss_error, "unable to init talloc context\n");
		goto done;
	}

	/* Call rpc function */
	
	werror = rpccli_spoolss_enum_ports( 
		cli->pipe_list, mem_ctx, level, &num_ports, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	/* Return value */
	
	switch (level) {
	case 1: 
		result = PyDict_New();

		for (i = 0; i < num_ports; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, ctr.port.info_1[i].port_name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_PORT_INFO_1(&value, &ctr.port.info_1[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(1));

			PyDict_SetItemString(result, name, value);
		}

		break;
	case 2:
		result = PyDict_New();

		for(i = 0; i < num_ports; i++) {
			PyObject *value;
			fstring name;

			rpcstr_pull(name, ctr.port.info_2[i].port_name.buffer,
				    sizeof(fstring), -1, STR_TERMINATE);

			py_from_PORT_INFO_2(&value, &ctr.port.info_2[i]);

			PyDict_SetItemString(
				value, "level", PyInt_FromLong(2));

			PyDict_SetItemString(result, name, value);
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
