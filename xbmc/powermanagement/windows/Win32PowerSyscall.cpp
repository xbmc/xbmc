/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "Win32PowerSyscall.h"
#include "platform/win32/WIN32Util.h"
#include "powermanagement/PowerManager.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"

#include <PowrProf.h>

bool CWin32PowerSyscall::m_OnResume = false;
bool CWin32PowerSyscall::m_OnSuspend = false;

IPowerSyscall* CWin32PowerSyscall::CreateInstance()
{
  return new CWin32PowerSyscall();
}

void CWin32PowerSyscall::Register()
{
  IPowerSyscall::RegisterPowerSyscall(CWin32PowerSyscall::CreateInstance);
}

bool CWin32PowerStateWorker::QueryStateChange(PowerState state)
{
  if (!IsRunning())
    return false;

  if (m_state.exchange(state) != state)
  {
    m_queryEvent.Set();
    return true;
  }

  return false;
}

void CWin32PowerStateWorker::Process(void)
{
  while (!m_bStop)
  {
    if (AbortableWait(m_queryEvent, -1) == WAIT_SIGNALED)
    {
      PowerManagement(m_state.load());
      m_state.exchange(POWERSTATE_NONE);
      m_queryEvent.Reset();
    }
  }
}

bool CWin32PowerStateWorker::PowerManagement(PowerState State)
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
      return false;
  }

  switch (State)
  {
  case POWERSTATE_HIBERNATE:
    CLog::Log(LOGINFO, "Asking Windows to hibernate...");
    return SetSuspendState(true, true, false) == TRUE;
  case POWERSTATE_SUSPEND:
    CLog::Log(LOGINFO, "Asking Windows to suspend...");
    return SetSuspendState(false, true, false) == TRUE;
  case POWERSTATE_SHUTDOWN:
    CLog::Log(LOGINFO, "Shutdown Windows...");
    if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8))
      return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_HYBRID | SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_POWEROFF,
                               SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
    return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_POWEROFF,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  case POWERSTATE_REBOOT:
    CLog::Log(LOGINFO, "Rebooting Windows...");
    if (g_sysinfo.IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin8))
      return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_RESTART,
                               SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
    return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_RESTART,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  default:
    CLog::Log(LOGERROR, "Unknown PowerState called.");
    return false;
  }
}


CWin32PowerSyscall::CWin32PowerSyscall()
{
  m_worker.Create();
}

CWin32PowerSyscall::~CWin32PowerSyscall()
{
  if (m_worker.IsRunning())
    m_worker.StopThread();
}

bool CWin32PowerSyscall::Powerdown()
{
  return m_worker.QueryStateChange(POWERSTATE_SHUTDOWN);
}
bool CWin32PowerSyscall::Suspend()
{
  return m_worker.QueryStateChange(POWERSTATE_SUSPEND);
}
bool CWin32PowerSyscall::Hibernate()
{
  return m_worker.QueryStateChange(POWERSTATE_HIBERNATE);
}
bool CWin32PowerSyscall::Reboot()
{
  return m_worker.QueryStateChange(POWERSTATE_REBOOT);
}

bool CWin32PowerSyscall::CanPowerdown()
{
  return true;
}
bool CWin32PowerSyscall::CanSuspend()
{
  return true;
}
bool CWin32PowerSyscall::CanHibernate()
{
  return true;
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
  return 0;
}

bool CWin32PowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  if (m_OnSuspend)
  {
    callback->OnSleep();
    m_OnSuspend = false;
    return true;
  }
  else if (m_OnResume)
  {
    callback->OnWake();
    m_OnResume = false;
    return true;
  }
  else
    return false;
}
