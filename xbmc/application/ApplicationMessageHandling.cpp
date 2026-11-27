/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationMessageHandling.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "Util.h"
#include "addons/Skin.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationEnums.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationStackHelper.h"
#include "application/ApplicationVolumeHandling.h"
#include "application/PlayListsMessageHandler.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/DataCacheCore.h"
#include "dialogs/GUIDialogBusy.h"
#include "favourites/FavouritesService.h"
#include "filesystem/IDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "interfaces/python/XBPython.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "network/Network.h"
#include "pictures/SlideShowDelegator.h"
#include "playlists/PlayListFileItemClassify.h"
#include "playlists/PlayListTypes.h"
#include "powermanagement/PowerManager.h"
#include "profiles/Profile.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/ItemProperties.h"
#include "utils/URIUtils.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <chrono>
#include <memory>
#include <optional>

#ifdef TARGET_ANDROID
#include "platform/android/activity/XBMCApp.h"
#endif
#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#endif

using namespace KODI;

namespace
{
std::optional<CApplication::PlaybackWindow> PlaybackWindowFromInt(int value)
{
  using enum CApplication::PlaybackWindow;
  for (const CApplication::PlaybackWindow window : {SlideShow, Video, Visualisation})
    if (value == static_cast<int>(window))
      return window;
  return std::nullopt;
}

class CPlaycountIncrementedHandler
{
public:
  explicit CPlaycountIncrementedHandler(const CFileItem& item) : m_item(item) {}

  bool HandlePlaycountIncremented() const
  {
    return m_item.GetProperty(ITEM::PROPERTY::PLAYCOUNT_INCREMENTED).asBoolean(false) &&
           m_item.IsPVRRecording() &&
           CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ProcessDeleteAfterWatch(
               m_item);
  }

private:
  const CFileItem m_item;
};

//! Whether pause message \p msg changes anything in the state \p player is in
bool PauseMessageApplies(uint32_t msg, const CApplicationPlayer& player)
{
  switch (msg)
  {
    case TMSG_MEDIA_PAUSE:
      return player.HasPlayer();
    case TMSG_MEDIA_UNPAUSE:
      return player.IsPausedPlayback();
    case TMSG_MEDIA_PAUSE_IF_PLAYING:
      return player.IsPlaying() && !player.IsPaused();
    default:
      return false;
  }
}

// Windows kept in memory keep the skin coordinates of the aspect ratio they
// were loaded for. Leave video playback alone, its refresh rate switches may
// use other aspect ratios for a while, and the calibration window, which
// switches resolutions itself.
bool IsSkinReloadNeeded()
{
  // the deferred check may run while Kodi is shutting down
  const auto gui = CServiceBroker::GetGUI();
  if (!gui)
    return false;

  const auto skin = gui->GetSkinInfo();
  return skin && skin->IsResolutionChanged() &&
         !CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()->IsPlayingVideo() &&
         !CServiceBroker::GetWinSystem()->GetGfxContext().IsCalibrating();
}

// Add-ons fill their windows and dialogs from code, and a skin reload would
// leave them empty.
bool IsAddonWindowOpen()
{
  const auto& windowManager = CServiceBroker::GetGUI()->GetWindowManager();
  for (const int id : {windowManager.GetActiveWindow(), windowManager.GetActiveWindowOrDialog()})
  {
    if (windowManager.IsPythonWindow(id) || windowManager.IsAddonWindow(id))
      return true;
  }
  return false;
}

// resizing a window sends a resize for every step, reload once it settles
constexpr auto SKIN_RELOAD_DELAY = std::chrono::milliseconds(500);
} // unnamed namespace

