#include "../../../stdafx.h"
#include "..\python.h"
#include "GuiTextBox.h"
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
	extern PyObject* ControlSpin_New(void);

	PyObject* ControlTextBox_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlTextBox *self;
		char *cFont = NULL;
		char *cTextColor = NULL;
		
		self = (ControlTextBox*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		self->pControlSpin = (ControlSpin*)ControlSpin_New();
		if (!self->pControlSpin) return NULL;

		if (!PyArg_ParseTuple(args, "llll|ss", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&cFont, &cTextColor)) return NULL;

		// set default values if needed
		self->strFont = cFont ? cFont : "font13";

		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
		else self->dwTextColor = 0xffffffff;

		// default values for spin control
		self->pControlSpin->dwPosX = self->dwPosX + self->dwWidth - 25;
		self->pControlSpin->dwPosY = self->dwPosY + self->dwHeight - 30;

		return (PyObject*)self;
	}

	void ControlTextBox_Dealloc(ControlTextBox* self)
	{
		Py_DECREF(self->pControlSpin);
		self->ob_type->tp_free((PyObject*)self);
	}

	CGUIControl* ControlTextBox_Create(ControlTextBox* pControl)
	{
		// create textbox
		pControl->pGUIControl = new CGUITextBox(pControl->iParentId, pControl->iControlId,
				pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
				pControl->strFont, pControl->pControlSpin->dwWidth, pControl->pControlSpin->dwHeight,
				pControl->pControlSpin->strTextureUp, pControl->pControlSpin->strTextureDown, pControl->pControlSpin->strTextureUpFocus,
				pControl->pControlSpin->strTextureDownFocus, pControl->pControlSpin->dwColor, pControl->pControlSpin->dwPosX,
				pControl->pControlSpin->dwPosY, pControl->strFont, pControl->dwTextColor);

		// reset textbox
		CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
		pControl->pGUIControl->OnMessage(msg);

		// set values for spincontrol
		pControl->pControlSpin->iControlId = pControl->iControlId;
		pControl->pControlSpin->iParentId = pControl->iParentId;

		return pControl->pGUIControl;
	}

	PyDoc_STRVAR(setText__doc__,
		"SetText(string text) -- Set's the text for this textbox.\n"
		"\n"
		"label     : string or unicode string");

	PyObject* ControlTextBox_SetText(ControlTextBox *self, PyObject *args)
	{
		PyObject *pObjectText;
		wstring strText;
		if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
		if (!PyGetUnicodeString(strText, pObjectText, 1)) return NULL;

		// create message
		ControlTextBox *pControl = (ControlTextBox*)self;
		CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
		msg.SetLabel(strText);

		// send message
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(reset__doc__,
		"reset() -- Clear's the text box.\n");

	PyObject* ControlTextBox_Reset(ControlTextBox *self, PyObject *args)
	{
		// create message
		ControlTextBox *pControl = (ControlTextBox*)self;
		CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

		// send message
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(getSpinControl__doc__,
		"getSpinControl() -- returns the associated ControlSpin."
		"\n"
		"- Not working completely yet -\n"
		"After adding this textbox to a window it is not possible to change\n"
		"the settings of this spin control.");

	PyObject* ControlTextBox_GetSpinControl(ControlTextBox *self, PyObject *args)
	{
		Py_INCREF(self->pControlSpin);
		return (PyObject*)self->pControlSpin;
	}

	PyMethodDef ControlTextBox_methods[] = {
		{"setText", (PyCFunction)ControlTextBox_SetText, METH_VARARGS, setText__doc__},
		{"reset", (PyCFunction)ControlTextBox_Reset, METH_VARARGS, reset__doc__},
		{"getSpinControl", (PyCFunction)ControlTextBox_GetSpinControl, METH_VARARGS, getSpinControl__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlTextBox__doc__,
		"ControlTextBox class.\n"
		"\n"
		"ControlTextBox(int x, int y, int width, int height[, font, textColor])\n"
		"\n"
		"font      : string fontname (example, 'font13' / 'font14')\n"
		"textColor : hexString (example, '0xFFFF3300')");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlTextBox_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlTextBox",  /*tp_name*/
			sizeof(ControlTextBox),    /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlTextBox_Dealloc,/*tp_dealloc*/
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
			controlTextBox__doc__,     /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlTextBox_methods,    /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlTextBox_New,        /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
