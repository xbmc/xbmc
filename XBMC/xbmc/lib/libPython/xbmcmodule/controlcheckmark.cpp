#include "stdafx.h"
#include "..\python.h"
#include "GuiCheckMarkControl.h"
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
	PyObject* ControlCheckMark_New(
        PyTypeObject *type,
        PyObject *args,
        PyObject *kwds )
	{
		static char *keywords[] = {	
			"x", "y", "width", "height", "label", "focusTexture", "noFocusTexture", 
			"checkWidth", "checkHeight", "alignment", 0 };
		ControlCheckMark *self;
		char* cTextureFocus = NULL;
		char* cTextureNoFocus = NULL;

		PyObject* pObjectText;
		
		self = (ControlCheckMark*)type->tp_alloc(type, 0);
		if (!self) return NULL;

		// set up default values in case they are not supplied
		self->dwCheckWidth = 30;
        self->dwCheckHeight = 30;
        self->dwAlign = XBFONT_RIGHT;
		self->strTextureFocus = PyGetDefaultImage( "checkmark", "textureFocus", "check-box.png" );
		self->strTextureNoFocus = PyGetDefaultImage( "checkmark", "textureNoFocus", "check-boxNF.png" );

		// these defaults cannot be supplied on construction
		self->strFont = "font13";
		self->dwTextColor = 0xffffffff;
		self->dwDisabledColor = 0x60ffffff;

		// parse arguments to constructor
		if (!PyArg_ParseTupleAndKeywords(
			args,
			kwds,
			"llll|Osslll:ControlCheckMark",
			keywords,
            &self->dwPosX,
            &self->dwPosY,
            &self->dwWidth,
            &self->dwHeight,
			&pObjectText,
            &cTextureFocus,
            &cTextureNoFocus,
            &self->dwCheckWidth,
            &self->dwCheckHeight,
            &self->dwAlign )) return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 5)) return NULL;

		return (PyObject*)self;
	}

	void ControlCheckMark_Dealloc(ControlCheckMark* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	CGUIControl* ControlCheckMark_Create(ControlCheckMark* pControl)
	{
		pControl->pGUIControl = new CGUICheckMarkControl(
            pControl->iParentId,
            pControl->iControlId,
            pControl->dwPosX,
            pControl->dwPosY,
            pControl->dwWidth,
            pControl->dwHeight,
			pControl->strTextureFocus,
            pControl->strTextureNoFocus,
			pControl->dwCheckWidth,
            pControl->dwCheckHeight,
            pControl->dwAlign );

		CGUICheckMarkControl* pGuiCheckMarkControl =
            (CGUICheckMarkControl*)pControl->pGUIControl;

		pGuiCheckMarkControl->SetLabel(
            pControl->strFont,
            pControl->strText,
            pControl->dwTextColor );
		pGuiCheckMarkControl->SetDisabledColor( pControl->dwDisabledColor );

		return pControl->pGUIControl;
	}

	PyDoc_STRVAR(setDisabledColor__doc__,
		"setDisabledColor(string hexcolor) -- .\n"
		"\n"
		"hexcolor     : hexString (example, '0xFFFF3300')");

	PyObject* ControlCheckMark_SetDisabledColor(
        ControlCheckMark *self,
        PyObject *args)
	{
		char *cDisabledColor = NULL;

		if (!PyArg_ParseTuple(args, "s", &cDisabledColor))	return NULL;

		if (cDisabledColor)
        {
            sscanf(cDisabledColor, "%x", &self->dwDisabledColor);
        }

		PyGUILock();
		if (self->pGUIControl) 
			((CGUICheckMarkControl*)self->pGUIControl)->SetDisabledColor(
                self->dwDisabledColor );
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(setLabel__doc__,
		"setLabel(string label[, string fontName[, string textColor]]) -- Set's text for this button.\n"
		"\n"
		"label     : string or unicode string\n"
        "fontName  : name of font\n"
        "textColor : label text color" );

	PyObject* ControlCheckMark_SetLabel(ControlCheckMark *self, PyObject *args)
	{
		PyObject *pObjectText;
		char *cFont = NULL;
		char *cTextColor = NULL;

		if (!PyArg_ParseTuple(args, "O|ss", &pObjectText, &cFont, &cTextColor))
            return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 1))
            return NULL;

		self->strFont = cFont ? cFont : "font13";		
		if (cTextColor)
            sscanf(cTextColor, "%x", &self->dwTextColor);
		else
            self->dwTextColor = 0xffffffff;

		PyGUILock();
		if (self->pGUIControl)
			((CGUICheckMarkControl*)self->pGUIControl)->SetLabel(
                self->strFont,
                self->strText,
                self->dwTextColor );
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyDoc_STRVAR(getSelected__doc__,
		"getSelected() -- Returns the selected value for the check mark.\n" );

	PyObject* ControlCheckMark_GetSelected( ControlCheckMark *self )
	{
        bool isSelected = 0;

		PyGUILock();
		if (self->pGUIControl)
			isSelected = ((CGUICheckMarkControl*)self->pGUIControl)->GetSelected();
		PyGUIUnlock();

		return Py_BuildValue("b", isSelected);
	}

	PyDoc_STRVAR(setSelected__doc__,
		"setSelected(bool isOn) -- Sets the check mark on or off.\n"
		"\n"
        "isOn   : True if selected, False if not selected" );

	PyObject* ControlCheckMark_SetSelected(
        ControlCheckMark *self,
        PyObject *args)
	{
        bool isSelected = 0;

		if (!PyArg_ParseTuple(args, "b", &isSelected))
            return NULL;

		PyGUILock();
		if (self->pGUIControl)
			((CGUICheckMarkControl*)self->pGUIControl)->SetSelected(
                isSelected );
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyMethodDef ControlCheckMark_methods[] = {
		{"getSelected", (PyCFunction)ControlCheckMark_GetSelected, METH_NOARGS, getSelected__doc__},
		{"setSelected", (PyCFunction)ControlCheckMark_SetSelected, METH_VARARGS, setSelected__doc__},
		{"setLabel", (PyCFunction)ControlCheckMark_SetLabel, METH_VARARGS, setLabel__doc__},
		{"setDisabledColor", (PyCFunction)ControlCheckMark_SetDisabledColor, METH_VARARGS, setDisabledColor__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlCheckMark__doc__,
		"ControlCheckMark class.\n"
		"\n"
		"ControlCheckMark(int x, int y, int width, int height[, label, focusTexture, noFocusTexture, checkWidth, checkHeight, align])\n"
		"\n"
		"label			: string or unicode string\n"
		"focusTexture   : filename for focus texture\n"
		"noFocusTexture : filename for no focus texture\n"
        "checkWidth		: width of checkmark\n"
        "checkHeight	: height of checkmark\n"
        "alignment		: alignment of checkmark label" );

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlCheckMark_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlCheckMark",/*tp_name*/
			sizeof(ControlCheckMark),  /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlCheckMark_Dealloc,/*tp_dealloc*/
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
			controlCheckMark__doc__,   /* tp_doc */
			0,		                   /* tp_traverse */
			0,		                   /* tp_clear */
			0,		                   /* tp_richcompare */
			0,		                   /* tp_weaklistoffset */
			0,		                   /* tp_iter */
			0,		                   /* tp_iternext */
			ControlCheckMark_methods,  /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlCheckMark_New,      /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
