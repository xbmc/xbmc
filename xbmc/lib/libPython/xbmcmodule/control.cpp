#include "..\..\..\application.h"
#include "GuiLabelControl.h"
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
	PyObject* Control_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		Control *self;

		self = (Control*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		self->iControlId = 0;
		self->iParentId = 0;
		self->pGUIControl = NULL;

		return (PyObject*)self;
	}

	void Control_Dealloc(Control* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	PyObject* Control_SetPosition(Control* self, PyObject* args)
	{
		DWORD posX = 0;
		DWORD posY = 0;
		if (!PyArg_ParseTuple(args, "ll", &posX, &posX)) return NULL;

		g_graphicsContext.Lock();
		self->pGUIControl->SetPosition(posX, posY);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetWidth(Control* self, PyObject* args)
	{
		DWORD width;
		if (!PyArg_ParseTuple(args, "l", &width)) return NULL;

		g_graphicsContext.Lock();
		self->pGUIControl->SetWidth(width);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetHeight(Control* self, PyObject* args)
	{
		DWORD height;
		if (!PyArg_ParseTuple(args, "l", &height)) return NULL;

		g_graphicsContext.Lock();
		self->pGUIControl->SetHeight(height);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Control_methods[] = {
		{"setPosition", (PyCFunction)Control_SetPosition, METH_VARARGS, ""},
		{"setWidth", (PyCFunction)Control_SetWidth, METH_VARARGS, ""},
		{"setHeight", (PyCFunction)Control_SetHeight, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject Control_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.Control",             /*tp_name*/
			sizeof(Control),            /*tp_basicsize*/
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
			"Control Objects",         /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			Control_methods,           /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			Control_New,               /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
