/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibInputTouch.h"

#include "ServiceBroker.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

static inline CPoint GetPos(libinput_event_touch *e)
{
  const double x = libinput_event_touch_get_x_transformed(e, CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth());
  const double y = libinput_event_touch_get_y_transformed(e, CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());

  CLog::LogF(LOGDEBUG, "CLibInputTouch: x: {:f} y: {:f}", x, y);

  return CPoint(x, y);
}

CLibInputTouch::CLibInputTouch()
{
  m_points.reserve(2);
  CGenericTouchInputHandler::GetInstance().RegisterHandler(&CGenericTouchActionHandler::GetInstance());
}

void CLibInputTouch::CheckSlot(int slot)
{
  if (slot + 1 > static_cast<int>(m_points.size()))
    m_points.resize(slot + 1, std::make_pair(TouchInputUnchanged, CPoint(0, 0)));
}

TouchInput CLibInputTouch::GetEvent(int slot)
{
  CheckSlot(slot);
  return m_points.at(slot).first;
}

void CLibInputTouch::SetEvent(int slot, TouchInput event)
{
  CheckSlot(slot);
  m_points.at(slot).first = event;
}

void CLibInputTouch::SetPosition(int slot, CPoint point)
{
  CheckSlot(slot);
  m_points.at(slot).second = point;
}

void CLibInputTouch::ProcessTouchDown(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_seat_slot(e);

  SetPosition(slot, GetPos(e));
  SetEvent(slot, TouchInputDown);
  CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input down");
}

void CLibInputTouch::ProcessTouchMotion(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_seat_slot(e);
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  SetPosition(slot, GetPos(e));

  if (GetEvent(slot) != TouchInputDown)
    SetEvent(slot, TouchInputMove);
  CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input move");

  CGenericTouchInputHandler::GetInstance().UpdateTouchPointer(slot, GetX(slot), GetY(slot), nanotime);
}

void CLibInputTouch::ProcessTouchUp(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_seat_slot(e);

  SetEvent(slot, TouchInputUp);
  CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input up");
}

void CLibInputTouch::ProcessTouchCancel(libinput_event_touch *e)
{
  int slot = libinput_event_touch_get_seat_slot(e);
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input cancel");
  CGenericTouchInputHandler::GetInstance().HandleTouchInput(TouchInputAbort, GetX(slot), GetY(slot), nanotime, slot);
}

void CLibInputTouch::ProcessTouchFrame(libinput_event_touch *e)
{
  uint64_t nanotime = libinput_event_touch_get_time_usec(e) * 1000LL;

  for (size_t slot = 0; slot < m_points.size(); ++slot)
  {
    CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input frame: event {}", GetEvent(slot));
    CLog::LogF(LOGDEBUG, "CLibInputTouch: touch input frame: slot {}", slot);
    CGenericTouchInputHandler::GetInstance().HandleTouchInput(GetEvent(slot), GetX(slot), GetY(slot), nanotime, slot);
    SetEvent(slot, TouchInputUnchanged);
  }
}

