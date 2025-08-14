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
#include "storage/MediaManager.h"
#include "utils/JobManager.h"
#include "utils/SaveFileStateJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"

#include <chrono>
#include <memory>

using namespace KODI;
using namespace std::chrono_literals;

CApplicationPlayerCallback::CApplicationPlayerCallback()
{
}

void CApplicationPlayerCallback::OnPlayBackEnded()
{
  CLog::LogF(LOGDEBUG, "call");

  CGUIMessage msg(GUI_MSG_PLAYBACK_ENDED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CApplicationPlayerCallback::OnPlayBackStarted(const CFileItem& file)
{
  CLog::LogF(LOGDEBUG, "call");
  std::shared_ptr<CFileItem> itemCurrentFile;

  // check if VideoPlayer should set file item stream details from its current streams
  const bool isBlu_dvd_image_or_stream = VIDEO::IsBDFile(file) || VIDEO::IsDVDFile(file) ||
                                         file.IsDiscImage() || NETWORK::IsInternetStream(file);

  const bool hasNoStreamDetails =
      (!file.HasVideoInfoTag() || !file.GetVideoInfoTag()->HasStreamDetails());

  // Always update streamdetails for bluray:// paths as existing details may be been taken from BLURAY_TITLE_INFO
  if (file.GetProperty("get_stream_details_from_player").asBoolean() ||
      (hasNoStreamDetails && isBlu_dvd_image_or_stream) ||
      URIUtils::IsBlurayPath(file.GetDynPath()))
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->SetUpdateStreamDetails();
  }

  auto& components = CServiceBroker::GetAppComponents();
  const auto stackHelper = components.GetComponent<CApplicationStackHelper>();

  if (stackHelper->IsPlayingStack())
    itemCurrentFile = std::make_shared<CFileItem>(*stackHelper->GetStack(file));
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

namespace
{
void UpdateRemovableBlurayPath(CFileItem& fileItem)
{
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
    }
  }
#endif
}

/*!
 * \brief Updates CApplicationStackHelper, fileItem and database stacktimes with the actual file played (eg. bluray://) and times
 * \param file The FileItem of the actual file played (updated in InputStream). file.DynPath() contains the path played (eg. bluray://)
 * \param fileItem The original FileItem from the stack. fileItem.DynPath() contains the old stack:// path
 * \return stackHelper The current CApplicationStackHelper
 */
bool UpdateDiscStack(const CFileItem& file,
                     CFileItem& fileItem,
                     const std::shared_ptr<CApplicationStackHelper>& stackHelper)
{
  // Build stacktimes for disc image stacks after each disc is played
  CVideoDatabase dbs;
  if (!dbs.Open())
  {
    CLog::LogF(LOGDEBUG, "Failed to open database.");
    return false;
  }

  // Get existing stacktimes (if any)
  std::vector<std::chrono::milliseconds> times;
  bool haveTimes{dbs.GetStackTimes(fileItem.GetDynPath(), times)};

  // See if new part played
  if (stackHelper->GetKnownStackParts() - 1 == static_cast<int>(times.size()))
  {
    // Update filename for this part in stack://
    std::string stackedPath{fileItem.GetDynPath()};
    const int partNumber{stackHelper->GetStackPartNumber(file)};
    std::vector<std::string> paths{};
    XFILE::CStackDirectory::GetPaths(stackedPath, paths);
    paths[partNumber] = file.GetDynPath();
    XFILE::CStackDirectory::ConstructStackPath(paths, stackedPath);
    fileItem.SetDynPath(stackedPath);
    fileItem.SetProperty("new_stack_path", true);

    // FileItems in the stack helper have stack:// path in DynPath
    // So update all FileItems in the stack
    stackHelper->SetStackDynPaths(stackedPath);

    std::chrono::milliseconds thisPartTime{
        file.GetVideoInfoTag()->m_streamDetails.GetVideoDuration() * 1000ms};
    const std::chrono::milliseconds startPartTime{haveTimes ? times.back() : 0ms};
    const std::chrono::milliseconds totalTime{startPartTime + thisPartTime};

    // Add this part's time to end of stacktimes
    times.emplace_back(totalTime);
    dbs.SetStackTimes(stackedPath, times);

    // Update stack times (for bookmark and % played)
    stackHelper->SetStackPartStartTime(file, startPartTime);
    stackHelper->SetStackTotalTime(totalTime);

    // If adding part then update streamdetails as well
    fileItem.GetVideoInfoTag()->m_streamDetails = file.GetVideoInfoTag()->m_streamDetails;

    // Also update video info tag with total time of the stack (as this is read for the library display)
    fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
        0, static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(totalTime).count()));
  }
  else
  {
    // Still need to update video duration for total stack time
    fileItem.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
        0, static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                                stackHelper->GetStackTotalTime(file))
                                .count()));
  }
  dbs.Close();

  return true;
}

