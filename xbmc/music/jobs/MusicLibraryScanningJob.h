/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>

#include "music/infoscanner/MusicInfoScanner.h"
#include "music/jobs/MusicLibraryJob.h"

/*!
 \brief Music library job implementation for scanning items.
 Uses CMusicInfoScanner for scanning and can be run with  or
 without a visible progress bar.
 */
class CMusicLibraryScanningJob : public CMusicLibraryJob
{
public:
  /*!
   \brief Creates a new music library tag scanning and data scraping job.
   \param[in] directory Directory to be scanned for new items
   \param[in] flags What kind of scan to do
   \param[in] showProgress Whether to show a progress bar or not
   */
  CMusicLibraryScanningJob(const std::string& directory, int flags, bool showProgress = true);
  ~CMusicLibraryScanningJob() override;

  // specialization of CMusicLibraryJob
  bool CanBeCancelled() const override { return true; }
  bool Cancel() override;

  // specialization of CJob
  const char *GetType() const override { return "MusicLibraryScanningJob"; }
  bool operator==(const CJob* job) const override;

protected:
  // implementation of CMusicLibraryJob
  bool Work(CMusicDatabase &db) override;

private:
  MUSIC_INFO::CMusicInfoScanner m_scanner;
  std::string m_directory;
  bool m_showProgress;
  int m_flags;
};
