/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "AndroidTouch.h"
#include "AndroidKey.h"
#include "AndroidMouse.h"
#include "AndroidJoyStick.h"

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

  virtual void setDPI(uint32_t dpi) { CAndroidTouch::setDPI(dpi); }
};
