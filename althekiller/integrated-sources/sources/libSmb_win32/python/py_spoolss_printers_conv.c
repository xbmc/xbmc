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

struct pyconv py_PRINTER_INFO_0[] = {
	{ "name", PY_UNISTR, offsetof(PRINTER_INFO_0, printername) },
	{ "server_name", PY_UNISTR, offsetof(PRINTER_INFO_0, servername) },

	{ "cjobs", PY_UINT32, offsetof(PRINTER_INFO_0, cjobs) },
	{ "total_jobs", PY_UINT32, offsetof(PRINTER_INFO_0, total_jobs) },
	{ "total_bytes", PY_UINT32, offsetof(PRINTER_INFO_0, total_bytes) },

	{ "year", PY_UINT16, offsetof(PRINTER_INFO_0, year) },
	{ "month", PY_UINT16, offsetof(PRINTER_INFO_0, month) },
	{ "day_of_week", PY_UINT16, offsetof(PRINTER_INFO_0, dayofweek) },
	{ "day", PY_UINT16, offsetof(PRINTER_INFO_0, day) },
	{ "hour", PY_UINT16, offsetof(PRINTER_INFO_0, hour) },
	{ "minute", PY_UINT16, offsetof(PRINTER_INFO_0, minute) },
	{ "second", PY_UINT16, offsetof(PRINTER_INFO_0, second) },
	{ "milliseconds", PY_UINT16, offsetof(PRINTER_INFO_0, milliseconds) },

	{ "global_counter", PY_UINT32, offsetof(PRINTER_INFO_0, global_counter) },
	{ "total_pages", PY_UINT32, offsetof(PRINTER_INFO_0, total_pages) },

	{ "major_version", PY_UINT16, offsetof(PRINTER_INFO_0, major_version) },
	{ "build_version", PY_UINT16, offsetof(PRINTER_INFO_0, build_version) },

	{ "unknown7", PY_UINT32, offsetof(PRINTER_INFO_0, unknown7) },
	{ "unknown8", PY_UINT32, offsetof(PRINTER_INFO_0, unknown8) },
	{ "unknown9", PY_UINT32, offsetof(PRINTER_INFO_0, unknown9) },
	{ "session_counter", PY_UINT32, offsetof(PRINTER_INFO_0, session_counter)},
	{ "unknown11", PY_UINT32, offsetof(PRINTER_INFO_0, unknown11) },
	{ "printer_errors", PY_UINT32, offsetof(PRINTER_INFO_0, printer_errors) },
	{ "unknown13", PY_UINT32, offsetof(PRINTER_INFO_0, unknown13) },
	{ "unknown14", PY_UINT32, offsetof(PRINTER_INFO_0, unknown14) },
	{ "unknown15", PY_UINT32, offsetof(PRINTER_INFO_0, unknown15) },
	{ "unknown16", PY_UINT32, offsetof(PRINTER_INFO_0, unknown16) },
	{ "change_id", PY_UINT32, offsetof(PRINTER_INFO_0, change_id) },
	{ "unknown18", PY_UINT32, offsetof(PRINTER_INFO_0, unknown18) },
	{ "status", PY_UINT32, offsetof(PRINTER_INFO_0, status) },
	{ "unknown20", PY_UINT32, offsetof(PRINTER_INFO_0, unknown20) },
	{ "c_setprinter", PY_UINT32, offsetof(PRINTER_INFO_0, c_setprinter) },
	{ "unknown22", PY_UINT32, offsetof(PRINTER_INFO_0, unknown22) },
	{ "unknown23", PY_UINT32, offsetof(PRINTER_INFO_0, unknown23) },
	{ "unknown24", PY_UINT32, offsetof(PRINTER_INFO_0, unknown24) },
	{ "unknown25", PY_UINT32, offsetof(PRINTER_INFO_0, unknown25) },
	{ "unknown26", PY_UINT32, offsetof(PRINTER_INFO_0, unknown26) },
	{ "unknown27", PY_UINT32, offsetof(PRINTER_INFO_0, unknown27) },
	{ "unknown28", PY_UINT32, offsetof(PRINTER_INFO_0, unknown28) },
	{ "unknown29", PY_UINT32, offsetof(PRINTER_INFO_0, unknown29) },

	{ NULL }
};	

struct pyconv py_PRINTER_INFO_1[] = {
	{ "name", PY_UNISTR, offsetof(PRINTER_INFO_1, name) },
	{ "description", PY_UNISTR, offsetof(PRINTER_INFO_1, description) },
	{ "comment", PY_UNISTR, offsetof(PRINTER_INFO_1, comment) },
	{ "flags", PY_UINT32, offsetof(PRINTER_INFO_1, flags) },
	{ NULL }
};	

