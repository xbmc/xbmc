/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
#include <Python.h>
#else
  #include "python/Include/Python.h"
#endif
#include "../XBPythonDll.h"
#include "guilib/GUIImage.h"
#include "control.h"
#include "pyutil.h"

using namespace std;

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
    static const char *keywords[] = {
      "x", "y", "width", "height", "filename", "aspectRatio", "colorDiffuse", NULL };
    ControlImage *self;
    char *cImage = NULL;
    char *cColorDiffuse = NULL;//"0xFFFFFFFF";

    self = (ControlImage*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFileName) string();

    //if (!PyArg_ParseTuple(args, "lllls|l", &self->dwPosX, &self->dwPosY, &self->dwWidth, &self->dwHeight,
    //  &cImage, &self->aspectRatio)) return NULL;
    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args, kwds,
      (char*)"lllls|ls",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cImage,
      &self->aspectRatio,
      &cColorDiffuse ))
    {
      Py_DECREF( self );
      return NULL;
    }

    // check if filename exists
    self->strFileName = cImage;
    if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &self->colorDiffuse);
    else self->colorDiffuse = 0;

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
      (CStdString)pControl->strFileName);

    if (pControl->pGUIControl && pControl->aspectRatio <= CAspectRatio::AR_KEEP)
      ((CGUIImage *)pControl->pGUIControl)->SetAspectRatio((CAspectRatio::ASPECT_RATIO)pControl->aspectRatio);

    if (pControl->pGUIControl && pControl->colorDiffuse)
      ((CGUIImage *)pControl->pGUIControl)->SetColorDiffuse(pControl->colorDiffuse);

    return pControl->pGUIControl;
  }


  PyDoc_STRVAR(setImage__doc__,
    "setImage(filename, colorKey) -- Changes the image.\n"
    "\n"
    "filename       : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.image.setImage('special://home/scripts/test.png')\n");

  PyObject* ControlImage_SetImage(ControlImage *self, PyObject *args)
  {
    char *cImage = NULL;

    if (!PyArg_ParseTuple(args, (char*)"s", &cImage)) return NULL;

    self->strFileName = cImage;

    PyXBMCGUILock();
    if (self->pGUIControl)
      ((CGUIImage*)self->pGUIControl)->SetFileName(self->strFileName);

    PyXBMCGUIUnlock();
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

    if (!PyArg_ParseTuple(args, (char*)"s", &cColorDiffuse)) return NULL;

    if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &self->colorDiffuse);
    else self->colorDiffuse = 0;

    PyXBMCGUILock();
    if (self->pGUIControl)
      ((CGUIImage *)self->pGUIControl)->SetColorDiffuse(self->colorDiffuse);

    PyXBMCGUIUnlock();
    Py_INCREF(Py_None);
    return Py_None;
  }


  PyMethodDef ControlImage_methods[] = {
    {(char*)"setImage", (PyCFunction)ControlImage_SetImage, METH_VARARGS, setImage__doc__},
    {(char*)"setColorDiffuse", (PyCFunction)ControlImage_SetColorDiffuse, METH_VARARGS, setColorDiffuse__doc__},
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
    PyXBMCInitializeTypeObject(&ControlImage_Type);

    ControlImage_Type.tp_name = (char*)"xbmcgui.ControlImage";
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
