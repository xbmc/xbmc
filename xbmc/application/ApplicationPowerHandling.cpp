/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPowerHandling.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/MusicLibraryQueue.h"
#include "powermanagement/DPMSSupport.h"
#include "powermanagement/PowerTypes.h"
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/AlarmClock.h"
#include "utils/log.h"
#include "video/VideoLibraryQueue.h"
#include "windowing/WinSystem.h"

void CApplicationPowerHandling::ResetScreenSaver()
{
  // reset our timers
  m_shutdownTimer.StartZero();

  // screen saver timer is reset only if we're not already in screensaver or
  // DPMS mode
  if ((!m_screensaverActive && m_iScreenSaveLock == 0) && !m_dpmsIsActive)
    ResetScreenSaverTimer();
}

void CApplicationPowerHandling::ResetScreenSaverTimer()
{
  m_screenSaverTimer.StartZero();
}

void CApplicationPowerHandling::ResetSystemIdleTimer()
{
  // reset system idle timer
  m_idleTimer.StartZero();
}

void CApplicationPowerHandling::ResetNavigationTimer()
{
  m_navigationTimer.StartZero();
}

void CApplicationPowerHandling::SetRenderGUI(bool renderGUI)
{
  m_renderGUI = renderGUI;
}

void CApplicationPowerHandling::StopScreenSaverTimer()
{
  m_screenSaverTimer.Stop();
}

bool CApplicationPowerHandling::ToggleDPMS(bool manual)
{
  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return false;

  std::shared_ptr<CDPMSSupport> dpms = winSystem->GetDPMSManager();
  if (!dpms)
    return false;

  if (manual || (m_dpmsIsManual == manual))
  {
    if (m_dpmsIsActive)
    {
      m_dpmsIsActive = false;
      m_dpmsIsManual = false;
      SetRenderGUI(true);
      CheckOSScreenSaverInhibitionSetting();
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "OnDPMSDeactivated");
      return dpms->DisablePowerSaving();
    }
    else
    {
      if (dpms->EnablePowerSaving(dpms->GetSupportedModes()[0]))
      {
        m_dpmsIsActive = true;
        m_dpmsIsManual = manual;
        SetRenderGUI(false);
        CheckOSScreenSaverInhibitionSetting();
        CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "OnDPMSActivated");
        return true;
      }
    }
  }
  return false;
}

bool CApplicationPowerHandling::WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed /* = false */)
{
  bool result = false;

  // First reset DPMS, if active
  if (m_dpmsIsActive)
  {
    if (m_dpmsIsManual)
      return false;
    //! @todo if screensaver lock is specified but screensaver is not active
    //! (DPMS came first), activate screensaver now.
    ToggleDPMS(false);
    ResetScreenSaverTimer();
    result = !m_screensaverActive || WakeUpScreenSaver(bPowerOffKeyPressed);
  }
  else if (m_screensaverActive)
    result = WakeUpScreenSaver(bPowerOffKeyPressed);

  if (result)
  {
    // allow listeners to ignore the deactivation if it precedes a powerdown/suspend etc
    CVariant data(CVariant::VariantTypeObject);
    data["shuttingdown"] = bPowerOffKeyPressed;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI,
                                                       "OnScreensaverDeactivated", data);
  }

  return result;
}

