/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <android/input.h>

class CAndroidMouse
{

public:
  CAndroidMouse();
  virtual ~CAndroidMouse();
  bool onMouseEvent(AInputEvent* event);

protected:

private:
  void MouseMove(float x, float y);
  void MouseButton(float x, float y, int32_t type, int32_t buttons);
  void MouseWheel(float x, float y, float value);

private:
  int32_t m_lastButtonState;
};
