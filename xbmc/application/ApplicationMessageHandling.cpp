/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationMessageHandling.h"

#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "Util.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "application/ApplicationEnums.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationStackHelper.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogBusy.h"
#include "favourites/FavouritesService.h"
#include "filesystem/IDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/UPnPDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/json-rpc/JSONUtils.h"
#include "interfaces/python/XBPython.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicFileItemClassify.h"
#include "network/Network.h"
#include "pictures/SlideShowDelegator.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFileItemClassify.h"
#include "powermanagement/PowerManager.h"
#include "profiles/Profile.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/URIUtils.h"
#include "video/VideoFileItemClassify.h"
#ifdef TARGET_ANDROID
#include "platform/android/activity/XBMCApp.h"
#endif
#ifdef TARGET_DARWIN_EMBEDDED
#include "platform/darwin/DarwinUtils.h"
#endif
#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#endif

using namespace KODI;

namespace
{
class CPlaycountIncrementedHandler
{
public:
  explicit CPlaycountIncrementedHandler(const CFileItem& item) : m_item(item) {}

  bool HandlePlaycountIncremented() const
  {
    return m_item.GetProperty("playcount_incremented").asBoolean(false) &&
           m_item.IsPVRRecording() &&
           CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ProcessDeleteAfterWatch(
               m_item);
  }

private:
  const CFileItem m_item;
};
} // unnamed namespace

CApplicationMessageHandling::CApplicationMessageHandling(CApplication& app)
  : CAppInboundProtocol(app),
    m_app(app)
{
}

