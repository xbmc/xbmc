/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIWindowTestPattern.h"
#include "input/Key.h"
#include "guilib/WindowIDs.h"
#include "windowing/WindowingFactory.h"


CGUIWindowTestPattern::CGUIWindowTestPattern(void)
    : CGUIWindow(WINDOW_TEST_PATTERN, "")
    , m_white(1.0)
    , m_black(0.0)
{
  m_pattern = 0;
  m_bounceX = 0;
  m_bounceY = 0;
  m_bounceDirectionX = 0;
  m_bounceDirectionY = 0;
  m_blinkFrame = 0;
  m_needsScaling = false;
}

CGUIWindowTestPattern::~CGUIWindowTestPattern(void)
{}


bool CGUIWindowTestPattern::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_MOVE_UP:
  case ACTION_MOVE_LEFT:
    m_pattern = m_pattern > 0 ? m_pattern - 1 : TEST_PATTERNS_COUNT - 1;
    SetInvalid();
    return true;

  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_RIGHT:
    m_pattern = (m_pattern + 1) % TEST_PATTERNS_COUNT;
    SetInvalid();
    return true;
  }
  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

bool CGUIWindowTestPattern::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    m_pattern = 0;
    m_bounceDirectionX = 1;
    m_bounceDirectionY = 1;
    m_bounceX = 0;
    m_bounceY = 0;
    m_blinkFrame = 0;
    break;

  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowTestPattern::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_pattern == 0 || m_pattern == 4)
    MarkDirtyRegion();
  CGUIWindow::Process(currentTime, dirtyregions);
  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());


  if(g_Windowing.UseLimitedColor())
  {
    m_white = 235.0f / 255;
    m_black =  16.0f / 255;
  }
  else
  {
    m_white = 1.0f;
    m_black = 0.0f;
  }
}

void CGUIWindowTestPattern::Render()
{
  BeginRender();
  const RESOLUTION_INFO info = g_graphicsContext.GetResInfo();

  int top    = info.Overscan.top;
  int bottom = info.Overscan.bottom;
  int left   = info.Overscan.left;
  int right  = info.Overscan.right;

  switch (m_pattern)
  {
    case 0:
      DrawContrastBrightnessPattern(top, left, bottom, right);
      break;

    case 1:
      DrawVerticalLines(top, left, bottom, right);
      break;

    case 2:
      DrawHorizontalLines(top, left, bottom, right);
      break;

    case 3:
      DrawCheckers(top, left, bottom, right);
      break;

    case 4:
      DrawBouncingRectangle(top, left, bottom, right);
      break;
  }

  EndRender();

  CGUIWindow::Render();
}

