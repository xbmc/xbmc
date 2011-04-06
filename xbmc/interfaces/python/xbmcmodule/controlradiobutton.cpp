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
#include "guilib/GUIRadioButtonControl.h"
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
  PyObject* ControlRadioButton_New(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwds )
  {
    static const char *keywords[] = {
      "x", "y", "width", "height", "label",
      "focusTexture", "noFocusTexture",
      "textOffsetX", "textOffsetY", "alignment",
      "font", "textColor", "disabledColor", "angle", "shadowColor", "focusedColor",
      "TextureRadioFocus", "TextureRadioNoFocus", NULL };
    ControlRadioButton *self;
    char* cFont = NULL;
    char* cTextureFocus = NULL;
    char* cTextureNoFocus = NULL;
    char* cTextColor = NULL;
    char* cDisabledColor = NULL;
    char* cShadowColor = NULL;
    char* cFocusedColor = NULL;
    char* cTextureRadioFocus = NULL;
    char* cTextureRadioNoFocus = NULL;

    PyObject* pObjectText;

    self = (ControlRadioButton*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();
    new(&self->strText) string();
    new(&self->strTextureFocus) string();
    new(&self->strTextureNoFocus) string();
    new(&self->strTextureRadioFocus) string();
    new(&self->strTextureRadioNoFocus) string();

    // set up default values in case they are not supplied
    self->textOffsetX = CONTROL_TEXT_OFFSET_X;
    self->textOffsetY = CONTROL_TEXT_OFFSET_Y;
    self->align = (XBFONT_LEFT | XBFONT_CENTER_Y);
    self->strFont = "font13";
    self->textColor = 0xffffffff;
    self->disabledColor = 0x60ffffff;
    self->iAngle = 0;
    self->shadowColor = 0;
    self->focusedColor = 0xffffffff;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llllO|sslllssslssss",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &pObjectText,
      &cTextureFocus,
      &cTextureNoFocus,
      &self->textOffsetX,
      &self->textOffsetY,
      &self->align,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &self->iAngle,
      &cShadowColor,
      &cFocusedColor,
      &cTextureRadioFocus,
      &cTextureRadioNoFocus))
    {
      Py_DECREF( self );
      return NULL;
    }

    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 5))
    {
      Py_DECREF( self );
      return NULL;
    }

    // if texture is supplied use it, else get default ones
    self->strTextureFocus = cTextureFocus ?
      cTextureFocus :
      PyXBMCGetDefaultImage((char*)"radiobutton", (char*)"texturefocus", (char*)"radiobutton-focus.png");
    self->strTextureNoFocus = cTextureNoFocus ?
      cTextureNoFocus :
      PyXBMCGetDefaultImage((char*)"radiobutton", (char*)"texturenofocus", (char*)"radiobutton-nofocus.jpg");
    self->strTextureRadioFocus = cTextureRadioFocus ?
      cTextureRadioFocus :
      PyXBMCGetDefaultImage((char*)"radiobutton", (char*)"textureradiofocus", (char*)"radiobutton-focus.png");
    self->strTextureRadioNoFocus = cTextureRadioNoFocus ?
      cTextureRadioNoFocus :
      PyXBMCGetDefaultImage((char*)"radiobutton", (char*)"textureradionofocus", (char*)"radiobutton-nofocus.jpg");

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf( cTextColor, "%x", &self->textColor );
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->disabledColor );
    if (cShadowColor) sscanf( cShadowColor, "%x", &self->shadowColor );
    if (cFocusedColor) sscanf( cFocusedColor, "%x", &self->focusedColor );
    return (PyObject*)self;
  }

  void ControlRadioButton_Dealloc(ControlRadioButton* self)
  {
    self->strFont.~string();
    self->strText.~string();
    self->strTextureFocus.~string();
    self->strTextureNoFocus.~string();
    self->strTextureRadioFocus.~string();
    self->strTextureRadioNoFocus.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlRadioButton_Create(ControlRadioButton* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = pControl->textColor;
    label.disabledColor = pControl->disabledColor;
    label.shadowColor = pControl->shadowColor;
    label.focusedColor = pControl->focusedColor;
    label.align = pControl->align;
    label.offsetX = (float)pControl->textOffsetX;
    label.offsetY = (float)pControl->textOffsetY;
    label.angle = (float)-pControl->iAngle;
    pControl->pGUIControl = new CGUIRadioButtonControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      (CStdString)pControl->strTextureFocus,
      (CStdString)pControl->strTextureNoFocus,
      label,
      (CStdString)pControl->strTextureRadioFocus,
      (CStdString)pControl->strTextureRadioNoFocus);

    CGUIRadioButtonControl* pGuiButtonControl =
      (CGUIRadioButtonControl*)pControl->pGUIControl;

    pGuiButtonControl->SetLabel(pControl->strText);

    return pControl->pGUIControl;
  }

  // setSelected() Method
  PyDoc_STRVAR(setSelected__doc__,
    "setSelected(selected) -- Sets the radio buttons's selected status.\n"
    "\n"
    "selected            : bool - True=selected (on) / False=not selected (off)\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setSelected(True)\n");

  PyObject* ControlRadioButton_SetSelected(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "selected",
      NULL};

    char selected = false;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"b",
      (char**)keywords,
      &selected))
    {
      return NULL;
    }

    PyXBMCGUILock();
    if (self->pGUIControl)
      ((CGUIRadioButtonControl*)self->pGUIControl)->SetSelected(0 != selected);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // isSelected() Method
  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the radio buttons's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.radiobutton.isSelected()\n");

  PyObject* ControlRadioButton_IsSelected(ControlRadioButton *self, PyObject *args)
  {
    bool isSelected = false;

    PyXBMCGUILock();
    if (self->pGUIControl)
      isSelected = ((CGUIRadioButtonControl*)self->pGUIControl)->IsSelected();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", isSelected);
  }

  // setLabel() Method
  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's the radio buttons text attributes.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor  : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n"
    "shadowColor    : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor   : [opt] hexstring - color of focused radio button's label. (e.g. '0xFFFFFF00')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')\n");

  PyObject* ControlRadioButton_SetLabel(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "label",
      "font",
      "textColor",
      "disabledColor",
      "shadowColor",
      "focusedColor",
      NULL};
    char *cFont = NULL;
    char *cTextColor = NULL;
    char *cDisabledColor = NULL;
    char *cShadowColor = NULL;
    char *cFocusedColor = NULL;
    PyObject *pObjectText = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O|sssss",
      (char**)keywords,
      &pObjectText,
      &cFont,
      &cTextColor,
      &cDisabledColor,
      &cShadowColor,
      &cFocusedColor))
    {
      return NULL;
    }

    if (!PyXBMCGetUnicodeString(self->strText, pObjectText, 1))
    {
      return NULL;
    }

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);
    if (cDisabledColor) sscanf( cDisabledColor, "%x", &self->disabledColor );
    if (cShadowColor) sscanf(cShadowColor, "%x", &self->shadowColor);
    if (cFocusedColor) sscanf(cFocusedColor, "%x", &self->focusedColor);

    PyXBMCGUILock();
    if (self->pGUIControl)
    {
      ((CGUIRadioButtonControl*)self->pGUIControl)->PythonSetLabel(
        self->strFont, self->strText, self->textColor, self->shadowColor, self->focusedColor );
      ((CGUIRadioButtonControl*)self->pGUIControl)->PythonSetDisabledColor(self->disabledColor);
    }
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setRadioDimension() Method
  PyDoc_STRVAR(setRadioDimension__doc__,
    "setRadioDimension(x, y, width, height) -- Sets the radio buttons's radio texture's position and size.\n"
    "\n"
    "x                   : integer - x coordinate of radio texture.\n"
    "y                   : integer - y coordinate of radio texture.\n"
    "width               : integer - width of radio texture.\n"
    "height              : integer - height of radio texture.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)\n");

  PyObject* ControlRadioButton_SetRadioDimension(ControlRadioButton *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = {
      "x",
      "y",
      "width",
      "height",
      NULL};

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llll",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight))
    {
      return NULL;
    }

    PyXBMCGUILock();
    if (self->pGUIControl)
      ((CGUIRadioButtonControl*)self->pGUIControl)->SetRadioDimensions((float)self->dwPosX, (float)self->dwPosY, (float)self->dwWidth, (float)self->dwHeight);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlRadioButton_methods[] = {
    {(char*)"setSelected", (PyCFunction)ControlRadioButton_SetSelected, METH_VARARGS|METH_KEYWORDS, setSelected__doc__},
    {(char*)"isSelected", (PyCFunction)ControlRadioButton_IsSelected, METH_VARARGS, isSelected__doc__},
    {(char*)"setLabel", (PyCFunction)ControlRadioButton_SetLabel, METH_VARARGS|METH_KEYWORDS, setLabel__doc__},
    {(char*)"setRadioDimension", (PyCFunction)ControlRadioButton_SetRadioDimension, METH_VARARGS|METH_KEYWORDS, setRadioDimension__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlRadioButton class
  PyDoc_STRVAR(ControlRadioButton__doc__,
    "ControlRadioButton class.\n"
    "\n"
    "ControlRadioButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,\n"
    "              alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor,\n"
    "              radioFocusTexture, noRadioFocusTexture])\n"
    "\n"
    "x                   : integer - x coordinate of control.\n"
    "y                   : integer - y coordinate of control.\n"
    "width               : integer - width of control.\n"
    "height              : integer - height of control.\n"
    "label               : string or unicode - text string.\n"
    "focusTexture        : [opt] string - filename for focus texture.\n"
    "noFocusTexture      : [opt] string - filename for no focus texture.\n"
    "textOffsetX         : [opt] integer - x offset of label.\n"
    "textOffsetY         : [opt] integer - y offset of label.\n"
    "alignment           : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "font                : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor           : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n"
    "disabledColor       : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n"
    "angle               : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)\n"
    "shadowColor         : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n"
    "focusedColor        : [opt] hexstring - color of focused radio button's label. (e.g. '0xFF00FFFF')\n"
    "radioFocusTexture   : [opt] string - filename for radio focus texture.\n"
    "noRadioFocusTexture : [opt] string - filename for radio no focus texture.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.radiobutton = xbmcgui.ControlRadioButton(100, 250, 200, 50, 'Status', font='font14')\n");

  // Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlRadioButton_Type;

  void initControlRadioButton_Type()
  {
    PyXBMCInitializeTypeObject(&ControlRadioButton_Type);

    ControlRadioButton_Type.tp_name = (char*)"xbmcgui.ControlRadioButton";
    ControlRadioButton_Type.tp_basicsize = sizeof(ControlRadioButton);
    ControlRadioButton_Type.tp_dealloc = (destructor)ControlRadioButton_Dealloc;
    ControlRadioButton_Type.tp_compare = 0;
    ControlRadioButton_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlRadioButton_Type.tp_doc = ControlRadioButton__doc__;
    ControlRadioButton_Type.tp_methods = ControlRadioButton_methods;
    ControlRadioButton_Type.tp_base = &Control_Type;
    ControlRadioButton_Type.tp_new = ControlRadioButton_New;
  }
}

#ifdef __cplusplus
}
#endif
