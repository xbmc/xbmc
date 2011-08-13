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

#include <Python.h>

#include "guilib/GUIProgressControl.h"
#include "control.h"
#include "pyutil.h"

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ControlProgress_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char* keywords[] = { "x", "y", "width", "height", "texturebg", "textureleft", "texturemid", "textureright", "textureoverlay", NULL };

    ControlProgress *self;
    char *cTextureBg = NULL;
    char *cTextureLeft = NULL;
    char *cTextureMid  = NULL;
    char *cTextureRight  = NULL;
    char *cTextureOverLay  = NULL;

    self = (ControlProgress*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strTextureLeft) string();
    new(&self->strTextureMid) string();
    new(&self->strTextureRight) string();
    new(&self->strTextureBg) string();
    new(&self->strTextureOverlay) string();

    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
      (char*)"llll|sssss",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cTextureBg,
      &cTextureLeft,
      &cTextureMid,
      &cTextureRight,
      &cTextureOverLay))
    {
      Py_DECREF( self );
      return NULL;
    }

    // if texture is supplied use it, else get default ones
    self->strTextureBg = cTextureBg ? cTextureBg : PyXBMCGetDefaultImage((char*)"progress", (char*)"texturebg", (char*)"progress_back.png");
    self->strTextureLeft = cTextureLeft ? cTextureLeft : PyXBMCGetDefaultImage((char*)"progress", (char*)"lefttexture", (char*)"progress_left.png");
    self->strTextureMid = cTextureMid ? cTextureMid : PyXBMCGetDefaultImage((char*)"progress", (char*)"midtexture", (char*)"progress_mid.png");
    self->strTextureRight = cTextureRight ? cTextureRight : PyXBMCGetDefaultImage((char*)"progress", (char*)"righttexture", (char*)"progress_right.png");
    self->strTextureOverlay = cTextureOverLay ? cTextureOverLay : PyXBMCGetDefaultImage((char*)"progress", (char*)"overlaytexture", (char*)"progress_over.png");

    //if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &self->colorDiffuse);
    //else self->colorDiffuse = 0;

    return (PyObject*)self;
  }

  void ControlProgress_Dealloc(ControlProgress* self)
  {
    self->strTextureLeft.~string();
    self->strTextureMid.~string();
    self->strTextureRight.~string();
    self->strTextureBg.~string();
    self->strTextureOverlay.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlProgress_Create(ControlProgress* pControl)
  {
    pControl->pGUIControl = new CGUIProgressControl(pControl->iParentId, pControl->iControlId,(float)pControl->dwPosX, (float)pControl->dwPosY,
      (float)pControl->dwWidth,(float)pControl->dwHeight,
      (CStdString)pControl->strTextureBg,(CStdString)pControl->strTextureLeft,
      (CStdString)pControl->strTextureMid,(CStdString)pControl->strTextureRight,
      (CStdString)pControl->strTextureOverlay);

    if (pControl->pGUIControl && pControl->colorDiffuse)
        ((CGUIProgressControl *)pControl->pGUIControl)->SetColorDiffuse(pControl->colorDiffuse);

    return pControl->pGUIControl;
  }

  PyDoc_STRVAR(setPercent__doc__,
    "setPercent(percent) -- Sets the percentage of the progressbar to show.\n"
    "\n"
    "percent       : float - percentage of the bar to show.\n"
    "\n"
    "*Note, valid range for percent is 0-100\n"
    "\n"
    "example:\n"
    "  - self.progress.setPercent(60)\n");

  PyObject* ControlProgress_SetPercent(ControlProgress *self, PyObject *args)
  {
    float fPercent = 0;
    if (!PyArg_ParseTuple(args, (char*)"f", &fPercent)) return NULL;

    if (self->pGUIControl)
      ((CGUIProgressControl*)self->pGUIControl)->SetPercentage(fPercent);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getPercent__doc__,
    "getPercent() -- Returns a float of the percent of the progress.\n"
    "\n"
    "example:\n"
    "  - print self.progress.getValue()\n");

  PyObject* ControlProgress_GetPercent(ControlProgress *self, PyObject *args)
  {
    float fPercent;
    if (self->pGUIControl)
    {
      fPercent = ((CGUIProgressControl*)self->pGUIControl)->GetPercentage();
      return Py_BuildValue((char*)"f", fPercent);
    }
    return Py_BuildValue((char*)"f", 0);
  }

  PyMethodDef ControlProgress_methods[] = {
    {(char*)"setPercent", (PyCFunction)ControlProgress_SetPercent, METH_VARARGS, setPercent__doc__},
    {(char*)"getPercent", (PyCFunction)ControlProgress_GetPercent, METH_VARARGS, getPercent__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlProgress class
  PyDoc_STRVAR(ControlProgress__doc__,
    "ControlProgress class.\n"
    "\n"
    "ControlProgress(x, y, width, height[, texturebg, textureleft, texturemid, textureright, textureoverlay])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "texturebg      : [opt] string - image filename.\n"
    "textureleft    : [opt] string - image filename.\n"
    "texturemid     : [opt] string - image filename.\n"
    "textureright   : [opt] string - image filename.\n"
    "textureoverlay : [opt] string - image filename.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.progress = xbmcgui.ControlProgress(100, 250, 125, 75)\n");

// Restore code and data sections to normal.

  PyTypeObject ControlProgress_Type;

  void initControlProgress_Type()
  {
    PyXBMCInitializeTypeObject(&ControlProgress_Type);

    ControlProgress_Type.tp_name = (char*)"xbmcgui.ControlProgress";
    ControlProgress_Type.tp_basicsize = sizeof(ControlProgress);
    ControlProgress_Type.tp_dealloc = (destructor)ControlProgress_Dealloc;
    ControlProgress_Type.tp_compare = 0;
    ControlProgress_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlProgress_Type.tp_doc = ControlProgress__doc__;
    ControlProgress_Type.tp_methods = ControlProgress_methods;
    ControlProgress_Type.tp_base = &Control_Type;
    ControlProgress_Type.tp_new = ControlProgress_New;
  }
}

#ifdef __cplusplus
}
#endif
