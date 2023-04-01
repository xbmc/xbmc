/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"
#include "windowing/WinEvents.h"
#include "windowing/XBMC_events.h"

#include <memory>

struct CWinEventsOSXImplWrapper;
@class NSEvent;

class CWinEventsOSX : public IWinEvents, public CThread
{
public:
  CWinEventsOSX();
  ~CWinEventsOSX();

  void MessagePush(XBMC_Event* newEvent);
  bool MessagePump();
  size_t GetQueueSize();

  void enableInputEvents();
  void disableInputEvents();

  void signalMouseEntered();
  void signalMouseExited();
  void SendInputEvent(NSEvent* nsEvent);

private:
  std::unique_ptr<CWinEventsOSXImplWrapper> m_eventsImplWrapper;
};
