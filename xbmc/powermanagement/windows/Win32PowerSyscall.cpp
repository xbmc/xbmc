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

#include "Win32PowerSyscall.h"
#include "powermanagement/PowerManager.h"
#ifdef TARGET_WINDOWS
#include "WIN32Util.h"
#include "utils/log.h"
#include <powrprof.h>
#pragma comment(lib, "PowrProf")

bool CWin32PowerSyscall::m_OnResume = false;
bool CWin32PowerSyscall::m_OnSuspend = false;
bool CWin32PowerSyscall::m_sleeping = false;


CWin32PowerSyscall::CWin32PowerSyscall()
{
}

bool CWin32PowerSyscall::Powerdown()
{
  return CWIN32Util::PowerManagement(POWERSTATE_SHUTDOWN);
}

bool CWin32PowerSyscall::Suspend()
{
  if (!CanSuspend())
  {
    CLog::LogF(LOGERROR, "Can't suspend: suspend is not supported by system");
    return false;
  }

  return CWIN32Util::PowerManagement(POWERSTATE_SUSPEND);
}

bool CWin32PowerSyscall::Hibernate()
{
  if (!CanHibernate())
  {
    CLog::LogF(LOGERROR, "Can't hibernate: hibernate is not supported by system");
    return false;
  }

  return CWIN32Util::PowerManagement(POWERSTATE_HIBERNATE);
}

bool CWin32PowerSyscall::Reboot()
{
  return CWIN32Util::PowerManagement(POWERSTATE_REBOOT);
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
  return CWIN32Util::BatteryLevel();
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
