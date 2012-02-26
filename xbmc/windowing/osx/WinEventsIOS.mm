/*
*      Copyright (C) 2010 Team XBMC
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
#include "WinEvents.h"
#include "WinEventsIOS.h"
#include "XBMC_vkeys.h"
#include "Application.h"
#include "WindowingFactory.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

static CCriticalSection g_inputCond;

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

static std::vector<XBMC_Event> events;

void CWinEventsIOS::DeInit()
{
}

void CWinEventsIOS::Init()
{
}

void CWinEventsIOS::MessagePush(XBMC_Event *newEvent)
{
  CSingleLock lock(g_inputCond);

  events.push_back(*newEvent);
}

bool CWinEventsIOS::MessagePump()
{
  bool ret = false;
  bool gotEvent = false;
  XBMC_Event pumpEvent;

  CSingleLock lock(g_inputCond);
  for (vector<XBMC_Event>::iterator it = events.begin(); it!=events.end(); ++it)
  {
    memcpy(&pumpEvent, (XBMC_Event *)&*it, sizeof(XBMC_Event));
    events.erase (events.begin(),events.begin()+1);
    gotEvent = true;
    break;
  }
  lock.Leave();

  if (gotEvent)
  {
    if (pumpEvent.type == XBMC_USEREVENT)
    {
      // On ATV2, we push in events as a XBMC_USEREVENT,
      // the user.code will be the keyID to translate using joystick.AppleRemote.xml
      std::string joystickName = "AppleRemote";
      bool isAxis = false;
      float fAmount = 1.0;
      unsigned short wKeyID = pumpEvent.user.code;

      CLog::Log(LOGDEBUG,"CWinEventsIOS: Button press keyID = %i", wKeyID);
      ret |= g_application.ProcessJoystickEvent(joystickName, wKeyID, isAxis, fAmount);
    }
    else
      ret |= g_application.OnEvent(pumpEvent);
  }

  return ret;
}
