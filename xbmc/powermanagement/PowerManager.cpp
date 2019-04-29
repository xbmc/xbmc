/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PowerManager.h"

#include <list>
#include <memory>

#include "PowerTypes.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/builtins/Builtins.h"
#include "network/Network.h"
#include "pvr/PVRManager.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "weather/WeatherManager.h"
#include "windowing/WinSystem.h"

#if defined(TARGET_WINDOWS_DESKTOP)
extern HWND g_hWnd;
#endif

CPowerManager::CPowerManager()
{
  m_settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  m_settings->GetSettingsManager()->RegisterSettingOptionsFiller("shutdownstates", SettingOptionsShutdownStatesFiller);
}

CPowerManager::~CPowerManager() = default;

void CPowerManager::Initialize()
{
  m_instance.reset(IPowerSyscall::CreateInstance());
}

void CPowerManager::SetDefaults()
{
  int defaultShutdown = m_settings->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);

  switch (defaultShutdown)
  {
    case POWERSTATE_QUIT:
    case POWERSTATE_MINIMIZE:
      // assume we can shutdown if --standalone is passed
      if (g_application.IsStandAlone())
        defaultShutdown = POWERSTATE_SHUTDOWN;
    break;
    case POWERSTATE_HIBERNATE:
      if (!CServiceBroker::GetPowerManager().CanHibernate())
      {
        if (CServiceBroker::GetPowerManager().CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = CServiceBroker::GetPowerManager().CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SUSPEND:
      if (!CServiceBroker::GetPowerManager().CanSuspend())
      {
        if (CServiceBroker::GetPowerManager().CanHibernate())
          defaultShutdown = POWERSTATE_HIBERNATE;
        else
          defaultShutdown = CServiceBroker::GetPowerManager().CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SHUTDOWN:
      if (!CServiceBroker::GetPowerManager().CanPowerdown())
      {
        if (CServiceBroker::GetPowerManager().CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = CServiceBroker::GetPowerManager().CanHibernate() ? POWERSTATE_HIBERNATE : POWERSTATE_QUIT;
      }
    break;
  }

  std::static_pointer_cast<CSettingInt>(m_settings->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))->SetDefault(defaultShutdown);
}

bool CPowerManager::Powerdown()
{
  if (CanPowerdown() && m_instance->Powerdown())
  {
    CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Open();

    return true;
  }

  return false;
}

bool CPowerManager::Suspend()
{
  return (CanSuspend() && m_instance->Suspend());
}

bool CPowerManager::Hibernate()
{
  return (CanHibernate() && m_instance->Hibernate());
}

bool CPowerManager::Reboot()
{
  bool success = CanReboot() ? m_instance->Reboot() : false;

  if (success)
  {
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnRestart");

    CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Open();
  }

  return success;
}

bool CPowerManager::CanPowerdown()
{
  return m_instance ? m_instance->CanPowerdown() : false;
}
bool CPowerManager::CanSuspend()
{
  return m_instance ? m_instance->CanSuspend() : false;
}
bool CPowerManager::CanHibernate()
{
  return m_instance ? m_instance->CanHibernate() : false;
}
bool CPowerManager::CanReboot()
{
  return m_instance ? m_instance->CanReboot() : false;
}
int CPowerManager::BatteryLevel()
{
  return m_instance ? m_instance->BatteryLevel() : 0;
}
void CPowerManager::ProcessEvents()
{
  if (!m_instance)
    return;

  static int nesting = 0;

  if (++nesting == 1)
    m_instance->PumpPowerEvents(this);

  nesting--;
}

void CPowerManager::OnSleep()
{
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnSleep");

  CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
  if (dialog)
    dialog->Open();

  CLog::Log(LOGNOTICE, "%s: Running sleep jobs", __FUNCTION__);

  CServiceBroker::GetPVRManager().OnSleep();
  StorePlayerState();
  g_application.StopPlaying();
  g_application.StopShutdownTimer();
  g_application.StopScreenSaverTimer();
  g_application.CloseNetworkShares();
  CServiceBroker::GetActiveAE()->Suspend();
}

void CPowerManager::OnWake()
{
  CLog::Log(LOGNOTICE, "%s: Running resume jobs", __FUNCTION__);

  CServiceBroker::GetNetwork().WaitForNet();

  // reset out timers
  g_application.ResetShutdownTimers();

  CGUIDialogBusy* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusy>(WINDOW_DIALOG_BUSY);
  if (dialog)
    dialog->Close(true); // force close. no closing animation, sound etc at this early stage

#if defined(HAS_SDL) || defined(TARGET_WINDOWS)
  if (CServiceBroker::GetWinSystem()->IsFullScreen())
  {
#if defined(TARGET_WINDOWS_DESKTOP)
    ShowWindow(g_hWnd, SW_RESTORE);
    SetForegroundWindow(g_hWnd);
#endif
  }
  g_application.ResetScreenSaver();
#endif

  CServiceBroker::GetActiveAE()->Resume();
  g_application.UpdateLibraries();
  CServiceBroker::GetWeatherManager().Refresh();
  CServiceBroker::GetPVRManager().OnWake();
  RestorePlayerState();

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnWake");
}

void CPowerManager::OnLowBattery()
{
  CLog::Log(LOGNOTICE, "%s: Running low battery jobs", __FUNCTION__);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13050), "");

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "xbmc", "OnLowBattery");
}

