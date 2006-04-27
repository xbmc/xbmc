#include "../../../stdafx.h"
#include "..\python\python.h"
#include "GuiButtonControl.h"
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
	PyObject* ControlButton_New(
        PyTypeObject *type,
        PyObject *args,
        PyObject *kwds )
	{
		static char *keywords[] = {	
			"x", "y", "width", "height", "label",
            "focusTexture", "noFocusTexture", 
			"textXOffset", "textYOffset", "alignment",
            "font", "textColor", "disabledColor", NULL };
		ControlButton *self;
        char* cFont = NULL;
		char* cTextureFocus = NULL;
		char* cTextureNoFocus = NULL;
        char* cTextColor = NULL;
        char* cDisabledColor = NULL;

		PyObject* pObjectText;
		
		self = (ControlButton*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		// set up default values in case they are not supplied
        self->dwTextXOffset = CONTROL_TEXT_OFFSET_X;
        self->dwTextYOffset = CONTROL_TEXT_OFFSET_Y;
        self->dwAlign = (XBFONT_LEFT | XBFONT_CENTER_Y);
		self->strFont = "font13";		
		self->dwTextColor = 0xffffffff;
		self->dwDisabledColor = 0x60ffffff;

		if (!PyArg_ParseTupleAndKeywords(
            args,
            kwds,
            "llll|Osslllsss",
            keywords,
            &self->dwPosX,
            &self->dwPosY,
            &self->dwWidth,
            &self->dwHeight,
			&pObjectText,
            &cTextureFocus,
            &cTextureNoFocus,
            &self->dwTextXOffset,
            &self->dwTextYOffset,
            &self->dwAlign,
            &cFont,
            &cTextColor,
            &cDisabledColor ))
        {
            Py_DECREF( self );
            return NULL;
        }
		if (!PyGetUnicodeString(self->strText, pObjectText, 5))
        {
            Py_DECREF( self );
            return NULL;
        }

		// if texture is supplied use it, else get default ones
		self->strTextureFocus = cTextureFocus ?
            cTextureFocus :
            PyGetDefaultImage("button", "texturefocus", "button-focus.png");		
		self->strTextureNoFocus = cTextureNoFocus ?
            cTextureNoFocus :
            PyGetDefaultImage("button", "texturenofocus", "button-nofocus.jpg");

        if (cFont) self->strFont = cFont;
        if (cTextColor) sscanf( cTextColor, "%x", &self->dwTextColor );
        if (cDisabledColor)
        {
            sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
        }
		return (PyObject*)self;
	}

	void ControlButton_Dealloc(ControlButton* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

  CGUIControl* ControlButton_Create(ControlButton* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->dwTextColor;
    label.disabledColor = pControl->dwDisabledColor;
    label.align = pControl->dwAlign;
    label.offsetX = pControl->dwTextXOffset;
    label.offsetY = pControl->dwTextYOffset;
    pControl->pGUIControl = new CGUIButtonControl(
      pControl->iParentId,
      pControl->iControlId,
      pControl->dwPosX,
      pControl->dwPosY,
      pControl->dwWidth,
      pControl->dwHeight,
      pControl->strTextureFocus,
      pControl->strTextureNoFocus,
      label);

    CGUIButtonControl* pGuiButtonControl =
      (CGUIButtonControl*)pControl->pGUIControl;

    pGuiButtonControl->SetLabel(pControl->strText);

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
        {
			((CGUIButtonControl*)self->pGUIControl)->PythonSetDisabledColor(self->dwDisabledColor);
        }
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setLabel__doc__,
		"setLabel(label, font, textColor, disabledColor)\n"
		"\n"
        "Method to set the text for this button.\n"
		"\n"
		"label         : string or unicode string\n"
        "font          : name of font for button text (opt)\n"
        "textColor     : color of text (opt)\n"
        "disabledColor : disabled color of text (opt)\n" );

	PyObject* ControlButton_SetLabel(ControlButton *self, PyObject *args)
	{
		PyObject *pObjectText;
		char *cFont = NULL;
		char *cTextColor = NULL;
		char* cDisabledColor = NULL;

		if (!PyArg_ParseTuple(
                args,
                "O|sss",
                &pObjectText,
                &cFont,
                &cTextColor,
                &cDisabledColor))
        {
            return NULL;
        }
		if (!PyGetUnicodeString(self->strText, pObjectText, 1))
        {
            return NULL;
        }

        if (cFont) self->strFont = cFont;
		if (cTextColor) sscanf(cTextColor, "%x", &self->dwTextColor);
        if (cDisabledColor)
        {
            sscanf( cDisabledColor, "%x", &self->dwDisabledColor );
        }

		PyGUILock();
		if (self->pGUIControl)
        {
			((CGUIButtonControl*)self->pGUIControl)->PythonSetLabel(
                self->strFont, self->strText, self->dwTextColor );
			((CGUIButtonControl*)self->pGUIControl)->PythonSetDisabledColor(
                self->dwDisabledColor );
        }
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
		"ControlButton(\n"
    "    x, y, width, height, label, focusTexture, noFocusTexture, \n"
    "    textXOffset, textYOffset, alignment, font, textColor, disabledColor)\n"
		"\n"
    "x              : integer x coordinate of control\n"
    "y              : integer y coordinate of control\n"
    "width          : integer width of control\n"
    "height         : integer height of control\n"
		"label          : string or unicode string (opt)\n"
		"focusTexture   : filename for focus texture (opt)\n"
		"noFocusTexture : filename for no focus texture (opt)\n"
		"textXOffset    : integer x offset of label (opt)\n"
		"textYOffset    : integer y offset of label (opt)\n"
		"alignment      : integer alignment of label - see xbfont.h (opt)\n"
		"font           : font used for label text e.g. 'font13' (opt)\n"
		"textColor      : color of button text e.g. '0xFFFFFFFF' (opt)\n"
		"disabledColor  : color of disabled button text e.g. '0xFFFFFFFF' (opt)\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlButton_Type;
	
  void initControlButton_Type()
	{
	  PyInitializeTypeObject(&ControlButton_Type);
	  
	  ControlButton_Type.tp_name = "xbmcgui.ControlButton";
	  ControlButton_Type.tp_basicsize = sizeof(ControlButton);
	  ControlButton_Type.tp_dealloc = (destructor)ControlButton_Dealloc;
	  ControlButton_Type.tp_compare = 0;
	  ControlButton_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  ControlButton_Type.tp_doc = controlButton__doc__;
	  ControlButton_Type.tp_methods = ControlButton_methods;
	  ControlButton_Type.tp_base = &Control_Type;
	  ControlButton_Type.tp_new = ControlButton_New;
	}
}

#ifdef __cplusplus
}
#endif
