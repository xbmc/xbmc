/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "dll_tracker_file.h"
#include "dll_tracker.h"
#include "DllLoader.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include <stdlib.h>

#ifdef TARGET_POSIX
#define dll_open open
#define dll_fopen fopen
#define dll_close close
#define dll_fclose fclose
#define dll_freopen freopen
#else
#include "exports/emu_msvcrt.h"
#include <io.h>
#endif

extern "C" void tracker_file_track(uintptr_t caller, uintptr_t handle, TrackedFileType type, const char* sFile)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    TrackedFile* file = new TrackedFile;
    file->handle = handle;
    file->type = type;
    file->name = strdup(sFile);
    pInfo->fileList.push_back(file);
  }
}

extern "C" void tracker_file_free(uintptr_t caller, uintptr_t handle, TrackedFileType type)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    CSingleLock lock(g_trackerLock);
    for (FileListIter it = pInfo->fileList.begin(); it != pInfo->fileList.end(); ++it)
    {
      TrackedFile* file = *it;
      if (file->handle == handle && file->type == type)
      {
        free(file->name);
        delete file;
        pInfo->fileList.erase(it);
        return;
      }
    }
  }
  CLog::Log(LOGWARNING, "unable to remove tracked file from tracker");
}

extern "C" void tracker_file_free_all(DllTrackInfo* pInfo)
{
  if (!pInfo->fileList.empty())
  {
    CSingleLock lock(g_trackerLock);
    CLog::Log(LOGDEBUG, "{0}: Detected open files: {1}", pInfo->pDll->GetFileName(), pInfo->fileList.size());
    for (FileListIter it = pInfo->fileList.begin(); it != pInfo->fileList.end(); ++it)
    {
      TrackedFile* file = *it;
      CLog::Log(LOGDEBUG, "%s", file->name);
      free(file->name);

      if (file->type == FILE_XBMC_OPEN) dll_close(file->handle);
      else if (file->type == FILE_XBMC_FOPEN) dll_fclose((FILE*)file->handle);
      else if (file->type == FILE_OPEN) close(file->handle);
      else if (file->type == FILE_FOPEN) fclose((FILE*)file->handle);

      delete file;
    }
  }
  pInfo->fileList.erase(pInfo->fileList.begin(), pInfo->fileList.end());
}

extern "C"
{
  int track_open(const char* sFileName, int iMode)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();

    int fd = dll_open(sFileName, iMode);
    if (fd >= 0) tracker_file_track(loc, fd, FILE_XBMC_OPEN, sFileName);
    return fd;
  }

  int track_close(int fd)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();

    tracker_file_free(loc, fd, FILE_XBMC_OPEN);
    return dll_close(fd);
  }

  FILE* track_fopen(const char* sFileName, const char* mode)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();

    FILE* fd = dll_fopen(sFileName, mode);
    if (fd) tracker_file_track(loc, (uintptr_t)fd, FILE_XBMC_FOPEN, sFileName);
    return fd;
  }

  int track_fclose(FILE* stream)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();

    tracker_file_free(loc, (uintptr_t)stream, FILE_XBMC_FOPEN);
    return dll_fclose(stream);
  }

  FILE* track_freopen(const char *path, const char *mode, FILE *stream)
  {
    uintptr_t loc = (uintptr_t)_ReturnAddress();

    tracker_file_free(loc, (uintptr_t)stream, FILE_XBMC_FOPEN);
    stream = dll_freopen(path, mode, stream);
    if (stream)
      tracker_file_track(loc, (uintptr_t)stream, FILE_XBMC_FOPEN, path);
    return stream;
  }

}
