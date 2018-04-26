/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://kodi.tv
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

#include "LibInputPointer.h"

#include "AppInboundProtocol.h"
#include "input/mouse/MouseStat.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <algorithm>
#include <linux/input.h>
#include <string.h>

void CLibInputPointer::ProcessButton(libinput_event_pointer *e)
{
  const uint32_t b = libinput_event_pointer_get_button(e);
  const bool pressed = libinput_event_pointer_get_button_state(e) == LIBINPUT_BUTTON_STATE_PRESSED;
  unsigned char xbmc_button = 0;

  switch (b)
  {
    case BTN_LEFT:
      xbmc_button = XBMC_BUTTON_LEFT;
      break;
    case BTN_RIGHT:
      xbmc_button = XBMC_BUTTON_RIGHT;
      break;
    case BTN_MIDDLE:
      xbmc_button = XBMC_BUTTON_MIDDLE;
      break;
    case BTN_SIDE:
      xbmc_button = XBMC_BUTTON_X1;
      break;
    case BTN_EXTRA:
      xbmc_button = XBMC_BUTTON_X2;
       break;
    case BTN_FORWARD:
      xbmc_button = XBMC_BUTTON_X3;
      break;
    case BTN_BACK:
      xbmc_button = XBMC_BUTTON_X4;
      break;
    default:
      break;
  }

  XBMC_Event event;
  memset(&event, 0, sizeof(event));

  event.type = pressed ? XBMC_MOUSEBUTTONDOWN : XBMC_MOUSEBUTTONUP;
  event.button.button = xbmc_button;
  event.button.x = static_cast<uint16_t>(m_pos.X);
  event.button.y = static_cast<uint16_t>(m_pos.Y);

  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.type: %i", __FUNCTION__, event.type);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.button.button: %i", __FUNCTION__, event.button.button);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.button.x: %i", __FUNCTION__, event.button.x);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.button.y: %i", __FUNCTION__, event.button.y);

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);
}

void CLibInputPointer::ProcessMotion(libinput_event_pointer *e)
{
  const double dx = libinput_event_pointer_get_dx(e);
  const double dy = libinput_event_pointer_get_dy(e);

  m_pos.X += static_cast<int>(dx);
  m_pos.Y += static_cast<int>(dy);

  // limit the mouse to the screen width
  m_pos.X = std::min(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), m_pos.X);
  m_pos.X = std::max(0, m_pos.X);

  // limit the mouse to the screen height
  m_pos.Y = std::min(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(), m_pos.Y);
  m_pos.Y = std::max(0, m_pos.Y);

  XBMC_Event event;
  memset(&event, 0, sizeof(event));

  event.type = XBMC_MOUSEMOTION;
  event.motion.x = static_cast<uint16_t>(m_pos.X);
  event.motion.y = static_cast<uint16_t>(m_pos.Y);

  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.type: %i", __FUNCTION__, event.type);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.motion.x: %i", __FUNCTION__, event.motion.x);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.motion.y: %i", __FUNCTION__, event.motion.y);

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);
}

void CLibInputPointer::ProcessAxis(libinput_event_pointer *e)
{
  unsigned char scroll = 0;
  if (!libinput_event_pointer_has_axis(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
    return;

  const double v = libinput_event_pointer_get_axis_value(e, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
  if (v < 0)
    scroll = XBMC_BUTTON_WHEELUP;
  else
    scroll = XBMC_BUTTON_WHEELDOWN;

  XBMC_Event event;
  memset(&event, 0, sizeof(event));

  event.type = XBMC_MOUSEBUTTONDOWN;
  event.button.button = scroll;
  event.button.x = static_cast<uint16_t>(m_pos.X);
  event.button.y = static_cast<uint16_t>(m_pos.Y);

  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - scroll: %s", __FUNCTION__, scroll == XBMC_BUTTON_WHEELUP ? "up" : "down");
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.button.x: %i", __FUNCTION__, event.button.x);
  CLog::Log(LOGDEBUG, "CLibInputPointer::%s - event.button.y: %i", __FUNCTION__, event.button.y);

  std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
  if (appPort)
    appPort->OnEvent(event);

  event.type = XBMC_MOUSEBUTTONUP;

  if (appPort)
    appPort->OnEvent(event);
}
