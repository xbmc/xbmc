#include "../../../stdafx.h"
#include "..\python.h"
#include "GuiSpinControl.h"
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
	/*
	// not used for now
	PyObject* ControlSpin_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlSpin *self;
		char* cTextureFocus = NULL;
		char* cTextureNoFocus = NULL;

		PyObject* pObjectText;
		
		self = (ControlSpin*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "llll|Oss", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&pObjectText, &cTextureFocus, &cTextureNoFocus)) return NULL;
		if (!PyGetUnicodeString(self->strText, pObjectText, 5)) return NULL;

		// SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
		self->strFont = "font13";		
		self->dwTextColor = 0xffffffff;
		self->dwDisabledColor = 0x60ffffff;

		self->strTextureFocus = cTextureFocus ? cTextureFocus : "button-focus.png";		
		self->strTextureNoFocus = cTextureNoFocus ? cTextureNoFocus : "button-nofocus.jpg";	

		return (PyObject*)self;
	}
*/
	/*
	 * allocate a new controlspin. Used for c++ and not the python user
	 */
	PyObject* ControlSpin_New()
	{
		//ControlSpin* self = (ControlSpin*)_PyObject_New(&ControlSpin_Type);
		ControlSpin*self = (ControlSpin*)ControlSpin_Type.tp_alloc(&ControlSpin_Type, 0);
		if (!self) return NULL;

		// default values for spin control
		self->dwColor = 0xffffffff;
		self->dwPosX = 0;
		self->dwPosY = 0;
		self->dwWidth = 16;
		self->dwHeight = 16;

		// get default images
    self->strTextureUp = PyGetDefaultImage("listcontrol", "textureUp", "scroll-up.png");
		self->strTextureDown = PyGetDefaultImage("listcontrol", "textureDown", "scroll-down.png");
		self->strTextureUpFocus = PyGetDefaultImage("listcontrol", "textureUpFocus", "scroll-up-focus.png");
		self->strTextureDownFocus = PyGetDefaultImage("listcontrol", "textureDownFocus", "scroll-down-focus.png");

		return (PyObject*)self;
	}

	void ControlSpin_Dealloc(ControlSpin* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	PyObject* ControlSpin_SetColor(ControlSpin *self, PyObject *args)
	{
		char *cColor = NULL;

		if (!PyArg_ParseTuple(args, "s", &cColor))	return NULL;

		if (cColor) sscanf(cColor, "%x", &self->dwColor);

		PyGUILock();
		//if (self->pGUIControl) 
			//((CGUISpinControl*)self->pGUIControl)->SetColor(self->dwDColor);
		PyGUIUnlock();

		Py_INCREF(Py_None);
		return Py_None;
	}

	/*
	 * set textures
	 * (string textureUp, string textureDown, string textureUpFocus, string textureDownFocus)
	 */
	PyDoc_STRVAR(setTextures__doc__,
		"setTextures(up, down, upFocus, downFocus) -- Set's textures for this control.\n"
		"\n"
		"texture are image files that are used for example in the skin");

	PyObject* ControlSpin_SetTextures(ControlSpin *self, PyObject *args)
	{
		char *cLine[4];

		if (!PyArg_ParseTuple(args, "ssss", &cLine[0], &cLine[1], &cLine[2], &cLine[3]))	return NULL;

		self->strTextureUp = cLine[0];
		self->strTextureDown = cLine[1];
		self->strTextureUpFocus = cLine[2];
		self->strTextureDownFocus = cLine[3];
		/*
		PyGUILock();
		if (self->pGUIControl) 
		{
      CGUISpinControl* pControl = (CGUISpinControl*)self->pGUIControl;
			pControl->se
		PyGUIUnlock();
		*/
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	PyMethodDef ControlSpin_methods[] = {
		//{"setColor", (PyCFunction)ControlSpin_SetColor, METH_VARARGS, ""},
		{"setTextures", (PyCFunction)ControlSpin_SetTextures, METH_VARARGS, setTextures__doc__},
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlSpin__doc__,
		"ControlSpin class.\n"
		"\n"
		" - Not working yet -.\n"
		"\n"
		"you can't create this object, it is returned by objects like ControlTextBox and ControlList.");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlSpin_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlSpin",     /*tp_name*/
			sizeof(ControlSpin),       /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlSpin_Dealloc,/*tp_dealloc*/
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
			controlSpin__doc__,        /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlSpin_methods,       /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			0,//ControlSpin_New,           /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