void CApplicationMessageHandling::OnApplicationMessage(MESSAGING::ThreadMessage* pMsg)
{
  uint32_t msg = pMsg->dwMessage;
  if (msg == TMSG_SYSTEM_POWERDOWN)
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
      msg = pMsg->param1; // perform requested shutdown action
    else
      return; // no shutdown
  }

  const auto appPlayer = m_app.GetComponent<CApplicationPlayer>();

  switch (msg)
  {
    case TMSG_POWERDOWN:
      if (m_app.Stop(EXITCODE_POWERDOWN))
        CServiceBroker::GetPowerManager().Powerdown();
      break;

    case TMSG_QUIT:
      m_app.Stop(EXITCODE_QUIT);
      break;

    case TMSG_SHUTDOWN:
      m_app.GetComponent<CApplicationPowerHandling>()->HandleShutdownMessage();
      break;

    case TMSG_RENDERER_FLUSH:
      appPlayer->FlushRenderer();
      break;

    case TMSG_HIBERNATE:
      CServiceBroker::GetPowerManager().Hibernate();
      break;

    case TMSG_SUSPEND:
      CServiceBroker::GetPowerManager().Suspend();
      break;

    case TMSG_RESTART:
    case TMSG_RESET:
      if (m_app.Stop(EXITCODE_REBOOT))
        CServiceBroker::GetPowerManager().Reboot();
      break;

    case TMSG_RESTARTAPP:
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
      m_app.Stop(EXITCODE_RESTARTAPP);
#endif
      break;

    case TMSG_INHIBITIDLESHUTDOWN:
      m_app.GetComponent<CApplicationPowerHandling>()->InhibitIdleShutdown(pMsg->param1 != 0);
      break;

    case TMSG_INHIBITSCREENSAVER:
      m_app.GetComponent<CApplicationPowerHandling>()->InhibitScreenSaver(pMsg->param1 != 0);
      break;

    case TMSG_ACTIVATESCREENSAVER:
      m_app.GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
      break;

    case TMSG_RESETSCREENSAVER:
      m_app.GetComponent<CApplicationPowerHandling>()->m_bResetScreenSaver = true;
      break;

    case TMSG_VOLUME_SHOW:
    {
      CAction action(pMsg->param1);
      m_app.GetComponent<CApplicationVolumeHandling>()->ShowVolumeBar(&action);
    }
    break;

#ifdef TARGET_ANDROID
    case TMSG_DISPLAY_SETUP:
      // We might come from a refresh rate switch destroying the native window; use the context resolution
      *static_cast<bool*>(pMsg->lpVoid) =
          m_app.InitWindow(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution());
      m_app.GetComponent<CApplicationPowerHandling>()->SetRenderGUI(true);
      break;

    case TMSG_DISPLAY_DESTROY:
      *static_cast<bool*>(pMsg->lpVoid) = CServiceBroker::GetWinSystem()->DestroyWindow();
      m_app.GetComponent<CApplicationPowerHandling>()->SetRenderGUI(false);
      break;

    case TMSG_RESUMEAPP:
    {
      m_app.m_ServiceManager->GetNetwork().NetworkMessage(CNetworkBase::SERVICES_UP, 0);
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
        gui->GetWindowManager().MarkDirty();
      break;
    }
#endif

    case TMSG_START_ANDROID_ACTIVITY:
    {
#if defined(TARGET_ANDROID)
      if (!pMsg->params.empty())
      {
        CXBMCApp::StartActivity(pMsg->params[0], pMsg->params.size() > 1 ? pMsg->params[1] : "",
                                pMsg->params.size() > 2 ? pMsg->params[2] : "",
                                pMsg->params.size() > 3 ? pMsg->params[3] : "",
                                pMsg->params.size() > 4 ? pMsg->params[4] : "",
                                pMsg->params.size() > 5 ? pMsg->params[5] : "",
                                pMsg->params.size() > 6 ? pMsg->params[6] : "",
                                pMsg->params.size() > 7 ? pMsg->params[7] : "",
                                pMsg->params.size() > 8 ? pMsg->params[8] : "");
      }
#endif
    }
    break;

    case TMSG_NETWORKMESSAGE:
      m_app.m_ServiceManager->GetNetwork().NetworkMessage(
          static_cast<CNetworkBase::EMESSAGE>(pMsg->param1), pMsg->param2);
      break;

    case TMSG_SETLANGUAGE:
      m_app.SetLanguage(pMsg->strParam);
      break;

    case TMSG_SWITCHTOFULLSCREEN:
    {
      if (CGUIComponent* gui = CServiceBroker::GetGUI(); gui)
        gui->GetWindowManager().SwitchToFullScreen(true);
      break;
    }
    case TMSG_VIDEORESIZE:
    {
      XBMC_Event newEvent = {};
      newEvent.type = XBMC_VIDEORESIZE;
      newEvent.resize.width = pMsg->param1;
      newEvent.resize.height = pMsg->param2;
      newEvent.resize.scale = 1.0;
      this->OnEvent(newEvent);
      CServiceBroker::GetGUI()->GetWindowManager().MarkDirty();
    }
    break;

    case TMSG_SETVIDEORESOLUTION:
      CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(
          static_cast<RESOLUTION>(pMsg->param1), pMsg->param2 == 1);
      break;

    case TMSG_TOGGLEFULLSCREEN:
      CServiceBroker::GetWinSystem()->GetGfxContext().ToggleFullScreen();
      appPlayer->TriggerUpdateResolution();
      break;

    case TMSG_MOVETOSCREEN:
      CServiceBroker::GetWinSystem()->MoveToScreen(pMsg->param1);
      break;

    case TMSG_MINIMIZE:
      CServiceBroker::GetWinSystem()->Minimize();
      break;

    case TMSG_EXECUTE_OS:
      // Suspend AE temporarily so exclusive or hog-mode sinks
      // don't block external player's access to audio device
      IAE* audioengine;
      audioengine = CServiceBroker::GetActiveAE();
      if (audioengine && !audioengine->Suspend())
        CLog::LogF(LOGINFO, "Failed to suspend AudioEngine before launching external program");

#if defined(TARGET_DARWIN)
      CLog::Log(LOGINFO, "ExecWait is not implemented on this platform");
#elif defined(TARGET_POSIX)
      CUtil::RunCommandLine(pMsg->strParam, (pMsg->param1 == 1));
#elif defined(TARGET_WINDOWS)
      CWIN32Util::XBMCShellExecute(pMsg->strParam, (pMsg->param1 == 1));
#endif
      // Resume AE processing of XBMC native audio
      if (audioengine && !audioengine->Resume())
        CLog::LogF(LOGFATAL, "Failed to restart AudioEngine after return from external player");

      break;

    case TMSG_EXECUTE_SCRIPT:
      CScriptInvocationManager::GetInstance().ExecuteAsync(pMsg->strParam);
      break;

    case TMSG_EXECUTE_BUILT_IN:
      CBuiltins::GetInstance().Execute(pMsg->strParam);
      break;

    case TMSG_PICTURE_SHOW:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();

      // stop playing file
      if (appPlayer->IsPlayingVideo())
        m_app.StopPlaying();

      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

      const auto appPower = m_app.GetComponent<CApplicationPowerHandling>();
      appPower->ResetScreenSaver();
      appPower->WakeUpScreenSaverAndDPMS();

      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
      if (URIUtils::IsZIP(pMsg->strParam) || URIUtils::IsRAR(pMsg->strParam)) // actually a cbz/cbr
      {
        CFileItemList items;
        CURL pathToUrl;
        if (URIUtils::IsZIP(pMsg->strParam))
          pathToUrl = URIUtils::CreateArchivePath("zip", CURL(pMsg->strParam), "");
        else
          pathToUrl = URIUtils::CreateArchivePath("rar", CURL(pMsg->strParam), "");

        CUtil::GetRecursiveListing(
            pathToUrl.Get(), items,
            CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
            XFILE::DIR_FLAG_NO_FILE_DIRS);
        if (!items.IsEmpty())
        {
          slideShow.Reset();
          for (const auto& item : items)
          {
            slideShow.Add(item.get());
          }
          slideShow.Select(items[0]->GetPath());
        }
      }
      else
      {
        CFileItem item(pMsg->strParam, false);
        slideShow.Reset();
        slideShow.Add(&item);
        slideShow.Select(pMsg->strParam);
      }
    }
    break;

    case TMSG_PICTURE_SLIDESHOW:
    {
      CSlideShowDelegator& slideShow = CServiceBroker::GetSlideShowDelegator();

      if (appPlayer->IsPlayingVideo())
        m_app.StopPlaying();

      slideShow.Reset();

      CFileItemList items;
      std::string strPath = pMsg->strParam;
      std::string extensions = CServiceBroker::GetFileExtensionProvider().GetPictureExtensions();
      if (pMsg->param1)
        extensions += "|.tbn";
      CUtil::GetRecursiveListing(strPath, items, extensions);

      if (!items.IsEmpty())
      {
        for (const auto& item : items)
          slideShow.Add(item.get());
        slideShow.StartSlideShow(); //Start the slideshow!
      }

      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
      {
        if (items.IsEmpty())
        {
          CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(
              CSettings::SETTING_SCREENSAVER_MODE, "screensaver.xbmc.builtin.dim");
          m_app.GetComponent<CApplicationPowerHandling>()->ActivateScreenSaver();
        }
        else
          CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
      }
    }
    break;

    case TMSG_LOADPROFILE:
    {
      const int profile = pMsg->param1;
      if (profile > INVALID_PROFILE_ID)
        CServiceBroker::GetSettingsComponent()->GetProfileManager()->LoadProfile(
            static_cast<unsigned int>(profile));
    }

    break;

    case TMSG_EVENT:
    {
      if (pMsg->lpVoid)
      {
        const auto* event{static_cast<XBMC_Event*>(pMsg->lpVoid)};
        this->OnEvent(*event);
        delete event;
      }
    }
    break;

    case TMSG_UPDATE_PLAYER_ITEM:
    {
      std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
      if (item)
      {
        m_app.CurrentFileItem().UpdateInfo(*item);
        CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(m_app.CurrentFileItem());
      }
    }
    break;

    case TMSG_SET_VOLUME:
    {
      const auto volumedB{static_cast<float>(pMsg->param3)};
      m_app.GetComponent<CApplicationVolumeHandling>()->SetVolume(volumedB);
    }
    break;

    case TMSG_SET_MUTE:
    {
      m_app.GetComponent<CApplicationVolumeHandling>()->SetMute(pMsg->param3 == 1 ? true : false);
    }
    break;

    case TMSG_PROCESS_DELETE_AFTER_WATCH:
    {
      const std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
      const CPlaycountIncrementedHandler playcountIncrementedHandler(*item);
      playcountIncrementedHandler.HandlePlaycountIncremented();
      break;
    }

    default:
      CLog::LogF(LOGERROR, "Unhandled threadmessage sent, {}", msg);
      break;
  }
}

