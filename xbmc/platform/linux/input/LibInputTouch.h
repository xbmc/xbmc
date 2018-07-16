/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
