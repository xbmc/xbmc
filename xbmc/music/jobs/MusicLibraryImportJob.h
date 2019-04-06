/*
*  Copyright (C) 2017-2018 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

#include "MusicLibraryProgressJob.h"

class CGUIDialogProgress;

/*!
\brief Music library job implementation for importing data to the music library.
*/
class CMusicLibraryImportJob : public CMusicLibraryProgressJob
{
public:
  /*!
  \brief Creates a new music library import job for the given xml file.

  \param[in] xmlFile       xml file to import
  \param[in] progressDialog Progress dialog to be used to display the import progress
  */
  CMusicLibraryImportJob(const std::string &xmlFile, CGUIDialogProgress* progressDialog);

  ~CMusicLibraryImportJob() override;

  // specialization of CJob
  const char *GetType() const override { return "MusicLibraryImportJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CMusicLibraryJob
  bool Work(CMusicDatabase &db) override;

private:
  std::string m_xmlFile;
};