CApplicationMessageHandling::CApplicationMessageHandling(CApplication& app)
  : CAppInboundProtocol(app),
    m_app(app),
    m_reloadSkinCallback(std::make_unique<MESSAGING::ThreadMessageCallback>(
        MESSAGING::ThreadMessageCallback{ReloadSkinIfNeeded, this})),
    m_skinReloadTimer(
        [this]
        {
          if (const auto messenger = CServiceBroker::GetAppMessenger())
            messenger->PostMsg(TMSG_CALLBACK, -1, -1, m_reloadSkinCallback.get());
        })
{
}

CApplicationMessageHandling::~CApplicationMessageHandling() = default;

void CApplicationMessageHandling::ReloadSkinIfNeeded(void* data)
{
  if (!IsSkinReloadNeeded())
    return;

  // ReloadSkin() drops requests while Kodi is starting, and a reload would empty
  // add-on windows, so try again later
  auto* handling = static_cast<CApplicationMessageHandling*>(data);
  if (!handling->m_app.IsInitialized() || IsAddonWindowOpen())
  {
    handling->ScheduleSkinReload();
    return;
  }

  CServiceBroker::GetAppComponents().GetComponent<CApplicationSkinHandling>()->ReloadSkin();
}

void CApplicationMessageHandling::ScheduleSkinReload()
{
  // the timer callback checks again whether the reload is needed
  if (m_skinReloadTimer.IsRunning())
    m_skinReloadTimer.RestartAsync(SKIN_RELOAD_DELAY);
  else
    m_skinReloadTimer.Start(SKIN_RELOAD_DELAY);
}

void CApplicationMessageHandling::ScheduleSkinReloadAfterPlayback()
{
  // pick up a window resized during playback, in full screen only video modes
  // change and the GUI resolution is restored when playback stops
  if (!CServiceBroker::GetWinSystem()->GetGfxContext().IsFullScreenRoot())
    ScheduleSkinReload();
}

