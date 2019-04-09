/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <list>
#include "WinEventsTVOS.h"
#include "input/InputManager.h"
#include "input/XBMC_vkeys.h"
#include "AppInboundProtocol.h"
#include "threads/CriticalSection.h"
#include "guilib/GUIWindowManager.h"
#include "ServiceBroker.h"
#include "utils/log.h"

static CCriticalSection g_inputCond;

static std::list<XBMC_Event> events;

CWinEventsTVOS::CWinEventsTVOS()
: CThread("CWinEventsTVOS")
{
    CLog::Log(LOGDEBUG, "CWinEventsAndroid::CWinEventsTVOS");
    Create();
}

CWinEventsTVOS::~CWinEventsTVOS()
{
    m_bStop = true;
    StopThread(true);
}

void CWinEventsTVOS::MessagePush(XBMC_Event *newEvent)
{
    CSingleLock lock(m_eventsCond);
    
    m_events.push_back(*newEvent);
}



size_t CWinEventsTVOS::GetQueueSize()
{
  CSingleLock lock(g_inputCond);
  return events.size();
}
