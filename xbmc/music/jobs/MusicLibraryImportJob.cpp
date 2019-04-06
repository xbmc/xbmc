/*
*  Copyright (C) 2017-2018 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "MusicLibraryImportJob.h"
#include "dialogs/GUIDialogProgress.h"
#include "music/MusicDatabase.h"

CMusicLibraryImportJob::CMusicLibraryImportJob(const std::string& xmlFile, CGUIDialogProgress* progressDialog)
  : CMusicLibraryProgressJob(nullptr)
  ,  m_xmlFile(xmlFile)
{
  if (progressDialog)
    SetProgressIndicators(nullptr, progressDialog);
  SetAutoClose(true);
}

CMusicLibraryImportJob::~CMusicLibraryImportJob() = default;

bool CMusicLibraryImportJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CMusicLibraryImportJob* importJob = dynamic_cast<const CMusicLibraryImportJob*>(job);
  if (importJob == nullptr)
    return false;

  return !(m_xmlFile != importJob->m_xmlFile);
}

bool CMusicLibraryImportJob::Work(CMusicDatabase &db)
{
  db.ImportFromXML(m_xmlFile, GetProgressDialog());

  return true;
}
