/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "windowing/WinEvents.h"

#include <list>
#include <queue>
#include <string>
#include <vector>

class CWinEventsAndroid : public IWinEvents, public CThread
{
public:
  CWinEventsAndroid();
  ~CWinEventsAndroid() override;

  void            MessagePush(XBMC_Event *newEvent);
  void            MessagePushRepeat(XBMC_Event *repeatEvent);
  bool MessagePump() override;

private:
  size_t          GetQueueSize();

  // for CThread
  void Process() override;

  CCriticalSection             m_eventsCond;
  std::list<XBMC_Event>        m_events;

  CCriticalSection             m_lasteventCond;
  std::queue<XBMC_Event>       m_lastevent;
};

