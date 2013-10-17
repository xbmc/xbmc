/*
 *      Copyright (C) 2013 Team XBMC
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

#include "PowerSyscallVirtualSleep.h"
#include "guilib/Key.h"

bool CPowerSyscallVirtualSleep::Suspend()
{
  if (m_virtualSleepState == VIRTUAL_SLEEP_STATE_AWAKE)
  {
    if (VirtualSleep())
    {
      m_virtualSleepState = VIRTUAL_SLEEP_STATE_WILL_SLEEP;
      return true;
    }
  }

  return false;
}

bool CPowerSyscallVirtualSleep::PumpPowerEvents(IPowerEventsCallback *callback)
{
  if (m_virtualSleepState == VIRTUAL_SLEEP_STATE_WILL_WAKE)
  {
    callback->OnWake();
    m_virtualSleepState = VIRTUAL_SLEEP_STATE_AWAKE;
    return true;
  }
  else if (m_virtualSleepState == VIRTUAL_SLEEP_STATE_WILL_SLEEP)
  {
    callback->OnSleep();
    m_virtualSleepState = VIRTUAL_SLEEP_STATE_ASLEEP;
    return true;
  }
  
  return false;
}

bool CPowerSyscallVirtualSleep::ProcessAction(const CAction& action)
{
  if (m_virtualSleepState != VIRTUAL_SLEEP_STATE_ASLEEP)
    return false;

  // device is in virtual sleep, only one of the power keys will
  // wake it up again.
  if (action.GetID() == ACTION_BUILT_IN_FUNCTION)
  {
    CStdString name = action.GetName();
    name.ToLower();
    if(name.Equals("shutdown") ||
       name.Equals("suspend")  ||
       name.Equals("hibernate"))
    {
      if(VirtualWake())
      {
        m_virtualSleepState = VIRTUAL_SLEEP_STATE_WILL_WAKE;
        return false;
      }
    }
  }

  // wasn't a power key, suppress this and stay asleep
  return true;
}
