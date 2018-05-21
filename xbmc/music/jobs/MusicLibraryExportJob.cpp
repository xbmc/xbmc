/*
 *      Copyright (C) 2017-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
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
