/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryCleaningJob.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "video/VideoDatabase.h"

CVideoLibraryCleaningJob::CVideoLibraryCleaningJob(const std::set<int>& paths /* = std::set<int>() */, bool showDialog /* = false */)
  : CVideoLibraryProgressJob(NULL),
    m_paths(paths),
    m_showDialog(showDialog)
{ }

CVideoLibraryCleaningJob::CVideoLibraryCleaningJob(const std::set<int>& paths, CGUIDialogProgressBarHandle* progressBar)
  : CVideoLibraryProgressJob(progressBar),
    m_paths(paths),
    m_showDialog(false)
{ }

CVideoLibraryCleaningJob::~CVideoLibraryCleaningJob() = default;

bool CVideoLibraryCleaningJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryCleaningJob* cleaningJob = dynamic_cast<const CVideoLibraryCleaningJob*>(job);
  if (cleaningJob == NULL)
    return false;

  return m_paths == cleaningJob->m_paths &&
         m_showDialog == cleaningJob->m_showDialog;
}

bool CVideoLibraryCleaningJob::Work(CVideoDatabase &db)
{
  db.CleanDatabase(GetProgressBar(), m_paths, m_showDialog);
  return true;
}