bool CApplicationMessageHandling::OnMessage(const CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        // Update general playlist: Remove DVD playlist items
        if (CServiceBroker::GetPlaylistPlayer().RemoveDVDItems() > 0)
        {
          CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
        }
        // stop the file if it's on dvd (will set the resume point etc)
        if (m_app.CurrentFileItem().IsOnDVD())
          m_app.StopPlaying();
      }
      else if (message.GetParam1() == GUI_MSG_UI_READY)
      {
        // remove splash window
        CServiceBroker::GetGUI()->GetWindowManager().Delete(WINDOW_SPLASH);

        // show the volumebar if the volume is muted
        const auto appVolume = m_app.GetComponent<CApplicationVolumeHandling>();
        if (appVolume->IsMuted() ||
            appVolume->GetVolumeRatio() <= CApplicationVolumeHandling::VOLUME_MINIMUM)
          appVolume->ShowVolumeBar();

        // offer enabling addons at kodi startup that are disabled due to
        // e.g. os package manager installation on linux
        m_app.ConfigureAndEnableAddons();

        m_app.DoneInitializing();

        if (message.GetSenderId() == WINDOW_SETTINGS_PROFILES)
          m_app.GetComponent<CApplicationSkinHandling>()->ReloadSkin(false);
      }
      else if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        if (m_app.CurrentFileItem().IsSamePath(item.get()))
        {
          m_app.CurrentFileItem().UpdateInfo(*item);
          CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(*item);
        }
      }
    }
    break;

    case GUI_MSG_PLAYBACK_STARTED:
    {
#ifdef TARGET_DARWIN_EMBEDDED
      // @TODO move this away to platform code
      CDarwinUtils::SetScheduling(m_app.GetComponent<CApplicationPlayer>()->IsPlayingVideo());
#endif
      m_app.SetCurrentFileItem(
          std::make_shared<CFileItem>(*std::static_pointer_cast<CFileItem>(message.GetItem())));
      m_app.ResetPlayerEvent();

      CServiceBroker::GetPVRManager().OnPlaybackStarted(m_app.CurrentFileItem());

      PLAYLIST::CPlayList playList = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());

      // Update our infoManager with the new details etc.
      if (m_app.m_nextPlaylistItem >= 0)
      {
        // playing an item which is not in the list - player might be stopped already
        // so do nothing
        if (playList.size() <= m_app.m_nextPlaylistItem)
          return true;

        // we've started a previously queued item
        CFileItemPtr item = playList[m_app.m_nextPlaylistItem];
        // update the playlist manager
        int currentSong = CServiceBroker::GetPlaylistPlayer().GetCurrentItemIdx();
        int param = ((currentSong & 0xffff) << 16) | (m_app.m_nextPlaylistItem & 0xffff);
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_CHANGED, 0, 0,
                        static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist()),
                        param, item);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(m_app.m_nextPlaylistItem);
        m_app.SetCurrentFileItem(std::make_shared<CFileItem>(*item));
      }
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*m_app.m_itemCurrentFile);
      g_partyModeManager.OnSongChange(true);

