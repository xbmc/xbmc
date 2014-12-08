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

#ifdef TARGET_WINDOWS
#include "Win32PowerSyscall.h"
#include "powermanagement/PowerManager.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"

#include <powrprof.h>
#pragma comment(lib, "PowrProf")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN 
#include <Windows.h>


bool CWin32PowerSyscall::m_OnResume = false;
bool CWin32PowerSyscall::m_OnSuspend = false;
bool CWin32PowerSyscall::m_sleeping = false;


// local helpers
static DWORD WINAPI threadForHibernateOs(_In_ LPVOID ignored)
{
  CLog::LogF(LOGNOTICE, "Requesting OS to hibernate");
  if (SetSuspendState(true, true, false) != FALSE) // may be blocked until resume
  {
    CLog::LogF(LOGDEBUG, "OS was hibernated successfully");
    return TRUE;
  }
  CLog::LogF(LOGERROR, "Can't hibernate system, error code: %lu", GetLastError());
  return FALSE;
}

static DWORD WINAPI threadForSuspendOs(_In_ LPVOID ignored)
{
  CLog::LogF(LOGNOTICE, "Requesting OS to suspend");
  if (SetSuspendState(false, true, false) != FALSE) // may be blocked until resume
  {
    CLog::LogF(LOGDEBUG, "OS was suspended successfully");
    return TRUE;
  }
  CLog::LogF(LOGERROR, "Can't suspend system, error code: %lu", GetLastError());
  return FALSE;
}

bool CWin32PowerSyscall::AdjustPrivileges()
{
  static bool gotShutdownPrivileges = false;
  if (!gotShutdownPrivileges)
  {
    HANDLE hToken;
    // Get a token for this process.
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
      // Get the LUID for the shutdown privilege.
      TOKEN_PRIVILEGES tkp = {};
      if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid))
      {
        tkp.PrivilegeCount = 1;  // one privilege to set
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        // Get the shutdown privilege for this process.
        if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0))
          gotShutdownPrivileges = true;
      }
      CloseHandle(hToken);
    }

    if (!gotShutdownPrivileges)
    {
      CLog::LogF(LOGERROR, "Can't get shutdown privileges");
      return false;
    }
    CLog::LogF(LOGDEBUG, "Got shutdown privileges");
  }
  else
    CLog::LogF(LOGDEBUG, "Already have shutdown privileges");

  return true;
}

CWin32PowerSyscall::CWin32PowerSyscall()
{
}

bool CWin32PowerSyscall::Powerdown()
{
  if (!AdjustPrivileges())
    return false;

  CLog::LogF(LOGINFO, "Requesting OS shutdown");

  if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8))
    return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_HYBRID | SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_POWEROFF,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_POWEROFF,
                           SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
}

bool CWin32PowerSyscall::Suspend()
{
  if (!CanSuspend())
  {
    CLog::LogF(LOGERROR, "Can't suspend: suspend is not supported by system");
    return false;
  }
  if (!AdjustPrivileges())
    return false;

  CWin32PowerSyscall::SetOnSuspend();
  // process OnSleep() events. This is called in main thread.
  g_powerManager.ProcessEvents();
  HANDLE threadHandle = CreateThread(NULL, 0, threadForSuspendOs, NULL, 0, NULL); // use separate thread, so main thread stay unblocked
  if (threadHandle == NULL)
  {
    CLog::LogF(LOGERROR, "Can't create thread for switching power mode");
    return false;
  }
  CloseHandle(threadHandle); // thread is one-shot, no need to track it
  return true;
}

bool CWin32PowerSyscall::Hibernate()
{
  if (!CanHibernate())
  {
    CLog::LogF(LOGERROR, "Can't hibernate: hibernate is not supported by system");
    return false;
  }
  if (!AdjustPrivileges())
    return false;

  CWin32PowerSyscall::SetOnSuspend();
  // process OnSleep() events. This is called in main thread.
  g_powerManager.ProcessEvents();
  HANDLE threadHandle = CreateThread(NULL, 0, threadForHibernateOs, NULL, 0, NULL); // use separate thread, so main thread stay unblocked
  if (threadHandle == NULL)
  {
    CLog::LogF(LOGERROR, "Can't create thread for switching power mode");
    return false;
  }
  CloseHandle(threadHandle); // thread is one-shot, no need to track it
  return true;
}