void CApplicationMessageHandling::OnApplicationMessage(MESSAGING::ThreadMessage* pMsg)
{
  CWinSystemBase* const winSystem{CServiceBroker::GetWinSystem()};
  uint32_t msg = pMsg->dwMessage;
  if (msg == TMSG_SYSTEM_POWERDOWN)
  {
    if (CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown())
      msg = pMsg->param1; // perform requested shutdown action
    else
      return; // no shutdown
  }

  const auto appPlayer = m_app.GetComponent<CApplicationPlayer>();
  const auto appPower = m_app.GetComponent<CApplicationPowerHandling>();
  const auto appVolume = m_app.GetComponent<CApplicationVolumeHandling>();

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
      appPower->HandleShutdownMessage();
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
      appPower->InhibitIdleShutdown(pMsg->param1 != 0);
      break;

    case TMSG_INHIBITSCREENSAVER:
      appPower->InhibitScreenSaver(pMsg->param1 != 0);
      break;

    case TMSG_ACTIVATESCREENSAVER:
      appPower->ActivateScreenSaver();
      break;

    case TMSG_RESETSCREENSAVER:
      appPower->m_bResetScreenSaver = true;
      break;

    case TMSG_VOLUME_SHOW:
    {
      CAction action(pMsg->param1);
      appVolume->ShowVolumeBar(&action);
    }
    break;

#ifdef TARGET_ANDROID
    case TMSG_DISPLAY_SETUP:
      // We might come from a refresh rate switch destroying the native window; use the context resolution
      *static_cast<bool*>(pMsg->lpVoid) =
          m_app.InitWindow(winSystem->GetGfxContext().GetVideoResolution());
      appPower->SetRenderGUI(true);
      break;

    case TMSG_DISPLAY_DESTROY:
      *static_cast<bool*>(pMsg->lpVoid) = winSystem->DestroyWindow();
      appPower->SetRenderGUI(false);
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
      winSystem->GetGfxContext().SetVideoResolution(
          static_cast<RESOLUTION>(pMsg->param1), pMsg->param2 == 1);
      break;

    case TMSG_TOGGLEFULLSCREEN:
      winSystem->GetGfxContext().ToggleFullScreen();
      appPlayer->TriggerUpdateResolution();
      break;

    case TMSG_MOVETOSCREEN:
      winSystem->MoveToScreen(pMsg->param1);
      break;

    case TMSG_MINIMIZE:
      winSystem->Minimize();
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
          appPower->ActivateScreenSaver();
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
      if (item && m_app.CurrentFileItem().IsSamePath(item.get()))
      {
        m_app.CurrentFileItem().UpdateInfo(*item);
        CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(m_app.CurrentFileItem());
        pMsg->SetResult(0);
      }
    }
    break;

    case TMSG_SET_VOLUME:
      appVolume->SetVolume(static_cast<float>(pMsg->param3));
      break;

    case TMSG_SET_MUTE:
      appVolume->SetMute(pMsg->param3 == 1);
      break;

    case TMSG_PROCESS_DELETE_AFTER_WATCH:
    {
      const std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
      const CPlaycountIncrementedHandler playcountIncrementedHandler(*item);
      playcountIncrementedHandler.HandlePlaycountIncremented();
      break;
    }

    case TMSG_APPLICATION_PLAY_MEDIA:
    {
      const std::unique_ptr<CFileItem> item{static_cast<CFileItem*>(pMsg->lpVoid)};
      m_app.PlayMedia(*item, pMsg->strParam, PLAYLIST::TypeFromInt(pMsg->param1));
      break;
    }

    case TMSG_MEDIA_RESTART:
      m_app.Restart(true);
      break;

    case TMSG_MEDIA_STOP:
    {
      m_app.LeavePlaybackWindow(PlaybackWindowFromInt(pMsg->param1));

      appPower->WakeScreen();

      if (appPlayer->IsPlaying())
        m_app.StopPlaying();
      break;
    }

    case TMSG_MEDIA_PAUSE:
    case TMSG_MEDIA_UNPAUSE:
    case TMSG_MEDIA_PAUSE_IF_PLAYING:
      if (PauseMessageApplies(msg, *appPlayer))
      {
        appPower->WakeScreen();
        appPlayer->Pause();
      }
      break;

    case TMSG_MEDIA_SEEK_TIME:
      if (appPlayer->IsPlaying() || appPlayer->IsPaused())
        appPlayer->SeekTime(pMsg->param3);
      break;

    default:
      CLog::LogF(LOGERROR, "Unhandled threadmessage sent, {}", msg);
      break;
  }
}

