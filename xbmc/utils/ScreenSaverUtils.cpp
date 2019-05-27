/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenSaverUtils.h"

#include "addons/AddonInfo.h"
#include "addons/AddonManager.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "Application.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/ApplicationMessenger.h"
#include "powermanagement/DPMSSupport.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/AlarmClock.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

namespace
{

// What sound does a python screensaver make?
const char* SCRIPT_ALARM{"sssssscreensaver"};
const int SCRIPT_TIMEOUT_SECONDS{15};

}

using namespace KODI::MESSAGING;
using namespace UTILS;

CScreenSaverUtils::CScreenSaverUtils() : CThread("screensaver")
{
  CApplicationMessenger::GetInstance().RegisterReceiver(this);

  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_SCREENSAVER_MODE);
  settingSet.insert(CSettings::SETTING_SCREENSAVER_PREVIEW);
  settingSet.insert(CSettings::SETTING_SCREENSAVER_SETTINGS);

  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, settingSet);

  Create();
  SetPriority(GetMinPriority());

  auto settingsComponent = CServiceBroker::GetSettingsComponent();

  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();

  if (!settings)
    return;

  auto setting = std::static_pointer_cast<CSettingString>(settings->GetSetting(CSettings::SETTING_SCREENSAVER_MODE));

  if (CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    setting->SetDefault("");
  }
  else
  {
    setting->SetDefault("screensaver.xbmc.builtin.dim");
  }
}

CScreenSaverUtils::~CScreenSaverUtils()
{
  StopThread();

  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);

  CApplicationMessenger::GetInstance().PostMsg(TMSG_DEACTIVATESCREENSAVER);

  m_osScreenSaverInhibitor.Release();
}

void CScreenSaverUtils::Process()
{
  while (!m_bStop)
  {
    auto winSystem = CServiceBroker::GetWinSystem();

    std::shared_ptr<CDPMSSupport> dpms;
    if (winSystem)
      dpms = winSystem->GetDPMSManager();

    if (g_application.GetAppPlayer().IsPlayingVideo() &&
        !g_application.GetAppPlayer().IsPaused())
    {
      ResetTimer();
    }
    else if (g_application.GetAppPlayer().IsPlayingAudio() &&
             CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION &&
             !CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION).empty())
    {
      ResetTimer();
    }
    else if (!m_screenSaverActive || (dpms && !dpms->IsActive()))
    {
      auto settingsComponent = CServiceBroker::GetSettingsComponent();
      if (settingsComponent)
      {
        auto settings = settingsComponent->GetSettings();
        if (settings)
        {
          int screenSaverSeconds = settings->GetInt(CSettings::SETTING_SCREENSAVER_TIME) * 60;
          int dpmsSeconds = settings->GetInt(CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) * 60;

          int elapsed = m_screenSaverTimer.IsRunning() ? m_screenSaverTimer.GetElapsedSeconds() : 0;

          if (elapsed > screenSaverSeconds && !m_screenSaverActive)
          {
            CApplicationMessenger::GetInstance().PostMsg(TMSG_ACTIVATESCREENSAVER);
          }

          if (dpms)
          {
            if (elapsed > dpmsSeconds && !dpms->IsActive())
            {
              dpms->Activate();
            }
          }
        }
      }
    }

    Sleep(1000);
  }
}

void CScreenSaverUtils::ResetTimer()
{
  m_screenSaverTimer.StartZero();
}

void CScreenSaverUtils::StopTimer()
{
  m_screenSaverTimer.Stop();
}

void CScreenSaverUtils::Deactivate()
{
  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  auto dpms = winSystem->GetDPMSManager();
  if (dpms)
    dpms->Deactivate();

  if (!m_screenSaverActive)
    return;

  if (m_timeSinceActivation.GetElapsedSeconds() <= 1)
    return;

  m_timeSinceActivation.Stop();
  ResetTimer();
  m_screenSaverActive = false;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnScreensaverDeactivated");

  auto settingsComponent = CServiceBroker::GetSettingsComponent();

  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();

  if (!settings)
    return;

  std::string screensaver = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

  if (screensaver == "screensaver.xbmc.builtin.dim" ||
      screensaver == "screensaver.xbmc.builtin.black" ||
      screensaver.empty())
  {
    CServiceBroker::GetGUI()->GetWindowManager().CloseDialogs();
    return;
  }
  else
  {
    if (m_pythonScreenSaver)
    {
      /* FIXME: This is a hack but a proper fix is non-trivial. Basically this code
      * makes sure the addon gets terminated after we've moved out of the screensaver window.
      * If we don't do this, we may simply lockup.
      */
      g_alarmClock.Start(SCRIPT_ALARM, SCRIPT_TIMEOUT_SECONDS, "StopScript(" + m_pythonScreenSaver->LibPath() + ")", true, false);
      m_pythonScreenSaver.reset();
    }

    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SCREENSAVER)
    {
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();  // show the previous window
    }
  }
}

