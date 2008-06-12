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

/* Return a tuple of (error code, error string) from a WERROR */

PyObject *py_werror_tuple(WERROR werror)
{
	return Py_BuildValue("[is]", W_ERROR_V(werror), 
			     dos_errstr(werror));
}

/* Return a tuple of (error code, error string) from a WERROR */

PyObject *py_ntstatus_tuple(NTSTATUS ntstatus)
{
	return Py_BuildValue("[is]", NT_STATUS_V(ntstatus), 
			     nt_errstr(ntstatus));
}

/* Initialise samba client routines */

static BOOL initialised;

void py_samba_init(void)
{
	if (initialised)
		return;

	load_case_tables();

	/* Load configuration file */

	if (!lp_load(dyn_CONFIGFILE, True, False, False, True))
		fprintf(stderr, "Can't load %s\n", dyn_CONFIGFILE);

	/* Misc other stuff */

	load_interfaces();
	init_names();

	initialised = True;
}

/* Debuglevel routines */

PyObject *get_debuglevel(PyObject *self, PyObject *args)
{
	PyObject *debuglevel;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	debuglevel = PyInt_FromLong(DEBUGLEVEL);

	return debuglevel;
}

PyObject *set_debuglevel(PyObject *self, PyObject *args)
{
	int debuglevel;

	if (!PyArg_ParseTuple(args, "i", &debuglevel))
		return NULL;

	DEBUGLEVEL = debuglevel;

	Py_INCREF(Py_None);
	return Py_None;
}

/* Initialise logging */

PyObject *py_setup_logging(PyObject *self, PyObject *args, PyObject *kw)
{
	BOOL interactive = False;
	char *logfilename = NULL;
	static char *kwlist[] = {"interactive", "logfilename", NULL};

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "|is", kwlist, &interactive, &logfilename))
		return NULL;
	
	if (interactive && logfilename) {
		PyErr_SetString(PyExc_RuntimeError,
				"can't be interactive and set log file name");
		return NULL;
	}

	if (interactive)
		setup_logging("spoolss", True);

	if (logfilename) {
		lp_set_logfile(logfilename);
		setup_logging(logfilename, False);
		reopen_logs();
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Parse credentials from a python dictionary.  The dictionary can
   only have the keys "username", "domain" and "password".  Return
   True for valid credentials in which case the username, domain and
   password are set to pointers to their values from the dicationary.
   If returns False, the errstr is set to point at some mallocated
   memory describing the error. */

BOOL py_parse_creds(PyObject *creds, char **username, char **domain, 
		    char **password, char **errstr)
{
	/* Initialise anonymous credentials */

	*username = "";
	*domain = "";
	*password = "";

	if (creds && PyDict_Size(creds) > 0) {
		PyObject *username_obj, *password_obj, *domain_obj;
		PyObject *key, *value;
		int i;

		/* Check for presence of required fields */

		username_obj = PyDict_GetItemString(creds, "username");
		domain_obj = PyDict_GetItemString(creds, "domain");
		password_obj = PyDict_GetItemString(creds, "password");

		if (!username_obj) {
			*errstr = SMB_STRDUP("no username field in credential");
			return False;
		}

		if (!domain_obj) {
			*errstr = SMB_STRDUP("no domain field in credential");
			return False;
		}

		if (!password_obj) {
			*errstr = SMB_STRDUP("no password field in credential");
			return False;
		}

		/* Check type of required fields */

		if (!PyString_Check(username_obj)) {
			*errstr = SMB_STRDUP("username field is not string type");
			return False;
		}

		if (!PyString_Check(domain_obj)) {
			*errstr = SMB_STRDUP("domain field is not string type");
			return False;
		}

		if (!PyString_Check(password_obj)) {
			*errstr = SMB_STRDUP("password field is not string type");
			return False;
		}

		/* Look for any extra fields */

		i = 0;

		while (PyDict_Next(creds, &i, &key, &value)) {
			if (strcmp(PyString_AsString(key), "domain") != 0 &&
			    strcmp(PyString_AsString(key), "username") != 0 &&
			    strcmp(PyString_AsString(key), "password") != 0) {
				asprintf(errstr,
					 "creds contain extra field '%s'",
					 PyString_AsString(key));
				return False;
			}
		}

		/* Assign values */

		*username = PyString_AsString(username_obj);
		*domain = PyString_AsString(domain_obj);
		*password = PyString_AsString(password_obj);
	}

	*errstr = NULL;

	return True;
}

/* Return a cli_state to a RPC pipe on the given server.  Use the
   credentials passed if not NULL.  If an error occurs errstr is set to a
   string describing the error and NULL is returned.  If set, errstr must
   be freed by calling free(). */

struct cli_state *open_pipe_creds(char *server, PyObject *creds, 
				  int pipe_idx, char **errstr)
{
	char *username, *password, *domain;
	struct cli_state *cli;
	struct rpc_pipe_client *pipe_hnd;
	NTSTATUS result;
	
	/* Extract credentials from the python dictionary */

	if (!py_parse_creds(creds, &username, &domain, &password, errstr))
		return NULL;

	/* Now try to connect */

	result = cli_full_connection(
		&cli, NULL, server, NULL, 0, "IPC$", "IPC",
		username, domain, password, 0, Undefined, NULL);
	
	if (!NT_STATUS_IS_OK(result)) {
		*errstr = SMB_STRDUP("error connecting to IPC$ pipe");
		return NULL;
	}

	pipe_hnd = cli_rpc_pipe_open_noauth(cli, pipe_idx, &result);
	if (!pipe_hnd) {
		cli_shutdown(cli);
		asprintf(errstr, "error opening pipe index %d", pipe_idx);
		return NULL;
	}

	*errstr = NULL;

	return cli;
}

/* Return true if a dictionary contains a "level" key with an integer
   value.  Set the value if so. */

BOOL get_level_value(PyObject *dict, uint32 *level)
{
	PyObject *obj;

	if (!(obj = PyDict_GetItemString(dict, "level")) ||
	    !PyInt_Check(obj))
		return False;

	if (level)
		*level = PyInt_AsLong(obj);

	return True;
}
