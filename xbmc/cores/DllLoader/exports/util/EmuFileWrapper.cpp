/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  for (EmuFileObject& file : m_files)
  {
    memset(&file, 0, sizeof(EmuFileObject));
    file.used = false;
    file.fd = -1;
  }
}

CEmuFileWrapper::~CEmuFileWrapper()
{
  CleanUp();
}

void CEmuFileWrapper::CleanUp()
{
  CSingleLock lock(m_criticalSection);
  for (EmuFileObject& file : m_files)
  {
    if (file.used)
    {
      file.file_xbmc->Close();
      delete file.file_xbmc;

      if (file.file_lock)
      {
        delete file.file_lock;
        file.file_lock = nullptr;
      }
      file.used = false;
      file.fd = -1;
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