#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnPlayBackStarted(*m_app.m_itemCurrentFile);
#endif

      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());

      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPlay",
                                                         m_app.CurrentFileItemPtr(), param);

      // we don't want a busy dialog when switching channels
      const auto appPlayer = m_app.GetComponent<CApplicationPlayer>();
      if (!m_app.CurrentFileItem().IsLiveTV() ||
          (!appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio()))
        CGUIDialogBusy::WaitOnEvent(m_app.m_playerEvent);

      return true;
    }
    break;

    case GUI_MSG_QUEUE_NEXT_ITEM:
    {
      // Check to see if our playlist player has a new item for us,
      // and if so, we check whether our current player wants the file
      int iNext = CServiceBroker::GetPlaylistPlayer().GetNextItemIdx();
      const PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(
          CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      if (iNext < 0 || iNext >= playlist.size())
      {
        m_app.GetComponent<CApplicationPlayer>()->OnNothingToQueueNotify();
        return true; // nothing to do
      }

      // ok, grab the next song
      CFileItem file(*playlist[iNext]);
      // handle plugin://
      CURL url(file.GetDynPath());
      if (url.IsProtocol("plugin"))
        XFILE::CPluginDirectory::GetPluginResult(url.Get(), file, false);

      // Don't queue if next media type is different from current one
      bool bNothingToQueue = false;

      const auto appPlayer = m_app.GetComponent<CApplicationPlayer>();
      if (!VIDEO::IsVideo(file) && appPlayer->IsPlayingVideo())
        bNothingToQueue = true;
      else if ((!MUSIC::IsAudio(file) || VIDEO::IsVideo(file)) && appPlayer->IsPlayingAudio())
        bNothingToQueue = true;

      if (bNothingToQueue)
      {
        appPlayer->OnNothingToQueueNotify();
        return true;
      }

#ifdef HAS_UPNP
      if (URIUtils::IsUPnP(file.GetDynPath()) &&
          !XFILE::CUPnPDirectory::GetResource(file.GetDynURL(), file))
        return true;
#endif

      // ok - send the file to the player, if it accepts it
      if (appPlayer->QueueNextFile(file))
      {
        // player accepted the next file
        m_app.m_nextPlaylistItem = iNext;
      }
      else
      {
        /* Player didn't accept next file: *ALWAYS* advance playlist in this case so the player can
            queue the next (if it wants to) and it doesn't keep looping on this song */
        CServiceBroker::GetPlaylistPlayer().SetCurrentItemIdx(iNext);
      }

      return true;
    }
    break;

    case GUI_MSG_PLAY_TRAILER:
    {
      const CFileItem* item = dynamic_cast<CFileItem*>(message.GetItem().get());
      if (item == nullptr)
      {
        CLog::LogF(LOGERROR, "Supplied item is not a CFileItem! Trailer cannot be played.");
        return false;
      }

      std::unique_ptr<CFileItem> trailerItem =
          ContentUtils::GeneratePlayableTrailerItem(*item, g_localizeStrings.Get(20410));

      if (PLAYLIST::IsPlayList(*item))
      {
        auto fileitemList{std::make_unique<CFileItemList>()};
        fileitemList->Add(std::move(trailerItem));
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, -1, -1,
                                                   static_cast<void*>(fileitemList.release()));
      }
      else
      {
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY, 1, 0,
                                                   static_cast<void*>(trailerItem.release()));
      }
      break;
    }

    case GUI_MSG_PLAYBACK_STOPPED:
    {
      CServiceBroker::GetPVRManager().OnPlaybackStopped(m_app.CurrentFileItem());
      CServiceBroker::GetFavouritesService().OnPlaybackStopped(m_app.CurrentFileItem());

      CVariant data(CVariant::VariantTypeObject);
      data["end"] = false;
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                         m_app.CurrentFileItemPtr(), data);

      const CPlaycountIncrementedHandler playCountIncrementedHandler{m_app.CurrentFileItem()};

      m_app.m_playerEvent.Set();
      m_app.ResetCurrentItem();
      m_app.PlaybackCleanup();

