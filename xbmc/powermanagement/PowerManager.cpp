/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "input/KeyboardStat.h"
#include "settings/GUISettings.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include "utils/Weather.h"
#include "interfaces/Builtins.h"
#include "interfaces/AnnouncementManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GraphicContext.h"
#include "dialogs/GUIDialogKaiToast.h"

#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif

#if defined(TARGET_DARWIN)
#include "osx/CocoaPowerSyscall.h"
#elif defined(TARGET_ANDROID)
#include "android/AndroidPowerSyscall.h"
#elif defined(_LINUX) && defined(HAS_DBUS)
#include "linux/ConsoleUPowerSyscall.h"
#include "linux/ConsoleDeviceKitPowerSyscall.h"
#ifdef HAS_HAL
#include "linux/HALPowerSyscall.h"
#endif
#elif defined(_WIN32)
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
#elif defined(_LINUX) && defined(HAS_DBUS)
  if (CConsoleUPowerSyscall::HasDeviceConsoleKit())
    m_instance = new CConsoleUPowerSyscall();
  else if (CConsoleDeviceKitPowerSyscall::HasDeviceConsoleKit())
    m_instance = new CConsoleDeviceKitPowerSyscall();
#ifdef HAS_HAL
  else
    m_instance = new CHALPowerSyscall();
#endif
#elif defined(_WIN32)
  m_instance = new CWin32PowerSyscall();
#endif

  if (m_instance == NULL)
    m_instance = new CNullPowerSyscall();
}

void CPowerManager::SetDefaults()
{
  int defaultShutdown = g_guiSettings.GetInt("powermanagement.shutdownstate");

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

  g_guiSettings.SetInt("powermanagement.shutdownstate", defaultShutdown);
}

bool CPowerManager::Powerdown()
{
  return CanPowerdown() ? m_instance->Powerdown() : false;
}

bool CPowerManager::Suspend()
{
  return CanSuspend() ? m_instance->Suspend() : false;
}

bool CPowerManager::Hibernate()
{
  return CanHibernate() ? m_instance->Hibernate() : false;
}
bool CPowerManager::Reboot()
{
  bool success = CanReboot() ? m_instance->Reboot() : false;

  if (success)
    CAnnouncementManager::Announce(System, "xbmc", "OnRestart");

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
  m_instance->PumpPowerEvents(this);
}

void CPowerManager::OnSleep()
{
  CAnnouncementManager::Announce(System, "xbmc", "OnSleep");
  CLog::Log(LOGNOTICE, "%s: Running sleep jobs", __FUNCTION__);

#ifdef HAS_LCD
  g_lcd->SetBackLight(0);
#endif

  // stop lirc
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CLog::Log(LOGNOTICE, "%s: Stopping lirc", __FUNCTION__);
  CBuiltins::Execute("LIRC.Stop");
#endif

  g_application.SaveFileState(true);
  g_application.StopPlaying();
  g_application.StopShutdownTimer();
  g_application.StopScreenSaverTimer();
}

void CPowerManager::OnWake()
{
  CLog::Log(LOGNOTICE, "%s: Running resume jobs", __FUNCTION__);

  // reset out timers
  g_application.ResetShutdownTimers();

#if defined(HAS_SDL) || defined(TARGET_WINDOWS)
  if (g_Windowing.IsFullScreen())
  {
#ifdef _WIN32
    ShowWindow(g_hWnd,SW_RESTORE);
    SetForegroundWindow(g_hWnd);
#else
    // Hack to reclaim focus, thus rehiding system mouse pointer.
    // Surely there's a better way?
    g_graphicsContext.ToggleFullScreenRoot();
    g_graphicsContext.ToggleFullScreenRoot();
#endif
  }
  g_application.ResetScreenSaver();
#endif

  // restart lirc
#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
  CLog::Log(LOGNOTICE, "%s: Restarting lirc", __FUNCTION__);
  CBuiltins::Execute("LIRC.Start");
#endif

  // restart and undim lcd
#ifdef HAS_LCD
  CLog::Log(LOGNOTICE, "%s: Restarting lcd", __FUNCTION__);
  g_lcd->SetBackLight(1);
  g_lcd->Stop();
  g_lcd->Initialize();
#endif

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
