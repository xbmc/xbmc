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
#include <cstring>


#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "pyutil.h"
#include "pythreadstate.h"
#include "FileItem.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "Util.h"

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

    PyDoc_STRVAR(file__doc__,
      "File class.\n"
      "\n"
      "'w' - opt open for write\n"
      "example:\n"
      " f = xbmcvfs.File(file, ['w'])\n");

    PyObject* File_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
      PyObject *f_line;
      char *cLine;
      if (!PyArg_ParseTuple(
                            args,
                            (char*)"O|s",
                            &f_line,
                            &cLine))
      {
        return NULL;
      }
      File *self = (File *)type->tp_alloc(type, 0);
      if (!self)
        return NULL;

      CStdString strSource;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1))
        return NULL;

      self->pFile = new CFile();
      if (strncmp(cLine, "w", 1) == 0)
      {
        CPyThreadState pyState;
        self->pFile->OpenForWrite(strSource,true);
        pyState.Restore();
      }
      else
      {
        CPyThreadState pyState;
        self->pFile->Open(strSource, READ_NO_CACHE);
        pyState.Restore();
      }
      return (PyObject*)self;
    }

    void File_Dealloc(File* self)
    {
      CPyThreadState pyState;
      delete self->pFile;
      pyState.Restore();

      self->pFile = NULL;
      self->ob_type->tp_free((PyObject*)self);
    }

    // copy() method
    PyDoc_STRVAR(copy__doc__,
      "copy(source, destination) -- Copy file to destination, returns true/false.\n"
      "\n"
      "source          : file to copy.\n"
      "destination     : destination file\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.copy(source, destination)\n");

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
      "delete(file) -- Delete file\n"
      "\n"
      "file        : file to delete\n"
      "\n"
      "example:\n"
      " - xbmcvfs.delete(file)\n");

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
      "rename(file, newFileName) -- Rename file, returns true/false.\n"
      "\n"
      "file        : file to reaname\n"
      "newFileName : new filename, including the full path\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.rename(file, newFileName)\n");

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
      "exists(path) -- Check if file exists, returns true/false.\n"
      "\n"
      "path        : file or folder\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.exists(path)\n");

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

    PyDoc_STRVAR(mkdir__doc__,
      "mkdir(path) -- Create a folder.\n"
      "\n"
      "path        : folder\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.mkdir(path)\n");
    // make a directory
    PyObject* vfs_mkdir(File *self, PyObject *args, PyObject *kwds)
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
      bResult = CDirectory::Create(strSource);
      pyState.Restore();

      return Py_BuildValue((char*)"b", bResult);
    }

    PyDoc_STRVAR(mkdirs__doc__,
      "mkdirs(path) -- Create folder(s) - it will create all folders in the path.\n"
      "\n"
      "path        : folder\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.mkdirs(path)\n");
    // make all directories along the path
    PyObject* vfs_mkdirs(File *self, PyObject *args, PyObject *kwds)
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
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1))
        return NULL;

      CPyThreadState pyState;
      bool bResult = CUtil::CreateDirectoryEx(strSource);
      pyState.Restore();

      return Py_BuildValue((char*)"b", bResult);
    }

    PyDoc_STRVAR(rmdir__doc__,
      "rmdir(path) -- Remove a folder.\n"
      "\n"
      "path        : folder\n"
      "\n"
      "example:\n"
      " - success = xbmcvfs.rmdir(path)\n");
    // remove a directory
    PyObject* vfs_rmdir(File *self, PyObject *args, PyObject *kwds)
    {
      PyObject *f_line;
      bool bForce = false;
      if (!PyArg_ParseTuple(
        args,
        (char*)"O|b",
        &f_line,
        &bForce))
      {
        return NULL;
      }
      CStdString strSource;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1)) return NULL;

      bool bResult;
      if (bForce)
      {
        CPyThreadState pyState;
        bResult = CFileUtils::DeleteItem(strSource, bForce);
        pyState.Restore();
      }
      else
      {
        CPyThreadState pyState;
        bResult = CDirectory::Remove(strSource);
        pyState.Restore();
      }

      return Py_BuildValue((char*)"b", bResult);
    }

    PyDoc_STRVAR(listdir__doc__,
      "listdir(path) -- lists content of a folder.\n"
      "\n"
      "path        : folder\n"
      "\n"
      "example:\n"
      " - dirs, files = xbmcvfs.listdir(path)\n");
    // lists content of a folder
    PyObject* vfs_listdir(File *self, PyObject *args, PyObject *kwds)
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
      CFileItemList items;
      if (!PyXBMCGetUnicodeString(strSource, f_line, 1))
        return NULL;

      CPyThreadState pyState;
      CDirectory::GetDirectory(strSource, items);
      pyState.Restore();

      PyObject *fileList = PyList_New(0);
      PyObject *folderList = PyList_New(0);
      for (int i=0; i < items.Size(); i++)
      {
        CStdString itemPath = items[i]->GetPath();
        PyObject *obj;
        if (URIUtils::HasSlashAtEnd(itemPath)) // folder
        {
          URIUtils::RemoveSlashAtEnd(itemPath);
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          obj = Py_BuildValue((char*)"s", strFileName.c_str());
          PyList_Append(folderList, obj);
        }
        else // file
        {
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          obj = Py_BuildValue((char*)"s", strFileName.c_str());
          PyList_Append(fileList, obj);
        }
        Py_DECREF(obj); //we have to do this as PyList_Append is known to leak
      }
      return Py_BuildValue((char*)"O,O", folderList, fileList);
    }

    // define c functions to be used in python here
    PyMethodDef xbmcvfsMethods[] = {
      {(char*)"copy", (PyCFunction)vfs_copy, METH_VARARGS, copy__doc__},
      {(char*)"delete", (PyCFunction)vfs_delete, METH_VARARGS, delete__doc__},
      {(char*)"rename", (PyCFunction)vfs_rename, METH_VARARGS, rename__doc__},
      {(char*)"mkdir", (PyCFunction)vfs_mkdir, METH_VARARGS, mkdir__doc__},
      {(char*)"mkdirs", (PyCFunction)vfs_mkdirs, METH_VARARGS, mkdirs__doc__},
      {(char*)"rmdir", (PyCFunction)vfs_rmdir, METH_VARARGS, rmdir__doc__},
      {(char*)"exists", (PyCFunction)vfs_exists, METH_VARARGS, exists__doc__},
      {(char*)"listdir", (PyCFunction)vfs_listdir, METH_VARARGS, listdir__doc__},
      {NULL, NULL, 0, NULL}
    };
    PyDoc_STRVAR(read__doc__,
      "read(bytes)\n"
      "\n"
      "bytes : how many bytes to read [opt]- if not set it will read the whole file"
      "\n"
      "example:\n"
      " f = xbmcvfs.File(file)\n"
      " b = f.read()\n"
      " f.close()\n");

    // read a file
    PyObject* File_read(File *self, PyObject *args)
    {
      unsigned int readBytes = 0;
      if (!PyArg_ParseTuple(args, (char*)"|i", &readBytes))
        return NULL;
      int64_t size = self->pFile->GetLength();
      if (!readBytes || readBytes > size)
        readBytes = size;
      char* buffer = new char[readBytes + 1];
      PyObject* ret = NULL;
      if (buffer)
      {
        unsigned int bytesRead;
        CPyThreadState pyState;
        bytesRead = self->pFile->Read(buffer, readBytes);
        pyState.Restore();
        buffer[std::min(bytesRead, readBytes)] = 0;
        ret = Py_BuildValue((char*)"s#", buffer,readBytes);
        delete[] buffer;
      }
      return ret;
    }

    PyDoc_STRVAR(write__doc__,
      "write(buffer)\n"
      "\n"
      "buffer : buffer to write to file"
      "\n"
      "example:\n"
      " f = xbmcvfs.File(file, 'w', True)\n"
      " result = f.write(buffer)\n"
      " f.close()\n");

    // read a file
    PyObject* File_write(File *self, PyObject *args)
    {
      const char* pBuffer;
      if (!PyArg_ParseTuple(args, (char*)"s", &pBuffer))
        return NULL;

      CPyThreadState pyState;
      bool bResult = self->pFile->Write( (void*) pBuffer, strlen( pBuffer ) + 1 );
      pyState.Restore();

      return Py_BuildValue((char*)"b", bResult);
    }

    PyDoc_STRVAR(size__doc__,
      "size()\n"
      "\n"
      "example:\n"
      " f = xbmcvfs.File(file)\n"
      " s = f.size()\n"
      " f.close()\n");

    // size of a file
    PyObject* File_size(File *self, PyObject *args)
    {
      int64_t size;
      CPyThreadState pyState;
      size = self->pFile->GetLength();
      pyState.Restore();

      return Py_BuildValue((char*)"L", size);
    }

    PyDoc_STRVAR(seek__doc__,
      "seek()\n"
      "\n"
      "FilePosition : position in the file\n"
      "Whence : where in a file to seek from[0 begining, 1 current , 2 end possition]\n"
      "example:\n"
      " f = xbmcvfs.File(file)\n"
      " result = f.seek(8129, 0)\n"
      " f.close()\n");

    // seek a file
    PyObject* File_seek(File *self, PyObject *args)
    {
      int64_t seekBytes;
      int iWhence;
      if (!PyArg_ParseTuple(args, (char*)"Li", &seekBytes, &iWhence ))
        return NULL;

      CPyThreadState pyState;
      int64_t bResult = self->pFile->Seek(seekBytes,iWhence);
      pyState.Restore();

      return Py_BuildValue((char*)"L", bResult);
    }

    PyDoc_STRVAR(close__doc__,
      "close()\n"
      "\n"
      "example:\n"
      " f = xbmcvfs.File(file)\n"
      " f.close()\n");

    // close a file
    PyObject* File_close(File *self, PyObject *args, PyObject *kwds)
    {
      CPyThreadState pyState;
      self->pFile->Close();
      pyState.Restore();

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
      {(char*)"write", (PyCFunction)File_write, METH_VARARGS, write__doc__},
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
      pXbmcvfsModule = Py_InitModule((char*)"xbmcvfs", xbmcvfsMethods);
      if (pXbmcvfsModule == NULL) return;

      PyModule_AddObject(pXbmcvfsModule, (char*)"File", (PyObject*)&File_Type);

      // constants
      PyModule_AddStringConstant(pXbmcvfsModule, (char*)"__author__", (char*)PY_XBMC_AUTHOR);
      PyModule_AddStringConstant(pXbmcvfsModule, (char*)"__date__", (char*)"25 May 2012");
      PyModule_AddStringConstant(pXbmcvfsModule, (char*)"__version__", (char*)"1.4");
      PyModule_AddStringConstant(pXbmcvfsModule, (char*)"__credits__", (char*)PY_XBMC_CREDITS);
      PyModule_AddStringConstant(pXbmcvfsModule, (char*)"__platform__", (char*)PY_XBMC_PLATFORM);
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
