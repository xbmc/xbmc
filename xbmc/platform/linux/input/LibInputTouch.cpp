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

#include "LibInputTouch.h"

#include "input/touch/generic/GenericTouchActionHandler.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

static inline CPoint GetPos(libinput_event_touch *e)
{
  const double x = libinput_event_touch_get_x_transformed(e, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth());
  const double y = libinput_event_touch_get_y_transformed(e, CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());

  CLog::Log(LOGDEBUG, "CLibInputTouch::%s - x: %f y: %f", __FUNCTION__, x, y);

  return CPoint(x, y);
}

CLibInputTouch::CLibInputTouch()
{
  CGenericTouchInputHandler::GetInstance().RegisterHandler(&CGenericTouchActionHandler::GetInstance());

  for (int i = 0; i < CGenericTouchInputHandler::MAX_POINTERS; i++)
    m_points.push_back(std::make_pair(TouchInputUnchanged, CPoint(0, 0)));
}

void CLibInputTouch::ProcessTouchDown(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_slot(e);

  m_points.at(slot).second = GetPos(e);
  m_points.at(slot).first = TouchInputDown;
  CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input down", __FUNCTION__);
}

void CLibInputTouch::ProcessTouchMotion(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_slot(e);
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  m_points.at(slot).second = GetPos(e);

  if (m_points.at(slot).first != TouchInputDown)
    m_points.at(slot).first = TouchInputMove;
  CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input move", __FUNCTION__);

  CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(slot, m_points.at(slot).second.x, m_points.at(slot).second.y, nanotime);
}

void CLibInputTouch::ProcessTouchUp(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_slot(e);

  m_points.at(slot).first = TouchInputUp;
  CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input up", __FUNCTION__);
}

void CLibInputTouch::ProcessTouchCancel(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_slot(e);
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input cancel", __FUNCTION__);
  CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputAbort, m_points.at(slot).second.x, m_points.at(slot).second.y, nanotime, slot);
}

void CLibInputTouch::ProcessTouchFrame(libinput_event_touch *e)
{
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  for (size_t slot = 0; slot < m_points.size(); ++slot)
  {
    CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input frame: event %i", __FUNCTION__, m_points.at(slot).first);
    CLog::Log(LOGDEBUG, "CLibInputTouch::%s - touch input frame: slot %i", __FUNCTION__, slot);
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(m_points.at(slot).first, m_points.at(slot).second.x, m_points.at(slot).second.y, nanotime, slot);
    m_points.at(slot).first = TouchInputUnchanged;
  }
}

