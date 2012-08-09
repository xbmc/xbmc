#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include <android/input.h>
#include <math.h>

#include "input/TouchInput.h"

class CAndroidTouch : protected ITouchHandler
{

public:
  CAndroidTouch();
  virtual ~CAndroidTouch();
  bool onTouchEvent(AInputEvent* event);

protected:
  virtual bool OnSingleTouchStart(float x, float y);

  virtual bool OnTouchGesturePanStart(float x, float y);
  virtual bool OnTouchGesturePan(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);
  virtual bool OnTouchGesturePanEnd(float x, float y, float offsetX, float offsetY, float velocityX, float velocityY);

  virtual void OnSingleTap(float x, float y);
  virtual void OnSingleLongPress(float x, float y);
  virtual void OnZoomPinch(float centerX, float centerY, float zoomFactor);

private:
  void XBMC_Touch(uint8_t type, uint8_t button, uint16_t x, uint16_t y);
  void XBMC_TouchGesture(int32_t action, float posX, float posY, float offsetX, float offsetY);
};
