/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowTestPattern.h"
#include "Application.h"
#include "GUIWindowManager.h"

#define NUM_PATTERNS 3

CGUIWindowTestPattern::CGUIWindowTestPattern(void)
    : CGUIWindow(WINDOW_TEST_PATTERN, "")
{
}

CGUIWindowTestPattern::~CGUIWindowTestPattern(void)
{}


bool CGUIWindowTestPattern::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PREVIOUS_MENU:
    {
      m_gWindowManager.PreviousWindow();
      return true;
    }
    break;

  case ACTION_MOVE_UP:
  case ACTION_MOVE_LEFT:
     m_pattern = m_pattern > 0 ? m_pattern-- : NUM_PATTERNS;
     break;

  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_RIGHT:
     m_pattern = (m_pattern + 1) % NUM_PATTERNS;
     break;
    
  }
  return CGUIWindow::OnAction(action); // base class to handle basic movement etc.
}

bool CGUIWindowTestPattern::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    m_pattern = 0;
    break;

  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowTestPattern::Render()
{
  int top = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.top;
  int bottom = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.bottom;
  int left = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.left;
  int right = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan.right;

  glDisable(GL_TEXTURE_2D);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  switch (m_pattern)
  {
    case 0:
      DrawVerticalLines(top, left, bottom, right);
      break;
 
    case 1:
      DrawHorizontalLines(top, left, bottom, right);
      break;

    case 2:
      DrawCheckers(top, left, bottom, right);
      break;
  }

  glEnable(GL_TEXTURE_2D);
  CGUIWindow::Render();
}

void CGUIWindowTestPattern::DrawVerticalLines(int top, int left, int bottom, int right)
{
  glBegin(GL_LINES);
  glColor3f(1, 1, 1);
  for (int i = left; i <= right; i += 2)
  {
    glVertex3i(i, top, 0);
    glVertex3i(i, bottom, 0);
  }
  glEnd();
}

void CGUIWindowTestPattern::DrawHorizontalLines(int top, int left, int bottom, int right)
{
  glBegin(GL_LINES);
  glColor3f(1, 1, 1);
  for (int i = top; i <= bottom; i += 2) 
  {
    glVertex3i(left, i, 0);
    glVertex3i(right, i, 0);
  }
  glEnd();
}

void CGUIWindowTestPattern::DrawCheckers(int top, int left, int bottom, int right)
{
  glBegin(GL_POINTS);
  glColor3f(1, 1, 1);
  for (int y = top; y <= bottom; y++) 
  {
    for (int x = left; x <= right; x += 2) 
    {
      if (y % 2 == 0)
        glVertex3i(x, y, 0);
      else
        glVertex3i(x+1, y, 0);
    }
  }
  glEnd();
}

