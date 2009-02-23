#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindow.h"

class CGUIWindowTestPattern : public CGUIWindow
{
public:
  CGUIWindowTestPattern(void);
  virtual ~CGUIWindowTestPattern(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

private:
  void DrawVerticalLines(int top, int left, int bottom, int right);
  void DrawHorizontalLines(int top, int left, int bottom, int right);
  void DrawCheckers(int top, int left, int bottom, int right);
  void DrawBouncingRectangle(int top, int left, int bottom, int right);
  void DrawContrastBrightnessPattern(int top, int left, int bottom, int right);
  void DrawCircle(int originX, int originY, int radius);

  int m_pattern;
  int m_bounceX;
  int m_bounceY;
  int m_bounceDirectionX;
  int m_bounceDirectionY;
  int m_blinkFrame;
};

