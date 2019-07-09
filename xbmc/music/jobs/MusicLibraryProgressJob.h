/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/jobs/MusicLibraryJob.h"
#include "utils/ProgressJob.h"

/*!
 \brief Combined base implementation of a music library job with a progress bar.
 */
class CMusicLibraryProgressJob : public CProgressJob, public CMusicLibraryJob
{
public:
  ~CMusicLibraryProgressJob() override;

  // implementation of CJob
  bool DoWork() override;
  const char *GetType() const override { return "CMusicLibraryProgressJob"; }
  bool operator==(const CJob* job) const override { return false; }

protected:
  explicit CMusicLibraryProgressJob(CGUIDialogProgressBarHandle* progressBar);
};
