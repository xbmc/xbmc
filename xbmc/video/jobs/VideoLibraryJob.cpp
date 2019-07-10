/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryJob.h"

#include "video/VideoDatabase.h"

CVideoLibraryJob::CVideoLibraryJob() = default;

CVideoLibraryJob::~CVideoLibraryJob() = default;

bool CVideoLibraryJob::DoWork()
{
  CVideoDatabase db;
  if (!db.Open())
    return false;

  return Work(db);
}
