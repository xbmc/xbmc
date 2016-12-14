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

#if defined(TARGET_WINDOWS) && (_MSC_VER >= 1900)
constexpr kodi_iobuf* FileDescriptor(FILE& f)
{
  return static_cast<kodi_iobuf*>(f._Placeholder);
}

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr && f->_Placeholder != nullptr);
}

#else
constexpr FILE* FileDescriptor(FILE& f)
{
  return &f;
}

constexpr bool isValidFilePtr(FILE* f)
{
  return (f != nullptr);
}
#endif
}
CEmuFileWrapper::CEmuFileWrapper()
{
  // since we always use dlls we might just initialize it directly
  memset(m_files, 0, sizeof(m_files));
}

CEmuFileWrapper::~CEmuFileWrapper()
{
  CleanUp();
}

void CEmuFileWrapper::CleanUp()
{
  CSingleLock lock(m_criticalSection);
  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  UnRegisterFileObject(&m_files[i], true);
}

EmuFileObject* CEmuFileWrapper::RegisterFileObject(XFILE::CFile* pFile)
{
  EmuFileObject* object = nullptr;

  CSingleLock lock(m_criticalSection);

  for (int i = 0; i < MAX_EMULATED_FILES; i++)
  {
    if (!m_files[i].file_xbmc)
    {
      // found a free location
      object = &m_files[i];
      object->file_xbmc = pFile;
      object->file_lock = new CCriticalSection();
      break;
    }
  }

  return object;
}

void CEmuFileWrapper::UnRegisterFileObject(EmuFileObject *object, bool free_file)
{
  if (object && object->file_xbmc)
  {
    if (object->file_xbmc && free_file)
    {
      object->file_xbmc->Close();
      delete object->file_xbmc;
    }
    if (object->file_lock)
    {
      delete object->file_lock;
    }

    memset(object, 0, sizeof(*object));
  }
}

void CEmuFileWrapper::UnRegisterFileObjectByDescriptor(int fd)
{
  CSingleLock lock(m_criticalSection);
  UnRegisterFileObject(GetFileObjectByDescriptor(fd), false);
}

void CEmuFileWrapper::UnRegisterFileObjectByStream(FILE* stream)
{
  CSingleLock lock(m_criticalSection);
  UnRegisterFileObject(GetFileObjectByStream(stream), false);
}

void CEmuFileWrapper::LockFileObjectByDescriptor(int fd)
{
  EmuFileObject* object = GetFileObjectByDescriptor(fd);
  if (object && object->file_xbmc)
  {
    object->file_lock->lock();
  }
}

bool CEmuFileWrapper::TryLockFileObjectByDescriptor(int fd)
{
  EmuFileObject* object = GetFileObjectByDescriptor(fd);
  if (object && object->file_xbmc)
  {
    return object->file_lock->try_lock();
  }
  return false;
}

void CEmuFileWrapper::UnlockFileObjectByDescriptor(int fd)
{
  EmuFileObject* object = GetFileObjectByDescriptor(fd);
  if (object && object->file_xbmc)
  {
    object->file_lock->unlock();
  }
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByDescriptor(int fd)
{
  int i = fd - 0x7000000;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    if (m_files[i].file_xbmc)
    {
      return &m_files[i];
    }
  }
  return nullptr;
}

int CEmuFileWrapper::GetDescriptorByFileObject(EmuFileObject *object)
{
  int i = object - m_files;
  if (i >= 0 && i < MAX_EMULATED_FILES)
  {
    return 0x7000000 + i;
  }
  return -1;
}

EmuFileObject* CEmuFileWrapper::GetFileObjectByStream(FILE* stream)
{
  EmuFileObject *object = (EmuFileObject*) stream;
  if (object >= &m_files[0] || object < &m_files[MAX_EMULATED_FILES])
  {
    if (object->file_xbmc)
    {
      return object;
    }
  }
  return nullptr;
}

FILE* CEmuFileWrapper::GetStreamByFileObject(EmuFileObject *object)
{
  return (FILE*) object;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByDescriptor(int fd)
{
  auto object = GetFileObjectByDescriptor(fd);
  if (object != nullptr)
  {
    return object->file_xbmc;
  }
  return nullptr;
}

XFILE::CFile* CEmuFileWrapper::GetFileXbmcByStream(FILE* stream)
{
  if (isValidFilePtr(stream))
  {
    EmuFileObject* object = GetFileObjectByStream(stream);
    if (object != nullptr)
    {
      return object->file_xbmc;
    }
  }
  return nullptr;
}

int CEmuFileWrapper::GetDescriptorByStream(FILE* stream)
{
  return GetDescriptorByFileObject(GetFileObjectByStream(stream));
}

FILE* CEmuFileWrapper::GetStreamByDescriptor(int fd)
{
  return GetStreamByFileObject(GetFileObjectByDescriptor(fd));
}

bool CEmuFileWrapper::DescriptorIsEmulatedFile(int fd)
{
  return GetFileObjectByDescriptor(fd) != NULL;
}

bool CEmuFileWrapper::StreamIsEmulatedFile(FILE* stream)
{
  return GetFileObjectByStream(stream) != NULL;
}
