
#ifndef _DLL_TRACKER_FILE
#define _DLL_TRACKER_FILE

#include "dll_tracker.h"

extern "C" void tracker_file_track(unsigned long caller, unsigned handle, TrackedFileType type, const char* sFile = "");
extern "C" void tracker_file_free(unsigned long caller, unsigned handle, TrackedFileType type);
extern "C" void tracker_file_free_all(DllTrackInfo* pInfo);

extern "C"
{
  int track_open(const char* sFileName, int iMode);
  int track_close(int fd);
  FILE* track_fopen(const char* sFileName, const char* mode);
  int track_fclose(FILE* stream);
  FILE* track_freopen(const char *path, const char *mode, FILE *stream);
}

#endif // _DLL_TRACKER_FILE