/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsOSX.h"

#import "WinEventsOSXImpl.h"

struct CWinEventsOSXImplWrapper
{
  CWinEventsOSXImpl* callbackClass;
};

CWinEventsOSX::CWinEventsOSX() : CThread("CWinEventsOSX")
{
  m_eventsImplWrapper = std::make_unique<CWinEventsOSXImplWrapper>();
  m_eventsImplWrapper->callbackClass = [CWinEventsOSXImpl new];
  Create();
}

CWinEventsOSX::~CWinEventsOSX()
{
  m_bStop = true;
  StopThread(true);
}

void CWinEventsOSX::MessagePush(XBMC_Event* newEvent)
{
  [m_eventsImplWrapper->callbackClass MessagePush:newEvent];
}

size_t CWinEventsOSX::GetQueueSize()
{
  return [m_eventsImplWrapper->callbackClass GetQueueSize];
}

bool CWinEventsOSX::MessagePump()
{
  return [m_eventsImplWrapper->callbackClass MessagePump];
}

void CWinEventsOSX::enableInputEvents()
{
  return [m_eventsImplWrapper->callbackClass enableInputEvents];
}

void CWinEventsOSX::disableInputEvents()
{
  return [m_eventsImplWrapper->callbackClass disableInputEvents];
}

void CWinEventsOSX::signalMouseEntered()
{
  return [m_eventsImplWrapper->callbackClass signalMouseEntered];
}

void CWinEventsOSX::signalMouseExited()
{
  return [m_eventsImplWrapper->callbackClass signalMouseExited];
}

void CWinEventsOSX::SendInputEvent(NSEvent* nsEvent)
{
  [m_eventsImplWrapper->callbackClass ProcessInputEvent:nsEvent];
}
