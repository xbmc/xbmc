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

/* Enumerate jobs */

PyObject *spoolss_hnd_enumjobs(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *result;
	int level = 1;
	uint32 i, num_jobs;
	static char *kwlist[] = {"level", NULL};
	JOB_INFO_CTR ctr;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "|i", kwlist, &level))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_enumjobs(
		hnd->cli, hnd->mem_ctx, &hnd->pol, level, 0, 1000, 
		&num_jobs, &ctr);

	/* Return value */
	
	result = Py_None;

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		goto done;
	}

	result = PyList_New(num_jobs);

	switch (level) {
	case 1: 
		for (i = 0; i < num_jobs; i++) {
			PyObject *value;

			py_from_JOB_INFO_1(&value, &ctr.job.job_info_1[i]);

			PyList_SetItem(result, i, value);
		}

		break;
	case 2:
		for(i = 0; i < num_jobs; i++) {
			PyObject *value;

			py_from_JOB_INFO_2(&value, &ctr.job.job_info_2[i]);

			PyList_SetItem(result, i, value);
		}
		
		break;
	}

 done:
	Py_INCREF(result);
	return result;
}

/* Set job command */

PyObject *spoolss_hnd_setjob(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	uint32 level = 0, command, jobid;
	static char *kwlist[] = {"jobid", "command", "level", NULL};

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "ii|i", kwlist, &jobid, &command, &level))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_setjob(
		hnd->cli, hnd->mem_ctx, &hnd->pol, jobid, level, command);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* Get job */

PyObject *spoolss_hnd_getjob(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	PyObject *result;
	uint32 level = 1, jobid;
	static char *kwlist[] = {"jobid", "level", NULL};
	JOB_INFO_CTR ctr;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "i|i", kwlist, &jobid, &level))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_getjob(
		hnd->cli, hnd->mem_ctx, &hnd->pol, jobid, level, &ctr);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	switch(level) {
	case 1:
		py_from_JOB_INFO_1(&result, &ctr.job.job_info_1[0]);
		break;
	case 2:
		py_from_JOB_INFO_2(&result, &ctr.job.job_info_2[0]);
		break;
	}

	return result;
}

/* Start page printer.  This notifies the spooler that a page is about to be
   printed on the specified printer. */

PyObject *spoolss_hnd_startpageprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = { NULL };

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_startpageprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* End page printer.  This notifies the spooler that a page has finished
   being printed on the specified printer. */

PyObject *spoolss_hnd_endpageprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = { NULL };

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_endpageprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* Start doc printer.  This notifies the spooler that a document is about to be
   printed on the specified printer. */

PyObject *spoolss_hnd_startdocprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = { "document_info", NULL };
	PyObject *info, *obj;
	uint32 level, jobid;
	char *document_name = NULL, *output_file = NULL, *data_type = NULL;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyDict_Type, &info))
		return NULL;
	
	/* Check document_info parameter */

	if (!get_level_value(info, &level)) {
		PyErr_SetString(spoolss_error, "invalid info level");
		return NULL;
	}

	if (level != 1) {
		PyErr_SetString(spoolss_error, "unsupported info level");
		return NULL;
	}

	if ((obj = PyDict_GetItemString(info, "document_name"))) {

		if (!PyString_Check(obj) && obj != Py_None) {
			PyErr_SetString(spoolss_error,
					"document_name not a string");
			return NULL;
		}
		
		if (PyString_Check(obj))
			document_name = PyString_AsString(obj);

	} else {
		PyErr_SetString(spoolss_error, "no document_name present");
		return NULL;
	}

	if ((obj = PyDict_GetItemString(info, "output_file"))) {

		if (!PyString_Check(obj) && obj != Py_None) {
			PyErr_SetString(spoolss_error,
					"output_file not a string");
			return NULL;
		}
		
		if (PyString_Check(obj))
			output_file = PyString_AsString(obj);

	} else {
		PyErr_SetString(spoolss_error, "no output_file present");
		return NULL;
	}

	if ((obj = PyDict_GetItemString(info, "data_type"))) {
		
		if (!PyString_Check(obj) && obj != Py_None) {
			PyErr_SetString(spoolss_error,
					"data_type not a string");
			return NULL;
		}

		if (PyString_Check(obj))
			data_type = PyString_AsString(obj);

	} else {
		PyErr_SetString(spoolss_error, "no data_type present");
		return NULL;
	}

	/* Call rpc function */
	
	werror = rpccli_spoolss_startdocprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol, document_name,
		output_file, data_type, &jobid);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}
	
	/* The return value is zero for an error (where does the status
	   code come from now??) and the return value is the jobid
	   allocated for the new job. */

	return Py_BuildValue("i", jobid);
}

/* End doc printer.  This notifies the spooler that a document has finished
   being printed on the specified printer. */

PyObject *spoolss_hnd_enddocprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = { NULL };

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(args, kw, "", kwlist))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_enddocprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* Write data to a printer */

PyObject *spoolss_hnd_writeprinter(PyObject *self, PyObject *args, PyObject *kw)
{
	spoolss_policy_hnd_object *hnd = (spoolss_policy_hnd_object *)self;
	WERROR werror;
	static char *kwlist[] = { "data", NULL };
	PyObject *data;
	uint32 num_written;

	/* Parse parameters */

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "O!", kwlist, &PyString_Type, &data))
		return NULL;
	
	/* Call rpc function */
	
	werror = rpccli_spoolss_writeprinter(
		hnd->cli, hnd->mem_ctx, &hnd->pol, PyString_Size(data),
		PyString_AsString(data), &num_written);

	if (!W_ERROR_IS_OK(werror)) {
		PyErr_SetObject(spoolss_werror, py_werror_tuple(werror));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject *spoolss_hnd_addjob(PyObject *self, PyObject *args, PyObject *kw)
{
	PyErr_SetString(spoolss_error, "Not implemented");
	return NULL;
}
