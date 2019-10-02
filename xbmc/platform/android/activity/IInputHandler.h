/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AndroidJoyStick.h"
#include "AndroidKey.h"
#include "AndroidMouse.h"
#include "AndroidTouch.h"

class IInputHandler
: public CAndroidKey
, public CAndroidMouse
, public CAndroidTouch
, public CAndroidJoyStick
{
public:
  IInputHandler()
  : CAndroidKey()
  , CAndroidMouse()
  , CAndroidTouch()
  , CAndroidJoyStick()
  {}

  void setDPI(uint32_t dpi) override { CAndroidTouch::setDPI(dpi); }
};
