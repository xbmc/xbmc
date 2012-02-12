/*
 *      Copyright (C) 2012 Team XBMC
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

#include "pyzeroconf.h"
#include "system.h"

#ifdef HAS_ZEROCONF

#include "pyutil.h"
#include "pythreadstate.h"
#include "network/Zeroconf.h"
#include "settings/GUISettings.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* Zeroconf_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Zeroconf *self = (Zeroconf*)type->tp_alloc(type, 0);
    if (!self)
      return NULL;

    self->zeroconf = CZeroconf::GetInstance();
    if (!self->zeroconf)
    {
        return PyErr_NoMemory();    //we will never hit this imho
    }
    
    self->txt_records = new std::map<std::string, std::string>();

    return (PyObject*)self;
  }

  void Zeroconf_Dealloc(Zeroconf* self)
  {
    self->txt_records->clear();
    delete self->txt_records;
    self->ob_type->tp_free((PyObject*)self);
  }
  
  // Zeroconf_ClearTxtRecords
  PyDoc_STRVAR(clearTxtRecords__doc__,
    "clearTxtRecords() -- clears self->txt_records\n");

  PyObject* Zeroconf_ClearTxtRecords(Zeroconf *self, PyObject *args)
  {   
    self->txt_records->clear();
    Py_INCREF(Py_None);
    return Py_None;
  }
  
  // Zeroconf_AddTxtRecord
  PyDoc_STRVAR(addTxtRecord__doc__,
    "addTxtRecords(key, value) -- add a txt record to self->txt_records\n");

  PyObject* Zeroconf_AddTxtRecord(Zeroconf *self, PyObject *args)
  { 
    char *key = NULL;
    char *value = NULL;

    if (PyArg_ParseTuple(args, (char*)"ss", &key, &value))
    {
      self->txt_records->insert(std::make_pair(CStdString(key), CStdString(value)));
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  // Zeroconf_IsEnabled
  PyDoc_STRVAR(isEnabled__doc__,
    "isEnabled() -- returns false if zeroconf is disabled in settings, else true\n");

  PyObject* Zeroconf_IsEnabled(Zeroconf *self, PyObject *args)
  {
    bool ret = false;
    ret = g_guiSettings.GetBool("services.zeroconf");
    return Py_BuildValue((char*)"b", ret);
  }

  // Zeroconf_RemoveService
  PyDoc_STRVAR(removeSerice__doc__,
    "removeService(fcr_identifier) -- returns false if fcr_identifier does not exist\n");

  PyObject* Zeroconf_RemoveService(Zeroconf *self, PyObject *args)
  {
    bool ret = false;
    char *fcr_identifier = NULL;
    
    if (PyArg_ParseTuple(args, (char*)"s", &fcr_identifier))
    {
      ret = self->zeroconf->RemoveService(fcr_identifier);
    }
    return Py_BuildValue((char*)"b", ret);
  }

  // Zeroconf_PublishService
  PyDoc_STRVAR(publishService__doc__,
    "removeService(fcr_identifier, fcr_type, fcr_name, f_port) -- Announce a service via zeroconf\n"
    "fcr_identifier         : can be used to stop this service later - so its a unique identifier for this service\n"
    "fcr_type               : is the zeroconf service type to publish (e.g. _http._tcp for webserver)\n"
    "fcr_name               : is the name of the service to publish (human readable). The hostname is currently automatically appended\n"
    "and used for name collisions. e.g. XBMC would get published as fcr_name@Martn or, after collision fcr_name@Martn-2\n"
    "f_port                 : port of the service to publish\n"
    "if txt-records where added via addTxtRecord these will be published aswell\n"
    "returns                : false if fcr_identifier was already present\n");

  PyObject* Zeroconf_PublishService(Zeroconf *self, PyObject *args)
  {
    bool ret = false;
    char *fcr_identifier = NULL;
    char *fcr_type = NULL;
    char *fcr_name = NULL;
    unsigned int f_port = 0;
    
    if (PyArg_ParseTuple(args, (char*)"sssi", &fcr_identifier, &fcr_type, &fcr_name, &f_port))
    {
      ret = self->zeroconf->PublishService(fcr_identifier, fcr_type, fcr_name, f_port, *self->txt_records);
      self->txt_records->clear();
    }
    return Py_BuildValue((char*)"b", ret);
  }  
  
  PyMethodDef Zeroconf_methods[] = {
    {(char*)"removeService",    (PyCFunction)Zeroconf_RemoveService,    METH_VARARGS, removeSerice__doc__     },
    {(char*)"publishService",   (PyCFunction)Zeroconf_PublishService,   METH_VARARGS, publishService__doc__   },
    {(char*)"clearTxtRecords",  (PyCFunction)Zeroconf_ClearTxtRecords,  METH_VARARGS, clearTxtRecords__doc__  },
    {(char*)"addTxtRecord",     (PyCFunction)Zeroconf_AddTxtRecord,     METH_VARARGS, addTxtRecord__doc__     },
    {(char*)"isEnabled",        (PyCFunction)Zeroconf_IsEnabled,        METH_VARARGS, isEnabled__doc__        },
    {NULL,                      NULL,                                   0,            NULL                    }
  };

  PyDoc_STRVAR(Zeroconf__doc__,
    "Zeroconf class.\n"
    "\n"
    "Announce a service via zeroconf.\n");

// Restore code and data sections to normal.

  PyTypeObject Zeroconf_Type;

  void initZeroconf_Type()
  {
    PyXBMCInitializeTypeObject(&Zeroconf_Type);

    Zeroconf_Type.tp_name      = (char*)"xbmc.Zeroconf";
    Zeroconf_Type.tp_basicsize = sizeof(Zeroconf);
    Zeroconf_Type.tp_dealloc   = (destructor)Zeroconf_Dealloc;
    Zeroconf_Type.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Zeroconf_Type.tp_doc       = Zeroconf__doc__;
    Zeroconf_Type.tp_methods   = Zeroconf_methods;
    Zeroconf_Type.tp_base      = 0;
    Zeroconf_Type.tp_new       = Zeroconf_New;
  }
}

#ifdef __cplusplus
}
#endif

#endif //HAS_PYZEROCONF


