#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *      Test patterns designed by Ofer LaOr - hometheater.co.il
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

#include "settings/windows/GUIWindowTestPattern.h"

class CGUIWindowTestPatternGL : public CGUIWindowTestPattern
{
public:
  CGUIWindowTestPatternGL(void);
  ~CGUIWindowTestPatternGL(void) override;

private:
  void DrawVerticalLines(int top, int left, int bottom, int right) override;
  void DrawHorizontalLines(int top, int left, int bottom, int right) override;
  void DrawCheckers(int top, int left, int bottom, int right) override;
  void DrawBouncingRectangle(int top, int left, int bottom, int right) override;
  void DrawContrastBrightnessPattern(int top, int left, int bottom, int right) override;
  void DrawCircle(int originX, int originY, int radius) override;
  void BeginRender() override;
  void EndRender() override;  
};

