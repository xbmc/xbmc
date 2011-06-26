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


#include "filesystem/File.h"
#include "pyutil.h"
#include "pythreadstate.h"

using namespace std;
using namespace XFILE;
using namespace PYXBMC;


#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif
  
  namespace xbmcvfs
  {
    /*****************************************************************
     * start of xbmcvfs methods
     *****************************************************************/
    typedef struct {
      PyObject_HEAD
      CFile* pFile;
    } File;
    
    // copy() method
    PyDoc_STRVAR(copy__doc__,
      "copy(source, destination) -- copy file to destination, returns true/false.\n"
      "\n"
      "source          : file to copy.\n"
      "destination     : destination file"
      "\n"
      "example:\n"
      "  success = xbmcvfs.copy(source, destination)\n");
    
    PyObject* vfs_copy(PyObject *self, PyObject *args)
    {
      PyObject *f_line;
      PyObject *d_line;
      if (!PyArg_ParseTuple(
        args,
        (char*)"OO",
        &f_line,
        &d_line))
      {
        return NULL;
      }
      CStdString strSource;
      CStdString strDestnation;
      bool bResult = true;
      
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;
      if (!PyXBMCGetUnicodeString(strDestnation, d_line, 1)) return NULL;

      CPyThreadState pyState;
      bResult = CFile::Cache(strSource, strDestnation);
      pyState.Restore();
      
      return Py_BuildValue((char*)"b", bResult);
    }
    PyDoc_STRVAR(delete__doc__,
      "delete(file)\n"
      "\n"
      "file        : file to delete"
      "\n"
      "example:\n"
      "  xbmcvfs.delete(file)\n");
    
    // delete a file
    PyObject* vfs_delete(File *self, PyObject *args, PyObject *kwds)
    {
      PyObject *f_line;
      if (!PyArg_ParseTuple(
        args,
        (char*)"O",
        &f_line))
      {
        return NULL;
      }
      CStdString strSource;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;
      
      CPyThreadState pyState;
      self->pFile->Delete(strSource);
      pyState.Restore();
      
      Py_INCREF(Py_None);
      return Py_None;
      
    }
    
    PyDoc_STRVAR(rename__doc__,
      "rename(file, newFileName)\n"
      "\n"
      "file        : file to reaname"
      "newFileName : new filename, including the full path"
      "\n"
      "example:\n"
      "  success = xbmcvfs.rename(file,newFileName)\n");
    
    // rename a file
    PyObject* vfs_rename(File *self, PyObject *args, PyObject *kwds)
    {
      PyObject *f_line;
      PyObject *d_line;
      if (!PyArg_ParseTuple(
        args,
        (char*)"OO",
        &f_line,
        &d_line))
      {
        return NULL;
      }
      CStdString strSource;
      CStdString strDestnation;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;
      if (!PyXBMCGetUnicodeString(strDestnation, d_line, 1)) return NULL;
      
      bool bResult;

      CPyThreadState pyState;
      bResult = self->pFile->Rename(strSource,strDestnation);
      pyState.Restore();
      
      return Py_BuildValue((char*)"b", bResult);
      
    }  

    PyDoc_STRVAR(exists__doc__,
      "exists(path)\n"
      "\n"
      "path        : file or folder"
      "\n"
      "example:\n"
      "  success = xbmcvfs.exists(path)\n");
   
    // check for a file or folder existance, mimics Pythons os.path.exists()
    PyObject* vfs_exists(File *self, PyObject *args, PyObject *kwds)
    {
      PyObject *f_line;
      if (!PyArg_ParseTuple(
        args,
        (char*)"O",
        &f_line))
      {
        return NULL;
      }
      CStdString strSource;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;
     
      bool bResult;
     
      CPyThreadState pyState;
      bResult = self->pFile->Exists(strSource, false);
      pyState.Restore();

      return Py_BuildValue((char*)"b", bResult);
    }      
    
    // define c functions to be used in python here
    PyMethodDef xbmcvfsMethods[] = {
      {(char*)"copy", (PyCFunction)vfs_copy, METH_VARARGS, copy__doc__},
      {(char*)"delete", (PyCFunction)vfs_delete, METH_VARARGS, delete__doc__},
      {(char*)"rename", (PyCFunction)vfs_rename, METH_VARARGS, rename__doc__},
      {(char*)"exists", (PyCFunction)vfs_exists, METH_VARARGS, exists__doc__},
      {NULL, NULL, 0, NULL}
    };
    
    /*****************************************************************
     * end of methods and python objects
     * initxbmc(void);
     *****************************************************************/
    
    
    PyMODINIT_FUNC
    DeinitVFSModule()
    {
      // no need to Py_DECREF our objects (see InitXBMCMVFSModule()) as they were created only
      // so that they could be added to the module, which steals a reference.
    }
    
    PyMODINIT_FUNC
    InitVFSModule()
    {
      // init general xbmc modules
      PyObject* pXbmcvfsModule;
      pXbmcvfsModule = Py_InitModule((char*)"xbmcvfs", xbmcvfsMethods);
      if (pXbmcvfsModule == NULL) return;
    }    
  }
  
#ifdef __cplusplus
}
#endif
