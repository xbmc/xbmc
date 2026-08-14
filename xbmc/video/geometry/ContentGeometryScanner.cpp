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
#include "XBDateTime.h"
#include "cores/VideoPlayer/DVDFileGeometry.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
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

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief A file the gates passed and the library row a measurement of it stores into.
struct StorageTarget
{
  FileIdentity identity;
  int idFile{-1};
};

} // unnamed namespace

CContentGeometryScanner& CContentGeometryScanner::GetInstance()
{
  static CContentGeometryScanner instance;
  return instance;
}

bool CanMeasureContentGeometry(const CFileItem& item)
{
  if (!CDVDFileInfo::CanExtract(item))
    return false;

  // One path standing for several files. The sampler seeks by time into a single stream and has
  // no notion of which part of a stack it is in.
  return !item.IsStack() && !URIUtils::IsStack(item.GetDynPath());
}

std::optional<ContentGeometryRecord> MeasureContentGeometry(const CFileItem& item,
                                                            const FileIdentity& identity,
                                                            SamplingDepth depth,
                                                            const std::function<bool()>& cancelled)
{
  const SampledGeometry scan{
      CDVDFileGeometry::ExtractContentGeometry(item, ContentGeometrySamplingFromSettings(depth),
                                               ContentGeometryCombiningFromSettings(), cancelled)};
  if (scan.cancelled)
    return std::nullopt;

  const ContentGeometryRecord record{
      MakeContentGeometryRecord(scan, identity, CDateTime::GetCurrentDateTime())};

  // What was stored, which is not always what was measured: a scan that read nothing still
  // produces a row, and telling that apart from a successful one afterwards otherwise means
  // decoding the retained detail by hand.
  CLog::LogF(LOGDEBUG, "storing content geometry for {}: {}", CURL::GetRedacted(item.GetDynPath()),
             record.outcome == ContentGeometryOutcome::Failed ? "could not be read"
             : record.hasReading                              ? "measured"
                                                              : "no reading, frame stands");

  return record;
}

namespace
{

/*!
 * \brief The gates a measurement passes before it is worth taking: enabled, measurable, a
 * known identity and a library row to store into.
 * \param db opened here, and left open for the caller's own gates and the store
 * \return nothing when any gate fails
 */
std::optional<StorageTarget> ResolveStorageTarget(const CFileItem& item, CVideoDatabase& db)
{
  if (!ContentGeometryEnabledFromSettings() || !CanMeasureContentGeometry(item))
    return std::nullopt;

  const FileIdentity identity{GetFileIdentity(item.GetDynPath())};
  if (!identity.IsKnown())
    return std::nullopt;

  if (!db.Open())
    return std::nullopt;

  // Resolved once and reused: the item is otherwise looked up twice, and one outside the
  // library has nowhere to store a measurement and would be sampled on every play.
  const int idFile{db.GetPlayedFileId(item)};
  if (idFile < 0)
    return std::nullopt;

  return StorageTarget{identity, idFile};
}

} // unnamed namespace

void MeasureContentGeometryBeforePlayback(const CFileItem& item,
                                          const std::function<bool()>& cancelled)
{
  CVideoDatabase db;
  const std::optional<StorageTarget> target{ResolveStorageTarget(item, db)};
  if (!target)
    return;

  // A declared ratio outranks every measurement, so measuring here would hold up playback to
  // produce a value that resolution then ignores.
  CVideoSettings settings;
  if (db.GetVideoSettings(item, settings) && settings.m_declaredAspect > 0.0f)
    return;

  if (!NeedsContentGeometry(db.GetContentGeometryAttempt(target->idFile), target->identity))
    return;

  CLog::LogF(LOGDEBUG, "measuring content geometry before playback of {}",
             CURL::GetRedacted(item.GetDynPath()));

  const std::optional<ContentGeometryRecord> record{
      MeasureContentGeometry(item, target->identity, SamplingDepth::Normal, cancelled)};
  if (record)
    db.SetContentGeometry(target->idFile, *record);
}

namespace
{

//! \brief Hosts MeasureContentGeometryBeforePlayback() under the busy dialog. Blocking the thread
//! that opens a file blocks the one that draws, so the user can back out; the sweep takes it.
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
  if (!ContentGeometryEnabledFromSettings() || !IsVideo(item))
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
  if (!ContentGeometryEnabledFromSettings())
    return;

  bool idle{false};
  if (!m_sweeping.compare_exchange_strong(idle, true))
    return;

  m_stop = false;

  // Handed over as CVideoLibraryJob: CVideoLibraryProgressJob reaches CJob down two
  // inheritance paths, so the conversion is only unambiguous through one of them.
  CVideoLibraryJob* job{new CVideoLibraryContentGeometryJob(retryFailed)};
  if (CServiceBroker::GetJobManager()->AddJob(job, this, CJob::PRIORITY_LOW) == 0)
  {
    // Refused, and already destroyed by the manager, which only happens while it is shutting
    // down. No completion callback will arrive to clear this.
    m_sweeping = false;
  }
}

void CContentGeometryScanner::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  m_sweeping = false;
}

void CContentGeometryScanner::OnJobAbort(unsigned int jobID, CJob* job)
{
  m_sweeping = false;
}

} // namespace KODI::VIDEO::GEOMETRY
