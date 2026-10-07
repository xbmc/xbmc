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
#include <optional>
#include <vector>

using namespace KODI::VIDEO::GEOMETRY;
using namespace std::chrono_literals;

namespace
{

//! \brief How often to look again while suspended.
constexpr auto IDLE_POLL_INTERVAL{1s};

//! \brief Whether something else is using the disk, the network or the database. Any playback
//! counts: the file being measured is usually on the same share.
bool IsBusy()
{
  if (CVideoLibraryQueue::GetInstance().IsRunning())
    return true;

  if (CContentGeometryScanner::GetInstance().IsOpeningForPlayback())
    return true;

  return CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()->IsPlaying();
}

bool StopRequested()
{
  return CContentGeometryScanner::GetInstance().IsStopRequested();
}

//! \brief Block until nothing is playing and no other library job is running.
//! \return false if the sweep was stopped while waiting
bool WaitUntilIdle()
{
  while (!StopRequested() && IsBusy())
    KODI::TIME::Sleep(IDLE_POLL_INTERVAL);

  return !StopRequested();
}

} // unnamed namespace

CVideoLibraryContentGeometryJob::CVideoLibraryContentGeometryJob(bool retryFailed)
  : CVideoLibraryProgressJob(nullptr),
    m_retryFailed(retryFailed)
{
}

CVideoLibraryContentGeometryJob::~CVideoLibraryContentGeometryJob() = default;

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
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(40815)));
  }

  unsigned int measured{0};
  unsigned int failed{0};

  // The index only advances once a file is finished with; an abandoned file is measured again.
  for (size_t index = 0; index < candidates.size();)
  {
    if (!WaitUntilIdle())
      break;

    const ContentGeometryCandidate& candidate{candidates[index]};
    SetProgress(static_cast<int>(index), static_cast<int>(candidates.size()));

    const CFileItem item{candidate.path, false};
    const std::optional<FileIdentity> identity{MeasurableIdentity(item)};
    if (!identity)
    {
      ++index;
      continue;
    }

    const bool retryThisOne{m_retryFailed && candidate.stored && !candidate.stored->HasReading()};
    if (!retryThisOne && !NeedsContentGeometry(candidate.stored, *identity))
    {
      ++index;
      continue;
    }

    SetText(URIUtils::GetFileName(candidate.path));

    const std::optional<ContentGeometryRecord> record{MeasureContentGeometry(
        item, *identity, SamplingDepth::Normal, []() { return StopRequested() || IsBusy(); })};
    if (!record)
      continue; // abandoned, not finished - take this file again once whatever interrupted it stops

    if (record->HasReading())
      ++measured;
    else
      ++failed;

    db.SetContentGeometry(candidate.idFile, *record);
    ++index;
  }

  CLog::LogF(LOGINFO, "measured {} of {} files, {} with no reading{}", measured, candidates.size(),
             failed, StopRequested() ? ", stopped" : "");

  return true;
}
