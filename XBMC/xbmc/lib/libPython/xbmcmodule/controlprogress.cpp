#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUIProgressControl.h"
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
  PyObject* ControlProgress_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "x", "y", "width", "height", "texturebg", "textureleft", "texturemid", "textureright", "textureoverlay", NULL };

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
      "llll|sssss",
      keywords,
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
    self->strTextureBg = cTextureBg ? cTextureBg : PyGetDefaultImage("progress", "texturebg", "progress_back.png");
    self->strTextureLeft = cTextureLeft ? cTextureLeft : PyGetDefaultImage("progress", "lefttexture", "progress_left.png");
    self->strTextureMid = cTextureMid ? cTextureMid : PyGetDefaultImage("progress", "midtexture", "progress_mid.png");
    self->strTextureRight = cTextureRight ? cTextureRight : PyGetDefaultImage("progress", "righttexture", "progress_right.png");
    self->strTextureOverlay = cTextureOverLay ? cTextureOverLay : PyGetDefaultImage("progress", "overlaytexture", "progress_over.png");

    //if (cColorDiffuse) sscanf(cColorDiffuse, "%x", &self->strColorDiffuse);
    //else self->strColorDiffuse = 0;

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
      (CStdString)pControl->strTextureOverlay, 0, 0);

    if (pControl->pGUIControl && pControl->strColorDiffuse)
        ((CGUIProgressControl *)pControl->pGUIControl)->SetColorDiffuse(pControl->strColorDiffuse);

    return pControl->pGUIControl;
  }


  PyDoc_STRVAR(setPercent__doc__,
    "setPercent(percent) -- Sets the pecertange for the progressbar to show.\n"
    "\n"
    "percent       : float - percentage of the bar.\n"
    "\n"
    "example:\n"
    "  - self.progress.setValue(60)\n");

  PyObject* ControlProgress_SetPercent(ControlProgress *self, PyObject *args)
  {
    float *cPercent;

    if (!PyArg_ParseTuple(args, "f", &cPercent)) return NULL;
    float fPercent = *cPercent;
    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUIProgressControl*)self->pGUIControl)->SetPercentage(fPercent);
    }
    PyGUIUnlock();
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
      return Py_BuildValue("f", fPercent);
    }
    return Py_BuildValue("f", 0);
  }

  PyMethodDef ControlProgress_methods[] = {
    {"setPercent", (PyCFunction)ControlProgress_SetPercent, METH_VARARGS, setPercent__doc__},
    {"getPercent", (PyCFunction)ControlProgress_GetPercent, METH_VARARGS, getPercent__doc__},
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
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlProgress_Type;

  void initControlProgress_Type()
  {
    PyInitializeTypeObject(&ControlProgress_Type);

    ControlProgress_Type.tp_name = "xbmcgui.ControlProgress";
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
