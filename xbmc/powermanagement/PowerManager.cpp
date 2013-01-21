/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "PowerManager.h"
#include "Application.h"
#include "cores/AudioEngine/AEFactory.h"
#include "input/KeyboardStat.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/Weather.h"
#include "interfaces/Builtins.h"
#include "interfaces/AnnouncementManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GraphicContext.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKaiToast.h"

#if defined(TARGET_DARWIN)
#include "osx/CocoaPowerSyscall.h"
#elif defined(TARGET_ANDROID)
#include "android/AndroidPowerSyscall.h"
#elif defined(TARGET_POSIX)
#include "linux/FallbackPowerSyscall.h"
#if defined(HAS_DBUS)
#include "linux/ConsoleUPowerSyscall.h"
#include "linux/ConsoleDeviceKitPowerSyscall.h"
#include "linux/LogindUPowerSyscall.h"
#include "linux/UPowerSyscall.h"
#if defined(HAS_HAL)
#include "linux/HALPowerSyscall.h"
#endif // HAS_HAL
#endif // HAS_DBUS
#elif defined(TARGET_WINDOWS)
#include "powermanagement/windows/Win32PowerSyscall.h"
extern HWND g_hWnd;
#endif

using namespace ANNOUNCEMENT;

CPowerManager g_powerManager;

CPowerManager::CPowerManager()
{
  m_instance = NULL;
}

CPowerManager::~CPowerManager()
{
  delete m_instance;
}

void CPowerManager::Initialize()
{
#if defined(TARGET_DARWIN)
  m_instance = new CCocoaPowerSyscall();
#elif defined(TARGET_ANDROID)
  m_instance = new CAndroidPowerSyscall();
#elif defined(TARGET_POSIX)
#if defined(HAS_DBUS)
  if (CConsoleUPowerSyscall::HasConsoleKitAndUPower())
    m_instance = new CConsoleUPowerSyscall();
  else if (CConsoleDeviceKitPowerSyscall::HasDeviceConsoleKit())
    m_instance = new CConsoleDeviceKitPowerSyscall();
  else if (CLogindUPowerSyscall::HasLogind())
    m_instance = new CLogindUPowerSyscall();
  else if (CUPowerSyscall::HasUPower())
    m_instance = new CUPowerSyscall();
#if defined(HAS_HAL)
  else if(1)
    m_instance = new CHALPowerSyscall();
#endif // HAS_HAL
  else
#endif // HAS_DBUS
    m_instance = new CFallbackPowerSyscall();
#elif defined(TARGET_WINDOWS)
  m_instance = new CWin32PowerSyscall();
#endif

  if (m_instance == NULL)
    m_instance = new CNullPowerSyscall();
}

void CPowerManager::SetDefaults()
{
  int defaultShutdown = CSettings::Get().GetInt("powermanagement.shutdownstate");

  switch (defaultShutdown)
  {
    case POWERSTATE_QUIT:
    case POWERSTATE_MINIMIZE:
      // assume we can shutdown if --standalone is passed
      if (g_application.IsStandAlone())
        defaultShutdown = POWERSTATE_SHUTDOWN;
    break;
    case POWERSTATE_HIBERNATE:
      if (!g_powerManager.CanHibernate())
      {
        if (g_powerManager.CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = g_powerManager.CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SUSPEND:
      if (!g_powerManager.CanSuspend())
      {
        if (g_powerManager.CanHibernate())
          defaultShutdown = POWERSTATE_HIBERNATE;
        else
          defaultShutdown = g_powerManager.CanPowerdown() ? POWERSTATE_SHUTDOWN : POWERSTATE_QUIT;
      }
    break;
    case POWERSTATE_SHUTDOWN:
      if (!g_powerManager.CanPowerdown())
      {
        if (g_powerManager.CanSuspend())
          defaultShutdown = POWERSTATE_SUSPEND;
        else
          defaultShutdown = g_powerManager.CanHibernate() ? POWERSTATE_HIBERNATE : POWERSTATE_QUIT;
      }
    break;
  }

  ((CSettingInt*)CSettings::Get().GetSetting("powermanagement.shutdownstate"))->SetDefault(defaultShutdown);
}

bool CPowerManager::Powerdown()
{
  if (CanPowerdown() && m_instance->Powerdown())
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Show();

    return true;
  }

  return false;
}

bool CPowerManager::Suspend()
{
  if (CanSuspend() && m_instance->Suspend())
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Show();

    return true;
  }

  return false;
}

