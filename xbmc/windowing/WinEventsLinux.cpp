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

#if defined(HAS_LINUX_EVENTS)

#include "WinEventsLinux.h"
#include "WinEvents.h"
#include "XBMC_events.h"
#include "input/XBMC_keysym.h"
#include "Application.h"
#include "input/MouseStat.h"
#include "utils/log.h"
#include "powermanagement/PowerManager.h"

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

bool CWinEventsLinux::m_initialized = false;
CLinuxInputDevices CWinEventsLinux::m_devices;
std::map<int, int> CWinEventsLinux::m_altMap;
std::map<int, int> CWinEventsLinux::m_keyPadMap;

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
  return false;
}

bool CWinEventsLinux::MessagePump()
{
  static int number;
  static bool accumlate;

  if (!m_initialized)
  {
    m_devices.InitAvailable();

    // Mapping of ALT-XXX of codepage 437 to unicode
    m_altMap[130] = 0x00E9;
    m_altMap[225] = 0x00DF;
    m_altMap[135] = 0x00E7;
    m_altMap[138] = 0x00E8;
    m_altMap[149] = 0x00F2;
    m_altMap[133] = 0x00E0;
    m_altMap[151] = 0x00F9;
    m_altMap[129] = 0x00FC;
    m_altMap[134] = 0x00E5;
    m_altMap[148] = 0x00F6;
    m_altMap[162] = 0x00F3;
    m_altMap[164] = 0x00F1;
    m_altMap[145] = 0x00E6;
    m_altMap[132] = 0x00E4;

    // Mapping of keypad scancodes to their numeric values
    m_keyPadMap[82] = 0;
    m_keyPadMap[79] = 1;
    m_keyPadMap[80] = 2;
    m_keyPadMap[81] = 3;
    m_keyPadMap[75] = 4;
    m_keyPadMap[76] = 5;
    m_keyPadMap[77] = 6;
    m_keyPadMap[71] = 7;
    m_keyPadMap[72] = 8;
    m_keyPadMap[73] = 9;

    m_initialized = true;
  }

  bool ret = false;
  XBMC_Event event = {0};
  while (1)
  {
    event = m_devices.ReadEvent();

    // Wait for first left ALT press, then capture numbers pressed, then wait for left ALT release.
    // Translate the number into a unicode value. This is to support latin characters.
    if (event.key.keysym.sym == XBMCK_LALT && event.type == XBMC_KEYDOWN)
    {
      accumlate = true;
      number = 0;
    }

    if (accumlate && (event.type == XBMC_KEYDOWN))
    {
      int keyPad = m_keyPadMap.at(event.key.keysym.scancode);
      number = number * 10 + keyPad;
    }

    if (accumlate && event.key.keysym.sym == XBMCK_LALT && event.type == XBMC_KEYUP)
    {
      accumlate = false;
      event.type = XBMC_KEYDOWN;
      event.key.keysym.sym = XBMCK_UNKNOWN;;

      if (m_altMap.find(number) != m_altMap.end())
      {
        event.key.keysym.unicode = m_altMap[number];
      }
      else
      {
        event.key.keysym.unicode = number;
      }
    }

    if ((event.type != XBMC_NOEVENT) && !accumlate)
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

#endif
