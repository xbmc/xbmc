#pragma once
/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#include "MusicLibraryProgressJob.h"
#include "settings/LibExportSettings.h"

class CGUIDialogProgress;

/*!
 \brief Music library job implementation for exporting the music library.
*/
class CMusicLibraryExportJob : public CMusicLibraryProgressJob
{
public:
  /*!
  \brief Creates a new music library export job for the given paths.

  \param[in] settings       Library export settings
  \param[in] progressDialog Progress dialog to be used to display the export progress
  */
  CMusicLibraryExportJob(const CLibExportSettings& settings, CGUIDialogProgress* progressDialog);

  ~CMusicLibraryExportJob() override;

  // specialization of CJob
  const char *GetType() const override { return "MusicLibraryExportJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CMusicLibraryJob
  bool Work(CMusicDatabase &db) override;

private:
  CLibExportSettings m_settings;
};
