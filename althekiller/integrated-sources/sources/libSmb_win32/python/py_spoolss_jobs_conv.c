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
#include "python/py_conv.h"

struct pyconv py_JOB_INFO_1[] = {
	{ "jobid", PY_UINT32, offsetof(JOB_INFO_1, jobid) },
	{ "printer_name", PY_UNISTR, offsetof(JOB_INFO_1, printername) },
	{ "server_name", PY_UNISTR, offsetof(JOB_INFO_1, machinename) },
	{ "user_name", PY_UNISTR, offsetof(JOB_INFO_1, username) },
	{ "document_name", PY_UNISTR, offsetof(JOB_INFO_1, document) },
	{ "data_type", PY_UNISTR, offsetof(JOB_INFO_1, datatype) },
	{ "text_status", PY_UNISTR, offsetof(JOB_INFO_1, text_status) },
	{ "status", PY_UINT32, offsetof(JOB_INFO_1, status) },
	{ "priority", PY_UINT32, offsetof(JOB_INFO_1, priority) },
	{ "position", PY_UINT32, offsetof(JOB_INFO_1, position) },
	{ "total_pages", PY_UINT32, offsetof(JOB_INFO_1, totalpages) },
	{ "pages_printed", PY_UINT32, offsetof(JOB_INFO_1, pagesprinted) },
	{ NULL }
};

struct pyconv py_JOB_INFO_2[] = {
	{ "jobid", PY_UINT32, offsetof(JOB_INFO_2, jobid) },
	{ "printer_name", PY_UNISTR, offsetof(JOB_INFO_2, printername) },
	{ "server_name", PY_UNISTR, offsetof(JOB_INFO_2, machinename) },
	{ "user_name", PY_UNISTR, offsetof(JOB_INFO_2, username) },
	{ "document_name", PY_UNISTR, offsetof(JOB_INFO_2, document) },
	{ "notify_name", PY_UNISTR, offsetof(JOB_INFO_2, notifyname) },
	{ "data_type", PY_UNISTR, offsetof(JOB_INFO_2, datatype) },
	{ "print_processor", PY_UNISTR, offsetof(JOB_INFO_2, printprocessor) },
	{ "parameters", PY_UNISTR, offsetof(JOB_INFO_2, parameters) },
	{ "driver_name", PY_UNISTR, offsetof(JOB_INFO_2, drivername) },
	{ "text_status", PY_UNISTR, offsetof(JOB_INFO_2, text_status) },
	{ "status", PY_UINT32, offsetof(JOB_INFO_2, status) },
	{ "priority", PY_UINT32, offsetof(JOB_INFO_2, priority) },
	{ "position", PY_UINT32, offsetof(JOB_INFO_2, position) },
	{ "start_time", PY_UINT32, offsetof(JOB_INFO_2, starttime) },
	{ "until_time", PY_UINT32, offsetof(JOB_INFO_2, untiltime) },
	{ "total_pages", PY_UINT32, offsetof(JOB_INFO_2, totalpages) },
	{ "size", PY_UINT32, offsetof(JOB_INFO_2, size) },
	{ "time_elapsed", PY_UINT32, offsetof(JOB_INFO_2, timeelapsed) },
	{ "pages_printed", PY_UINT32, offsetof(JOB_INFO_2, pagesprinted) },
	{ NULL }
};

struct pyconv py_DOC_INFO_1[] = {
	{ "document_name", PY_UNISTR, offsetof(DOC_INFO_1, docname) },
	{ "output_file", PY_UNISTR, offsetof(DOC_INFO_1, outputfile) },
	{ "data_type", PY_UNISTR, offsetof(DOC_INFO_1, datatype) },
	{ NULL }
};

BOOL py_from_JOB_INFO_1(PyObject **dict, JOB_INFO_1 *info)
{
	*dict = from_struct(info, py_JOB_INFO_1);
	return True;
}

BOOL py_to_JOB_INFO_1(JOB_INFO_1 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_JOB_INFO_2(PyObject **dict, JOB_INFO_2 *info)
{
	*dict = from_struct(info, py_JOB_INFO_2);
	return True;
}

BOOL py_to_JOB_INFO_2(JOB_INFO_2 *info, PyObject *dict)
{
	return False;
}

BOOL py_from_DOC_INFO_1(PyObject **dict, DOC_INFO_1 *info)
{
	*dict = from_struct(info, py_DOC_INFO_1);
	return True;
}

BOOL py_to_DOC_INFO_1(DOC_INFO_1 *info, PyObject *dict)
{
	return to_struct(info, dict, py_DOC_INFO_1);
}
