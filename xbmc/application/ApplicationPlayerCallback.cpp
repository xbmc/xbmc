/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayerCallback.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationStackHelper.h"
#include "filesystem/BlurayDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/StereoscopicsManager.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/python/XBPython.h"
#include "jobs/JobManager.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/SaveFileStateJob.h"
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
  CLog::LogF(LOGDEBUG, "call");

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

namespace
{
bool ShouldUpdateStreamDetails(const CFileItem& file)
{
  // If a title/playlist hasn't been selected for a bluray/dvds then the stream details may not be known
  const bool isDiscOrStream{VIDEO::IsBDFile(file) || VIDEO::IsDVDFile(file) || file.IsDiscImage() ||
                            NETWORK::IsInternetStream(file)};

  // Stream details may be already set from a previous playback or nfo
  const bool hasNoStreamDetails{!file.HasVideoInfoTag() ||
                                !file.GetVideoInfoTag()->HasStreamDetails()};

  return hasNoStreamDetails && isDiscOrStream;
}
} // namespace

void CApplicationPlayerCallback::OnPlayBackStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "call");
  std::shared_ptr<CFileItem> itemCurrentFile;

  // check if VideoPlayer should set file item stream details from its current streams
  if (ShouldUpdateStreamDetails(file))
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetUpdateStreamDetails();
  }

  auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingISOStack() || stackHelper->IsPlayingRegularStack())
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

  std::unique_lock lock(stackHelper->m_critSection);

  CFileItem fileItem(file);
  CBookmark bookmark = bookmarkParam;
  CBookmark resumeBookmark;
  bool playCountUpdate = false;
  float percent = 0.0f;

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmark.timeInSeconds == 0.0)
    return;

    // Adjust paths of new fileItem for physical/removable blurays
    // DynPath contains the mpls (playlist) played
    // VideoInfoTag()->m_strFileNameAndPath contains the removable:// path (if played through Disc node)
    // otherwise if played through Video->Files we need to retrieve the removable:// path
    // We need to update DynPath with the removable:// path (for the database), keeping the playlist
#ifdef HAVE_LIBBLURAY
  if (fileItem.HasVideoInfoTag())
  {
    const std::string dynPath{fileItem.GetDynPath()};
    if (URIUtils::IsBlurayPath(dynPath))
    {
      const CURL fileUrl{dynPath};
      CURL url;
      if (fileItem.GetVideoInfoTag()->m_strFileNameAndPath.starts_with("bluray://removable"))
      {
        // Played through Disc node
        url.Parse(fileItem.GetVideoInfoTag()->m_strFileNameAndPath);
      }
      else
      {
#ifdef HAS_OPTICAL_DRIVE
        // Played through Video->Files
        ::UTILS::DISCS::DiscInfo info{
            CServiceBroker::GetMediaManager().GetDiscInfo(fileUrl.GetHostName())};
        if (!info.empty() && info.type == ::UTILS::DISCS::DiscType::BLURAY)
        {
          url.Parse(CServiceBroker::GetMediaManager().GetDiskUniqueId(fileUrl.GetHostName()));
        }
#endif
      }
      if (!url.Get().empty())
      {
        url.SetFileName(fileUrl.GetFileName());
        fileItem.SetPath(url.Get());
        fileItem.SetDynPath("");
#ifdef HAS_OPTICAL_DRIVE
        CServiceBroker::GetMediaManager().ResetBlurayPlaylistStatus();
#endif
      }

      // Get streamdetails from file
      if (file.GetProperty("update_stream_details").asBoolean(false))
      {
        XFILE::CBlurayDirectory dir;
        CFileItemList items;
        dir.GetDirectory(CURL(fileItem.GetDynPath()), items);
        if (items.Size() == 1 && items.Get(0)->HasVideoInfoTag())
        {
          fileItem.GetVideoInfoTag()->m_streamDetails =
              items.Get(0)->GetVideoInfoTag()->m_streamDetails;
        }
      }
    }
  }
#endif

  if (stackHelper->GetRegisteredStack(fileItem) != nullptr)
  {
    if (stackHelper->GetRegisteredStackTotalTimeMs(fileItem) > 0)
    {
      // Regular (not disc image) stack case: We have to save the bookmark on the stack.
      fileItem = *stackHelper->GetRegisteredStack(file);

      // The bookmark coming from the player is only relative to the current part, thus needs
      // to be corrected with these attributes (start time will be 0 for non-stackparts).
      bookmark.timeInSeconds +=
          static_cast<double>(stackHelper->GetRegisteredStackPartStartTimeMs(file)) / 1000.0;

      const uint64_t registeredStackTotalTimeMs{stackHelper->GetRegisteredStackTotalTimeMs(file)};
      if (registeredStackTotalTimeMs > 0)
        bookmark.totalTimeInSeconds = static_cast<double>(registeredStackTotalTimeMs) / 1000.0;
    }
    // Any stack case: We need to save the part number.
    bookmark.partNumber =
        stackHelper->GetRegisteredStackPartNumber(file) + 1; // CBookmark part numbers are 1-based
  }

  percent = bookmark.timeInSeconds / bookmark.totalTimeInSeconds * 100;

  const std::shared_ptr<CAdvancedSettings> advancedSettings =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if ((MUSIC::IsAudio(fileItem) && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
       percent >= advancedSettings->m_audioPlayCountMinimumPercent) ||
      (VIDEO::IsVideo(fileItem) && advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
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
  CLog::LogF(LOGDEBUG, "call");

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
  CLog::LogF(LOGDEBUG, "call");

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
  CLog::LogF(LOGDEBUG, "call");

  CServiceBroker::GetGUI()->GetStereoscopicsManager().OnStreamChange();

  CGUIMessage msg(GUI_MSG_PLAYBACK_AVCHANGE, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnAVStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "call");

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
