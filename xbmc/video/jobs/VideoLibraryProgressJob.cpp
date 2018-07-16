/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryProgressJob.h"

CVideoLibraryProgressJob::CVideoLibraryProgressJob(CGUIDialogProgressBarHandle* progressBar)
  : CProgressJob(progressBar)
{ }

CVideoLibraryProgressJob::~CVideoLibraryProgressJob() = default;

bool CVideoLibraryProgressJob::DoWork()
{
  bool result = CVideoLibraryJob::DoWork();

  MarkFinished();

  return result;
}
