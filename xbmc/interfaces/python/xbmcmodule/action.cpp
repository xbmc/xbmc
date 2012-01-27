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

// Get rid of 'dereferencing type-punned pointer will break strict-aliasing rules'
// warnings caused by Py_RETURN_TRUE/FALSE.
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#include "action.h"
#include "guilib/Key.h"
#include "pyutil.h"

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* Action_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Action *self;

    self = (Action*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->strAction) string();

    //if (!PyArg_ParseTuple(args, "l", &self->action)) return NULL;
    //self->action = -1;

    self->id = -1;
    self->fAmount1 = 0.0f;
    self->fAmount2 = 0.0f;
    self->fRepeat = 0.0f;
    self->buttonCode = 0;
    self->strAction = "";

    return (PyObject*)self;
  }

  PyObject* Action_FromAction(const CAction& action)
  {
    Action* pyAction = (Action*)Action_Type.tp_alloc(&Action_Type, 0);
    new(&pyAction->strAction) string();

    if (pyAction)
    {
      pyAction->id = action.GetID();
      pyAction->buttonCode = action.GetButtonCode();
      pyAction->fAmount1 = action.GetAmount(0);
      pyAction->fAmount2 = action.GetAmount(1);
      pyAction->fRepeat = action.GetRepeat();
      pyAction->strAction = action.GetName();
    }

    return (PyObject*)pyAction;
  }

  void Action_Dealloc(Action* self)
  {
    self->strAction.~string();
    self->ob_type->tp_free((PyObject*)self);
  }

  /* For backwards compatability we have to check the action code
   * against an integer
   * The first argument is always an Action object
   */
  PyObject* Action_RichCompare(Action* obj1, PyObject* obj2, int method)
  {
    if (method == Py_EQ)
    {
      if (Action_Check(obj2))
      {
        // both are Action objects
        Action* a2 = (Action*)obj2;

        if (obj1->id == a2->id &&
            obj1->buttonCode == a2->buttonCode &&
            obj1->fAmount1 == a2->fAmount1 &&
            obj1->fAmount2 == a2->fAmount2 &&
            obj1->fRepeat == a2->fRepeat &&
            obj1->strAction == a2->strAction)
        {
          Py_RETURN_TRUE;
        }
        else
        {
          Py_RETURN_FALSE;
        }
      }
      else
      {
        // for backwards compatability in python scripts
        PyObject* o1 = PyLong_FromLong(obj1->id);
        return PyObject_RichCompare(o1, obj2, method);
      }
    }
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  // getId() Method
  PyDoc_STRVAR(getId__doc__,
    "getId() -- Returns the action's current id as a long or 0 if no action is mapped in the xml's.\n"
    "\n");

  PyObject* Action_GetId(Action* self, PyObject* args)
  {
    return Py_BuildValue((char*)"l", self->id);
  }

  // getButtonCode() Method
  PyDoc_STRVAR(getButtonCode__doc__,
    "getButtonCode() -- Returns the button code for this action.\n"
    "\n");

  PyObject* Action_GetButtonCode(Action* self, PyObject* args)
  {
    return Py_BuildValue((char*)"l", self->buttonCode);
  }

  PyDoc_STRVAR(getAmount1__doc__,
    "getAmount1() -- Returns the first amount of force applied to the thumbstick n.\n"
    "\n");

  PyDoc_STRVAR(getAmount2__doc__,
    "getAmount2() -- Returns the second amount of force applied to the thumbstick n.\n"
    "\n");

  PyObject* Action_GetAmount1(Action* self, PyObject* args)
  {
    return Py_BuildValue((char*)"f", self->fAmount1);
  }

  PyObject* Action_GetAmount2(Action* self, PyObject* args)
  {
    return Py_BuildValue((char*)"f", self->fAmount2);
  }

  PyMethodDef Action_methods[] = {
    {(char*)"getId", (PyCFunction)Action_GetId, METH_VARARGS, getId__doc__},
    {(char*)"getButtonCode", (PyCFunction)Action_GetButtonCode, METH_VARARGS, getButtonCode__doc__},
    {(char*)"getAmount1", (PyCFunction)Action_GetAmount1, METH_VARARGS, getAmount1__doc__},
    {(char*)"getAmount2", (PyCFunction)Action_GetAmount2, METH_VARARGS, getAmount2__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(action__doc__,
    "Action class.\n"
    "\n"
    "For backwards compatibility reasons the == operator is extended so that it"
    "can compare an action with other actions and action.GetID() with numbers"
    "  example: (action == ACTION_MOVE_LEFT)"
    "");

// Restore code and data sections to normal.

  PyTypeObject Action_Type;

  void initAction_Type()
  {
    PyXBMCInitializeTypeObject(&Action_Type);

    Action_Type.tp_name = (char*)"xbmcgui.Action";
    Action_Type.tp_basicsize = sizeof(Action);
    Action_Type.tp_dealloc = (destructor)Action_Dealloc;
    //Action_Type.tp_compare = Action_Compare;
    Action_Type.tp_richcompare = (richcmpfunc)Action_RichCompare;
    Action_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Action_Type.tp_doc = action__doc__;
    Action_Type.tp_methods = Action_methods;
    Action_Type.tp_base = 0;
    Action_Type.tp_new = Action_New;
  }
}

#ifdef __cplusplus
}
#endif
