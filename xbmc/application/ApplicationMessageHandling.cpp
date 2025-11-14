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
#include "ServiceBroker.h"
#include "ServiceManager.h"
#include "Util.h"
#include "application/AppInboundProtocol.h"
#include "application/Application.h"
#include "application/ApplicationEnums.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "filesystem/IDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/ThreadMessage.h"
#include "network/Network.h"
#include "pictures/SlideShowDelegator.h"
#include "powermanagement/PowerManager.h"
#include "profiles/Profile.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileExtensionProvider.h"
#include "utils/URIUtils.h"
#ifdef TARGET_ANDROID
#include "platform/android/activity/XBMCApp.h"
#endif
#ifdef TARGET_WINDOWS
#include "platform/win32/WIN32Util.h"
#endif

using namespace KODI;

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
      CGUIComponent* gui = CServiceBroker::GetGUI();
      if (gui)
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
      CServiceBroker::GetWinSystem()->MoveToScreen(static_cast<int>(pMsg->param1));
      break;

    case TMSG_MINIMIZE:
      CServiceBroker::GetWinSystem()->Minimize();
      break;

    case TMSG_EXECUTE_OS:
      // Suspend AE temporarily so exclusive or hog-mode sinks
      // don't block external player's access to audio device
      IAE* audioengine;
      audioengine = CServiceBroker::GetActiveAE();
      if (audioengine)
      {
        if (!audioengine->Suspend())
        {
          CLog::Log(LOGINFO, "{}: Failed to suspend AudioEngine before launching external program",
                    __FUNCTION__);
        }
      }
#if defined(TARGET_DARWIN)
      CLog::Log(LOGINFO, "ExecWait is not implemented on this platform");
#elif defined(TARGET_POSIX)
      CUtil::RunCommandLine(pMsg->strParam, (pMsg->param1 == 1));
#elif defined(TARGET_WINDOWS)
      CWIN32Util::XBMCShellExecute(pMsg->strParam, (pMsg->param1 == 1));
#endif
      // Resume AE processing of XBMC native audio
      if (audioengine)
      {
        if (!audioengine->Resume())
        {
          CLog::Log(LOGFATAL, "{}: Failed to restart AudioEngine after return from external player",
                    __FUNCTION__);
        }
      }
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
        if (items.Size() > 0)
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

      if (items.Size() > 0)
      {
        for (const auto& item : items)
          slideShow.Add(item.get());
        slideShow.StartSlideShow(); //Start the slideshow!
      }

      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() != WINDOW_SLIDESHOW)
      {
        if (items.Size() == 0)
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
        XBMC_Event* event = static_cast<XBMC_Event*>(pMsg->lpVoid);
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
      const float volumedB = static_cast<float>(pMsg->param3);
      m_app.GetComponent<CApplicationVolumeHandling>()->SetVolume(volumedB);
    }
    break;

    case TMSG_SET_MUTE:
    {
      m_app.GetComponent<CApplicationVolumeHandling>()->SetMute(pMsg->param3 == 1 ? true : false);
    }
    break;

    default:
      CLog::Log(LOGERROR, "{}: Unhandled threadmessage sent, {}", __FUNCTION__, msg);
      break;
  }
}