void ConvertRelativeStackTimesToAbsolute(
    CBookmark& bookmark,
    const CFileItem& file,
    const std::shared_ptr<CApplicationStackHelper>& stackHelper)
{
  // The bookmark from player is relative; needs to be corrected for absolute position within stack
  bookmark.timeInSeconds +=
      static_cast<double>(stackHelper->GetStackPartStartTime(file).count()) / 1000.0;
  bookmark.totalTimeInSeconds =
      static_cast<double>(stackHelper->GetStackTotalTime(file).count()) / 1000.0;
}

bool WithinPercentOfEnd(const CBookmark& bookmark, float ignorePercentAtEnd)
{
  return ignorePercentAtEnd > 0.0 &&
         (bookmark.totalTimeInSeconds - bookmark.timeInSeconds) <
             (static_cast<double>(ignorePercentAtEnd) * bookmark.totalTimeInSeconds / 100.0);
}

bool UpdateDiscStackBookmark(CBookmark& bookmark,
                             const CFileItem& file,
                             const std::shared_ptr<CAdvancedSettings>& advancedSettings,
                             const std::shared_ptr<CApplicationStackHelper>& stackHelper)
{
  // Only show a stack as played if all parts have been played
  const bool allStackPartsPlayed{
      [&stackHelper]
      {
        if (stackHelper->GetCurrentPartNumber() + 1 == stackHelper->GetTotalPartNumbers())
          return true; // If playing a disc stack then all parts are played when the last part is played
        return false;
      }()};

  // Define finished as all parts have been played to end and overall stack has been played to within
  // videoIgnorePercentAtEnd of the end (if defined)
  const bool finished{
      [&]
      {
        if (!file.GetProperty("stopped_before_end").asBoolean(false))
          return true; // For disc stacks, if not flagged then we are at end of part (decision made in InputStream)
        if (allStackPartsPlayed &&
            WithinPercentOfEnd(bookmark, advancedSettings->m_videoIgnorePercentAtEnd))
          return true; // Within videoIgnorePercentAtEnd of the end so consider watched
        return false;
      }()};

  const bool noMainTitle{file.GetProperty("no_main_title").asBoolean(false)};

  bookmark.partNumber = stackHelper->GetStackPartNumber(file);
  stackHelper->SetCurrentPartFinished(finished);
  if (finished)
  {
    if (noMainTitle)
    {
      if (stackHelper->GetCurrentPartNumber() > 0)
      {
        // Not played main title of this part yet
        bookmark.timeInSeconds = bookmark.partNumber;
        bookmark.totalTimeInSeconds = stackHelper->GetTotalPartNumbers();
        bookmark.playerState = StringUtils::Format("<nextpart>{}</nextpart>", bookmark.partNumber);
      }
      else
      {
        // Not played anything yet
        bookmark.timeInSeconds = bookmark.totalTimeInSeconds = 0;
        return false;
      }
    }
    else
    {
      if (allStackPartsPlayed)
      {
        // Finished entire stack
        bookmark.timeInSeconds = -1.0;
      }
      else
      {
        // Ended in menu (or non-main title) with part(s) still to play
        bookmark.partNumber += 1;
        bookmark.timeInSeconds = bookmark.partNumber;
        bookmark.totalTimeInSeconds = stackHelper->GetTotalPartNumbers();
        bookmark.playerState = StringUtils::Format("<nextpart>{}</nextpart>", bookmark.partNumber);
      }
    }
  }
  else
  {
    // Not finished current part
    if (allStackPartsPlayed &&
        WithinPercentOfEnd(bookmark, advancedSettings->m_videoIgnorePercentAtEnd))
      bookmark.timeInSeconds = -1.0;
    else if (stackHelper->GetCurrentPartNumber() == 0 &&
             bookmark.timeInSeconds < advancedSettings->m_videoIgnoreSecondsAtStart)
      bookmark.timeInSeconds = 0.0;
    else
      ConvertRelativeStackTimesToAbsolute(bookmark, file, stackHelper);
  }
  return true;
}
} // namespace

