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

#include "GUIWindowTestPattern.h"
#include "ServiceBroker.h"
#include "input/Key.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "windowing/WinSystem.h"


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

CGUIWindowTestPattern::~CGUIWindowTestPattern(void) = default;


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
  m_renderRegion.SetRect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());

#ifndef HAS_DX
  if(CServiceBroker::GetWinSystem()->UseLimitedColor())
  {
    m_white = 235.0f / 255;
    m_black =  16.0f / 255;
  }
  else
#endif
  {
    m_white = 1.0f;
    m_black = 0.0f;
  }
}

void CGUIWindowTestPattern::Render()
{
  BeginRender();
  const RESOLUTION_INFO info = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

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

