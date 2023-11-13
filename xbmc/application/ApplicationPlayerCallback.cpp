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
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationStackHelper.h"
#include "cores/DataCacheCore.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/json-rpc/JSONUtils.h"
#include "interfaces/python/XBPython.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/SaveFileStateJob.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <memory>

CApplicationPlayerCallback::CApplicationPlayerCallback()
  : m_itemCurrentFile(new CFileItem), m_playerEvent(true, true)
{
}

void CApplicationPlayerCallback::OnPlayBackEnded()
{
  CLog::LogF(LOGDEBUG, "CApplicationPlayerCallback::OnPlayBackEnded");

  CServiceBroker::GetPVRManager().OnPlaybackEnded(*m_itemCurrentFile);

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                     m_itemCurrentFile, data);

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "CApplication::OnPlayBackStarted");

  // check if VideoPlayer should set file item stream details from its current streams
  const bool isBlu_dvd_image_or_stream = (URIUtils::IsBluray(file.GetPath()) || file.IsDVDFile() ||
                                          file.IsDiscImage() || file.IsInternetStream());

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
  if (stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack())
    m_itemCurrentFile = std::make_shared<CFileItem>(*stackHelper->GetRegisteredStack(file));
  else
    m_itemCurrentFile = std::make_shared<CFileItem>(file);

  /* When playing video pause any low priority jobs, they will be unpaused  when playback stops.
   * This should speed up player startup for files on internet filesystems (eg. webdav) and
   * increase performance on low powered systems (Atom/ARM).
   */
  if (file.IsVideo() || file.IsGame())
  {
    CServiceBroker::GetJobManager()->PauseJobs();
  }

  CServiceBroker::GetPVRManager().OnPlaybackStarted(*m_itemCurrentFile);
  stackHelper->OnPlayBackStarted(file);

  m_playerEvent.Reset();

  CGUIMessage msg(GUI_MSG_PLAYBACK_STARTED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayerCloseFile(const CFileItem& file,
                                                   const CBookmark& bookmarkParam)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();

  std::unique_lock<CCriticalSection> lock(stackHelper->m_critSection);

  CFileItem fileItem(file);
  CBookmark bookmark = bookmarkParam;
  CBookmark resumeBookmark;
  bool playCountUpdate = false;
  float percent = 0.0f;

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmark.timeInSeconds == 0.0)
    return;

  if (stackHelper->GetRegisteredStack(fileItem) != nullptr &&
      stackHelper->GetRegisteredStackTotalTimeMs(fileItem) > 0)
  {
    // regular stack case: we have to save the bookmark on the stack
    fileItem = *stackHelper->GetRegisteredStack(file);
    // the bookmark coming from the player is only relative to the current part, thus needs to be corrected with these attributes (start time will be 0 for non-stackparts)
    bookmark.timeInSeconds += stackHelper->GetRegisteredStackPartStartTimeMs(file) / 1000.0;
    if (stackHelper->GetRegisteredStackTotalTimeMs(file) > 0)
      bookmark.totalTimeInSeconds = stackHelper->GetRegisteredStackTotalTimeMs(file) / 1000.0;
    bookmark.partNumber = stackHelper->GetRegisteredStackPartNumber(file);
  }

  percent = bookmark.timeInSeconds / bookmark.totalTimeInSeconds * 100;

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if ((fileItem.IsAudio() && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_audioPlayCountMinimumPercent) ||
      (fileItem.IsVideo() && advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_videoPlayCountMinimumPercent))
  {
    playCountUpdate = true;
  }

  if (advancedSettings->m_videoIgnorePercentAtEnd > 0 &&
      bookmark.totalTimeInSeconds - bookmark.timeInSeconds <
          0.01 * static_cast<double>(advancedSettings->m_videoIgnorePercentAtEnd) *
              bookmark.totalTimeInSeconds)
  {
    resumeBookmark.timeInSeconds = -1.0;
  }
  else if (bookmark.timeInSeconds > advancedSettings->m_videoIgnoreSecondsAtStart)
  {
    resumeBookmark = bookmark;
    if (stackHelper->GetRegisteredStack(file) != nullptr)
    {
      // also update video info tag with total time
      fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
          0, resumeBookmark.totalTimeInSeconds);
    }
  }
  else
  {
    resumeBookmark.timeInSeconds = 0.0;
  }

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

  CVariant param;
  param["player"]["speed"] = 0;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPause",
                                                     m_itemCurrentFile, param);
}

void CApplicationPlayerCallback::OnPlayBackResumed()
{
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnPlayBackResumed();
#endif

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnResume",
                                                     m_itemCurrentFile, param);
}

void CApplicationPlayerCallback::OnPlayBackStopped()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnPlayBackStopped");

  CServiceBroker::GetPVRManager().OnPlaybackStopped(*m_itemCurrentFile);

  CVariant data(CVariant::VariantTypeObject);
  data["end"] = false;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                     m_itemCurrentFile, data);

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

  CVariant param;
  JSONRPC::CJSONUtils::MillisecondsToTimeObject(iTime, param["player"]["time"]);
  JSONRPC::CJSONUtils::MillisecondsToTimeObject(seekOffset, param["player"]["seekoffset"]);
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  param["player"]["speed"] = static_cast<int>(appPlayer->GetPlaySpeed());
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSeek",
                                                     m_itemCurrentFile, param);

  CDataCacheCore::GetInstance().SeekFinished(static_cast<int>(seekOffset));
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

  CVariant param;
  param["player"]["speed"] = iSpeed;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSpeedChanged",
                                                     m_itemCurrentFile, param);
}

void CApplicationPlayerCallback::OnAVChange()
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVChange");

  CServiceBroker::GetGUI()->GetStereoscopicsManager().OnStreamChange();

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVCHANGE, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVChange",
                                                     m_itemCurrentFile, param);
}

void CApplicationPlayerCallback::OnAVStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "CApplication::OnAVStarted");

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVSTARTED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  CVariant param;
  param["player"]["speed"] = 1;
  param["player"]["playerid"] = CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist();
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVStart",
                                                     m_itemCurrentFile, param);
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
