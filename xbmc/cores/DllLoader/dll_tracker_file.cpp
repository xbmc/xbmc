
#include "../../stdafx.h"
#include "dll_tracker_file.h"
#include "dll_tracker.h"
#include "DllLoader.h"
#include "exports/emu_msvcrt.h"
#include <io.h>

extern "C" void tracker_file_track(unsigned long caller, unsigned handle, TrackedFileType type, const char* sFile)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    TrackedFile* file = new TrackedFile;
    file->handle = handle;
    file->type = type;
    file->name = strdup(sFile);
    pInfo->fileList.push_back(file);
  }
}

extern "C" void tracker_file_free(unsigned long caller, unsigned handle, TrackedFileType type)
{
  DllTrackInfo* pInfo = tracker_get_dlltrackinfo(caller);
  if (pInfo)
  {
    TrackedFile* file;
    for (FileListIter it = pInfo->fileList.begin(); it != pInfo->fileList.end(); ++it)
    {
      file = *it;
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
    TrackedFile* file;
    CLog::DebugLog("%s: Detected open files: %d", pInfo->pDll->GetFileName(), pInfo->fileList.size());
    for (FileListIter it = pInfo->fileList.begin(); it != pInfo->fileList.end(); ++it)
    {
      file = *it;
      CLog::DebugLog(file->name);
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
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    int fd = dll_open(sFileName, iMode);
    if (fd >= 0) tracker_file_track(loc, fd, FILE_XBMC_OPEN, sFileName);
    return fd;
  }

  int track_close(int fd)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    tracker_file_free(loc, fd, FILE_XBMC_OPEN);
    return dll_close(fd);
  }

  FILE* track_fopen(const char* sFileName, const char* mode)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    FILE* fd = dll_fopen(sFileName, mode);
    if (fd) tracker_file_track(loc, (unsigned)fd, FILE_XBMC_FOPEN, sFileName);
    return fd;
  }
  
  int track_fclose(FILE* stream)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    tracker_file_free(loc, (unsigned)stream, FILE_XBMC_FOPEN);
    return dll_fclose(stream);
  }
  
  FILE* track_freopen(const char *path, const char *mode, FILE *stream)
  {
    unsigned loc;
    __asm mov eax, [ebp + 4]
    __asm mov loc, eax
    
    tracker_file_free(loc, (unsigned)stream, FILE_XBMC_FOPEN);
    stream = freopen(path, mode, stream);
    if (stream) tracker_file_track(loc, (unsigned)stream, FILE_XBMC_FOPEN, path);
    return stream;
  }

}