/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

