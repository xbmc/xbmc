#include "../../../stdafx.h"
#include "..\python\python.h"
#include "GuiImage.h"
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
	PyObject* ControlImage_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		ControlImage *self;
		char *cImage = NULL;
		char *cColorKey = NULL;
		
		self = (ControlImage*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		if (!PyArg_ParseTuple(args, "lllls|s", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
			&cImage, &cColorKey)) return NULL;

		// check if filename exists
		self->strFileName = cImage;
		if (cColorKey) sscanf(cColorKey, "%x", &self->strColorKey);
		else self->strColorKey = 0;

		return (PyObject*)self;
	}

	void ControlImage_Dealloc(ControlImage* self)
	{
		self->ob_type->tp_free((PyObject*)self);
	}

	CGUIControl* ControlImage_Create(ControlImage* pControl)
	{
		pControl->pGUIControl = new CGUIImage(pControl->iParentId, pControl->iControlId,
				pControl->dwPosX, pControl->dwPosY, pControl->dwWidth, pControl->dwHeight,
				pControl->strFileName, pControl->strColorKey);

		return pControl->pGUIControl;
	}

	PyMethodDef ControlImage_methods[] = {
		{NULL, NULL, 0, NULL}
	};

	PyDoc_STRVAR(controlImage__doc__,
		"ControlImage class.\n"
		"\n"
		"ControlImage(int x, int y, int width, int height[, filename, ColorKey])\n"
		"\n"
		"filename  : image filename\n"
		"ColorKey  : hexString (example, '0xFFFF3300')");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

	PyTypeObject ControlImage_Type;
	
  void initControlImage_Type()
	{
	  PyInitializeTypeObject(&ControlImage_Type);
	  
	  ControlImage_Type.tp_name = "xbmcgui.ControlImage";
	  ControlImage_Type.tp_basicsize = sizeof(ControlImage);
	  ControlImage_Type.tp_dealloc = (destructor)ControlImage_Dealloc;
	  ControlImage_Type.tp_compare = 0;
	  ControlImage_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	  ControlImage_Type.tp_doc = controlImage__doc__;
	  ControlImage_Type.tp_methods = ControlImage_methods;
	  ControlImage_Type.tp_base = &Control_Type;
	  ControlImage_Type.tp_new = ControlImage_New;
	}
}

#ifdef __cplusplus
}
#endif
