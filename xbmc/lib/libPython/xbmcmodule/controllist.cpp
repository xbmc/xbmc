#include "stdafx.h"
#include "..\python.h"
#include "GuiListControl.h"
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
	PyObject* ControlList_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlList *self;
		//char* cTextureFocus = NULL;
		//char* cTextureNoFocus = NULL;

		//PyObject* pObjectText;
		
		self = (ControlList*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "llll", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight)) return NULL;
		//if (!PyGetUnicodeString(self->strText, pObjectText, 5)) return NULL;

		// SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
		self->strFont = "font13";
		self->dwSpinWidth = 16;
		self->dwSpinHeight = 16;

    self->strTextureUp = "scroll-up.png";
		self->strTextureDown = "scroll-down.png";
		self->strTextureUpFocus = "scroll-up-focus.png";
		self->strTextureDownFocus = "scroll-down-focus.png";

		self->dwSpinColor = 0xFFB2D4F5;
		self->dwSpinX = 640;
		self->dwSpinY = 425;

		self->dwTextColor = 0xFFFFFFFF;
		self->dwSelectedColor = 0xFFF8BC70;
		self->strButton = "list-nofocus.png";
		self->strButtonFocus = "list-nofocus.png";


		//self->dwTextColor = 0xffffffff;
		//self->dwDisabledColor = 0x60ffffff;

		//self->strTextureFocus = cTextureFocus ? cTextureFocus : "button-focus.png";		
		//self->strTextureNoFocus = cTextureNoFocus ? cTextureNoFocus : "button-nofocus.jpg";	

		/*

		<spinWidth>16</spinWidth>
		<spinHeight>16</spinHeight>
		<spinPosX>640</spinPosX>
		<spinPosY>425</spinPosY>
		<spinColor>FFB2D4F5</spinColor>
		<textureUp>scroll-up.png</textureUp>
		<textureDown>scroll-down.png</textureDown>
		<textureUpFocus>scroll-up-focus.png</textureUpFocus>
		<textureDownFocus>scroll-down-focus.png</textureDownFocus>
		<textureFocus>list-focus.png</textureFocus>
		<textureNoFocus>list-nofocus.png</textureNoFocus>
		<textureHeight>27</textureHeight>
		<image>defaultFolderBig.png</image>
		<font>font14</font>
		<selectedColor>FFF8BC70</selectedColor>
		<textcolor>FFFFFFFF</textcolor>
		<colordiffuse>ffffffff</colordiffuse>
		<suffix>|</suffix>
		*/
		return (PyObject*)self;
	}

	void ControlList_Dealloc(ControlList* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	PyMethodDef ControlList_methods[] = {
		{NULL, NULL, 0, NULL}
	};

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlList_Type = {
			PyObject_HEAD_INIT(NULL)
			0,                         /*ob_size*/
			"xbmcgui.ControlList",    /*tp_name*/
			sizeof(ControlList),      /*tp_basicsize*/
			0,                         /*tp_itemsize*/
			(destructor)ControlList_Dealloc,/*tp_dealloc*/
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
			"ControlList Objects",    /* tp_doc */
			0,		                     /* tp_traverse */
			0,		                     /* tp_clear */
			0,		                     /* tp_richcompare */
			0,		                     /* tp_weaklistoffset */
			0,		                     /* tp_iter */
			0,		                     /* tp_iternext */
			ControlList_methods,      /* tp_methods */
			0,                         /* tp_members */
			0,                         /* tp_getset */
			&Control_Type,             /* tp_base */
			0,                         /* tp_dict */
			0,                         /* tp_descr_get */
			0,                         /* tp_descr_set */
			0,                         /* tp_dictoffset */
			0,                         /* tp_init */
			0,                         /* tp_alloc */
			ControlList_New,          /* tp_new */
	};
}

#ifdef __cplusplus
}
#endif
