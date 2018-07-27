/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>

#include "video/jobs/VideoLibraryProgressJob.h"

class CGUIDialogProgressBarHandle;

/*!
 \brief Video library job implementation for cleaning the video library.
*/
class CVideoLibraryCleaningJob : public CVideoLibraryProgressJob
{
public:
  /*!
   \brief Creates a new video library cleaning job for the given paths.

   \param[in] paths Set with database IDs of paths to be cleaned
   \param[in] showDialog Whether to show a modal dialog or not
  */
  CVideoLibraryCleaningJob(const std::set<int>& paths = std::set<int>(), bool showDialog = false);

  /*!
  \brief Creates a new video library cleaning job for the given paths.

  \param[in] paths Set with database IDs of paths to be cleaned
  \param[in] progressBar Progress bar to be used to display the cleaning progress
  */
  CVideoLibraryCleaningJob(const std::set<int>& paths, CGUIDialogProgressBarHandle* progressBar);
  ~CVideoLibraryCleaningJob() override;

  // specialization of CJob
  const char *GetType() const override { return "VideoLibraryCleaningJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CVideoLibraryJob
  bool Work(CVideoDatabase &db) override;

private:
  std::set<int> m_paths;
  bool m_showDialog;
};