#ifdef HAS_PYTHON
      CServiceBroker::GetXBPython().OnPlayBackStopped();
#endif

      playCountIncrementedHandler.HandlePlaycountIncremented();
      return true;
    }

    case GUI_MSG_PLAYBACK_ENDED:
    {
      CServiceBroker::GetPVRManager().OnPlaybackEnded(m_app.CurrentFileItem());
      CServiceBroker::GetFavouritesService().OnPlaybackEnded(m_app.CurrentFileItem());

      CVariant data(CVariant::VariantTypeObject);
      data["end"] = true;
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop",
                                                         m_app.CurrentFileItemPtr(), data);

      m_app.m_playerEvent.Set();

      if (const auto stackHelper{m_app.GetComponent<CApplicationStackHelper>()};
          stackHelper->IsPlayingStack() && stackHelper->HasNextStackPartFileItem())
      {
        // If current stack part finished then play the next part
        if (stackHelper->IsCurrentPartFinished())
        {
          m_app.PlayFile(stackHelper->SetNextStackPartAsCurrent(), "", true);
          if (!m_app.WasPlaybackCancelled())
            return true;

          // Selection of next part playlist cancelled so create bookmark for next part
          stackHelper->SetNextPartBookmark(m_app.CurrentFileItem().GetDynPath());
        }
      }

      // For EPG playlist items we keep the player open to ensure continuous viewing experience.
      const bool isEpgPlaylistItem{
          m_app.CurrentFileItem().GetProperty("epg_playlist_item").asBoolean(false)};

      const CPlaycountIncrementedHandler playCountIncrementedHandler{m_app.CurrentFileItem()};

      m_app.ResetCurrentItem();

      if (!isEpgPlaylistItem)
      {
        if (!CServiceBroker::GetPlaylistPlayer().PlayNext(1, true))
          m_app.GetComponent<CApplicationPlayer>()->ClosePlayer();

        m_app.PlaybackCleanup();
      }

