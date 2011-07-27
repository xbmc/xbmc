/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "guilib/GUIEditControl.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIWindowManager.h"
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
  PyObject* ControlEdit_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "x", "y", "width", "height", "label", "font", "textColor",
      "disabledColor", "alignment", "focusTexture", "noFocusTexture", "isPassword", NULL };

    ControlEdit *self;
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cDisabledColor = NULL;
    PyObject* pObjectText;
    char* cTextureFocus = NULL;
    char* cTextureNoFocus = NULL;

    self = (ControlEdit*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strText) string();
    new(&self->strFont) string();
    new(&self->strTextureFocus) string();
    new(&self->strTextureNoFocus) string();

    // set up default values in case they are not supplied
    self->strFont = "font13";
    self->textColor = 0xffffffff;
    self->disabledColor = 0x60ffffff;
    self->align = XBFONT_LEFT;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llllO|ssslssb",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &self->align,
      &cTextureFocus,
      &cTextureNoFocus,
      &self->bIsPassword))
    {
        Py_DECREF( self );
        return NULL;
    }

    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    self->strTextureFocus = cTextureFocus ?
      cTextureFocus :
      PyXBMCGetDefaultImage((char*)"edit", (char*)"texturefocus", (char*)"button-focus.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyXBMCGetDefaultImage((char*)"edit", (char*)"texturenofocus", (char*)"button-nofocus.jpg");


    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);
    if (cDisabledColor)
    {
      sscanf( cDisabledColor, "%x", &self->disabledColor );
    }

    return (PyObject*)self;
  }

  void ControlEdit_Dealloc(ControlLabel* self)
  {
    self->strText.~string();
    self->strFont.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlEdit_Create(ControlEdit* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->textColor;
    label.disabledColor = pControl->disabledColor;
    label.align = pControl->align;

    pControl->pGUIControl = new CGUIEditControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      (CStdString)pControl->strTextureFocus,
      (CStdString)pControl->strTextureNoFocus,
      label,
      pControl->strText);
    if (pControl->bIsPassword)
      ((CGUIEditControl *) pControl->pGUIControl)->SetInputType(CGUIEditControl::INPUT_TYPE_PASSWORD, 0);
    return pControl->pGUIControl;
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Set's text heading for this edit control.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.edit.setLabel('Status')\n");

  PyObject* ControlEdit_SetLabel(ControlLabel *self, PyObject *args)
  {
    PyObject *pObjectText;

    if (!PyArg_ParseTuple(args, (char*)"O", &pObjectText)) return NULL;
    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 1)) return NULL;

    ControlLabel *pControl = (ControlLabel*)self;
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(self->strText);
    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getLabel() Method
  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the text heading for this edit control.\n"
    "\n"
    "example:\n"
    "  - label = self.edit.getLabel()\n");

  PyObject* ControlEdit_GetLabel(ControlLabel *self, PyObject *args)
  {
    if (!self->pGUIControl) return NULL;
    return Py_BuildValue((char*)"s", self->strText.c_str());
  }

  // setLabel() Method
  PyDoc_STRVAR(setText__doc__,
    "setText(value) -- Set's text value for this edit control.\n"
    "\n"
    "value          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.edit.setText('online')\n");

  PyObject* ControlEdit_SetText(ControlLabel *self, PyObject *args)
  {
    PyObject *pObjectText;
    std::string strValue;
    if (!PyArg_ParseTuple(args, (char*)"O", &pObjectText)) return NULL;
    if (!PyXBMCGetUnicodeString(strValue, pObjectText, 1)) return NULL;

    ControlLabel *pControl = (ControlLabel*)self;
    CGUIMessage msg(GUI_MSG_LABEL2_SET, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(strValue);
    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

    // getText() Method
  PyDoc_STRVAR(getText__doc__,
    "getText() -- Returns the text value for this edit control.\n"
    "\n"
    "example:\n"
    "  - value = self.edit.getText()\n");

  PyObject* ControlEdit_GetText(ControlLabel *self, PyObject *args)
  {
    if (!self->pGUIControl) return NULL;

    ControlLabel *pControl = (ControlLabel*)self;
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, pControl->iParentId, pControl->iControlId);
    g_windowManager.SendMessage(msg, pControl->iParentId);

    return Py_BuildValue((char*)"s", msg.GetLabel().c_str());
  }

  PyMethodDef ControlEdit_methods[] = {
    {(char*)"setLabel", (PyCFunction)ControlEdit_SetLabel, METH_VARARGS, setLabel__doc__},
    {(char*)"getLabel", (PyCFunction)ControlEdit_GetLabel, METH_VARARGS, getLabel__doc__},
    {(char*)"setText", (PyCFunction)ControlEdit_SetText, METH_VARARGS, setText__doc__},
    {(char*)"getText", (PyCFunction)ControlEdit_GetText, METH_VARARGS, getText__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlEdit class
  PyDoc_STRVAR(controlEdit__doc__,
    "ControlEdit class.\n"
    "\n"
    "ControlEdit(x, y, width, height, label[, font, textColor, \n"
    "             disabledColor, alignment, focusTexture, noFocusTexture])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')\n"
    "alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "focusTexture   : [opt] string - filename for focus texture.\n"
    "noFocusTexture : [opt] string - filename for no focus texture.\n"
    "isPassword     : [opt] bool - if true, mask text value.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.edit = xbmcgui.ControlEdit(100, 250, 125, 75, 'Status')\n");

  // Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlEdit_Type;

  void initControlEdit_Type()
  {
    PyXBMCInitializeTypeObject(&ControlEdit_Type);

    ControlEdit_Type.tp_name = (char*)"xbmcgui.ControlEdit";
    ControlEdit_Type.tp_basicsize = sizeof(ControlEdit);
    ControlEdit_Type.tp_dealloc = (destructor)ControlEdit_Dealloc;
    ControlEdit_Type.tp_compare = 0;
    ControlEdit_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlEdit_Type.tp_doc = controlEdit__doc__;
    ControlEdit_Type.tp_methods = ControlEdit_methods;
    ControlEdit_Type.tp_base = &Control_Type;
    ControlEdit_Type.tp_new = ControlEdit_New;
  }
}

#ifdef __cplusplus
}
#endif
