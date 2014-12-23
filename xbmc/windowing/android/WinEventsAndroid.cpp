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

#include "system.h"

#include "WinEventsAndroid.h"

#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "input/XBMC_vkeys.h"
#include "input/SDLJoystick.h"
#include "utils/log.h"
#include "windowing/WindowingFactory.h"

#include "android/jni/View.h"

#define DEBUG_MESSAGEPUMP 0

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

  // Hat
  if(newEvent.type == XBMC_JOYHATMOTION)
    return (curEvent.jhat.value != newEvent.jhat.value);

  // different axis
  if (curEvent.jaxis.axis != newEvent.jaxis.axis)
    return true;

  // different axis direction (handles -1 vs 1)
  if (std::signbit(curEvent.jaxis.fvalue) != std::signbit(newEvent.jaxis.fvalue))
    return true;

  // different axis value (handles 0 vs 1 or -1)
  if ((fabs(curEvent.jaxis.fvalue) < ALMOST_ZERO) != (fabs(newEvent.jaxis.fvalue) < ALMOST_ZERO))
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
  CSingleLock lock(m_eventsCond);

  m_events.push_back(*newEvent);
  #if DEBUG_MESSAGEPUMP
  CLog::Log(LOGDEBUG, "push     event, size(%d), fvalue(%f)",
    m_events.size(), newEvent->jaxis.fvalue);
  #endif
}

void CWinEventsAndroid::MessagePushRepeat(XBMC_Event *repeatEvent)
{
  CSingleLock lock(m_eventsCond);

  std::list<XBMC_Event>::iterator itt;
  for (itt = m_events.begin(); itt != m_events.end(); ++itt)
  {
    // we have events pending, if we we just
    // repush, we might push the repeat event
    // in back of a canceling non-active event.
    // do not repush if pending are different event.
    if (different_event(*itt, *repeatEvent))
    {
      #if DEBUG_MESSAGEPUMP
      CLog::Log(LOGDEBUG, "repush    skip, size(%d), fvalue(%f)",
        m_events.size(), repeatEvent->jaxis.fvalue);
      #endif
      return;
    }
  }
  // is a repeat, push it
  m_events.push_back(*repeatEvent);
  #if DEBUG_MESSAGEPUMP
  CLog::Log(LOGDEBUG, "repush   event, size(%d), fvalue(%f)",
    m_events.size(), repeatEvent->jaxis.fvalue);
  #endif
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
      CSingleLock lock(m_eventsCond);
      if (m_events.empty())
        return ret;
      pumpEvent = m_events.front();
      m_events.pop_front();
      #if DEBUG_MESSAGEPUMP
      CLog::Log(LOGDEBUG, "  pop    event, size(%d), fvalue(%f)",
        m_events.size(), pumpEvent.jaxis.fvalue);
      #endif
    }

    if ((pumpEvent.type == XBMC_JOYBUTTONUP)   ||
        (pumpEvent.type == XBMC_JOYBUTTONDOWN) ||
        (pumpEvent.type == XBMC_JOYAXISMOTION) ||
        (pumpEvent.type == XBMC_JOYHATMOTION))
    {
      int             item;
      int             type;
      uint32_t        holdTime;
      APP_InputDevice input_device;
      float           amount = 1.0f;
      short           input_type;

      type = pumpEvent.type;
      switch (type)
      {
        case XBMC_JOYAXISMOTION:
          // The typical joystick keymap xml has the following where 'id' is the axis
          //  and 'limit' is which action to choose (ie. Up or Down).
          //  <axis id="5" limit="-1">Up</axis>
          //  <axis id="5" limit="+1">Down</axis>
          // One would think that limits is in reference to fvalue but
          // it is really in reference to id :) The sign of item passed
          // into ProcessJoystickEvent indicates the action mapping.
          item = pumpEvent.jaxis.axis;
          if (fabs(pumpEvent.jaxis.fvalue) < ALMOST_ZERO)
            amount = 0.0f;
          else if (pumpEvent.jaxis.fvalue  < 0.0f)
            item = -item;
          holdTime = 0;
          input_device.id = pumpEvent.jaxis.which;
          input_type = JACTIVE_AXIS;
          break;

        case XBMC_JOYHATMOTION:
          item = pumpEvent.jhat.hat | 0xFFF00000 | (pumpEvent.jhat.value<<16);
          holdTime = 0;
          input_device.id = pumpEvent.jhat.which;
          input_type = JACTIVE_HAT;
          break;

        case XBMC_JOYBUTTONUP:
        case XBMC_JOYBUTTONDOWN:
          item = pumpEvent.jbutton.button;
          holdTime = pumpEvent.jbutton.holdTime;
          input_device.id = pumpEvent.jbutton.which;
          input_type = JACTIVE_BUTTON;
          break;
      }

      // look for device name in our inputdevice cache
      // so we can lookup and match the right joystick.xxx.xml
      for (size_t i = 0; i < m_input_devices.size(); i++)
      {
        if (m_input_devices[i].id == input_device.id)
          input_device.name = m_input_devices[i].name;
      }
      if (input_device.name.empty())
      {
        // not in inputdevice cache, fetch and cache it.
        CJNIViewInputDevice view_input_device = CJNIViewInputDevice::getDevice(input_device.id);
        input_device.name = view_input_device.getName();
        CLog::Log(LOGDEBUG, "CWinEventsAndroid::MessagePump:caching  id(%d), device(%s)",
          input_device.id, input_device.name.c_str());
        m_input_devices.push_back(input_device);
      }

      if (type == XBMC_JOYAXISMOTION || type == XBMC_JOYHATMOTION)
      {
        // Joystick autorepeat -> only handle axis
        CSingleLock lock(m_lasteventCond);
        m_lastevent.push(pumpEvent);
      }

      if (fabs(amount) >= ALMOST_ZERO)
      {
        ret |= g_application.ProcessJoystickEvent(input_device.name,
          item, input_type, amount, holdTime);
      }
    }
    else
    {
      ret |= g_application.OnEvent(pumpEvent);
    }

    if (pumpEvent.type == XBMC_MOUSEBUTTONUP)
      g_windowManager.SendMessage(GUI_MSG_UNFOCUS_ALL, 0, 0, 0, 0);
  }

  return ret;
}

