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

#include "python/py_smb.h"

/* Create a new cli_state python object */

PyObject *new_cli_state_object(struct cli_state *cli)
{
	cli_state_object *o;

	o = PyObject_New(cli_state_object, &cli_state_type);

	o->cli = cli;

	return (PyObject*)o;
}

static PyObject *py_smb_connect(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = { "server", NULL };
	struct cli_state *cli;
	char *server;
	struct in_addr ip;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &server))
		return NULL;

	if (!(cli = cli_initialise(NULL)))
		return NULL;

	ZERO_STRUCT(ip);

	if (!cli_connect(cli, server, &ip))
		return NULL;

	return new_cli_state_object(cli);
}

static PyObject *py_smb_session_request(PyObject *self, PyObject *args,
					PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "called", "calling", NULL };
	char *calling_name = NULL, *called_name;
	struct nmb_name calling, called;
	BOOL result;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s|s", kwlist, &called_name, 
					 &calling_name))
		return NULL;

	if (!calling_name)
		calling_name = global_myname();

	make_nmb_name(&calling, calling_name, 0x00);
	make_nmb_name(&called, called_name, 0x20);

	result = cli_session_request(cli->cli, &calling, &called);

	return Py_BuildValue("i", result);
}
				      
static PyObject *py_smb_negprot(PyObject *self, PyObject *args, PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { NULL };
	BOOL result;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;

	result = cli_negprot(cli->cli);

	return Py_BuildValue("i", result);
}

static PyObject *py_smb_session_setup(PyObject *self, PyObject *args, 
				      PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "creds", NULL };
	PyObject *creds;
	char *username, *domain, *password, *errstr;
	BOOL result;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|O", kwlist, &creds))
		return NULL;

	if (!py_parse_creds(creds, &username, &domain, &password, &errstr)) {
		free(errstr);
		return NULL;
	}

	result = cli_session_setup(
		cli->cli, username, password, strlen(password) + 1,
		password, strlen(password) + 1, domain);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "session setup failed");
		return NULL;
	}

	return Py_BuildValue("i", result);
}

static PyObject *py_smb_tconx(PyObject *self, PyObject *args, PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "service", NULL };
	char *service;
	BOOL result;

	if (!PyArg_ParseTupleAndKeywords(args, kw, "s", kwlist, &service))
		return NULL;

	result = cli_send_tconX(
		cli->cli, service, strequal(service, "IPC$") ? "IPC" : 
		"?????", "", 1);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "tconx failed");
		return NULL;
	}

	return Py_BuildValue("i", result);
}

static PyObject *py_smb_nt_create_andx(PyObject *self, PyObject *args,
				       PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "filename", "desired_access", 
				  "file_attributes", "share_access",
				  "create_disposition", "create_options",
				  NULL };
	char *filename;
	uint32 desired_access, file_attributes = 0, 
		share_access = FILE_SHARE_READ | FILE_SHARE_WRITE,
		create_disposition = OPENX_FILE_EXISTS_OPEN, create_options = 0;
	int result;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "si|iiii", kwlist, &filename, &desired_access,
		    &file_attributes, &share_access, &create_disposition,
		    &create_options))
		return NULL;

	result = cli_nt_create_full(
		cli->cli, filename, 0, desired_access, file_attributes,
		share_access, create_disposition, create_options, 0);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "nt_create_andx failed");
		return NULL;
	}

	/* Return FID */

	return PyInt_FromLong(result);
}

static PyObject *py_smb_open(PyObject *self, PyObject *args, PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "filename", "flags", 
				  "share_mode", NULL };
	char *filename;
	uint32 flags, share_mode = DENY_NONE;
	int result;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "si|i", kwlist, &filename, &flags, &share_mode))
		return NULL;

	result = cli_open(cli->cli, filename, flags, share_mode);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "open failed");
		return NULL;
	}

	/* Return FID */

	return PyInt_FromLong(result);
}