#ifdef HAS_PYTHON
      CServiceBroker::GetXBPython().OnPlayBackEnded();
#endif

      playCountIncrementedHandler.HandlePlaycountIncremented();
      return true;
    }

    case GUI_MSG_PLAYLISTPLAYER_STOPPED:
      m_app.ResetCurrentItem();
      if (m_app.GetComponent<CApplicationPlayer>()->IsPlaying())
        m_app.StopPlaying();
      m_app.PlaybackCleanup();
      return true;

    case GUI_MSG_PLAYBACK_AVSTARTED:
    {
      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVStart",
                                                         m_app.CurrentFileItemPtr(), param);
      m_app.m_playerEvent.Set();
#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnAVStarted(m_app.CurrentFileItem());
#endif
      return true;
    }

    case GUI_MSG_PLAYBACK_AVCHANGE:
    {
#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnAVChange();
#endif
      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnAVChange",
                                                         m_app.CurrentFileItemPtr(), param);
      return true;
    }

    case GUI_MSG_PLAYBACK_PAUSED:
    {
      CVariant param;
      param["player"]["speed"] = 0;
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPause",
                                                         m_app.CurrentFileItemPtr(), param);
      return true;
    }

    case GUI_MSG_PLAYBACK_RESUMED:
    {
      CVariant param;
      param["player"]["speed"] = 1;
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnResume",
                                                         m_app.CurrentFileItemPtr(), param);
      return true;
    }

    case GUI_MSG_PLAYBACK_SEEKED:
    {
      CVariant param;
      const int64_t iTime = message.GetParam1AsI64();
      const int64_t seekOffset = message.GetParam2AsI64();
      JSONRPC::CJSONUtils::MillisecondsToTimeObject(static_cast<int>(iTime),
                                                    param["player"]["time"]);
      JSONRPC::CJSONUtils::MillisecondsToTimeObject(static_cast<int>(seekOffset),
                                                    param["player"]["seekoffset"]);
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      const auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      param["player"]["speed"] = static_cast<int>(appPlayer->GetPlaySpeed());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSeek",
                                                         m_app.CurrentFileItemPtr(), param);

      CDataCacheCore::GetInstance().SeekFinished(static_cast<int>(seekOffset));

      return true;
    }

    case GUI_MSG_PLAYBACK_SPEED_CHANGED:
    {
      CVariant param;
      param["player"]["speed"] = message.GetParam1();
      param["player"]["playerid"] =
          static_cast<int>(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist());
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnSpeedChanged",
                                                         m_app.CurrentFileItemPtr(), param);

      return true;
    }

    case GUI_MSG_PLAYBACK_ERROR:
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{16026}, CVariant{16027});
      return true;

    case GUI_MSG_PLAYLISTPLAYER_STARTED:
    case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    {
      return true;
    }
    break;
    case GUI_MSG_FULLSCREEN:
    {
      // Switch to fullscreen, if we can
      if (CGUIComponent* gui = CServiceBroker::GetGUI(); gui)
        gui->GetWindowManager().SwitchToFullScreen();

      return true;
    }
    break;
    case GUI_MSG_EXECUTE:
      if (message.GetNumStringParams())
        return m_app.ExecuteXBMCAction(message.GetStringParam(), message.GetItem());
      break;
    default:
      break;
  }
  return false;
}