bool CApplicationPowerHandling::WakeUpScreenSaver(bool bPowerOffKeyPressed /* = false */)
{
  if (m_iScreenSaveLock == 2)
    return false;

  // if Screen saver is active
  if (m_screensaverActive && !m_screensaverIdInUse.empty())
  {
    if (m_iScreenSaveLock == 0)
    {
      const std::shared_ptr<CProfileManager> profileManager =
          CServiceBroker::GetSettingsComponent()->GetProfileManager();
      if (profileManager->GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          (profileManager->UsingLoginScreen() ||
           CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
               CSettings::SETTING_MASTERLOCK_STARTUPLOCK)) &&
          profileManager->GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
          m_screensaverIdInUse != "screensaver.xbmc.builtin.dim" &&
          m_screensaverIdInUse != "screensaver.xbmc.builtin.black" &&
          m_screensaverIdInUse != "visualization")
      {
        m_iScreenSaveLock = 2;
        CGUIMessage msg(GUI_MSG_CHECK_LOCK, 0, 0);

        CGUIWindow* pWindow =
            CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_SCREENSAVER);
        if (pWindow)
          pWindow->OnMessage(msg);
      }
    }
    if (m_iScreenSaveLock == -1)
    {
      m_iScreenSaveLock = 0;
      return true;
    }

    // disable screensaver
    m_screensaverActive = false;
    m_iScreenSaveLock = 0;
    ResetScreenSaverTimer();

    if (m_screensaverIdInUse == "visualization")
    {
      // we can just continue as usual from vis mode
      return false;
    }
    else if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
             m_screensaverIdInUse == "screensaver.xbmc.builtin.black" ||
             m_screensaverIdInUse.empty())
    {
      return true;
    }
    else
    { // we're in screensaver window
      if (m_pythonScreenSaver)
      {
// What sound does a python screensaver make?
#define SCRIPT_ALARM "sssssscreensaver"
#define SCRIPT_TIMEOUT 15 // seconds

        /* FIXME: This is a hack but a proper fix is non-trivial. Basically this code
        * makes sure the addon gets terminated after we've moved out of the screensaver window.
        * If we don't do this, we may simply lockup.
        */
        g_alarmClock.Start(SCRIPT_ALARM, SCRIPT_TIMEOUT,
                           "StopScript(" + m_pythonScreenSaver->LibPath() + ")", true, false);
        m_pythonScreenSaver.reset();
      }
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SCREENSAVER)
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow(); // show the previous window
      else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW)
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
                                                   static_cast<void*>(new CAction(ACTION_STOP)));
    }
    return true;
  }
  else
    return false;
}

void CApplicationPowerHandling::CheckOSScreenSaverInhibitionSetting()
{
  // Kodi screen saver overrides OS one: always inhibit OS screen saver then
  // except when DPMS is active (inhibiting the screen saver then might also
  // disable DPMS again)
  if (!m_dpmsIsActive &&
      !CServiceBroker::GetSettingsComponent()
           ->GetSettings()
           ->GetString(CSettings::SETTING_SCREENSAVER_MODE)
           .empty() &&
      CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    if (!m_globalScreensaverInhibitor)
    {
      m_globalScreensaverInhibitor =
          CServiceBroker::GetWinSystem()->GetOSScreenSaver()->CreateInhibitor();
    }
  }
  else if (m_globalScreensaverInhibitor)
  {
    m_globalScreensaverInhibitor.Release();
  }
}

