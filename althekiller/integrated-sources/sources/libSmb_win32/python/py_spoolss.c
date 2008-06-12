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

/* Exceptions this module can raise */

PyObject *spoolss_error, *spoolss_werror;

/* 
 * Method dispatch table
 */

static PyMethodDef spoolss_methods[] = {

	/* Open/close printer handles */
	
	{ "openprinter", (PyCFunction)spoolss_openprinter, METH_VARARGS | METH_KEYWORDS, 
	  "Open a printer by name in UNC format.\n"
"\n"
"Optionally a dictionary of (domain, username, password) may be given in\n"
"which case they are used when opening the RPC pipe.  An access mask may\n"
"also be given which defaults to MAXIMUM_ALLOWED_ACCESS.\n"
"\n"
"Example:\n"
"\n"
">>> hnd = spoolss.openprinter(\"\\\\\\\\NPSD-PDC2\\\\meanie\")"},
	
	{ "closeprinter", spoolss_closeprinter, METH_VARARGS, 
	  "Close a printer handle opened with openprinter or addprinter.\n"
"\n"
"Example:\n"
"\n"
">>> spoolss.closeprinter(hnd)"},

	{ "addprinterex", (PyCFunction)spoolss_addprinterex, METH_VARARGS, 
	  "addprinterex()"},

	/* Server enumeratation functions */

	{ "enumprinters", (PyCFunction)spoolss_enumprinters, 
	  METH_VARARGS | METH_KEYWORDS,
	  "Enumerate printers on a print server.\n"
"\n"
"Return a list of printers on a print server.  The credentials, info level\n"
"and flags may be specified as keyword arguments.\n"
"\n"
"Example:\n"
"\n"
">>> print spoolss.enumprinters(\"\\\\\\\\npsd-pdc2\")\n"
"[{'comment': 'i am a comment', 'printer_name': 'meanie', 'flags': 8388608, \n"
"  'description': 'meanie,Generic / Text Only,i am a location'}, \n"
" {'comment': '', 'printer_name': 'fileprint', 'flags': 8388608, \n"
"  'description': 'fileprint,Generic / Text Only,'}]"},

	{ "enumports", (PyCFunction)spoolss_enumports, 
	  METH_VARARGS | METH_KEYWORDS,
	  "Enumerate ports on a print server.\n"
"\n"
"Return a list of ports on a print server.\n"
"\n"
"Example:\n"
"\n"
">>> print spoolss.enumports(\"\\\\\\\\npsd-pdc2\")\n"
"[{'name': 'LPT1:'}, {'name': 'LPT2:'}, {'name': 'COM1:'}, \n"
"{'name': 'COM2:'}, {'name': 'FILE:'}, {'name': '\\\\nautilus1\\zpekt3r'}]"},

	{ "enumprinterdrivers", (PyCFunction)spoolss_enumprinterdrivers, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Enumerate printer drivers on a print server.\n"
"\n"
"Return a list of printer drivers."},

	/* Miscellaneous other commands */

	{ "getprinterdriverdir", (PyCFunction)spoolss_getprinterdriverdir, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Return printer driver directory.\n"
"\n"
"Return the printer driver directory for a given architecture.  The\n"
"architecture defaults to \"Windows NT x86\"."},

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
">>> spoolss.setup_logging(interactive = 1)" },

	{ "get_debuglevel", (PyCFunction)get_debuglevel, 
	  METH_VARARGS, 
	  "Set the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> spoolss.get_debuglevel()\n"
"0" },

	{ "set_debuglevel", (PyCFunction)set_debuglevel, 
	  METH_VARARGS, 
	  "Get the current debug level.\n"
"\n"
"Example:\n"
"\n"
">>> spoolss.set_debuglevel(10)" },

	/* Printer driver routines */
	
	{ "addprinterdriver", (PyCFunction)spoolss_addprinterdriver, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Add a printer driver." },

	{ "addprinterdriverex", (PyCFunction)spoolss_addprinterdriverex, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Add a printer driver." },

	{ "deleteprinterdriver", (PyCFunction)spoolss_deleteprinterdriver, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Delete a printer driver." },

	{ "deleteprinterdriverex", (PyCFunction)spoolss_deleteprinterdriverex, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Delete a printer driver." },

	{ NULL }
};

/* Methods attached to a spoolss handle object */

static PyMethodDef spoolss_hnd_methods[] = {

	/* Printer info */

	{ "getprinter", (PyCFunction)spoolss_hnd_getprinter, 
           METH_VARARGS | METH_KEYWORDS,
	  "Get printer information.\n"
"\n"
"Return a dictionary of print information.  The info level defaults to 1.\n"
"\n"
"Example:\n"
"\n"
">>> hnd.getprinter()\n"
"{'comment': 'i am a comment', 'printer_name': '\\\\NPSD-PDC2\\meanie',\n"
" 'description': '\\\\NPSD-PDC2\\meanie,Generic / Text Only,i am a location',\n"
" 'flags': 8388608}"},

	{ "setprinter", (PyCFunction)spoolss_hnd_setprinter, 
          METH_VARARGS | METH_KEYWORDS,
	  "Set printer information."},

	/* Printer drivers */

	{ "getprinterdriver", (PyCFunction)spoolss_hnd_getprinterdriver, 
	  METH_VARARGS | METH_KEYWORDS, 
	  "Return printer driver information.\n"
"\n"
"Return a dictionary of printer driver information for the printer driver\n"
"bound to this printer."},

	/* Forms */

	{ "enumforms", (PyCFunction)spoolss_hnd_enumforms, 
          METH_VARARGS | METH_KEYWORDS,
	  "Enumerate supported forms.\n"
"\n"
"Return a list of forms supported by this printer or print server."},

	{ "setform", (PyCFunction)spoolss_hnd_setform, 
          METH_VARARGS | METH_KEYWORDS,
	  "Set form data.\n"
"\n"
"Set the form given by the dictionary argument."},

	{ "addform", (PyCFunction)spoolss_hnd_addform, 
          METH_VARARGS | METH_KEYWORDS,
	  "Add a new form." },

	{ "getform", (PyCFunction)spoolss_hnd_getform, 
          METH_VARARGS | METH_KEYWORDS,
	  "Get form properties." },

	{ "deleteform", (PyCFunction)spoolss_hnd_deleteform, 
          METH_VARARGS | METH_KEYWORDS,
	  "Delete a form." },

        /* Job related methods */

        { "enumjobs", (PyCFunction)spoolss_hnd_enumjobs, 
          METH_VARARGS | METH_KEYWORDS,
          "Enumerate jobs." },

        { "setjob", (PyCFunction)spoolss_hnd_setjob, 
          METH_VARARGS | METH_KEYWORDS,
          "Set job information." },

        { "getjob", (PyCFunction)spoolss_hnd_getjob, 
          METH_VARARGS | METH_KEYWORDS,
          "Get job information." },

        { "startpageprinter", (PyCFunction)spoolss_hnd_startpageprinter, 
           METH_VARARGS | METH_KEYWORDS,
          "Notify spooler that a page is about to be printed." },

        { "endpageprinter", (PyCFunction)spoolss_hnd_endpageprinter, 
           METH_VARARGS | METH_KEYWORDS,
          "Notify spooler that a page is about to be printed." },

        { "startdocprinter", (PyCFunction)spoolss_hnd_startdocprinter, 
           METH_VARARGS | METH_KEYWORDS,
          "Notify spooler that a document is about to be printed." },

        { "enddocprinter", (PyCFunction)spoolss_hnd_enddocprinter, 
           METH_VARARGS | METH_KEYWORDS,
          "Notify spooler that a document is about to be printed." },

        { "writeprinter", (PyCFunction)spoolss_hnd_writeprinter,
          METH_VARARGS | METH_KEYWORDS,
          "Write job data to a printer." },

        { "addjob", (PyCFunction)spoolss_hnd_addjob,
          METH_VARARGS | METH_KEYWORDS,
          "Add a job to the list of print jobs." },

        /* Printer data */

        { "getprinterdata", (PyCFunction)spoolss_hnd_getprinterdata,
           METH_VARARGS | METH_KEYWORDS,
          "Get printer data." },

        { "setprinterdata", (PyCFunction)spoolss_hnd_setprinterdata,
           METH_VARARGS | METH_KEYWORDS,
          "Set printer data." },

        { "enumprinterdata", (PyCFunction)spoolss_hnd_enumprinterdata,
           METH_VARARGS | METH_KEYWORDS,
          "Enumerate printer data." },

        { "deleteprinterdata", (PyCFunction)spoolss_hnd_deleteprinterdata,
           METH_VARARGS | METH_KEYWORDS,
          "Delete printer data." },

        { "getprinterdataex", (PyCFunction)spoolss_hnd_getprinterdataex,
           METH_VARARGS | METH_KEYWORDS,
          "Get printer data." },

        { "setprinterdataex", (PyCFunction)spoolss_hnd_setprinterdataex,
           METH_VARARGS | METH_KEYWORDS,
          "Set printer data." },

        { "enumprinterdataex", (PyCFunction)spoolss_hnd_enumprinterdataex,
           METH_VARARGS | METH_KEYWORDS,
          "Enumerate printer data." },

        { "deleteprinterdataex", (PyCFunction)spoolss_hnd_deleteprinterdataex,
           METH_VARARGS | METH_KEYWORDS,
          "Delete printer data." },

        { "enumprinterkey", (PyCFunction)spoolss_hnd_enumprinterkey,
           METH_VARARGS | METH_KEYWORDS,
          "Enumerate printer key." },

#if 0
        /* Not implemented */

        { "deleteprinterkey", (PyCFunction)spoolss_hnd_deleteprinterkey,
           METH_VARARGS | METH_KEYWORDS,
          "Delete printer key." },
#endif

	{ NULL }

};

static void py_policy_hnd_dealloc(PyObject* self)
{
        spoolss_policy_hnd_object *hnd;

        /* Close down policy handle and free talloc context */

        hnd = (spoolss_policy_hnd_object*)self;

        cli_shutdown(hnd->cli);
        talloc_destroy(hnd->mem_ctx);

	PyObject_Del(self);
}

static PyObject *py_policy_hnd_getattr(PyObject *self, char *attrname)
{
	return Py_FindMethod(spoolss_hnd_methods, self, attrname);
}

static char spoolss_type_doc[] = 
"Python wrapper for Windows NT SPOOLSS rpc pipe.";

PyTypeObject spoolss_policy_hnd_type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"spoolss.hnd",
	sizeof(spoolss_policy_hnd_object),
	0,
	py_policy_hnd_dealloc,	/* tp_dealloc*/
	0,			/* tp_print*/
	py_policy_hnd_getattr,	/* tp_getattr*/
	0,			/* tp_setattr*/
	0,			/* tp_compare*/
	0,			/* tp_repr*/
	0,			/* tp_as_number*/
	0,			/* tp_as_sequence*/
	0,			/* tp_as_mapping*/
	0,			/* tp_hash */
	0,			/* tp_call */
	0,			/* tp_str */
	0,			/* tp_getattro */
	0,			/* tp_setattro */
	0,			/* tp_as_buffer*/
	Py_TPFLAGS_DEFAULT,	/* tp_flags */
	spoolss_type_doc,	/* tp_doc */
};

/* Initialise constants */

static struct const_vals {
	char *name;
	uint32 value;
} module_const_vals[] = {
	
	/* Access permissions */

	{ "MAXIMUM_ALLOWED_ACCESS", MAXIMUM_ALLOWED_ACCESS },
	{ "SERVER_ALL_ACCESS", SERVER_ALL_ACCESS },
	{ "SERVER_READ", SERVER_READ },
	{ "SERVER_WRITE", SERVER_WRITE },
	{ "SERVER_EXECUTE", SERVER_EXECUTE },
	{ "SERVER_ACCESS_ADMINISTER", SERVER_ACCESS_ADMINISTER },
	{ "SERVER_ACCESS_ENUMERATE", SERVER_ACCESS_ENUMERATE },
	{ "PRINTER_ALL_ACCESS", PRINTER_ALL_ACCESS },
	{ "PRINTER_READ", PRINTER_READ },
	{ "PRINTER_WRITE", PRINTER_WRITE },
	{ "PRINTER_EXECUTE", PRINTER_EXECUTE },
	{ "PRINTER_ACCESS_ADMINISTER", PRINTER_ACCESS_ADMINISTER },
	{ "PRINTER_ACCESS_USE", PRINTER_ACCESS_USE },
	{ "JOB_ACCESS_ADMINISTER", JOB_ACCESS_ADMINISTER },
	{ "JOB_ALL_ACCESS", JOB_ALL_ACCESS },
	{ "JOB_READ", JOB_READ },
	{ "JOB_WRITE", JOB_WRITE },
	{ "JOB_EXECUTE", JOB_EXECUTE },
	{ "STANDARD_RIGHTS_ALL_ACCESS", STANDARD_RIGHTS_ALL_ACCESS },
	{ "STANDARD_RIGHTS_EXECUTE_ACCESS", STANDARD_RIGHTS_EXECUTE_ACCESS },
	{ "STANDARD_RIGHTS_READ_ACCESS", STANDARD_RIGHTS_READ_ACCESS },
	{ "STANDARD_RIGHTS_REQUIRED_ACCESS", STANDARD_RIGHTS_REQUIRED_ACCESS },
	{ "STANDARD_RIGHTS_WRITE_ACCESS", STANDARD_RIGHTS_WRITE_ACCESS },

	/* Printer enumeration flags */

	{ "PRINTER_ENUM_DEFAULT", PRINTER_ENUM_DEFAULT },
	{ "PRINTER_ENUM_LOCAL", PRINTER_ENUM_LOCAL },
	{ "PRINTER_ENUM_CONNECTIONS", PRINTER_ENUM_CONNECTIONS },
	{ "PRINTER_ENUM_FAVORITE", PRINTER_ENUM_FAVORITE },
	{ "PRINTER_ENUM_NAME", PRINTER_ENUM_NAME },
	{ "PRINTER_ENUM_REMOTE", PRINTER_ENUM_REMOTE },
	{ "PRINTER_ENUM_SHARED", PRINTER_ENUM_SHARED },
	{ "PRINTER_ENUM_NETWORK", PRINTER_ENUM_NETWORK },

	/* Form types */

	{ "FORM_USER", FORM_USER },
	{ "FORM_BUILTIN", FORM_BUILTIN },
	{ "FORM_PRINTER", FORM_PRINTER },

	/* WERRORs */

	{ "WERR_OK", 0 },
	{ "WERR_BADFILE", 2 },
	{ "WERR_ACCESS_DENIED", 5 },
	{ "WERR_BADFID", 6 },
	{ "WERR_BADFUNC", 1 },
	{ "WERR_INSUFFICIENT_BUFFER", 122 },
	{ "WERR_NO_SUCH_SHARE", 67 },
	{ "WERR_ALREADY_EXISTS", 80 },
	{ "WERR_INVALID_PARAM", 87 },
	{ "WERR_NOT_SUPPORTED", 50 },
	{ "WERR_BAD_PASSWORD", 86 },
	{ "WERR_NOMEM", 8 },
	{ "WERR_INVALID_NAME", 123 },
	{ "WERR_UNKNOWN_LEVEL", 124 },
	{ "WERR_OBJECT_PATH_INVALID", 161 },
	{ "WERR_NO_MORE_ITEMS", 259 },
	{ "WERR_MORE_DATA", 234 },
	{ "WERR_UNKNOWN_PRINTER_DRIVER", 1797 },
	{ "WERR_INVALID_PRINTER_NAME", 1801 },
	{ "WERR_PRINTER_ALREADY_EXISTS", 1802 },
	{ "WERR_INVALID_DATATYPE", 1804 },
	{ "WERR_INVALID_ENVIRONMENT", 1805 },
	{ "WERR_INVALID_FORM_NAME", 1902 },
	{ "WERR_INVALID_FORM_SIZE", 1903 },
	{ "WERR_BUF_TOO_SMALL", 2123 },
	{ "WERR_JOB_NOT_FOUND", 2151 },
	{ "WERR_DEST_NOT_FOUND", 2152 },
	{ "WERR_NOT_LOCAL_DOMAIN", 2320 },
	{ "WERR_PRINTER_DRIVER_IN_USE", 3001 },
	{ "WERR_STATUS_MORE_ENTRIES  ", 0x0105 },

	/* Job control constants */

	{ "JOB_CONTROL_PAUSE", JOB_CONTROL_PAUSE },
	{ "JOB_CONTROL_RESUME", JOB_CONTROL_RESUME },
	{ "JOB_CONTROL_CANCEL", JOB_CONTROL_CANCEL },
	{ "JOB_CONTROL_RESTART", JOB_CONTROL_RESTART },
	{ "JOB_CONTROL_DELETE", JOB_CONTROL_DELETE },

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

/* Module initialisation */

void initspoolss(void)
{
	PyObject *module, *dict;

	/* Initialise module */

	module = Py_InitModule("spoolss", spoolss_methods);
	dict = PyModule_GetDict(module);

	/* Exceptions we can raise */

	spoolss_error = PyErr_NewException("spoolss.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", spoolss_error);

	spoolss_werror = PyErr_NewException("spoolss.werror", NULL, NULL);
	PyDict_SetItemString(dict, "werror", spoolss_werror);

	/* Initialise policy handle object */

	spoolss_policy_hnd_type.ob_type = &PyType_Type;

	PyDict_SetItemString(dict, "spoolss.hnd", 
			     (PyObject *)&spoolss_policy_hnd_type);

	/* Initialise constants */

	const_init(dict);

	/* Do samba initialisation */

	py_samba_init();
}
