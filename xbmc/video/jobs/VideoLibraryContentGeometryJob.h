/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/jobs/VideoLibraryProgressJob.h"

#include <atomic>

/*!
 * \brief Measure the content geometry of every library file that does not have one. The work
 * list is recomputed from the database on each run, so the job is resumable.
 *
 * Playback and any other video library job suspend it between files, and the file being
 * measured when playback starts is abandoned mid-sample.
 */
class CVideoLibraryContentGeometryJob : public CVideoLibraryProgressJob
{
public:
  //! \brief \p retryFailed also measures files a previous run could not read, which is the
  //! only way to retry one that has not changed since. Off for the automatic sweeps.
  explicit CVideoLibraryContentGeometryJob(bool retryFailed);
  ~CVideoLibraryContentGeometryJob() override;

  // implementation of CJob
  const char* GetType() const override { return "VideoLibraryContentGeometryJob"; }
  bool Equals(const CJob* job) const override;

  // implementation of CVideoLibraryJob
  bool CanBeCancelled() const override { return true; }
  bool Cancel() override;

protected:
  bool Work(CVideoDatabase& db) override;

private:
  bool IsCancelled() const;

  //! \brief Should the file currently being measured be abandoned?
  bool ShouldYield() const;

  //! \brief Block until nothing is playing and no other library job is running.
  //! \return false if the job was cancelled while waiting
  bool WaitUntilIdle() const;

  std::atomic<bool> m_cancelled{false};
  const bool m_retryFailed;
};
