/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/IJobCallback.h"
#include "video/geometry/ContentGeometryRecord.h"
#include "video/geometry/FrameSampling.h"

#include <atomic>
#include <functional>
#include <optional>

class CFileItem;

namespace KODI::VIDEO::GEOMETRY
{

//! \brief Whether \p item can be measured: the extractors' policy, plus no stacks.
bool CanMeasureContentGeometry(const CFileItem& item);

//! \brief Sample \p item and produce the row to store. Nothing when sampling was abandoned; a
//! file that could not be read is a Failed record rather than nothing.
std::optional<ContentGeometryRecord> MeasureContentGeometry(
    const CFileItem& item,
    const FileIdentity& identity,
    SamplingDepth depth = SamplingDepth::Normal,
    const std::function<bool()>& cancelled = {});

//! \brief Measure the file \p item plays, unless something already has, and store it. Blocks,
//! so the rectangle is known before the first frame. Skips a file with a declared ratio.
void MeasureContentGeometryBeforePlayback(const CFileItem& item,
                                          const std::function<bool()>& cancelled);

//! \brief MeasureContentGeometryBeforePlayback() under the busy dialog. Video items only;
//! backing out abandons the measurement and starts the film.
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

  //! \brief Run the background sweep over everything that still needs measuring. Nothing when
  //! one is already running or the library has not opted in. It suspends itself while any
  //! other video library job runs rather than queueing behind it.
  void Sweep(bool retryFailed = false);

  bool IsSweeping() const { return m_sweeping; }

  //! \brief Ask a running sweep to stop, abandoning the file it is measuring.
  void StopSweep() { m_stop = true; }

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

  //! \brief The job manager owns the job and may destroy it before the completion callback
  //! arrives.
  std::atomic<bool> m_sweeping{false};
  std::atomic<bool> m_stop{false};
  std::atomic<bool> m_opening{false};
};

} // namespace KODI::VIDEO::GEOMETRY
