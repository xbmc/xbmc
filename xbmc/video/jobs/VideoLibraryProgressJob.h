/*
 *      Copyright (C) 2014 Team XBMC
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

#include "utils/ProgressJob.h"
#include "video/jobs/VideoLibraryJob.h"

/*!
 \brief Combined base implementation of a video library job with a progress bar.
 */
class CVideoLibraryProgressJob : public CProgressJob, public CVideoLibraryJob
{
public:
  ~CVideoLibraryProgressJob() override;

  // implementation of CJob
  bool DoWork() override;
  const char *GetType() const override { return "CVideoLibraryProgressJob"; }
  bool operator==(const CJob* job) const override { return false; }

protected:
  explicit CVideoLibraryProgressJob(CGUIDialogProgressBarHandle* progressBar);
};
