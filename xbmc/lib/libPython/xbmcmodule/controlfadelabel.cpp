#include "stdafx.h"
#include "..\python.h"
#include "GuiLabelControl.h"
#include "control.h"
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
	PyObject* ControlFadeLabel_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlFadeLabel *self;
		char *cFont = NULL;
		char *cTextColor = NULL;
		
		self = (ControlFadeLabel*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "llll|ss", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&cFont, &cTextColor)) return NULL;

		self->strFont = cFont ? cFont : "font13";
		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
		else self->dwTextColor = 0xffffffff;

		self->pGUIControl = NULL;

		return (PyObject*)self;
	}

	void ControlFadeLabel_Dealloc(Control* self)
	{
		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		pControl->vecLabels.clear();
		self->ob_type->tp_free((PyObject*)self);
	}

	PyObject* ControlFadeLabel_AddLabel(ControlFadeLabel *self, PyObject *args)
	{
		PyObject *pObjectText;
		wstring strText;

		if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
		if (!PyGetUnicodeString(strText, pObjectText, 1)) return NULL;

		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		CGUIMessage msg(GUI_MSG_LABEL_ADD, pControl->iParentId, pControl->iControlId);
		msg.SetLabel(strText);

		g_graphicsContext.Lock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* ControlFadeLabel_Reset(ControlFadeLabel *self, PyObject *args)
	{
		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

		pControl->vecLabels.clear();
		g_graphicsContext.Lock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		g_graphicsContext.Unlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ControlFadeLabel_methods[] = {
		{"addLabel", (PyCFunction)ControlFadeLabel_AddLabel, METH_VARARGS, ""},
		{"reset", (PyCFunction)ControlFadeLabel_Reset, METH_VARARGS, ""},
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlFadeLabel_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlFadeLabel",/*tp_name*/
			sizeof(ControlFadeLabel),  /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlFadeLabel_Dealloc,/*tp_dealloc*/
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
			"ControlFadeLabel Objects",/* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlFadeLabel_methods,  /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlFadeLabel_New,      /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
