/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayerCallback.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationStackHelper.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/python/XBPython.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <memory>

using namespace KODI;

CApplicationPlayerCallback::CApplicationPlayerCallback()
{
}

void CApplicationPlayerCallback::OnPlayBackEnded()
{
  CLog::LogF(LOGDEBUG, "CApplicationPlayerCallback::OnPlayBackEnded");

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "CApplication::OnPlayBackStarted");
  std::shared_ptr<CFileItem> itemCurrentFile;

  // check if VideoPlayer should set file item stream details from its current streams
  const bool isBlu_dvd_image_or_stream = URIUtils::IsBlurayPath(file.GetPath()) ||
                                         VIDEO::IsDVDFile(file) || file.IsDiscImage() ||
                                         NETWORK::IsInternetStream(file);

  const bool hasNoStreamDetails =
      (!file.HasVideoInfoTag() || !file.GetVideoInfoTag()->HasStreamDetails());

  if (file.GetProperty("get_stream_details_from_player").asBoolean() ||
      (hasNoStreamDetails && isBlu_dvd_image_or_stream))
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetUpdateStreamDetails();
  }

  auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingDiscStack() || stackHelper->IsPlayingRegularStack())
    itemCurrentFile = std::make_shared<CFileItem>(*stackHelper->GetRegisteredStack(file));
  else
    itemCurrentFile = std::make_shared<CFileItem>(file);

  /* When playing video pause any low priority jobs, they will be unpaused  when playback stops.
   * This should speed up player startup for files on internet filesystems (eg. webdav) and
   * increase performance on low powered systems (Atom/ARM).
   */
  if (VIDEO::IsVideo(file) || file.IsGame())
  {
    CServiceBroker::GetJobManager()->PauseJobs();
  }

  stackHelper->OnPlayBackStarted(file);

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, itemCurrentFile);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayerCloseFile(const CFileItem& file,
                                                   const CBookmark& bookmarkParam)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();

  std::unique_lock<CCriticalSection> lock(stackHelper->m_critSection);

  CBookmark bookmark = bookmarkParam;

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmark.timeInSeconds == 0.0)
    return;

  CFileItem fileItem{file};
  CBookmark resumeBookmark;
  bool playCountUpdate = false;
  float percent = 0.0f;
  bool stackAllPartsPlayed{true};
  int64_t totalTime{file.GetProperty("longest_stream_duration").asInteger()};
  const std::string longestPath{file.GetProperty("longest_stream_path").asString()};

  // If part of stack then pressing stop could be the only way out (ie. movie ends and return to menu)
  // If a BD is played from the simple menu/play main movie then DynPath() is a bluray://...mpls
  // (which doesn't use the disc menu therefore assume stop means stop)
  // If played through BD/DVD menu and stopped in menu then bookmark is for menu not the watched episode/movie
  const bool inMenu{file.GetProperty("stopped_in_menu").asBoolean()};
  const bool endedInMenu{inMenu && (file.GetProperty("longest_stream_duration").asDouble() /
                                    1000.0) > bookmark.totalTimeInSeconds};

  if (stackHelper->GetRegisteredStack(fileItem) != nullptr)
  {
    fileItem = *stackHelper->GetRegisteredStack(file);
    bookmark.partNumber = stackHelper->GetRegisteredStackPartNumber(file);
    fileItem.SetProperty("original_path", file.GetPath());

    // Build stacktimes for disc image stacks after each disc image is played
    // unless in DVD menu before playing movie
    if (stackHelper->WasPlayingDiscStack() && !(inMenu && !endedInMenu))
    {
      CVideoDatabase dbs;
      if (dbs.Open())
      {
        // Get existing stacktimes (if any)
        std::vector<uint64_t> times;
        bool haveTimes{dbs.GetStackTimes(fileItem.GetDynPath(), times)};
        if (stackHelper->GetKnownStackParts() - 1 == static_cast<long>(times.size()))
        {
          // Update filename
          std::string stackedPath;
          if (URIUtils::IsBlurayPath(file.GetDynPath()) || URIUtils::IsDVDPath(file.GetDynPath()))
          {
            std::vector<std::string> paths{};
            XFILE::CStackDirectory::GetPaths(fileItem.GetDynPath(), paths);
            paths[bookmark.partNumber] = longestPath;
            XFILE::CStackDirectory::ConstructStackPath(paths, stackedPath);
            fileItem.SetProperty("new_stack_path", stackedPath);

            // fileItem will have the original stack:// path in DynPath
            // So update all CFileItems in the stack
            stackHelper->SetRegisteredStackDynPaths(stackedPath);
          }
          else
            stackedPath = fileItem.GetDynPath();

          uint64_t time{0};
          if (haveTimes)
            time = times.back(); // Times are cumulative

          // Add this part's time to end of stacktimes
          totalTime += time;
          times.emplace_back(totalTime);
          dbs.SetStackTimes(stackedPath, times);

          // Update stack (for bookmark and % played)
          stackHelper->SetRegisteredStackPartStartTimeMs(file, time);
          stackHelper->SetRegisteredStackTotalTimeMs(file, totalTime);

          // If adding part then update streamdetails as well
          fileItem.GetVideoInfoTag()->m_streamDetails = file.GetVideoInfoTag()->m_streamDetails;

          // Also update video info tag with total time of the stack (as this is read for the library display)
          fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(0, totalTime / 1000.0);
        }
        else
        {
          // Still need to update video duration for total stack time
          fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
              0, stackHelper->GetRegisteredStackTotalTimeMs(file) / 1000.0);
        }
        dbs.Close();
      }
    }

    if (stackHelper->GetRegisteredStackTotalTimeMs(file) > 0)
    {
      // the bookmark coming from the player is only relative to the current part, thus needs to be corrected with these attributes
      // (start time will be 0 for non-stackparts)
      bookmark.timeInSeconds += stackHelper->GetRegisteredStackPartStartTimeMs(file) / 1000.0;
      bookmark.totalTimeInSeconds = stackHelper->GetRegisteredStackTotalTimeMs(file) / 1000.0;
    }

    stackHelper->SetStackPartStopped(inMenu);

    // Only show a stack as played if all parts have been played
    stackAllPartsPlayed =
        (stackHelper->WasPlayingDiscStack() &&
         stackHelper->GetCurrentPartNumber() + 1 == stackHelper->GetTotalPartNumbers()) ||
        (!stackHelper->WasPlayingDiscStack() && stackHelper->IsPlayingRegularStack());
  }
  else
    fileItem.SetDynPath(longestPath);

  percent = bookmark.timeInSeconds / bookmark.totalTimeInSeconds * 100;

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if ((MUSIC::IsAudio(fileItem) && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_audioPlayCountMinimumPercent) ||
      (VIDEO::IsVideo(fileItem) &&
       ((advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
         percent >= advancedSettings->m_videoPlayCountMinimumPercent) ||
        (file.HasProperty("chapter_finished") &&
         file.GetProperty("chapter_finished").asBoolean())) &&
       stackAllPartsPlayed) ||
      (endedInMenu && stackAllPartsPlayed))
  {
    // In case of disc stacks only increase play count when all parts have been played
    playCountUpdate = true;
  }

  if (inMenu && !(endedInMenu && stackAllPartsPlayed))
  {
    if (endedInMenu)
    {
      // Ended in menu
      resumeBookmark.timeInSeconds = totalTime / 1000.0;
      resumeBookmark.playerState =
          StringUtils::Format("<nextpart>{}</nextpart>", bookmark.partNumber + 1);
    }
    else
    {
      if (stackHelper->GetCurrentPartNumber() > 0)
      {
        // Not played movie of this part yet
        resumeBookmark.timeInSeconds = stackHelper->GetStackTotalTimeMs() / 1000.0;
        resumeBookmark.playerState =
            StringUtils::Format("<nextpart>{}</nextpart>", bookmark.partNumber);
      }
      else
      {
        // Not played anything yet
        resumeBookmark.timeInSeconds = 0;
        fileItem.GetVideoInfoTag()->m_streamDetails.Reset(); // Don't save menu streamdetails
      }
    }
    resumeBookmark.totalTimeInSeconds = stackHelper->GetStackTotalTimeMs() / 1000.0;
  }
  else if (file.HasProperty("chapter_finished") && file.GetProperty("chapter_finished").asBoolean())
  {
    resumeBookmark.timeInSeconds = 0.0;
  }
  else if ((advancedSettings->m_videoIgnorePercentAtEnd > 0 &&
            bookmark.totalTimeInSeconds - bookmark.timeInSeconds <
                0.01 * static_cast<double>(advancedSettings->m_videoIgnorePercentAtEnd) *
                    bookmark.totalTimeInSeconds) ||
           inMenu)
  {
    resumeBookmark.timeInSeconds = -1.0;
  }
  else if (bookmark.timeInSeconds > 0.0 &&
           bookmark.timeInSeconds > advancedSettings->m_videoIgnoreSecondsAtStart)
  {
    resumeBookmark = bookmark;
  }
  else
  {
    resumeBookmark.timeInSeconds = 0.0;
  }

  // If DVD chapter then update stream duration from entire title to chapter
  if (fileItem.IsDVDChapter())
    if (file.HasProperty("chapter_duration"))
      fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
          0, file.GetProperty("chapter_duration").asUnsignedInteger());

  if (CServiceBroker::GetSettingsComponent()
          ->GetProfileManager()
          ->GetCurrentProfile()
          .canWriteDatabases())
  {
    CSaveFileState::DoWork(fileItem, resumeBookmark, playCountUpdate);
  }
}

