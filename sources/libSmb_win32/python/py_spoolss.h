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

#ifndef _PY_SPOOLSS_H
#define _PY_SPOOLSS_H

#include "python/py_common.h"

/* Spoolss policy handle object */

typedef struct {
	PyObject_HEAD
	struct rpc_pipe_client *cli;
	TALLOC_CTX *mem_ctx;
	POLICY_HND pol;
} spoolss_policy_hnd_object;
     
/* Exceptions raised by this module */

extern PyTypeObject spoolss_policy_hnd_type;

extern PyObject *spoolss_error, *spoolss_werror;

/* The following definitions come from python/py_spoolss_common.c */

PyObject *new_spoolss_policy_hnd_object(struct cli_state *cli, 
					TALLOC_CTX *mem_ctx, POLICY_HND *pol);

/* The following definitions come from python/py_spoolss_drivers.c  */

PyObject *spoolss_enumprinterdrivers(PyObject *self, PyObject *args,
				     PyObject *kw);
PyObject *spoolss_hnd_getprinterdriver(PyObject *self, PyObject *args,
				   PyObject *kw);
PyObject *spoolss_getprinterdriverdir(PyObject *self, PyObject *args, 
				      PyObject *kw);
PyObject *spoolss_addprinterdriver(PyObject *self, PyObject *args,
				   PyObject *kw);
PyObject *spoolss_addprinterdriverex(PyObject *self, PyObject *args,
					     PyObject *kw);
PyObject *spoolss_deleteprinterdriver(PyObject *self, PyObject *args,
				      PyObject *kw);
PyObject *spoolss_deleteprinterdriverex(PyObject *self, PyObject *args,
					PyObject *kw);

/* The following definitions come from python/py_spoolss_drivers_conv.c  */

BOOL py_from_DRIVER_INFO_1(PyObject **dict, DRIVER_INFO_1 *info);
BOOL py_to_DRIVER_INFO_1(DRIVER_INFO_1 *info, PyObject *dict);
BOOL py_from_DRIVER_INFO_2(PyObject **dict, DRIVER_INFO_2 *info);
BOOL py_to_DRIVER_INFO_2(DRIVER_INFO_2 *info, PyObject *dict);
BOOL py_from_DRIVER_INFO_3(PyObject **dict, DRIVER_INFO_3 *info);
BOOL py_to_DRIVER_INFO_3(DRIVER_INFO_3 *info, PyObject *dict,
			 TALLOC_CTX *mem_ctx);
BOOL py_from_DRIVER_INFO_6(PyObject **dict, DRIVER_INFO_6 *info);
BOOL py_to_DRIVER_INFO_6(DRIVER_INFO_6 *info, PyObject *dict);
BOOL py_from_DRIVER_DIRECTORY_1(PyObject **dict, DRIVER_DIRECTORY_1 *info);
BOOL py_to_DRIVER_DIRECTORY_1(DRIVER_DIRECTORY_1 *info, PyObject *dict);

/* The following definitions come from python/py_spoolss_forms.c  */

PyObject *spoolss_hnd_addform(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_getform(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_setform(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_deleteform(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_enumforms(PyObject *self, PyObject *args, PyObject *kw);

/* The following definitions come from python/py_spoolss_forms_conv.c  */

BOOL py_from_FORM_1(PyObject **dict, FORM_1 *form);
BOOL py_to_FORM(FORM *form, PyObject *dict);

/* The following definitions come from python/py_spoolss_jobs.c  */

PyObject *spoolss_hnd_enumjobs(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_setjob(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_getjob(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_startpageprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_endpageprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_startdocprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_enddocprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_writeprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_addjob(PyObject *self, PyObject *args, PyObject *kw);

/* The following definitions come from python/py_spoolss_jobs_conv.c  */

BOOL py_from_JOB_INFO_1(PyObject **dict, JOB_INFO_1 *info);
BOOL py_to_JOB_INFO_1(JOB_INFO_1 *info, PyObject *dict);
BOOL py_from_JOB_INFO_2(PyObject **dict, JOB_INFO_2 *info);
BOOL py_to_JOB_INFO_2(JOB_INFO_2 *info, PyObject *dict);
BOOL py_from_DOC_INFO_1(PyObject **dict, DOC_INFO_1 *info);
BOOL py_to_DOC_INFO_1(DOC_INFO_1 *info, PyObject *dict);

/* The following definitions come from python/py_spoolss_ports.c  */

PyObject *spoolss_enumports(PyObject *self, PyObject *args, PyObject *kw);

/* The following definitions come from python/py_spoolss_ports_conv.c  */

BOOL py_from_PORT_INFO_1(PyObject **dict, PORT_INFO_1 *info);
BOOL py_to_PORT_INFO_1(PORT_INFO_1 *info, PyObject *dict);
BOOL py_from_PORT_INFO_2(PyObject **dict, PORT_INFO_2 *info);
BOOL py_to_PORT_INFO_2(PORT_INFO_2 *info, PyObject *dict);

/* The following definitions come from python/py_spoolss_printerdata.c  */

PyObject *spoolss_hnd_getprinterdata(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_setprinterdata(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_enumprinterdata(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_deleteprinterdata(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_getprinterdataex(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_setprinterdataex(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_enumprinterdataex(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_deleteprinterdataex(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_enumprinterkey(PyObject *self, PyObject *args,
				     PyObject *kw);
PyObject *spoolss_hnd_deleteprinterkey(PyObject *self, PyObject *args,
				       PyObject *kw);

/* The following definitions come from python/py_spoolss_printers.c  */

PyObject *spoolss_openprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_closeprinter(PyObject *self, PyObject *args);
PyObject *spoolss_hnd_getprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_hnd_setprinter(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_enumprinters(PyObject *self, PyObject *args, PyObject *kw);
PyObject *spoolss_addprinterex(PyObject *self, PyObject *args, PyObject *kw);

/* The following definitions come from python/py_spoolss_printers_conv.c  */

BOOL py_from_DEVICEMODE(PyObject **dict, DEVICEMODE *devmode);
BOOL py_to_DEVICEMODE(DEVICEMODE *devmode, PyObject *dict);
BOOL py_from_PRINTER_INFO_0(PyObject **dict, PRINTER_INFO_0 *info);
BOOL py_to_PRINTER_INFO_0(PRINTER_INFO_0 *info, PyObject *dict);
BOOL py_from_PRINTER_INFO_1(PyObject **dict, PRINTER_INFO_1 *info);
BOOL py_to_PRINTER_INFO_1(PRINTER_INFO_1 *info, PyObject *dict);
BOOL py_from_PRINTER_INFO_2(PyObject **dict, PRINTER_INFO_2 *info);
BOOL py_to_PRINTER_INFO_2(PRINTER_INFO_2 *info, PyObject *dict,
			  TALLOC_CTX *mem_ctx);
BOOL py_from_PRINTER_INFO_3(PyObject **dict, PRINTER_INFO_3 *info);
BOOL py_to_PRINTER_INFO_3(PRINTER_INFO_3 *info, PyObject *dict,
			  TALLOC_CTX *mem_ctx);

#endif /* _PY_SPOOLSS_H */
