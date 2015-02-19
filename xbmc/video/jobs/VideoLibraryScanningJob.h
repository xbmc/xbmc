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

#include <string>

#include "video/VideoInfoScanner.h"
#include "video/jobs/VideoLibraryJob.h"

/*!
 \brief Video library job implementation for scanning items.

 Uses CVideoInfoScanner for the whole filesystem scanning and can be run with
 or without a visible progress bar.
 */
class CVideoLibraryScanningJob : public CVideoLibraryJob
{
public:
  /*!
   \brief Creates a new video library scanning job.

   \param[in] directory Directory to be scanned for new items
   \param[in] scanAll Whether to scan all items or not
   \param[in] showProgress Whether to show a progress bar or not
   */
  CVideoLibraryScanningJob(const std::string& directory, bool scanAll = false, bool showProgress = true);
  virtual ~CVideoLibraryScanningJob();

  // specialization of CVideoLibraryJob
  virtual bool CanBeCancelled() const { return true; }
  virtual bool Cancel();

  // specialization of CJob
  virtual const char *GetType() const { return "VideoLibraryScanningJob"; }
  virtual bool operator==(const CJob* job) const;

protected:
  // implementation of CVideoLibraryJob
  virtual bool Work(CVideoDatabase &db);

private:
  VIDEO::CVideoInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
  bool m_scanAll;
};
