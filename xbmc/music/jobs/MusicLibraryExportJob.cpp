/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicLibraryExportJob.h"
#include "dialogs/GUIDialogProgress.h"
#include "music/MusicDatabase.h"
#include "settings/LibExportSettings.h"

CMusicLibraryExportJob::CMusicLibraryExportJob(const CLibExportSettings& settings, CGUIDialogProgress* progressDialog)
  : CMusicLibraryProgressJob(NULL),
    m_settings(settings)
{
  if (progressDialog)
    SetProgressIndicators(NULL, progressDialog);
  SetAutoClose(true);
}

CMusicLibraryExportJob::~CMusicLibraryExportJob() = default;

bool CMusicLibraryExportJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CMusicLibraryExportJob* exportJob = dynamic_cast<const CMusicLibraryExportJob*>(job);
  if (exportJob == NULL)
    return false;

  return !(m_settings != exportJob->m_settings);
}

bool CMusicLibraryExportJob::Work(CMusicDatabase &db)
{
  db.ExportToXML(m_settings, GetProgressDialog());

  return true;
}