void CApplicationPlayerCallback::OnPlayBackPaused()
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackPaused();
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_PAUSED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackResumed();
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_RESUMED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStopped()
{
  // If playing a DVD/Bluray stack and using a menu then stop can be the only way
  // to exit. Therefore, if part way through a stack we may want to play the next
  // stacked item.
  // Already flagged above if this may be the case (based on media played)
  const auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();
  if (stackHelper->IsPlayingDiscStack() && stackHelper->HasNextStackPartFileItem() &&
      stackHelper->GetStackPartStopped())
  {
    // Dialog box to ask continue/stop later
    OnPlayBackEnded();
    return;
  }

  CLog::LogF(LOGDEBUG, "CApplication::OnPlayBackStopped");

  CGUIMessage msg(GUI_MSG_PLAYBACK_STOPPED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackError()
{
  //@todo Playlists can be continued by calling OnPlaybackEnded instead
  // open error dialog
  CGUIMessage msg(GUI_MSG_PLAYBACK_ERROR, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  OnPlayBackStopped();
}

void CApplicationPlayerCallback::OnQueueNextItem()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnQueueNextItem");

  // informs python script currently running that we are requesting the next track
  // (does nothing if python is not loaded)
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnQueueNextItem(); // currently unimplemented
#endif

  CGUIMessage msg(GUI_MSG_QUEUE_NEXT_ITEM, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackSeek(int64_t iTime, int64_t seekOffset)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSeek(static_cast<int>(iTime),
                                               static_cast<int>(seekOffset));
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_SEEKED, 0, 0, iTime, seekOffset);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackSeekChapter(int iChapter)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSeekChapter(iChapter);
#endif
}

void CApplicationPlayerCallback::OnPlayBackSpeedChanged(int iSpeed)
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackSpeedChanged(iSpeed);
#endif

  CGUIMessage msg(GUI_MSG_PLAYBACK_SPEED_CHANGED, 0, 0, iSpeed);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnAVChange()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVChange");

  CServiceBroker::GetGUI()->GetStereoscopicsManager().OnStreamChange();

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVCHANGE, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnAVStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVStarted");

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVSTARTED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::RequestVideoSettings(const CFileItem& fileItem)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    CLog::Log(LOGDEBUG, "Loading settings for {}", CURL::GetRedacted(fileItem.GetPath()));

    // Load stored settings if they exist, otherwise use default
    CVideoSettings vs;
    if (!dbs.GetVideoSettings(fileItem, vs))
      vs = CMediaSettings::GetInstance().GetDefaultVideoSettings();

    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetVideoSettings(vs);

    dbs.Close();
  }
}

void CApplicationPlayerCallback::StoreVideoSettings(const CFileItem& fileItem,
                                                    const CVideoSettings& vs)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
    if (vs != CMediaSettings::GetInstance().GetDefaultVideoSettings())
    {
      dbs.SetVideoSettings(fileItem, vs);
    }
    else
    {
      dbs.EraseVideoSettings(fileItem);
    }
    dbs.Close();
  }
}