struct pyconv py_PRINTER_INFO_2[] = {
	{ "server_name", PY_UNISTR, offsetof(PRINTER_INFO_2, servername) },
	{ "name", PY_UNISTR, offsetof(PRINTER_INFO_2, printername) },
	{ "share_name", PY_UNISTR, offsetof(PRINTER_INFO_2, sharename) },
	{ "port_name", PY_UNISTR, offsetof(PRINTER_INFO_2, portname) },
	{ "driver_name", PY_UNISTR, offsetof(PRINTER_INFO_2, drivername) },
	{ "comment", PY_UNISTR, offsetof(PRINTER_INFO_2, comment) },
	{ "location", PY_UNISTR, offsetof(PRINTER_INFO_2, location) },
	{ "datatype", PY_UNISTR, offsetof(PRINTER_INFO_2, datatype) },
	{ "sepfile", PY_UNISTR, offsetof(PRINTER_INFO_2, sepfile) },
	{ "print_processor", PY_UNISTR, offsetof(PRINTER_INFO_2, printprocessor) },
	{ "parameters", PY_UNISTR, offsetof(PRINTER_INFO_2, parameters) },
	{ "attributes", PY_UINT32, offsetof(PRINTER_INFO_2, attributes) },
	{ "default_priority", PY_UINT32, offsetof(PRINTER_INFO_2, defaultpriority) },
	{ "priority", PY_UINT32, offsetof(PRINTER_INFO_2, priority) },
	{ "start_time", PY_UINT32, offsetof(PRINTER_INFO_2, starttime) },
	{ "until_time", PY_UINT32, offsetof(PRINTER_INFO_2, untiltime) },
	{ "status", PY_UINT32, offsetof(PRINTER_INFO_2, status) },
	{ "cjobs", PY_UINT32, offsetof(PRINTER_INFO_2, cjobs) },
	{ "average_ppm", PY_UINT32, offsetof(PRINTER_INFO_2, averageppm) },
	{ NULL }
};	

struct pyconv py_PRINTER_INFO_3[] = {
	{ "flags", PY_UINT32, offsetof(PRINTER_INFO_3, flags) },
	{ NULL }
};	

struct pyconv py_DEVICEMODE[] = {
	{ "device_name", PY_UNISTR, offsetof(DEVICEMODE, devicename) },
	{ "spec_version", PY_UINT16, offsetof(DEVICEMODE, specversion) },
	{ "driver_version", PY_UINT16, offsetof(DEVICEMODE, driverversion) },
	{ "size", PY_UINT16, offsetof(DEVICEMODE, size) },
	{ "fields", PY_UINT16, offsetof(DEVICEMODE, fields) },
	{ "orientation", PY_UINT16, offsetof(DEVICEMODE, orientation) },
	{ "paper_size", PY_UINT16, offsetof(DEVICEMODE, papersize) },
	{ "paper_width", PY_UINT16, offsetof(DEVICEMODE, paperwidth) },
	{ "paper_length", PY_UINT16, offsetof(DEVICEMODE, paperlength) },
	{ "scale", PY_UINT16, offsetof(DEVICEMODE, scale) },
	{ "copies", PY_UINT16, offsetof(DEVICEMODE, copies) },
	{ "default_source", PY_UINT16, offsetof(DEVICEMODE, defaultsource) },
	{ "print_quality", PY_UINT16, offsetof(DEVICEMODE, printquality) },
	{ "color", PY_UINT16, offsetof(DEVICEMODE, color) },
	{ "duplex", PY_UINT16, offsetof(DEVICEMODE, duplex) },
	{ "y_resolution", PY_UINT16, offsetof(DEVICEMODE, yresolution) },
	{ "tt_option", PY_UINT16, offsetof(DEVICEMODE, ttoption) },
	{ "collate", PY_UINT16, offsetof(DEVICEMODE, collate) },
	{ "form_name", PY_UNISTR, offsetof(DEVICEMODE, formname) },
	{ "log_pixels", PY_UINT16, offsetof(DEVICEMODE, logpixels) },
	{ "bits_per_pel", PY_UINT32, offsetof(DEVICEMODE, bitsperpel) },
	{ "pels_width", PY_UINT32, offsetof(DEVICEMODE, pelswidth) },
	{ "pels_height", PY_UINT32, offsetof(DEVICEMODE, pelsheight) },
	{ "display_flags", PY_UINT32, offsetof(DEVICEMODE, displayflags) },
	{ "display_frequency", PY_UINT32, offsetof(DEVICEMODE, displayfrequency) },
	{ "icm_method", PY_UINT32, offsetof(DEVICEMODE, icmmethod) },
	{ "icm_intent", PY_UINT32, offsetof(DEVICEMODE, icmintent) },
	{ "media_type", PY_UINT32, offsetof(DEVICEMODE, mediatype) },
	{ "dither_type", PY_UINT32, offsetof(DEVICEMODE, dithertype) },
	{ "reserved1", PY_UINT32, offsetof(DEVICEMODE, reserved1) },
	{ "reserved2", PY_UINT32, offsetof(DEVICEMODE, reserved2) },
	{ "panning_width", PY_UINT32, offsetof(DEVICEMODE, panningwidth) },
	{ "panning_height", PY_UINT32, offsetof(DEVICEMODE, panningheight) },
	{ NULL }
};

/*
 * Convert between DEVICEMODE and Python
 */

BOOL py_from_DEVICEMODE(PyObject **dict, DEVICEMODE *devmode)
{
	*dict = from_struct(devmode, py_DEVICEMODE);

	PyDict_SetItemString(*dict, "private",
			     PyString_FromStringAndSize(
				     devmode->dev_private, devmode->driverextra));

	return True;
}