static PyObject *py_smb_read(PyObject *self, PyObject *args, PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "fnum", "offset", "size", NULL };
	int fnum, offset=0, size=0;
	ssize_t result;
	SMB_OFF_T fsize;
	char *data;
	PyObject *ret;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "i|ii", kwlist, &fnum, &offset, &size))
		return NULL;

	if (!cli_qfileinfo(cli->cli, fnum, NULL, &fsize, NULL, NULL,
		    NULL, NULL, NULL) &&
	    !cli_getattrE(cli->cli, fnum, NULL, &fsize, NULL, NULL, NULL)) {
		PyErr_SetString(PyExc_RuntimeError, "getattrib failed");
		return NULL;
	}

	if (offset < 0)
		offset = 0;

	if (size < 1 || size > fsize - offset)
		size = fsize - offset;

	if (!(data = SMB_XMALLOC_ARRAY(char, size))) {
		PyErr_SetString(PyExc_RuntimeError, "malloc failed");
		return NULL;
	}

	result = cli_read(cli->cli, fnum, data, (off_t) offset, (size_t) size);

	if (result==-1 || cli_is_error(cli->cli)) {
		SAFE_FREE(data);
		PyErr_SetString(PyExc_RuntimeError, "read failed");
		return NULL;
	}

	/* Return a python string */

	ret = Py_BuildValue("s#", data, result);
	SAFE_FREE(data);

	return ret;
}

static PyObject *py_smb_close(PyObject *self, PyObject *args,
			      PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "fnum", NULL };
	BOOL result;
	int fnum;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "i", kwlist, &fnum))
		return NULL;

	result = cli_close(cli->cli, fnum);

	return PyInt_FromLong(result);
}

static PyObject *py_smb_unlink(PyObject *self, PyObject *args,
			       PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "filename", NULL };
	char *filename;
	BOOL result;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "s", kwlist, &filename))
		return NULL;

	result = cli_unlink(cli->cli, filename);

	return PyInt_FromLong(result);
}

static PyObject *py_smb_query_secdesc(PyObject *self, PyObject *args,
				      PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "fnum", NULL };
	PyObject *result = NULL;
	SEC_DESC *secdesc = NULL;
	int fnum;
	TALLOC_CTX *mem_ctx = NULL;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "i", kwlist, &fnum))
		return NULL;

	mem_ctx = talloc_init("py_smb_query_secdesc");

	secdesc = cli_query_secdesc(cli->cli, fnum, mem_ctx);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "query_secdesc failed");
		goto done;
	}

	if (!secdesc) {
		Py_INCREF(Py_None);
		result = Py_None;
		goto done;
	}

	if (!py_from_SECDESC(&result, secdesc)) {
		PyErr_SetString(
			PyExc_TypeError,
			"Invalid security descriptor returned");
		goto done;
	}

 done:
	talloc_destroy(mem_ctx);

	return result;
	
}

static PyObject *py_smb_set_secdesc(PyObject *self, PyObject *args,
				    PyObject *kw)
{
	cli_state_object *cli = (cli_state_object *)self;
	static char *kwlist[] = { "fnum", "security_descriptor", NULL };
	PyObject *result = NULL;
	PyObject *py_secdesc;
	SEC_DESC *secdesc;
	TALLOC_CTX *mem_ctx = NULL;
	int fnum;
	BOOL err;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "iO", kwlist, &fnum, &py_secdesc))
		return NULL;

	mem_ctx = talloc_init("py_smb_set_secdesc");

	if (!py_to_SECDESC(&secdesc, py_secdesc, mem_ctx)) {
		PyErr_SetString(PyExc_TypeError, 
				"Invalid security descriptor");
		goto done;
	}

	err = cli_set_secdesc(cli->cli, fnum, secdesc);

	if (cli_is_error(cli->cli)) {
		PyErr_SetString(PyExc_RuntimeError, "set_secdesc failed");
		goto done;
	}

	result =  PyInt_FromLong(err);
 done:
	talloc_destroy(mem_ctx);

	return result;
}

