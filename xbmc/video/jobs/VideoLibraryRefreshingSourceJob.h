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
#include <string>

/*!
 \brief Video library job implementation for refreshing all library items in a source.
*/
class CVideoLibraryRefreshingSourceJob : public CVideoLibraryProgressJob
{
public:
  /*!
   \brief Creates a new video library job refreshing all library items in the given source.

   \param[in] sourcePath Path of the source, which may be a multipath
  */
  explicit CVideoLibraryRefreshingSourceJob(std::string sourcePath);

  const std::string& GetSourcePath() const { return m_sourcePath; }
  bool HasStarted() const { return m_started; }

  // implementation of CVideoLibraryJob
  bool CanBeCancelled() const override { return true; }
  bool Cancel() override;

  static constexpr const char* TYPE = "VideoLibraryRefreshingSourceJob";

  // specialization of CJob
  const char* GetType() const override { return TYPE; }
  bool Equals(const CJob* job) const override;

protected:
  // implementation of CVideoLibraryJob
  bool Work(CVideoDatabase& db) override;

private:
  std::string m_sourcePath;
  std::atomic<bool> m_cancelled{false};
  std::atomic<bool> m_started{false};
};
