#pragma once
/*
 * Copyright (C) 2014 Team Kodi
 * http://xbmc.org
 *
 * This Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kodi; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "JNIBase.h"

class CJNIDisplayMode : public CJNIBase
{
public:
  ~CJNIDisplayMode() {};
  CJNIDisplayMode(const jni::jhobject &object) : CJNIBase(object) {};

  int getModeId();
  int getPhysicalHeight();
  int getPhysicalWidth();
  float getRefreshRate();

protected:
  CJNIDisplayMode();
};

class CJNIDisplay : public CJNIBase
{
public:
  CJNIDisplay();
  CJNIDisplay(const jni::jhobject &object) : CJNIBase(object) {};
  ~CJNIDisplay() {};

  float getRefreshRate();
  std::vector<float> getSupportedRefreshRates();
  CJNIDisplayMode getMode();
  int getWidth();
  int getHeight();
  std::vector<CJNIDisplayMode> getSupportedModes();
};

typedef std::vector<CJNIDisplayMode> CJNIDisplayModes;
