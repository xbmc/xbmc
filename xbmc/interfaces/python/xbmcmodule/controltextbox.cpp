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
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
  #include "python/Include/Python.h"
#endif
#include "../XBPythonDll.h"
#include "guilib/GUITextBox.h"
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
  extern PyObject* ControlSpin_New(void);

  PyObject* ControlTextBox_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "x", "y", "width", "height", "font", "textColor", NULL };
    ControlTextBox *self;
    char *cFont = NULL;
    char *cTextColor = NULL;

    self = (ControlTextBox*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    new(&self->strFont) string();

    // parse arguments to constructor
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"llll|ss",
      (char**)keywords,
      &self->dwPosX,
      &self->dwPosY,
      &self->dwWidth,
      &self->dwHeight,
      &cFont,
      &cTextColor))
    {
      Py_DECREF( self );
      return NULL;
    }

    // set default values if needed
    self->strFont = cFont ? cFont : "font13";

    if (cTextColor) sscanf(cTextColor, "%x", &self->textColor);
    else self->textColor = 0xffffffff;

    return (PyObject*)self;
  }

  void ControlTextBox_Dealloc(ControlTextBox* self)
  {
    //Py_DECREF(self->pControlSpin);
    self->strFont.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  CGUIControl* ControlTextBox_Create(ControlTextBox* pControl)
  {
    // create textbox
    CLabelInfo label;
    label.font = g_fontManager.GetFont(pControl->strFont);
    label.textColor = label.focusedColor = pControl->textColor;

    pControl->pGUIControl = new CGUITextBox(pControl->iParentId, pControl->iControlId,
      (float)pControl->dwPosX, (float)pControl->dwPosY, (float)pControl->dwWidth, (float)pControl->dwHeight,
      label);

    // reset textbox
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
    pControl->pGUIControl->OnMessage(msg);

    return pControl->pGUIControl;
  }

  // SetText() Method
  PyDoc_STRVAR(setText__doc__,
    "setText(text) -- Set's the text for this textbox.\n"
    "\n"
    "text           : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.textbox.setText('This is a line of text that can wrap.')");


  PyObject* ControlTextBox_SetText(ControlTextBox *self, PyObject *args)
  {
    PyObject *pObjectText;
    string strText;
    if (!PyArg_ParseTuple(args, (char*)"O", &pObjectText)) return NULL;
    if (!PyXBMCGetUnicodeString(strText, pObjectText, 1)) return NULL;

    // create message
    ControlTextBox *pControl = (ControlTextBox*)self;
    CGUIMessage msg(GUI_MSG_LABEL_SET, pControl->iParentId, pControl->iControlId);
    msg.SetLabel(strText);

    // send message
    g_windowManager.SendThreadMessage(msg, pControl->iParentId);
    Py_INCREF(Py_None);
    return Py_None;
  }

  // scroll() Method
  PyDoc_STRVAR(Scroll__doc__,
    "scroll(position) -- Scrolls to the given position.\n"
    "\n"
    "id           : integer - position to scroll to.\n"
    "\n"
    "example:\n"
    "  - self.textbox.scroll(10)");

  PyObject* ControlTextBox_Scroll(ControlTextBox *self, PyObject *args)
  {
    int position = 0;
    if (!PyArg_ParseTuple(args, (char*)"l", &position))
      return NULL;

    ControlTextBox *pControl = (ControlTextBox*)self;
    static_cast<CGUITextBox*>(pControl->pGUIControl)->Scroll(position);

    return Py_None;
  }

  // reset() Method
  PyDoc_STRVAR(reset__doc__,
    "reset() -- Clear's this textbox.\n"
    "\n"
    "example:\n"
    "  - self.textbox.reset()\n");

  PyObject* ControlTextBox_Reset(ControlTextBox *self, PyObject *args)
  {
    // create message
    ControlTextBox *pControl = (ControlTextBox*)self;
    CGUIMessage msg(GUI_MSG_LABEL_RESET, pControl->iParentId, pControl->iControlId);
    g_windowManager.SendThreadMessage(msg, pControl->iParentId);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ControlTextBox_methods[] = {
    {(char*)"setText", (PyCFunction)ControlTextBox_SetText, METH_VARARGS, setText__doc__},
    {(char*)"reset", (PyCFunction)ControlTextBox_Reset, METH_VARARGS, reset__doc__},
    {(char*)"scroll",(PyCFunction)ControlTextBox_Scroll, METH_VARARGS, Scroll__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(controlTextBox__doc__,
    "ControlTextBox class.\n"
    "\n"
    "ControlTextBox(x, y, width, height[, font, textColor])\n"
    "\n"
    "x              : integer - x coordinate of control.\n"
    "y              : integer - y coordinate of control.\n"
    "width          : integer - width of control.\n"
    "height         : integer - height of control.\n"
    "font           : [opt] string - font used for text. (e.g. 'font13')\n"
    "textColor      : [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "       After you create the control, you need to add it to the window with addControl().\n"
    "\n"
    "example:\n"
    "  - self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ControlTextBox_Type;

  void initControlTextBox_Type()
  {
    PyXBMCInitializeTypeObject(&ControlTextBox_Type);

    ControlTextBox_Type.tp_name = (char*)"xbmcgui.ControlTextBox";
    ControlTextBox_Type.tp_basicsize = sizeof(ControlTextBox);
    ControlTextBox_Type.tp_dealloc = (destructor)ControlTextBox_Dealloc;
    ControlTextBox_Type.tp_compare = 0;
    ControlTextBox_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ControlTextBox_Type.tp_doc = controlTextBox__doc__;
    ControlTextBox_Type.tp_methods = ControlTextBox_methods;
    ControlTextBox_Type.tp_base = &Control_Type;
    ControlTextBox_Type.tp_new = ControlTextBox_New;
  }
}

#ifdef __cplusplus
}
#endif
