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

#include "keyboard.h"
#include "pythreadstate.h"
#include "pyutil.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "Application.h"

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* Keyboard_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Keyboard *self;

    self = (Keyboard*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strDefault) string();
    new(&self->strHeading) string();

    PyObject *line = NULL;
    PyObject *heading = NULL;
    char bHidden = false;
    if (!PyArg_ParseTuple(args, (char*)"|OOb", &line, &heading, &bHidden)) return NULL;

    string utf8Line;
    if (line && !PyXBMCGetUnicodeString(utf8Line, line, 1)) return NULL;
    string utf8Heading;
    if (heading && !PyXBMCGetUnicodeString(utf8Heading, heading, 2)) return NULL;

    self->strDefault = utf8Line;
    self->strHeading = utf8Heading;
    self->bHidden = (0 != bHidden);
    PyXBMCGUILock();
    self->dlg = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    PyXBMCGUIUnlock();

    return (PyObject*)self;
  }

  void Keyboard_Dealloc(Keyboard* self)
  {
    self->strDefault.~string();
    self->strHeading.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  // doModal() Method
  PyDoc_STRVAR(doModal__doc__,
    "doModal([autoclose]) -- Show keyboard and wait for user action.\n"
    "\n"
    "autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)\n"
    "\n"
    "example:\n"
    "  - kb.doModal(30000)");

  PyObject* Keyboard_DoModal(Keyboard *self, PyObject *args)
  {
    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load virtual keyboard");
      return NULL;
    }
    int autoClose = 0;

    if (!PyArg_ParseTuple(args, (char*)"|i", &autoClose)) return NULL;

    PyXBMCGUILock();
    pKeyboard->Initialize();
    pKeyboard->SetHeading(self->strHeading);
    CStdString strDefault(self->strDefault);
    pKeyboard->SetText(strDefault);
    pKeyboard->SetHiddenInput(self->bHidden);
    if (autoClose > 0)
      pKeyboard->SetAutoClose(autoClose);

    // do modal of dialog
    PyXBMCGUIUnlock();
    PyXBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_KEYBOARD, g_windowManager.GetActiveWindow());

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setDefault() Method
  PyDoc_STRVAR(setDefault__doc__,
    "setDefault(default) -- Set the default text entry.\n"
    "\n"
    "default        : string - default text entry.\n"
    "\n"
    "example:\n"
    "  - kb.setDefault('password')");

  PyObject* Keyboard_SetDefault(Keyboard *self, PyObject *args)
  {
    PyObject *line = NULL;
    if (!PyArg_ParseTuple(args, (char*)"|O", &line)) return NULL;

    string utf8Line;
    if (line && !PyXBMCGetUnicodeString(utf8Line, line, 1)) return NULL;
    self->strDefault = utf8Line;

    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
      return NULL;
    }

    CStdString strDefault(self->strDefault);
    PyXBMCGUILock();
    pKeyboard->SetText(strDefault);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setHiddenInput() Method
  PyDoc_STRVAR(setHiddenInput__doc__,
    "setHiddenInput(hidden) -- Allows hidden text entry.\n"
    "\n"
    "hidden        : boolean - True for hidden text entry.\n"
    "example:\n"
    "  - kb.setHiddenInput(True)");

  PyObject* Keyboard_SetHiddenInput(Keyboard *self, PyObject *args)
  {
    char bHidden = false;
    if (!PyArg_ParseTuple(args, (char*)"|b", &bHidden)) return NULL;
    self->bHidden = (0 != bHidden);

    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
      return NULL;
    }

    PyXBMCGUILock();
    pKeyboard->SetHiddenInput(self->bHidden);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // setHeading() Method
  PyDoc_STRVAR(setHeading__doc__,
    "setHeading(heading) -- Set the keyboard heading.\n"
    "\n"
    "heading        : string - keyboard heading.\n"
    "\n"
    "example:\n"
    "  - kb.setHeading('Enter password')");

  PyObject* Keyboard_SetHeading(Keyboard *self, PyObject *args)
  {
    PyObject *line = NULL;
    if (!PyArg_ParseTuple(args, (char*)"|O", &line)) return NULL;

    string utf8Line;
    if (line && !PyXBMCGetUnicodeString(utf8Line, line, 1)) return NULL;
    self->strHeading = utf8Line;

    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
      return NULL;
    }

    PyXBMCGUILock();
    pKeyboard->SetHeading(self->strHeading);
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // getText() Method
  PyDoc_STRVAR(getText__doc__,
    "getText() -- Returns the user input as a string.\n"
    "\n"
    "*Note, This will always return the text entry even if you cancel the keyboard.\n"
    "       Use the isConfirmed() method to check if user cancelled the keyboard.\n"
    "\n"
    "example:\n"
    "  - text = kb.getText()");

  PyObject* Keyboard_GetText(Keyboard *self, PyObject *args)
  {
    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
      return NULL;
    }

    PyXBMCGUILock();
    CStdString result = pKeyboard->GetText();
    PyXBMCGUIUnlock();
    return Py_BuildValue((char*)"s", result.c_str());
  }

  // isConfirmed() Method
  PyDoc_STRVAR(isConfirmed__doc__,
    "isConfirmed() -- Returns False if the user cancelled the input.\n"
    "\n"
    "example:\n"
    "  - if (kb.isConfirmed()):");

  PyObject* Keyboard_IsConfirmed(Keyboard *self, PyObject *args)
  {
    CGUIDialogKeyboard *pKeyboard = ((Keyboard*)self)->dlg;
    if(!pKeyboard)
    {
      PyErr_SetString(PyExc_SystemError, "Unable to load keyboard");
      return NULL;
    }

    PyXBMCGUILock();
    bool result = pKeyboard->IsConfirmed();
    PyXBMCGUIUnlock();
    return Py_BuildValue((char*)"b", result);
  }

  PyMethodDef Keyboard_methods[] = {
    {(char*)"doModal", (PyCFunction)Keyboard_DoModal, METH_VARARGS, doModal__doc__},
    {(char*)"setDefault", (PyCFunction)Keyboard_SetDefault, METH_VARARGS, setDefault__doc__},
    {(char*)"setHeading", (PyCFunction)Keyboard_SetHeading, METH_VARARGS, setHeading__doc__},
    {(char*)"setHiddenInput", (PyCFunction)Keyboard_SetHiddenInput, METH_VARARGS, setHiddenInput__doc__},
    {(char*)"getText", (PyCFunction)Keyboard_GetText, METH_VARARGS, getText__doc__},
    {(char*)"isConfirmed", (PyCFunction)Keyboard_IsConfirmed, METH_VARARGS, isConfirmed__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(keyboard__doc__,
    "Keyboard class.\n"
    "\n"
    "Keyboard([default, heading, hidden]) -- Creates a new Keyboard object with default text\n"
    "                                heading and hidden input flag if supplied.\n"
    "\n"
    "default        : [opt] string - default text entry.\n"
    "heading        : [opt] string - keyboard heading.\n"
    "hidden         : [opt] boolean - True for hidden text entry.\n"
    "\n"
    "example:\n"
    "  - kb = xbmc.Keyboard('default', 'heading', True)\n"
    "  - kb.setDefault('password') # optional\n"
    "  - kb.setHeading('Enter password') # optional\n"
    "  - kb.setHiddenInput(True) # optional\n"
    "  - kb.doModal()\n"
    "  - if (kb.isConfirmed()):\n"
    "  -   text = kb.getText()");

// Restore code and data sections to normal.

  PyTypeObject Keyboard_Type;

  void initKeyboard_Type()
  {
    PyXBMCInitializeTypeObject(&Keyboard_Type);

    Keyboard_Type.tp_name = (char*)"xbmc.Keyboard";
    Keyboard_Type.tp_basicsize = sizeof(Keyboard);
    Keyboard_Type.tp_dealloc = (destructor)Keyboard_Dealloc;
    Keyboard_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Keyboard_Type.tp_doc = keyboard__doc__;
    Keyboard_Type.tp_methods = Keyboard_methods;
    Keyboard_Type.tp_base = 0;
    Keyboard_Type.tp_new = Keyboard_New;
  }
}

#ifdef __cplusplus
}
#endif
