/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicLibraryCleaningJob.h"
#include "dialogs/GUIDialogProgress.h"
#include "music/MusicDatabase.h"

CMusicLibraryCleaningJob::CMusicLibraryCleaningJob(CGUIDialogProgress* progressDialog)
  : CMusicLibraryProgressJob(nullptr)
{
  if (progressDialog)
    SetProgressIndicators(nullptr, progressDialog);
  SetAutoClose(true);
}

CMusicLibraryCleaningJob::~CMusicLibraryCleaningJob() = default;

bool CMusicLibraryCleaningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CMusicLibraryCleaningJob* cleaningJob = dynamic_cast<const CMusicLibraryCleaningJob*>(job);
  if (cleaningJob == nullptr)
    return false;

  return true;
}

bool CMusicLibraryCleaningJob::Work(CMusicDatabase &db)
{
  db.Cleanup(GetProgressDialog());
  return true;
}