bool CWin32PowerSyscall::Reboot()
{
  if (!AdjustPrivileges())
    return false;

  CLog::LogF(LOGINFO, "Requesting OS reboot");

  if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8))
    return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_RESTART,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_RESTART,
                           SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
}

bool CWin32PowerSyscall::CanPowerdown()
{
  return true;
}

bool CWin32PowerSyscall::CanSuspend()
{
  static int suspendSupported = -1;
  if (suspendSupported == -1)
  {
    SYSTEM_POWER_CAPABILITIES pwcp = {};
    if (GetPwrCapabilities(&pwcp))
    {
      if (pwcp.SystemS3 || pwcp.SystemS2 || pwcp.SystemS1)
      {
        CLog::LogF(LOGDEBUG, "System supports suspend. S1 state: %s; S2 state: %s; S3 state: %s",
                   pwcp.SystemS1 ? "supported" : "not supported", pwcp.SystemS2 ? "supported" : "not supported",
                   pwcp.SystemS3 ? "supported" : "not supported");
        suspendSupported = 1;
      }
      else
      {
        CLog::LogF(LOGDEBUG, "System doesn't supports suspend");
        suspendSupported = 0;
      }
    }
    else
    {
      CLog::LogF(LOGERROR, "Can't determine support of \"suspend\" system state");
      suspendSupported = 0;
    }
  }
  return suspendSupported == 1;
}

bool CWin32PowerSyscall::CanHibernate()
{
  static int hibernateSupported = -1;
  if (hibernateSupported == -1)
  {
    SYSTEM_POWER_CAPABILITIES pwcp = {};
    if (GetPwrCapabilities(&pwcp))
    {
      if (pwcp.SystemS4 && pwcp.HiberFilePresent)
      {
        CLog::LogF(LOGDEBUG, "System supports hibernate");
        hibernateSupported = 1;
      }
      else
      {
        CLog::LogF(LOGDEBUG, "System doesn't supports hibernate. S4 state: %s; Hibernate file: %s",
                   pwcp.SystemS4 ? "supported" : "not supported", pwcp.HiberFilePresent ? "present" : "absent");
        hibernateSupported = 0;
      }
    }
    else
    {
      CLog::LogF(LOGERROR, "Can't determine support of \"hibernate\" system state");
      hibernateSupported = 0;
    }
  }
  return hibernateSupported == 1;
}

bool CWin32PowerSyscall::CanReboot()
{
  return true;
}

int CWin32PowerSyscall::BatteryLevel()
{
  SYSTEM_POWER_STATUS SystemPowerStatus;

  if (GetSystemPowerStatus(&SystemPowerStatus) && SystemPowerStatus.BatteryLifePercent != 255)
    return SystemPowerStatus.BatteryLifePercent;

  return -1;
}

bool CWin32PowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  if (m_OnSuspend)
  {
    if (!m_sleeping)
    {
      CLog::LogF(LOGDEBUG, "Processing OnSuspend event");
      callback->OnSleep();
      m_OnSuspend = false;
      m_sleeping = true;
      return true;
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Sleeping (already) - ignoring OnSuspend events");
      m_OnSuspend = false;
      return true;
    }
  }
  else if (m_OnResume)
  {
    if (m_sleeping)
    {
      CLog::LogF(LOGDEBUG, "Processing OnResume event");
      callback->OnWake();
      m_OnResume = false;
      m_sleeping = false;
      return true;
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Not sleeping (already) - ignoring OnResume events");
      m_OnResume = false;
      return true;
    }
  }
  else
    return false;
}
#endif
