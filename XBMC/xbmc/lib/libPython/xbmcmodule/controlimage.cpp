/*	PyObject* ControlImage_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
			// check if filename exists

*/
#include "..\python.h"
#include "..\..\..\application.h"
#include "GuiImage.h"
#include <vector>
#include "control.h"

using namespace std;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

/*****************************************************************
 * start of window methods and python objects
 *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	PyObject* ControlImage_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlImage *self;
		char *cImage = NULL;
		char *cColorKey = NULL;
		
		self = (ControlImage*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "lllls|s", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&cImage, &cColorKey)) return NULL;

		// check if filename exists
		self->strFileName = cImage;
		if (cColorKey) sscanf(cColorKey, "%x", &self->strColorKey);
		else self->strColorKey = 0;

		return (PyObject*)self;
	}

	PyMethodDef ControlImage_methods[] = {
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlImage_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlImage",    /*tp_name*/
			sizeof(ControlImage),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)Control_Dealloc,/*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			0,                         /*tp_compare*/
			0,                         /*tp_repr*/
			0,                         /*tp_as_number*/
			0,                         /*tp_as_sequence*/
			0,                         /*tp_as_mapping*/
			0,                         /*tp_hash */
			0,                         /*tp_call*/
			0,                         /*tp_str*/
			0,                         /*tp_getattro*/
			0,                         /*tp_setattro*/
			0,                         /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
			"ControlImage Objects",    /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlImage_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlImage_New,          /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