void CApplicationPowerHandling::CheckScreenSaverAndDPMS()
{
  bool maybeScreensaver = true;
  if (m_dpmsIsActive)
    maybeScreensaver = false;
  else if (m_screensaverActive)
    maybeScreensaver = false;
  else if (CServiceBroker::GetSettingsComponent()
               ->GetSettings()
               ->GetString(CSettings::SETTING_SCREENSAVER_MODE)
               .empty())
    maybeScreensaver = false;

  auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  std::shared_ptr<CDPMSSupport> dpms = winSystem->GetDPMSManager();

  bool maybeDPMS = true;
  if (m_dpmsIsActive)
    maybeDPMS = false;
  else if (!dpms || !dpms->IsSupported())
    maybeDPMS = false;
  else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
               CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) <= 0)
    maybeDPMS = false;

  // whether the current state of the application should be regarded as active even when there is no
  // explicit user activity such as input
  bool haveIdleActivity = false;

  if (m_bResetScreenSaver)
  {
    m_bResetScreenSaver = false;
    haveIdleActivity = true;
  }

  // When inhibit screensaver is enabled prevent screensaver from kicking in
  if (m_bInhibitScreenSaver)
    haveIdleActivity = true;

  // Are we playing a video and it is not paused?
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer && appPlayer->IsPlayingVideo() && !appPlayer->IsPaused())
    haveIdleActivity = true;

  // Are we playing audio and screensaver is disabled globally for audio?
  else if (appPlayer && appPlayer->IsPlayingAudio() &&
           CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
               CSettings::SETTING_SCREENSAVER_DISABLEFORAUDIO))
  {
    haveIdleActivity = true;
  }

  // Handle OS screen saver state
  if (haveIdleActivity && CServiceBroker::GetWinSystem()->GetOSScreenSaver())
  {
    // Always inhibit OS screen saver during these kinds of activities
    if (!m_screensaverInhibitor)
    {
      m_screensaverInhibitor =
          CServiceBroker::GetWinSystem()->GetOSScreenSaver()->CreateInhibitor();
    }
  }
  else if (m_screensaverInhibitor)
  {
    m_screensaverInhibitor.Release();
  }

  // Has the screen saver window become active?
  if (maybeScreensaver &&
      CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_SCREENSAVER))
  {
    m_screensaverActive = true;
    maybeScreensaver = false;
  }

  if (m_screensaverActive && haveIdleActivity)
  {
    WakeUpScreenSaverAndDPMS();
    return;
  }

  if (!maybeScreensaver && !maybeDPMS)
    return; // Nothing to do.

  // See if we need to reset timer.
  if (haveIdleActivity)
  {
    ResetScreenSaverTimer();
    return;
  }

  float elapsed = m_screenSaverTimer.IsRunning() ? m_screenSaverTimer.GetElapsedSeconds() : 0.f;

  // DPMS has priority (it makes the screensaver not needed)
  if (maybeDPMS && elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                                 CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF) *
                                 60)
  {
    ToggleDPMS(false);
    WakeUpScreenSaver();
  }
  else if (maybeScreensaver &&
           elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                         CSettings::SETTING_SCREENSAVER_TIME) *
                         60)
  {
    ActivateScreenSaver();
  }
}

// activate the screensaver.
// if forceType is true, we ignore the various conditions that can alter
// the type of screensaver displayed
void CApplicationPowerHandling::ActivateScreenSaver(bool forceType /*= false */)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  m_screensaverActive = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "OnScreensaverActivated");

  // disable screensaver lock from the login screen
  m_iScreenSaveLock =
      CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_LOGIN_SCREEN ? 1 : 0;

  m_screensaverIdInUse = settings->GetString(CSettings::SETTING_SCREENSAVER_MODE);

  if (!forceType)
  {
    if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
        m_screensaverIdInUse == "screensaver.xbmc.builtin.black" || m_screensaverIdInUse.empty())
    {
      return;
    }

    // Enforce Dim for special cases.
    bool bUseDim = false;
    if (appPlayer && appPlayer->IsPlayingVideo() &&
        settings->GetBool(CSettings::SETTING_SCREENSAVER_USEDIMONPAUSE))
      bUseDim = true;
    else if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().IsRunningChannelScan())
      bUseDim = true;

    if (bUseDim)
      m_screensaverIdInUse = "screensaver.xbmc.builtin.dim";
  }

  if (m_screensaverIdInUse == "screensaver.xbmc.builtin.dim" ||
      m_screensaverIdInUse == "screensaver.xbmc.builtin.black" || m_screensaverIdInUse.empty())
  {
    return;
  }
  else if (CServiceBroker::GetAddonMgr().GetAddon(m_screensaverIdInUse, m_pythonScreenSaver,
                                                  ADDON::AddonType::SCREENSAVER,
                                                  ADDON::OnlyEnabled::CHOICE_YES))
  {
    std::string libPath = m_pythonScreenSaver->LibPath();
    if (CScriptInvocationManager::GetInstance().HasLanguageInvoker(libPath))
    {
      CLog::Log(LOGDEBUG, "using python screensaver add-on {}", m_screensaverIdInUse);

      // Don't allow a previously-scheduled alarm to kill our new screensaver
      g_alarmClock.Stop(SCRIPT_ALARM, true);

      if (!CScriptInvocationManager::GetInstance().Stop(libPath))
        CScriptInvocationManager::GetInstance().ExecuteAsync(
            libPath,
            ADDON::AddonPtr(new ADDON::CAddon(dynamic_cast<ADDON::CAddon&>(*m_pythonScreenSaver))));
      return;
    }
    m_pythonScreenSaver.reset();
  }

  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SCREENSAVER);
}

