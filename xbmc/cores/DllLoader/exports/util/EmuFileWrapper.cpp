/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "EmuFileWrapper.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"

CEmuFileWrapper g_emuFileWrapper;

namespace
{

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr);
}

}
CEmuFileWrapper::CEmuFileWrapper()
{
  // since we always use dlls we might just initialize it directly
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    memset(&m_files[i], 0, sizeof(EmuFileObject));
    m_files[i].used = false;
    m_files[i].fd = -1;
  }
}

CEmuFileWrapper::~CEmuFileWrapper()
{
  CleanUp();
}

void CEmuFileWrapper::CleanUp()
{
  CSingleLock lock(m_criticalSection);
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (m_files[i].used)
    {
      m_files[i].file_xbmc->Close();
      delete m_files[i].file_xbmc;

      if (m_files[i].file_lock)
      {
        delete m_files[i].file_lock;
        m_files[i].file_lock = nullptr;
      }
      m_files[i].used = false;
      m_files[i].fd = -1;
    }
  }
}

EmuFileObject* CEmuFileWrapper::RegisterFileObject(XFILE::CFile* pFile)
{
  EmuFileObject* object = nullptr;

  CSingleLock lock(m_criticalSection);

  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (!m_files[i].used)
    {
      // found a free location
      object = &m_files[i];
      object->used = true;
      object->file_xbmc = pFile;
      object->fd = (i + FILE_WRAPPER_OFFSET);
      object->file_lock = new CCriticalSection();
      break;
    }
  }

  return object;
}

void CEmuFileWrapper::UnRegisterFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (! (i >= 0 && i < MAX_EMULATED_FILES))
    return;

  if (!m_files[i].used)
    return;

  CSingleLock lock(m_criticalSection);

  // we assume the emulated function already deleted the CFile object
  if (m_files[i].file_lock)
  {
    delete m_files[i].file_lock;
    m_files[i].file_lock = nullptr;
  }
  m_files[i].used = false;
  m_files[i].fd = -1;
}

void CEmuFileWrapper::UnRegisterFileObjectByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* o = reinterpret_cast<EmuFileObject*>(stream);
    return UnRegisterFileObjectByDescriptor(o->fd);
  }
}

void CEmuFileWrapper::LockFileObjectByDescriptor(int fd)
{
  int i = fd - FILE_WRAPPER_OFFSET;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].used)
    {
      m_files[i].file_lock->lock();
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
      return m_files[i].file_lock->try_lock();
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
      m_files[i].file_lock->unlock();
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
  return nullptr;
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* o = reinterpret_cast<EmuFileObject*>(stream);
    return GetFileObjectByDescriptor(o->fd);
  }

  return nullptr;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByDescriptor(int fd)
{
  auto object = GetFileObjectByDescriptor(fd);
  if (object != nullptr && object->used)
  {
    return object->file_xbmc;
  }
  return nullptr;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* object = reinterpret_cast<EmuFileObject*>(stream);
    if (object != nullptr && object->used)
    {
      return object->file_xbmc;
    }
  }
  return nullptr;
}

int CEmuFileWrapper::GetDescriptorByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* obj = reinterpret_cast<EmuFileObject*>(stream);
    int i = obj->fd - FILE_WRAPPER_OFFSET;
    if (i >= 0 && i < MAX_EMULATED_FILES)
    {
      return i + FILE_WRAPPER_OFFSET;
    }
  }
  return -1;
}

FILE* CEmuFileWrapper::GetStreamByDescriptor(int fd)
{
  auto object = GetFileObjectByDescriptor(fd);
  if (object != nullptr && object->used)
  {
    return reinterpret_cast<FILE*>(object);
  }
  return nullptr;
}

bool CEmuFileWrapper::StreamIsEmulatedFile(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* obj = reinterpret_cast<EmuFileObject*>(stream);
    return DescriptorIsEmulatedFile(obj->fd);
  }
  return false;
}
