#include "../../../stdafx.h"
#include "..\python.h"
#include "listitem.h"
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
	PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ListItem *self;
        static char *keywords[] = {
            "label", "label2", "iconImage", "thumbnailImage", NULL };
		char* cLabel = NULL;
        char* cLabel2 = NULL;
        char* cIconImage = NULL;
        char* cThumbnailImage = NULL;

		// allocate new object
		self = (ListItem*)type->tp_alloc(type, 0);
		if (!self) return NULL;
        self->item = NULL;

		// parse user input
		if (!PyArg_ParseTupleAndKeywords(
            args,
            kwds,
            "|ssss",
            keywords,
            &cLabel,
            &cLabel2,
            &cIconImage,
            &cThumbnailImage))
        {
            Py_DECREF( self );
            return NULL;
        }

		// create CGUIListItem
        self->item = new CGUIListItem();
        if (!self->item)
        {
            Py_DECREF( self );
            return NULL;
        }
        if (cLabel)
        {
            self->item->SetLabel( cLabel );
        }
        if (cLabel2)
        {
            self->item->SetLabel2( cLabel2 );
        }
        if (cIconImage)
        {
            self->item->SetIconImage( cIconImage );
        }
        if (cThumbnailImage)
        {
            self->item->SetThumbnailImage( cThumbnailImage );
        }

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
        if (!self->item)
        {
            Py_DECREF( self );
            return NULL;
        }

		return self;
	}

	void ListItem_Dealloc(ListItem* self)
	{
		if (self->item) delete self->item;
		self->ob_type->tp_free((PyObject*)self);
	}

	PyDoc_STRVAR(getLabel__doc__,
		"getLabel() -- Returns the listitem label.");

	PyObject* ListItem_GetLabel(ListItem *self, PyObject *args)
	{
		if (!self->item) return NULL;

		PyGUILock();
		const char *cLabel =  self->item->GetLabel().c_str();
		PyGUIUnlock();

		return Py_BuildValue("s", cLabel);
	}

	PyDoc_STRVAR(getLabel2__doc__,
		"getLabel2() -- Returns the listitem's second label.");

	PyObject* ListItem_GetLabel2(ListItem *self, PyObject *args)
	{
		if (!self->item) return NULL;

		PyGUILock();
		const char *cLabel =  self->item->GetLabel2().c_str();
		PyGUIUnlock();

		return Py_BuildValue("s", cLabel);
	}

	PyDoc_STRVAR(setLabel__doc__,
		"setLabel(string label) -- Sets the listitem's label.");

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

	PyDoc_STRVAR(setLabel2__doc__,
		"setLabel2(string label) -- Sets the listitem's second label.");

	PyObject* ListItem_SetLabel2(ListItem *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!self->item) return NULL;

		if (!PyArg_ParseTuple(args, "s", &cLine))	return NULL;

		// set label
		PyGUILock();
		self->item->SetLabel2(cLine ? cLine : "");
		PyGUIUnlock();
		
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setIconImage__doc__,
		"setIconImage(string iconName) -- Sets the listitem's icon image.");

	PyObject* ListItem_SetIconImage(ListItem *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!self->item) return NULL;

		if (!PyArg_ParseTuple(args, "s", &cLine))	return NULL;

		// set label
		PyGUILock();
		self->item->SetIconImage(cLine ? cLine : "");
		PyGUIUnlock();
		
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setThumbnailImage__doc__,
		"setThumbnailImage(string iconName) -- Sets the listitem's thumbnail image.");

	PyObject* ListItem_SetThumbnailImage(ListItem *self, PyObject *args)
	{
		char *cLine = NULL;
		if (!self->item) return NULL;

		if (!PyArg_ParseTuple(args, "s", &cLine))	return NULL;

		// set label
		PyGUILock();
		self->item->SetThumbnailImage(cLine ? cLine : "");
		PyGUIUnlock();
		
		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ListItem_methods[] = {
		{"getLabel",
            (PyCFunction)ListItem_GetLabel,
            METH_VARARGS,
            getLabel__doc__},
		{"setLabel",
            (PyCFunction)ListItem_SetLabel,
            METH_VARARGS,
            setLabel__doc__},
		{"getLabel2",
            (PyCFunction)ListItem_GetLabel2,
            METH_VARARGS,
            getLabel2__doc__},
		{"setLabel2",
            (PyCFunction)ListItem_SetLabel2,
            METH_VARARGS,
            setLabel2__doc__},
		{"setIconImage",
            (PyCFunction)ListItem_SetIconImage,
            METH_VARARGS,
            setIconImage__doc__},
		{"setThumbnailImage",
            (PyCFunction)ListItem_SetThumbnailImage,
            METH_VARARGS,
            setThumbnailImage__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(listItem__doc__,
		"ListItem class.\n"
		"\n"
		"ListItem([string label, string label2, string iconImage, string thumbnailImage]) -- Creates a new ListItem.");

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
			listItem__doc__,        /* tp_doc */
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
