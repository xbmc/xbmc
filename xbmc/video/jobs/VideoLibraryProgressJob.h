/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
