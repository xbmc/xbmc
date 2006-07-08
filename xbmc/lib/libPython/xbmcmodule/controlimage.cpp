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
		static char *keywords[] = {	
			"x", "y", "width", "height", "filename", "colorKey", "aspectRatio", NULL };
    ControlImage *self;
		char *cImage = NULL;
		char *cColorKey = NULL;
		
		self = (ControlImage*)type->tp_alloc(type, 0);
		if (!self) return NULL;
		
		//if (!PyArg_ParseTuple(args, "lllls|sl", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
		//	&cImage, &cColorKey, &self->aspectRatio)) return NULL;
		// parse arguments to constructor
		if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "lllls|sl",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cImage,
      &cColorKey,
      &self->aspectRatio ))
		{
			Py_DECREF( self );
			return NULL;
		}

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

    if (pControl->pGUIControl && pControl->aspectRatio >= CGUIImage::ASPECT_RATIO_STRETCH && pControl->aspectRatio <= CGUIImage::ASPECT_RATIO_KEEP)
      ((CGUIImage *)pControl->pGUIControl)->SetAspectRatio((CGUIImage::GUIIMAGE_ASPECT_RATIO)pControl->aspectRatio);
		return pControl->pGUIControl;
	}


	PyDoc_STRVAR(setImage__doc__,
		"setImage(filename) -- Changes the image.\n"
		"\n"
		"filename          : string or unicode - text string.\n"
		"\n"
		"example:\n"
		"  - self.image.setImage('q:\\scripts\\test.png')\n");
	
	PyObject* ControlImage_SetImage(ControlImage *self, PyObject *args)
	{
		char *cImage;
		if (!PyArg_ParseTuple(args, "s", &cImage)) return NULL;
		self->strFileName = cImage;
		PyGUILock();
		if (self->pGUIControl)
		{
			((CGUIImage*)self->pGUIControl)->SetFileName(self->strFileName);
		}
		PyGUIUnlock();
		Py_INCREF(Py_None);
		return Py_None;
	}



	PyMethodDef ControlImage_methods[] = {
		{"setImage", (PyCFunction)ControlImage_SetImage, METH_VARARGS, setImage__doc__},
		{NULL, NULL, 0, NULL}
	};

	// ControlImage class
	PyDoc_STRVAR(controlImage__doc__,
		"ControlImage class.\n"
		"\n"
		"ControlImage(x, y, width, height, filename[, colorKey, aspectRatio])\n"
		"\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
		"filename       : string - image filename.\n"
    "colorKey       : [opt] hexString - (example, '0xFFFF3300')\n"
    "aspectRatio    : [opt] integer - (values 0 = stretch (default), 1 = scale up (crops), 2 = scale down (black bars)"
		"\n"
		"*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
		"\n"
		"example:\n"
		"  - self.image = xbmcgui.ControlImage(100, 250, 125, 75, aspectRatio=2)\n");

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