BOOL py_to_DEVICEMODE(DEVICEMODE *devmode, PyObject *dict)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	if (!(obj = PyDict_GetItemString(dict_copy, "private")))
		goto done;

	if (!PyString_Check(obj))
		goto done;

	devmode->dev_private = PyString_AsString(obj);
	devmode->driverextra = PyString_Size(obj);

	PyDict_DelItemString(dict_copy, "private");

	if (!to_struct(devmode, dict_copy, py_DEVICEMODE))
		goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

/*
 * Convert between PRINTER_INFO_0 and Python 
 */

BOOL py_from_PRINTER_INFO_0(PyObject **dict, PRINTER_INFO_0 *info)
{
	*dict = from_struct(info, py_PRINTER_INFO_0);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(0));
	return True;
}

BOOL py_to_PRINTER_INFO_0(PRINTER_INFO_0 *info, PyObject *dict)
{
	return False;
}

/*
 * Convert between PRINTER_INFO_1 and Python 
 */

BOOL py_from_PRINTER_INFO_1(PyObject **dict, PRINTER_INFO_1 *info)
{
	*dict = from_struct(info, py_PRINTER_INFO_1);
	PyDict_SetItemString(*dict, "level", PyInt_FromLong(1));
	return True;
}

BOOL py_to_PRINTER_INFO_1(PRINTER_INFO_1 *info, PyObject *dict)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	if (!(obj = PyDict_GetItemString(dict_copy, "level")) ||
	    !PyInt_Check(obj))
		goto done;

	PyDict_DelItemString(dict_copy, "level");

	if (!to_struct(info, dict_copy, py_PRINTER_INFO_1))
		goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

/*
 * Convert between PRINTER_INFO_2 and Python 
 */

BOOL py_from_PRINTER_INFO_2(PyObject **dict, PRINTER_INFO_2 *info)
{
	PyObject *obj;

	*dict = from_struct(info, py_PRINTER_INFO_2);

	/* The security descriptor could be NULL */

	if (info->secdesc) {
		if (py_from_SECDESC(&obj, info->secdesc))
			PyDict_SetItemString(*dict, "security_descriptor", obj);
	}

	/* Bong!  The devmode could be NULL */

	if (info->devmode)
		py_from_DEVICEMODE(&obj, info->devmode);
	else
		obj = PyDict_New();

	PyDict_SetItemString(*dict, "device_mode", obj);

	PyDict_SetItemString(*dict, "level", PyInt_FromLong(2));

	return True;
}

BOOL py_to_PRINTER_INFO_2(PRINTER_INFO_2 *info, PyObject *dict,
			  TALLOC_CTX *mem_ctx)
{
	PyObject *obj, *dict_copy = PyDict_Copy(dict);
	BOOL result = False;

	/* Convert security descriptor - may be NULL */

	info->secdesc = NULL;

	if ((obj = PyDict_GetItemString(dict_copy, "security_descriptor"))) {

		if (!PyDict_Check(obj))
			goto done;

		if (!py_to_SECDESC(&info->secdesc, obj, mem_ctx))
			goto done;

		PyDict_DelItemString(dict_copy, "security_descriptor");
	}

	/* Convert device mode */

	if (!(obj = PyDict_GetItemString(dict_copy, "device_mode"))
	    || !PyDict_Check(obj))
		goto done;

	info->devmode = _talloc(mem_ctx, sizeof(DEVICEMODE));

	if (!py_to_DEVICEMODE(info->devmode, obj))
		goto done;

	PyDict_DelItemString(dict_copy, "device_mode");

	/* Check info level */

	if (!(obj = PyDict_GetItemString(dict_copy, "level")) ||
	    !PyInt_Check(obj))
		goto done;

	PyDict_DelItemString(dict_copy, "level");

	/* Convert remaining elements of dictionary */

	if (!to_struct(info, dict_copy, py_PRINTER_INFO_2))
		goto done;

	result = True;

done:
	Py_DECREF(dict_copy);
	return result;
}

/*
 * Convert between PRINTER_INFO_1 and Python 
 */

BOOL py_from_PRINTER_INFO_3(PyObject **dict, PRINTER_INFO_3 *info)
{
	PyObject *obj;	

	*dict = from_struct(info, py_PRINTER_INFO_3);

	if (py_from_SECDESC(&obj, info->secdesc))
		PyDict_SetItemString(*dict, "security_descriptor", obj);

	PyDict_SetItemString(*dict, "level", PyInt_FromLong(3));

	return True;
}

BOOL py_to_PRINTER_INFO_3(PRINTER_INFO_3 *info, PyObject *dict,
			  TALLOC_CTX *mem_ctx)
{
	PyObject *obj;

	if (!to_struct(info, dict, py_PRINTER_INFO_3))
		return False;

	if (!(obj = PyDict_GetItemString(dict, "security_descriptor")))
		return False;

	if (!py_to_SECDESC(&info->secdesc, obj, mem_ctx))
		return False;

	return True;
}
