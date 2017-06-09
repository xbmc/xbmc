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

#if defined(HAS_LINUX_EVENTS)

#include "WinEventsLinux.h"
#include "WinEvents.h"
#include "XBMC_events.h"
#include "input/XBMC_keysym.h"
#include "Application.h"
#include "input/MouseStat.h"
#include "utils/log.h"
#include "powermanagement/PowerManager.h"
#include "peripherals/Peripherals.h"
#include "ServiceBroker.h"


bool CWinEventsLinux::m_initialized = false;
CLinuxInputDevices CWinEventsLinux::m_devices;

CWinEventsLinux::CWinEventsLinux()
{
}

void CWinEventsLinux::RefreshDevices()
{
  m_devices.InitAvailable();
}

bool CWinEventsLinux::IsRemoteLowBattery()
{
  return m_devices.IsRemoteLowBattery();
}

bool CWinEventsLinux::MessagePump()
{
  if (!m_initialized)
  {
    CServiceBroker::GetPeripherals().RegisterObserver(this);
    m_devices.InitAvailable();
    m_checkHotplug = std::unique_ptr<CLinuxInputDevicesCheckHotplugged>(new CLinuxInputDevicesCheckHotplugged(m_devices));
    m_initialized = true;
  }

  bool ret = false;
  XBMC_Event event = {0};
  while (1)
  {
    event = m_devices.ReadEvent();
    if (event.type != XBMC_NOEVENT)
    {
      ret |= g_application.OnEvent(event);
    }
    else
    {
      break;
    }
  }

  return ret;
}

void CWinEventsLinux::MessagePush(XBMC_Event *ev)
{
  g_application.OnEvent(*ev);
}

#endif
