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
#include "guilib/GUIFadeLabelControl.h"
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
  PyObject* ControlFadeLabel_New(PyTypeObject *type,
    PyObject *args,
    PyObject *kwds )
  {
    static const char *keywords[] = {
      "x", "y", "width", "height", "font", "textColor", "alignment", NULL };

    ControlFadeLabel *self;
    char *cFont = NULL;
    char *cTextColor = NULL;

    self = (ControlFadeLabel*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strFont) string();
    new(&self->vecLabels) std::vector<string>();

    // set up default values in case they are not supplied
    self->strFont = "font13";
    self->textColor = 0xffffffff;
    self->align = XBFONT_LEFT;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llll|ssl",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cFont,
      &cTextColor,
      &self->align ))
    {
      Py_DECREF( self );
      return NULL;
    }

    if (cFont) self->strFont = cFont;
    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);

    self->pGUIControl = NULL;

    return (PyObject*)self;
  }

  void ControlFadeLabel_Dealloc(Control* self)
  {
    ControlFadeLabel *pControl = (ControlFadeLabel*)self;
    pControl->vecLabels.clear();
    pControl->vecLabels.~vector();
    pControl->strFont.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlFadeLabel_Create(ControlFadeLabel* pControl)
  {
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->textColor;
    label.align = pControl->align;
    pControl->pGUIControl = new CGUIFadeLabelControl(
      pControl->iParentId,
      pControl->iControlId,
      (float)pControl->dwPosX,
      (float)pControl->dwPosY,
      (float)pControl->dwWidth,
      (float)pControl->dwHeight,
      label,
      true,
      0,
      true);

    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
    pControl->pGUIControl->OnMessage(msg);

    return pControl->pGUIControl;
  }

  // addLabel() Method
  PyDoc_STRVAR(addLabel__doc__,
    "addLabel(label) -- Add a label to this control for scrolling.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.fadelabel.addLabel('This is a line of text that can scroll.')");

  PyObject* ControlFadeLabel_AddLabel(ControlFadeLabel *self, PyObject *args)
  {
    PyObject *pObjectText;
    string strText;

    if (!PyArg_ParseTuple(args, (char*)"O", &pObjectText))   return NULL;
    if (!PyXBMCGetUnicodeString(strText, pObjectText, 1)) return NULL;

    ControlFadeLabel *pControl = (ControlFadeLabel*)self;
    CGUIMessage msg(GUI_MSG_LABEL_ADD, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(strText);

    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  // reset() Method
  PyDoc_STRVAR(reset__doc__,
    "reset() -- Clears this fadelabel.\n"
    "\n"
    "example:\n"
    "  - self.fadelabel.reset()\n");

  PyObject* ControlFadeLabel_Reset(ControlFadeLabel *self, PyObject *args)
  {
    ControlFadeLabel *pControl = (ControlFadeLabel*)self;
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);

    pControl->vecLabels.clear();
    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlFadeLabel_methods[] = {
    {(char*)"addLabel", (PyCFunction)ControlFadeLabel_AddLabel, METH_VARARGS, addLabel__doc__},
    {(char*)"reset", (PyCFunction)ControlFadeLabel_Reset, METH_VARARGS, reset__doc__},
    {NULL, NULL, 0, NULL}
  };

  // ControlFadeLabel class
  PyDoc_STRVAR(controlFadeLabel__doc__,
    "ControlFadeLabel class.\n"
    "Control that scroll's lables"
    "\n"
    "ControlFadeLabel(x, y, width, height[, font, textColor, alignment])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "font           : [opt] string - font used for label text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')\n"
    "alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.fadelabel = xbmcgui.ControlFadeLabel(100, 250, 200, 50, textColor='0xFFFFFFFF')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlFadeLabel_Type;

  void initControlFadeLabel_Type()
  {
    PyXBMCInitializeTypeObject(&ControlFadeLabel_Type);

    ControlFadeLabel_Type.tp_name = (char*)"xbmcgui.ControlFadeLabel";
    ControlFadeLabel_Type.tp_basicsize = sizeof(ControlFadeLabel);
    ControlFadeLabel_Type.tp_dealloc = (destructor)ControlFadeLabel_Dealloc;
    ControlFadeLabel_Type.tp_compare = 0;
    ControlFadeLabel_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlFadeLabel_Type.tp_doc = controlFadeLabel__doc__;
    ControlFadeLabel_Type.tp_methods = ControlFadeLabel_methods;
    ControlFadeLabel_Type.tp_base = &Control_Type;
    ControlFadeLabel_Type.tp_new = ControlFadeLabel_New;
  }
}

#ifdef __cplusplus
}
#endif
