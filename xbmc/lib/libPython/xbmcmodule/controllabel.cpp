#include "../../../stdafx.h"
#include "..\python.h"
#include "GuiLabelControl.h"
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
			"disabledColor", "alignment", "hasPath", NULL };
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

		if (!PyArg_ParseTupleAndKeywords(
            args,
            kwds,
            "llll|Ossslb",
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
            &self->bHasPath))
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
		pControl->pGUIControl = new CGUILabelControl(
            pControl->iParentId,
            pControl->iControlId,
			pControl->dwPosX,
            pControl->dwPosY,
            pControl->dwWidth,
            pControl->dwHeight,
			pControl->strFont,
            pControl->strText,
            pControl->dwTextColor,
            pControl->dwDisabledColor,
            pControl->dwAlign,
            pControl->bHasPath );
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
        "             disabledColor, alignment, hasPath )\n"
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
        "hasPath       : flag indicating label stores a path (opt)" );
// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlLabel_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlLabel",    /*tp_name*/
			sizeof(ControlLabel),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlLabel_Dealloc,/*tp_dealloc*/
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
			controlLabel__doc__,       /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlLabel_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlLabel_New,          /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