bool CApplicationMessageHandling::OnMessage(const CGUIMessage& message)
{
  auto& pvrManager{CServiceBroker::GetPVRManager()};
  const auto appPlayer = m_app.GetComponent<CApplicationPlayer>();

  switch (message.GetMessage())
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        CServiceBroker::GetPlayLists()->RemoveDiscItems();
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
      else if (message.GetParam1() == GUI_MSG_WINDOW_RESIZE)
      {
        if (IsSkinReloadNeeded())
          ScheduleSkinReload();
      }
      else if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        if (m_app.CurrentFileItem().IsSamePath(item.get()))
        {
          const bool replaceLabels = !(message.GetParam2() & GUI_MSG_FLAG_KEEP_CURRENT_LABELS);
          m_app.CurrentFileItem().UpdateInfo(*item, replaceLabels);
          CServiceBroker::GetGUI()->GetInfoManager().UpdateCurrentItem(*item, replaceLabels);
        }
      }
    }
    break;

    case GUI_MSG_PLAYBACK_STARTED:
    {
      m_app.m_ServiceManager->GetPlatform().OnPlayingVideoChanged(appPlayer->IsPlayingVideo());
      m_app.ResetPlayerEvent();

      pvrManager.OnPlaybackStarted(m_app.CurrentFileItem());

      if (!message.GetItem())
        return true;

      // the playlists have already recorded what started
      const std::shared_ptr<CFileItem> started = CServiceBroker::GetPlayLists()->GetCurrentItem();
      CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(*started);

#ifdef HAS_PYTHON
      // informs python script currently running playback has started
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnPlayBackStarted(*started);
#endif

      // we don't want a busy dialog when switching channels
      if (!m_app.CurrentFileItem().IsLiveTV() ||
          (!appPlayer->IsPlayingVideo() && !appPlayer->IsPlayingAudio()))
        CGUIDialogBusy::WaitOnEvent(m_app.m_playerEvent);

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

      std::unique_ptr<CFileItem> trailerItem = ContentUtils::GeneratePlayableTrailerItem(
          *item, CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(20410));

      if (PLAYLIST::IsPlayList(*item))
      {
        auto fileitemList{std::make_unique<CFileItemList>()};
        fileitemList->Add(std::move(trailerItem));
        APPLICATION::PostPlayItems(std::move(fileitemList));
      }
      else
      {
        CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MEDIA_PLAY_ITEM, 1, 0,
                                                   static_cast<void*>(trailerItem.release()));
      }
      break;
    }

    case GUI_MSG_PLAYBACK_STOPPED:
    {
      ScheduleSkinReloadAfterPlayback();

      pvrManager.OnPlaybackStopped(m_app.CurrentFileItem());
      CServiceBroker::GetFavouritesService().OnPlaybackStopped(m_app.CurrentFileItem());

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
      ScheduleSkinReloadAfterPlayback();

      pvrManager.OnPlaybackEnded(m_app.CurrentFileItem());
      CServiceBroker::GetFavouritesService().OnPlaybackEnded(m_app.CurrentFileItem());

      m_app.m_playerEvent.Set();

      if (const auto stackHelper{m_app.GetComponent<CApplicationStackHelper>()};
          stackHelper->IsPlayingStack() && stackHelper->HasNextStackPartFileItem())
      {
        // If current stack part finished then play the next part
        if (stackHelper->IsCurrentPartFinished())
        {
          if (m_app.PlayFile(stackHelper->SetNextStackPartAsCurrent(), "",
                             CApplication::Reopen::Yes,
                             CApplication::StartsRun::No) != CApplication::PlayResult::Cancelled)
            return true;

          // Selection of next part playlist cancelled so create bookmark for next part
          stackHelper->SetNextPartBookmark();
        }
      }

      const CFileItem ended{m_app.CurrentFileItem()};

      const CPlaycountIncrementedHandler playCountIncrementedHandler{ended};

      m_app.ResetCurrentItem();

      if (CServiceBroker::GetPlayLists()->OnEntryEnded(ended))
        m_app.PlaybackCleanup();

#ifdef HAS_PYTHON
      CServiceBroker::GetXBPython().OnPlayBackEnded();
#endif

      playCountIncrementedHandler.HandlePlaycountIncremented();
      return true;
    }

    case GUI_MSG_PLAYLISTPLAYER_STOPPED:
      // a stop announces the current item, so it is reset once the stop has been handled
      if (appPlayer->IsPlaying())
        m_app.StopPlaying();
      else
        m_app.ResetCurrentItem();
      m_app.PlaybackCleanup();
      return true;

    case GUI_MSG_PLAYBACK_AVSTARTED:
    {
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
      // informs python scripts that the audio or video streams changed
      // (does nothing if python is not loaded)
      CServiceBroker::GetXBPython().OnAVChange();
#endif
      return true;
    }

    case GUI_MSG_PLAYBACK_SEEKED:
    {
      CDataCacheCore::GetInstance().SeekFinished(static_cast<int>(message.GetParam2AsI64()));

      return true;
    }

    case GUI_MSG_PLAYBACK_ERROR:
    {
      // The slideshow skips a video that won't play
      if (!CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_SLIDESHOW))
        MESSAGING::HELPERS::ShowOKDialogText(CVariant{16026}, CVariant{16027});
      return true;
    }

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