bool CPowerManager::Hibernate()
{
  if (CanHibernate() && m_instance->Hibernate())
  {
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Show();

    return true;
  }

  return false;
}
bool CPowerManager::Reboot()
{
  bool success = CanReboot() ? m_instance->Reboot() : false;

  if (success)
  {
    CAnnouncementManager::Announce(System, "xbmc", "OnRestart");

    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
    if (dialog)
      dialog->Show();
  }

  return success;
}

bool CPowerManager::CanPowerdown()
{
  return m_instance->CanPowerdown();
}
bool CPowerManager::CanSuspend()
{
  return m_instance->CanSuspend();
}
bool CPowerManager::CanHibernate()
{
  return m_instance->CanHibernate();
}
bool CPowerManager::CanReboot()
{
  return m_instance->CanReboot();
}
int CPowerManager::BatteryLevel()
{
  return m_instance->BatteryLevel();
}
void CPowerManager::ProcessEvents()
{
  static int nesting = 0;

  if (++nesting == 1)
    m_instance->PumpPowerEvents(this);

  nesting--;
}

void CPowerManager::OnSleep()
{
  CAnnouncementManager::Announce(System, "xbmc", "OnSleep");
  CLog::Log(LOGNOTICE, "%s: Running sleep jobs", __FUNCTION__);

  // stop lirc
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CLog::Log(LOGNOTICE, "%s: Stopping lirc", __FUNCTION__);
  CBuiltins::Execute("LIRC.Stop");
#endif

  g_application.SaveFileState(true);
  g_application.StopPlaying();
  g_application.StopShutdownTimer();
  g_application.StopScreenSaverTimer();
  g_application.CloseNetworkShares();
  CAEFactory::Suspend();
}

void CPowerManager::OnWake()
{
  CLog::Log(LOGNOTICE, "%s: Running resume jobs", __FUNCTION__);

  // reset out timers
  g_application.ResetShutdownTimers();

  CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
  if (dialog)
    dialog->Close();

#if defined(HAS_SDL) || defined(TARGET_WINDOWS)
  if (g_Windowing.IsFullScreen())
  {
#if defined(TARGET_WINDOWS)
    ShowWindow(g_hWnd,SW_RESTORE);
    SetForegroundWindow(g_hWnd);
#endif
  }
  g_application.ResetScreenSaver();
#endif

  // restart lirc
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CLog::Log(LOGNOTICE, "%s: Restarting lirc", __FUNCTION__);
  CBuiltins::Execute("LIRC.Start");
#endif

  CAEFactory::Resume();
  g_application.UpdateLibraries();
  g_weatherManager.Refresh();

  CAnnouncementManager::Announce(System, "xbmc", "OnWake");
}

void CPowerManager::OnLowBattery()
{
  CLog::Log(LOGNOTICE, "%s: Running low battery jobs", __FUNCTION__);

  CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(13050), "");

  CAnnouncementManager::Announce(System, "xbmc", "OnLowBattery");
}

void CPowerManager::SettingOptionsShutdownStatesFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current)
{
  if (g_powerManager.CanPowerdown())
    list.push_back(make_pair(g_localizeStrings.Get(13005), POWERSTATE_SHUTDOWN));
  if (g_powerManager.CanHibernate())
    list.push_back(make_pair(g_localizeStrings.Get(13010), POWERSTATE_HIBERNATE));
  if (g_powerManager.CanSuspend())
    list.push_back(make_pair(g_localizeStrings.Get(13011), POWERSTATE_SUSPEND));
  if (!g_application.IsStandAlone())
  {
    list.push_back(make_pair(g_localizeStrings.Get(13009), POWERSTATE_QUIT));
#if !defined(TARGET_DARWIN_IOS)
    list.push_back(make_pair(g_localizeStrings.Get(13014), POWERSTATE_MINIMIZE));
#endif
  }
}
