/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>

#include <libinput.h>

struct pos
{
  int X;
  int Y;
};

class CLibInputPointer
{
public:
  CLibInputPointer() = default;
  ~CLibInputPointer() = default;

  void ProcessButton(libinput_event_pointer *e);
  void ProcessMotion(libinput_event_pointer *e);
  void ProcessMotionAbsolute(libinput_event_pointer *e);
  void ProcessAxis(libinput_event_pointer *e);

  /*!
   * \brief Start the settle window in which motion is discarded
   *
   * Called when a pointing device appears. The length of the window comes from
   * input.pointersettletime, and a value of zero leaves motion untouched.
   */
  void DeviceAdded();

private:
  bool IsSettling() const;

  std::chrono::steady_clock::time_point m_settled;
  struct pos m_pos = { 0, 0 };
};
