#ifndef _DLL_TRACKER_FILE
#define _DLL_TRACKER_FILE

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "dll_tracker.h"
#include <stdio.h>

extern "C" void tracker_file_track(uintptr_t caller, uintptr_t handle, TrackedFileType type, const char* sFile = "");
extern "C" void tracker_file_free(uintptr_t caller, uintptr_t handle, TrackedFileType type);
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
