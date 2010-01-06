/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "PowerManager.h"
#include "Application.h"
#include "KeyboardStat.h"
#include "MouseStat.h"
#include "GUISettings.h"
#include "WindowingFactory.h"
#include "utils/log.h"

#ifdef HAS_LCD
#include "utils/LCDFactory.h"
#endif

#ifdef __APPLE__
#include "osx/CocoaPowerSyscall.h"
#elif defined(_LINUX) && defined(HAS_DBUS)
#include "linux/ConsoleDeviceKitPowerSyscall.h"
#ifdef HAS_HAL
#include "linux/HALPowerSyscall.h"
#endif
#elif defined(_WIN32)
#include "win32/Win32PowerSyscall.h"
extern HWND g_hWnd;
#endif

#ifdef HAS_LIRC
#include "common/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE
  #include "common/IRServerSuite/IRServerSuite.h"
#endif

CPowerManager g_powerManager;

CPowerManager::CPowerManager()
{
  m_instance = new CNullPowerSyscall();
}

CPowerManager::~CPowerManager()
{
  delete m_instance;
}

void CPowerManager::Initialize()
{
  delete m_instance;

#ifdef __APPLE__
  m_instance = new CCocoaPowerSyscall();
#elif defined(_LINUX) && defined(HAS_DBUS)
  if (CConsoleDeviceKitPowerSyscall::HasDeviceConsoleKit())
    m_instance = new CConsoleDeviceKitPowerSyscall();
#ifdef HAS_HAL
  else
    m_instance = new CHALPowerSyscall();
#else
  else
    m_instance = new CNULLPowerSyscall();
#endif
#elif defined(_WIN32)
  m_instance = new CWin32PowerSyscall();
#else
  m_instance = new CNullPowerSyscall();
#endif

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
  if (CanSuspend())
  {
    g_application.m_bRunResumeJobs = true;
#ifdef HAS_LCD
    g_lcd->SetBackLight(0);
#endif
    g_Keyboard.ResetState();
    return m_instance->Suspend();
  }
  
  return false;
}
bool CPowerManager::Hibernate()
{
  if (CanHibernate())
  {
    g_application.m_bRunResumeJobs = true;
    g_Keyboard.ResetState();
    return m_instance->Hibernate();
  }

  return false;
}
bool CPowerManager::Reboot()
{
  return CanReboot() ? m_instance->Reboot() : false;
}

void CPowerManager::Resume()
{
  CLog::Log(LOGNOTICE, "%s: Running resume jobs", __FUNCTION__);

#ifdef HAS_SDL
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
  g_RemoteControl.Disconnect();
  g_RemoteControl.Initialize();
#endif

  // restart and undim lcd
#ifdef HAS_LCD
  CLog::Log(LOGNOTICE, "%s: Restarting lcd", __FUNCTION__);
  g_lcd->SetBackLight(1);
  g_lcd->Stop();
  g_lcd->Initialize();
#endif

  g_application.UpdateLibraries();

  // reset
  g_application.m_bRunResumeJobs = false;
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
