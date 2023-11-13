/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PowerManager.h"

#include "FileItem.h"
#include "PowerTypes.h"
#include "ServiceBroker.h"
#include "application/AppParams.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationStackHelper.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "dialogs/GUIDialogBusyNoCancel.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "network/Network.h"
#include "pvr/PVRManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/lib/SettingsManager.h"
#include "utils/log.h"
#include "weather/WeatherManager.h"

#include <list>
#include <memory>

#if defined(TARGET_WINDOWS_DESKTOP)
extern HWND g_hWnd;
#endif

CPowerManager::CPowerManager() : m_settings(CServiceBroker::GetSettingsComponent()->GetSettings())
{
  m_settings->GetSettingsManager()->RegisterSettingOptionsFiller("shutdownstates", SettingOptionsShutdownStatesFiller);
}

CPowerManager::~CPowerManager() = default;

void CPowerManager::Initialize()
{
  m_instance.reset(IPowerSyscall::CreateInstance());
}

void CPowerManager::SetDefaults()
{
  auto setting = m_settings->GetSetting(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
  if (!setting)
  {
    CLog::Log(LOGERROR, "Failed to load setting for: {}",
              CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);
    return;
  }

  int defaultShutdown = m_settings->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE);

  switch (defaultShutdown)
  {
    case POWERSTATE_QUIT:
    case POWERSTATE_MINIMIZE:
      // assume we can shutdown if --standalone is passed
      if (CServiceBroker::GetAppParams()->IsStandAlone())
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

  std::static_pointer_cast<CSettingInt>(setting)->SetDefault(defaultShutdown);
}

bool CPowerManager::Powerdown()
{
  if (CanPowerdown() && m_instance->Powerdown())
  {
    CGUIDialogBusyNoCancel* dialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusyNoCancel>(
            WINDOW_DIALOG_BUSY_NOCANCEL);
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
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "OnRestart");

    CGUIDialogBusyNoCancel* dialog =
        CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusyNoCancel>(
            WINDOW_DIALOG_BUSY_NOCANCEL);
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
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "OnSleep");

  CGUIDialogBusyNoCancel* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusyNoCancel>(
          WINDOW_DIALOG_BUSY_NOCANCEL);
  if (dialog)
    dialog->Open();

  CLog::Log(LOGINFO, "{}: Running sleep jobs", __FUNCTION__);

  StorePlayerState();

  g_application.StopPlaying();
  CServiceBroker::GetPVRManager().OnSleep();
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->StopShutdownTimer();
  appPower->StopScreenSaverTimer();
  g_application.CloseNetworkShares();
  CServiceBroker::GetActiveAE()->Suspend();
}

void CPowerManager::OnWake()
{
  CLog::Log(LOGINFO, "{}: Running resume jobs", __FUNCTION__);

  CServiceBroker::GetNetwork().WaitForNet();

  // reset out timers
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  appPower->ResetShutdownTimers();

  CGUIDialogBusyNoCancel* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogBusyNoCancel>(
          WINDOW_DIALOG_BUSY_NOCANCEL);
  if (dialog)
    dialog->Close(true); // force close. no closing animation, sound etc at this early stage

#if defined(TARGET_DARWIN_OSX) || defined(TARGET_WINDOWS)
  if (CServiceBroker::GetWinSystem()->IsFullScreen())
  {
#if defined(TARGET_WINDOWS_DESKTOP)
    ShowWindow(g_hWnd, SW_RESTORE);
    SetForegroundWindow(g_hWnd);
#endif
  }
  appPower->ResetScreenSaver();
#endif

  CServiceBroker::GetActiveAE()->Resume();
  g_application.UpdateLibraries();
  CServiceBroker::GetWeatherManager().Refresh();
  CServiceBroker::GetPVRManager().OnWake();
  RestorePlayerState();

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "OnWake");
}

void CPowerManager::OnLowBattery()
{
  CLog::Log(LOGINFO, "{}: Running low battery jobs", __FUNCTION__);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13050), "");

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::System, "OnLowBattery");
}

void CPowerManager::StorePlayerState()
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlaying())
  {
    m_lastUsedPlayer = appPlayer->GetCurrentPlayer();
    m_lastPlayedFileItem = std::make_unique<CFileItem>(g_application.CurrentFileItem());
    // set the actual offset instead of store and load it from database
    m_lastPlayedFileItem->SetStartOffset(appPlayer->GetTime());
    // in case of regular stack, correct the start offset by adding current part start time
    const auto stackHelper = components.GetComponent<CApplicationStackHelper>();
    if (stackHelper->IsPlayingRegularStack())
      m_lastPlayedFileItem->SetStartOffset(m_lastPlayedFileItem->GetStartOffset() +
                                           stackHelper->GetCurrentStackPartStartTimeMs());
    // in case of iso stack, keep track of part number
    m_lastPlayedFileItem->m_lStartPartNumber =
        stackHelper->IsPlayingISOStack() ? stackHelper->GetCurrentPartNumber() + 1 : 1;
    // for iso and iso stacks, keep track of playerstate
    m_lastPlayedFileItem->SetProperty("savedplayerstate", appPlayer->GetPlayerState());
    CLog::Log(LOGDEBUG,
              "CPowerManager::StorePlayerState - store last played item (startOffset: {} ms)",
              m_lastPlayedFileItem->GetStartOffset());
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

  CLog::Log(LOGDEBUG,
            "CPowerManager::RestorePlayerState - resume last played item (startOffset: {} ms)",
            m_lastPlayedFileItem->GetStartOffset());
  g_application.PlayFile(*m_lastPlayedFileItem, m_lastUsedPlayer);
}

void CPowerManager::SettingOptionsShutdownStatesFiller(const SettingConstPtr& setting,
                                                       std::vector<IntegerSettingOption>& list,
                                                       int& current,
                                                       void* data)
{
  if (CServiceBroker::GetPowerManager().CanPowerdown())
    list.emplace_back(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN);
  if (CServiceBroker::GetPowerManager().CanHibernate())
    list.emplace_back(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE);
  if (CServiceBroker::GetPowerManager().CanSuspend())
    list.emplace_back(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND);
  if (!CServiceBroker::GetAppParams()->IsStandAlone())
  {
    list.emplace_back(g_localizeStrings.Get(13009), POWERSTATE_QUIT);
#if !defined(TARGET_DARWIN_EMBEDDED)
    list.emplace_back(g_localizeStrings.Get(13014), POWERSTATE_MINIMIZE);
#endif
  }
}
