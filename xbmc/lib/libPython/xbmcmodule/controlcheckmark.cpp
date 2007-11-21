#include "stdafx.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "GUICheckMarkControl.h"
#include "GUIFontManager.h"
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
  PyObject* ControlCheckMark_New(PyTypeObject *type, PyObject *args, PyObject *kwds )
  {
    static char *keywords[] = {
      "x", "y", "width", "height", "label", "focusTexture", "noFocusTexture",
      "checkWidth", "checkHeight", "alignment", "font", "textColor", "disabledColor", NULL };
    ControlCheckMark *self;
    char* cFont = NULL;
    char* cTextureFocus = NULL;
    char* cTextureNoFocus = NULL;
    char* cTextColor = NULL;
    char* cDisabledColor = NULL;

    PyObject* pObjectText;

    self = (ControlCheckMark*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();    
    new(&self->strText) string();    
    new(&self->strTextureFocus) string();    
    new(&self->strTextureNoFocus) string();    

    // set up default values in case they are not supplied
    self->dwCheckWidth = 30;
    self->dwCheckHeight = 30;
    self->dwAlign = XBFONT_RIGHT;
    self->strFont = "font13";
    self->dwTextColor = 0xffffffff;
    self->dwDisabledColor = 0x60ffffff;

    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "llllO|sslllsss:ControlCheckMark",
      keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cTextureFocus,
      &cTextureNoFocus,
      &self->dwCheckWidth,
      &self->dwCheckHeight,
      &self->dwAlign,
      &cFont,
      &cTextColor,
      &cDisabledColor ))
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
    if (cTextColor) sscanf(cTextColor, "%lx", &self->dwTextColor);
    if (cDisabledColor)
    {
      sscanf( cDisabledColor, "%lx", &self->dwDisabledColor );
    }
    self->strTextureFocus = cTextureFocus ?
      cTextureFocus :
      PyGetDefaultImage("checkmark", "texturefocus", "check-box.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyGetDefaultImage("checkmark", "texturenofocus", "check-boxNF.png");

    return (PyObject*)self;
  }

  void ControlCheckMark_Dealloc(ControlCheckMark* self)
  {
    self->strFont.~string();
    self->strText.~string();
    self->strTextureFocus.~string();
    self->strTextureNoFocus.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlCheckMark_Create(ControlCheckMark* pControl)
  {
    CLabelInfo label;
    label.disabledColor = pControl->dwDisabledColor;
    label.textColor = label.focusedColor = pControl->dwTextColor;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.align = pControl->dwAlign;
    CImage imageFocus;
    imageFocus.file = pControl->strTextureFocus;
    CImage imageNoFocus;
    imageNoFocus.file = pControl->strTextureNoFocus;
    pControl->pGUIControl = new CGUICheckMarkControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      imageFocus, imageNoFocus,
      (float)pControl->dwCheckWidth,
      (float)pControl->dwCheckHeight,
      label );

    CGUICheckMarkControl* pGuiCheckMarkControl = (CGUICheckMarkControl*)pControl->pGUIControl;
    pGuiCheckMarkControl->SetLabel(pControl->strText);

    return pControl->pGUIControl;
  }

  // setDisabledColor() Method
  PyDoc_STRVAR(setDisabledColor__doc__,
    "setDisabledColor(disabledColor) -- Set's this controls disabled color.\n"
    "\n"
    "disabledColor  : hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n"
    "\n"
    "example:\n"
    "  - self.checkmark.setDisabledColor('0xFFFF3300')\n");

  PyObject* ControlCheckMark_SetDisabledColor(ControlCheckMark *self, PyObject *args)
  {
    char *cDisabledColor = NULL;

    if (!PyArg_ParseTuple(args, "s", &cDisabledColor))	return NULL;

    if (cDisabledColor)
    {
      sscanf(cDisabledColor, "%lx", &self->dwDisabledColor);
    }

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetDisabledColor( self->dwDisabledColor );
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label[, font, textColor, disabledColor]) -- Set's this controls text attributes.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n"
    "\n"
    "example:\n"
    "  - self.checkmark.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300')\n");

  PyObject* ControlCheckMark_SetLabel(ControlCheckMark *self, PyObject *args)
  {
    PyObject *pObjectText;
    char *cFont = NULL;
    char *cTextColor = NULL;
    char* cDisabledColor = NULL;

    if (!PyArg_ParseTuple(
      args, "O|sss",
      &pObjectText, &cFont,
      &cTextColor,  &cDisabledColor))
      return NULL;

    if (!PyGetUnicodeString(self->strText, pObjectText, 1))
      return NULL;

    if (cFont) self->strFont = cFont;
    if (cTextColor)
    {
      sscanf(cTextColor, "%lx", &self->dwTextColor);
    }
    if (cDisabledColor)
    {
      sscanf(cDisabledColor, "%lx", &self->dwDisabledColor);
    }

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetLabel(
        self->strFont,
        self->strText,
        self->dwTextColor );
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetDisabledColor(
        self->dwDisabledColor );
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getSelected() Method
  PyDoc_STRVAR(getSelected__doc__,
    "getSelected() -- Returns the selected status for this checkmark as a bool.\n"
    "\n"
    "example:\n"
    "  - selected = self.checkmark.getSelected()\n");

  PyObject* ControlCheckMark_GetSelected( ControlCheckMark *self )
  {
    bool isSelected = 0;

    PyGUILock();
    if (self->pGUIControl)
    {
      isSelected = ((CGUICheckMarkControl*)self->pGUIControl)->GetSelected();
    }
    PyGUIUnlock();

    return Py_BuildValue("b", isSelected);
  }

  // setSelected() Method
  PyDoc_STRVAR(setSelected__doc__,
    "setSelected(isOn) -- Sets this checkmark status to on or off.\n"
    "\n"
    "isOn           : bool - True=selected (on) / False=not selected (off)"
    "\n"
    "example:\n"
    "  - self.checkmark.setSelected(True)\n");

  PyObject* ControlCheckMark_SetSelected(ControlCheckMark *self, PyObject *args)
  {
    bool isSelected = 0;

    if (!PyArg_ParseTuple(args, "b", &isSelected))
      return NULL;

    PyGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->SetSelected(isSelected);
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlCheckMark_methods[] = {
    {"getSelected", (PyCFunction)ControlCheckMark_GetSelected, METH_NOARGS, getSelected__doc__},
    {"setSelected", (PyCFunction)ControlCheckMark_SetSelected, METH_VARARGS, setSelected__doc__},
    {"setLabel", (PyCFunction)ControlCheckMark_SetLabel, METH_VARARGS, setLabel__doc__},
    {"setDisabledColor", (PyCFunction)ControlCheckMark_SetDisabledColor, METH_VARARGS, setDisabledColor__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlCheckMark class
  PyDoc_STRVAR(controlCheckMark__doc__,
    "ControlCheckMark class.\n"
    "\n"
    "ControlCheckMark(x, y, width, height, label[, focusTexture, noFocusTexture,\n"
    "                 checkWidth, checkHeight, alignment, font, textColor, disabledColor])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "label          : string or unicode - text string.\n"
    "focusTexture   : [opt] string - filename for focus texture.\n"
    "noFocusTexture : [opt] string - filename for no focus texture.\n"
    "checkWidth     : [opt] integer - width of checkmark.\n"
    "checkHeight    : [opt] integer - height of checkmark.\n"
    "alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.checkmark = xbmcgui.ControlCheckMark(100, 250, 200, 50, 'Status', font='font14')\n");


// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlCheckMark_Type;

  void initControlCheckMark_Type()
  {
    PyInitializeTypeObject(&ControlCheckMark_Type);

    ControlCheckMark_Type.tp_name = "xbmcgui.ControlCheckMark";
    ControlCheckMark_Type.tp_basicsize = sizeof(ControlCheckMark);
    ControlCheckMark_Type.tp_dealloc = (destructor)ControlCheckMark_Dealloc;
    ControlCheckMark_Type.tp_compare = 0;
    ControlCheckMark_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlCheckMark_Type.tp_doc = controlCheckMark__doc__;
    ControlCheckMark_Type.tp_methods = ControlCheckMark_methods;
    ControlCheckMark_Type.tp_base = &Control_Type;
    ControlCheckMark_Type.tp_new = ControlCheckMark_New;
  }
}

#ifdef __cplusplus
}
#endif
