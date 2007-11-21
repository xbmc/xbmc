#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "guiImage.h"
#include "control.h"
#include "pyutil.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ControlImage_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = {
      "x", "y", "width", "height", "filename", "colorKey", "aspectRatio", "colorDiffuse", NULL };
    ControlImage *self;
    char *cImage = NULL;
    char *cColorKey = NULL;
    char *cColorDiffuse = NULL;//"0xFFFFFFFF";

    self = (ControlImage*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFileName) string();    

    //if (!PyArg_ParseTuple(args, "lllls|sl", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
    //	&cImage, &cColorKey, &self->aspectRatio)) return NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args, kwds,
      "lllls|sls", keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cImage,
      &cColorKey,
      &self->aspectRatio,
      &cColorDiffuse ))
    {
      Py_DECREF( self );
      return NULL;
    }

    // check if filename exists
    self->strFileName = cImage;
    if (cColorKey) sscanf(cColorKey, "%lx", &self->strColorKey);
    else self->strColorKey = 0;
    if (cColorDiffuse) sscanf(cColorDiffuse, "%lx", &self->strColorDiffuse);
    else self->strColorDiffuse = 0;

    return (PyObject*)self;
  }

  void ControlImage_Dealloc(ControlImage* self)
  {
    self->strFileName.~string();  
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlImage_Create(ControlImage* pControl)
  {
    pControl->pGUIControl = new CGUIImage(pControl->iParentId, pControl->iControlId,
      (float)pControl->dwPosX, (float)pControl->dwPosY, (float)pControl->dwWidth, (float)pControl->dwHeight,
      (CStdString)pControl->strFileName, pControl->strColorKey);

    if (pControl->pGUIControl && pControl->aspectRatio >= CGUIImage::ASPECT_RATIO_STRETCH && pControl->aspectRatio <= CGUIImage::ASPECT_RATIO_KEEP)
      ((CGUIImage *)pControl->pGUIControl)->SetAspectRatio((CGUIImage::GUIIMAGE_ASPECT_RATIO)pControl->aspectRatio);

    if (pControl->pGUIControl && pControl->strColorDiffuse)
      ((CGUIImage *)pControl->pGUIControl)->SetColorDiffuse(pControl->strColorDiffuse);

    return pControl->pGUIControl;
  }


  PyDoc_STRVAR(setImage__doc__,
    "setImage(filename, colorKey) -- Changes the image.\n"
    "\n"
    "filename       : string - image filename.\n"
    "colorKey       : [opt] hexString - (example, '0xFFFF3300')\n"
    "\n"
    "example:\n"
    "  - self.image.setImage('q:\\scripts\\test.png', '0xFFFF3300')\n");

  PyObject* ControlImage_SetImage(ControlImage *self, PyObject *args)
  {
    char *cImage = NULL;
    char *cColorKey = NULL;

    if (!PyArg_ParseTuple(args, "s|s", &cImage, &cColorKey)) return NULL;

    self->strFileName = cImage;
    if (cColorKey) sscanf(cColorKey, "%lx", &self->strColorKey);
    else self->strColorKey = 0;

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUIImage*)self->pGUIControl)->SetFileName(self->strFileName);
      ((CGUIImage*)self->pGUIControl)->PythonSetColorKey(self->strColorKey);
    }
    PyGUIUnlock();
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setColorDiffuse__doc__,
    "setColorDiffuse(colorDiffuse) -- Changes the images color.\n"
    "\n"
    "colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))\n"
    "\n"
    "example:\n"
    "  - self.image.setColorDiffuse('0xC0FF0000')\n");

  PyObject* ControlImage_SetColorDiffuse(ControlImage *self, PyObject *args)
  {
    char *cColorDiffuse = NULL;

    if (!PyArg_ParseTuple(args, "s", &cColorDiffuse)) return NULL;

    if (cColorDiffuse) sscanf(cColorDiffuse, "%lx", &self->strColorDiffuse);
    else self->strColorDiffuse = 0;

    PyGUILock();
    if (self->pGUIControl)
      ((CGUIImage *)self->pGUIControl)->SetColorDiffuse(self->strColorDiffuse);

    PyGUIUnlock();
    Py_INCREF(Py_None);
    return Py_None;
  }


  PyMethodDef ControlImage_methods[] = {
    {"setImage", (PyCFunction)ControlImage_SetImage, METH_VARARGS, setImage__doc__},
    {"setColorDiffuse", (PyCFunction)ControlImage_SetColorDiffuse, METH_VARARGS, setColorDiffuse__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlImage class
  PyDoc_STRVAR(controlImage__doc__,
    "ControlImage class.\n"
    "\n"
    "ControlImage(x, y, width, height, filename[, colorKey, aspectRatio, colorDiffuse])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "filename       : string - image filename.\n"
    "colorKey       : [opt] hexString - (example, '0xFFFF3300')\n"
    "aspectRatio    : [opt] integer - (values 0 = stretch (default), 1 = scale up (crops), 2 = scale down (black bars)"
    "colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.image = xbmcgui.ControlImage(100, 250, 125, 75, aspectRatio=2)\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

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
