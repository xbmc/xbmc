#include "../../../stdafx.h"
#include "control.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
	/*
	// not used for now

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
*/

	/* This function should return -1 if obj1 is less than obj2,
	 * 0 if they are equal, and 1 if obj1 is greater than obj2
	 */
	int Control_Compare(PyObject* obj1, PyObject* obj2)
	{
		if(((Control*)obj1)->iControlId < ((Control*)obj2)->iControlId) return -1;
		if(((Control*)obj1)->iControlId > ((Control*)obj2)->iControlId) return 1;
		return 0;
	}

	PyDoc_STRVAR(getId__doc__,
		"getId() -- Get the control's current id.\n");

	PyObject* Control_GetId(Control* self, PyObject* args)
	{
		return Py_BuildValue("i", self->iControlId);
	}

	PyDoc_STRVAR(setVisible__doc__,
		"setVisible(bool) -- Hide's or Show's this control.\n");

	PyObject* Control_SetVisible(Control* self, PyObject* args)
	{
		bool visible;
		if (!PyArg_ParseTuple(args, "b", &visible)) return NULL;

		PyGUILock();
		if (self->pGUIControl) 	self->pGUIControl->SetVisible(visible);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setPosition__doc__,
		"setPosition(int x, int y) -- Set's the position of this control.\n");

	PyObject* Control_SetPosition(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "ll", &self->dwPosX, &self->dwPosY)) return NULL;

		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetPosition(self->dwPosX, self->dwPosY);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setWidth__doc__,
		"setWidth(int width) -- Set's the width of this control.\n");

	PyObject* Control_SetWidth(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwWidth)) return NULL;

		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetWidth(self->dwWidth);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setHeight__doc__,
		"setHeight(int height) -- Set's the height of this control.\n");

	PyObject* Control_SetHeight(Control* self, PyObject* args)
	{
		if (!PyArg_ParseTuple(args, "l", &self->dwHeight)) return NULL;

		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetHeight(self->dwHeight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setNavigation__doc__,
		"setNavigation( Control up, Control down, Control left, Control right) -- Set's navigation.\n"
		"\n"
		"Throws: TypeError, if one of the supplied arguments is not a Control type\n"
		"        ReferenceError, if one of the controls is not added to a window\n"
		"\n"
		"Same as controlUp(), controlDown(), controlLeft(), controlRight().");

	PyObject* Control_SetNavigation(Control* self, PyObject* args)
	{
		Control* pUpControl = NULL;
		Control* pDownControl = NULL;
		Control* pLeftControl = NULL;
		Control* pRightControl = NULL;

		if (!PyArg_ParseTuple(args, "OOOO", &pUpControl, &pDownControl, &pLeftControl, &pRightControl)) return NULL;

		// type checking, objects should be of type Control
		if(!(Control_Check(pUpControl) &&	Control_Check(pDownControl) &&
				Control_Check(pLeftControl) && Control_Check(pRightControl)))
		{
			PyErr_SetString(PyExc_TypeError, "Objects should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlUp = pUpControl->iControlId;
		self->iControlDown = pDownControl->iControlId;
		self->iControlLeft = pLeftControl->iControlId;
		self->iControlRight = pRightControl->iControlId;

		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(controlUp__doc__,
		"controlUp( Control ) -- Set onUp control.\n"
		"\n"
		"Throws: TypeError, if the supplied argument is not a Control type\n"
		"        ReferenceError, if the control is not added to a window\n"
		"\n"
		"When this control is active and the user presses up, the supplied Control\n"
		"will recieve focus.");

	PyObject* Control_ControlUp(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if(!Control_Check(pControl))
		{
			PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlUp = pControl->iControlId;
		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(controlDown__doc__,
		"controlDown( Control ) -- Set onDown control.\n"
		"\n"
		"Throws: TypeError, if the supplied argument is not a Control type\n"
		"        ReferenceError, if the control is not added to a window\n"
		"\n"
		"When this control is active and the user presses down, the supplied Control\n"
		"will recieve focus.");

	PyObject* Control_ControlDown(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if (!Control_Check(pControl))
		{
			PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlDown = pControl->iControlId;
		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(controlLeft__doc__,
		"controlLeft( Control ) -- Set onLeft control.\n"
		"\n"
		"Throws: TypeError, if the supplied argument is not a Control type\n"
		"        ReferenceError, if the control is not added to a window\n"
		"\n"
		"When this control is active and the user presses left, the supplied Control\n"
		"will recieve focus.");

	PyObject* Control_ControlLeft(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if (!Control_Check(pControl))
		{
			PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlLeft = pControl->iControlId;
		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(controlRight__doc__,
		"controlRight( Control ) -- Set onRight control.\n"
		"\n"
		"Throws: TypeError, if the supplied argument is not a Control type\n"
		"        ReferenceError, if the control is not added to a window\n"
		"\n"
		"When this control is active and the user presses right, the supplied Control\n"
		"will recieve focus.");

	PyObject* Control_ControlRight(Control* self, PyObject* args)
	{
		Control* pControl;
		if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
		// type checking, object should be of type Control
		if (!Control_Check(pControl))
		{
			PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
			return NULL;
		}
		if(self->iControlId == 0)
		{
			PyErr_SetString(PyExc_ReferenceError, "Control has to be added to a window first");
			return NULL;
		}

		self->iControlRight = pControl->iControlId;
		PyGUILock();
		if (self->pGUIControl) self->pGUIControl->SetNavigation(
				self->iControlUp,self->iControlDown,self->iControlLeft,self->iControlRight);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef Control_methods[] = {
		{"getId", (PyCFunction)Control_GetId, METH_VARARGS, getId__doc__},
		{"setVisible", (PyCFunction)Control_SetVisible, METH_VARARGS, setVisible__doc__},
		{"setPosition", (PyCFunction)Control_SetPosition, METH_VARARGS, setPosition__doc__},
		{"setWidth", (PyCFunction)Control_SetWidth, METH_VARARGS, setWidth__doc__},
		{"setHeight", (PyCFunction)Control_SetHeight, METH_VARARGS, setHeight__doc__},
		{"setNavigation", (PyCFunction)Control_SetNavigation, METH_VARARGS, setNavigation__doc__},
		{"controlUp", (PyCFunction)Control_ControlUp, METH_VARARGS, controlUp__doc__},
		{"controlDown", (PyCFunction)Control_ControlDown, METH_VARARGS, controlDown__doc__},
		{"controlLeft", (PyCFunction)Control_ControlLeft, METH_VARARGS, controlLeft__doc__},
		{"controlRight", (PyCFunction)Control_ControlRight, METH_VARARGS, controlRight__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(control__doc__,
		"Control class.\n"
		"\n"
		"Base class for all controls.");

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
			0,//(destructor)Control_Dealloc,/*tp_dealloc*/
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
			control__doc__,            /* tp_doc */
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
			0,//Control_New,               /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