void CPowerManager::StorePlayerState()
{
  CApplicationPlayer &appPlayer = g_application.GetAppPlayer();
  if (appPlayer.IsPlaying())
  {
    m_lastUsedPlayer = appPlayer.GetCurrentPlayer();
    m_lastPlayedFileItem.reset(new CFileItem(g_application.CurrentFileItem()));
    // set the actual offset instead of store and load it from database
    m_lastPlayedFileItem->m_lStartOffset = appPlayer.GetTime();
    // in case of regular stack, correct the start offset by adding current part start time
    if (g_application.GetAppStackHelper().IsPlayingRegularStack())
      m_lastPlayedFileItem->m_lStartOffset += g_application.GetAppStackHelper().GetCurrentStackPartStartTimeMs();
    // in case of iso stack, keep track of part number
    m_lastPlayedFileItem->m_lStartPartNumber = g_application.GetAppStackHelper().IsPlayingISOStack() ? g_application.GetAppStackHelper().GetCurrentPartNumber() + 1 : 1;
    // for iso and iso stacks, keep track of playerstate
    m_lastPlayedFileItem->SetProperty("savedplayerstate", appPlayer.GetPlayerState());
    CLog::Log(LOGDEBUG, "CPowerManager::StorePlayerState - store last played item (startOffset: %i ms)", m_lastPlayedFileItem->m_lStartOffset);
  }
  else
  {
    m_lastUsedPlayer.clear();
    m_lastPlayedFileItem.reset();
  }
}

void CPowerManager::RestorePlayerState()
{
  if (!m_lastPlayedFileItem)
    return;

  CLog::Log(LOGDEBUG, "CPowerManager::RestorePlayerState - resume last played item (startOffset: %i ms)", m_lastPlayedFileItem->m_lStartOffset);
  g_application.PlayFile(*m_lastPlayedFileItem, m_lastUsedPlayer);
}

void CPowerManager::SettingOptionsShutdownStatesFiller(SettingConstPtr setting, std::vector<IntegerSettingOption> &list, int &current, void *data)
{
  if (CServiceBroker::GetPowerManager().CanPowerdown())
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN));
  if (CServiceBroker::GetPowerManager().CanHibernate())
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE));
  if (CServiceBroker::GetPowerManager().CanSuspend())
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND));
  if (!g_application.IsStandAlone())
  {
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(13009), POWERSTATE_QUIT));
#if !defined(TARGET_DARWIN_EMBEDDED)
    list.push_back(IntegerSettingOption(g_localizeStrings.Get(13014), POWERSTATE_MINIMIZE));
#endif
  }
}
