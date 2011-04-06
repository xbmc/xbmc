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

#include "../XBPythonDll.h"
#include "guilib/GUICheckMarkControl.h"
#include "guilib/GUIFontManager.h"
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
  PyObject* ControlCheckMark_New(PyTypeObject *type, PyObject *args, PyObject *kwds )
  {
    static const char *keywords[] = {
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
    self->checkWidth = 30;
    self->checkHeight = 30;
    self->align = XBFONT_RIGHT;
    self->strFont = "font13";
    self->textColor = 0xffffffff;
    self->disabledColor = 0x60ffffff;

    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llllO|sslllsss:ControlCheckMark",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cTextureFocus,
      &cTextureNoFocus,
      &self->checkWidth,
      &self->checkHeight,
      &self->align,
      &cFont,
      &cTextColor,
      &cDisabledColor ))
    {
      Py_DECREF( self );
      return NULL;
    }
    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);
    if (cDisabledColor)
    {
      sscanf( cDisabledColor, "%x", &self->disabledColor );
    }
    self->strTextureFocus = cTextureFocus ?
      cTextureFocus :
      PyXBMCGetDefaultImage((char*)"checkmark", (char*)"texturefocus", (char*)"check-box.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyXBMCGetDefaultImage((char*)"checkmark", (char*)"texturenofocus", (char*)"check-boxNF.png");

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
    label.disabledColor = pControl->disabledColor;
    label.textColor = label.focusedColor = pControl->textColor;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.align = pControl->align;
    CTextureInfo imageFocus(pControl->strTextureFocus);
    CTextureInfo imageNoFocus(pControl->strTextureNoFocus);
    pControl->pGUIControl = new CGUICheckMarkControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      imageFocus, imageNoFocus,
      (float)pControl->checkWidth,
      (float)pControl->checkHeight,
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

    if (!PyArg_ParseTuple(args, (char*)"s", &cDisabledColor)) return NULL;

    if (cDisabledColor)
    {
      sscanf(cDisabledColor, "%x", &self->disabledColor);
    }

    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetDisabledColor( self->disabledColor );
    }
    PyXBMCGUIUnlock();

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
      args, (char*)"O|sss",
      &pObjectText, &cFont,
      &cTextColor,  &cDisabledColor))
      return NULL;

    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 1))
      return NULL;

    if (cFont) self->strFont = cFont;
    if (cTextColor)
    {
      sscanf(cTextColor, "%x", &self->textColor);
    }
    if (cDisabledColor)
    {
      sscanf(cDisabledColor, "%x", &self->disabledColor);
    }

    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetLabel(
        self->strFont,
        self->strText,
        self->textColor );
      ((CGUICheckMarkControl*)self->pGUIControl)->PythonSetDisabledColor(
        self->disabledColor );
    }
    PyXBMCGUIUnlock();

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

    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      isSelected = ((CGUICheckMarkControl*)self->pGUIControl)->GetSelected();
    }
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", isSelected);
  }

  // setSelected() Method
  PyDoc_STRVAR(setSelected__doc__,
    "setSelected(isOn) -- Sets this checkmark status to on or off.\n"
    "\n"
    "isOn           : bool - True=selected (on) / False=not selected (off)\n"
    "\n"
    "example:\n"
    "  - self.checkmark.setSelected(True)\n");

  PyObject* ControlCheckMark_SetSelected(ControlCheckMark *self, PyObject *args)
  {
    char isSelected = 0;

    if (!PyArg_ParseTuple(args, (char*)"b", &isSelected))
      return NULL;

    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      ((CGUICheckMarkControl*)self->pGUIControl)->SetSelected(0 != isSelected);
    }
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlCheckMark_methods[] = {
    {(char*)"getSelected", (PyCFunction)ControlCheckMark_GetSelected, METH_NOARGS, getSelected__doc__},
    {(char*)"setSelected", (PyCFunction)ControlCheckMark_SetSelected, METH_VARARGS, setSelected__doc__},
    {(char*)"setLabel", (PyCFunction)ControlCheckMark_SetLabel, METH_VARARGS, setLabel__doc__},
    {(char*)"setDisabledColor", (PyCFunction)ControlCheckMark_SetDisabledColor, METH_VARARGS, setDisabledColor__doc__},
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
    PyXBMCInitializeTypeObject(&ControlCheckMark_Type);

    ControlCheckMark_Type.tp_name = (char*)"xbmcgui.ControlCheckMark";
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
