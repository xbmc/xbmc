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

#include "system.h"
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

#include "filesystem/File.h"
#include "pyutil.h"
#include "FileUtils.h"

using namespace std;
using namespace XFILE;
using namespace PYXBMC;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

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

  PyDoc_STRVAR(file__doc__,
    "File class.\n"
    "\n"
    "example:\n"               
    "  f = xbmcvfs.File(file)\n");

  PyObject* File_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    PyObject *f_line;
    if (!PyArg_ParseTuple(
      args,
      (char*)"O",
      &f_line))
    {
      return NULL;
    }
    File *self = (File *)type->tp_alloc(type, 0);
    if (!self) return NULL;
    CStdString strSource;
    if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;
    self->pFile = new CFile();
    Py_BEGIN_ALLOW_THREADS
    self->pFile->Open(strSource, READ_NO_CACHE);
    Py_END_ALLOW_THREADS

    return (PyObject*)self;
  }

  void File_Dealloc(File* self)
  {
    Py_BEGIN_ALLOW_THREADS
    delete self->pFile;
    Py_END_ALLOW_THREADS

    self->pFile = NULL;
    self->ob_type->tp_free((PyObject*)self);
  }

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
    Py_BEGIN_ALLOW_THREADS
    bResult = CFile::Cache(strSource, strDestnation);
    Py_END_ALLOW_THREADS
    
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
    
    Py_BEGIN_ALLOW_THREADS
    self->pFile->Delete(strSource);
    Py_END_ALLOW_THREADS
    
    Py_INCREF(Py_None);
    return Py_None;
    
  }
  
  PyDoc_STRVAR(rename__doc__,
    "rename(file, newFileName)\n"
    "\n"
    "file        : file to reaname"
    "newFileName : new filename"
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
    Py_BEGIN_ALLOW_THREADS
    bResult = self->pFile->Rename(strSource,strDestnation);
    Py_END_ALLOW_THREADS
    
    return Py_BuildValue((char*)"b", bResult);
    
  }  
  PyDoc_STRVAR(subHash__doc__,
    "subHash(file)\n"
    "\n"
    "file        : file to calculate subtitle hash for"
    "\n"
    "example:\n"
    "  hash = xbmcvfs.subHash(file)\n"); 
  PyObject* vfs_subHash(File *self, PyObject *args, PyObject *kwds)
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
    
    CStdString s_buffer;
    Py_BEGIN_ALLOW_THREADS
    s_buffer = CFileUtils::SubtitleHash(strSource);
    Py_END_ALLOW_THREADS
    
    return Py_BuildValue((char*)"s",s_buffer.c_str());
  }  
  
  // define c functions to be used in python here
  PyMethodDef xbmcvfsMethods[] = {
    {(char*)"copy", (PyCFunction)vfs_copy, METH_VARARGS, copy__doc__},
    {(char*)"delete", (PyCFunction)vfs_delete, METH_VARARGS, delete__doc__},
    {(char*)"rename", (PyCFunction)vfs_rename, METH_VARARGS, rename__doc__},
    {(char*)"subHash", (PyCFunction)vfs_subHash, METH_VARARGS, subHash__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(read__doc__,
    "read(bytes)\n"
    "\n"
    "bytes        : now many bytes to read [opt]- if not set it will read the whole file"
    "\n"
    "example:\n"
    "  f = xbmcvfs.File(file)\n"
    "  b = f.read()\n"
    "  f.close()\n"
    );

  // read a file
  PyObject* File_read(File *self, PyObject *args)
  {
    unsigned int readBytes;
    if (!PyArg_ParseTuple(args, (char*)"|i", &readBytes)) return NULL;

    if (readBytes < 0)
    {
      Py_BEGIN_ALLOW_THREADS
      readBytes = self->pFile->GetLength();
      Py_END_ALLOW_THREADS
    }

    char* buffer = new char[readBytes + 1];
    unsigned int bytesRead;
    Py_BEGIN_ALLOW_THREADS
    bytesRead = self->pFile->Read(buffer, readBytes);
    Py_END_ALLOW_THREADS
    buffer[(bytesRead <= readBytes ? bytesRead : readBytes) + 1] = 0;
    PyObject* ret = Py_BuildValue((char*)"s", buffer);
    delete[] buffer;
    return ret;
  }

  PyDoc_STRVAR(size__doc__,
    "size()\n"
    "\n"
    "example:\n"
    "  f = xbmcvfs.File(file)\n"
    "  s = f.size()\n"
    "  f.close()\n"
    );           
  
  // size of a file
  PyObject* File_size(File *self, PyObject *args)
  {
    int readBytes;
    Py_BEGIN_ALLOW_THREADS
    readBytes = self->pFile->GetLength();
    Py_END_ALLOW_THREADS
    
    return Py_BuildValue((char*)"i", readBytes);
  }  

  PyDoc_STRVAR(seek__doc__,
    "seek()\n"
    "\n"
    "FilePosition    : possition in the file\n"
    "Whence          : where in a file to seek from[0 begining, 1 current , 2 end possition]\n"           
    "example:\n"
    "  f = xbmcvfs.File(file)\n"
    "  s = f.seek(8129, 0)\n"
    "  f.close()\n"
    );           
  
  // seek a file
  PyObject* File_seek(File *self, PyObject *args)
  {
    long long readBytes;
    int iWhence;
    if (!PyArg_ParseTuple(args, (char*)"Li", &readBytes, &iWhence )) return NULL;
    
    Py_BEGIN_ALLOW_THREADS
    self->pFile->Seek((int64_t)readBytes,iWhence);
    Py_END_ALLOW_THREADS
    
    Py_INCREF(Py_None);
    return Py_None;
  } 

  PyDoc_STRVAR(close__doc__,
    "close()\n"
    "\n"
    "example:\n"
    "  f = xbmcvfs.File(file)\n"
    "  f.close()\n"
    );  
  
  // close a file
  PyObject* File_close(File *self, PyObject *args, PyObject *kwds)
  {
    Py_BEGIN_ALLOW_THREADS
    self->pFile->Close();
    Py_END_ALLOW_THREADS
    
    Py_INCREF(Py_None);
    return Py_None;
  }  

  
  
/*****************************************************************
 * end of methods and python objects
 * initxbmc(void);
 *****************************************************************/

  /*************************************************************
   * Meta data for the xbmcvfs.File class
   *************************************************************/
  PyMethodDef File_methods[] = {
    {(char*)"read", (PyCFunction)File_read, METH_VARARGS, read__doc__},
    {(char*)"size", (PyCFunction)File_size, METH_NOARGS, size__doc__},
    {(char*)"seek", (PyCFunction)File_seek , METH_VARARGS, seek__doc__},
    {(char*)"close", (PyCFunction)File_close, METH_NOARGS, close__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyTypeObject File_Type;

  void initFile_Type()
  {
    PyXBMCInitializeTypeObject(&File_Type);

    File_Type.tp_name = (char*)"xbmcvfs.File";
    File_Type.tp_basicsize = sizeof(File);
    File_Type.tp_dealloc = (destructor)File_Dealloc;
    File_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    File_Type.tp_doc = file__doc__;
    File_Type.tp_methods = File_methods;
    File_Type.tp_base = 0;
    File_Type.tp_new = File_New;
  }


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

    Py_INCREF(&File_Type);

    pXbmcvfsModule = Py_InitModule((char*)"xbmcvfs", xbmcvfsMethods);
    if (pXbmcvfsModule == NULL) return;
    
    PyModule_AddObject(pXbmcvfsModule, (char*)"File", (PyObject*)&File_Type);
  }

  PyMODINIT_FUNC
  InitVFSTypes(bool bInitTypes)
  {
    initFile_Type();

    if (PyType_Ready(&File_Type)) return;
  }
}

#ifdef __cplusplus
}
#endif
