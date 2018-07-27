/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
