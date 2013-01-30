/*
 *      Copyright (C) 2005-2013 Team XBMC
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

%module xbmcvfs

%{
#include "interfaces/legacy/ModuleXbmcvfs.h"
#include "interfaces/legacy/File.h"
#include "interfaces/legacy/Stat.h"
#include "utils/log.h"

using namespace XBMCAddon;
using namespace xbmcvfs;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// TODO: replace this with a correct API
%feature("python:method:read") File
{
    TRACE;
    static const char *keywords[] = {
      "bytes",
      NULL};

    unsigned long readBytes = 0;
    if (!PyArg_ParseTupleAndKeywords(args,kwds,(char*)"|k",
         (char**)keywords,
           &readBytes
         ))
    {
      return NULL;
    }

    XBMCAddon::xbmcvfs::File* file = ((XBMCAddon::xbmcvfs::File*)retrieveApiInstance((PyObject*)self,&PyXBMCAddon_xbmcvfs_File_Type,"read","XBMCAddon::xbmcvfs::File"));

    XFILE::CFile* cfile = (XFILE::CFile*)file->getFile();
    int64_t size = cfile->GetLength();
    if (!readBytes || (((int64_t)readBytes) > size))
      readBytes = (unsigned long) size;
    char* buffer = new char[readBytes + 1];
    PyObject* ret = NULL;
    if (buffer)
    {
      unsigned long bytesRead = file->read(  buffer,  readBytes  );
      buffer[std::min(bytesRead, readBytes)] = 0;
      ret = Py_BuildValue((char*)"s#", buffer,bytesRead);
      delete[] buffer;
    }
    return ret;
}

%feature("python:method:write") File
{
    TRACE;
    static const char *keywords[] = {
      "buffer",
      NULL};


    PyObject * bufferObj = 0 ;
    if (!PyArg_ParseTupleAndKeywords(args,kwds,(char*)"O",
         (char**)keywords,
           &bufferObj
         ))
    {
      return NULL;
    }

    if (!PyString_Check(bufferObj)) {
      PyErr_SetString(PyExc_ValueError,"Expected a string");
      return NULL;
    }
    const char *buffer = PyString_AsString(bufferObj);
    unsigned long numBytes = PyString_Size(bufferObj);

    bool  apiResult;
    try
    {

      apiResult = (bool )((XBMCAddon::xbmcvfs::File*)retrieveApiInstance((PyObject*)self,&PyXBMCAddon_xbmcvfs_File_Type,"write","XBMCAddon::xbmcvfs::File"))-> write(  buffer,  numBytes  );

    }
    catch (const XBMCAddon::WrongTypeException& e)
    { 
      CLog::Log(LOGERROR,"EXCEPTION: %s",e.GetMessage());
      PyErr_SetString(PyExc_TypeError, e.GetMessage()); 
      return NULL; 
    }
    catch (const XbmcCommons::Exception& e)
    { 
      CLog::Log(LOGERROR,"EXCEPTION: %s",e.GetMessage());
      PyErr_SetString(PyExc_RuntimeError, e.GetMessage()); 
      return NULL; 
    }
    catch (...)
    {
      CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call 'write'");
      PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call 'write'"); 
      return NULL; 
    }

    PyObject* result;

    // transform the result
    result = Py_BuildValue((char*)"b", apiResult);

    return result; 
}

%include "interfaces/legacy/File.h"

%rename ("st_atime") XBMCAddon::xbmcvfs::Stat::atime;
%rename ("st_mtime") XBMCAddon::xbmcvfs::Stat::mtime;
%rename ("st_ctime") XBMCAddon::xbmcvfs::Stat::ctime;
%include "interfaces/legacy/Stat.h"

%rename ("delete") XBMCAddon::xbmcvfs::deleteFile;
%include "interfaces/legacy/ModuleXbmcvfs.h"

