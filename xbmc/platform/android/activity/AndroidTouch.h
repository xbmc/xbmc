/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <android/input.h>
#include <math.h>

class CAndroidTouch
{

public:
  CAndroidTouch();
  virtual ~CAndroidTouch();
  bool onTouchEvent(AInputEvent* event);

protected:
  virtual void setDPI(uint32_t dpi);

private:
  uint32_t m_dpi;
};
