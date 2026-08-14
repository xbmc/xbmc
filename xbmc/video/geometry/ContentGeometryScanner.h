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

//! \brief The thumb and flag extractors' policy - local or LAN, no discs or disc images - plus
//! stacks, which are several files behind one path.
bool CanMeasureContentGeometry(const CFileItem& item);

/*!
 * \brief Sample \p item and produce the row to store for it.
 *
 * \param identity the file as it is now, established by the caller so deciding the work is
 *                 needed and recording what it was done against use the same one
 * \param cancelled asked between sample points; see CDVDFileGeometry::ExtractContentGeometry
 * \return the row to store, or nothing if sampling was abandoned. A file that could not be read
 *         is a Failed record rather than nothing.
 */
std::optional<ContentGeometryRecord> MeasureContentGeometry(
    const CFileItem& item,
    const FileIdentity& identity,
    SamplingDepth depth = SamplingDepth::Normal,
    const std::function<bool()>& cancelled = {});

/*!
 * \brief Measure the file \p item plays, unless something already has, and store it.
 *
 * Blocking: it runs on the way into playback so the rectangle is known before the first frame.
 * Returns at once when there is nothing to do, which includes every file the user has declared a
 * ratio for, a declaration outranking detection.
 */
void MeasureContentGeometryBeforePlayback(const CFileItem& item,
                                          const std::function<bool()>& cancelled);

//! \brief MeasureContentGeometryBeforePlayback() under the busy dialog, for the thread that
//! opens files. Video items only; backing out abandons the measurement and starts the film.
void MeasureContentGeometryBeforePlaybackBlocking(const CFileItem& item);

/*!
 * \brief Measure \p item again and replace whatever is stored for it, without asking whether the
 *        work is needed - a file measured successfully with the wrong answer never becomes a
 *        candidate for any other path.
 *
 * Measures even when a ratio has been declared, refreshing what detection would have said
 * without changing what is served.
 *
 * \return false when the item cannot be measured at all - not local, a stack, a disc, or not in
 *         the library
 */
bool RemeasureContentGeometry(const CFileItem& item,
                              SamplingDepth depth = SamplingDepth::Normal,
                              const std::function<bool()>& cancelled = {});

//! \brief Coordinates the background sweep over everything that still needs measuring.
class CContentGeometryScanner : public IJobCallback
{
public:
  static CContentGeometryScanner& GetInstance();

  /*!
   * \brief Run the background sweep over everything that still needs measuring. Does nothing if
   *        a sweep is already running or the feature is off.
   *
   * Not on the video library queue, which is single-job and would keep a library scan waiting
   * for hours; it suspends itself while any other video library job runs instead.
   *
   * \param retryFailed also take another look at files a previous run could not read
   */
  void Sweep(bool retryFailed = false);

  bool IsSweeping() const { return m_sweeping; }

  //! \brief Ask a running sweep to stop, abandoning the file it is measuring.
  void StopSweep() { m_stop = true; }

  //! \brief Read by the running sweep between files and between sample points.
  bool IsStopRequested() const { return m_stop; }

  // implementation of IJobCallback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;
  void OnJobAbort(unsigned int jobID, CJob* job) override;

private:
  CContentGeometryScanner() = default;
  CContentGeometryScanner(const CContentGeometryScanner&) = delete;
  CContentGeometryScanner& operator=(const CContentGeometryScanner&) = delete;

  //! \brief Flags rather than a pointer to the job: the job manager takes ownership and may
  //! destroy what it was handed before the completion callback arrives.
  std::atomic<bool> m_sweeping{false};
  std::atomic<bool> m_stop{false};
};

} // namespace KODI::VIDEO::GEOMETRY
