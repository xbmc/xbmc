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

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN) && !defined(TARGET_FREEBSD) && !defined(TARGET_ANDROID) && !defined(__UCLIBC__)
#define _file _fileno
#elif defined(__UCLIBC__)
#define _file __filedes
#endif

#define MAX_EMULATED_FILES    50
#define FILE_WRAPPER_OFFSET   0x00000200

namespace XFILE
{
  class CFile;
}

#if (defined(TARGET_WINDOWS) && (_MSC_VER >= 1900)) || defined(TARGET_WIN10)
struct kodi_iobuf {
  int   _file;
};
#endif

typedef struct stEmuFileObject
{
  FILE    file_emu;
  XFILE::CFile*  file_xbmc;
  CCriticalSection *file_lock;
  int mode;
  //Stick this last to avoid 3-7 bytes of padding
  bool    used;
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
  void UnRegisterFileObjectByDescriptor(int fd);
  void UnRegisterFileObjectByStream(FILE* stream);
  void LockFileObjectByDescriptor(int fd);
  bool TryLockFileObjectByDescriptor(int fd);
  void UnlockFileObjectByDescriptor(int fd);
  EmuFileObject* GetFileObjectByDescriptor(int fd);
  EmuFileObject* GetFileObjectByStream(FILE* stream);
  XFILE::CFile* GetFileXbmcByDescriptor(int fd);
  XFILE::CFile* GetFileXbmcByStream(FILE* stream);
  static int GetDescriptorByStream(FILE* stream);
  FILE* GetStreamByDescriptor(int fd);
  static constexpr bool DescriptorIsEmulatedFile(int fd)
  {
    return fd >= FILE_WRAPPER_OFFSET && fd < FILE_WRAPPER_OFFSET + MAX_EMULATED_FILES;
  }
  static bool StreamIsEmulatedFile(FILE* stream);
private:
  EmuFileObject m_files[MAX_EMULATED_FILES];
  CCriticalSection m_criticalSection;
};

extern CEmuFileWrapper g_emuFileWrapper;

