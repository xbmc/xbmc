#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIWindow.h"

#define TEST_PATTERNS_COUNT 5
#define TEST_PATTERNS_BOUNCE_SQUARE_SIZE 100
#define TEST_PATTERNS_BLINK_CYCLE 100

class CGUIWindowTestPattern : public CGUIWindow
{
public:
  CGUIWindowTestPattern(void);
  ~CGUIWindowTestPattern(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void Render() override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

protected:
  virtual void DrawVerticalLines(int top, int left, int bottom, int right) = 0;
  virtual void DrawHorizontalLines(int top, int left, int bottom, int right) = 0;
  virtual void DrawCheckers(int top, int left, int bottom, int right) = 0;
  virtual void DrawBouncingRectangle(int top, int left, int bottom, int right) = 0;
  virtual void DrawContrastBrightnessPattern(int top, int left, int bottom, int right) = 0;
  virtual void DrawCircle(int originX, int originY, int radius) = 0;
  virtual void BeginRender() = 0;
  virtual void EndRender() = 0;

  int m_pattern;
  int m_bounceX;
  int m_bounceY;
  int m_bounceDirectionX;
  int m_bounceDirectionY;
  int m_blinkFrame;

  float m_white;
  float m_black;
};