void CScreenSaverUtils::Activate(bool force)
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();

  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();

  if (!settings)
    return;

  if (g_application.GetAppPlayer().IsPlayingAudio() && settings->GetBool(CSettings::SETTING_SCREENSAVER_USEMUSICVISINSTEAD))
  {
    std::string visualisation = settings->GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION);

    if (!visualisation.empty())
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VISUALISATION);
      return;
    }
  }

  m_timeSinceActivation.StartZero();
  m_screenSaverActive = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnScreensaverActivated");

  bool useDim = false;
  if (!force)
  {
    if (CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true) ||
        (g_application.GetAppPlayer().IsPlayingVideo() && settings->GetBool(CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE)) ||
        CServiceBroker::GetPVRManager().GUIActions()->IsRunningChannelScan())
    {
      useDim = true;
    }
  }

  std::string screensaver = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

  if (useDim || screensaver.empty())
  {
    screensaver = "screensaver.xbmc.builtin.dim";
  }
  else
  {
    screensaver = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);
  }

  if (screensaver == "screensaver.xbmc.builtin.dim" ||
      screensaver == "screensaver.xbmc.builtin.black")
  {
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREENSAVER_DIM);
    return;
  }

  if (CServiceBroker::GetAddonMgr().GetAddon(screensaver, m_pythonScreenSaver, ADDON::ADDON_SCREENSAVER))
  {
    std::string libPath = m_pythonScreenSaver->LibPath();
    if (CScriptInvocationManager::GetInstance().HasLanguageInvoker(libPath))
    {
      CLog::Log(LOGDEBUG, "[SCREENSAVER] using python screensaver add-on {}", screensaver);

      // Don't allow a previously-scheduled alarm to kill our new screensaver
      g_alarmClock.Stop(SCRIPT_ALARM, true);

      if (!CScriptInvocationManager::GetInstance().Stop(libPath))
      {
        CScriptInvocationManager::GetInstance().ExecuteAsync(libPath, m_pythonScreenSaver);
      }

      return;
    }

    m_pythonScreenSaver.reset();
  }

  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREENSAVER);
}

void CScreenSaverUtils::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (!setting)
    return;

  const std::string &settingId = setting->GetId();

  auto settingsComponent = CServiceBroker::GetSettingsComponent();

  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();

  if (settingId == CSettings::SETTING_SCREENSAVER_MODE)
  {
    std::string screensaver = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

    if (screensaver.empty())
    {
      m_osScreenSaverInhibitor.Release();
    }
    else
    {
      auto winSystem = CServiceBroker::GetWinSystem();
      if (winSystem)
      {
        auto osScreenSaver = winSystem->GetOSScreenSaver();
        if (osScreenSaver)
        {
          m_osScreenSaverInhibitor = osScreenSaver->CreateInhibitor();
        }
      }
    }
  }
}

void CScreenSaverUtils::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (!setting)
    return;

  const std::string &settingId = setting->GetId();

  auto settingsComponent = CServiceBroker::GetSettingsComponent();

  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();

  if (!settings)
    return;

  if (settingId == CSettings::SETTING_SCREENSAVER_PREVIEW)
  {
    Activate(true);
  }
  else if (settingId == CSettings::SETTING_SCREENSAVER_SETTINGS)
  {
    ADDON::AddonPtr addon;
    std::string screensaver = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

    if (CServiceBroker::GetAddonMgr().GetAddon(screensaver, addon, ADDON::ADDON_SCREENSAVER))
    {
      CGUIDialogAddonSettings::ShowForAddon(addon);
    }
  }
}

int CScreenSaverUtils::GetMessageMask()
{
  return TMSG_MASK_SCREENSAVER;
}

void CScreenSaverUtils::OnApplicationMessage(ThreadMessage* pMsg)
{
  uint32_t msg = pMsg->dwMessage;

  switch(msg)
  {
  case TMSG_ACTIVATESCREENSAVER:
  {
    Activate(true);
    break;
  }
  case TMSG_DEACTIVATESCREENSAVER:
  {
    Deactivate();
    break;
  }
  case TMSG_STOPSCREENSAVERTIMER:
  {
    StopTimer();
    break;
  }
  case TMSG_RESETSCREENSAVERTIMER:
  {
    ResetTimer();
    break;
  }
  }
}

