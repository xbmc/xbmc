/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32PowerSyscall.h"

#include "utils/SystemInfo.h"
#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

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
    if (AbortableWait(m_queryEvent) == WAIT_SIGNALED)
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
    return InitiateShutdownW(NULL, NULL, 0,
                             SHUTDOWN_HYBRID | SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_POWEROFF,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER |
                                 SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  case POWERSTATE_REBOOT:
    CLog::Log(LOGINFO, "Rebooting Windows...");
    return InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_INSTALL_UPDATES | SHUTDOWN_RESTART,
                             SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER |
                                 SHTDN_REASON_FLAG_PLANNED) == ERROR_SUCCESS;
  default:
    CLog::Log(LOGERROR, "Unknown PowerState called.");
    return false;
  }
}


CWin32PowerSyscall::CWin32PowerSyscall()
{
  m_hascapabilities = GetPwrCapabilities(&m_capabilities);
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
  if (m_hascapabilities)
    return (m_capabilities.SystemS1 == TRUE || m_capabilities.SystemS2 == TRUE || m_capabilities.SystemS3 == TRUE);
  return true;
}
bool CWin32PowerSyscall::CanHibernate()
{
  if (m_hascapabilities)
    return (m_capabilities.SystemS4 == TRUE && m_capabilities.HiberFilePresent == TRUE);
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
