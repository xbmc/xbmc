/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "windowing/WinEvents.h"
#include "windowing/XBMC_events.h"

#include <deque>
#include <mutex>

class CWinEventsWasm : public IWinEvents
{
public:
  CWinEventsWasm();
  ~CWinEventsWasm() override;

  bool MessagePump() override;

  void MessagePush(const XBMC_Event& newEvent);

private:
  void ProcessProxyCallbacks();
  bool DispatchQueuedEvents();

  std::mutex m_mutex;
  std::deque<XBMC_Event> m_events;
};
