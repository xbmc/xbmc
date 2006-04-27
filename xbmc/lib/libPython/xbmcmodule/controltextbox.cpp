#include "../../../stdafx.h"
#include "..\python\python.h"
#include "GuiTextBox.h"
#include "GUIFontManager.h"
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
		self->pControlSpin->dwPosX = self->dwWidth - 25;
		self->pControlSpin->dwPosY = self->dwHeight - 30;

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
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->dwTextColor;
    CLabelInfo spinLabel;
    spinLabel.font = g_fontManager.GetFont(pControl->strFont);
    spinLabel.textColor = pControl->pControlSpin->dwColor;
    pControl->pGUIControl = new CGUITextBox(pControl->iParentId, pControl->iControlId,
      pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
      pControl->pControlSpin->dwWidth, pControl->pControlSpin->dwHeight,
      pControl->pControlSpin->strTextureUp, pControl->pControlSpin->strTextureDown, pControl->pControlSpin->strTextureUpFocus,
      pControl->pControlSpin->strTextureDownFocus, spinLabel, pControl->pControlSpin->dwPosX,
      pControl->pControlSpin->dwPosY, label);

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
		string strText;
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

	PyTypeObject ControlTextBox_Type;
	
  void initControlTextBox_Type()
	{
	  PyInitializeTypeObject(&ControlTextBox_Type);
	  
	  ControlTextBox_Type.tp_name = "xbmcgui.ControlTextBox";
	  ControlTextBox_Type.tp_basicsize = sizeof(ControlTextBox);
	  ControlTextBox_Type.tp_dealloc = (destructor)ControlTextBox_Dealloc;
	  ControlTextBox_Type.tp_compare = 0;
	  ControlTextBox_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  ControlTextBox_Type.tp_doc = controlTextBox__doc__;
	  ControlTextBox_Type.tp_methods = ControlTextBox_methods;
	  ControlTextBox_Type.tp_base = &Control_Type;
	  ControlTextBox_Type.tp_new = ControlTextBox_New;
	}
}

#ifdef __cplusplus
}
#endif