void CApplicationPlayerCallback::OnPlayerCloseFile(const CFileItem& file,
                                                   const CBookmark& bookmarkParam)
{
  auto& components{CServiceBroker::GetAppComponents()};
  const auto stackHelper{components.GetComponent<CApplicationStackHelper>()};
  const std::shared_ptr advancedSettings{
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()};

  std::unique_lock lock(stackHelper->m_critSection);

  // Make sure we don't reset existing bookmark etc. on eg. player start failure
  if (bookmarkParam.timeInSeconds == 0.0)
    return;

  const bool isStack{stackHelper->GetStack(file) != nullptr};
  CFileItem fileItem;

  if (isStack)
  {
    // Get stack component (current fileItem refers to single part)
    fileItem = *stackHelper->GetStack(file);
  }
  else
  {
    fileItem = file;

#ifdef HAVE_LIBBLURAY
    // Adjust paths of new fileItem for physical/removable blurays if needed
    UpdateRemovableBlurayPath(fileItem);
#endif
  }

  CBookmark bookmark{bookmarkParam};
  if (isStack && stackHelper->WasPlayingDiscStack())
  {
    // Update disc stack stack:// url and times if needed
    if (!UpdateDiscStack(file, fileItem, stackHelper))
      return;

    // Update bookmark
    if (!UpdateDiscStackBookmark(bookmark, file, advancedSettings, stackHelper))
      fileItem.GetVideoInfoTag()
          ->m_streamDetails.Reset(); // Don't save streamdetails as nothing played
  }
  else
  {
    // Now deal with regular stacks and non-stacks
    if (WithinPercentOfEnd(bookmark, advancedSettings->m_videoIgnorePercentAtEnd))
      bookmark.timeInSeconds = -1.0;
    else if (bookmark.timeInSeconds < advancedSettings->m_videoIgnoreSecondsAtStart)
      bookmark.timeInSeconds = 0.0;
    else if (isStack)
      ConvertRelativeStackTimesToAbsolute(bookmark, file, stackHelper);

    // SaveFileStateJob needs the (updated) stack:// path in the DynPath
    if (isStack)
      fileItem.SetDynPath(file.GetDynPath());
  }

  const bool playCountUpdate{
      [&]
      {
        if (bookmark.timeInSeconds < 0.0)
          return true; // Finished
        const float percent{
            static_cast<float>(bookmark.timeInSeconds / bookmark.totalTimeInSeconds) * 100.0f};
        if (MUSIC::IsAudio(fileItem) && advancedSettings->m_audioPlayCountMinimumPercent > 0 &&
            percent >= advancedSettings->m_audioPlayCountMinimumPercent)
          return true;
        if (VIDEO::IsVideo(fileItem) && advancedSettings->m_videoPlayCountMinimumPercent > 0 &&
            percent >= advancedSettings->m_videoPlayCountMinimumPercent)
          return true;
        return false;
      }()};

  if (CServiceBroker::GetSettingsComponent()
          ->GetProfileManager()
          ->GetCurrentProfile()
          .canWriteDatabases())
  {
    CSaveFileState::DoWork(fileItem, bookmark, playCountUpdate);
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
