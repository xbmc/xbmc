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
            PyGetDefaultImage("button", "textureFocus", "button-focus.png");		
		self->strTextureNoFocus = cTextureNoFocus ?
            cTextureNoFocus :
            PyGetDefaultImage("button", "textureNoFocus", "button-nofocus.jpg");

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
		pControl->pGUIControl = new CGUIButtonControl(
            pControl->iParentId,
            pControl->iControlId,
			pControl->dwPosX,
            pControl->dwPosY,
            pControl->dwWidth,
            pControl->dwHeight,
			pControl->strTextureFocus,
            pControl->strTextureNoFocus,
			pControl->dwTextXOffset,
            pControl->dwTextYOffset,
            pControl->dwAlign );

		CGUIButtonControl* pGuiButtonControl =
            (CGUIButtonControl*)pControl->pGUIControl;

		pGuiButtonControl->SetLabel(
            pControl->strFont, pControl->strText, pControl->dwTextColor);
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
        {
			((CGUIButtonControl*)self->pGUIControl)->SetDisabledColor(self->dwDisabledColor);
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
			((CGUIButtonControl*)self->pGUIControl)->SetLabel(
                self->strFont, self->strText, self->dwTextColor );
			((CGUIButtonControl*)self->pGUIControl)->SetDisabledColor(
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
