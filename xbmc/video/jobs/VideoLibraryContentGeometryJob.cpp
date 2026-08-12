/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryContentGeometryJob.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"
#include "video/geometry/ContentGeometryScanner.h"
#include "video/geometry/GeometrySettings.h"

#include <chrono>
#include <cstring>
#include <vector>

using namespace KODI::VIDEO::GEOMETRY;
using namespace std::chrono_literals;

namespace
{

//! \brief How often to look again while suspended.
constexpr auto IDLE_POLL_INTERVAL{1s};

/*!
 * \brief Is something else using the disk, the network or the database right now?
 *
 * Any playback counts, not only video: the file being measured is usually on the same share the
 * picture or the music is streaming from.
 */
bool IsBusy()
{
  if (CVideoLibraryQueue::GetInstance().IsRunning())
    return true;

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  return appPlayer && appPlayer->IsPlaying();
}

} // unnamed namespace

CVideoLibraryContentGeometryJob::CVideoLibraryContentGeometryJob(bool retryFailed)
  : CVideoLibraryProgressJob(nullptr),
    m_retryFailed(retryFailed)
{
}

CVideoLibraryContentGeometryJob::~CVideoLibraryContentGeometryJob() = default;

bool CVideoLibraryContentGeometryJob::Cancel()
{
  m_cancelled = true;
  return true;
}

bool CVideoLibraryContentGeometryJob::Equals(const CJob* job) const
{
  if (std::strcmp(job->GetType(), GetType()) != 0)
    return false;

  const auto* other = dynamic_cast<const CVideoLibraryContentGeometryJob*>(job);
  return other != nullptr && m_retryFailed == other->m_retryFailed;
}

bool CVideoLibraryContentGeometryJob::IsCancelled() const
{
  // Two sources: the job manager owns the job and may destroy it without this being told, so
  // the flag a caller sets cannot live here alone.
  return m_cancelled || CContentGeometryScanner::GetInstance().IsStopRequested();
}

bool CVideoLibraryContentGeometryJob::ShouldYield() const
{
  return IsCancelled() || IsBusy();
}

bool CVideoLibraryContentGeometryJob::WaitUntilIdle() const
{
  while (!IsCancelled() && IsBusy())
    KODI::TIME::Sleep(IDLE_POLL_INTERVAL);

  return !IsCancelled();
}

bool CVideoLibraryContentGeometryJob::Work(CVideoDatabase& db)
{
  if (!ContentGeometryEnabledFromSettings())
    return true;

  const std::vector<ContentGeometryCandidate> candidates{db.GetContentGeometryCandidates()};
  if (candidates.empty())
    return true;

  if (CGUIComponent* gui = CServiceBroker::GetGUI(); gui && !HasProgressIndicator())
  {
    if (auto* dialog = gui->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(
            WINDOW_DIALOG_EXT_PROGRESS))
      SetProgressBar(dialog->GetHandle(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(40814)));
  }

  unsigned int measured{0};
  unsigned int failed{0};

  // Deliberately not a range-for: a file abandoned because playback started is measured again
  // rather than skipped, so the index only advances once a file is finished with.
  for (size_t index = 0; index < candidates.size();)
  {
    if (!WaitUntilIdle())
      break;

    const ContentGeometryCandidate& candidate{candidates[index]};
    SetProgress(static_cast<int>(index), static_cast<int>(candidates.size()));

    const CFileItem item{candidate.path, false};
    if (!CanMeasureContentGeometry(item))
    {
      ++index;
      continue;
    }

    // A file that cannot be stat'd would carry an identity that never verifies, so its record
    // would be served to nobody and remeasured on every run.
    const FileIdentity identity{GetFileIdentity(candidate.path)};
    if (!identity.IsKnown())
    {
      ++index;
      continue;
    }

    const bool retryThisOne{m_retryFailed && candidate.attempt.exists &&
                            candidate.attempt.outcome == ContentGeometryOutcome::Failed};
    if (!retryThisOne && !NeedsContentGeometry(candidate.attempt, identity))
    {
      ++index;
      continue;
    }

    SetText(URIUtils::GetFileName(candidate.path));

    const std::optional<ContentGeometryRecord> record{MeasureContentGeometry(
        item, identity, SamplingDepth::Normal, [this]() { return ShouldYield(); })};
    if (!record)
      continue; // abandoned, not finished - take this file again once whatever interrupted it stops

    if (record->outcome == ContentGeometryOutcome::Failed)
      ++failed;
    else
      ++measured;

    db.SetContentGeometry(candidate.idFile, *record);
    ++index;
  }

  CLog::LogF(LOGINFO, "measured {} of {} files, {} could not be read{}", measured,
             candidates.size(), failed, IsCancelled() ? ", cancelled" : "");

  return true;
}
