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

#include "GUIPythonWindow.h"
#include "GUIPythonWindowXML.h"
#include "GUIPythonWindowXMLDialog.h"
#include "GUIPythonWindowDialog.h"
#include "control.h"

#pragma once

#define Window_Check(op) PyObject_TypeCheck(op, &Window_Type)
#define Window_CheckExact(op) ((op)->ob_type == &Window_Type)

#define WindowXML_Check(op) PyObject_TypeCheck(op, &WindowXML_Type)
#define WindowXML_CheckExact(op) ((op)->ob_type == &WindowXML_Type)

#define WindowDialog_Check(op) PyObject_TypeCheck(op, &WindowDialog_Type)
#define WindowDialog_CheckExact(op) ((op)->ob_type == &WindowDialog_Type)

#define WindowXMLDialog_Check(op) PyObject_TypeCheck(op, &WindowXMLDialog_Type)
#define WindowXMLDialog_CheckExact(op) ((op)->ob_type == &WindowXMLDialog_Type)

#define PyObject_HEAD_XBMC_WINDOW \
    PyObject_HEAD                 \
    int iWindowId;                \
    int iOldWindowId;             \
    int iCurrentControlId;        \
    bool bIsPythonWindow;         \
    bool bModal;                  \
    bool bUsingXML;               \
    std::string sXMLFileName;     \
    std::string sFallBackPath;    \
    CGUIWindow* pWindow;          \
    std::vector<Control*> vecControls;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD_XBMC_WINDOW
  } Window;

  extern PyMethodDef Window_methods[];
  extern PyTypeObject Window_Type;

  void initWindow_Type();

  bool Window_CreateNewWindow(Window* pWindow, bool bAsDialog);
  void Window_Dealloc(Window* self);
  PyObject* Window_Close(Window *self, PyObject *args);
}

#ifdef __cplusplus
}
#endif