static PyMethodDef smb_hnd_methods[] = {

	/* Session and connection handling */

	{ "session_request", (PyCFunction)py_smb_session_request, 
	  METH_VARARGS | METH_KEYWORDS, "Request a session" },

	{ "negprot", (PyCFunction)py_smb_negprot, 
	  METH_VARARGS | METH_KEYWORDS, "Protocol negotiation" },

	{ "session_setup", (PyCFunction)py_smb_session_setup,
	  METH_VARARGS | METH_KEYWORDS, "Session setup" },

	{ "tconx", (PyCFunction)py_smb_tconx,
	  METH_VARARGS | METH_KEYWORDS, "Tree connect" },

	/* File operations */

	{ "nt_create_andx", (PyCFunction)py_smb_nt_create_andx,
	  METH_VARARGS | METH_KEYWORDS, "NT Create&X" },

	{ "open", (PyCFunction)py_smb_open,
	  METH_VARARGS | METH_KEYWORDS,
	  "Open a file\n"
"\n"
"This function returns a fnum handle to an open file.  The file is\n"
"opened with flags and optional share mode.  If unspecified, the\n"
"default share mode is DENY_NONE\n"
"\n"
"Example:\n"
"\n"
">>> fnum=conn.open(filename, os.O_RDONLY)" },

	{ "read", (PyCFunction)py_smb_read,
	  METH_VARARGS | METH_KEYWORDS,
	  "Read from an open file\n"
"\n"
"This function returns a string read from an open file starting at\n"
"offset for size bytes (until EOF is reached).  If unspecified, the\n"
"default offset is 0, and default size is the remainder of the file.\n"
"\n"
"Example:\n"
"\n"
">>> conn.read(fnum)           # read entire file\n"
">>> conn.read(fnum,5)         # read entire file from offset 5\n"
">>> conn.read(fnum,size=64)   # read 64 bytes from start of file\n"
">>> conn.read(fnum,4096,1024) # read 1024 bytes from offset 4096\n" },

	{ "close", (PyCFunction)py_smb_close,
	  METH_VARARGS | METH_KEYWORDS, "Close" },

	{ "unlink", (PyCFunction)py_smb_unlink,
	  METH_VARARGS | METH_KEYWORDS, "Unlink" },

	/* Security descriptors */

	{ "query_secdesc", (PyCFunction)py_smb_query_secdesc,
	  METH_VARARGS | METH_KEYWORDS, "Query security descriptor" },

	{ "set_secdesc", (PyCFunction)py_smb_set_secdesc,
	  METH_VARARGS | METH_KEYWORDS, "Set security descriptor" },

	{ NULL }
};

/*
 * Method dispatch tables
 */

static PyMethodDef smb_methods[] = {

	{ "connect", (PyCFunction)py_smb_connect, METH_VARARGS | METH_KEYWORDS,
	  "Connect to a host" },

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
">>> smb.setup_logging(interactive = 1)" },

	{ "get_debuglevel", (PyCFunction)get_debuglevel, 
	  METH_VARARGS, 
	  "Set the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> smb.get_debuglevel()\n"
"0" },

	{ "set_debuglevel", (PyCFunction)set_debuglevel, 
	  METH_VARARGS, 
	  "Get the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> smb.set_debuglevel(10)" },

	{ NULL }
};

static void py_cli_state_dealloc(PyObject* self)
{
	cli_state_object *cli = (cli_state_object *)self;

	if (cli->cli)
		cli_shutdown(cli->cli);

	PyObject_Del(self);
}

static PyObject *py_cli_state_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(smb_hnd_methods, self, attrname);
}

PyTypeObject cli_state_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"SMB client connection",
	sizeof(cli_state_object),
	0,
	py_cli_state_dealloc, /*tp_dealloc*/
	0,          /*tp_print*/
	py_cli_state_getattr,          /*tp_getattr*/
	0,          /*tp_setattr*/
	0,          /*tp_compare*/
	0,          /*tp_repr*/
	0,          /*tp_as_number*/
	0,          /*tp_as_sequence*/
	0,          /*tp_as_mapping*/
	0,          /*tp_hash */
};

/*
 * Module initialisation 
 */

void initsmb(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("smb", smb_methods);
	dict = PyModule_GetDict(module);

	/* Initialise policy handle object */

	cli_state_type.ob_type = &PyType_Type;

	/* Do samba initialisation */

	py_samba_init();

	setup_logging("smb", True);
	DEBUGLEVEL = 3;
}
