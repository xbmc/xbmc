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

#include "Win32PowerSyscall.h"
#ifdef _WIN32
#include "WIN32Util.h"

bool CWin32PowerSyscall::m_OnResume = false;
bool CWin32PowerSyscall::m_OnSuspend = false;


CWin32PowerSyscall::CWin32PowerSyscall()
{
}

bool CWin32PowerSyscall::Powerdown()
{
  return CWIN32Util::PowerManagement(POWERSTATE_SHUTDOWN);
}
bool CWin32PowerSyscall::Suspend()
{
  // On Vista+, we don't receive the PBT_APMSUSPEND message as we have fired the suspend mode
  // Set the flag manually
  CWin32PowerSyscall::SetOnSuspend();

  return CWIN32Util::PowerManagement(POWERSTATE_SUSPEND);
}
bool CWin32PowerSyscall::Hibernate()
{
  // On Vista+, we don't receive the PBT_APMSUSPEND message as we have fired the suspend mode
  // Set the flag manually
  CWin32PowerSyscall::SetOnSuspend();

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
  return CWIN32Util::BatteryLevel();
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
#endif
