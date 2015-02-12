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

#include "utils/ProgressJob.h"
#include "video/jobs/VideoLibraryJob.h"

/*!
 \brief Combined base implementation of a video library job with a progress bar.
 */
class CVideoLibraryProgressJob : public CProgressJob, public CVideoLibraryJob
{
public:
  virtual ~CVideoLibraryProgressJob();

  // implementation of CJob
  virtual bool DoWork();
  virtual const char *GetType() const { return "CVideoLibraryProgressJob"; }
  virtual bool operator==(const CJob* job) const { return false; }

protected:
  CVideoLibraryProgressJob(CGUIDialogProgressBarHandle* progressBar);
};
