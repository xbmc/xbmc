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

#pragma once

#include "input/touch/generic/GenericTouchInputHandler.h"
#include "utils/Geometry.h"

#include <libinput.h>
#include <vector>

struct libinput_event_touch;
struct libinput_device;

class CLibInputTouch
{
public:
  CLibInputTouch();
  void ProcessTouchDown(libinput_event_touch *e);
  void ProcessTouchMotion(libinput_event_touch *e);
  void ProcessTouchUp(libinput_event_touch *e);
  void ProcessTouchCancel(libinput_event_touch *e);
  void ProcessTouchFrame(libinput_event_touch *e);

private:
  void CheckSlot(int slot);
  TouchInput GetEvent(int slot);
  void SetEvent(int slot, TouchInput event);
  void SetPosition(int slot, CPoint point);
  int GetX(int slot) { return m_points.at(slot).second.x; }
  int GetY(int slot) { return m_points.at(slot).second.y; }

  std::vector<std::pair<TouchInput, CPoint>> m_points{std::make_pair(TouchInputUnchanged, CPoint(0, 0))};
};
