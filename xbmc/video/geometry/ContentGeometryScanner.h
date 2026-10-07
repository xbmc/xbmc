/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/IJobCallback.h"
#include "threads/CriticalSection.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/FrameSampling.h"

#include <atomic>
#include <functional>
#include <optional>

class CFileItem;

namespace KODI::VIDEO::GEOMETRY
{

//! \brief The identity to measure \p item under. Nothing when the extractors' policy refuses
//! it, it is a stack, or its size and time cannot be read.
std::optional<FileIdentity> MeasurableIdentity(const CFileItem& item);

//! \brief Sample \p item and produce the row to store. Nothing when sampling was abandoned; a
//! file that could not be read gives a record with no ratios.
std::optional<ContentGeometryRecord> MeasureContentGeometry(
    const CFileItem& item,
    const FileIdentity& identity,
    SamplingDepth depth = SamplingDepth::Normal,
    const std::function<bool()>& cancelled = {});

//! \brief Measure the file \p item plays, unless something already has, and store it, under
//! the busy dialog. Blocks, so the rectangle is known before the first frame. Skips a file with
//! a declared ratio. Video items only; backing out abandons the measurement and starts the film.
void MeasureContentGeometryBeforePlaybackBlocking(const CFileItem& item);

//! \brief Measure \p item again and replace what is stored, without asking whether the work is
//! needed, and even where a ratio is declared. False when the item cannot be measured at all.
bool RemeasureContentGeometry(const CFileItem& item,
                              SamplingDepth depth = SamplingDepth::Normal,
                              const std::function<bool()>& cancelled = {});

//! \brief Coordinates the background sweep over everything that still needs measuring.
class CContentGeometryScanner : public IJobCallback
{
public:
  static CContentGeometryScanner& GetInstance();

  //! \brief Run the background sweep over everything that still needs measuring, unless the
  //! library has not opted in. A request made while one runs is held and run when it ends.
  //! It suspends itself while any other video library job runs rather than queueing behind it.
  void Sweep(bool retryFailed = false);

  //! \brief For tests only.
  bool IsSweeping() const;

  //! \brief Ask a running sweep to stop, abandoning the file it is measuring, and drop any
  //! held request.
  void StopSweep();

  //! \brief Read by the running sweep between files and between sample points.
  bool IsStopRequested() const { return m_stop; }

  //! \brief A file is being opened for playback, so the sweep gets off the disk. Suspends it
  //! rather than ending it, and it retakes the abandoned file afterwards. Set before the play
  //! path touches the media and cleared once the player owns it.
  void SetOpeningForPlayback(bool opening) { m_opening = opening; }
  bool IsOpeningForPlayback() const { return m_opening; }

  // implementation of IJobCallback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;
  void OnJobAbort(unsigned int jobID, CJob* job) override;

private:
  CContentGeometryScanner() = default;
  CContentGeometryScanner(const CContentGeometryScanner&) = delete;
  CContentGeometryScanner& operator=(const CContentGeometryScanner&) = delete;

  //! \brief Queue a sweep job, with m_sweeping already set.
  void Start(bool retryFailed);

  //! \brief A sweep job finished or was aborted: start the held request, if any.
  void Ended();

  mutable CCriticalSection m_lock;
  //! \brief A sweep job is queued or running.
  bool m_sweeping{false};
  //! \brief A request made while a sweep ran, holding whether it retries failed files.
  std::optional<bool> m_held;
  std::atomic<bool> m_stop{false};
  std::atomic<bool> m_opening{false};
};

} // namespace KODI::VIDEO::GEOMETRY