size_t CWinEventsAndroid::GetQueueSize()
{
  CSingleLock lock(m_eventsCond);
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
    Sleep(timeout);

    CSingleLock lock(m_lasteventCond);

    switch(state)
    {
      default:
      case EVENT_STATE_TEST:
        // check for axis action events
        if (!m_lastevent.empty())
        {
          if ((m_lastevent.front().type == XBMC_JOYAXISMOTION && fabs(m_lastevent.front().jaxis.fvalue) >= ALMOST_ZERO)
              || (m_lastevent.front().type == XBMC_JOYHATMOTION && m_lastevent.front().jhat.value > XBMC_HAT_CENTERED))
          {
            // new active event
            cur_event = m_lastevent.front();
            #if DEBUG_MESSAGEPUMP
            CLog::Log(LOGDEBUG, "test   -> hold, size(%d), fvalue(%f)",
              m_lastevent.size(), m_lastevent.front().jaxis.fvalue);
            #endif
            m_lastevent.pop();
            repeatDuration = 0;
            state = EVENT_STATE_HOLD;
            break;
          }
          #if DEBUG_MESSAGEPUMP
          CLog::Log(LOGDEBUG, "munch     test, size(%d), fvalue(%f)",
            m_lastevent.size(), m_lastevent.front().jaxis.fvalue);
          #endif
          // non-active event, eat it
          m_lastevent.pop();
        }
        break;

      case EVENT_STATE_HOLD:
        repeatDuration += timeout;
        if (!m_lastevent.empty())
        {
          if (different_event(cur_event, m_lastevent.front()))
          {
            // different axis event, cycle back to test
            state = EVENT_STATE_TEST;
            break;
          }
          #if DEBUG_MESSAGEPUMP
          CLog::Log(LOGDEBUG, "munch     hold, size(%d), fvalue(%f)",
            m_lastevent.size(), m_lastevent.front().jaxis.fvalue);
          #endif
          // same axis event, eat it
          m_lastevent.pop();
        }
        if (repeatDuration >= holdTimeout)
        {
          CLog::Log(LOGDEBUG, "hold  ->repeat, size(%d), repeatDuration(%d)", m_lastevent.size(), repeatDuration);
          state = EVENT_STATE_REPEAT;
        }
        break;

      case EVENT_STATE_REPEAT:
        repeatDuration += timeout;
        if (!m_lastevent.empty())
        {
          if (different_event(cur_event, m_lastevent.front()))
          {
            // different axis event, cycle back to test
            state = EVENT_STATE_TEST;
            #if DEBUG_MESSAGEPUMP
            CLog::Log(LOGDEBUG, "repeat->  test, size(%d), fvalue(%f)",
              m_lastevent.size(), m_lastevent.front().jaxis.fvalue);
            #endif
            break;
          }
          #if DEBUG_MESSAGEPUMP
          CLog::Log(LOGDEBUG, "munch   repeat, size(%d), fvalue(%f)",
            m_lastevent.size(), m_lastevent.front().jaxis.fvalue);
          #endif
          // same axis event, eat it
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
