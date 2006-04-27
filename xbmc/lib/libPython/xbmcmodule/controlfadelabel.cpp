#include "../../../stdafx.h"
#include "..\python\python.h"
#include "GuiFadeLabelControl.h"
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
	PyObject* ControlFadeLabel_New(
        PyTypeObject *type,
        PyObject *args,
        PyObject *kwds )
	{
		static char *keywords[] = {	
			"x", "y", "width", "height", "font", "textColor",
            "alignment", NULL };
		ControlFadeLabel *self;
		char *cFont = NULL;
		char *cTextColor = NULL;
		
		self = (ControlFadeLabel*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		// set up default values in case they are not supplied
        self->strFont = "font13";
        self->dwTextColor = 0xffffffff;
        self->dwAlign = XBFONT_LEFT;

		if (!PyArg_ParseTupleAndKeywords(
            args,
            kwds,
            "llll|ssl",
            keywords,
            &self->dwPosX,
            &self->dwPosY,
            &self->dwWidth,
            &self->dwHeight,
			&cFont,
            &cTextColor,
            &self->dwAlign ))
        {
            Py_DECREF( self );
            return NULL;
        }

        if (cFont) self->strFont = cFont;
		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);

		self->pGUIControl = NULL;

		return (PyObject*)self;
	}

	void ControlFadeLabel_Dealloc(Control* self)
	{
		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		pControl->vecLabels.clear();
		self->ob_type->tp_free((PyObject*)self);
	}

  CGUIControl* ControlFadeLabel_Create(ControlFadeLabel* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->dwTextColor;
    label.align = pControl->dwAlign;
    pControl->pGUIControl = new CGUIFadeLabelControl(
      pControl->iParentId,
      pControl->iControlId,
      pControl->dwPosX,
      pControl->dwPosY,
      pControl->dwWidth,
      pControl->dwHeight,
      label);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
    pControl->pGUIControl->OnMessage(msg);

    return pControl->pGUIControl;
  }

	PyDoc_STRVAR(addLabel__doc__,
		"addLabel(string label) -- Add a label to this control for scrolling.\n"
		"\n"
		"label     : string or unicode string");

	PyObject* ControlFadeLabel_AddLabel(ControlFadeLabel *self, PyObject *args)
	{
		PyObject *pObjectText;
		string strText;

		if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
		if (!PyGetUnicodeString(strText, pObjectText, 1)) return NULL;

		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		CGUIMessage msg(GUI_MSG_LABEL_ADD, pControl->iParentId, pControl->iControlId);
		msg.SetLabel(strText);

		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(reset__doc__,
		"reset() -- Reset's the fade label.\n");

	PyObject* ControlFadeLabel_Reset(ControlFadeLabel *self, PyObject *args)
	{
		ControlFadeLabel *pControl = (ControlFadeLabel*)self;
		CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

		pControl->vecLabels.clear();
		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ControlFadeLabel_methods[] = {
		{"addLabel", (PyCFunction)ControlFadeLabel_AddLabel, METH_VARARGS, addLabel__doc__},
		{"reset", (PyCFunction)ControlFadeLabel_Reset, METH_VARARGS, reset__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlFadeLabel__doc__,
		"ControlFadeLabel class.\n"
		"Control that scroll's lables"
		"\n"
		"ControlFadeLabel(x, y, width, height, font, textColor, alignment)\n"
		"\n"
        "x         : x coordinate of control\n"
        "y         : y coordinate of control\n"
        "width     : width of control\n"
        "height    : height of control\n"
		"font      : string fontname (example, 'font13' / 'font14') (opt)\n"
		"textColor : hexString e.g. '0xFFFF3300' (opt)\n"
        "alignment : alignment of text - see xbfont.h (opt)\n" );

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlFadeLabel_Type;
	
	void initControlFadeLabel_Type()
	{
	  PyInitializeTypeObject(&ControlFadeLabel_Type);
	  
	  ControlFadeLabel_Type.tp_name = "xbmcgui.ControlFadeLabel";
	  ControlFadeLabel_Type.tp_basicsize = sizeof(ControlFadeLabel);
	  ControlFadeLabel_Type.tp_dealloc = (destructor)ControlFadeLabel_Dealloc;
	  ControlFadeLabel_Type.tp_compare = 0;
	  ControlFadeLabel_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  ControlFadeLabel_Type.tp_doc = controlFadeLabel__doc__;
	  ControlFadeLabel_Type.tp_methods = ControlFadeLabel_methods;
	  ControlFadeLabel_Type.tp_base = &Control_Type;
	  ControlFadeLabel_Type.tp_new = ControlFadeLabel_New;
	}
}

#ifdef __cplusplus
}
#endif
