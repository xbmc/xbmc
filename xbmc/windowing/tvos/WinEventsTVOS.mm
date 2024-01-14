/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsTVOS.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include <list>
#include <mutex>

static CCriticalSection g_inputCond;

static std::list<XBMC_Event> events;

CWinEventsTVOS::CWinEventsTVOS() : CThread("CWinEventsTVOS")
{
  CLog::Log(LOGDEBUG, "CWinEventsTVOS::CWinEventsTVOS");
  Create();
}

CWinEventsTVOS::~CWinEventsTVOS()
{
  m_bStop = true;
  StopThread(true);
}

void CWinEventsTVOS::MessagePush(XBMC_Event* newEvent)
{
  std::unique_lock<CCriticalSection> lock(m_eventsCond);

  m_events.push_back(*newEvent);
}

size_t CWinEventsTVOS::GetQueueSize()
{
  std::unique_lock<CCriticalSection> lock(g_inputCond);
  return events.size();
}


bool CWinEventsTVOS::MessagePump()
{
  bool ret = false;
  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();

  // Do not always loop, only pump the initial queued count events. else if ui keep pushing
  // events the loop won't finish then it will block xbmc main message loop.
  for (size_t pumpEventCount = GetQueueSize(); pumpEventCount > 0; --pumpEventCount)
  {
    // Pop up only one event per time since in App::OnEvent it may init modal dialog which init
    // deeper message loop and call the deeper MessagePump from there.
    XBMC_Event pumpEvent;
    {
      std::unique_lock<CCriticalSection> lock(g_inputCond);
      if (events.empty())
        return ret;
      pumpEvent = events.front();
      events.pop_front();
    }

    if (appPort)
      ret = appPort->OnEvent(pumpEvent);
  }
  return ret;
}
