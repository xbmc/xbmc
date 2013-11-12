/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#pragma once

#ifndef WINDOW_EVENTS_ANDROID_H
#define WINDOW_EVENTS_ANDROID_H

#include <list>
#include <queue>
#include <vector>
#include <string>

#include "threads/Event.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "windowing/WinEvents.h"

typedef struct {
  int32_t id;
  std::string name;
} APP_InputDevice;

class CWinEventsAndroid : public IWinEvents, public CThread
{
public:
  CWinEventsAndroid();
 ~CWinEventsAndroid();

  void            MessagePush(XBMC_Event *newEvent);
  void            MessagePushRepeat(XBMC_Event *repeatEvent);
  bool            MessagePump();
  virtual size_t  GetQueueSize();

private:
  // for CThread
  virtual void    Process();

  CCriticalSection             m_eventsCond;
  std::list<XBMC_Event>        m_events;

  CCriticalSection             m_lasteventCond;
  std::queue<XBMC_Event>       m_lastevent;

  std::vector<APP_InputDevice> m_input_devices;
};

#endif // WINDOW_EVENTS_ANDROID_H

