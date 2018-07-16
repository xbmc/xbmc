/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  float m_white = 1.0;
  float m_black = 0.0;
};


