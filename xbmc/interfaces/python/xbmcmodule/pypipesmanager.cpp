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

#include "pypipesmanager.h"
#include "system.h"

#include "pyutil.h"
#include "pythreadstate.h"
#include "filesystem/PipesManager.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* PipesManager_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    PipesManager *self = (PipesManager*)type->tp_alloc(type, 0);
    if (!self)
      return NULL;

    return (PyObject*)self;
  }

  void PipesManager_Dealloc(PipesManager* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }
  
  // PipesManager_OpenPipeForWrite
  PyDoc_STRVAR(openPipeForWrite__doc__,
    "openPipeForWrite(name) -- creates and opens a pipe with the given name/url - name has to be in form pipe://pipename/\n"
    "return true on success");

  PyObject* PipesManager_OpenPipeForWrite(PipesManager *self, PyObject *args)
  {   
    char *name = NULL;
    bool ret = false;
    if (PyArg_ParseTuple(args, (char*)"s", &name))
    {
      ret = XFILE::PipesManager::GetInstance().OpenPipeForWrite(CStdString(name));
    }  
    return Py_BuildValue((char*)"b", ret);
  }

  // PipesManager_Write
  PyDoc_STRVAR(write__doc__,
    "write(name, buff) -- write buff in pipe name. returns true on success\n");

  PyObject* PipesManager_Write(PipesManager *self, PyObject *args)
  {
    bool ret = false;
    char *name = NULL;
    char *buff = NULL;
    int nSize = 0;

    if (PyArg_ParseTuple(args, (char*)"ss#", &name, &buff, &nSize))
    {
      ret = XFILE::PipesManager::GetInstance().Write(CStdString(name), buff, nSize);
    }
    return Py_BuildValue((char*)"b", ret);
  }

  // PipesManager_ClosePipe
  PyDoc_STRVAR(closePipe__doc__,
    "closePipe(name), close the given pipe\n");

  PyObject* PipesManager_ClosePipe(PipesManager *self, PyObject *args)
  {
    char *name = NULL;
    
    if (PyArg_ParseTuple(args, (char*)"s", &name))
    {
      XFILE::PipesManager::GetInstance().ClosePipe(CStdString(name));
    }
    Py_INCREF(Py_None);
    return Py_None;
  }  
  
  // PipesManager_SetOpenThreshold
  PyDoc_STRVAR(setOpenThreshold__doc__,
    "setOpenThreshold(name, thre), close the given pipe\n");

  PyObject* PipesManager_SetOpenThreshold(PipesManager *self, PyObject *args)
  {
    char *name = NULL;
    int thresh = 0;
    
    if (PyArg_ParseTuple(args, (char*)"si", &name, &thresh))
    {
      XFILE::PipesManager::GetInstance().SetOpenThreashold(CStdString(name),thresh);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }  
  
   // PipesManager_SetEof
  PyDoc_STRVAR(setEof__doc__,
    "setEof(name), sets eof on the given pipe\n");

  PyObject* PipesManager_SetEof(PipesManager *self, PyObject *args)
  {
    char *name = NULL;
    
    if (PyArg_ParseTuple(args, (char*)"s", &name))
    {
      XFILE::PipesManager::GetInstance().SetEof(CStdString(name));
    }
    Py_INCREF(Py_None);
    return Py_None;
  }  
  
  // PipesManager_Flush
  PyDoc_STRVAR(flush__doc__,
    "flush(name), flush the given pipe\n");

  PyObject* PipesManager_Flush(PipesManager *self, PyObject *args)
  {
    char *name = NULL;
    
    if (PyArg_ParseTuple(args, (char*)"s", &name))
    {
      XFILE::PipesManager::GetInstance().Flush(CStdString(name));
    }
    Py_INCREF(Py_None);
    return Py_None;
  }  
  
  // PipesManager_Exists
  PyDoc_STRVAR(exists__doc__,
    "exists(name), returns true if given pipe exists\n");

  PyObject* PipesManager_Exists(PipesManager *self, PyObject *args)
  {
    char *name = NULL;
    bool ret = false;
    
    if (PyArg_ParseTuple(args, (char*)"s", &name))
    {
      ret = XFILE::PipesManager::GetInstance().Exists(CStdString(name));
    }
    return Py_BuildValue((char*)"b", ret);;
  }  
  
  PyMethodDef PipesManager_methods[] = {
    {(char*)"openPipeForWrite",   (PyCFunction)PipesManager_OpenPipeForWrite,   METH_VARARGS, openPipeForWrite__doc__   },
    {(char*)"write",              (PyCFunction)PipesManager_Write,              METH_VARARGS, write__doc__              },
    {(char*)"closePipe",          (PyCFunction)PipesManager_ClosePipe,          METH_VARARGS, closePipe__doc__          },
    {(char*)"setOpenThreshold",   (PyCFunction)PipesManager_SetOpenThreshold,   METH_VARARGS, setOpenThreshold__doc__   },
    {(char*)"setEof",             (PyCFunction)PipesManager_SetEof,             METH_VARARGS, setEof__doc__             },
    {(char*)"flush",              (PyCFunction)PipesManager_Flush,              METH_VARARGS, flush__doc__              },
    {(char*)"exists",             (PyCFunction)PipesManager_Exists,             METH_VARARGS, exists__doc__             },
    {NULL,                        NULL,                                         0,            NULL                      }
  };
  
  PyDoc_STRVAR(PipesManager__doc__,
    "PipesManager class.\n"
    "\n"
    "Access pipes via our PipesManager instance.\n");

// Restore code and data sections to normal.

  PyTypeObject PipesManager_Type;

  void initPipesManager_Type()
  {
    PyXBMCInitializeTypeObject(&PipesManager_Type);

    PipesManager_Type.tp_name      = (char*)"xbmc.PipesManager";
    PipesManager_Type.tp_basicsize = sizeof(PipesManager);
    PipesManager_Type.tp_dealloc   = (destructor)PipesManager_Dealloc;
    PipesManager_Type.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    PipesManager_Type.tp_doc       = PipesManager__doc__;
    PipesManager_Type.tp_methods   = PipesManager_methods;
    PipesManager_Type.tp_base      = 0;
    PipesManager_Type.tp_new       = PipesManager_New;
  }
}

#ifdef __cplusplus
}
#endif

