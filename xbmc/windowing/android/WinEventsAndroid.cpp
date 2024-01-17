/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinEventsAndroid.h"

#include "ServiceBroker.h"
#include "application/AppInboundProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/keyboard/XBMC_vkeys.h"
#include "utils/log.h"

#include <mutex>

#define ALMOST_ZERO 0.125f
enum {
  EVENT_STATE_TEST,
  EVENT_STATE_HOLD,
  EVENT_STATE_REPEAT
};

/************************************************************************/
/************************************************************************/
static bool different_event(XBMC_Event &curEvent, XBMC_Event &newEvent)
{
  // different type
  if (curEvent.type != newEvent.type)
    return true;

  return false;
}

/************************************************************************/
/************************************************************************/
CWinEventsAndroid::CWinEventsAndroid()
: CThread("CWinEventsAndroid")
{
  CLog::Log(LOGDEBUG, "CWinEventsAndroid::CWinEventsAndroid");
  Create();
}

CWinEventsAndroid::~CWinEventsAndroid()
{
  m_bStop = true;
  StopThread(true);
}

void CWinEventsAndroid::MessagePush(XBMC_Event *newEvent)
{
  std::unique_lock<CCriticalSection> lock(m_eventsCond);

  m_events.push_back(*newEvent);
}

void CWinEventsAndroid::MessagePushRepeat(XBMC_Event *repeatEvent)
{
  std::unique_lock<CCriticalSection> lock(m_eventsCond);

  std::list<XBMC_Event>::iterator itt;
  for (itt = m_events.begin(); itt != m_events.end(); ++itt)
  {
    // we have events pending, if we we just
    // repush, we might push the repeat event
    // in back of a canceling non-active event.
    // do not repush if pending are different event.
    if (different_event(*itt, *repeatEvent))
      return;
  }

  // is a repeat, push it
  m_events.push_back(*repeatEvent);
}

bool CWinEventsAndroid::MessagePump()
{
  bool ret = false;

  // Do not always loop, only pump the initial queued count events. else if ui keep pushing
  // events the loop won't finish then it will block xbmc main message loop.
  for (size_t pumpEventCount = GetQueueSize(); pumpEventCount > 0; --pumpEventCount)
  {
    // Pop up only one event per time since in App::OnEvent it may init modal dialog which init
    // deeper message loop and call the deeper MessagePump from there.
    XBMC_Event pumpEvent;
    {
      std::unique_lock<CCriticalSection> lock(m_eventsCond);
      if (m_events.empty())
        return ret;
      pumpEvent = m_events.front();
      m_events.pop_front();
    }

    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      ret |= appPort->OnEvent(pumpEvent);

    if (pumpEvent.type == XBMC_MOUSEBUTTONUP)
      CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
  }

  return ret;
}

size_t CWinEventsAndroid::GetQueueSize()
{
  std::unique_lock<CCriticalSection> lock(m_eventsCond);
  return m_events.size();
}

void CWinEventsAndroid::Process()
{
  uint32_t timeout = 10;
  uint32_t holdTimeout = 500;
  uint32_t repeatTimeout = 100;
  uint32_t repeatDuration = 0;

  XBMC_Event cur_event;
  int state = EVENT_STATE_TEST;
  while (!m_bStop)
  {
    // run a 10ms (timeout) wait cycle
    CThread::Sleep(std::chrono::milliseconds(timeout));

    std::unique_lock<CCriticalSection> lock(m_lasteventCond);

    switch(state)
    {
      default:
      case EVENT_STATE_TEST:
        // non-active event, eat it
        if (!m_lastevent.empty())
          m_lastevent.pop();
        break;

      case EVENT_STATE_HOLD:
        repeatDuration += timeout;
        if (!m_lastevent.empty())
        {
          if (different_event(cur_event, m_lastevent.front()))
          {
            // different event, cycle back to test
            state = EVENT_STATE_TEST;
            break;
          }

          // same event, eat it
          m_lastevent.pop();
        }

        if (repeatDuration >= holdTimeout)
        {
          CLog::Log(LOGDEBUG, "hold  ->repeat, size({}), repeatDuration({})", m_lastevent.size(),
                    repeatDuration);
          state = EVENT_STATE_REPEAT;
        }
        break;

      case EVENT_STATE_REPEAT:
        repeatDuration += timeout;
        if (!m_lastevent.empty())
        {
          if (different_event(cur_event, m_lastevent.front()))
          {
            // different event, cycle back to test
            state = EVENT_STATE_TEST;
            break;
          }

          // same event, eat it
          m_lastevent.pop();
        }

        if (repeatDuration >= holdTimeout)
        {
          // this is a repeat, push it
          MessagePushRepeat(&cur_event);
          // assuming holdTimeout > repeatTimeout,
          // just subtract the repeatTimeout
          // to get the next cycle time
          repeatDuration -= repeatTimeout;
        }
        break;
    }
  }
}