void CApplicationPowerHandling::InhibitScreenSaver(bool inhibit)
{
  m_bInhibitScreenSaver = inhibit;
}

bool CApplicationPowerHandling::IsScreenSaverInhibited() const
{
  return m_bInhibitScreenSaver;
}

// Global Idle Time in Seconds
// idle time will be reset if on any OnKey()
// int return: system Idle time in seconds! 0 is no idle!
int CApplicationPowerHandling::GlobalIdleTime()
{
  if (!m_idleTimer.IsRunning())
    m_idleTimer.StartZero();
  return (int)m_idleTimer.GetElapsedSeconds();
}

float CApplicationPowerHandling::NavigationIdleTime()
{
  if (!m_navigationTimer.IsRunning())
    m_navigationTimer.StartZero();
  return m_navigationTimer.GetElapsedSeconds();
}

void CApplicationPowerHandling::StopShutdownTimer()
{
  m_shutdownTimer.Stop();
}

void CApplicationPowerHandling::ResetShutdownTimers()
{
  // reset system shutdown timer
  m_shutdownTimer.StartZero();

  // delete custom shutdown timer
  if (g_alarmClock.HasAlarm("shutdowntimer"))
    g_alarmClock.Stop("shutdowntimer", true);
}

void CApplicationPowerHandling::HandleShutdownMessage()
{
  switch (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
  {
    case POWERSTATE_SHUTDOWN:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_POWERDOWN);
      break;

    case POWERSTATE_SUSPEND:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SUSPEND);
      break;

    case POWERSTATE_HIBERNATE:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_HIBERNATE);
      break;

    case POWERSTATE_QUIT:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
      break;

    case POWERSTATE_MINIMIZE:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_MINIMIZE);
      break;

    default:
      CLog::Log(LOGERROR, "{}: No valid shutdownstate matched", __FUNCTION__);
      break;
  }
}

void CApplicationPowerHandling::CheckShutdown()
{
  // first check if we should reset the timer
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer)
    return;

  if (m_bInhibitIdleShutdown || appPlayer->IsPlaying() ||
      appPlayer->IsPausedPlayback() // is something playing?
      || CMusicLibraryQueue::GetInstance().IsRunning() ||
      CVideoLibraryQueue::GetInstance().IsRunning() ||
      CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(
          WINDOW_DIALOG_PROGRESS) // progress dialog is onscreen
      ||
      !CServiceBroker::GetPVRManager().Get<PVR::GUI::PowerManagement>().CanSystemPowerdown(false))
  {
    m_shutdownTimer.StartZero();
    return;
  }

  float elapsed = m_shutdownTimer.IsRunning() ? m_shutdownTimer.GetElapsedSeconds() : 0.f;
  if (elapsed > CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                    CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNTIME) *
                    60)
  {
    // Since it is a sleep instead of a shutdown, let's set everything to reset when we wake up.
    m_shutdownTimer.Stop();

    // Sleep the box
    CLog::LogF(LOGDEBUG, "Timer is over (shutdown function: {})",
               CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                   CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE));
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_SHUTDOWN);
  }
}

void CApplicationPowerHandling::InhibitIdleShutdown(bool inhibit)
{
  m_bInhibitIdleShutdown = inhibit;
}

bool CApplicationPowerHandling::IsIdleShutdownInhibited() const
{
  return m_bInhibitIdleShutdown;
}

bool CApplicationPowerHandling::OnSettingChanged(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (settingId == CSettings::SETTING_SCREENSAVER_MODE)
  {
    CheckOSScreenSaverInhibitionSetting();
  }
  else
    return false;

  return true;
}

bool CApplicationPowerHandling::OnSettingAction(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (settingId == CSettings::SETTING_SCREENSAVER_PREVIEW)
    ActivateScreenSaver(true);
  else if (settingId == CSettings::SETTING_SCREENSAVER_SETTINGS)
  {
    ADDON::AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
                CSettings::SETTING_SCREENSAVER_MODE),
            addon, ADDON::AddonType::SCREENSAVER, ADDON::OnlyEnabled::CHOICE_YES))
      CGUIDialogAddonSettings::ShowForAddon(addon);
  }
  else
    return false;

  return true;
}
