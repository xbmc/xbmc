#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>

#include "system.h"
#include "threads/CriticalSection.h"

#define MAX_EMULATED_FILES    50

namespace XFILE
{
  class CFile;
}

#if defined(TARGET_WINDOWS) && _MSC_VER >= 1900
struct kodi_iobuf {
  int   _file;
};
#endif

typedef struct stEmuFileObject
{
  XFILE::CFile*  file_xbmc;
  CCriticalSection *file_lock;
  int mode;
} EmuFileObject;

class CEmuFileWrapper
{
public:
  CEmuFileWrapper();
  ~CEmuFileWrapper();

  /**
   * Only to be called when shutting down xbmc
   */
  void CleanUp();

  EmuFileObject* RegisterFileObject(XFILE::CFile* pFile);
  void UnRegisterFileObject(EmuFileObject*, bool free_file);
  void UnRegisterFileObjectByDescriptor(int fd);
  void UnRegisterFileObjectByStream(FILE* stream);
  void LockFileObjectByDescriptor(int fd);
  bool TryLockFileObjectByDescriptor(int fd);
  void UnlockFileObjectByDescriptor(int fd);
  EmuFileObject* GetFileObjectByDescriptor(int fd);
  int GetDescriptorByFileObject(EmuFileObject*);
  EmuFileObject* GetFileObjectByStream(FILE* stream);
  FILE* GetStreamByFileObject(EmuFileObject*);
  XFILE::CFile* GetFileXbmcByDescriptor(int fd);
  XFILE::CFile* GetFileXbmcByStream(FILE* stream);
  int GetDescriptorByStream(FILE* stream);
  FILE* GetStreamByDescriptor(int fd);
  bool DescriptorIsEmulatedFile(int fd);
  bool StreamIsEmulatedFile(FILE* stream);
private:
  EmuFileObject m_files[MAX_EMULATED_FILES];
  CCriticalSection m_criticalSection;
};

extern CEmuFileWrapper g_emuFileWrapper;

