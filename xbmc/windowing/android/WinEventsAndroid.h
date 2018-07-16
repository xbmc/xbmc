/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <list>
#include <queue>
#include <vector>
#include <string>

#include "threads/Event.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "windowing/WinEvents.h"

class CWinEventsAndroid : public IWinEvents, public CThread
{
public:
  CWinEventsAndroid();
 ~CWinEventsAndroid();

  void            MessagePush(XBMC_Event *newEvent);
  void            MessagePushRepeat(XBMC_Event *repeatEvent);
  bool            MessagePump();

private:
  size_t          GetQueueSize();

  // for CThread
  virtual void    Process();

  CCriticalSection             m_eventsCond;
  std::list<XBMC_Event>        m_events;

  CCriticalSection             m_lasteventCond;
  std::queue<XBMC_Event>       m_lastevent;
};

