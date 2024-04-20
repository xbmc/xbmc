/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/VideoInfoScanner.h"
#include "video/jobs/VideoLibraryJob.h"

#include <string>

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
  ~CVideoLibraryScanningJob() override;

  // specialization of CVideoLibraryJob
  bool CanBeCancelled() const override { return true; }
  bool Cancel() override;

  // specialization of CJob
  const char *GetType() const override { return "VideoLibraryScanningJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CVideoLibraryJob
  bool Work(CVideoDatabase &db) override;

private:
  KODI::VIDEO::CVideoInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
  bool m_scanAll;
};
