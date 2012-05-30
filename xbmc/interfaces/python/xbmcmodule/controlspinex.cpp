/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIFontManager.h"
#include "control.h"
#include "pyutil.h"
#include "threads/SingleLock.h"

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
  PyObject* ControlSpinEx_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "x", "y", "width", "height", "label", "font", "textColor", "disabledColor", "type", NULL
    };
 
    ControlSpinEx *self;
    char* cFont = NULL;
    char* cTextColor = NULL;
    char* cDisabledColor = NULL;
    PyObject* pObjectText;

    self = (ControlSpinEx*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();
    new(&self->strText) string();
    new(&self->strTextureFocus) string();
    new(&self->strTextureNoFocus) string();
    new(&self->strTextureUp) string();
    new(&self->strTextureDown) string();
    new(&self->strTextureUpFocus) string();
    new(&self->strTextureDownFocus) string();

    // set up default values in case they are not supplied
    self->strFont = "font13";
    self->textColor = 0xffffffff;
    self->disabledColor = 0x60ffffff;
    self->iType=SPIN_CONTROL_TYPE_INT;
    self->align = (XBFONT_LEFT | XBFONT_CENTER_Y);
 
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llllO|sssssi",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &self->iType))
    {
      Py_DECREF( self );
      return NULL;
    }

    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    self->strTextureFocus = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"texturefocus", (char*)"button-focus.png");
    self->strTextureNoFocus = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"texturenofocus", (char*)"button-nofocus.png");
    self->strTextureUp = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"textureup", (char*)"scroll-up.png");
    self->strTextureDown = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"texturedown", (char*)"scroll-down.png");
    self->strTextureUpFocus = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"textureup", (char*)"scroll-up-focus.png");
    self->strTextureDownFocus = PyXBMCGetDefaultImage((char*)"spincontrolex", (char*)"textureup", (char*)"scroll-down-focus.png");
    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);
    if (cDisabledColor) sscanf(cDisabledColor, "%x", &self->disabledColor);

    return (PyObject*)self;
  }

  void ControlSpinEx_Dealloc(ControlSpinEx* self)
  {
    self->strText.~string();
    self->strFont.~string();
    self->strTextureFocus.~string();
    self->strTextureNoFocus.~string();
    self->strTextureUp.~string();
    self->strTextureDown.~string();
    self->strTextureUpFocus.~string();
    self->strTextureDownFocus.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlSpinEx_Create(ControlSpinEx* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->textColor;
    label.disabledColor = pControl->disabledColor;
    label.align = pControl->align;

    CLabelInfo spinlabel;
    spinlabel.font = g_fontManager.GetFont(pControl->strFont);
    spinlabel.textColor = pControl->textColor;
    spinlabel.disabledColor = pControl->disabledColor;
    spinlabel.align = pControl->align;

    pControl->pGUIControl = new CGUISpinControlEx(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      (float)pControl->dwSpinWidth,
      (float)pControl->dwSpinHeight,
      spinlabel,
      (CStdString)pControl->strTextureFocus,
      (CStdString)pControl->strTextureNoFocus,
      (CStdString)pControl->strTextureUp,
      (CStdString)pControl->strTextureDown,
      (CStdString)pControl->strTextureUpFocus,
      (CStdString)pControl->strTextureDownFocus,
      label,
      pControl->iType);

    CGUISpinControlEx* pGuiButtonControl =
      (CGUISpinControlEx*)pControl->pGUIControl;

    pGuiButtonControl->SetText(pControl->strText);
    pGuiButtonControl->SetValue(0);

    return pControl->pGUIControl;
  }

  // getType() Method
  PyDoc_STRVAR(getType__doc__,
    "getType() -- Returns the spin control's type.\n"
    "\n"
    "*Note, return value can be:\n"
    "       SPIN_CONTROL_TYPE_INT, SPIN_CONTROL_TYPE_FLOAT, SPIN_CONTROL_TYPE_TEXT, SPIN_CONTROL_TYPE_PAGE\n"
    "\n"
    "example:\n"
    "  - type = self.spincontrol.getType()\n");

  PyObject* ControlSpinEx_GetType(ControlSpinEx *self, PyObject *args)
  {
    if (!self->pGUIControl) return NULL;

    PyXBMCGUILock();
      int value = self->iType;
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"i", value);
  }

  // setType() Method
  PyDoc_STRVAR(setType__doc__,
    "setType(type) -- Sets the spinner control's type.\n"
    "\n"
    "type                : integer - Four types available.\n"
    "\n"
    "*Note, SPIN_CONTROL_TYPE_INT (default), SPIN_CONTROL_TYPE_FLOAT, SPIN_CONTROL_TYPE_TEXT, SPIN_CONTROL_TYPE_PAGE\n"
    "\n"
    "       You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - self.spinner.setType(type=xbmcgui.SPIN_CONTROL_TYPE_TEXT)\n");

  PyObject* ControlSpinEx_SetType(ControlSpinEx *self, PyObject *args, PyObject *kwds)
  {
    if (!self->pGUIControl) return NULL;

    int iType = -1;
    static const char *keywords[] = {"type", NULL};
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"i",
      (char**)keywords,
      &iType))
    {
      return NULL;
    }

    PyXBMCGUILock();
      if (self->pGUIControl && (iType >= SPIN_CONTROL_TYPE_INT && iType <= SPIN_CONTROL_TYPE_PAGE))
      {
        self->iType = iType;
        ((CGUISpinControlEx *)self->pGUIControl)->SetType(iType);
      }
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getValue() Method
  PyDoc_STRVAR(getValue__doc__,
    "getValue() -- Returns the spin control's current value.\n"
    "\n"
    "*Note, If type is xbmcgui.SPIN_CONTROL_TYPE_TEXT, getValue() returns the value as a unicode string.\n"
    "       otherwise getValue() returns the position as an integer.\n"
    "\n"
    "example:\n"
    "  - value = self.spincontrol.getValue()\n");

  PyObject* ControlSpinEx_GetValue(ControlSpinEx *self, PyObject *args)
  {
    if (!self->pGUIControl) return NULL;

    if (self->iType == SPIN_CONTROL_TYPE_TEXT)
    {
      PyXBMCGUILock();
        CStdString label = ((CGUISpinControlEx*)self->pGUIControl)->GetLabel();
      PyXBMCGUIUnlock();

      return PyUnicode_DecodeUTF8(label.c_str(), label.size(), "replace");
    }
    else
    {
      PyXBMCGUILock();
        int value = ((CGUISpinControlEx*)self->pGUIControl)->GetValue();
      PyXBMCGUIUnlock();

      return Py_BuildValue((char*)"i", value);
    }
  }

  // setValue() Method
  PyDoc_STRVAR(setValue__doc__,
    "setValue(value) -- Sets the spinner control's value.\n"
    "\n"
    "value               : unicode string or integer - value to set.\n"
    "\n"
    "*Note, may be an integer for position or a unicode string.\n"
    "\n"
    "       You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - self.spinner.setValue(value=u'Windows')\n");

  PyObject* ControlSpinEx_SetValue(ControlSpinEx *self, PyObject *args, PyObject *kwds)
  {
    if (!self->pGUIControl) return NULL;

    PyObject* pObject;
    CStdString value;
    static const char *keywords[] = {"value", NULL};
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pObject))
    {
      return NULL;
    }

    if (PyInt_CheckExact(pObject))
    {
      PyXBMCGUILock();
        ((CGUISpinControlEx*)self->pGUIControl)->SetValue((int)PyInt_AsLong(pObject));
      PyXBMCGUIUnlock();
    }
    else if (PyXBMCGetUnicodeString(value, pObject, 1))
    {
      PyXBMCGUILock();
        ((CGUISpinControlEx*)self->pGUIControl)->SetValueFromLabel(value);
      PyXBMCGUIUnlock();
    }
    else
      return NULL;

    Py_INCREF(Py_None);
    return Py_None;
  }

  // addItems() method
  PyDoc_STRVAR(addItems__doc__,
    "addItems(items) -- Adds a list of strings to this spinner control.\n"
    "\n"
    "items                : List - list of unicode strings to add.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - spinner.addItems(items=[u'Windows', u'OSX', u'Linux'])\n");

  PyObject* ControlSpinEx_AddItems(ControlSpinEx *self, PyObject *args, PyObject *kwds)
  {
    if (!self->pGUIControl) return NULL;

    PyObject *pList = NULL;
    static const char *keywords[] = {"items", NULL};
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pList) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    CSingleLock lock(g_graphicsContext);

    ((CGUISpinControlEx *)self->pGUIControl)->Clear();

    CStdString label;
    for (int item = 0; item < PyList_Size(pList); item++)
    {
      PyObject *pItem = PyList_GetItem(pList, item);
      if (!PyXBMCGetUnicodeString(label, pItem, 1)) return NULL;
      ((CGUISpinControlEx *)self->pGUIControl)->AddLabel(label, item);
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlSpinEx_methods[] = {
    {(char*)"getType", (PyCFunction)ControlSpinEx_GetType, METH_VARARGS, getType__doc__},
    {(char*)"setType", (PyCFunction)ControlSpinEx_SetType, METH_VARARGS|METH_KEYWORDS, setType__doc__},
    {(char*)"getValue", (PyCFunction)ControlSpinEx_GetValue, METH_VARARGS, getValue__doc__},
    {(char*)"setValue", (PyCFunction)ControlSpinEx_SetValue, METH_VARARGS|METH_KEYWORDS, setValue__doc__},
    {(char*)"addItems", (PyCFunction)ControlSpinEx_AddItems, METH_VARARGS|METH_KEYWORDS, addItems__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlSpinEx class
  PyDoc_STRVAR(ControlSpinEx__doc__,
    "ControlSpinEx class.\n"
    "\n"
    "ControlSpinEx(x, y, width, height, label[, font, textColor, disabledColor, type]\n"
    "\n"
    "x                   : integer - x coordinate of control.\n"
    "y                   : integer - y coordinate of control.\n"
    "width               : integer - width of control.\n"
    "height              : integer - height of control.\n"
    "label               : string or unicode - text string.\n"
    "font                : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor           : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor       : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n"
    "type                : [opt] integer - type of spinner. (e.g. xbmcgui.SPIN_CONTROL_TYPE_TEXT)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.spinner = xbmcgui.ControlSpinEx(100, 250, 500, 40, 'Trailer quality', type=xbmcgui.SPIN_CONTROL_TYPE_TEXT)\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlSpinEx_Type;

  void initControlSpinEx_Type()
  {
    PyXBMCInitializeTypeObject(&ControlSpinEx_Type);

    ControlSpinEx_Type.tp_name = (char*)"xbmcgui.ControlSpinEx";
    ControlSpinEx_Type.tp_basicsize = sizeof(ControlSpinEx);
    ControlSpinEx_Type.tp_dealloc = (destructor)ControlSpinEx_Dealloc;
    ControlSpinEx_Type.tp_compare = 0;
    ControlSpinEx_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlSpinEx_Type.tp_doc = ControlSpinEx__doc__;
    ControlSpinEx_Type.tp_methods = ControlSpinEx_methods;
    ControlSpinEx_Type.tp_base = &Control_Type;
    ControlSpinEx_Type.tp_new = ControlSpinEx_New;
  }
}

#ifdef __cplusplus
}
#endif
