#include "../../../stdafx.h"
#include "..\python.h"
#include "GuiButtonControl.h"
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
	PyObject* ControlButton_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlButton *self;
		char* cTextureFocus = NULL;
		char* cTextureNoFocus = NULL;

		PyObject* pObjectText;
		
		self = (ControlButton*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "llll|Oss", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&pObjectText, &cTextureFocus, &cTextureNoFocus)) return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 5)) return NULL;

		// SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
		self->strFont = "font13";		
		self->dwTextColor = 0xffffffff;
		self->dwDisabledColor = 0x60ffffff;

		// if texture is supplied use it, else get default ones
		self->strTextureFocus = cTextureFocus ? cTextureFocus : PyGetDefaultImage("button", "textureFocus", "button-focus.png");		
		self->strTextureNoFocus = cTextureNoFocus ? cTextureNoFocus : PyGetDefaultImage("button", "textureNoFocus", "button-nofocus.jpg");	

		return (PyObject*)self;
	}

	void ControlButton_Dealloc(ControlButton* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	CGUIControl* ControlButton_Create(ControlButton* pControl)
	{
		pControl->pGUIControl = new CGUIButtonControl(pControl->iParentId, pControl->iControlId,
				pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
				pControl->strTextureFocus, pControl->strTextureNoFocus,
				CONTROL_TEXT_OFFSET_X, CONTROL_TEXT_OFFSET_Y, (XBFONT_LEFT | XBFONT_CENTER_Y));

		CGUIButtonControl* pGuiButtonControl = (CGUIButtonControl*)pControl->pGUIControl;

		pGuiButtonControl->SetLabel(pControl->strFont, pControl->strText, pControl->dwTextColor);
		pGuiButtonControl->SetDisabledColor(pControl->dwDisabledColor);

		return pControl->pGUIControl;
	}

	PyDoc_STRVAR(setDisabledColor__doc__,
		"setDisabledColor(string hexcolor) -- .\n"
		"\n"
		"hexcolor     : hexString (example, '0xFFFF3300')");

	PyObject* ControlButton_SetDisabledColor(ControlButton *self, PyObject *args)
	{
		char *cDisabledColor = NULL;

		if (!PyArg_ParseTuple(args, "s", &cDisabledColor))	return NULL;

		// ControlButton *pControl = (ControlButton*)self;
		
		if (cDisabledColor) sscanf(cDisabledColor, "%x", &self->dwDisabledColor);

		PyGUILock();
		if (self->pGUIControl) 
			((CGUIButtonControl*)self->pGUIControl)->SetDisabledColor(self->dwDisabledColor);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setLabel__doc__,
		"setLabel(string label) -- Set's text for this button.\n"
		"\n"
		"label     : string or unicode string");

	PyObject* ControlButton_SetLabel(ControlButton *self, PyObject *args)
	{
		PyObject *pObjectText;
		char *cFont = NULL;
		char *cTextColor = NULL;

		if (!PyArg_ParseTuple(args, "O|ss", &pObjectText, &cFont, &cTextColor))	return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 1)) return NULL;

		self->strFont = cFont ? cFont : "font13";		
		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
		else self->dwTextColor = 0xffffffff;

		PyGUILock();
		if (self->pGUIControl)
			((CGUIButtonControl*)self->pGUIControl)->SetLabel(self->strFont, self->strText, self->dwTextColor);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ControlButton_methods[] = {
		{"setLabel", (PyCFunction)ControlButton_SetLabel, METH_VARARGS, setLabel__doc__},
		{"setDisabledColor", (PyCFunction)ControlButton_SetDisabledColor, METH_VARARGS, setDisabledColor__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlButton__doc__,
		"ControlButton class.\n"
		"\n"
		"ControlButton(int x, int y, int width, int height[, label, focus, nofocus])\n"
		"\n"
		"label     : string or unicode string\n"
		"focus     : filename for focus texture\n"
		"nofocus   : filename for no focus texture");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlButton_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlButton",    /*tp_name*/
			sizeof(ControlButton),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlButton_Dealloc,/*tp_dealloc*/
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
			controlButton__doc__,      /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlButton_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlButton_New,          /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
