#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
  virtual ~CVideoLibraryCleaningJob();

  // specialization of CJob
  virtual const char *GetType() const { return "VideoLibraryCleaningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CVideoLibraryJob
  virtual bool Work(CVideoDatabase &db);

private:
  std::set<int> m_paths;
  bool m_showDialog;
};
