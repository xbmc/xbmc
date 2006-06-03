#include "../../../stdafx.h"
#include "..\python\python.h"
#include "GuiLabelControl.h"
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
	PyObject* ControlLabel_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		static char *keywords[] = {	
			"x", "y", "width", "height", "label", "font", "textColor", 
			"disabledColor", "alignment", "hasPath", "angle", NULL };

		ControlLabel *self;
		char *cFont = NULL;
		char *cTextColor = NULL;
		char *cDisabledColor = NULL;
		PyObject* pObjectText = NULL;
		
		self = (ControlLabel*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		// set up default values in case they are not supplied
        self->strFont = "font13";
        self->dwTextColor = 0xffffffff;
		self->dwDisabledColor = 0x60ffffff;
        self->dwAlign = XBFONT_LEFT;
        self->bHasPath = false;
        self->iAngle = 0;

		if (!PyArg_ParseTupleAndKeywords(
            args,
            kwds,
            "llll|Ossslbi",
            keywords,
            &self->dwPosX,
            &self->dwPosY,
            &self->dwWidth,
            &self->dwHeight,
			&pObjectText,
            &cFont,
            &cTextColor,
            &cDisabledColor,
            &self->dwAlign,
            &self->bHasPath,
            &self->iAngle))
        {
            Py_DECREF( self );
            return NULL;
        }
		if (!PyGetUnicodeString(self->strText, pObjectText, 5))
        {
            Py_DECREF( self );
            return NULL;
        }

        if (cFont) self->strFont = cFont;
		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
        if (cDisabledColor)
        {
            sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
        }

		return (PyObject*)self;
	}

	void ControlLabel_Dealloc(ControlLabel* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

  CGUIControl* ControlLabel_Create(ControlLabel* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->dwTextColor;
    label.disabledColor = pControl->dwDisabledColor;
    label.align = pControl->dwAlign;
    pControl->pGUIControl = new CGUILabelControl(
      pControl->iParentId,
      pControl->iControlId,
      pControl->dwPosX,
      pControl->dwPosY,
      pControl->dwWidth,
      pControl->dwHeight,
      pControl->strText,
      label,
      pControl->bHasPath,
      pControl->iAngle );
    return pControl->pGUIControl;
  }

	PyDoc_STRVAR(setLabel__doc__,
		"setLabel(string label) -- Set's text for this label.\n"
		"\n"
		"label     : string or unicode string");

	PyObject* ControlLabel_SetLabel(ControlLabel *self, PyObject *args)
	{
		PyObject *pObjectText;

		if (!PyArg_ParseTuple(args, "O", &pObjectText))	return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 1)) return NULL;

		ControlLabel *pControl = (ControlLabel*)self;
		CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
		msg.SetLabel(self->strText);

		PyGUILock();
		if (pControl->pGUIControl) pControl->pGUIControl->OnMessage(msg);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ControlLabel_methods[] = {
		{"setLabel", (PyCFunction)ControlLabel_SetLabel, METH_VARARGS, setLabel__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlLabel__doc__,
		"ControlLabel class.\n"
		"\n"
		"ControlLabel(x, y, width, height, label, font, textColor, \n"
        "             disabledColor, alignment, hasPath, angle )\n"
		"\n"
        "x             : integer x coordinate of control\n"
        "y             : integer y coordinate of control\n"
        "width         : integer width of control\n"
        "height        : integer height of control\n"
		"label         : string or unicode string (opt)\n"
		"font          : string fontname (e.g., 'font13' / 'font14') (opt)\n"
		"textColor     : hexString (e.g., '0xFFFF3300') (opt)\n"
		"disabledColor : hexString (e.g., '0xFFFF3300') (opt)\n"
		"alignment     : alignment of text - see xbfont.h (opt)\n"
		"hasPath       : flag indicating label stores a path (opt)\n"
		"angle         : integer angle of control (opt)" );
// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlLabel_Type;
	
  void initControlLabel_Type()
	{
	  PyInitializeTypeObject(&ControlLabel_Type);
	  
	  ControlLabel_Type.tp_name = "xbmcgui.ControlLabel";
	  ControlLabel_Type.tp_basicsize = sizeof(ControlLabel);
	  ControlLabel_Type.tp_dealloc = (destructor)ControlLabel_Dealloc;
	  ControlLabel_Type.tp_compare = 0;
	  ControlLabel_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  ControlLabel_Type.tp_doc = controlLabel__doc__;
	  ControlLabel_Type.tp_methods = ControlLabel_methods;
	  ControlLabel_Type.tp_base = &Control_Type;
	  ControlLabel_Type.tp_new = ControlLabel_New;
	}
}

#ifdef __cplusplus
}
#endif
