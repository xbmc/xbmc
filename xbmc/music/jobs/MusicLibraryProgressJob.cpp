/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicLibraryProgressJob.h"

CMusicLibraryProgressJob::CMusicLibraryProgressJob(CGUIDialogProgressBarHandle* progressBar)
  : CProgressJob(progressBar)
{ }

CMusicLibraryProgressJob::~CMusicLibraryProgressJob() = default;

bool CMusicLibraryProgressJob::DoWork()
{
  bool result = CMusicLibraryJob::DoWork();

  MarkFinished();

  return result;
}
