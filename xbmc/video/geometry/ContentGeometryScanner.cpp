/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentGeometryScanner.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/VideoPlayer/VideoFileGeometry.h"
#include "cores/VideoSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "jobs/JobManager.h"
#include "threads/IRunnable.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/geometry/GeometrySettings.h"
#include "video/geometry/SampledGeometry.h"
#include "video/jobs/VideoLibraryContentGeometryJob.h"
#include "video/jobs/VideoLibraryJob.h"

#include <mutex>
#include <utility>

namespace KODI::VIDEO::GEOMETRY
{

CContentGeometryScanner& CContentGeometryScanner::GetInstance()
{
  static CContentGeometryScanner instance;
  return instance;
}

std::optional<FileIdentity> MeasurableIdentity(const CFileItem& item)
{
  if (!CVideoFileGeometry::CanMeasure(item) || item.IsStack() ||
      URIUtils::IsStack(item.GetDynPath()))
    return std::nullopt;

  const FileIdentity identity{GetFileIdentity(item.GetDynPath())};
  if (!identity.IsKnown())
    return std::nullopt;

  return identity;
}

std::optional<ContentGeometryRecord> MeasureContentGeometry(const CFileItem& item,
                                                            const FileIdentity& identity,
                                                            SamplingDepth depth,
                                                            const std::function<bool()>& cancelled)
{
  const SampledGeometry scan{CVideoFileGeometry::ExtractContentGeometry(
      item, ContentGeometrySamplingFromSettings(depth), ContentGeometryCombiningFromSettings(),
      cancelled)};
  if (scan.cancelled)
    return std::nullopt;

  const ContentGeometryRecord record{MakeContentGeometryRecord(scan, identity)};

  // A scan that read nothing still produces a record, so the file is not measured again.
  CLog::LogF(LOGDEBUG, "measured content geometry of {}: {}", CURL::GetRedacted(item.GetDynPath()),
             record.HasReading() ? EncodeContentAspects(record.aspects) : "no reading");

  return record;
}

namespace
{

//! \brief A file the gates passed and the library row a measurement of it stores into.
struct StorageTarget
{
  FileIdentity identity;
  int idFile{-1};
};

/*!
 * \brief The gates a measurement passes before it is worth taking: enabled, measurable, a
 * known identity and a library row to store into.
 * \param db opened here, and left open for the caller's own gates and the store
 * \return nothing when any gate fails
 */
std::optional<StorageTarget> ResolveStorageTarget(const CFileItem& item, CVideoDatabase& db)
{
  if (!ContentGeometryEnabledFromSettings())
    return std::nullopt;

  const std::optional<FileIdentity> identity{MeasurableIdentity(item)};
  if (!identity || !db.Open())
    return std::nullopt;

  // An item outside the library has nowhere to store a measurement.
  const int idFile{db.GetFileId(item)};
  if (idFile < 0)
    return std::nullopt;

  return StorageTarget{*identity, idFile};
}

void MeasureContentGeometryBeforePlayback(const CFileItem& item,
                                          const std::function<bool()>& cancelled)
{
  CVideoDatabase db;
  const std::optional<StorageTarget> target{ResolveStorageTarget(item, db)};
  if (!target)
    return;

  CVideoSettings settings;
  if (db.GetVideoSettings(item, settings) && settings.m_declaredAspect > 0.0f)
    return;

  if (!NeedsContentGeometry(db.GetStoredContentGeometry(target->idFile), target->identity))
    return;

  CLog::LogF(LOGDEBUG, "measuring content geometry before playback of {}",
             CURL::GetRedacted(item.GetDynPath()));

  const std::optional<ContentGeometryRecord> record{
      MeasureContentGeometry(item, target->identity, SamplingDepth::Normal, cancelled)};
  if (record)
    db.SetContentGeometry(target->idFile, *record);
}

//! \brief Hosts MeasureContentGeometryBeforePlayback() under the busy dialog, so the user can
//! back out.
class CContentGeometryPlaybackRunnable : public IRunnable
{
public:
  explicit CContentGeometryPlaybackRunnable(const CFileItem& item) : m_item(item) {}

  void Run() override
  {
    MeasureContentGeometryBeforePlayback(m_item, [this]() { return m_cancelled.load(); });
  }

  void Cancel() override { m_cancelled = true; }

private:
  const CFileItem& m_item;
  std::atomic<bool> m_cancelled{false};
};

} // unnamed namespace

void MeasureContentGeometryBeforePlaybackBlocking(const CFileItem& item)
{
  if (!ContentGeometryNonLiveFromSettings() || !IsVideo(item))
    return;

  CContentGeometryPlaybackRunnable measure{item};
  CGUIDialogBusy::Wait(&measure, 500, true);
}

bool RemeasureContentGeometry(const CFileItem& item,
                              SamplingDepth depth,
                              const std::function<bool()>& cancelled)
{
  CVideoDatabase db;
  const std::optional<StorageTarget> target{ResolveStorageTarget(item, db)};
  if (!target)
    return false;

  CLog::LogF(LOGDEBUG, "remeasuring content geometry of {}", CURL::GetRedacted(item.GetDynPath()));

  const std::optional<ContentGeometryRecord> record{
      MeasureContentGeometry(item, target->identity, depth, cancelled)};
  if (!record)
    return false; // abandoned, so the row that was there is left alone

  db.SetContentGeometry(target->idFile, *record);
  return true;
}

void CContentGeometryScanner::Sweep(bool retryFailed /* = false */)
{
  if (!ContentGeometryNonLiveFromSettings())
    return;

  {
    std::unique_lock lock(m_lock);
    if (m_sweeping)
    {
      m_held = m_held.value_or(false) || retryFailed;
      return;
    }

    m_sweeping = true;
    m_stop = false;
  }

  Start(retryFailed);
}

bool CContentGeometryScanner::IsSweeping() const
{
  std::unique_lock lock(m_lock);
  return m_sweeping;
}

void CContentGeometryScanner::StopSweep()
{
  std::unique_lock lock(m_lock);
  m_held.reset();
  m_stop = true;
}

void CContentGeometryScanner::Start(bool retryFailed)
{
  // Not under m_lock: the job manager calls back into Ended() while holding its own lock.
  // CVideoLibraryProgressJob reaches CJob down two paths; only CVideoLibraryJob converts
  // unambiguously.
  CVideoLibraryJob* job{new CVideoLibraryContentGeometryJob(retryFailed)};
  if (CServiceBroker::GetJobManager()->AddJob(job, this, CJob::PRIORITY_LOW_PAUSABLE) == 0)
  {
    // Refused and already destroyed by the manager, so no callback will arrive.
    std::unique_lock lock(m_lock);
    m_sweeping = false;
    m_held.reset();
  }
}

void CContentGeometryScanner::Ended()
{
  std::optional<bool> held;
  {
    std::unique_lock lock(m_lock);
    held = std::exchange(m_held, std::nullopt);
    m_sweeping = held.has_value();
    if (held)
      m_stop = false;
  }

  if (held)
    Start(*held);
}

void CContentGeometryScanner::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  Ended();
}

void CContentGeometryScanner::OnJobAbort(unsigned int jobID, CJob* job)
{
  Ended();
}

} // namespace KODI::VIDEO::GEOMETRY
