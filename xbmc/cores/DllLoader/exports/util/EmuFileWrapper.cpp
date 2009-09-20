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
 
#include "EmuFileWrapper.h"
#include "FileSystem/File.h"
#include "utils/Mutex.h"

CEmuFileWrapper g_emuFileWrapper;

CEmuFileWrapper::CEmuFileWrapper()
{
  InitializeCriticalSection(&m_criticalSection);

  // since we always use dlls we might just initialize it directly
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    memset(&m_files[i], 0, sizeof(EmuFileObject));
    m_files[i].used = false;
    m_files[i].file_emu._file = -1;
  }
}

CEmuFileWrapper::~CEmuFileWrapper()
{
  DeleteCriticalSection(&m_criticalSection);
}

void CEmuFileWrapper::CleanUp()
{
  EnterCriticalSection(&m_criticalSection);
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (m_files[i].used)
    {
      m_files[i].file_xbmc->Close();
      delete m_files[i].file_xbmc;

      if (m_files[i].file_lock)
      {
        delete m_files[i].file_lock;
        m_files[i].file_lock = NULL;
      }
      memset(&m_files[i], 0, sizeof(EmuFileObject));
      m_files[i].used = false;
      m_files[i].file_emu._file = -1;
    }
  }
  LeaveCriticalSection(&m_criticalSection);
}

EmuFileObject* CEmuFileWrapper::RegisterFileObject(XFILE::CFile* pFile)
{
  EmuFileObject* object = NULL;

  EnterCriticalSection(&m_criticalSection);

  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (!m_files[i].used)
    {
      // found a free location
      object = &m_files[i];
      object->used = true;
      object->file_xbmc = pFile;
      object->file_emu._file = (i + FILE_WRAPPER_OFFSET);
      object->file_lock = new CMutex();
      break;
    }
  }

  LeaveCriticalSection(&m_criticalSection);

  return object;
}

void CEmuFileWrapper::UnRegisterFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      EnterCriticalSection(&m_criticalSection);

      // we assume the emulated function alreay deleted the CFile object
      if (m_files[i].used)
      {
        if (m_files[i].file_lock)
        {
          delete m_files[i].file_lock;
          m_files[i].file_lock = NULL;
        }
        memset(&m_files[i], 0, sizeof(EmuFileObject));
        m_files[i].used = false;
        m_files[i].file_emu._file = -1;
      }

      LeaveCriticalSection(&m_criticalSection);
    }
  }
}

void CEmuFileWrapper::UnRegisterFileObjectByStream(FILE* stream)
{
  if (stream != NULL)
  {
    return UnRegisterFileObjectByDescriptor(stream->_file);
  }
}

void CEmuFileWrapper::LockFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      m_files[i].file_lock->Wait();
    }
  }
}

bool CEmuFileWrapper::TryLockFileObjectByDescriptor(int fd)
{ 
  int i = fd - FILE_WRAPPER_OFFSET;  
  if (i >= 0 && i < MAX_EMULATED_FILES)
  { 
    if (m_files[i].used)
    {   
      return m_files[i].file_lock->WaitMSec(0);
    }
  }
  return false;
}

void CEmuFileWrapper::UnlockFileObjectByDescriptor(int fd)
{ 
  int i = fd - FILE_WRAPPER_OFFSET;  
  if (i >= 0 && i < MAX_EMULATED_FILES)
  { 
    if (m_files[i].used)
    {   
      m_files[i].file_lock->Release();
    }
  }
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      return &m_files[i];
    }
  }
  return NULL;
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByStream(FILE* stream)
{
  if (stream != NULL)
  {
    return GetFileObjectByDescriptor(stream->_file);
  }

  return NULL;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByDescriptor(int fd)
{
  EmuFileObject* object = GetFileObjectByDescriptor(fd);
  if (object != NULL && object->used)
  {
    return object->file_xbmc;
  }
  return NULL;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByStream(FILE* stream)
{
  if (stream != NULL)
  {
    EmuFileObject* object = GetFileObjectByDescriptor(stream->_file);
    if (object != NULL && object->used)
    {
      return object->file_xbmc;
    }
  }
  return NULL;
}

int CEmuFileWrapper::GetDescriptorByStream(FILE* stream)
{
  if (stream != NULL)
  {
    int i = stream->_file - FILE_WRAPPER_OFFSET;
    if (i >= 0 && i < MAX_EMULATED_FILES)
    {
      return stream->_file;
    }
  }
  return -1;
}

FILE* CEmuFileWrapper::GetStreamByDescriptor(int fd)
{
  EmuFileObject* object = GetFileObjectByDescriptor(fd);
  if (object != NULL && object->used)
  {
    return &object->file_emu;
  }
  return NULL;
}

bool CEmuFileWrapper::DescriptorIsEmulatedFile(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    return true;
  }
  return false;
}

bool CEmuFileWrapper::StreamIsEmulatedFile(FILE* stream)
{
  if (stream != NULL)
  {
    return DescriptorIsEmulatedFile(stream->_file);
  }
  return false;
}
