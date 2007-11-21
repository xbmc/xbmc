#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUISpinControl.h"
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
    new(&self->strTextureUp) string();    
    new(&self->strTextureDown) string();    
    new(&self->strTextureUpFocus) string();    
    new(&self->strTextureDownFocus) string();      

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
    new(&self->strTextureUp) string();    
    new(&self->strTextureDown) string();    
    new(&self->strTextureUpFocus) string();    
    new(&self->strTextureDownFocus) string();      

    // default values for spin control
    self->dwColor = 0xffffffff;
    self->dwPosX = 0;
    self->dwPosY = 0;
    self->dwWidth = 16;
    self->dwHeight = 16;

    // get default images
    self->strTextureUp = PyGetDefaultImage("listcontrol", "textureup", "scroll-up.png");
    self->strTextureDown = PyGetDefaultImage("listcontrol", "texturedown", "scroll-down.png");
    self->strTextureUpFocus = PyGetDefaultImage("listcontrol", "textureupfocus", "scroll-up-focus.png");
    self->strTextureDownFocus = PyGetDefaultImage("listcontrol", "texturedownfocus", "scroll-down-focus.png");

    return (PyObject*)self;
  }

  void ControlSpin_Dealloc(ControlSpin* self)
  {
    self->strTextureUp.~string();
    self->strTextureDown.~string();
    self->strTextureUpFocus.~string();
    self->strTextureDownFocus.~string();  
    self->ob_type->tp_free((PyObject*)self);
  }

  PyObject* ControlSpin_SetColor(ControlSpin *self, PyObject *args)
  {
    char *cColor = NULL;

    if (!PyArg_ParseTuple(args, "s", &cColor))	return NULL;

    if (cColor) sscanf(cColor, "%lx", &self->dwColor);

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
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlSpin_Type;

  void initControlSpin_Type()
  {
    PyInitializeTypeObject(&ControlSpin_Type);

    ControlSpin_Type.tp_name = "xbmcgui.ControlSpin";
    ControlSpin_Type.tp_basicsize = sizeof(ControlSpin);
    ControlSpin_Type.tp_dealloc = (destructor)ControlSpin_Dealloc;
    ControlSpin_Type.tp_compare = 0;
    ControlSpin_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlSpin_Type.tp_doc = controlSpin__doc__;
    ControlSpin_Type.tp_methods = ControlSpin_methods;
    ControlSpin_Type.tp_base = &Control_Type;
    ControlSpin_Type.tp_new = 0; //ControlSpin_New
  }
}

#ifdef __cplusplus
}
#endif
