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

bool CWin32PowerSyscall::m_OnResume = false;
bool CWin32PowerSyscall::m_OnSuspend = false;

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
      CWIN32Util::PowerManagement(m_state.load());
      m_state.exchange(POWERSTATE_NONE);
      m_queryEvent.Reset();
    }
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
