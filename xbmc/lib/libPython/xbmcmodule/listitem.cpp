#include "stdafx.h"
#include "..\python.h"
#include "listitem.h"
#include "pyutil.h"

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
	PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ListItem *self;
		char* cLabel = NULL;

		// allocate new object
		self = (ListItem*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		// parse user input
		if (!PyArg_ParseTuple(args, "|s", &cLabel)) return NULL;

		// create CGUIListItem
		if (cLabel) self->item = new CGUIListItem(cLabel);
		else self->item = new CGUIListItem();

		return (PyObject*)self;
	}

	/*
	 * allocate a new listitem. Used for c++ and not the python user
	 * returns a new reference
	 */
	ListItem* ListItem_FromString(wstring strLabel)
	{
		ListItem* self = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
		if (!self) return NULL;

		self->item = new CGUIListItem(strLabel);

		return self;
	}

	void ListItem_Dealloc(ListItem* self)
	{
		if (self->item) delete self->item;
		self->ob_type->tp_free((PyObject*)self);
	}
	
	PyObject* ListItem_GetLabel(ListItem *self, PyObject *args)
	{
		if (!self->item) return NULL;

		PyGUILock();
		const char *cLabel =  self->item->GetLabel().c_str();
		PyGUIUnlock();

		return Py_BuildValue("s", cLabel);
	}

	PyObject* ListItem_SetLabel(ListItem *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!self->item) return NULL;

		if (!PyArg_ParseTuple(args, "s", &cLine))	return NULL;

		// set label
		PyGUILock();
		self->item->SetLabel(cLine ? cLine : "");
		PyGUIUnlock();
		
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ListItem_methods[] = {
		{"getLabel", (PyCFunction)ListItem_GetLabel, METH_VARARGS, ""},
		{"setLabel", (PyCFunction)ListItem_SetLabel, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ListItem_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ListItem",        /*tp_name*/
			sizeof(ListItem),          /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ListItem_Dealloc,/*tp_dealloc*/
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
			"ListItem Objects",        /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ListItem_methods,          /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			0,                         /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ListItem_New,              /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
