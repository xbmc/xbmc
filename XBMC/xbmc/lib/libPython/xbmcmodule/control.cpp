#include "stdafx.h"
#include "..\..\..\application.h"
#include "GuiLabelControl.h"
#include <vector>
#include "control.h"

using namespace std;

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

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

	/* This function should return -1 if obj1 is less than obj2,
	 * 0 if they are equal, and 1 if obj1 is greater than obj2
	 */
	int Control_Compare(PyObject* obj1, PyObject* obj2)
	{
		if(((Control*)obj1)->iControlId < ((Control*)obj2)->iControlId) return -1;
		if(((Control*)obj1)->iControlId > ((Control*)obj2)->iControlId) return 1;
		return 0;
	}

	PyObject* Control_GetId(Control* self, PyObject* args)
	{
		return Py_BuildValue("i", self->iControlId);
	}

	PyObject* Control_SetVisible(Control* self, PyObject* args)
	{
		bool visible;
		if (!PyArg_ParseTuple(args, "b", &visible)) return NULL;

		g_graphicsContext.Lock();
		if (self->pGUIControl) 	self->pGUIControl->SetVisible(visible);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetPosition(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "ll", &self->dwPosX, &self->dwPosY)) return NULL;

		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetPosition(self->dwPosX, self->dwPosY);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetWidth(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwWidth)) return NULL;

		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetWidth(self->dwWidth);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetHeight(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwHeight)) return NULL;

		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetHeight(self->dwHeight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_SetNavigation(Control* self, PyObject* args)
	{
		Control* pUpControl = NULL;
		Control* pDownControl = NULL;
		Control* pLeftControl = NULL;
		Control* pRightControl = NULL;

		if (!PyArg_ParseTuple(args, "OOOO", &pUpControl, &pDownControl, &pLeftControl, &pRightControl)) return NULL;

		// type checking, objects should be of type Control
		if(strcmp(((PyObject*)pUpControl)->ob_type->tp_base->tp_name, Control_Type.tp_name) &&
			strcmp(((PyObject*)pDownControl)->ob_type->tp_base->tp_name, Control_Type.tp_name) &&
			strcmp(((PyObject*)pLeftControl)->ob_type->tp_base->tp_name, Control_Type.tp_name) &&
			strcmp(((PyObject*)pRightControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Objects should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString((PyObject*)self, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlUp = pUpControl->iControlId;
		self->iControlDown = pDownControl->iControlId;
		self->iControlLeft = pLeftControl->iControlId;
		self->iControlRight = pRightControl->iControlId;

		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_ControlUp(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString((PyObject*)self, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlUp = pControl->iControlId;
		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_ControlDown(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString((PyObject*)self, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlDown = pControl->iControlId;
		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_ControlLeft(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString((PyObject*)self, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlLeft = pControl->iControlId;
		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* Control_ControlRight(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(strcmp(((PyObject*)pControl)->ob_type->tp_base->tp_name, Control_Type.tp_name))
		{
			PyErr_SetString((PyObject*)self, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString((PyObject*)self, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlRight = pControl->iControlId;
		g_graphicsContext.Lock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Control_methods[] = {
		{"getId", (PyCFunction)Control_GetId, METH_VARARGS, ""},
		{"setVisible", (PyCFunction)Control_SetVisible, METH_VARARGS, ""},
		{"setPosition", (PyCFunction)Control_SetPosition, METH_VARARGS, ""},
		{"setWidth", (PyCFunction)Control_SetWidth, METH_VARARGS, ""},
		{"setHeight", (PyCFunction)Control_SetHeight, METH_VARARGS, ""},
		{"setNavigation", (PyCFunction)Control_SetNavigation, METH_VARARGS, ""},
		{"controlUp", (PyCFunction)Control_ControlUp, METH_VARARGS, ""},
		{"controlDown", (PyCFunction)Control_ControlDown, METH_VARARGS, ""},
		{"controlLeft", (PyCFunction)Control_ControlLeft, METH_VARARGS, ""},
		{"controlRight", (PyCFunction)Control_ControlRight, METH_VARARGS, ""},
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
			"xbmcgui.Control",         /*tp_name*/
			sizeof(Control),           /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)Control_Dealloc,/*tp_dealloc*/
			0,                         /*tp_print*/
			0,                         /*tp_getattr*/
			0,                         /*tp_setattr*/
			Control_Compare,           /*tp_compare*/
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
