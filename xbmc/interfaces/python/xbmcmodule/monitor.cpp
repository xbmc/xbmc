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

#include "monitor.h"
#include "pyutil.h"
#include "pythreadstate.h"
#include "PythonMonitor.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* Monitor_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Monitor *self;

    self = (Monitor*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    std::string addonId;
    if (!PyXBMCGetAddonId(addonId) || addonId.empty())
    {
      PyErr_SetString((PyObject*)self, "Unable to identify addon");
      return NULL;
    }    
    CPyThreadState pyState;
    self->pMonitor = new CPythonMonitor();
    pyState.Restore();
    self->pMonitor->Id = addonId;    
    self->pMonitor->SetCallback(PyThreadState_Get(), (PyObject*)self);
 
    return (PyObject*)self;
  }

  void Monitor_Dealloc(Monitor* self)
  {
    self->pMonitor->SetCallback(NULL, NULL);

    CPyThreadState pyState;
    self->pMonitor->Release();
    pyState.Restore();
      
    self->pMonitor = NULL;
    self->ob_type->tp_free((PyObject*)self);
  }


  // Monitor_onSettingsChanged
  PyDoc_STRVAR(onSettingsChanged__doc__,
               "onSettingsChanged() -- onSettingsChanged method.\n"
               "\n"
               "Will be called when addon settings are changed");

  PyObject* Monitor_OnSettingsChanged(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Monitor_onScreensaverActivated
  PyDoc_STRVAR(onScreensaverActivated__doc__,
               "onScreensaverActivated() -- onScreensaverActivated method.\n"
               "\n"
               "Will be called when screensaver kicks in");
  
  PyObject* Monitor_OnScreensaverActivated(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Monitor_onScreensaverDeactivated
  PyDoc_STRVAR(onScreensaverDeactivated__doc__,
               "onScreensaverDeactivated() -- onScreensaverDeactivated method.\n"
               "\n"
               "Will be called when screensaver goes off");
  
  PyObject* Monitor_OnScreensaverDeactivated(PyObject *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }  

  // Monitor_onDatabaseUpdated
  PyDoc_STRVAR(onDatabaseUpdated__doc__,
               "onDatabaseUpdated(database) -- onDatabaseUpdated method.\n"
               "\n"
               "database - video/music as string"
               "\n"
               "Will be called when database gets updated and return video or music to indicate which DB has been changed");
  
  PyObject* Monitor_OnDatabaseUpdated(PyObject *self, PyObject *args)
  {
   Py_INCREF(Py_None);
   return Py_None;
  }  
 
  PyMethodDef Monitor_methods[] = {
    {(char*)"onSettingsChanged", (PyCFunction)Monitor_OnSettingsChanged, METH_VARARGS, onSettingsChanged__doc__},
    {(char*)"onScreensaverActivated", (PyCFunction)Monitor_OnScreensaverActivated, METH_VARARGS, onScreensaverActivated__doc__},
    {(char*)"onScreensaverDeactivated", (PyCFunction)Monitor_OnScreensaverDeactivated, METH_VARARGS, onScreensaverDeactivated__doc__},
    {(char*)"onDatabaseUpdated", (PyCFunction)Monitor_OnDatabaseUpdated, METH_VARARGS  , onDatabaseUpdated__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(monitor__doc__,
    "Monitor class.\n"
    "\n"
    "Monitor() -- Creates a new Monitor to notify addon about changes.\n"
    "\n");

  PyTypeObject Monitor_Type;

  void initMonitor_Type()
  {
    PyXBMCInitializeTypeObject(&Monitor_Type);

    Monitor_Type.tp_name = (char*)"xbmc.Monitor";
    Monitor_Type.tp_basicsize = sizeof(Monitor);
    Monitor_Type.tp_dealloc = (destructor)Monitor_Dealloc;
    Monitor_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Monitor_Type.tp_doc = monitor__doc__;
    Monitor_Type.tp_methods = Monitor_methods;
    Monitor_Type.tp_base = 0;
    Monitor_Type.tp_new = Monitor_New;
  }
}

#ifdef __cplusplus
}
#endif
